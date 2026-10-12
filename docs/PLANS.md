# Plans

현재 상태와 다음 작업을 관리합니다. 계획 자체는 `Design/IMPLEMENTATION_PLAN.md`, 완료 이력은 커밋을 기준으로 합니다.

상태: `대기` / `진행중` / `구현 완료(사용자 검증 대기)` / `완료` / `보류` / `확인 필요`

- `구현 완료(사용자 검증 대기)`: 코드와 이 환경에서 가능한 검증은 끝났지만, Windows MSVC 빌드·GPU 실행이 필요한 V-ID가 남은 상태. 남은 V-ID는 비고에 적는다.

## 현재 마일스톤

- M3 광원 샘플링 — `대기` (사용자 지시로 M2 이후 작업 중단)

## 마일스톤 상태

| ID | 주제 | 상태 | 비고 |
| --- | --- | --- | --- |
| M0 | 골격 | 구현 완료(사용자 검증 대기) | 남은 검증: V-BUILD(MSVC), V-RESIZE. llvm-mingw 컴파일·링크, DXC 셰이더 컴파일 통과 |
| M1 | 첫 레이 | 구현 완료(사용자 검증 대기) | KernelTool 테스트로 V-NORMAL, V-SCENE 통과(CPU). 남은 검증: V-BUILD(MSVC), GPU 화면의 Normal/Albedo/Depth 확인 |
| M2 | 누적과 확산 | 구현 완료(사용자 검증 대기) | KernelTool로 V-DETERMINISM, V-NAN(Cornell 1024 spp, 비유한 0개) CPU 통과. 남은 검증: V-BUILD(MSVC), V-RESET, GPU의 V-DETERMINISM·V-NAN |
| M3 | 광원 샘플링 | 대기 | 참조 렌더러·저장 포맷 결정 필요 |
| M4 | 재질 확장 | 대기 | |
| M5 | MIS | 대기 | |
| M6 | 환경맵 | 대기 | HDR 로더 라이브러리 결정 필요 |
| M7 | 결과 정리 | 대기 | |

## 다음 작업

1. 사용자: `PathTracing.slnx`를 Debug/Release x64로 빌드·실행해 M0~M2의 남은 V-ID 확인
2. M3 카드를 `docs/WORK_REQUEST.md`에 복사해 시작 (참조 렌더러·HDR 저장 포맷 결정 필요)

## 확인 필요

| 항목 | 선택지 | 필요 시점 |
| --- | --- | --- |
| 참조 렌더러 | Mitsuba 3(pip 설치, Python으로 장면 작성) / PBRT-v4 / Blender Cycles | M3 |
| HDR 저장 포맷 | PFM(구현 단순) / EXR(도구 호환, 라이브러리 필요) | M3 |
| HDR 환경맵 로더 | stb_image(.hdr) / 직접 구현 | M6 |

## 보류

`Design/IMPLEMENTATION_PLAN.md`의 보류 항목 참고.

## 최근 메모

- 2026-10-13: 문서 구조 작성.
- 2026-10-13: M2 누적과 확산 구현. 사용자 지시로 M2에서 연속 진행 종료.
- 2026-10-13: M1 첫 레이 구현. 커널을 C++로도 컴파일하는 `Tests/KernelTool` 추가.
- 2026-10-13: M0 골격 구현. 셰이더 컴파일러는 FXC로 결정(DECISIONS). git은 사용자가 만든 `AACHANJINAA/PathTracing` 저장소 사용.
