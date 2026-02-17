# RFC (Request for Comments) Convention

## Purpose

This document defines RFC naming, structure, and content rules.
Operational workflow and tooling are documented in `docs/rfc/README.md`.

---

## Naming Convention

### Format

```
NNN-short-description.md
```

### Components

| Component | Rule | Example |
|-----------|------|---------|
| `NNN` | Zero-padded 3-digit sequential number | `001`, `042`, `123` |
| `-` | Hyphen separator (required) | `-` |
| `short-description` | Lowercase, hyphen-separated, descriptive | `context-management`, `git-workflow` |
| `.md` | Markdown extension (required) | `.md` |

### Valid Examples

- [V] `001-context-management-system.md`
- [V] `002-parallel-task-execution.md`
- [V] `010-git-workflow-automation.md`
- [V] `042-ci-integration.md`

### Invalid Examples

- [X] `1-context.md` (not zero-padded)
- [X] `001_context_management.md` (underscore instead of hyphen)
- [X] `001-ContextManagement.md` (not lowercase)
- [X] `RFC-001-context.md` (prefix not allowed)
- [X] `001-context-mgmt.txt` (not .md extension)

---

## File Structure

### Required Header

```markdown
# RFC NNN: {Title}

**Status:** {Proposed|Accepted|Rejected|Implemented|Superseded}
**Author:** @{github-username}
**Created:** YYYY-MM-DD
**Updated:** YYYY-MM-DD
**Issue:** #{issue-number}
```

**Note:** Add issue number after creating the issue

### Required Sections

1. **Summary**: 1-2 sentence overview
2. **Motivation**: Problem statement and inspiration
3. **Design**: Detailed technical design
4. **Implementation Plan**: Phased approach with concrete steps
5. **Expected Benefits**: Quantified benefits with Problem/Solution/Effect
6. **Risks & Mitigations**: Table format with Probability/Impact
7. **Alternatives Considered**: At least 2 alternatives with Pros/Cons/Decision
8. **References**: Links to related docs/articles
9. **Checklist**: Track RFC progress

### Optional Sections

- **Open Questions**: Unresolved issues
- **Diagrams**: Visual representations
- **Examples**: Code samples or usage examples

---

## Status Lifecycle

```
Proposed -> Accepted -> Implemented
         -> Rejected
         -> Superseded (by RFC-XXX)
```

| Status | Meaning |
|--------|---------|
| `Proposed` | Under review |
| `Accepted` | Approved |
| `Rejected` | Not proceeding |
| `Implemented` | Completed |
| `Superseded` | Replaced |

---

## Content Guidelines

### Language

- **Technical content**: English only
- **Code comments**: English only
- **Markdown prose**: Korean or English (project default: Korean for internal docs)

### Style

- Use tables for comparisons
- Use code blocks for examples
- Use bullet points for lists
- Keep sentences concise
- Avoid emoji and decorative Unicode

### Structure

- Each section should stand alone
- Use consistent heading levels
- Include concrete examples
- Quantify benefits where possible

---

## References

- Process details: `docs/rfc/README.md`
- Design skill: `skills/design.md`
- Architecture philosophy: `docs/ARCHITECTURE.md`
