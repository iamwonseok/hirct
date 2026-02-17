# SoC C Coding Convention Rules

> **Standard**: C17 (ISO/IEC 9899:2018)
>
> **References**:
> - [Linux Kernel Coding Style](https://www.kernel.org/doc/html/latest/process/coding-style.html)
> - [GNU Coding Standards](https://www.gnu.org/prep/standards/standards.html)
> - [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)

## 1. Formatting (C-01)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|------|------------------|------------|------|------------|
| C-01-01 | M | 들여쓰기는 탭(Tab)을 사용하며, 탭의 크기는 4 공백(Space)으로 설정한다. | Yes | clang-format | - |
| C-01-02 | M | switch 문에서는 case 레이블을 switch와 동일한 들여쓰기 레벨(column)에 위치시킨다. | Yes | clang-format | - |
| C-01-03 | M | 한 줄에 여러 문장(Statement)을 넣지 않는다. | Yes | clang-format | - |
| C-01-04 | M | 한 줄에 하나의 변수 선언을 원칙으로 하되, 포인터가 아닌 기본 자료형의 연관된 변수는 예외적으로 허용한다. (포인터는 한 줄 선언 금지) | No | - | - |
| C-01-05 | M | 줄의 끝(Trailing whitespace)에 공백 문자를 남기지 않는다. | Yes | clang-format | - |
| C-01-06 | M | 파일의 마지막에 불필요한 빈 라인을 추가하지 않는다. | Yes | clang-format | - |
| C-01-07 | M | 파일의 줄바꿈 형식은 Unix 스타일(LF)을 사용한다. | Yes | pre-commit | mixed-line-ending |
| C-01-08 | O | 코드 한 줄은 최대 100자를 넘지 않도록 작성한다. | Yes | clang-format | - |
| C-01-09 | M | 줄을 나눌 때 하위 항목은 상위 항목보다 짧게 작성하고, 오른쪽으로 들여쓰기하여 배치한다. | Yes | clang-format | - |
| C-01-10 | M | 로그 메시지 등 출력을 위한 문자열 리터럴 자체는 줄을 나누지 않는다. | Yes | clang-format | BreakStringLiterals |
| C-01-11 | M | 함수의 시작 중괄호({)는 함수 선언부의 다음 줄 시작 지점에 위치한다. | Yes | clang-format | AfterFunction |
| C-01-12 | M | 제어문(if, switch, for, while)의 시작 중괄호({)는 해당 선언부와 같은 줄에 위치한다. | Yes | clang-format | AfterControlStatement |
| C-01-13 | M | 닫는 중괄호(})와 이어지는 키워드(else, while)는 같은 줄에 위치한다. (ex. `} else {`) | Yes | clang-format | BeforeElse |
| C-01-14 | M | 조건문이나 루프의 본문이 한 줄이라도 반드시 중괄호를 사용한다. | Yes | clang-tidy | readability-braces-around-statements |
| C-01-15 | M | if 조건식 내부에서 변수 할당(Assignment)을 수행하지 않는다. | Yes | clang-tidy | bugprone-assignment-in-if-condition |
| C-01-16 | M | 제어문 키워드(if, switch, for, while 등) 뒤에는 공백을 하나 추가한다. | Yes | clang-format | SpaceBeforeParens |
| C-01-17 | M | sizeof, typeof, alignof, __attribute__ 뒤에는 공백을 추가하지 않는다. | Yes | clang-format | SpaceAfterCStyleCast |
| C-01-18 | M | 포인터 변수 선언 및 반환 타입 지정 시 * 기호는 변수명(또는 함수명) 쪽에 붙인다. | Yes | clang-format | PointerAlignment |
| C-01-19 | M | 이항 및 삼항 연산자의 양쪽에는 공백을 하나씩 추가한다. | Yes | clang-format | - |
| C-01-20 | M | 단항 연산자(&, *, +, -, ~, !) 뒤에는 공백을 두지 않는다. | Yes | clang-format | - |
| C-01-21 | M | 증감 연산자(++, --)와 피연산자 사이에는 공백을 두지 않는다. | Yes | clang-format | - |
| C-01-22 | M | 구조체 멤버 접근 연산자(., ->) 앞뒤에는 공백을 두지 않는다. | Yes | clang-format | - |
| C-01-23 | O | 매크로와 열거형 상수는 대문자를 사용한다. (함수형 매크로는 소문자 허용) | Yes | clang-tidy | readability-identifier-naming |
| C-01-24 | O | 표현식을 분할할 때는 연산자 위치를 일관성 있게(앞 또는 뒤) 맞춘다. | Yes | clang-format | BreakBeforeBinaryOperators |
| C-01-25 | O | 주석을 포함한 모든 코드 내 텍스트는 영어를 사용한다. | No | - | - |
| C-01-26 | O | 코드는 그 자체로 설명되도록 작성하며, 불필요한 동작 설명 주석은 지양한다. | No | - | - |
| C-01-27 | M | 헤더 파일에는 #pragma once 대신 표준 #define 가드를 사용한다. | Yes | clang-tidy | llvm-header-guard |
| C-01-28 | M | 컴파일 타임 제약 조건 위반 시 #error 지시자를 사용하여 빌드를 중단한다. | No | - | - |

## 2. Syntactic Convention (C-02)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|------|------------------|------------|------|------------|
| C-02-01 | O | 구조체와 포인터 정의에 typedef를 사용하여 타입을 숨기지 않는다. | No | - | - |
| C-02-02 | O | 함수 종료 시 공통된 자원 해제(Cleanup)가 필요한 경우에만 goto문을 사용한다. | No | - | - |
| C-02-03 | O | goto 레이블 이름은 해당 위치의 역할이나 이유를 명확히 기술한다. | No | - | - |
| C-02-04 | O | 자원 해제가 필요 없는 단순 종료 시에는 goto 대신 직접 return한다. | No | - | - |
| C-02-05 | M | 복수 문장으로 구성된 매크로는 do-while(0) 구문으로 감싸서 작성한다. | Yes | clang-tidy | bugprone-macro-parentheses |
| C-02-06 | O | 호출부의 제어 흐름(return, goto 등)에 영향을 주는 매크로를 사용하지 않는다. | No | - | - |
| C-02-07 | M | 매크로 내부에서 인자로 전달되지 않은 외부 변수(전역/지역)를 참조하지 않는다. | No | - | - |
| C-02-08 | M | 매크로 정의 시 모든 인자는 괄호()로 감싸서 연산자 우선순위 문제를 방지한다. | Yes | clang-tidy | bugprone-macro-parentheses |
| C-02-09 | M | 매크로를 좌변 값(L-value)으로 사용하여 대입하지 않는다. | No | - | - |
| C-02-11 | O | 메모리 할당 시 크기 지정은 sizeof(*pointer) 형식을 사용한다. | Yes | clang-tidy | bugprone-sizeof-expression |
| C-02-12 | M | 상수를 정의하는 매크로 표현식 전체를 괄호로 감싼다. | Yes | clang-tidy | bugprone-macro-parentheses |
| C-02-13 | O | 전처리 지시문은 1열에서 시작하며, #endif 뒤에는 주석으로 해당 조건을 명시한다. | No | - | - |
| C-02-14 | M | 전역 변수나 상위 스코프 변수를 가리는(Shadowing) 지역 변수명을 사용하지 않는다. | Yes | clang-tidy | bugprone-shadow |
| C-02-15 | O | 기본 자료형 대신 <stdint.h>의 고정 폭 정수 타입을 사용한다. | No | - | - |
| C-02-16 | O | 변수의 값을 변경하는 증감 연산자를 조건식이나 다른 연산 내부에 섞어 쓰지 않는다. | Yes | clang-tidy | bugprone-inc-dec-in-conditions |
| C-02-17 | M | 제약 조건 위반 시 #error를 사용하여 명시적으로 컴파일을 막는다. | No | - | - |

## 3. Naming (C-03)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|------|------------------|------------|------|------------|
| C-03-01 | O | 전역 변수와 함수 이름은 그 용도를 명확히 알 수 있도록 충분히 설명적으로 짓는다. | No | - | - |
| C-03-02 | O | 지역 변수는 간결하게 짓되, 용도에 따라 관습적인 이름(i, j, ret 등)을 사용한다. | No | - | - |
| C-03-03 | O | 개발자 개인만 아는 모호한 약어 사용을 금지한다. (공용 약어는 허용) | No | - | - |
| C-03-04 | M | 식별자 단어 구분에는 밑줄(snake_case)을 사용한다. | Yes | clang-tidy | readability-identifier-naming |
| C-03-05 | O | 함수 및 변수 이름은 소문자를 사용한다. (Datasheet 매칭 등 특수 상황 예외) | Yes | clang-tidy | readability-identifier-naming |
| C-03-06 | O | 헝가리안 표기법(타입 정보를 접두어로 붙이는 것)을 사용하지 않는다. | No | - | - |
| C-03-07 | O | 모든 네이밍과 주석은 미국식 영어(American English)를 사용한다. | No | - | - |
| C-03-08 | O | 변수명 작성 시 단수와 복수(배열, 리스트 등)의 의미를 명확히 구분한다. | No | - | - |
| C-03-09 | O | 식별자에 이중 밑줄(__)을 사용하지 않는다. | Yes | clang-tidy | cert-dcl37-c |
| C-03-10 | M | 소스 및 헤더 파일의 이름은 변수 명명 규칙(snake_case)을 따른다. | No | - | - |

## 4. Function (C-04)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|------|------------------|------------|------|------------|
| C-04-01 | O | 함수는 한 가지 기능만 명확하게 수행하도록 작게 작성한다. | No | - | - |
| C-04-02 | O | 함수의 매개변수 개수는 4개를 초과하지 않도록 설계한다. | Yes | clang-tidy | readability-function-size |
| C-04-03 | O | 함수 내부의 지역 변수 개수는 10개를 초과하지 않도록 한다. | Yes | clang-tidy | readability-function-size |
| C-04-04 | M | 함수 정의와 함수 정의 사이에는 빈 줄 하나를 둔다. | Yes | clang-format | SeparateDefinitionBlocks |
| C-04-05 | M | 함수 프로토타입 선언 시 매개변수의 타입뿐만 아니라 변수명도 명시한다. | Yes | clang-tidy | readability-named-parameter |
| C-04-06 | O | Public 함수는 명확한 반환 값 규칙(에러 코드 등)을 따른다. | No | - | - |
| C-04-07 | O | 동작/명령을 수행하는 함수는 int형 에러 코드를 반환한다. (0: 성공, <0: 실패) | No | - | - |
| C-04-08 | O | 상태를 확인(Query)하는 함수는 bool형을 반환한다. | No | - | - |
| C-04-09 | O | 이미 존재하는 표준 매크로나 라이브러리 함수를 재구현하지 않는다. | No | - | - |
| C-04-10 | O | 함수 원형은 헤더 파일에 선언하고 include하여 사용하며, 소스 내 extern 선언을 금지한다. | Yes | clang-tidy | misc-use-anonymous-namespace |
| C-04-11 | O | 전역 변수를 소스 코드 내에서 extern으로 직접 선언하여 사용하지 않는다. | No | - | - |

## 5. Safety & Reliability (C-05)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|------|------------------|------------|------|------------|
| C-05-01 | M | 데이터 크기 보장과 이식성을 위해 int, long 대신 <stdint.h>의 고정 폭 정수(int32_t 등)를 사용한다. | No | - | - |
| C-05-02 | M | 모든 지역 변수는 선언과 동시에 초기화하거나, 사용하기 전에 반드시 명시적으로 값을 할당한다. | Yes | clang-tidy | cppcoreguidelines-init-variables |
| C-05-03 | M | ISR(인터럽트)과 공유되는 전역 변수나 하드웨어 레지스터 포인터는 반드시 volatile로 선언한다. | No | - | - |
| C-05-04 | M | 스택 오버플로우 방지를 위해 재귀 함수(Recursion) 사용을 엄격히 금지한다. | Yes | clang-tidy | misc-no-recursion |
| C-05-05 | O | switch 문에는 default 분기를 작성한다. (단, enum 완전 커버로 인한 Dead Code 경고 시 생략 가능) | Yes | clang-tidy | hicpp-multiway-paths-covered |
| C-05-06 | M | 외부(통신, 센서 등)에서 입력된 값은 사용 전 반드시 유효 범위(Range) 및 무결성을 검증한다. | No | - | - |
| C-05-07 | M | 버퍼 오버플로우 방지를 위해 strcpy, strcat, sprintf 대신 길이 검사가 포함된 strncpy, strncat, snprintf를 사용한다. | Yes | clang-tidy | bugprone-unsafe-functions |
| C-05-08 | M | 민감 데이터 소거 시 컴파일러 최적화(Dead Store Elimination) 방지를 위해 memset 대신 memset_s를 사용한다. | No | - | - |
| C-05-09 | M | 보안 데이터(키, 해시, 비밀번호) 비교 시 타이밍 공격 방지를 위해 memcmp 대신 상수 시간 비교 함수(timingsafe_memcmp)를 사용한다. | No | - | - |
| C-05-10 | M | 메모리 영역이 겹칠(Overlap) 가능성이 있는 복사 작업에는 memcpy 대신 memmove를 사용한다. | No | - | - |
| C-05-11 | M | 동적 할당된 메모리는 goto를 활용한 중앙 집중식 에러 처리 패턴(Centralized Exit)을 사용하여 반드시 해제됨을 보장한다. | No | - | - |

## Legend

- **M (Mandatory)**: 필수 준수 규칙
- **O (Optional)**: 권장 규칙
