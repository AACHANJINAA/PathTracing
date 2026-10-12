# Verify

구현 후 확인할 검증 항목입니다. 요청서와 마일스톤 카드는 아래 **V-ID**로 검증 항목을 지정합니다.

## 공통 규칙

- 검증 결과는 실행 조건(GPU, 해상도, spp, 시드, Sampling 방식, 빌드 구성)과 함께 보고한다.
- 실행하지 못한 검증은 "미검증"으로 명시하고 `완료`로 표시하지 않는다.
- Path tracing의 오차는 노이즈와 편향이 섞여 있다. **"눈으로 비슷함"은 통과 기준이 아니다.** 수치 기준이 있는 항목은 수치로 판정한다.
- 통계 비교의 "오차 범위 안"은 같은 장면을 시드만 바꿔 여러 번 렌더링했을 때의 픽셀 평균 차이 분포를 기준으로 판단한다.

## 실행 도구

- **KernelTool** (`Tests/`): GPU와 같은 HLSL 커널 코드를 C++로 컴파일해 CPU에서 실행한다.
  - Windows: `PathTracing.slnx`의 `KernelTool` 프로젝트 → `KernelTool.exe test`
  - Linux/WSL: `Tests/build.sh` → `build/KernelTool test`
- KernelTool 통과는 **커널 로직**의 검증이다. FXC 컴파일 결과·GPU 실행은 앱에서 따로 확인한다.

## 항목

| V-ID | 대상 | 방법 | 통과 기준 | 도입 |
| --- | --- | --- | --- | --- |
| V-BUILD | 빌드 | Debug/Release x64 빌드, Debug에서 1분 실행 | 경고 증가 없음, Debug Layer 오류 0 | M0 |
| V-RESIZE | 창 크기 | 크기 변경, 최소화·복원, 전체화면 전환 | 크래시·오류 없음, 누적 리셋 | M0 |
| V-NORMAL | 교차·법선 | Normal 표시에서 축 방향 면 색 확인, `KernelTool test`의 V-NORMAL 테스트 | 법선 바깥쪽, `t`·법선 오차 1e-5 이내 | M1 |
| V-SCENE | 장면 로딩 | 정상·오류 JSON 각각 로드 (`KernelTool test`, 앱의 RELOAD) | 오류 시 메시지 + 기존 장면 유지 | M1 |
| V-RESET | 누적 리셋 | `RENDERING_MODEL.md` 10장 표의 각 동작 수행 | 표대로 리셋/유지 | M2 |
| V-DETERMINISM | 재현성 | 같은 seed·spp로 두 번 저장 | HDR 파일 비트 단위 동일 | M2 |
| V-NAN | 수치 안정성 | 모든 표준 장면 1024 spp, NaN 카운터 확인 | NaN/Inf 0 | M2 |
| V-NEE | NEE 정확성 | Cornell Box를 BSDF only / Light only로 각각 고 spp 렌더 | 평균 이미지 차이가 오차 범위 안, 같은 spp에서 Light only의 RMSE가 더 낮음 | M3 |
| V-REFERENCE | 참조 일치 | 참조 렌더러 이미지와 RMSE 비교 | 기준 수치는 M3에서 측정 후 이 표에 기록 | M3 |
| V-CONVERGENCE | 수렴률 | spp 1~4096에서 참조 대비 RMSE, log-log 기울기 | 기울기 ≈ -0.5 (편향 없으면 1/√N) | M3 |
| V-FURNACE | 에너지 보존 | 상수 환경광 L=1 안의 albedo 1 Lambert 구, 거울 구 | 구 픽셀 평균 1 ± 오차 범위, GGX는 roughness별 손실 기록 | M4 |
| V-MIS | MIS 정확성 | Veach 장면을 세 방식으로 렌더 | 고 spp 평균 일치, MIS RMSE ≤ min(두 방식) | M5 |
| V-ENV | 환경맵 샘플링 | 균등 vs 중요도 샘플링 | 고 spp 평균 일치, 중요도 샘플링 RMSE 더 낮음 | M6 |

## 회귀 확인

마일스톤이 끝날 때마다 이전 마일스톤의 V-ID를 다시 실행한다. 특히:
- 새 재질·광원 추가 후 V-NAN, V-DETERMINISM
- 샘플링 경로 변경 후 V-NEE, V-CONVERGENCE

## 기준 수치 기록

M3 이후 측정한 기준값을 아래에 기록한다(장면, spp, 시드, RMSE, GPU).

| 날짜 | V-ID | 장면 | 조건 | 결과 |
| --- | --- | --- | --- | --- |

## 알려진 문제

(아직 없음)
