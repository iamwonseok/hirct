# SoC Make/Makefile Coding Convention Rules

> **Standard**: GNU Make 4.x
>
> **References**:
> - [GNU Make Manual](https://www.gnu.org/software/make/manual/make.html)
> - [Linux Kernel Makefile Documentation](https://www.kernel.org/doc/html/latest/kbuild/makefiles.html)

## 1. File Structure (Make-01)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Make-01-01 | M | Makefile 이름은 `Makefile` 또는 `*.mk` 확장자를 사용한다. | No | - | - |
| Make-01-02 | M | 파일명은 snake_case를 사용한다: `utils.mk`, `paths.mk` | No | - | - |
| Make-01-03 | M | 파일의 줄바꿈 형식은 Unix 스타일(LF)을 사용한다. | Yes | pre-commit | mixed-line-ending |
| Make-01-04 | M | 파일 끝에 빈 줄 하나를 추가한다. | Yes | pre-commit | end-of-file-fixer |
| Make-01-05 | O | 파일 상단에 목적을 설명하는 주석 블록을 포함한다. | No | - | - |

## 2. Formatting (Make-02)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Make-02-01 | M | 레시피(recipe) 들여쓰기는 반드시 탭(Tab)을 사용한다. | Yes | make | syntax error |
| Make-02-02 | M | 변수 정의 및 기타 줄은 탭 또는 공백을 일관성 있게 사용한다. | No | - | - |
| Make-02-03 | M | 줄의 끝(Trailing whitespace)에 공백 문자를 남기지 않는다. | Yes | pre-commit | trailing-whitespace |
| Make-02-04 | O | 한 줄은 최대 100자를 넘지 않도록 작성한다. | No | - | - |
| Make-02-05 | M | 긴 줄은 `\`를 사용하여 나누고, 다음 줄은 탭으로 들여쓴다. | No | - | - |
| Make-02-06 | O | 논리적 섹션 사이에 빈 줄을 추가하여 가독성을 높인다. | No | - | - |

## 3. Comments (Make-03)

| ID | Category | Rule Description |
|----|----------|------------------|
| Make-03-01 | M | 섹션 구분에는 주석 블록을 사용한다. |
| Make-03-02 | O | 복잡한 로직에는 인라인 주석을 추가한다. |
| Make-03-03 | M | 주석은 `#`으로 시작하며, `#` 뒤에 공백 하나를 추가한다. |

```makefile
###############
# SECTION NAME #
###############

# Inline comment for complex logic
VARIABLE := value
```

## 4. Variables (Make-04)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Make-04-01 | M | 변수 참조는 `${VARIABLE}` 형식을 사용한다. (`$(VAR)` 대신) | No | - | - |
| Make-04-02 | M | 변수명은 대문자와 밑줄(UPPER_SNAKE_CASE)을 사용한다. | No | - | - |
| Make-04-03 | M | 내부/중간 변수는 소문자와 밑줄(lower-snake-case)을 사용할 수 있다. | No | - | - |
| Make-04-04 | M | 변수 할당 연산자와 값 사이에 공백을 둔다: `VAR := value` | No | - | - |
| Make-04-05 | O | 관련 변수들은 값의 열(column)을 맞춰 정렬한다. | No | - | - |
| Make-04-06 | M | 즉시 확장(`:=`)과 지연 확장(`=`)을 구분하여 사용한다. | No | - | - |
| Make-04-07 | O | 조건부 할당에는 `?=`를 사용한다. | No | - | - |

```makefile
# Immediate expansion (recommended for most cases)
CC       := gcc
CFLAGS   := -Wall -Werror

# Recursive expansion (when lazy evaluation needed)
OBJECTS = ${SOURCES:.c=.o}

# Conditional assignment
PREFIX ?= /usr/local
```

## 5. Variable Assignment Operators (Make-05)

| ID | Category | Rule Description |
|----|----------|------------------|
| Make-05-01 | M | `:=` (즉시 확장): 정의 시점에 값을 확정한다. 대부분의 경우 권장. |
| Make-05-02 | M | `=` (지연 확장): 사용 시점에 값을 확장한다. 동적 값에 사용. |
| Make-05-03 | M | `?=` (조건부): 변수가 미정의일 때만 할당한다. 기본값 설정에 사용. |
| Make-05-04 | M | `+=` (추가): 기존 값에 추가한다. |

## 6. Targets and Rules (Make-06)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Make-06-01 | M | 기본 타겟은 파일 상단에 위치시킨다. (보통 `all`) | No | - | - |
| Make-06-02 | M | Phony 타겟은 `.PHONY`로 명시적 선언한다. | Yes | checkmake | minphony |
| Make-06-03 | M | 타겟명은 소문자와 하이픈(kebab-case)을 사용한다. | No | - | - |
| Make-06-04 | M | 타겟과 의존성 사이에 공백 없이 `:`를 사용한다. | No | - | - |
| Make-06-05 | O | 관련 타겟들은 그룹화하여 배치한다. | No | - | - |

```makefile
.PHONY: all clean build test

all: build

build: ${OBJECTS}
	${CC} ${LDFLAGS} -o ${TARGET} ${OBJECTS}

clean:
	${RM} ${OBJECTS} ${TARGET}
```

## 7. Recipes (Make-07)

| ID | Category | Rule Description |
|----|----------|------------------|
| Make-07-01 | M | 레시피 줄은 반드시 탭(Tab)으로 시작한다. |
| Make-07-02 | O | 명령어 출력을 숨기려면 `@` 접두사를 사용한다. |
| Make-07-03 | O | 명령어 실패를 무시하려면 `-` 접두사를 사용한다. |
| Make-07-04 | M | 복잡한 쉘 명령어는 여러 줄로 나누되 `\`로 연결한다. |
| Make-07-05 | O | Verbose 모드 지원을 위해 `${Q}` 변수를 사용한다. |

```makefile
# Verbose control
Q := $(if $(filter 1,${V}),,@)

build:
	${Q}${CC} ${CFLAGS} -c $< -o $@

clean:
	@echo "Cleaning..."
	-${RM} ${OBJECTS}
```

## 8. Conditionals (Make-08)

| ID | Category | Rule Description |
|----|----------|------------------|
| Make-08-01 | M | 조건문 키워드(`ifeq`, `ifneq`, `ifdef`, `ifndef`)는 들여쓰지 않는다. |
| Make-08-02 | M | 조건문 본문은 들여쓴다. |
| Make-08-03 | M | `endif` 뒤에 주석으로 어떤 조건의 끝인지 명시한다. |
| Make-08-04 | M | 문자열 비교에는 `ifeq`/`ifneq`를 사용한다. |

```makefile
ifeq (${CONFIG_DEBUG},y)
CFLAGS += -g -O0
else
CFLAGS += -O2
endif # CONFIG_DEBUG

ifneq ($(CONFIG_BUILD_DASM),y)
BUILD_DASM :=
endif # !CONFIG_BUILD_DASM
```

## 9. Functions (Make-09)

| ID | Category | Rule Description |
|----|----------|------------------|
| Make-09-01 | M | 내장 함수 호출은 `${function args}` 형식을 사용한다. |
| Make-09-02 | O | 사용자 정의 함수는 `define`/`endef`로 정의한다. |
| Make-09-03 | O | 단순 연산 함수는 `call`로 호출한다. |

```makefile
# Built-in functions
SOURCES := $(wildcard src/*.c)
OBJECTS := $(SOURCES:.c=.o)
DIRS    := $(sort $(dir ${OBJECTS}))

# User-defined functions
add = $(shell expr $(1) + $(2))
get_file_size = $(shell stat -c %s $(1))

# Usage
SIZE := $(call get_file_size,${FILE})
```

## 10. Include (Make-10)

| ID | Category | Rule Description |
|----|----------|------------------|
| Make-10-01 | M | include 경로는 변수로 정의하여 사용한다. |
| Make-10-02 | M | 필수 include는 `include`를 사용한다. |
| Make-10-03 | M | 선택적 include는 `-include`를 사용한다. |
| Make-10-04 | O | include 순서는 의존성을 고려하여 배치한다. |

```makefile
include ${UTILS_MK}
include ${PATHS_MK}
-include ${OPTIONAL_MK}
```

## 11. Automatic Variables (Make-11)

| ID | Category | Rule Description |
|----|----------|------------------|
| Make-11-01 | M | 자동 변수를 적극 활용한다: `$@`, `$<`, `$^`, `$*` |
| Make-11-02 | O | 자동 변수 사용 시 주석으로 의미를 명시할 수 있다. |

| Variable | Description |
|----------|-------------|
| `$@` | 타겟 이름 |
| `$<` | 첫 번째 의존성 |
| `$^` | 모든 의존성 (중복 제거) |
| `$?` | 타겟보다 새로운 의존성들 |
| `$*` | 패턴 매칭된 stem |

```makefile
%.o: %.c
	${CC} ${CFLAGS} -c $< -o $@

${TARGET}: ${OBJECTS}
	${LD} ${LDFLAGS} -o $@ $^
```

## 12. Organization (Make-12)

| ID | Category | Rule Description |
|----|----------|------------------|
| Make-12-01 | O | 파일 구조는 논리적 섹션으로 나눈다. |
| Make-12-02 | O | 권장 섹션 순서: 변수 -> include -> 타겟 -> 규칙 |
| Make-12-03 | O | 공통 유틸리티는 별도 `.mk` 파일로 분리한다. |

```makefile
###############
# VARIABLES   #
###############
CC := gcc

###############
# INCLUDES    #
###############
include config.mk

###############
# TARGETS     #
###############
.PHONY: all clean

all: build

###############
# RULES       #
###############
%.o: %.c
	${CC} -c $< -o $@
```

---

## Tools

| Tool | Purpose | Installation |
|------|---------|--------------|
| checkmake | Makefile 린터 | `go install github.com/mrtazz/checkmake/cmd/checkmake@latest` |
| make --warn-undefined-variables | 미정의 변수 경고 | GNU Make 내장 옵션 |

### Usage Example

```bash
# Lint Makefile
checkmake Makefile

# Run make with undefined variable warnings
make --warn-undefined-variables
```

---

## Legend

- **M (Mandatory)**: 필수 준수 규칙
- **O (Optional)**: 권장 규칙
