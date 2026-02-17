# Task 301: VCS DPI-C co-simulation

> **목표**: gen-dpic 산출물로 VCS와 lock-step co-simulation 검증
> **예상 시간**: 2일
> **산출물**: VCS 빌드 스크립트, SV lock-step 테스트벤치, simv
> **게이트**: VCS lock-step 10시드×1000cyc PASS
> **피드백 루프**: DPI-C 인터페이스 문제 → 103-gen-dpic

---

## 목표

hirct-gen이 생성한 dpi/ 디렉터리의 gen-dpic 산출물을 사용해 VCS DPI-C co-simulation을 수행한다. RTL과 C++ 모델을 lock-step으로 비교하는 SV 테스트벤치를 작성하고, 10시드×1000사이클 검증이 PASS인지 확인한다.

## 주요 작업

- gen-dpic 산출물 생성 (dpi/ 디렉터리)
- DPI-C g++ 단독 컴파일 검증
- VCS 경로·버전 확인
- RTL + DPI-C + TB 디렉터리 구성
- SV lock-step 테스트벤치 작성 (DPI-C 로드, 매 사이클 RTL vs DPI 출력 비교)
- VCS 빌드 (RTL + DPI-C + TB)
- simv 10시드×1000사이클 실행 및 PASS 검증

## VCS X 처리 전략

VCS X 처리는 전역 기본값(리셋 후 10사이클 warmup)으로 시작. 실패 모듈에 대해서만 개별 오버라이드.

## 게이트 (완료 기준)

- [ ] `g++ -std=c++17 -c output/.../dpi/*dpi*.cpp` → exit 0 (DPI-C 단독 컴파일)
- [ ] `vcs -sverilog -cpp g++ -CFLAGS "-std=c++17" <RTL> <DPI-C> <TB>` → simv 빌드 성공
- [ ] `./simv` → `PASS: 1000 pass, 0 fail` (10시드 전체) 메시지 출력 + exit 0
- [ ] 실패 시 `103-gen-dpic`로 되돌림 문서화

---

## 3자 비교 진단 (Verilator 버그 추적)

Phase 1~2에서 hirct-gen ≠ Verilator인 mismatch가 있었다면, VCS를 추가 레퍼런스로 사용하여 원인을 특정합니다:

| hirct-gen | Verilator | VCS | 진단 | 조치 |
|-----------|-----------|-----|------|------|
| ≠ | = | = | **hirct-gen 버그** | 101 gen-model 수정 |
| = | ≠ | = | **Verilator 버그** | XFAIL 등록 + Verilator 이슈 리포트 |
| ≠ | ≠ | = | 공통 가정 실패 | hirct-gen 모델 재검토 |

**절차**:
1. Phase 2의 `verify-report.json`에서 FAIL 모듈 목록 추출
2. 해당 모듈에 대해 VCS DPI-C co-sim 실행
3. 3자 비교 결과를 `known-limitations.md`에 분류 기록
