/*
 * 장면 로딩 테스트(scene.h): GLB 사전 검사와 Sponza 전체 로드·감사 보고서. GPU는 쓰지 않는다.
 * - 실제 117,889,600바이트 GLB 통과, 133바이트 Git LFS 포인터는 lfsPointer로 실패, 없는 파일 실패,
 *   fastgltf로 전체 로드 후 메시·머티리얼·정점 수가 0보다 크고 bounds가 유효.
 */
#include "scene.h"
#include "test.h"

#include <string>

namespace {

const std::string kDir = RD_ASSET_DIR;

void realGlbPasses() {
    RD::GlbProbe probe = RD::probeGlb(kDir + "/sponza-png.glb");
    CHECK(probe.ok);
    CHECK_EQ(probe.version, 2u);
    CHECK_EQ(probe.fileSize, uint64_t(117889600));
    CHECK_EQ(probe.declaredLength, probe.fileSize);
}

void lfsPointerIsDetected() {
    RD::GlbProbe probe = RD::probeGlb(kDir + "/sponza-avif-hi.glb");
    CHECK(!probe.ok);
    CHECK(probe.lfsPointer);
}

void missingFileFails() {
    RD::GlbProbe probe = RD::probeGlb(kDir + "/does-not-exist.glb");
    CHECK(!probe.ok);
    CHECK(!probe.lfsPointer);
}

void fullLoadAudit() {
    RD::SceneManager scene;
    std::string error;
    CHECK(scene.load(kDir + "/sponza-png.glb", error));
    const RD::AssetAudit &audit = scene.audit();
    CHECK(audit.meshes > 0);
    CHECK(audit.primitives > 0);
    CHECK(audit.materials > 0);
    CHECK(audit.vertices > 0);
    CHECK(audit.indices > 0);
    const RD::SceneView &view = scene.view();
    CHECK(view.boundsMax[0] > view.boundsMin[0]);
    CHECK(view.boundsMax[1] > view.boundsMin[1]);
    std::printf("  %s\n", scene.auditText().c_str());
}

void lfsPointerLoadFails() {
    RD::SceneManager scene;
    std::string error;
    CHECK(!scene.load(kDir + "/sponza-avif-lo.glb", error));
    CHECK(error.find("LFS") != std::string::npos);
}

} // namespace

int main() {
    RUN(realGlbPasses);
    RUN(lfsPointerIsDetected);
    RUN(missingFileFails);
    RUN(fullLoadAudit);
    RUN(lfsPointerLoadFails);
    return TEST_RESULT();
}
