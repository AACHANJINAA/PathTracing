# Renderer Spec

이 문서는 **무엇을 만드는가**를 정하는 기준 문서입니다. 구현 순서는 `Design/IMPLEMENTATION_PLAN.md`,
수치·좌표·샘플링 규약은 `docs/RENDERING_MODEL.md`를 따릅니다.

## 1. 개요

| 항목 | 내용 |
| --- | --- |
| 형태 | Windows 데스크톱 앱, 창 하나에 진행형 누적 렌더링 결과 표시 |
| 기술 | C++20, Direct3D 12, HLSL compute shader, Dear ImGui |
| 적분기 | 단방향(unidirectional) Monte Carlo path tracing |
| 장면 | 해석적 도형 + 재질 + 광원을 JSON으로 정의 |
| 결과물 | 화면 표시, HDR 이미지 저장, 참조 이미지 대비 오차 수치 |

## 2. 목표 경험

1. 장면을 열면 노이즈 많은 이미지가 나타나고, 가만히 두면 점점 깨끗한 이미지로 수렴한다.
2. 카메라나 재질을 바꾸면 누적이 즉시 리셋되고 다시 수렴한다.
3. 샘플링 방식(BSDF만 / 광원만 / MIS)을 바꿔 **같은 샘플 수에서 노이즈 차이**를 직접 비교할 수 있다.
4. 결과 이미지를 저장하고 참조 렌더러 결과와 수치로 비교할 수 있다.

## 3. 기능 사양

### 3.1 도형

| 도형 | 정의 | 비고 |
| --- | --- | --- |
| Sphere | 중심, 반지름 | 광원으로도 사용 |
| Quad | 원점 코너 + 두 변 벡터(평행사변형) | Cornell Box 벽, 면광원 |
| Box | 최소/최대 꼭짓점 + 회전 | Cornell Box 내부 상자 |

- 도형 수는 수백 개 이하를 가정하고 초기에는 선형 순회한다. 가속 구조는 보류 항목이다.
- 모든 도형은 재질 인덱스 하나를 가진다.

### 3.2 재질

| 재질 | 마일스톤 | 설명 |
| --- | --- | --- |
| Diffuse (Lambert) | M2 | 반사율 albedo |
| Emissive | M2 | 방사 휘도 radiance × 세기, 한쪽 면만 방출 |
| Mirror | M4 | 완전 거울 반사 (델타 BSDF) |
| Dielectric | M4 | 매끈한 유리, 굴절률 IOR, Fresnel 반사/굴절 |
| Conductor (GGX) | M4 | 금속 거칠기 roughness, 반사율 F0 |
| Rough Dielectric (GGX) | 보류 | 거친 유리 |

### 3.3 광원

| 광원 | 마일스톤 | 설명 |
| --- | --- | --- |
| 면광원 (Emissive Quad/Sphere) | M2, M3 | 도형 + Emissive 재질. NEE 대상 |
| 상수 환경광 | M4 | 모든 방향 동일 휘도. Furnace test용 |
| HDR 환경맵 | M6 | 등장방형(equirectangular) HDR 이미지, 중요도 샘플링 |
| 점광원·방향광 | 보류 | 델타 광원은 MIS 처리 범위를 넓히므로 후순위 |

### 3.4 카메라

- 핀홀 카메라: 위치, 바라보는 점, 위쪽 벡터, 수직 FOV.
- 픽셀 내부 지터(jitter)로 안티에일리어싱. 피사계 심도는 보류.
- 조작: 우클릭 + 마우스 회전, W/A/S/D/Q/E 이동. 움직이면 누적 리셋.

### 3.5 적분기 설정 (UI)

| 설정 | 기본값 | 설명 |
| --- | --- | --- |
| Sampling | MIS | BSDF only / Light only(NEE) / MIS |
| Max depth | 8 | 경로 최대 반사 횟수 |
| Russian roulette | depth ≥ 3 | 시작 깊이 |
| Target spp | 무제한 | 도달하면 누적 정지 |
| Samples per frame | 1 | 프레임당 픽셀 샘플 수 |
| Seed | 0 | 같은 시드 = 같은 결과 |

### 3.6 디버그 표시

Lit(누적 결과), Normal, Albedo, Depth, 경로 길이 히트맵, NaN/Inf 표시.
디버그 표시는 누적과 분리되어 즉시 그려진다.

### 3.7 출력

- 화면: 누적 HDR → 노출 → 톤 매핑 → sRGB.
- 저장: HDR(PFM 또는 EXR, `docs/PLANS.md` 확인 필요 항목) + 톤 매핑된 PNG.
- 저장 파일에 장면 이름, spp, 시드, 샘플링 방식을 기록한다.

## 4. 표준 검증 장면

| 장면 | 목적 | 사용 시점 |
| --- | --- | --- |
| `cornell_box` | 간접광·색 번짐, 참조 이미지 비교 | M2~M3 |
| `furnace` | 균일 환경 속 구, 에너지 보존 확인 | M4 |
| `veach_mis` | 광택 판 4장 × 크기가 다른 광원 4개, 샘플링 방식 비교 | M5 |
| `env_spheres` | HDR 환경맵 아래 재질 구들 | M6 |

## 5. 비목표

- 삼각형 메시, glTF, BVH, DXR (보류: `Design/IMPLEMENTATION_PLAN.md` 보류 항목)
- 디노이저, 실시간 프레임 목표
- 볼륨, 스펙트럴 렌더링, 양방향(BDPT)·MLT 같은 고급 적분기
- 다른 렌더러보다 빠르다는 성능 주장
