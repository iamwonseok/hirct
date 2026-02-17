# SoC Python Coding Convention Rules

> **Standard**: Python 3.10+, PEP8
>
> **References**:
> - [PEP 8 - Style Guide for Python Code](https://peps.python.org/pep-0008/)
> - [PEP 257 - Docstring Conventions](https://peps.python.org/pep-0257/)
> - [Google Python Style Guide](https://google.github.io/styleguide/pyguide.html)
> - [Black Documentation](https://black.readthedocs.io/en/stable/)
> - [Flake8 Documentation](https://flake8.pycqa.org/en/latest/)

## 1. File Structure (Py-01)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Py-01-01 | M | 파일 인코딩은 UTF-8을 사용한다. | Yes | Flake8 | W503 |
| Py-01-02 | M | 파일의 줄바꿈 형식은 Unix 스타일(LF)을 사용한다. | Yes | pre-commit | mixed-line-ending |
| Py-01-03 | M | 파일의 마지막에 빈 줄 하나를 추가한다. | Yes | pre-commit | end-of-file-fixer |
| Py-01-04 | O | 실행 스크립트는 shebang `#!/usr/bin/env python3`으로 시작한다. | No | - | - |
| Py-01-05 | O | 모듈 상단에 docstring으로 모듈 설명을 작성한다. | Yes | Flake8 | D100 |

## 2. Formatting (Py-02)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Py-02-01 | M | 들여쓰기는 공백 4칸을 사용한다. (탭 금지) | Yes | Black, Flake8 | E101, W191 |
| Py-02-02 | M | 한 줄은 최대 88자를 넘지 않는다. (Black 기본값) | Yes | Black, Flake8 | E501 |
| Py-02-03 | M | 줄의 끝(Trailing whitespace)에 공백 문자를 남기지 않는다. | Yes | Flake8 | W291, W293 |
| Py-02-04 | M | 최상위 함수/클래스 정의 사이에는 빈 줄 2개를 둔다. | Yes | Black, Flake8 | E302 |
| Py-02-05 | M | 클래스 내 메서드 정의 사이에는 빈 줄 1개를 둔다. | Yes | Black, Flake8 | E301 |
| Py-02-06 | M | 함수 내 논리적 섹션 구분에는 빈 줄 1개를 사용할 수 있다. | No | - | - |

## 3. Imports (Py-03)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Py-03-01 | M | import 문은 파일 상단에 위치한다. (docstring 다음) | Yes | Flake8 | E402 |
| Py-03-02 | M | import는 한 줄에 하나의 모듈만 작성한다. | Yes | isort, Flake8 | E401 |
| Py-03-03 | M | import 순서: 표준 라이브러리 -> 서드파티 -> 로컬 모듈 (각 그룹 사이 빈 줄) | Yes | isort | I001 |
| Py-03-04 | M | 와일드카드 import (`from module import *`)를 사용하지 않는다. | Yes | Flake8 | F401, F403 |
| Py-03-05 | O | 절대 import를 권장한다. 상대 import는 패키지 내부에서만 사용. | No | - | - |
| Py-03-06 | O | isort를 사용하여 import를 자동 정렬한다. | Yes | isort | - |

```python
# Good
import os
import sys

import numpy as np
import requests

from mypackage import mymodule
from mypackage.subpackage import helper

# Bad
import os, sys
from os import *
```

## 4. Naming (Py-04)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Py-04-01 | M | 변수, 함수, 메서드: `snake_case` | Yes | pep8-naming | N806 |
| Py-04-02 | M | 클래스: `PascalCase` (CapWords) | Yes | pep8-naming | N801 |
| Py-04-03 | M | 상수: `UPPER_SNAKE_CASE` | No | - | - |
| Py-04-04 | M | 모듈, 패키지: `snake_case` (짧고 소문자) | Yes | pep8-naming | N999 |
| Py-04-05 | M | 내부용(private): `_leading_underscore` | No | - | - |
| Py-04-06 | M | 강한 내부용: `__double_leading_underscore` (name mangling) | No | - | - |
| Py-04-07 | M | 매직 메서드: `__dunder__` (직접 정의 금지, 오버라이드만) | No | - | - |
| Py-04-08 | O | 약어는 대문자로 유지하되 PascalCase 규칙 적용: `HTTPServer`, `XMLParser` | No | - | - |
| Py-04-09 | M | 예약어와 충돌 시 후행 밑줄 사용: `class_`, `type_` | No | - | - |

```python
# Constants
MAX_RETRY_COUNT = 3
DEFAULT_TIMEOUT = 30

# Classes
class DataProcessor:
    pass

class HTTPRequestHandler:
    pass

# Functions and variables
def calculate_checksum(data):
    result_value = 0
    return result_value

# Private
class MyClass:
    def __init__(self):
        self._internal_state = None
        self.__very_private = None

    def _helper_method(self):
        pass
```

## 5. Strings (Py-05)

| ID | Category | Rule Description |
|----|----------|------------------|
| Py-05-01 | M | 문자열은 큰따옴표(`"`)를 기본으로 사용한다. (Black 기본값) |
| Py-05-02 | M | 문자열 내 큰따옴표가 있으면 작은따옴표(`'`)를 사용한다. |
| Py-05-03 | M | 문자열 포매팅은 f-string을 우선 사용한다. (Python 3.6+) |
| Py-05-04 | M | 복잡한 포매팅이나 지연 평가 필요 시 `.format()` 사용. |
| Py-05-05 | M | `%` 포매팅은 사용하지 않는다. (logging 예외) |
| Py-05-06 | M | docstring은 삼중 큰따옴표(`"""`)를 사용한다. |

```python
# Good
name = "Alice"
message = f"Hello, {name}!"
sql = 'SELECT * FROM "users"'

# Bad
name = 'Alice'
message = "Hello, %s!" % name
```

## 6. Whitespace (Py-06)

| ID | Category | Rule Description |
|----|----------|------------------|
| Py-06-01 | M | 콤마, 콜론, 세미콜론 뒤에는 공백을 추가한다. |
| Py-06-02 | M | 괄호 바로 안쪽에는 공백을 두지 않는다: `func(arg)` not `func( arg )` |
| Py-06-03 | M | 콜론이 슬라이스로 사용될 때는 양쪽에 동일한 공백을 적용한다. |
| Py-06-04 | M | 함수 호출 괄호 앞에는 공백을 두지 않는다: `func()` not `func ()` |
| Py-06-05 | M | 인덱싱/슬라이싱 괄호 앞에는 공백을 두지 않는다: `list[0]` |
| Py-06-06 | M | 대입 연산자(`=`) 양쪽에는 공백 하나를 둔다. |
| Py-06-07 | M | 키워드 인자/기본값의 `=` 주위에는 공백을 두지 않는다. |

```python
# Good
spam(ham[1], {eggs: 2})
foo = (0,)
x = 1
y = 2
long_variable = 3

def func(arg1, arg2=None):
    pass

result = func(value, key=True)

# Bad
spam( ham[ 1 ], { eggs: 2 } )
x             = 1
def func(arg1, arg2 = None):
    pass
```

## 7. Operators (Py-07)

| ID | Category | Rule Description |
|----|----------|------------------|
| Py-07-01 | M | 이항 연산자 양쪽에는 공백 하나를 둔다. |
| Py-07-02 | O | 우선순위가 다른 연산자 혼용 시 낮은 우선순위 연산자 주위에만 공백을 둔다. |
| Py-07-03 | M | 줄 바꿈 시 연산자는 새 줄의 앞에 위치한다. (Black 스타일) |

```python
# Good
i = i + 1
x = x*2 - 1
hypot2 = x*x + y*y
c = (a+b) * (a-b)

# Line break before operator
income = (
    gross_wages
    + taxable_interest
    + (dividends - qualified_dividends)
    - ira_deduction
)
```

## 8. Functions (Py-08)

| ID | Category | Rule Description |
|----|----------|------------------|
| Py-08-01 | M | 함수는 한 가지 기능만 명확하게 수행한다. |
| Py-08-02 | O | 함수의 매개변수는 5개를 초과하지 않도록 설계한다. |
| Py-08-03 | M | 가변 인자는 `*args`, `**kwargs`를 사용한다. |
| Py-08-04 | M | 기본값이 있는 인자는 뒤에 배치한다. |
| Py-08-05 | M | mutable 객체(list, dict)를 기본값으로 사용하지 않는다. |
| Py-08-06 | O | 타입 힌트를 사용하여 인자와 반환값의 타입을 명시한다. |
| Py-08-07 | M | public 함수에는 docstring을 작성한다. |

```python
# Good
def fetch_data(
    url: str,
    timeout: int = 30,
    headers: dict | None = None,
) -> dict:
    """Fetch data from the given URL.

    Args:
        url: The target URL to fetch.
        timeout: Request timeout in seconds.
        headers: Optional HTTP headers.

    Returns:
        Parsed JSON response as dictionary.

    Raises:
        RequestError: If the request fails.
    """
    if headers is None:
        headers = {}
    # ...

# Bad - mutable default argument
def append_to(element, target=[]):
    target.append(element)
    return target
```

## 9. Classes (Py-09)

| ID | Category | Rule Description |
|----|----------|------------------|
| Py-09-01 | M | 클래스 정의 후 docstring을 작성한다. |
| Py-09-02 | M | `__init__`에서 모든 인스턴스 변수를 초기화한다. |
| Py-09-03 | M | 메서드 순서: `__init__` -> 특수 메서드 -> public 메서드 -> private 메서드 |
| Py-09-04 | O | 클래스 변수와 인스턴스 변수를 명확히 구분한다. |
| Py-09-05 | M | 상속 시 `super()`를 사용한다. |
| Py-09-06 | O | 데이터 클래스는 `@dataclass` 데코레이터를 사용한다. (Python 3.7+) |

```python
from dataclasses import dataclass


class BaseProcessor:
    """Base class for data processors."""

    def __init__(self, name: str):
        self.name = name
        self._cache = {}

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}(name={self.name!r})"

    def process(self, data):
        """Process the given data."""
        raise NotImplementedError

    def _validate(self, data):
        """Internal validation method."""
        pass


@dataclass
class Config:
    """Configuration container."""

    host: str
    port: int = 8080
    debug: bool = False
```

## 10. Error Handling (Py-10)

| ID | Category | Rule Description |
|----|----------|------------------|
| Py-10-01 | M | 베어 `except:` 절을 사용하지 않는다. 최소한 `except Exception:` |
| Py-10-02 | M | 예외는 구체적인 타입을 명시하여 처리한다. |
| Py-10-03 | M | 예외 재발생 시 `raise`만 사용하여 트레이스백을 보존한다. |
| Py-10-04 | M | 예외 체이닝 시 `raise ... from ...`을 사용한다. |
| Py-10-05 | O | 커스텀 예외는 `Exception`을 상속하고 명확한 이름을 사용한다. |
| Py-10-06 | M | `try` 블록은 최소한의 코드만 포함한다. |

```python
# Good
try:
    value = data["key"]
except KeyError:
    logger.warning("Key not found, using default")
    value = default_value
except (TypeError, ValueError) as e:
    raise ConfigurationError("Invalid data format") from e

# Bad
try:
    # too much code here
    ...
except:
    pass
```

## 11. Context Managers (Py-11)

| ID | Category | Rule Description |
|----|----------|------------------|
| Py-11-01 | M | 파일 작업에는 `with` 문을 사용한다. |
| Py-11-02 | M | 리소스 관리가 필요한 작업에는 context manager를 사용한다. |
| Py-11-03 | O | 여러 context manager는 괄호로 그룹화한다. (Python 3.10+) |

```python
# Good
with open("file.txt", "r") as f:
    content = f.read()

with (
    open("input.txt") as infile,
    open("output.txt", "w") as outfile,
):
    outfile.write(infile.read())

# Bad
f = open("file.txt", "r")
content = f.read()
f.close()
```

## 12. Comprehensions (Py-12)

| ID | Category | Rule Description |
|----|----------|------------------|
| Py-12-01 | M | 단순 변환/필터에는 comprehension을 사용한다. |
| Py-12-02 | M | 복잡한 로직은 일반 루프를 사용한다. |
| Py-12-03 | M | 중첩 comprehension은 2단계까지만 허용한다. |
| Py-12-04 | O | generator expression은 즉시 소비되는 경우에만 괄호 생략 가능. |

```python
# Good - simple comprehension
squares = [x**2 for x in range(10)]
even_squares = [x**2 for x in range(10) if x % 2 == 0]
mapping = {k: v for k, v in pairs}

# Good - use regular loop for complex logic
results = []
for item in items:
    if complex_condition(item):
        transformed = complex_transformation(item)
        results.append(transformed)

# Bad - too complex
result = [[y * 2 for y in x if y > 0] for x in matrix if sum(x) > 10]
```

## 13. Type Hints (Py-13)

| ID | Category | Rule Description |
|----|----------|------------------|
| Py-13-01 | O | public API에는 타입 힌트를 작성한다. |
| Py-13-02 | O | Python 3.10+ 문법 사용: `list[int]`, `dict[str, int]`, `X \| None` |
| Py-13-03 | O | 복잡한 타입은 `TypeAlias`로 정의한다. |
| Py-13-04 | O | 제네릭은 `TypeVar`를 사용한다. |

```python
from typing import TypeAlias

JsonValue: TypeAlias = dict[str, "JsonValue"] | list["JsonValue"] | str | int | float | bool | None

def parse_config(path: str) -> dict[str, JsonValue]:
    ...

def process_items(items: list[int] | None = None) -> list[int]:
    if items is None:
        items = []
    return [x * 2 for x in items]
```

## 14. Documentation (Py-14)

| ID | Category | Rule Description |
|----|----------|------------------|
| Py-14-01 | M | 모든 public 모듈, 클래스, 함수에 docstring을 작성한다. |
| Py-14-02 | M | docstring 스타일은 Google style을 사용한다. |
| Py-14-03 | M | 한 줄 docstring은 한 줄에 모두 작성한다. (여는 따옴표, 내용, 닫는 따옴표) |
| Py-14-04 | M | 여러 줄 docstring은 요약 -> 빈 줄 -> 상세 설명 순서로 작성한다. |

```python
def simple_function():
    """Return the answer to everything."""
    return 42


def complex_function(param1: str, param2: int = 0) -> dict:
    """Process the input and return results.

    This function performs complex processing on the input
    parameters and returns a structured result.

    Args:
        param1: The primary input string.
        param2: Optional multiplier value.

    Returns:
        A dictionary containing the processed results with keys:
        - 'status': Processing status string
        - 'data': Processed data list

    Raises:
        ValueError: If param1 is empty.
        TypeError: If param2 is not an integer.

    Example:
        >>> result = complex_function("test", 2)
        >>> print(result["status"])
        'success'
    """
    pass
```

---

## Tools

| Tool | Purpose | Configuration |
|------|---------|---------------|
| Black | 자동 포맷팅 | `pyproject.toml` - `line-length = 88` |
| Flake8 | 린팅 | `.flake8` - `max-line-length = 88` |
| isort | import 정렬 | `pyproject.toml` - `profile = "black"` |
| mypy | 타입 체크 (Optional) | `pyproject.toml` |

### Configuration Example (`pyproject.toml`)

```toml
[tool.black]
line-length = 88
target-version = ['py310']

[tool.isort]
profile = "black"
line_length = 88

[tool.mypy]
python_version = "3.10"
warn_return_any = true
warn_unused_ignores = true
```

### Configuration Example (`.flake8`)

```ini
[flake8]
max-line-length = 88
extend-ignore = E203, E501
exclude = .git,__pycache__,build,dist
```

---

## Legend

- **M (Mandatory)**: 필수 준수 규칙
- **O (Optional)**: 권장 규칙
