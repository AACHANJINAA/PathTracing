# AI Usage

AI가 문서를 적게 읽고 필요한 범위만 확인하기 위한 규칙입니다.

## 기본 원칙

- 모든 문서를 자동으로 읽지 않습니다.
- `docs/WORK_REQUEST.md`를 먼저 확인합니다.
- 요청서의 `참조할 문서`가 있으면 그 문서만 우선 읽습니다.
- 긴 문서는 전체를 읽기보다 제목·키워드로 관련 섹션만 찾습니다.
- 관련성이 불분명한 문서는 읽기 전에 사용자에게 후보와 이유를 알립니다.

## 작업 유형별 우선 참조 (요청서에 참조 문서가 없을 때)

| 작업 유형 | 우선 참조 |
| --- | --- |
| 사양 확인·변경 | `Design/RENDERER_SPEC.md` |
| 마일스톤 계획 | `Design/IMPLEMENTATION_PLAN.md`, `docs/PLANS.md` |
| 코드 구현 | 관련 코드, 필요 시 `docs/ARCHITECTURE.md` |
| 셰이더·샘플링·BSDF | `docs/RENDERING_MODEL.md` |
| 버그 수정 | 관련 코드, `docs/VERIFY.md`의 실패한 V-ID |
| 구조 변경 | `docs/ARCHITECTURE.md` |
| 규약 변경(좌표계, 단위, 색 공간) | `docs/RENDERING_MODEL.md` |
| 검증 | `docs/VERIFY.md` |
| 과거 결정 확인 | `docs/DECISIONS.md` → 부족하면 커밋 기록 |
| 완료 이력 확인 | 커밋 기록, `docs/PLANS.md` |

## 읽지 않는 것

- `Study/`는 **예외 자료**입니다. 작업 흐름에 포함되지 않으며, 사용자가 직접 지시한 경우에만 읽거나 수정합니다.
- `Captures/`, `Reference/`의 이미지는 검증 작업에서만 확인합니다.

## 사용자 확인이 필요한 경우

- 작업 유형이나 범위가 불명확할 때
- `수정 허용 범위`를 넘는 코드·문서를 바꿔야 할 때
- `docs/RENDERING_MODEL.md`의 규약과 충돌하는 구현이 필요할 때
- 새 폴더, 새 외부 라이브러리, 새 문서가 필요할 때
- 검증이 실패했는데 원인이 사양·규약 쪽일 가능성이 있을 때
