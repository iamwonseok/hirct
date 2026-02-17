# SoC YAML/Docker/CI Coding Convention Rules

> **Standard**: YAML 1.2
>
> **References**:
> - [YAML Specification](https://yaml.org/spec/1.2.2/)
> - [Dockerfile Best Practices](https://docs.docker.com/develop/develop-images/dockerfile_best-practices/)
> - [GitHub Actions Syntax](https://docs.github.com/en/actions/reference/workflow-syntax-for-github-actions)
> - [GitLab CI/CD Syntax](https://docs.gitlab.com/ee/ci/yaml/)

---

## 1. YAML Formatting (YAML-01)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| YAML-01-01 | M | 들여쓰기는 공백 2칸을 사용한다. (탭 금지) | Yes | yamllint | indentation |
| YAML-01-02 | O | 한 줄은 최대 100자를 넘지 않도록 작성한다. | Yes | yamllint | line-length |
| YAML-01-03 | M | 줄의 끝(Trailing whitespace)에 공백 문자를 남기지 않는다. | Yes | yamllint | trailing-spaces |
| YAML-01-04 | M | 파일의 마지막에 빈 줄 하나를 추가한다. | Yes | yamllint | new-line-at-end-of-file |
| YAML-01-05 | M | 파일의 줄바꿈 형식은 Unix 스타일(LF)을 사용한다. | Yes | yamllint | new-lines |

## 2. YAML Structure (YAML-02)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| YAML-02-01 | M | 대괄호([ ]) 내부에 불필요한 공백을 두지 않는다. | Yes | yamllint | brackets |
| YAML-02-02 | M | 중괄호({ }) 내부에 불필요한 공백을 두지 않는다. | Yes | yamllint | braces |
| YAML-02-03 | M | 콜론(:) 앞에는 공백을 두지 않고, 뒤에는 공백 하나를 둔다. | Yes | yamllint | colons |
| YAML-02-04 | M | 쉼표(,) 앞에는 공백을 두지 않고, 뒤에는 공백 하나를 둔다. | Yes | yamllint | commas |
| YAML-02-05 | M | 리스트 항목의 하이픈(-) 뒤에는 공백 하나를 둔다. | Yes | yamllint | hyphens |
| YAML-02-06 | O | Flow style({ }, [ ])보다 Block style을 선호한다. | No | - | - |
| YAML-02-07 | O | 복잡한 구조는 앵커(&)와 별칭(*)을 사용하여 중복을 제거한다. | No | - | - |

## 3. YAML Content (YAML-03)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| YAML-03-01 | M | 동일한 레벨에서 중복된 키를 사용하지 않는다. | Yes | yamllint | key-duplicates |
| YAML-03-02 | O | 키의 순서는 논리적 그룹화 또는 알파벳 순으로 정렬한다. | No | - | - |
| YAML-03-03 | O | 빈 값은 명시적으로 `null` 또는 `~`로 표현한다. | Yes | yamllint | empty-values |
| YAML-03-04 | M | Boolean 값은 `true`/`false`를 사용한다. (yes/no, on/off 금지) | Yes | yamllint | truthy |
| YAML-03-05 | O | 숫자가 아닌 값이 숫자로 해석될 수 있으면 따옴표로 감싼다. | No | - | - |

## 4. YAML Comments (YAML-04)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| YAML-04-01 | M | 주석 `#` 기호 뒤에는 공백 하나를 추가한다. | Yes | yamllint | comments |
| YAML-04-02 | O | 주석은 해당 항목과 동일한 들여쓰기 레벨에 위치한다. | Yes | yamllint | comments-indentation |
| YAML-04-03 | O | 섹션 구분에는 주석 블록을 사용한다. | No | - | - |

## 5. YAML Document (YAML-05)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| YAML-05-01 | O | 문서 시작에 `---` 마커를 사용한다. | Yes | yamllint | document-start |
| YAML-05-02 | O | 다중 문서 파일에서는 `...`로 문서 끝을 명시한다. | Yes | yamllint | document-end |

## 6. YAML Quoting (YAML-06)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| YAML-06-01 | O | 특수문자가 포함된 문자열은 따옴표로 감싼다. | Yes | yamllint | quoted-strings |
| YAML-06-02 | O | 변수 확장이 필요한 경우 큰따옴표를 사용한다. | No | - | - |
| YAML-06-03 | O | 리터럴 문자열은 작은따옴표를 사용한다. | No | - | - |

## 7. YAML Special Values (YAML-07)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| YAML-07-01 | M | 8진수 값은 `0o` 접두사를 사용한다. (암시적 8진수 금지) | Yes | yamllint | octal-values |
| YAML-07-02 | O | 긴 문자열은 리터럴 블록(`|`) 또는 접힌 블록(`>`)을 사용한다. | No | - | - |

---

## 8. Dockerfile Formatting (Docker-01)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Docker-01-01 | M | 베이스 이미지는 명시적인 태그를 사용한다. (`:latest` 금지) | Yes | hadolint | DL3006, DL3007 |
| Docker-01-02 | O | 패키지 설치 시 버전을 명시한다. | Yes | hadolint | DL3008 |
| Docker-01-03 | M | 명령어 키워드(FROM, RUN 등)는 대문자를 사용한다. | No | - | - |
| Docker-01-04 | O | 각 명령어 블록 사이에 빈 줄을 추가한다. | No | - | - |

## 9. Dockerfile Optimization (Docker-02)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Docker-02-01 | M | 관련 RUN 명령어는 `&&`로 결합하여 레이어 수를 최소화한다. | Yes | hadolint | DL3059 |
| Docker-02-02 | O | 변경 빈도가 낮은 명령어를 상단에 배치한다. (캐시 최적화) | No | - | - |
| Docker-02-03 | M | apt-get 사용 후 캐시를 삭제한다. (`rm -rf /var/lib/apt/lists/*`) | Yes | hadolint | DL3009 |
| Docker-02-04 | O | 멀티스테이지 빌드를 활용하여 최종 이미지 크기를 줄인다. | No | - | - |

## 10. Dockerfile Security (Docker-03)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Docker-03-01 | M | 비밀 정보(키, 비밀번호)를 빌드 인자나 ENV에 포함하지 않는다. | Yes | hadolint | DL3060 |
| Docker-03-02 | O | `.dockerignore` 파일을 사용하여 불필요한 파일 복사를 방지한다. | No | - | - |
| Docker-03-03 | O | 프로덕션 이미지에서는 root가 아닌 사용자로 실행한다. | Yes | hadolint | DL3002 |

## 11. Dockerfile Instructions (Docker-04)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Docker-04-01 | M | 단순 파일 복사에는 `ADD` 대신 `COPY`를 사용한다. | Yes | hadolint | DL3010 |
| Docker-04-02 | O | `CMD`와 `ENTRYPOINT`는 exec form(JSON 배열)을 사용한다. | Yes | hadolint | DL3025 |
| Docker-04-03 | M | `WORKDIR`은 절대 경로를 사용한다. | Yes | hadolint | DL3003 |
| Docker-04-04 | O | `LABEL`을 사용하여 이미지 메타데이터를 명시한다. | No | - | - |

## 12. Dockerfile Package Management (Docker-05)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Docker-05-01 | O | pip 설치 시 `--no-cache-dir` 옵션을 사용한다. | Yes | hadolint | DL3013 |
| Docker-05-02 | O | npm 설치 시 `--production` 플래그를 고려한다. | Yes | hadolint | DL3016 |
| Docker-05-03 | O | apk 설치 시 `--no-cache` 옵션을 사용한다. | Yes | hadolint | DL3018 |

---

## 13. docker-compose Structure (Compose-01)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Compose-01-01 | M | 버전 명시: Compose V2에서는 `version` 키 생략 가능 | No | - | - |
| Compose-01-02 | M | 서비스 이름은 소문자와 하이픈(kebab-case)을 사용한다. | No | - | - |
| Compose-01-03 | O | 서비스 정의 순서: 메인 서비스 -> 의존 서비스 -> 인프라 서비스 | No | - | - |
| Compose-01-04 | O | 환경 변수는 `.env` 파일 또는 `environment` 섹션에 정의한다. | No | - | - |

## 14. docker-compose Best Practices (Compose-02)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| Compose-02-01 | M | 볼륨 마운트 시 상대 경로는 `./`로 시작한다. | No | - | - |
| Compose-02-02 | O | 서비스 간 의존성은 `depends_on`으로 명시한다. | No | - | - |
| Compose-02-03 | O | 네트워크는 명시적으로 정의한다. (기본 네트워크 사용 지양) | No | - | - |
| Compose-02-04 | O | 리소스 제한(`deploy.resources`)을 설정한다. | No | - | - |

---

## 15. CI/CD General (CI-01)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| CI-01-01 | M | 작업(job) 이름은 명확하고 설명적으로 작성한다. | No | - | - |
| CI-01-02 | M | 민감 정보는 시크릿/변수 기능을 사용한다. (하드코딩 금지) | No | - | - |
| CI-01-03 | O | 공통 설정은 앵커/템플릿을 사용하여 재사용한다. | No | - | - |
| CI-01-04 | O | 파이프라인 실행 조건(브랜치, 태그 등)을 명시한다. | No | - | - |

## 16. GitHub Actions (CI-02)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| CI-02-01 | M | 액션 버전은 메이저 버전 태그 또는 SHA를 사용한다. | No | - | - |
| CI-02-02 | O | `permissions`를 명시하여 최소 권한 원칙을 적용한다. | No | - | - |
| CI-02-03 | O | 캐시(`actions/cache`)를 활용하여 빌드 시간을 단축한다. | No | - | - |
| CI-02-04 | O | 매트릭스 빌드로 여러 환경을 병렬 테스트한다. | No | - | - |

## 17. GitLab CI (CI-03)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| CI-03-01 | M | 스테이지(stages)를 명시적으로 정의한다. | No | - | - |
| CI-03-02 | O | `extends`를 사용하여 작업 설정을 재사용한다. | No | - | - |
| CI-03-03 | O | `rules`를 사용하여 작업 실행 조건을 정의한다. (`only`/`except` 대신) | No | - | - |
| CI-03-04 | O | `cache`와 `artifacts`를 구분하여 사용한다. | No | - | - |

---

## Tools

| Tool | Purpose | Configuration |
|------|---------|---------------|
| yamllint | YAML 린팅 | `.yamllint.yml` |
| hadolint | Dockerfile 린팅 | `.hadolint.yaml` |
| docker-compose config | Compose 파일 검증 | - |

### Usage Examples

```bash
# YAML 린팅
yamllint -c .yamllint.yml file.yaml

# Dockerfile 린팅
hadolint --config .hadolint.yaml Dockerfile

# docker-compose 검증
docker-compose config --quiet
```

### Configuration Reference

yamllint 설정: [`.yamllint.yml`](../../.yamllint.yml)
hadolint 설정: [`.hadolint.yaml`](../../.hadolint.yaml)

---

## Legend

- **M (Mandatory)**: 필수 준수 규칙
- **O (Optional)**: 권장 규칙
