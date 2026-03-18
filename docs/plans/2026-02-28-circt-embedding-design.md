# CIRCT 내장 아키텍처 전환 설계

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**날짜**: 2026-02-28
**상태**: IN-PROGRESS (Phase A/B 완료, Phase C LLHD 갭 발견)
**결정**: 외부 프로세스 호출 + 텍스트 파싱 → CIRCT 라이브러리 직접 링크 + MLIR API 순회

---

## 1. 전환 동기

### 1.1 현재 아키텍처의 한계

현행 HIRCT는 CIRCT를 외부 프로세스(`circt-verilog`, `circt-opt`)로 호출하고, stdout MLIR 텍스트를 82개 이상의 정규식으로 파싱한다.

| 문제 | 증거 |
|------|------|
| MLIR 파싱 성공률 36.7% | Phase 2 실측: 1,597개 중 586개만 성공. 실패 98.3%가 `unknown module` |
| 텍스트 파싱 정보 손실 | SSA use-def chain 추적 불가. 타입 정보 부분 파싱만 가능 |
| 계층 클럭 전파 분석 불가 | old_ssa 타이밍 이슈: UART IP Phase D FIFO TX 며칠째 FAIL |
| 정규식 유지보수 부담 | ModuleAnalyzer 1,812줄, 새 IR 패턴마다 정규식 추가 필요 |
| CIRCT pass 활용 불가 | canonicalize, flatten 등을 외부 프로세스로만 호출 가능 |

### 1.2 전환 후 기대 효과

| 현재 이슈 | CIRCT 내장 후 |
|-----------|--------------|
| `unknown module` 98.3% | `importVerilog()`에 `-y`/`-v` 옵션 직접 전달. SourceMgr에 다중 파일 로드 |
| SSA 문자열 매칭으로 의존성 추적 | `Value::getDefiningOp()`, `Value::getUsers()`로 정확한 use-def chain |
| 레지스터 클럭 소스 추정 (포트 이름 패턴) | `FirRegOp::getClk()` → `seq::ToClockOp` → 입력 포트까지 정확 추적 |
| eval_comb 인스턴스 순서 문제 | `hw::InstanceOp`의 operand→result 관계로 정확한 topo sort |
| old_ssa 타이밍: 계층 클럭 전파 경로 미파악 | InstanceOp 포트 매핑 + use-def chain으로 클럭 전파 경로 정밀 분석 |
| 타입 파싱 실패 (`!hw.array<NxT>`) | `Type::cast<hw::ArrayType>()` → `getSize()`, `getElementType()` |
| 82+ 정규식 유지보수 | `Operation::walk()` + `dyn_cast<OpType>()` — 새 op 추가 시 파서 수정 불필요 |

---

## 2. 새 아키텍처

### 2.1 파이프라인 비교

```
[현재 — 레거시]
  .v → fork(circt-verilog) → MLIR 텍스트 stdout
     → ModuleAnalyzer(regex 82+) → PortInfo/OpInfo/InstanceInfo
     → GenModel/GenTB/... (자체 구조체 사용)

[변경 — MLIR 내장]
  .v → importVerilog() (in-process) → mlir::ModuleOp (인메모리 IR)
     → populateLlhdToCorePipeline (hw/comb/seq로 부분 lowering)
     → Emitter가 mlir::Operation을 직접 순회
     ※ LLHD 잔존: always 블록 → llhd.process + cf.cond_br/cf.br
       (UART 기준 31 process, 150 cond_br, 66 br)
       → GenModel이 MLIR API로 직접 flatten/emit 필요
```

#### 전환 단계 및 검증 게이트

| 단계 | 대응 Phase | 핵심 산출물 | 검증 게이트 |
|------|-----------|-----------|-----------|
| 빌드 인프라 구축 | Phase A | CMake + CIRCT 링크 | `cmake + ninja` exit 0, 기존 코드 그대로 빌드 | **완료** |
| IR 분석 계층 구축 | Phase B | IRAnalysis API (VerilogLoader + PortView + 레지스터/클럭/topo/메모리) | gtest 전체 PASS | **완료** |
| GenModel 재작성 (hw/comb/seq) | Phase C-core | MLIR 기반 eval_comb/step/save_old_ssa | LevelGateway 산출물 diff, lit PASS | **완료** |
| GenModel LLHD 처리 | Phase C-llhd | llhd.process flatten + sig/prb/drv emit | LLHD fixture lit + UART 11/11 | **진행 중** |
| 나머지 Emitter 전환 | Phase D | 8종 emitter MLIR API 사용 (dual path 구현됨) | lit 전체 PASS, 산출물 diff | **완료** |
| CLI 통합 | Phase E | hirct-gen/hirct-verify VerilogLoader 사용 | E2E PASS | **완료** |
| 레거시 전면 제거 + 검증 | Phase F | ModuleAnalyzer/CirctRunner 삭제, `use_mlir_` 분기 제거 | UART 11/11 + lit 전체 + 빌드 클린 |

각 Phase 완료 시 다음 Phase로 진행하기 전에 해당 검증 게이트를 통과해야 한다.

### 2.2 삭제/교체 대상

| 컴포넌트 | 라인 수 | 처리 |
|---------|--------|------|
| `CirctRunner.cpp` | ~300 | **삭제** → `importVerilog()` 인라인 호출 |
| `ModuleAnalyzer.cpp` 정규식 파싱 | ~1,500 | **삭제** → MLIR API 순회 |
| `ModuleAnalyzer.h` 자체 구조체 | ~200 | **삭제** → MLIR 네이티브 타입 사용 |
| 자체 topo sort (`sort_instances_topologically`) | ~260 | **교체** → MLIR/CIRCT 그래프 분석 유틸리티 |

### 2.3 보존/이관 대상

| 컴포넌트 | 처리 |
|---------|------|
| 분석 로직의 **의미** (클럭 도메인, 버스 감지, 레지스터 탐지) | 로직 보존, 구현만 MLIR API로 교체 |
| Emitter **출력 포맷** (C++/SV/Python 코드) | 동일 유지 |
| lit/gtest 테스트 | 기존 테스트 그대로 통과해야 함 |
| Makefile 오케스트레이션 | 동일 유지 |
| CLI 인터페이스 (hirct-gen/hirct-verify) | 동일 유지 |

### 2.4 빌드 시스템 변경

```cmake
# 현재
project(hirct LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
# LLVM/MLIR 링크 없음

# 변경 후
project(hirct LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

set(CIRCT_DIR "$ENV{CIRCT_BUILD}/lib/cmake/circt" CACHE PATH "CIRCT CMake dir")
set(MLIR_DIR "$ENV{CIRCT_BUILD}/lib/cmake/mlir" CACHE PATH "MLIR CMake dir")
set(LLVM_DIR "$ENV{CIRCT_BUILD}/lib/cmake/llvm" CACHE PATH "LLVM CMake dir")

find_package(LLVM REQUIRED CONFIG)
find_package(MLIR REQUIRED CONFIG)
find_package(CIRCT REQUIRED CONFIG)

# 필수 라이브러리
target_link_libraries(hirct-gen PRIVATE
    CIRCTImportVerilog    # Verilog → MLIR
    CIRCTHW               # hw dialect
    CIRCTComb             # comb dialect
    CIRCTSeq              # seq dialect
    CIRCTLLHD             # llhd dialect
    CIRCTSim              # sim dialect
    CIRCTSupport
    CIRCTTransforms       # canonicalize 등
    MLIRIR
    MLIRParser
    MLIRSupport
)
```

**주의**: `HIRCT_STATIC` (완전 정적 링크)는 LLVM/MLIR과 호환되지 않을 수 있음. 정적 링크 정책 재검토 필요.

---

## 3. Emitter 전환 전략

### 3.1 난이도별 분류

| 난이도 | Emitter | 라인 수 | MA 의존도 | 전환 전략 | 전환 순서 | 선행 의존 |
|--------|---------|---------|----------|----------|----------|----------|
| **HIGH** | GenModel | 2,450 | 전체 (OpInfo, InstanceInfo, MemoryInfo, LLHD 등) | 전면 재작성 | Phase C | Phase B 전체 |
| **MED** | GenTB | 514 | ports + 버스 감지 + 레지스터 맵 | 분석 로직 MLIR API 교체 | Phase D-4 | B-2, B-3 |
| **MED** | GenDPIC | 521 | ports + 클럭 도메인 맵 | 클럭 분석 MLIR API 교체 | Phase D-5 | B-2, B-4 |
| **MED** | main.cpp | 879 | MultiModuleContext 오케스트레이션 | MLIR ModuleOp 직접 사용 | Phase E | Phase C, D 완료 |
| **LOW** | GenDoc | 233 | ports + detect_registers | 얇은 어댑터 | Phase D-3 | B-2, B-3 |
| **LOW** | GenVerify | 214 | ports만 | 포트 API만 교체 | Phase D-1 | B-2 |
| **LOW** | GenWrapper | 127 | ports + prefix 그룹 | 포트 API만 교체 | Phase D-1 | B-2 |
| **LOW** | GenFormat | 374 | ports + has_registers | 포트 API만 교체 | Phase D-3 | B-2, B-3 |
| **LOW** | GenRAL | 228 | detect_registers만 | 레지스터 분석만 교체 | Phase D-3 | B-3 |
| **LOW** | GenCocotb | 154 | ports만 | 포트 API만 교체 | Phase D-1 | B-2 |
| **LOW** | GenMakefile | 100 | module_name만 | 거의 변경 없음 | Phase D-1 | 없음 |

**병렬화 참고**: D-1(LOW) 그룹과 D-3(LOW-MED) 그룹은 선행 의존이 다르므로 Phase B 완료 후 동시 진행 가능.

### 3.2 공통 유틸리티 계층

Emitter들이 공통으로 필요한 분석 기능을 MLIR API 기반 유틸리티로 제공:

```cpp
// hirct/include/hirct/Analysis/IRAnalysis.h (신규)
namespace hirct {

struct PortView {
  llvm::StringRef name;
  hw::PortDirection direction;
  mlir::Type type;
  unsigned width;  // convenience: IntegerType의 width
};

std::vector<PortView> get_ports(hw::HWModuleOp module);
std::vector<PortView> get_input_ports(hw::HWModuleOp module);
std::vector<PortView> get_output_ports(hw::HWModuleOp module);

bool is_clock_port(const PortView &port);
bool is_reset_port(const PortView &port);
bool is_active_low_reset(const PortView &port);

unsigned get_type_width(mlir::Type type);
std::string cpp_type_for_width(unsigned width);

struct ClockDomainMap { ... };
ClockDomainMap build_clock_domain_map(hw::HWModuleOp module);

struct RegisterInfo {
  seq::FirRegOp op;
  mlir::Value clock;
  mlir::Value reset;
  mlir::Value resetValue;
  bool isAsync;
  unsigned width;
};
std::vector<RegisterInfo> collect_registers(hw::HWModuleOp module);

std::vector<hw::InstanceOp> sort_instances_topologically(hw::HWModuleOp module);

}  // namespace hirct
```

이 유틸리티 계층은 기존 `ModuleAnalyzer`의 공개 API를 대체하되, 내부 구현은 MLIR API로 전환한다.

---

## 4. 현재 이슈별 개선 비교

### 4.1 Phase D FIFO TX 실패 (old_ssa 타이밍)

**현재 문제**:
```
DW_apb_uart::step_pclk() → eval_comb() → PCLK=1을 BCM57.clk에 전파
BCM57::step() → old_ssa_clk = clk = 1 (이미 오염)
→ rising edge 미감지 → FIFO push 실패
```

**근본 원인**: GenModel이 인스턴스 경계를 넘는 클럭 전파 순서를 파악하지 못해, `save_old_ssa()`의 호출 시점이 잘못됨.

**CIRCT 내장 후 해결 경로**:
```cpp
// 1. hw::InstanceOp의 입력 포트에서 클럭 연결 추적
auto instance = dyn_cast<hw::InstanceOp>(op);
for (auto [idx, operand] : llvm::enumerate(instance.getOperands())) {
  // operand가 클럭 신호인지 확인
  if (auto toClk = operand.getDefiningOp<seq::ToClockOp>()) {
    // 이 인스턴스는 부모의 클럭을 받음
    // → save_old_ssa()를 eval_comb() 전에 재귀 호출해야 함
  }
}

// 2. 하위 모듈의 레지스터가 이 클럭을 사용하는지 확인
hw::HWModuleOp childModule = symbolTable.lookup<hw::HWModuleOp>(instance.getModuleName());
childModule.walk([&](seq::FirRegOp reg) {
  Value clk = reg.getClk();
  // clk이 포트 인자인지, 어떤 포트인지 추적 가능
});
```

### 4.2 eval_comb 인스턴스 순서 (topo sort vs deferred resolution)

**현재 문제**: deferred resolution 패턴이 pre-register로 인해 모든 인스턴스를 즉시 emit → topo sort 순서 무시

**CIRCT 내장 후 해결 경로**: `hw::InstanceOp`의 operand/result use-def chain으로 정확한 의존성 그래프를 구축하여, pre-register 없이 연산 순서를 결정한다. 구현 상세는 구현 계획의 Task C-3을 참조.

### 4.3 멀티클럭 도메인 step 분리

**현재 문제**: `build_clock_domain_map()`이 `seq.to_clock` 체인을 최대 8 depth까지 텍스트 역추적. 포트 이름 패턴으로 클럭 판별.

**CIRCT 내장 후 해결 경로**: `seq::FirRegOp::getClk()` → `getDefiningOp()` 체인을 depth 무제한 추적하여 `BlockArgument`(포트)까지 도달. 포트 인덱스로 정확한 클럭 포트를 식별한다. 구현 상세는 구현 계획의 Task B-4를 참조.

### 4.4 LLHD process 잔존 (populateLlhdToCorePipeline 불완전 lowering)

**현재 문제**: 초기 설계 시 `populateLlhdToCorePipeline`이 Verilog `always` 블록을 완전히 hw/comb/seq로 lowering할 것으로 가정했으나, 실측 결과 UART 기준 31개 `llhd.process` + 150개 `cf.cond_br` + 66개 `cf.br`이 IR에 잔존한다.

**영향**: MLIR emit 경로(`emit_eval_comb_mlir` 등)가 LLHD ops를 처리하지 않으면 해당 always 블록의 로직이 누락되어 UART standalone test 실패.

**CIRCT 내장 후 해결 경로**: GenModel에서 LLHD ops를 MLIR API로 직접 처리:
- `llhd::SignalOp` → 멤버 변수 바인딩
- `llhd::ProbeOp` → signal 값 참조
- `llhd::ProcessOp` → Region/Block CFG를 재귀 순회하여 C++ 표현식으로 flatten
- `llhd::DriveOp` → signal 쓰기 코드 생성
- `cf::CondBranchOp` → 삼항 mux, `cf::BranchOp` → passthrough

레거시 경로에 이미 완성된 구현(`flatten_process_body`, `flatten_from_block`, `try_unroll_loop`)이 있으며, 이를 MLIR API 기반으로 재구현한다. 구현 상세는 구현 계획의 Task C-6~C-8을 참조.

### 4.5 unknown module 파싱 실패 (36.7% 성공률)

**현재 문제**: `circt-verilog`를 단일 파일로 호출 시 의존 모듈 미해결

**CIRCT 내장 후 해결 경로**: `importVerilog()`에 `SourceMgr`로 다중 파일 직접 로드하고, `ImportVerilogOptions::libDirs`에 라이브러리 경로를 전달한다. 파싱 실패 시 CIRCT의 진단 메시지를 in-process로 수신하여 정확한 에러 리포트를 제공한다. 구현 상세는 구현 계획의 Task B-1을 참조.

---

## 5. 제약 사항 및 리스크

| 리스크 | 완화 방안 | 검증 절차 |
|--------|---------|----------|
| 빌드 시간 증가 (LLVM/MLIR/CIRCT 전체 빌드 전제) | CIRCT는 이미 빌드되어 있음. hirct 자체 빌드만 링크 시간 추가 (~수십 초) | Phase A에서 `cmake -DCIRCT_BUILD=...` 실행 시 CIRCT 발견 확인. 미발견 시 `utils/setup-env.sh` 안내 |
| CIRCT 버전 업데이트 시 API 변경 | `tool-versions.env`의 CIRCT_COMMIT 고정. 업데이트는 명시적 마이그레이션 | CI에서 `git -C $CIRCT_SRC rev-parse HEAD`로 커밋 해시 검증 |
| 바이너리 크기 증가 | 정적 링크 시 수십~수백 MB. 동적 링크 옵션 제공 | Phase A 완료 후 `ls -lh hirct/build/bin/hirct-gen`으로 크기 기록 |
| `HIRCT_STATIC` 정적 링크 호환성 | LLVM은 정적 라이브러리 빌드 지원. 단, 일부 플러그인 로딩은 동적 링크 필요 | Phase A에서 `HIRCT_STATIC=ON` 빌드 시도 → 실패 시 OFF로 전환, `open-decisions.md`에 기록 |
| 기존 lit 테스트 회귀 | 출력 포맷이 동일하므로 FileCheck 패턴 유지. CI에서 회귀 확인 | 매 Phase 완료 시 `ninja -C hirct/build check-hirct` 실행, 46/46 확인 |
| importVerilog API 시그니처 불일치 | 빌드된 CIRCT 소스의 헤더를 직접 참조하여 시그니처 확인 | Phase B-1에서 VerilogLoader 빌드 성공 여부로 즉시 검증 |
| LLHD 불완전 lowering (always → llhd.process 잔존) | GenModel에서 MLIR API로 직접 flatten/emit. 레거시 구현 참조하여 포팅 | Phase C-llhd에서 LLHD fixture lit + UART standalone 11/11로 검증 |
| 레거시 전면 제거 시 emitter 회귀 | 모든 emitter가 dual path(MLIR + legacy) 보유. F-1 전체 PASS 후에만 삭제 | `grep -rc 'ModuleAnalyzer\|CirctRunner\|dummy_analyzer' hirct/` → 0 |

---

## 6. 성공 기준

### 6.1 정량 KPI

| # | 지표 | 검증 명령 | 기대값 | 허용 오차 |
|---|------|---------|--------|----------|
| 1 | lit 테스트 | `ninja -C hirct/build check-hirct` | 46/46 PASS | 0 (전체 통과 필수) |
| 2 | gtest | `ninja -C hirct/build check-hirct-unit` | 전체 PASS | 0 |
| 3 | UART standalone | `cd examples/fc6161/pt_plat/cosim/uart_top && ./cmodel_standalone_test` | 11/11 PASS | 0 |
| 4 | VCS equiv | `cd examples/fc6161/pt_plat && make run-equiv-uart-vcs` | 26 checks, 0 mismatches | 0 |
| 5 | 산출물 일관성 | `diff -r output/old output/new --exclude='*.timestamp'` | 의미 있는 차이 0건 (날짜/경로 제외) | 헤더 주석의 날짜/경로만 허용 |

### 6.2 구조 기준

| # | 기준 | 확인 방법 |
|---|------|---------|
| 6 | 8종 산출물 생성 | `hirct-gen rtl/LevelGateway.v` 실행 후 `.h`, `.cpp`, `_tb.sv`, `_dpi.sv`, `_wrapper.sv`, `_cocotb.py`, `Makefile`, `_doc.md` 존재 확인 |
| 7 | `CirctRunner.cpp` 삭제 | `test ! -f hirct/lib/Support/CirctRunner.cpp` |
| 8 | 정규식 파싱 코드 제거 | `grep -c 'std::regex' hirct/lib/Analysis/ModuleAnalyzer.cpp` → 0 (또는 파일 삭제) |
| 9 | importVerilog 직접 호출 | `grep -c 'importVerilog' hirct/lib/Support/VerilogLoader.cpp` → 1 이상 |
| 10 | 레거시 코드 완전 제거 | `grep -rc 'ModuleAnalyzer\|CirctRunner\|dummy_analyzer\|use_mlir_' hirct/lib/ hirct/include/ hirct/tools/` → 0 |
| 11 | LLHD process 처리 | LLHD process 포함 fixture에서 올바른 old_/sig_/flatten 출력 확인 (lit PASS) |

---

## 변경 이력

| 날짜 | 내용 |
|------|------|
| 2026-02-28 | 초안 작성 |
| 2026-02-28 | 품질 보완: 섹션 2.1에 전환 단계/검증 게이트 추가, 섹션 3.1에 전환 순서/의존성 추가, 섹션 4 코드 예시 축소(YAGNI), 섹션 5 리스크 표에 검증 절차 열 추가, 섹션 6 성공 기준을 정량 KPI로 재작성 |
| 2026-03-01 | LLHD 갭 반영: (1) 상태를 IN-PROGRESS로 변경 (2) 섹션 2.1 파이프라인에 LLHD 잔존 사실 추가, 전환 단계 표에 Phase C-llhd 분리 및 진행 상태 열 추가 (3) 섹션 4.4 LLHD process 잔존 이슈 신규 추가 (기존 4.4→4.5 번호 이동) (4) 섹션 5 리스크에 LLHD 불완전 lowering, 레거시 전면 제거 리스크 추가 (5) 섹션 6 구조 기준에 레거시 완전 제거·LLHD 처리 항목 추가 |
