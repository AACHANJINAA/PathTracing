# PathTracing

C++20 / Direct3D 12 compute shader로 만드는 **메시 없는 GPU Path Tracer**입니다.
구·사각형·박스 같은 해석적 도형만으로 장면을 구성하고, 빛이 장면 안에서 튕기며 이동하는 과정을
몬테카를로 적분으로 계산해 **물리적으로 정답에 가까운 이미지**를 만드는 것이 목표입니다.

> 현재 상태: **M2 구현 완료(사용자 검증 대기)**. Cornell Box를 BSDF 샘플링 경로 추적으로 누적 렌더링합니다. 진행 상황은 `docs/PLANS.md`를 봅니다.

## Path Tracing이 무엇인가

카메라의 각 픽셀에서 광선(ray)을 쏘고, 광선이 물체에 맞으면 그 표면의 재질에 따라 **무작위 방향**으로
다시 튕겨 보냅니다. 이렇게 이어진 경로(path)가 광원에 닿으면 그 빛이 픽셀로 돌아온 것으로 계산합니다.

- 한 픽셀에 경로 하나만 쓰면 결과는 노이즈투성이입니다.
- 프레임마다 새 경로를 더해 평균을 내면(누적) 노이즈가 점점 줄어 **정답 이미지로 수렴**합니다.
- 그림자, 부드러운 반그림자, 색 번짐(color bleeding), 반사·굴절, 간접광이 별도 기법 없이 한 가지 계산에서 함께 나옵니다.

래스터화 렌더러(예: 이전 프로젝트 `D3D12`의 Forward/Deferred)는 "삼각형을 화면에 투영한 뒤 근사식으로 조명"합니다.
Path Tracer는 "빛의 이동을 직접 표본 추출"하므로 느리지만 근사가 적어, 래스터화 결과를 검증하는 **기준 이미지**로도 쓰입니다.

## 왜 메시 없이 만드는가

이 프로젝트의 주제는 **광 수송(light transport)과 샘플링의 정확성**입니다.

- 해석적 도형은 교차 판정이 수식 몇 줄이라 glTF 로딩, BVH, 법선·UV 버그 같은 변수를 통제할 수 있습니다.
- DXR(BLAS/TLAS, Shader Table) 없이 compute shader 하나로 구현해 API 준비 작업보다 적분기 자체에 집중합니다.
- Cornell Box, White Furnace, Veach MIS 장면처럼 **정답이 알려진 표준 장면**을 도형만으로 만들 수 있습니다.

교차 판정은 `intersect(ray) → hit` 인터페이스로 분리해 두므로, 이후 삼각형 + BVH를 추가해도 셰이딩 코드는 바뀌지 않게 설계합니다.

## 목표와 비목표

| 목표 | 비목표 (현재 범위 밖) |
| --- | --- |
| 진행형 누적(progressive) GPU path tracing | 실시간 게임용 화질, 디노이저 |
| Lambert, 거울, 유리, GGX 금속 재질 | 삼각형 메시·glTF·BVH (보류 항목) |
| 광원 직접 샘플링(NEE)과 MIS | 볼륨·참여 매질, 스펙트럴 렌더링 |
| HDR 환경맵 중요도 샘플링 | 성능 경쟁, 다른 렌더러 대비 속도 주장 |
| 참조 렌더러 대비 오차(RMSE)·수렴 검증 | |

## 빌드와 실행

- Windows, Direct3D 12 지원 GPU(Feature Level 11_0 이상)
- Visual Studio 또는 Rider + MSVC `v145`, C++20, Windows SDK 10.0
- 외부 패키지 설치 없음 (Dear ImGui는 저장소에 포함)

```powershell
msbuild .\PathTracing.slnx /m /p:Configuration=Release /p:Platform=x64
& .\PathTracer\bin\x64\Release\PathTracer.exe
```

셰이더(`Shaders/`)는 빌드 후 실행 파일 옆으로 복사되고 실행 중 FXC로 컴파일됩니다.
소스 파일을 추가·삭제했다면 `python Tools/generate_vcxproj.py .`로 프로젝트 파일을 다시 생성합니다.

| 입력 | 동작 |
| --- | --- |
| 우클릭 드래그 | 카메라 회전 |
| W/A/S/D, Q/E | 이동, 아래/위 (Shift: 빠르게) |
| F11 | 창 / 전체화면 |

장면은 실행 파일 옆 `Scenes/*.json`에서 고릅니다(`Scenes/` 폴더가 빌드 후 복사됨).

| 패널 | 내용 |
| --- | --- |
| VIEW | Lit(누적), Normal, Albedo, Depth, Path length, NaN / Inf(마젠타 표시) |
| INTEGRATOR | Max depth(경로 세그먼트 수), RR from, Seed, Samples/frame, 비유한 샘플 수 |
| ACCUMULATION | Pause, RESET, Target spp |

카메라·장면·Max depth·Seed를 바꾸면 누적이 리셋됩니다. 노출·톤 매핑은 리셋하지 않습니다.

> 현재 구현 단계: **M2** (BSDF 샘플링만 사용하는 진행형 경로 추적). 광원 직접 샘플링(NEE)은 M3에서 추가합니다.

### KernelTool (CPU 검증 도구)

셰이더 커널 코드를 C++로 그대로 컴파일해 CPU에서 실행합니다. GPU 없이 교차·샘플링 로직을 테스트합니다.

```powershell
msbuild .\Tests\KernelTool.vcxproj /p:Configuration=Release /p:Platform=x64
& .\Tests\bin\x64\Release\KernelTool.exe test
```

## 폴더 구조

```text
README.md                     # 이 문서: 프로젝트 소개와 문서 안내
Design/
  RENDERER_SPEC.md            # 무엇을 만드는가: 목표, 범위, 기능 사양
  IMPLEMENTATION_PLAN.md      # 어떤 순서로 만드는가: 마일스톤 카드 (WORK_REQUEST로 복사해 사용)
docs/
  WORK_REQUEST.md             # 지금 할 작업 한 건의 요청서
  AI_USAGE.md                 # AI가 읽을 문서를 최소화하는 규칙
  WORKFLOW.md                 # 작업 진행 순서와 사용자 확인 지점
  ARCHITECTURE.md             # 폴더·모듈 책임, GPU 리소스, 장면 형식, 코드 컨벤션
  RENDERING_MODEL.md          # 렌더링 규약: 좌표계, 단위, 색 공간, 샘플링·누적 규칙
  VERIFY.md                   # 검증 항목(V-ID)과 완료 기준
  PLANS.md                    # 현재 상태, 다음 작업, 보류·확인 필요 항목
  DECISIONS.md                # 확정된 결정만 짧게 기록
Study/                        # 예외 자료 (작업 흐름 밖)
  PATH_TRACING_STUDY.md       # 이론 학습 노트: 렌더링 방정식부터 MIS까지
```

코드 폴더 구조는 `docs/ARCHITECTURE.md`에 정의되어 있으며 M0에서 생성합니다.

## 작업 방식

```text
IMPLEMENTATION_PLAN의 마일스톤 카드 복사
  → docs/WORK_REQUEST.md에 붙여넣고 메모·제한만 추가
  → AI가 요청서 범위 안에서 구현
  → docs/VERIFY.md의 해당 V-ID로 검증
  → docs/PLANS.md 상태 갱신, 필요한 결정만 docs/DECISIONS.md에 기록
  → 커밋
```

`Study/`는 개인 학습용 예외 자료로, 위 작업 흐름(카드·요청서·문서 갱신)에 포함되지 않습니다.

처음 읽는다면 **README → `Study/PATH_TRACING_STUDY.md` → `Design/RENDERER_SPEC.md`** 순서를 추천합니다.
