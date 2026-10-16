/*
 * 내장 Pass가 주고받는 core.* 데이터 타입 목록. 플러그인이 내장 Pass와 연결하려면 같은 타입을 알아야 해서 공개한다.
 * - 외부 입력: SceneGeometry, SceneMaterials, CameraFrame, LightFrame, PresentTarget.
 * - 텍스처: ShadowDepth, Albedo, WorldNormal, MaterialParams, LinearDepth, Emissive, SceneHDR, LinearLDR.
 * - 범위: SDK 공개. 플러그인은 core.* 를 재정의할 수 없다.
 */
