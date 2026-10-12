# Architecture

폴더·코드 모듈의 책임, GPU 리소스 구성, 장면 파일 형식, 코드 컨벤션의 기준 문서입니다.
수치·좌표 규약은 `docs/RENDERING_MODEL.md`가 기준입니다.

## 1. 폴더 구조 (M0에서 생성)

```text
PathTracing.slnx
PathTracer/                       # 실행 프로젝트
  Core/main.cpp                   # 진입점
  Common/                         # PCH(stdafx), D3DX12.h, HlslTypes.h(C++용 float3 등)
  Renderer/
    Renderer.*                    # 장치·스왑체인·프레임 동기화, 패스 실행 순서
    ShaderCompiler.*              # 실행 중 FXC 컴파일 (Shaders/ 폴더)
    ViewerSettings.h              # UI가 편집하는 설정
    GpuScene.*                    # 장면 → GPU 버퍼 업로드, 변경 감지
    Accumulator.*                 # 누적 텍스처, spp 카운트, 리셋
    Pass/
      IntegratorPass.*            # path tracing compute dispatch
      DisplayPass.*               # 노출·톤 매핑·sRGB → 백 버퍼
    Shader/
      Integrator.hlsl             # 경로 추적 메인 (compute)
      Display.hlsl                # 톤 매핑 (fullscreen)
      Shared.hlsli                # CPU와 공유하는 구조체 정의
      Random.hlsli                # PCG 난수
      Intersection.hlsli          # 도형 교차
      Sampling.hlsli              # 반구·원뿔·면적 샘플링, 좌표계 변환
      Bsdf.hlsli                  # 재질별 eval/sample/pdf
      Light.hlsli                 # 광원 선택·샘플링·pdf, 환경맵
  Scene/
    Scene.*                       # 도형·재질·광원·카메라 데이터 (CPU)
    SceneLoader.*                 # JSON ↔ Scene
  Output/ImageWriter.*            # HDR(PFM/EXR)·PNG 저장
  UI/                             # ImGui 패널
  ThirdParty/ImGui/
Scenes/                           # *.json 장면
Asset/                            # HDR 환경맵 (M6)
Reference/                        # 참조 렌더러 장면 변환본·참조 이미지 (M3)
Tools/                            # generate_vcxproj.py, 이미지 비교(RMSE), 수렴 그래프 스크립트 (M3, M7)
Captures/                         # README용 결과 이미지 (M5, M7)
Tests/                            # CPU 측 단위 테스트(교차·샘플링 함수 검증)
```

## 2. 모듈 책임

| 모듈 | 책임 | 하지 않는 일 |
| --- | --- | --- |
| `Renderer` | 초기화, 프레임 순서, 제출, 창 크기 변경, 종료 | 장면 해석, 셰이딩 |
| `ShaderCompiler` | `Shaders/`의 HLSL을 FXC(SM 5.1)로 컴파일, 오류 메시지 반환 | 캐시 |
| `GpuScene` | Scene을 StructuredBuffer로 변환·업로드, 변경 시 누적 리셋 요청 | 파일 입출력 |
| `Accumulator` | 누적 텍스처 수명, spp, 리셋 조건 판정 | 샘플 계산 |
| `IntegratorPass` | 루트 시그니처·PSO, 프레임 상수, dispatch | 표시 변환 |
| `DisplayPass` | 노출·톤 매핑·sRGB 인코딩, 디버그 표시 | 누적 |
| `Scene` / `SceneLoader` | 장면 데이터와 JSON 변환, 입력 검증 | GPU 리소스 |
| `ImageWriter` | 누적 결과 readback 후 파일 저장 | 톤 매핑 외 이미지 처리 |
| `UI` | 설정 편집, 통계 표시, 요청 전달 | 렌더링 상태 직접 변경(요청으로 전달) |

## 3. 프레임 흐름

```text
입력·UI → 변경 감지(카메라/장면/설정) → 필요 시 누적 리셋
  → IntegratorPass: 픽셀당 N 샘플 → 누적 텍스처 갱신 (UAV)
  → UAV 배리어 → DisplayPass: 누적 텍스처 → 백 버퍼
  → UI → Present
```

- 누적 텍스처는 평소 `UNORDERED_ACCESS`에 두고, 표시 패스(픽셀 셰이더) 동안만 `PIXEL_SHADER_RESOURCE`로 전환한다.
- 장면 버퍼 재업로드는 GPU 완료를 기다린 뒤 수행한다.

## 4. GPU 리소스와 바인딩

| 레지스터 | 리소스 | 형식 | 도입 |
| --- | --- | --- | --- |
| b0 | `FRAME_CONSTANTS` | 카메라 기저, 해상도, 프레임 번호, spp, 시드, 설정 플래그 | M0 |
| t0 | `StructuredBuffer<PRIMITIVE>` | 도형 | M1 |
| t1 | `StructuredBuffer<MATERIAL>` | 재질 | M1 |
| t2 | `StructuredBuffer<LIGHT>` | 발광 도형 인덱스·선택 확률 | M3 |
| t3 | `Texture2D<float4>` | HDR 환경맵 | M6 |
| t4, t5 | `StructuredBuffer<float>` | 환경맵 marginal / conditional CDF | M6 |
| u0 | `RWTexture2D<float4>` | 누적(평균) 결과 `R32G32B32A32_FLOAT` | M0 |
| u1 | `RWStructuredBuffer<uint>` | 디버그 카운터(NaN 수 등) | M2 |
| s0 | 정적 샘플러 | 선형, 환경맵용 | M6 |

- 구조체는 `Shader/Shared.hlsli`와 C++ 구조체를 1:1로 맞추고 C++ 쪽에 `static_assert(sizeof)`로 크기를 고정한다.
- 스레드 그룹은 8×8을 기본으로 한다.

## 5. 장면 파일 형식 (초안, M1에서 확정)

```json
{
  "camera": { "position": [0, 1, 3.4], "target": [0, 1, 0], "up": [0, 1, 0], "fov_y": 39.3 },
  "materials": {
    "white": { "type": "diffuse", "albedo": [0.73, 0.73, 0.73] },
    "light": { "type": "emissive", "radiance": [17, 12, 4] }
  },
  "primitives": [
    { "type": "quad", "corner": [-1, 0, -1], "u": [2, 0, 0], "v": [0, 0, 2], "material": "white" },
    { "type": "sphere", "center": [0, 0.5, 0], "radius": 0.5, "material": "white" },
    { "type": "box", "min": [-0.3, 0, -0.3], "max": [0.3, 1.2, 0.3], "rotation_y": 15, "material": "white" }
  ],
  "environment": { "type": "constant", "radiance": [0, 0, 0] }
}
```

- 색 값은 **선형 RGB**다(sRGB 인코딩 아님).
- 알 수 없는 키·타입, 범위를 벗어난 값은 로드 오류로 처리하고 기존 장면을 유지한다.

## 6. 코드 컨벤션 (D3D12 프로젝트와 동일)

- 클래스 이름은 대문자: `RENDERER`, `GPU_SCENE`.
- 일반 변수는 소문자, 복합어는 `snake_case`.
- bool 변수는 `is` 접두사, `is` 바로 뒤에 밑줄 없음: `isaccumulating`.
- 멤버 변수는 `_` 접두사: `_device`, `_sample_count`.
- 함수는 소문자 + 밑줄: `create_pipeline`, `reset_accumulation`.
- Manager 파일은 `Manager` 접미사: `CameraManager.h`.
- 공통 include는 `stdafx.h`, 기본 수학은 `Common/Math.h`.
- HLSL 함수도 같은 규칙을 따르고, 공유 구조체 이름은 C++과 동일하게 쓴다.
- 외부 라이브러리 소스는 원래 컨벤션을 유지한다.

## 7. 가져오는 코드

`D3D12` 프로젝트(`GITHUB/D3D12/Deffered`)에서 복사해 독립적으로 발전시킨다. 공용 라이브러리로 묶지 않는다.

| 가져올 것 | 원본 위치 |
| --- | --- |
| `Math.h`, `D3DX12.h`, `stdafx` | `Deffered/Common/` |
| 카메라·입력 매니저 | `Deffered/Renderer/Manager/` |
| ImGui와 Win32/DX12 백엔드, 테마 | `Deffered/ThirdParty/ImGui/`, `Deffered/UI/UiTheme.*` |
| 창 생성·전체화면·Fence 패턴 | `Deffered/Renderer/Renderer.cpp` 참고 |

glTF·메시·텍스처 리소스 코드는 가져오지 않는다.

## 8. 구조 변경 기록

- 2026-10-13: 초기 구조 정의(코드 미생성).
- 2026-10-13 (M0): `Math.h`와 Camera/Input 매니저는 가져오지 않음. 셰이더와 공유하는 구조체를 위해 `HlslTypes.h`를 두고, 카메라·입력은 M1에서 ImGui 입력 상태로 구현한다. 프로젝트 파일은 `Tools/generate_vcxproj.py`로 생성한다.
