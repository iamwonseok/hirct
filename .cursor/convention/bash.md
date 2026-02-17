# SoC Bash Coding Convention Rules

> **Standard**: Bash 4.0+
>
> **References**:
> - [Google Shell Style Guide](https://google.github.io/styleguide/shellguide.html)
> - [Bash Reference Manual](https://www.gnu.org/software/bash/manual/bash.html)
> - [ShellCheck Wiki](https://www.shellcheck.net/wiki/)

## 1. File Header (Bash-01)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Bash-01-01 | M | 모든 실행 스크립트는 `#!/bin/bash` shebang으로 시작한다. | Yes | ShellCheck | SC2039, SC3010 |
| Bash-01-02 | M | source 되는 라이브러리 스크립트는 shebang 없이 주석으로 시작할 수 있다. | No | - | - |
| Bash-01-03 | O | 파일 상단에 스크립트의 목적을 설명하는 주석 블록을 포함한다. | No | - | - |
| Bash-01-04 | M | 에러 발생 시 즉시 종료하려면 `set -e`를 사용한다. | No | - | - |
| Bash-01-05 | O | 디버깅이 필요한 경우 `set -x`를 사용한다. | No | - | - |

## 2. Formatting (Bash-02)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Bash-02-01 | M | 들여쓰기는 탭(Tab)을 사용하며, 탭의 크기는 4 공백으로 설정한다. | Yes | shfmt | -i 0 |
| Bash-02-02 | M | 한 줄은 최대 100자를 넘지 않도록 작성한다. | No | - | - |
| Bash-02-03 | M | 파일의 줄바꿈 형식은 Unix 스타일(LF)을 사용한다. | Yes | pre-commit | mixed-line-ending |
| Bash-02-04 | M | 줄의 끝(Trailing whitespace)에 공백 문자를 남기지 않는다. | Yes | pre-commit | trailing-whitespace |
| Bash-02-05 | M | 파일의 마지막에 빈 줄 하나를 추가한다. | Yes | pre-commit | end-of-file-fixer |
| Bash-02-06 | M | 긴 명령어를 여러 줄로 나눌 때는 `\`를 사용하고, 다음 줄은 탭으로 들여쓴다. | Yes | shfmt | - |
| Bash-02-07 | O | 논리적 섹션 사이에 빈 줄을 하나 추가하여 가독성을 높인다. | No | - | - |

## 3. Variables (Bash-03)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Bash-03-01 | M | 변수 참조 시 반드시 중괄호를 사용한다: `${variable}` (not `$variable`) | Yes | ShellCheck | require-variable-braces |
| Bash-03-02 | M | 지역 변수는 `local` 키워드를 사용하여 선언한다. | No | - | - |
| Bash-03-03 | M | 전역 변수와 환경 변수는 대문자와 밑줄을 사용한다: `GLOBAL_VARIABLE` | No | - | - |
| Bash-03-04 | M | 지역 변수는 소문자와 밑줄을 사용한다: `local_variable` | No | - | - |
| Bash-03-05 | M | 변수를 문자열 내에서 사용할 때는 큰따옴표로 감싼다: `"${variable}"` | Yes | ShellCheck | SC2086 |
| Bash-03-06 | M | 배열 참조 시 중괄호를 사용한다: `${array[@]}`, `${array[0]}` | Yes | ShellCheck | SC2086 |
| Bash-03-07 | O | 읽기 전용 변수는 `readonly` 또는 `declare -r`로 선언한다. | No | - | - |
| Bash-03-08 | M | 변수 할당 시 `=` 앞뒤에 공백을 두지 않는다: `var="value"` | Yes | ShellCheck | SC1068 |

## 4. Quoting (Bash-04)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Bash-04-01 | M | 변수가 포함된 문자열은 큰따옴표로 감싼다: `"${var}"` | Yes | ShellCheck | SC2086, SC2046 |
| Bash-04-02 | M | 리터럴 문자열(변수 확장 불필요)은 작은따옴표를 사용한다: `'literal'` | No | - | - |
| Bash-04-03 | M | 파일 경로를 포함하는 변수는 항상 따옴표로 감싼다: `"${path}"` | Yes | ShellCheck | SC2086 |
| Bash-04-04 | O | 빈 문자열 검사 시 따옴표를 사용한다: `[ "${var}" == "" ]` | Yes | ShellCheck | SC2086 |

## 5. Functions (Bash-05)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Bash-05-01 | M | 함수 정의는 `function_name() {` 형식을 사용한다. (function 키워드 생략) | Yes | shfmt | - |
| Bash-05-02 | M | 함수의 여는 중괄호 `{`는 함수명과 같은 줄에 위치한다. | Yes | shfmt | - |
| Bash-05-03 | M | 함수 내 변수는 `local`로 선언한다. | No | - | - |
| Bash-05-04 | M | 함수 이름은 소문자와 밑줄(snake_case)을 사용한다. | No | - | - |
| Bash-05-05 | O | 함수 정의 전에 간단한 설명 주석을 추가한다. | No | - | - |
| Bash-05-06 | M | 함수 간에는 빈 줄 하나를 둔다. | No | - | - |
| Bash-05-07 | M | 반환값이 있는 함수는 명시적으로 `return`을 사용한다. | No | - | - |

## 6. Control Structures (Bash-06)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Bash-06-01 | M | `if`, `for`, `while`, `case` 키워드 뒤에 공백을 추가한다. | Yes | shfmt | - |
| Bash-06-02 | M | `then`, `do`는 조건문/반복문과 같은 줄에 위치하며, 앞에 `;`를 붙인다. | Yes | shfmt | - |
| Bash-06-03 | M | `case` 문에서 각 패턴은 `)` 로 끝나고, 처리 블록은 `;;`로 종료한다. | Yes | ShellCheck | SC2221 |
| Bash-06-04 | M | `case` 문에서 기본 패턴 `*)`를 포함한다. | No | - | - |
| Bash-06-05 | M | 조건 테스트에는 `[[ ]]`를 사용한다. (POSIX `[ ]` 대신) | Yes | ShellCheck | SC2039, SC3010 |
| Bash-06-06 | M | 산술 연산에는 `$(( ))`를 사용한다. | Yes | ShellCheck | SC2007 |

```bash
# Good
if [[ "${var}" == "value" ]]; then
    echo "match"
fi

for file in "${files[@]}"; do
    process "${file}"
done

case "${option}" in
    start)
        start_service
        ;;
    stop)
        stop_service
        ;;
    *)
        echo "Unknown option"
        ;;
esac
```

## 7. Command Substitution (Bash-07)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Bash-07-01 | M | 명령어 치환에는 `$(command)` 형식을 사용한다. (백틱 `` `command` `` 금지) | Yes | ShellCheck | SC2006 |
| Bash-07-02 | M | 중첩된 명령어 치환도 `$()`를 사용한다. | Yes | ShellCheck | SC2006 |
| Bash-07-03 | O | 복잡한 명령어 치환은 가독성을 위해 변수에 먼저 저장한다. | No | - | - |

```bash
# Good
output=$(command)
nested=$(echo "$(date +%Y)-$(hostname)")

# Bad
output=`command`
```

## 8. Error Handling (Bash-08)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Bash-08-01 | M | 실패 가능한 명령어 후에는 반환 코드를 확인한다. | No | - | - |
| Bash-08-02 | M | 에러 메시지는 표준 에러(stderr)로 출력한다: `echo "error" >&2` | No | - | - |
| Bash-08-03 | O | 사용법(usage) 함수를 제공하여 잘못된 입력에 도움말을 출력한다. | No | - | - |
| Bash-08-04 | M | 필수 명령어 존재 여부는 `command -v` 또는 `-x`로 확인한다. | Yes | ShellCheck | SC2230 |
| Bash-08-05 | M | 에러 발생 시 적절한 종료 코드와 함께 `exit`한다. (0: 성공, 1: 일반 에러) | No | - | - |

```bash
# Check command exists
if ! [[ -x "$(command -v docker)" ]]; then
    echo "[ERROR] docker not found!" >&2
    exit 1
fi

# Check file exists
if [[ ! -f "${config_file}" ]]; then
    echo "error: ${config_file} not found" >&2
    exit 1
fi
```

## 9. Naming (Bash-09)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Bash-09-01 | M | 스크립트 파일명은 snake_case를 사용하고 `.sh` 확장자를 붙인다. | No | - | - |
| Bash-09-02 | M | 실행 파일은 shebang을 포함하고 실행 권한을 부여한다. | Yes | ShellCheck | SC2148 |
| Bash-09-03 | O | 라이브러리/source용 파일은 `_` 접두사를 사용할 수 있다: `_utils.sh` | No | - | - |
| Bash-09-04 | M | 내부용(private) 함수/변수는 `_` 접두사를 사용한다. | No | - | - |

## 10. Best Practices (Bash-10)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Bash-10-01 | M | `cd` 사용 시 실패를 처리한다: `cd "${dir}" || exit 1` | Yes | ShellCheck | SC2164 |
| Bash-10-02 | M | 파이프라인의 모든 명령어 성공을 확인하려면 `set -o pipefail`을 사용한다. | Yes | ShellCheck | SC2312 |
| Bash-10-03 | O | 임시 파일은 `mktemp`로 생성하고, trap으로 정리한다. | No | - | - |
| Bash-10-04 | M | 사용자 입력을 명령어에 직접 전달하기 전에 검증한다. | No | - | - |
| Bash-10-05 | O | 긴 스크립트는 `main()` 함수를 사용하여 구조화한다. | No | - | - |
| Bash-10-06 | M | associative array 사용 시 `declare -A`로 명시적 선언한다. | Yes | ShellCheck | SC2034 |

```bash
#!/bin/bash

set -e
set -o pipefail

main() {
    local input="${1:-}"

    if [[ -z "${input}" ]]; then
        usage
        exit 1
    fi

    process "${input}"
}

usage() {
    echo "usage: $(basename "$0") <input>"
}

process() {
    local file="$1"
    # processing logic
}

main "$@"
```

---

## Tools

| Tool | Purpose | Configuration |
|------|---------|---------------|
| ShellCheck | 정적 분석 및 린팅 | `.shellcheckrc` |
| shfmt | 자동 포맷팅 | `-i 0 -ci -bn` (탭 들여쓰기, case 들여쓰기, 이항연산자 줄바꿈) |

---

## Legend

- **M (Mandatory)**: 필수 준수 규칙
- **O (Optional)**: 권장 규칙
