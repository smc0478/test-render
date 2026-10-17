/*
 * Sponza 장면 로드와 카메라·광원. 그래프 외부 입력(scene.geometry, frame.camera, frame.lights)을 공급한다.
 * - probeGlb: 파싱 전에 GLB 헤더 확인. 134바이트짜리 Git LFS 포인터 텍스트면 명확히 실패한다.
 * - SceneManager::load: GLB 확인 → fastgltf 파싱 → 정점·인덱스 수, bounds, 머티리얼 alpha mode 집계(감사 보고서).
 * - SceneView: Pass가 읽는 읽기 전용 장면 정보. CameraFrame / LightFrame: bounds로 만든 기본값.
 * - 필요: GPU 업로드(정점·인덱스 버퍼, 텍스처), PNG 디코딩, draw 목록(셰이더 단계에서).
 * - 범위: 엔진 내부. 구현: src/scene.cc
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core-schemas.h"

namespace RD {

struct GlbProbe {
    bool ok = false;
    bool lfsPointer = false;
    uint32_t version = 0;
    uint64_t declaredLength = 0;
    uint64_t fileSize = 0;
    std::string error;
};

GlbProbe probeGlb(const std::string &path);

struct AssetAudit {
    size_t nodes = 0;
    size_t meshes = 0;
    size_t primitives = 0;
    size_t materials = 0;
    size_t images = 0;
    size_t textures = 0;
    size_t vertices = 0;
    size_t indices = 0;
    size_t opaque = 0;
    size_t mask = 0;
    size_t blend = 0;
    std::vector<std::string> extensionsUsed;
};

struct SceneView {
    size_t drawCount = 0;
    float boundsMin[3] = {0, 0, 0};
    float boundsMax[3] = {0, 0, 0};
};

class SceneManager {
public:
    bool load(const std::string &path, std::string &error);

    const AssetAudit &audit() const { return _audit; }
    const SceneView &view() const { return _view; }
    std::string auditText() const;

    // bounds를 내려다보는 기본 카메라와 기본 방향광.
    CameraFrame defaultCamera(float aspect) const;
    LightFrame defaultLight() const;

private:
    AssetAudit _audit;
    SceneView _view;
};

}
