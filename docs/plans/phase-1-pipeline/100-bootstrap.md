# Task 100: Bootstrap — 빌드 시스템 + 최소 파이프라인 스켈레톤

> **목표**: CMakeLists.txt, ModuleAnalyzer, 최소 GenModel, CLI main.cpp를 작성하여 hirct-gen이 빌드되고 `--help`를 출력하는 상태까지 도달
> **예상 시간**: 3일
> **산출물**: CMakeLists.txt (루트 + lib + tools), ModuleAnalyzer 최소 구현, GenModel 스켈레톤, main.cpp CLI
> **TDD**: RED → GREEN → REFACTOR
> **선행 조건**: Phase 0 완료 (외부 도구 검증 PASS)
>
> **왜 이 태스크가 필요한가**: 이 저장소에는 C++ 소스 코드가 아직 존재하지 않는다.
> 기존 EmitCppModel.cpp, EmitDPIC.cpp 등은 이전 프로토타입에서 참고할 수 있는 로직이지만,
> 현재 저장소에는 포함되어 있지 않다. 따라서 모든 C++ 코드를 처음부터 작성해야 하며,
> 이 Bootstrap 태스크에서 최소한의 빌드 가능한 스켈레톤을 만든다.

---

## 최종 산출물 목록

```
include/hirct/
├── Analysis/
│   └── ModuleAnalyzer.h
├── Support/
│   └── CirctRunner.h         (circt-verilog / circt-opt 외부 프로세스 호출)
└── Target/
    ├── GenModel.h
    └── GenMakefile.h

lib/
├── Analysis/
│   ├── ModuleAnalyzer.cpp
│   └── CMakeLists.txt
├── Support/
│   ├── CirctRunner.cpp       (외부 프로세스 래퍼: circt-verilog, circt-opt)
│   └── CMakeLists.txt
├── Target/
│   ├── GenModel.cpp          (최소: 빈 .h + .cpp 생성)
│   ├── GenMakefile.cpp       (최소: 빈 Makefile 생성)
│   └── CMakeLists.txt
└── CMakeLists.txt

tools/
├── hirct-gen/
│   ├── main.cpp              (CLI: input.v → MLIR → GenModel)
│   └── CMakeLists.txt
└── hirct-verify/
    ├── main.cpp              (최소: --help만 출력)
    └── CMakeLists.txt

test/
└── fixtures/
    ├── LevelGateway.mlir     (정규화된 MLIR 출력 — gtest 픽스처)
    └── RVCExpander.mlir

unittests/
└── Analysis/
    └── ModuleAnalyzerTest.cpp  (최소 gtest)

CMakeLists.txt                (루트)
```

---

## Step 1: RED — 빌드 시스템 없음 확인 (5분)

**Goal**: CMakeLists.txt가 존재하지 않음을 확인

**Run**:
```bash
test -f CMakeLists.txt && echo "EXISTS" || echo "NOT FOUND"
test -f lib/Target/GenModel.cpp && echo "EXISTS" || echo "NOT FOUND"
```

**Expect**:
```
NOT FOUND
NOT FOUND
```

---

## Step 2: GREEN — 디렉토리 구조 생성 (15분)

**Goal**: CIRCT 스타일 v2 디렉토리 구조 생성

**Run**:
```bash
mkdir -p include/hirct/{Analysis,Support,Target}
mkdir -p lib/{Analysis,Support,Target}
mkdir -p tools/{hirct-gen,hirct-verify}
mkdir -p test/{Target,Analysis,Tools/hirct-gen,fixtures}
mkdir -p unittests/{Analysis,Target}
mkdir -p integration_test/{smoke,hirct-gen,hirct-verify}
mkdir -p utils
mkdir -p config
```

**Expect**:
```
exit 0 (디렉토리 구조 생성)
```

---

## Step 3: GREEN — 루트 CMakeLists.txt 작성 (30분)

**Goal**: 순수 C++17 프로젝트로 CMake 빌드 시스템 구성 (LLVM/MLIR C++ API 사용 금지)

> **중요 설계 결정 1: MLIR 텍스트 파싱 방식**
>
> hirct-gen은 `circt-verilog`를 **외부 프로세스로 호출**하고, stdout으로 출력되는 MLIR 텍스트를
> **문자열/regex 기반으로 파싱**한다. LLVM/MLIR C++ API에 직접 링크하지 않는다.
>
> 이유:
> - MLIR C++ API 링크는 CMake 설정이 매우 복잡하고 빌드 시간이 크게 증가
> - `circt-verilog`의 MLIR 출력은 안정적인 텍스트 형식이며, regex로 충분히 파싱 가능
> - 순수 C++17 코드로 유지하면 빌드와 디버깅이 단순화됨
>
> 따라서 CMakeLists.txt에서 `find_package(MLIR/LLVM)`은 **사용하지 않는다**.

> **설계 결정 2: MLIR 입력 정규화 (선택적 안정화)**
>
> 실측 검증 결과, `circt-verilog`의 원본 출력은 이미 안정적인 정규화 형태이다
> (`circt-opt --canonicalize` 적용 전후 차이 없음, CIRCT `5e760efa9` 기준).
>
> 따라서 정규화 단계는 **필수가 아니라 선택적 안정화**이다:
>
> ```
> circt-verilog input.v → (MLIR, 이미 안정적) → ModuleAnalyzer
>                          ↑ 필요 시 circt-opt --canonicalize 삽입 가능
> ```
>
> - 현재 CIRCT 버전에서는 원본 직접 파싱으로 충분
> - CIRCT 버전 업데이트 시 출력 형태가 바뀔 경우에만 `--canonicalize` 파이프라인 활성화
> - CirctRunner에 `canonicalize` 옵션을 두되, 기본값은 `false`
> - 정규화 여부와 무관하게 `test/fixtures/`에 MLIR 픽스처를 저장하여 gtest에서 사용
>
> (근거: `docs/plans/risk-validation-results.md` §1 참조)

**Run**:
```bash
# CMakeLists.txt 생성 후 cmake 구성 테스트
cmake -B build -G Ninja
```

**Expect**:
```
-- Configuring done
-- Generating done
-- Build files have been written to: .../build
```

**CMakeLists.txt 핵심 구조**:

```cmake
cmake_minimum_required(VERSION 3.20)
project(hirct LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 순수 C++17 프로젝트 — LLVM/MLIR C++ API에 링크하지 않음
# circt-verilog를 외부 프로세스로 호출하고 MLIR 텍스트를 파싱한다
include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(lib)
add_subdirectory(tools)

# gtest 통합
include(FetchContent)
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0)
FetchContent_MakeAvailable(googletest)
enable_testing()
add_subdirectory(unittests)
```

---

## Step 3.5: GREEN — CirctRunner 외부 프로세스 래퍼 (1시간)

**Goal**: circt-verilog / circt-opt를 외부 프로세스로 호출하는 래퍼 구현

**CirctRunner.h 핵심 인터페이스**:

> **설계 원칙**: stdout/stderr 분리, 타임아웃, 대용량 출력 처리를 고려한다.

```cpp
#pragma once
#include <string>
#include <optional>

namespace hirct {

struct RunResult {
    int exitCode;
    std::string stdout;
    std::string stderr;
};

class CirctRunner {
public:
    /// circt-verilog 호출: input.v → MLIR 텍스트
    RunResult runCirctVerilog(const std::string& inputPath);

    /// circt-verilog multi-file 호출: 여러 .v → MLIR
    /// circt-verilog는 -f 미지원 → hirct-gen이 filelist를 파싱하여 인자로 전달
    /// multi-file 모드에서는 timescale을 항상 강제한다 (기본 1ns/1ps, 사용자 오버라이드 가능)
    RunResult runCirctVerilogMulti(const std::vector<std::string>& inputPaths,
                                   const std::string& topModule,
                                   const std::string& timescale = "1ns/1ps");

    /// circt-opt 호출: MLIR → 변환된 MLIR
    RunResult runCirctOpt(const std::string& mlirContent,
                          const std::vector<std::string>& passes);

    /// 타임아웃 설정 (초, 기본값 60)
    void setTimeout(int seconds) { timeout_ = seconds; }

    /// canonicalize 활성화 (기본값 false — 현재 CIRCT 출력이 이미 안정적)
    void setCanonicalize(bool enable) { canonicalize_ = enable; }

private:
    int timeout_ = 60;
    bool canonicalize_ = false;

    RunResult runProcess(const std::string& command);
};

} // namespace hirct
```

**구현 고려사항**:
- **대용량 출력**: `popen()` 대신 `pipe()` + `fork()` + `exec()`로 stdout/stderr 분리 읽기 (MLIR 출력이 수 MB 가능)
- **타임아웃**: `alarm()` 또는 `waitpid()` + `WNOHANG` 루프로 자식 프로세스 감시
- **에러 추출**: stderr에서 `error:` 패턴을 찾아 진단 메시지로 변환
- **pipe 버퍼 한계**: 64KB 기본 버퍼 초과 시 deadlock 방지를 위해 비동기 읽기 또는 임시 파일 사용

> **결정: 임시 파일 방식 채택**
>
> 대용량 MLIR 출력(수 MB)에서 pipe 버퍼(64KB) deadlock을 방지하기 위해 stdout을 임시 파일로 리다이렉트한다:
> 1. `mkstemp()`로 임시 파일 생성
> 2. `fork()` + `exec()` 시 stdout fd를 임시 파일 fd로 `dup2()`
> 3. 자식 프로세스 완료(`waitpid`) 후 임시 파일 전체를 `std::string`으로 읽기
> 4. 임시 파일 삭제 (`unlink`)
> 5. stderr는 별도 pipe로 읽기 (stderr는 진단만 포함하여 64KB 미만)
>
> 비동기 읽기(`select`/`poll`)보다 구현이 단순하고, MLIR 출력 크기에 제한이 없다.

**Run**:
```bash
ninja -C build
```

**Expect**:
```
lib/Support/CirctRunner.cpp 컴파일 성공
```

---

## Step 4: GREEN — ModuleAnalyzer 최소 구현 (2시간)

**Goal**: MLIR 파일을 파싱하고 포트 정보를 추출하는 최소 구현

**MLIR 텍스트 파싱 전략**:

> **핵심 원칙**: MLIR 텍스트를 3단계로 파싱한다. LLVM/MLIR C++ API를 사용하지 않는다.

파싱 단계:
1. **Module signature 추출**: `hw.module @Name(...)` 라인에서 모듈명과 포트 파싱
2. **Body 라인 수집**: `{` ~ `}` 사이의 모든 라인을 수집 (중괄호 깊이 추적)
3. **Op 라인 파싱**: 각 라인을 `%result = dialect.op %operands : type` 패턴으로 분해

핵심 regex 패턴:
```
Module:  hw\.module\s+@(\w+)\(([^)]*)\)
Port:    (in|out)\s+(%?\w+)\s*:\s*(i\d+|!hw\.\w+)
Op:      (%\w+)\s*=\s*([\w.]+)\s+(.+?)\s*:\s*(.+)
Const:   (%\w+)\s*=\s*hw\.constant\s+(.+?)\s*:\s*(i\d+)
Output:  hw\.output\s+(.+)
```

엣지 케이스 처리:
- `hw.module.extern @Name(...)` (body 없음): signature만 파싱, body 단계 스킵
- 다중 module: 첫 번째 `hw.module`(extern 아닌)을 사용, `--top`으로 선택 가능
- 중첩 타입 `!hw.struct<...>`: 비트폭 추출 시 fallback으로 0 반환 + 경고
- `sv.attributes` 등 비-연산 라인: 무시 (파싱 실패 시 skip, 경고만 출력)

**SSA 값 타입 테이블**:

ModuleAnalyzer는 파싱 중 **SSA 값별 타입 매핑**(`std::map<std::string, unsigned> ssaWidths_`)을 구축한다:
- 포트 파싱 시: `%port_name` → 비트폭 등록
- Op 파싱 시: `%result` → 결과 타입의 비트폭 등록
- 피연산자 참조 시: ssaWidths_ 테이블에서 비트폭 조회

이를 통해 GenModel이 `comb.extract`, `comb.concat`, `comb.replicate` 등에서 필요한 피연산자 비트폭을 항상 알 수 있다.

**ModuleAnalyzer.h 핵심 인터페이스**:

> **파싱 방식**: `circt-verilog`의 stdout MLIR 텍스트를 regex/문자열 기반으로 파싱한다.
> LLVM/MLIR C++ API는 사용하지 않는다.
>
> **MLIR 텍스트 예시** (LevelGateway):
> ```
> hw.module @Fadu_K2_S5_LevelGateway(in %clock : i1, in %reset : i1, in %io_interrupt : i1, out io_plic_valid : i1, ...) {
>   %true = hw.constant true
>   %0 = comb.and %io_interrupt, %io_plic_ready : i1
>   %inFlight = seq.firreg %9 clock %8 reset async %preset_flops, %false : i1
>   hw.output %2 : i1
> }
> ```

```cpp
#pragma once
#include <string>
#include <vector>
#include <map>

namespace hirct {

struct PortInfo {
    std::string name;
    std::string direction;  // "input" | "output" | "inout"
    unsigned width;          // 비트폭 (i1=1, i32=32 등)
};

struct OpInfo {
    std::string opName;        // "comb.and", "comb.xor", "seq.firreg" 등
    std::string resultName;    // SSA 결과 이름 ("%0", "%inFlight" 등)
    std::string resultType;    // "i1", "i32" 등
    std::vector<std::string> operands;  // 피연산자 이름 목록
    std::map<std::string, std::string> attributes;  // 속성 (sv.namehint 등)
};

struct ConstantInfo {
    std::string name;    // "%true", "%c115_i15" 등
    std::string type;    // "i1", "i15" 등
    std::string value;   // "true", "115" 등
};

class ModuleAnalyzer {
public:
    /// circt-verilog의 MLIR 텍스트 출력을 파싱
    explicit ModuleAnalyzer(const std::string& mlirContent);

    // 모듈 정보
    const std::string& getModuleName() const;
    bool isValid() const;

    // 포트 정보
    const std::vector<PortInfo>& getPorts() const;
    const std::vector<PortInfo>& getInputPorts() const;
    const std::vector<PortInfo>& getOutputPorts() const;

    // Operation 정보 (토폴로지 정렬 순서)
    const std::vector<OpInfo>& getOperations() const;
    /// 토폴로지 정렬 알고리즘: Kahn's Algorithm (BFS 기반)
    /// - seq.firreg/seq.compreg는 "끊는 점": 출력은 입력에 비의존 (조합 로직 기준)
    /// - combinational loop 감지: 정렬 후 미방문 노드 존재 → Error + 진단 메시지
    /// - 정렬 실패(= combinational loop) 시: **hard fail (ERROR)** 처리 (미정렬 op의 잘못된 emit 방지)
    const std::vector<ConstantInfo>& getConstants() const;

    // hw.output 정보
    const std::vector<std::string>& getOutputValues() const;

    // 분석 결과
    bool hasRegisters() const;          // seq.firreg/seq.compreg 존재 여부
    bool hasInstances() const;          // hw.instance 존재 여부
    std::map<std::string, std::vector<PortInfo>> groupPortsByPrefix() const;

    /// SSA 값의 비트폭 조회 (미등록 시 0 반환)
    unsigned getValueWidth(const std::string& ssaName) const;

private:
    std::string moduleName_;
    std::vector<PortInfo> ports_;
    std::vector<PortInfo> inputPorts_;
    std::vector<PortInfo> outputPorts_;
    std::vector<OpInfo> operations_;
    std::vector<ConstantInfo> constants_;
    std::vector<std::string> outputValues_;
    bool hasRegisters_ = false;
    bool hasInstances_ = false;
    bool valid_ = false;
    std::map<std::string, unsigned> ssaWidths_;  // SSA 값별 비트폭 매핑

    void parse(const std::string& mlirContent);
    void parseModule(const std::string& moduleLine);
    void parseBody(const std::string& bodyContent);
};

} // namespace hirct
```

**Run**:
```bash
ninja -C build
```

**Expect**:
```
lib/Analysis/ModuleAnalyzer.cpp 컴파일 성공
```

**토폴로지 정렬 상세 (Kahn's Algorithm)**:

```
입력: ModuleAnalyzer가 파싱한 OpInfo 목록 (SSA def-use 그래프)

알고리즘:
1. 각 SSA 값(%name)에 대해 in-degree 계산 (피연산자로 참조된 횟수)
2. seq.firreg/seq.compreg의 출력 → in-degree 계산에서 제외 (피드백 루프 차단)
3. in-degree == 0인 op을 큐에 삽입 (상수, 포트 등)
4. BFS: 큐에서 꺼내 결과 목록에 추가, 해당 op의 결과를 참조하는 op의 in-degree 감소
5. 큐 비어도 미방문 op 존재 → combinational loop 감지

Combinational Loop 처리:
- 감지 시: stderr에 "ERROR: combinational loop detected: %a → %b → ... → %a"
- meta.json에 "combinational_loop": true, 해당 emitter "fail" 기록
- 모듈 처리 중단 (hard fail — 미정렬 op의 잘못된 emit 방지)
- known-limitations.md에 combinational_loop 카테고리로 등록 가능
- (근거: open-decisions.md A-8, hirct-convention.md §2.10.1)
```

> **실측 근거**: LevelGateway의 `%inFlight = seq.firreg %9 ...`에서 `%9 = comb.mux ... %inFlight`는
> 레지스터 피드백 루프이다. `seq.firreg`를 끊는 점으로 처리하면 DAG가 되어 정렬 성공.
> 진짜 combinational loop은 CIRCT 레벨에서 대부분 거부되지만, 예외적으로 통과할 수 있으므로 방어적 처리가 필요하다.

---

## Step 5: GREEN — GenModel 최소 구현 (2시간)

**Goal**: ModuleAnalyzer 정보로 최소한의 .h + .cpp 파일을 생성

**GenModel.h 핵심 인터페이스**:

```cpp
#pragma once
#include "hirct/Analysis/ModuleAnalyzer.h"
#include <string>

namespace hirct {

class GenModel {
public:
    explicit GenModel(const ModuleAnalyzer& analyzer);

    /// C++ 모델 .h + .cpp를 outputDir에 생성
    /// @return true if successful
    bool emit(const std::string& outputDir);

private:
    const ModuleAnalyzer& analyzer_;

    std::string emitHeader();
    std::string emitSource();
};

} // namespace hirct
```

**최소 GenModel 출력 예시** (LevelGateway 기준):

```cpp
// AUTO-GENERATED by hirct-gen — DO NOT EDIT
#pragma once
#include <cstdint>

class LevelGateway {
public:
    // Input ports
    uint32_t io_in;
    bool clock;
    bool reset;

    // Output ports
    uint32_t io_out;

    void do_reset();
    void step();
    void eval_comb();
};
```

**Run**:
```bash
ninja -C build
```

**Expect**:
```
lib/Target/GenModel.cpp 컴파일 성공
```

---

## Step 6: GREEN — CLI main.cpp + GenMakefile 최소 구현 (2시간)

**Goal**: `hirct-gen input.v` → circt-verilog 호출 → MLIR → ModuleAnalyzer → GenModel → 출력

**tools/hirct-gen/main.cpp 핵심 흐름**:

```
1. CLI 인자 파싱 (input.v, -o, --only, --top, -f)
2. circt-verilog input.v 호출 → MLIR 텍스트 획득
3. ModuleAnalyzer(mlirText) 생성
4. 출력 경로 계산 (소스 트리 미러링)
5. GenModel.emit(outputDir) 호출
6. meta.json 생성
```

**Run**:
```bash
ninja -C build
build/tools/hirct-gen/hirct-gen --help
```

**Expect**:
```
Usage: hirct-gen [options] input.v
  ...
exit 0
```

---

## Step 7: GREEN — hirct-gen 파이프라인 스모크 테스트 (1시간)

**Goal**: 실제 RTL 파일로 hirct-gen 파이프라인 최초 관통

**Run**:
```bash
build/tools/hirct-gen/hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
ls output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/cmodel/
g++ -std=c++17 -c output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/cmodel/*.cpp
```

**Expect**:
```
.h + .cpp 파일 존재
g++ 컴파일 성공 (exit 0)
```

---

## Step 8: GREEN — MLIR 테스트 픽스처 저장 + gtest (1시간)

**Goal**: 정규화된 MLIR 출력을 픽스처로 저장하고, ModuleAnalyzer 최소 gtest 작성

**Run**:
```bash
# 픽스처 생성 (circt-verilog | circt-opt 파이프라인 결과 저장)
circt-verilog rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v | circt-opt --canonicalize > test/fixtures/LevelGateway.mlir
circt-verilog rtl/plat/src/s5/design/Fadu_K2_S5_RVCExpander.v | circt-opt --canonicalize > test/fixtures/RVCExpander.mlir
# gtest 빌드 + 실행
ninja -C build
./build/unittests/ModuleAnalyzerTest
```

**Expect**:
```
test/fixtures/LevelGateway.mlir 존재 (비어있지 않음)
ModuleAnalyzerTest: 포트 파싱, 모듈명 추출, 레지스터 감지 PASS
```

---

## Step 9: GREEN — hirct-verify 최소 스켈레톤 (30분)

**Goal**: `hirct-verify --help` → exit 0

**Run**:
```bash
ninja -C build
build/tools/hirct-verify/hirct-verify --help
```

**Expect**:
```
Usage: hirct-verify [options] input.v
  ...
exit 0
```

---

## Step 10: 루트 Makefile `make build` 타겟 연결 (15분)

**Goal**: `make build` → CMake + Ninja 빌드 동작

**Run**:
```bash
make build
hirct-gen --help  # PATH에 build/tools/hirct-gen/ 추가 후
```

**Expect**:
```
exit 0
```

---

## Step 11: 커밋

**Run**:
```bash
git add CMakeLists.txt lib/ include/ tools/ test/ unittests/ integration_test/ utils/ config/
git commit -m "feat(phase-1): bootstrap — CMake build system, ModuleAnalyzer, GenModel skeleton, CLI"
```

---

## 게이트 (완료 기준)

- [ ] `cmake -B build -G Ninja && ninja -C build` → 빌드 성공
- [ ] `hirct-gen --help` → exit 0
- [ ] `hirct-verify --help` → exit 0
- [ ] `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v` → `output/.../cmodel/` 존재
- [ ] `g++ -std=c++17 -c output/.../cmodel/*.cpp` → 컴파일 성공
- [ ] `make build` → exit 0
- [ ] `include/hirct/Analysis/ModuleAnalyzer.h` 존재
- [ ] `include/hirct/Support/CirctRunner.h` 존재
- [ ] `lib/Analysis/ModuleAnalyzer.cpp` 존재
- [ ] `lib/Support/CirctRunner.cpp` 존재
- [ ] `tools/hirct-gen/main.cpp` 존재
- [ ] `tools/hirct-verify/main.cpp` 존재
- [ ] `test/fixtures/LevelGateway.mlir` 존재 (정규화된 MLIR 픽스처)
- [ ] `unittests/Analysis/ModuleAnalyzerTest.cpp` 존재 + PASS
- [ ] `output/.../meta.json` 존재

---

## 변경 이력

| 날짜 | 내용 |
|------|------|
| 2026-02-16 | 신규 작성: dry-run 리뷰 결과 기존 C++ 소스 미존재 확인, Bootstrap 태스크 추가 |
| 2026-02-16 | 리뷰 반영: MLIR 정규화 파이프라인(circt-opt --canonicalize) 설계 결정 추가, CirctRunner 래퍼, 테스트 픽스처(test/fixtures/), ModuleAnalyzerTest gtest 스텝 추가 |
