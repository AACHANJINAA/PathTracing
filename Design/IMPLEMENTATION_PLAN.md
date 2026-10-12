# Implementation Plan

`Design/RENDERER_SPEC.md`를 구현 단위로 나눈 계획입니다.
각 마일스톤은 **`docs/WORK_REQUEST.md`와 같은 항목 구조의 카드**로 작성되어 있습니다.

## 사용 방법

1. 진행할 마일스톤 카드의 `요청서 시작`부터 `요청서 끝` 사이를 복사합니다.
2. `docs/WORK_REQUEST.md`의 `## 요청서` 아래에 붙여넣습니다.
3. 이번 작업에만 해당하는 제한이나 취향은 `메모`에 추가합니다.
4. 카드 내용을 바꿔야 하면 요청서가 아니라 이 문서의 카드를 먼저 고칩니다.

상태는 `docs/PLANS.md`에서 관리하며, 이 문서에는 계획만 둡니다.

## 마일스톤 요약

| ID | 주제 | 핵심 결과 | 선행 |
| --- | --- | --- | --- |
| M0 | 골격 | 창, D3D12 장치, compute 출력, 톤 매핑, ImGui | - |
| M1 | 첫 레이 | 카메라 레이, 도형 교차, 장면 JSON, 디버그 표시 | M0 |
| M2 | 누적과 확산 | 난수, 진행형 누적, Lambert, 면광원, Cornell Box | M1 |
| M3 | 광원 샘플링 | NEE, 그림자 레이, 참조 이미지 비교 | M2 |
| M4 | 재질 확장 | 거울, 유리, GGX 금속, Furnace test | M3 |
| M5 | MIS | BSDF·광원 샘플링 결합, Veach 장면 비교 | M4 |
| M6 | 환경맵 | HDR 등장방형 환경맵과 중요도 샘플링 | M5 |
| M7 | 결과 정리 | HDR 저장, 수렴 그래프, README 캡처 | M6 |

---

## M0. 골격

<!-- 요청서 시작 -->
- 작업 ID: M0
- 작업 유형: 코드 구현
- 작업 대상: 프로젝트 골격, D3D12 초기화, compute → 화면 출력 경로
- 선행 조건: 없음
- 요청 내용:
  - `docs/ARCHITECTURE.md`의 폴더 구조대로 솔루션과 프로젝트를 만든다.
  - 창, 장치, 스왑체인, 프레임 동기화(Fence), 창 크기 변경을 구현한다.
  - `R32G32B32A32_FLOAT` 누적 텍스처(UAV)에 compute shader로 UV 그라디언트를 쓴다.
  - 톤 매핑 패스가 누적 텍스처를 읽어 백 버퍼에 그린다.
  - ImGui로 FPS, 해상도, 프레임 번호를 표시한다.
  - 재사용 가능 코드(`Math.h`, `D3DX12.h`, ImGui 통합, 카메라·입력)는 `D3D12` 프로젝트에서 가져오고 출처를 커밋 메시지에 적는다.
- 참조할 문서: `docs/ARCHITECTURE.md`
- 수정 허용 범위: 코드 전체 생성, `docs/PLANS.md`, `README.md`의 `빌드와 실행` 섹션
- 건드리지 말 것: `Design/RENDERER_SPEC.md`, `docs/RENDERING_MODEL.md`
- 원하는 결과: 빌드되는 빈 앱
- 사용자 확인이 필요한 조건: 셰이더 컴파일러(DXC/FXC) 선택, `ARCHITECTURE.md`와 다른 폴더 구조가 필요할 때
- 완료 기준:
  - Debug/Release x64 빌드 성공.
  - 창에 그라디언트와 ImGui가 보이고 창 크기 변경·최소화 후에도 정상.
  - Debug Layer 오류 없음.
- 검증: V-BUILD, V-RESIZE
- 메모:
<!-- 요청서 끝 -->

## M1. 첫 레이

<!-- 요청서 시작 -->
- 작업 ID: M1
- 작업 유형: 코드 구현
- 작업 대상: 카메라 레이 생성, 도형 교차, 장면 로딩, 디버그 표시
- 선행 조건: M0
- 요청 내용:
  - `docs/RENDERING_MODEL.md`의 좌표계·카메라 규약으로 픽셀 중심 레이를 만든다.
  - Sphere, Quad, Box 교차 함수를 `intersect(ray, tmin, tmax) → hit` 형태로 구현한다.
  - `Scenes/*.json`을 읽어 도형·재질 버퍼(StructuredBuffer)를 업로드한다.
  - 디버그 표시: Normal, Albedo, Depth. 카메라 조작을 연결한다.
  - `Scenes/cornell_box.json`을 작성한다.
- 참조할 문서: `docs/RENDERING_MODEL.md`(좌표계, 카메라, 법선), `docs/ARCHITECTURE.md`(장면 형식, 바인딩)
- 수정 허용 범위: `PathTracer/` 코드, `Scenes/`, `docs/PLANS.md`
- 건드리지 말 것: `Design/RENDERER_SPEC.md`
- 원하는 결과: Cornell Box가 Normal/Albedo/Depth로 올바르게 보인다.
- 사용자 확인이 필요한 조건: 장면 JSON 형식을 `ARCHITECTURE.md`와 다르게 바꿔야 할 때
- 완료 기준:
  - 모든 도형의 법선이 바깥을 향하고 Normal 표시 색이 축 방향과 일치.
  - 카메라 이동·회전이 규약대로 동작.
  - 잘못된 JSON은 오류 메시지를 띄우고 기존 장면을 유지.
- 검증: V-BUILD, V-NORMAL, V-SCENE
- 메모:
<!-- 요청서 끝 -->

## M2. 누적과 확산

<!-- 요청서 시작 -->
- 작업 ID: M2
- 작업 유형: 코드 구현
- 작업 대상: 난수, 진행형 누적, Lambert BSDF, 발광 재질, 기본 경로 추적
- 선행 조건: M1
- 요청 내용:
  - 픽셀·프레임·시드로 초기화하는 PCG 난수를 구현한다.
  - 픽셀 지터 + 경로 추적 루프(최대 깊이, 러시안 룰렛)를 구현한다. 광원은 BSDF 샘플링으로 우연히 맞을 때만 기여한다.
  - Lambert는 코사인 가중 반구 샘플링을 쓴다.
  - 누적 규칙과 리셋 조건은 `docs/RENDERING_MODEL.md`를 따른다.
  - UI: spp 표시, 누적 일시정지, 리셋 버튼, Max depth, Target spp, Seed. NaN/Inf 디버그 표시.
- 참조할 문서: `docs/RENDERING_MODEL.md`(샘플링, 누적, 난수)
- 수정 허용 범위: `PathTracer/` 코드, `Scenes/`, `docs/PLANS.md`
- 건드리지 말 것: `Design/RENDERER_SPEC.md`
- 원하는 결과: Cornell Box가 노이즈에서 깨끗한 이미지로 수렴한다.
- 사용자 확인이 필요한 조건: 누적 버퍼 형식이나 리셋 조건을 바꿔야 할 때
- 완료 기준:
  - 가만히 두면 수렴하고, 카메라를 움직이면 즉시 리셋.
  - 같은 시드·spp에서 결과가 같다.
  - 1024 spp까지 NaN/Inf 픽셀 0개.
- 검증: V-BUILD, V-RESET, V-DETERMINISM, V-NAN
- 메모:
<!-- 요청서 끝 -->

## M3. 광원 샘플링 (NEE)

<!-- 요청서 시작 -->
- 작업 ID: M3
- 작업 유형: 코드 구현 + 검증
- 작업 대상: 광원 직접 샘플링, 그림자 레이, 참조 비교 도구
- 선행 조건: M2
- 요청 내용:
  - 발광 도형 목록을 만들고 광원 하나를 선택 → 표면 한 점을 샘플링하는 NEE를 구현한다.
  - Quad는 면적 샘플링, Sphere는 보이는 원뿔(solid angle) 샘플링을 쓴다. pdf를 입체각으로 변환한다.
  - 그림자 레이는 가장 가까운 교차가 아니라 "가려짐 여부"만 확인한다.
  - NEE를 쓴 경로에서 BSDF 샘플링으로 광원에 맞은 기여는 중복되지 않게 제외한다.
  - UI Sampling에 `BSDF only` / `Light only` 선택을 추가한다.
  - HDR 이미지 저장과 RMSE 비교 도구(`Tools/`)를 추가하고, 참조 렌더러로 Cornell Box 참조 이미지를 만든다.
- 참조할 문서: `docs/RENDERING_MODEL.md`(pdf 측도, 그림자 레이 ε), `docs/VERIFY.md`(V-NEE, V-REFERENCE)
- 수정 허용 범위: `PathTracer/` 코드, `Tools/`, `Reference/`, `docs/PLANS.md`, `docs/VERIFY.md`의 기준 수치
- 건드리지 말 것: `Design/RENDERER_SPEC.md`
- 원하는 결과: 같은 spp에서 NEE가 훨씬 덜 노이즈하고, 두 방식이 같은 평균으로 수렴한다.
- 사용자 확인이 필요한 조건: 참조 렌더러 선택(`docs/PLANS.md` 확인 필요 항목), 저장 포맷 선택
- 완료 기준:
  - BSDF only와 Light only의 고 spp 결과 차이가 통계 오차 범위 안.
  - 참조 이미지 대비 RMSE가 spp 증가에 따라 감소(V-CONVERGENCE).
- 검증: V-NEE, V-REFERENCE, V-CONVERGENCE, V-NAN
- 메모:
<!-- 요청서 끝 -->

## M4. 재질 확장

<!-- 요청서 시작 -->
- 작업 ID: M4
- 작업 유형: 코드 구현 + 검증
- 작업 대상: Mirror, Dielectric, GGX Conductor, 상수 환경광, Furnace test
- 선행 조건: M3
- 요청 내용:
  - BSDF 인터페이스를 `eval`, `sample`, `pdf`로 통일하고 델타 BSDF 플래그를 둔다.
  - Mirror, Dielectric(Fresnel 반사/굴절 확률 선택), GGX Conductor(VNDF 샘플링)를 구현한다.
  - 델타 BSDF에서는 NEE를 생략하는 규칙을 적용한다.
  - 상수 환경광을 추가하고 `Scenes/furnace.json`을 작성한다.
- 참조할 문서: `docs/RENDERING_MODEL.md`(BSDF 규약, 법선 방향, IOR), `docs/VERIFY.md`(V-FURNACE)
- 수정 허용 범위: `PathTracer/` 코드, `Scenes/`, `docs/PLANS.md`
- 건드리지 말 것: `Design/RENDERER_SPEC.md`
- 원하는 결과: 재질 구가 늘어선 장면과 Furnace test 통과
- 사용자 확인이 필요한 조건: 다중 산란 에너지 보정 등 사양에 없는 보정을 추가하려 할 때
- 완료 기준:
  - 흰 Lambert 구, 거울 구가 Furnace에서 배경과 구분되지 않음(오차 범위 안).
  - GGX는 거칠기 증가에 따른 에너지 손실 양상을 기록(단일 산란 모델의 알려진 한계).
  - 유리 구 내부에서 NaN 없음.
- 검증: V-FURNACE, V-NAN, V-DETERMINISM
- 메모:
<!-- 요청서 끝 -->

## M5. MIS

<!-- 요청서 시작 -->
- 작업 ID: M5
- 작업 유형: 코드 구현 + 검증
- 작업 대상: 다중 중요도 샘플링(power heuristic)
- 선행 조건: M4
- 요청 내용:
  - NEE 기여와 BSDF 샘플링으로 광원에 맞은 기여를 power heuristic(β=2)으로 결합한다.
  - 이전 정점의 BSDF pdf를 경로 상태로 넘겨 광원 히트 시 가중치를 계산한다.
  - `Scenes/veach_mis.json`을 작성하고 BSDF only / Light only / MIS 비교 이미지를 저장한다.
- 참조할 문서: `docs/RENDERING_MODEL.md`(pdf 측도), `docs/VERIFY.md`(V-MIS)
- 수정 허용 범위: `PathTracer/` 코드, `Scenes/`, `Captures/`, `docs/PLANS.md`
- 건드리지 말 것: `Design/RENDERER_SPEC.md`
- 원하는 결과: Veach 장면 3종 비교 이미지
- 사용자 확인이 필요한 조건: 없음
- 완료 기준:
  - 세 방식의 고 spp 평균이 일치(편향 없음).
  - 같은 spp에서 MIS의 RMSE가 두 방식 중 나은 쪽 이하.
- 검증: V-MIS, V-CONVERGENCE
- 메모:
<!-- 요청서 끝 -->

## M6. 환경맵

<!-- 요청서 시작 -->
- 작업 ID: M6
- 작업 유형: 코드 구현 + 검증
- 작업 대상: HDR 등장방형 환경맵, 휘도 기반 중요도 샘플링
- 선행 조건: M5
- 요청 내용:
  - HDR 이미지를 로드해 GPU 텍스처로 올린다.
  - 휘도 × sinθ로 행(marginal)·열(conditional) CDF를 CPU에서 만들어 버퍼로 올린다.
  - 환경맵 샘플링을 NEE 광원 후보에 포함하고 MIS에 연결한다.
  - `Scenes/env_spheres.json`을 작성한다.
- 참조할 문서: `docs/RENDERING_MODEL.md`(방향 ↔ UV 변환), `docs/VERIFY.md`(V-ENV)
- 수정 허용 범위: `PathTracer/` 코드, `Scenes/`, `Asset/`, `docs/PLANS.md`
- 건드리지 말 것: `Design/RENDERER_SPEC.md`
- 원하는 결과: 밝은 태양이 있는 HDR에서도 빠르게 수렴
- 사용자 확인이 필요한 조건: HDR 이미지 로더 외부 라이브러리 사용
- 완료 기준:
  - 균등 샘플링과 중요도 샘플링의 고 spp 평균 일치.
  - 같은 spp에서 중요도 샘플링 RMSE가 더 낮음.
- 검증: V-ENV, V-CONVERGENCE, V-NAN
- 메모:
<!-- 요청서 끝 -->

## M7. 결과 정리

<!-- 요청서 시작 -->
- 작업 ID: M7
- 작업 유형: 검증 + 문서
- 작업 대상: 수렴 그래프, 캡처, README 결과 섹션
- 선행 조건: M6
- 요청 내용:
  - 장면별 spp–RMSE 로그 그래프를 생성하는 스크립트를 `Tools/`에 둔다.
  - `Captures/`에 장면별 최종 이미지와 샘플링 비교 이미지를 저장한다.
  - README에 결과 이미지, 검증 수치, 한계를 추가한다.
- 참조할 문서: `docs/VERIFY.md`, `README.md`
- 수정 허용 범위: `Tools/`, `Captures/`, `README.md`, `docs/PLANS.md`
- 건드리지 말 것: `PathTracer/` 코드(버그 발견 시 별도 요청)
- 원하는 결과: 처음 보는 사람이 README만으로 무엇을 검증했는지 알 수 있음
- 사용자 확인이 필요한 조건: README 구성 변경
- 완료 기준: 모든 수치가 실제 실행 결과에서 나오고 실행 조건(GPU, spp, 시드)이 함께 적혀 있음
- 검증: V-CONVERGENCE, V-REFERENCE
- 메모:
<!-- 요청서 끝 -->

---

## 보류 항목

필요해지면 위와 같은 카드로 만든 뒤 진행합니다.

- 삼각형 메시 + BVH(또는 DXR) — 교차 인터페이스 재사용
- Rough Dielectric, 다중 산란 GGX 에너지 보정
- 점광원·방향광(델타 광원)
- 피사계 심도, 모션 블러
- 볼륨·참여 매질
- 디노이저(오프라인 OIDN)

## 리스크

| 리스크 | 대응 |
| --- | --- |
| 편향 버그가 노이즈에 가려 보이지 않음 | 고 spp 평균 비교와 Furnace test를 마일스톤 완료 기준에 포함 |
| pdf 측도(면적/입체각) 혼동 | `RENDERING_MODEL.md`에 측도 규약 고정, 변환 지점을 한 함수로 제한 |
| GPU 타임아웃(TDR) | 프레임당 샘플 수 상한, 고 spp는 여러 프레임에 나눠 누적 |
| 참조 렌더러와 장면 정의 차이 | 단위·색 공간·카메라를 `RENDERING_MODEL.md` 기준으로 맞추고 변환 스크립트로 관리 |
