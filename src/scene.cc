/*
 * scene.h 구현. glTF 파서(fastgltf) 의존은 이 파일에만 둔다.
 * - 구현할 것: GPU 업로드, PNG 디코딩, 텍스처 색 공간(sRGB/Linear) 지정, draw 목록.
 */
#include "scene.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

namespace RD {

GlbProbe probeGlb(const std::string &path) {
    GlbProbe probe;
    std::error_code ec;
    probe.fileSize = std::filesystem::file_size(path, ec);
    if(ec) {
        probe.error = "cannot open '" + path + "'";
        return probe;
    }

    std::ifstream file(path, std::ios::binary);
    char header[12] = {};
    file.read(header, sizeof(header));
    if(file.gcount() < static_cast<std::streamsize>(sizeof(header)) || std::memcmp(header, "glTF", 4) != 0) {
        // Git LFS 포인터는 "version https://git-lfs.github.com/spec/v1"로 시작하는 텍스트다.
        std::ifstream text(path);
        std::string firstLine;
        std::getline(text, firstLine);
        probe.lfsPointer = firstLine.rfind("version https://git-lfs", 0) == 0;
        probe.error = probe.lfsPointer ? "'" + path + "' is a Git LFS pointer, not the real GLB (run git lfs pull)"
                                       : "'" + path + "' is not a GLB file";
        return probe;
    }

    uint32_t version = 0, length = 0;
    std::memcpy(&version, header + 4, 4);
    std::memcpy(&length, header + 8, 4);
    probe.version = version;
    probe.declaredLength = length;
    if(version != 2) {
        probe.error = "unsupported GLB version " + std::to_string(version);
        return probe;
    }
    if(length != probe.fileSize) {
        probe.error = "GLB length mismatch (header " + std::to_string(length) + ", file " +
                      std::to_string(probe.fileSize) + ")";
        return probe;
    }
    probe.ok = true;
    return probe;
}

bool SceneManager::load(const std::string &path, std::string &error) {
    GlbProbe probe = probeGlb(path);
    if(!probe.ok) {
        error = probe.error;
        return false;
    }

    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    if(data.error() != fastgltf::Error::None) {
        error = "fastgltf: " + std::string(fastgltf::getErrorMessage(data.error()));
        return false;
    }

    fastgltf::Parser parser(fastgltf::Extensions::KHR_texture_transform |
                            fastgltf::Extensions::KHR_materials_emissive_strength |
                            fastgltf::Extensions::KHR_mesh_quantization);
    auto asset = parser.loadGltfBinary(data.get(), std::filesystem::path(path).parent_path());
    if(asset.error() != fastgltf::Error::None) {
        error = "fastgltf: " + std::string(fastgltf::getErrorMessage(asset.error()));
        return false;
    }
    const fastgltf::Asset &gltf = asset.get();

    _audit = AssetAudit{};
    _audit.nodes = gltf.nodes.size();
    _audit.meshes = gltf.meshes.size();
    _audit.materials = gltf.materials.size();
    _audit.images = gltf.images.size();
    _audit.textures = gltf.textures.size();
    for(const auto &extension : gltf.extensionsUsed)
        _audit.extensionsUsed.emplace_back(extension);
    for(const fastgltf::Material &material : gltf.materials) {
        if(material.alphaMode == fastgltf::AlphaMode::Blend)
            ++_audit.blend;
        else if(material.alphaMode == fastgltf::AlphaMode::Mask)
            ++_audit.mask;
        else
            ++_audit.opaque;
    }

    // 정점 위치로 bounds를 구한다(노드 변환은 TODO: 지금은 메시 공간 bounds).
    float lo[3] = {INFINITY, INFINITY, INFINITY};
    float hi[3] = {-INFINITY, -INFINITY, -INFINITY};
    for(const fastgltf::Mesh &mesh : gltf.meshes) {
        for(const fastgltf::Primitive &primitive : mesh.primitives) {
            ++_audit.primitives;
            if(primitive.indicesAccessor.has_value())
                _audit.indices += gltf.accessors[*primitive.indicesAccessor].count;

            auto position = primitive.findAttribute("POSITION");
            if(position == primitive.attributes.end())
                continue;
            const fastgltf::Accessor &accessor = gltf.accessors[position->accessorIndex];
            _audit.vertices += accessor.count;
            fastgltf::iterateAccessor<fastgltf::math::fvec3>(gltf, accessor, [&](fastgltf::math::fvec3 p) {
                for(int i = 0; i < 3; ++i) {
                    lo[i] = std::min(lo[i], p[i]);
                    hi[i] = std::max(hi[i], p[i]);
                }
            });
        }
    }

    _view = SceneView{};
    _view.drawCount = _audit.primitives;
    if(_audit.vertices > 0)
        for(int i = 0; i < 3; ++i) {
            _view.boundsMin[i] = lo[i];
            _view.boundsMax[i] = hi[i];
        }
    return true;
}

std::string SceneManager::auditText() const {
    std::ostringstream out;
    out << "asset audit: nodes " << _audit.nodes << ", meshes " << _audit.meshes << ", primitives "
        << _audit.primitives << ", materials " << _audit.materials << " (opaque " << _audit.opaque << ", mask "
        << _audit.mask << ", blend " << _audit.blend << "), images " << _audit.images << ", vertices "
        << _audit.vertices << ", indices " << _audit.indices;
    if(!_audit.extensionsUsed.empty()) {
        out << ", extensions";
        for(const std::string &extension : _audit.extensionsUsed)
            out << " " << extension;
    }
    if(_audit.blend > 0)
        out << "\n  warning: alpha BLEND materials are not supported by the deferred path yet";
    return out.str();
}

static void lookAt(const float eye[3], const float target[3], float out[16]) {
    float f[3] = {target[0] - eye[0], target[1] - eye[1], target[2] - eye[2]};
    float fl = std::sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    for(float &v : f)
        v /= fl;
    const float up[3] = {0, 1, 0};
    float s[3] = {f[1] * up[2] - f[2] * up[1], f[2] * up[0] - f[0] * up[2], f[0] * up[1] - f[1] * up[0]};
    float sl = std::sqrt(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
    for(float &v : s)
        v /= sl;
    float u[3] = {s[1] * f[2] - s[2] * f[1], s[2] * f[0] - s[0] * f[2], s[0] * f[1] - s[1] * f[0]};
    // 열 우선(column-major), 오른손 좌표계
    float m[16] = {s[0], u[0], -f[0], 0, s[1], u[1], -f[1], 0, s[2], u[2], -f[2], 0,
                   -(s[0] * eye[0] + s[1] * eye[1] + s[2] * eye[2]),
                   -(u[0] * eye[0] + u[1] * eye[1] + u[2] * eye[2]),
                   f[0] * eye[0] + f[1] * eye[1] + f[2] * eye[2], 1};
    std::memcpy(out, m, sizeof(m));
}

CameraFrame SceneManager::defaultCamera(float aspect) const {
    CameraFrame camera{};
    float center[3], extent = 0;
    for(int i = 0; i < 3; ++i) {
        center[i] = (_view.boundsMin[i] + _view.boundsMax[i]) * 0.5f;
        extent = std::max(extent, _view.boundsMax[i] - _view.boundsMin[i]);
    }
    float eye[3] = {center[0] + extent * 0.4f, center[1] + extent * 0.1f, center[2]};
    lookAt(eye, center, camera.view);

    // Vulkan clip space(depth 0..1, y 아래 방향) 원근 투영, fovY 60도.
    float fovY = 60.0f * 3.14159265f / 180.0f, zn = 0.05f, zf = std::max(extent * 4.0f, 10.0f);
    float t = 1.0f / std::tan(fovY * 0.5f);
    float p[16] = {t / aspect, 0, 0, 0, 0, -t, 0, 0, 0, 0, zf / (zn - zf), -1, 0, 0, zn * zf / (zn - zf), 0};
    std::memcpy(camera.proj, p, sizeof(p));
    camera.position[0] = eye[0];
    camera.position[1] = eye[1];
    camera.position[2] = eye[2];
    camera.position[3] = 1;
    return camera;
}

LightFrame SceneManager::defaultLight() const {
    LightFrame light{};
    const float direction[3] = {-0.3f, -1.0f, -0.2f};
    float length = std::sqrt(0.09f + 1.0f + 0.04f);
    for(int i = 0; i < 3; ++i) {
        light.direction[i] = direction[i] / length;
        light.radiance[i] = 3.0f;
    }
    return light;
}

}
