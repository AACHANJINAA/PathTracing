# Decisions

반복해서 확인할 필요가 있는 **확정 결정**만 짧게 기록합니다. 임시 아이디어나 미확정 논의는 `docs/PLANS.md`의 `확인 필요`에 둡니다.

## 작성 형식

- `[YYYY-MM-DD] 결정 제목`
  - 결정: 확정된 내용
  - 이유: 결정한 이유
  - 영향: 관련 문서, 시스템, 작업 범위

## Decisions

- `[2026-10-13] D3D12 엔진과 별도 프로젝트로 분리`
  - 결정: Path Tracer는 `GITHUB/PathTracing`의 독립 프로젝트로 만든다. 필요한 코드는 D3D12 프로젝트에서 복사하고 공용 라이브러리로 묶지 않는다.
  - 이유: 래스터 패스 조립 구조와 경로 추적 구조가 거의 겹치지 않고, 포트폴리오 주제(래스터 파이프라인 설계 / 광 수송 정확성)를 분리하기 위해서다.
  - 영향: `docs/ARCHITECTURE.md` 7장

- `[2026-10-13] 메시 없이 해석적 도형만 사용`
  - 결정: Sphere, Quad, Box만 지원한다. 삼각형·BVH는 보류 항목이며 교차는 `intersect(ray) → hit` 인터페이스로 분리한다.
  - 이유: 광 수송과 샘플링 정확성에 집중하고, 정답이 알려진 표준 장면으로 변수를 통제하기 위해서다.
  - 영향: `Design/RENDERER_SPEC.md` 3.1, `Design/IMPLEMENTATION_PLAN.md` 보류 항목

- `[2026-10-13] DXR 대신 compute shader`
  - 결정: 경로 추적은 D3D12 compute shader에서 직접 교차 판정한다.
  - 이유: 해석적 도형에는 하드웨어 가속 구조가 필요 없고, API 준비 작업보다 적분기 구현에 집중하기 위해서다.
  - 영향: `docs/ARCHITECTURE.md` 4장

- `[2026-10-13] 문서 구조`
  - 결정: BASE_MD 템플릿을 렌더러용으로 바꾼다. `GDD` → `Design`, `PLAY_REQUEST` → `WORK_REQUEST`, `GAME_RULES` → `RENDERING_MODEL`. 마일스톤은 요청서와 같은 항목 구조의 카드로 작성한다.
  - 이유: 게임용 용어를 정리하고, 카드를 복사하는 것만으로 작업 요청서가 되게 하기 위해서다.
  - 영향: 모든 문서, `docs/WORKFLOW.md`

- `[2026-10-13] 학습 노트는 예외 자료`
  - 결정: `PATH_TRACING_STUDY.md`는 `Study/`에 두고 마일스톤 카드·요청서·문서 갱신 대상에서 제외한다.
  - 이유: 개인 학습 자료를 작업 흐름과 분리하고, AI가 구현 중 긴 이론 문서를 읽지 않게 하기 위해서다.
  - 영향: `docs/AI_USAGE.md`, `docs/WORKFLOW.md`, `Design/IMPLEMENTATION_PLAN.md`

- `[2026-10-13] 기본값에서 편향 처리 금지`
  - 결정: firefly clamp 등 편향을 만드는 처리는 기본으로 끄고, 쓰더라도 UI에 `biased`로 표시한다.
  - 이유: 참조 이미지 대비 수렴 검증(V-CONVERGENCE)이 프로젝트의 핵심 근거이기 때문이다.
  - 영향: `docs/RENDERING_MODEL.md` 11장, `docs/VERIFY.md`
