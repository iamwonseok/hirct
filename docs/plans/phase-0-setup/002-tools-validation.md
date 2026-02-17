# Task 002: 외부 도구 체인 Validation

> **목표**: CIRCT, Verilator, g++ 등 외부 도구 체인이 정상 동작함을 확인한다
> **예상 시간**: 0.5일
> **산출물**: Pre-test 결과 로그

---

## 목표

Phase 1에서 hirct-gen/hirct-verify를 **처음부터 작성**하기 전에, 외부 도구 체인(circt-verilog, verilator, g++, python3)이
실제 RTL 입력에서 정상 동작하는지 검증한다. 이 태스크는 hirct-gen 바이너리를 요구하지 않는다.

> **참고**: hirct-gen/hirct-verify 바이너리 빌드 및 동작 검증은 Phase 1 Bootstrap(Task 100)에서 수행한다.
> 기존 EmitCppModel.cpp, verify_levelgateway 등은 이 저장소에 존재하지 않으며,
> Phase 1에서 CIRCT 스타일 v2 구조로 신규 작성된다.

## 전제 조건

- Task 001(도구 설치 + 환경 검증) 완료
- CIRCT/LLVM 빌드 완료 (`$CIRCT_BUILD` 환경변수 설정됨, PATH에 추가됨)

## 주요 작업

### 1단계: CIRCT 파이프라인 동작 확인

```bash
# Verilog → MLIR 변환
circt-verilog rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v \
  -o /tmp/hirct-pretest/LevelGateway.mlir
test -s /tmp/hirct-pretest/LevelGateway.mlir  # 비어있지 않음

# CIRCT 계층 인라인(flatten) pass 동작 확인
#
# Note (실측): CIRCT `5e760efa9` 기준 `circt-opt --flatten` 옵션은 존재하지 않는다.
# 계층 인라인은 `--hw-flatten-modules` pass로 수행한다.
circt-opt --hw-flatten-modules /tmp/hirct-pretest/LevelGateway.mlir \
  -o /tmp/hirct-pretest/LevelGateway_hw_flat.mlir

# (추가) multi-file + --top 실측 (단일 파일 unknown module 문제 재현/해소 확인)
# Note: RTL 파일들에 timescale 정의가 섞여 있을 수 있으므로 기본 timescale을 명시한다.
circt-verilog --timescale=1ns/1ps --top=Fadu_K2_S5_AXI4Buffer \
  rtl/plat/src/s5/design/*.v \
  -o /tmp/hirct-pretest/AXI4Buffer_multifile_ts.mlir
test -s /tmp/hirct-pretest/AXI4Buffer_multifile_ts.mlir
```

### 2단계: Verilator RTL 모델 빌드 확인

```bash
# Verilator로 RTL → C++ 시뮬레이션 모델 빌드 가능 여부
verilator --cc rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v \
  -Mdir /tmp/hirct-pretest/obj_dir --build
test -f /tmp/hirct-pretest/obj_dir/VFadu_K2_S5_LevelGateway__ALL.a  # 라이브러리 존재

# 두 번째 모듈도 확인
verilator --cc rtl/plat/src/s5/design/Fadu_K2_S5_RVCExpander.v \
  -Mdir /tmp/hirct-pretest/obj_dir_rvc --build
```

### 3단계: C++ 컴파일러 기능 확인

```bash
# C++17 컴파일 + Verilator 헤더 include 가능 여부
cat > /tmp/hirct-pretest/test_compile.cpp << 'EOF'
#include <cstdint>
#include <string>
#include <vector>
#include <array>
struct TestModel {
    uint32_t io_in;
    uint32_t io_out;
    void do_reset() { io_out = 0; }
    void step() { io_out = io_in; }
};
int main() { TestModel m; m.do_reset(); m.step(); return 0; }
EOF
g++ -std=c++17 -c /tmp/hirct-pretest/test_compile.cpp -o /tmp/hirct-pretest/test_compile.o
```

### 4단계: Python 의존성 확인

```bash
python3 -c "import json, pathlib, subprocess, concurrent.futures, argparse, dataclasses; print('OK')"
```

### 5단계 (선택): VCS 접근 확인

```bash
vcs -ID 2>/dev/null || echo "WARN: VCS not available (Phase 3 co-sim will be skipped)"
```

## 게이트 (완료 기준)

- [ ] `circt-verilog` → LevelGateway.v MLIR 변환 성공
- [ ] `circt-verilog` → RVCExpander.v MLIR 변환 성공
- [ ] `verilator --cc` → LevelGateway RTL 모델 빌드 성공
- [ ] `verilator --cc` → RVCExpander RTL 모델 빌드 성공
- [ ] `g++ -std=c++17` → 테스트 C++ 파일 컴파일 성공
- [ ] `python3` → 필수 stdlib 모듈 import 성공
- [ ] (선택) `vcs -ID` → 버전 출력

---

## 변경 이력

| 날짜 | 내용 |
|------|------|
| 2026-02-15 | 초안 작성 |
| 2026-02-16 | v2 구조 반영: `tools/hirct` 래퍼 참조 제거 (Phase 1에서 hirct-gen/main.cpp로 흡수), `tools/tests/` 12개 스크립트 참조 제거 (lit으로 전환), `output/compile/` 캐시 참조를 v2 출력 구조로 정리 |
| 2026-02-16 | dry-run 반영: hirct-gen/verify_* 바이너리 의존 제거 (미존재), 외부 도구 체인 검증으로 범위 축소 |
