# SoC C++ Coding Convention Rules

> **Standard**: C++17 (ISO/IEC 14882:2017)
>
> **References**:
> - [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
> - [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
> - [Linux Kernel Coding Style](https://www.kernel.org/doc/html/latest/process/coding-style.html) (C 기반)
> - [c.md](c.md) (기본 규칙 참조)

## 0. C 규칙 상속 (CPP-00)

C++ 코드는 [c.md](c.md)의 모든 규칙을 기본으로 상속한다. 아래는 C++ 전용 추가/수정 규칙이다.

## 1. Formatting (CPP-01)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| CPP-01-01 | M | C-01 규칙을 모두 따른다. | Yes | clang-format | - |
| CPP-01-02 | M | 클래스/구조체의 접근 지정자(public, protected, private)는 들여쓰지 않는다. | Yes | clang-format | AccessModifierOffset |
| CPP-01-03 | M | 클래스 정의의 시작 중괄호({)는 클래스 선언부와 같은 줄에 위치한다. | Yes | clang-format | AfterClass |
| CPP-01-04 | M | 네임스페이스의 시작 중괄호({)는 선언부와 같은 줄에 위치한다. | Yes | clang-format | AfterNamespace |
| CPP-01-05 | M | 네임스페이스 내부 코드는 들여쓰지 않는다. | Yes | clang-format | NamespaceIndentation |
| CPP-01-06 | M | 닫는 중괄호 뒤에 네임스페이스 이름을 주석으로 명시한다: `} // namespace foo` | Yes | clang-tidy | llvm-namespace-comment |
| CPP-01-07 | M | 템플릿 선언과 함수/클래스 정의 사이에 빈 줄을 두지 않는다. | Yes | clang-format | - |
| CPP-01-08 | M | 참조자(&)와 포인터(*)는 타입이 아닌 변수명 쪽에 붙인다: `int *ptr`, `int &ref` | Yes | clang-format | PointerAlignment |
| CPP-01-09 | O | 람다 표현식이 길 경우 캡처 리스트 뒤에서 줄을 바꾼다. | Yes | clang-format | - |

```cpp
namespace foo {

class MyClass {
public:
    MyClass();
    ~MyClass();

    void public_method();

protected:
    void protected_method();

private:
    void private_method();
    int member_;
};

} // namespace foo
```

## 2. Naming (CPP-02)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| CPP-02-01 | M | C-03 규칙을 기본으로 따른다. | Yes | clang-tidy | readability-identifier-naming |
| CPP-02-02 | M | 클래스명은 PascalCase를 사용한다: `MyClassName` | Yes | clang-tidy | readability-identifier-naming |
| CPP-02-03 | M | 멤버 변수는 후행 밑줄(trailing underscore)을 사용한다: `member_variable_` | Yes | clang-tidy | readability-identifier-naming |
| CPP-02-04 | M | 함수명, 지역 변수, 매개변수는 snake_case를 사용한다. | Yes | clang-tidy | readability-identifier-naming |
| CPP-02-05 | M | 상수와 열거형 값은 UPPER_SNAKE_CASE를 사용한다. | Yes | clang-tidy | readability-identifier-naming |
| CPP-02-06 | M | 네임스페이스명은 소문자 snake_case를 사용한다. | Yes | clang-tidy | readability-identifier-naming |
| CPP-02-07 | M | 템플릿 타입 매개변수는 PascalCase를 사용한다: `template<typename T>` | No | - | - |
| CPP-02-08 | O | getter는 `get_` 접두사 없이 멤버명으로, setter는 `set_` 접두사를 사용한다. | No | - | - |

```cpp
namespace my_namespace {

constexpr int MAX_BUFFER_SIZE = 1024;

enum class ErrorCode {
    SUCCESS = 0,
    INVALID_ARGUMENT,
    OUT_OF_MEMORY,
};

template<typename ValueType>
class DataContainer {
public:
    void set_value(ValueType val) { value_ = val; }
    ValueType value() const { return value_; }

private:
    ValueType value_;
};

} // namespace my_namespace
```

## 3. Classes and Structs (CPP-03)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| CPP-03-01 | M | struct는 POD(Plain Old Data) 타입에만 사용하고, 멤버 함수가 있으면 class를 사용한다. | No | - | - |
| CPP-03-02 | M | 클래스 멤버 순서: public → protected → private | Yes | clang-format | - |
| CPP-03-03 | M | 각 접근 지정자 내 순서: types/aliases → static constants → constructors → destructor → methods → data members | No | - | - |
| CPP-03-04 | M | 가상 소멸자가 필요한 클래스는 반드시 virtual ~ClassName()을 선언한다. | Yes | clang-tidy | cppcoreguidelines-virtual-class-destructor |
| CPP-03-05 | M | 오버라이드하는 가상 함수에는 반드시 override 키워드를 명시한다. | Yes | clang-tidy | modernize-use-override |
| CPP-03-06 | M | 오버라이드 시 virtual 키워드를 중복 사용하지 않는다. | Yes | clang-tidy | modernize-use-override |
| CPP-03-07 | M | 상속을 금지할 클래스는 final을 사용한다. | No | - | - |
| CPP-03-08 | M | 복사/이동이 금지되어야 하는 클래스는 명시적으로 = delete 처리한다. | Yes | clang-tidy | cppcoreguidelines-special-member-functions |
| CPP-03-09 | O | Rule of Zero/Five를 따른다: 특수 멤버 함수가 필요 없으면 정의하지 않고, 하나라도 필요하면 다섯 개 모두 정의한다. | Yes | clang-tidy | cppcoreguidelines-special-member-functions |

```cpp
class NonCopyable {
public:
    NonCopyable() = default;
    ~NonCopyable() = default;

    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;
    NonCopyable(NonCopyable &&) = default;
    NonCopyable &operator=(NonCopyable &&) = default;
};

class Base {
public:
    virtual ~Base() = default;
    virtual void process() = 0;
};

class Derived final : public Base {
public:
    void process() override;  // virtual 생략, override 명시
};
```

## 4. Modern C++ Features (CPP-04)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| CPP-04-01 | M | NULL 대신 nullptr을 사용한다. | Yes | clang-tidy | modernize-use-nullptr |
| CPP-04-02 | M | C 스타일 캐스트 대신 static_cast, dynamic_cast, const_cast, reinterpret_cast를 사용한다. | Yes | clang-tidy | cppcoreguidelines-pro-type-cstyle-cast |
| CPP-04-03 | M | 가능한 경우 auto를 사용하되, 타입이 명확하지 않으면 명시적 타입을 사용한다. | No | - | - |
| CPP-04-04 | M | 범위 기반 for 루프를 사용한다: `for (auto &item : container)` | Yes | clang-tidy | modernize-loop-convert |
| CPP-04-05 | M | 컨테이너 초기화에는 중괄호 초기화를 사용한다: `std::vector<int> v{1, 2, 3};` | No | - | - |
| CPP-04-06 | M | 람다에서 불필요한 캡처를 하지 않는다. [=]나 [&] 남용 금지. | Yes | clang-tidy | cppcoreguidelines-avoid-capture-default-when-capturing-this |
| CPP-04-07 | O | constexpr을 사용하여 컴파일 타임 상수와 함수를 정의한다. | No | - | - |
| CPP-04-08 | M | enum 대신 enum class를 사용한다. | Yes | clang-tidy | modernize-use-using |
| CPP-04-09 | M | typedef 대신 using을 사용한다. | Yes | clang-tidy | modernize-use-using |
| CPP-04-10 | O | 구조적 바인딩(structured binding)을 활용한다: `auto [key, value] = pair;` | No | - | - |

```cpp
// Good
auto *ptr = static_cast<MyClass *>(void_ptr);
for (const auto &item : items) {
    process(item);
}

using Callback = std::function<void(int)>;
enum class State { IDLE, RUNNING, STOPPED };

constexpr int compute_size(int n) { return n * 2; }

// Bad
MyClass *ptr = (MyClass *)void_ptr;  // C 스타일 캐스트
for (int i = 0; i < items.size(); ++i) { ... }  // 범위 기반 for 사용 가능
typedef std::function<void(int)> Callback;  // typedef 대신 using
enum State { IDLE, RUNNING, STOPPED };  // enum class 사용
```

## 5. Memory Management and RAII (CPP-05)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| CPP-05-01 | M | 동적 할당에는 new/delete 대신 스마트 포인터를 사용한다. | Yes | clang-tidy | cppcoreguidelines-owning-memory |
| CPP-05-02 | M | 소유권이 단독인 경우 std::unique_ptr을 사용한다. | No | - | - |
| CPP-05-03 | M | 소유권을 공유해야 하는 경우에만 std::shared_ptr을 사용한다. | No | - | - |
| CPP-05-04 | M | std::shared_ptr 생성 시 std::make_shared를 사용한다. | Yes | clang-tidy | modernize-make-shared |
| CPP-05-05 | M | std::unique_ptr 생성 시 std::make_unique를 사용한다. | Yes | clang-tidy | modernize-make-unique |
| CPP-05-06 | M | 순환 참조 방지를 위해 std::weak_ptr을 적절히 사용한다. | No | - | - |
| CPP-05-07 | M | RAII 패턴을 따른다: 리소스는 생성자에서 획득하고 소멸자에서 해제한다. | No | - | - |
| CPP-05-08 | M | 스마트 포인터의 raw pointer가 필요할 때는 .get()을 사용한다. | No | - | - |
| CPP-05-09 | M | 소유권 이전 시 std::move를 명시적으로 사용한다. | Yes | clang-tidy | cppcoreguidelines-rvalue-reference-param-not-moved |

```cpp
// Good
auto ptr = std::make_unique<MyClass>(args);
auto shared = std::make_shared<MyClass>(args);

void take_ownership(std::unique_ptr<MyClass> ptr);
take_ownership(std::move(ptr));

// Bad
MyClass *ptr = new MyClass(args);  // raw new
delete ptr;  // manual delete
```

## 6. Error Handling (CPP-06)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| CPP-06-01 | M | 예외는 진정한 예외 상황에서만 사용한다. 정상 제어 흐름에 사용 금지. | No | - | - |
| CPP-06-02 | M | 예외를 던질 때는 std::exception을 상속한 타입을 사용한다. | No | - | - |
| CPP-06-03 | M | 예외를 catch할 때는 const 참조로 받는다: `catch (const std::exception &e)` | Yes | clang-tidy | misc-throw-by-value-catch-by-reference |
| CPP-06-04 | M | 예외를 re-throw할 때는 `throw;`를 사용한다. `throw e;` 사용 금지. | Yes | clang-tidy | misc-throw-by-value-catch-by-reference |
| CPP-06-05 | M | noexcept를 적절히 사용한다. 예외를 던지지 않는 함수에 noexcept 명시. | Yes | clang-tidy | modernize-use-noexcept |
| CPP-06-06 | M | 소멸자, 이동 연산자, swap 함수는 noexcept로 선언한다. | Yes | clang-tidy | performance-noexcept-move-constructor |
| CPP-06-07 | O | 예외가 비활성화된 환경에서는 에러 코드나 std::optional을 사용한다. | No | - | - |

```cpp
class MyException : public std::runtime_error {
public:
    explicit MyException(const std::string &msg)
        : std::runtime_error(msg) {}
};

void risky_operation()
{
    if (error_condition) {
        throw MyException("Operation failed");
    }
}

void safe_wrapper() noexcept
{
    try {
        risky_operation();
    } catch (const std::exception &e) {
        log_error(e.what());
    }
}
```

## 7. Templates (CPP-07)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| CPP-07-01 | M | 템플릿 정의는 헤더 파일에 위치시킨다. | No | - | - |
| CPP-07-02 | O | 복잡한 템플릿 메타프로그래밍은 지양한다. | No | - | - |
| CPP-07-03 | O | SFINAE 대신 C++17 if constexpr 또는 concepts(C++20)를 고려한다. | No | - | - |
| CPP-07-04 | M | 템플릿 인스턴스화 오류 메시지 개선을 위해 static_assert를 활용한다. | No | - | - |
| CPP-07-05 | M | 템플릿 매개변수명은 의미 있게 짓는다: `T` 대신 `ValueType`, `Container` 등 | No | - | - |

```cpp
template<typename Container>
void process_container(const Container &c)
{
    static_assert(
        std::is_same_v<typename Container::value_type, int>,
        "Container must hold int values"
    );

    for (const auto &item : c) {
        // ...
    }
}

template<typename T>
auto safe_divide(T a, T b) -> std::optional<T>
{
    if constexpr (std::is_integral_v<T>) {
        if (b == 0) {
            return std::nullopt;
        }
    }
    return a / b;
}
```

## 8. Standard Library Usage (CPP-08)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| CPP-08-01 | M | C 헤더 대신 C++ 래퍼를 사용한다: `<cstdint>`, `<cstring>`, `<cstdio>` | Yes | clang-tidy | modernize-deprecated-headers |
| CPP-08-02 | M | std::string을 char* 대신 사용한다. | No | - | - |
| CPP-08-03 | M | 고정 크기 배열에는 std::array를 사용한다. | Yes | clang-tidy | modernize-avoid-c-arrays |
| CPP-08-04 | M | 동적 배열에는 std::vector를 사용한다. | No | - | - |
| CPP-08-05 | O | 문자열 조회에는 std::string_view를 사용한다. (C++17) | No | - | - |
| CPP-08-06 | O | 값이 없을 수 있음을 표현할 때 std::optional을 사용한다. | No | - | - |
| CPP-08-07 | O | 여러 타입 중 하나를 가질 때 std::variant를 사용한다. | No | - | - |
| CPP-08-08 | M | 알고리즘 사용 시 `<algorithm>` 헤더의 표준 알고리즘을 우선한다. | No | - | - |

```cpp
#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include <algorithm>

std::array<int, 10> fixed_array{};
std::vector<int> dynamic_array;

std::optional<int> find_value(std::string_view key)
{
    auto it = std::find_if(items.begin(), items.end(),
        [key](const auto &item) { return item.name == key; });

    if (it != items.end()) {
        return it->value;
    }
    return std::nullopt;
}
```

## 9. Concurrency (CPP-09)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| CPP-09-01 | M | 공유 데이터 접근 시 std::mutex로 보호한다. | No | - | - |
| CPP-09-02 | M | mutex 잠금에는 std::lock_guard 또는 std::unique_lock을 사용한다. | Yes | clang-tidy | cppcoreguidelines-prefer-member-initializer |
| CPP-09-03 | M | 데드락 방지를 위해 std::scoped_lock을 사용한다. (여러 mutex 동시 잠금) | No | - | - |
| CPP-09-04 | M | 조건 변수 사용 시 spurious wakeup을 고려하여 while 루프로 조건 검사한다. | No | - | - |
| CPP-09-05 | M | 원자적 연산에는 std::atomic을 사용한다. | No | - | - |
| CPP-09-06 | O | 스레드 생성/관리에는 std::thread 또는 std::async를 사용한다. | No | - | - |

```cpp
class ThreadSafeQueue {
public:
    void push(int value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(value);
        cv_.notify_one();
    }

    int pop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty(); });
        int value = queue_.front();
        queue_.pop();
        return value;
    }

private:
    std::queue<int> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
};
```

---

## Tools

| Tool | Purpose | Configuration |
|------|---------|---------------|
| clang-format | 자동 포맷팅 | `.clang-format` |
| clang-tidy | 정적 분석/린팅 | `.clang-tidy` |

### Configuration Example (`.clang-format`)

```yaml
BasedOnStyle: LLVM
IndentWidth: 4
UseTab: Always
TabWidth: 4
ColumnLimit: 100
PointerAlignment: Right
ReferenceAlignment: Right
AccessModifierOffset: -4
NamespaceIndentation: None
BreakBeforeBraces: Custom
BraceWrapping:
  AfterClass: false
  AfterControlStatement: false
  AfterFunction: true
  AfterNamespace: false
  BeforeElse: false
AllowShortFunctionsOnASingleLine: None
```

### Configuration Example (`.clang-tidy`)

```yaml
Checks: >
  -*,
  clang-analyzer-*,
  cppcoreguidelines-*,
  modernize-*,
  readability-*,
  bugprone-*,
  performance-*,
  -modernize-use-trailing-return-type
WarningsAsErrors: ''
HeaderFilterRegex: '.*'
CheckOptions:
  - key: readability-identifier-naming.ClassCase
    value: CamelCase
  - key: readability-identifier-naming.MemberCase
    value: lower_case
  - key: readability-identifier-naming.MemberSuffix
    value: '_'
  - key: readability-identifier-naming.FunctionCase
    value: lower_case
  - key: readability-identifier-naming.VariableCase
    value: lower_case
  - key: readability-identifier-naming.NamespaceCase
    value: lower_case
  - key: readability-identifier-naming.ConstantCase
    value: UPPER_CASE
```

---

## Legend

- **M (Mandatory)**: 필수 준수 규칙
- **O (Optional)**: 권장 규칙
