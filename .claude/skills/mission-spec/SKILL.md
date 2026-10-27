# Mission Spec Skill

## Purpose

Define the canonical mission-spec template — the contract between a technical task (`TETO-NNN` GitHub
issue key, Jira key, raw text, or URL) and every downstream delivery agent — and the rules the `refiner` agent follows when
producing one. Sole consumer: the `refiner` agent.

## Task identifier

The tracker key when there is one — the `TETO-NNN` key of a GitHub issue (`TETO-004`) or a Jira key
(`BOF-3100`) — otherwise a kebab-case slug (`better-workflow`). This
is simultaneously the mission-spec filename stem (`.claude/tasks/<task-id>/specs/<task-id>.md`) and, later in the
pipeline, the task-artifact directory name (`.claude/tasks/<task-id>/`).

## Naming

`.claude/tasks/<task-id>/specs/<task-id>.md`. Tracker key verbatim when available (`TETO-004.md`, `BOF-3100.md`); otherwise kebab-case
(`better-workflow.md`). Pre-existing snake_case mission specs are grandfathered, not renamed.

## Canonical template

The exact section skeleton shared by every existing mission spec in this repository. All of these
top-level `#` sections, in this order:

```markdown
# <task-id>                     ← MUST equal the filename stem

# Problem
<current behaviour, why it is wrong, what the reporter/ticket observed>

# Solution
<intended behaviour; numbered steps or ### sub-sections when the change has parts>

# Variables
- `PLAN_DIR: .claude/tasks/<task-id>/plan/`
- `CONVERSATION_LANGUAGE: <English name of the language, e.g. Spanish>`
- `<NAME>: <value>`            ← one bullet per concrete constant the task hinges on

[# Assumptions                  ← OPTIONAL, see rule below; only when assumptions exist]
- ASSUMPTION: <statement> — basis: <repo path or "none, needs confirmation"> — if wrong: <impact>

# Workflow
See CLAUDE.md for the workflow definition.
<any task-specific restriction, e.g. a commit restriction>

# Definition Of Done
## Workflow Complete
- [ ] ...
[## <themed group> — optional, one per major deliverable]

# Functional Requirements
- [ ] ...

# Technical Requirements
- [ ] Must follow `docs/best_practices.md`.
- [ ] Must follow the existing conventions defined in `CLAUDE.md`.
- [ ] ...

# Verification
- [ ] Relevant automated tests pass (`make test`, see `.claude/skills/testing/SKILL.md`).
- [ ] `make clean && make` builds with zero warnings (`-Werror`).
- [ ] `clang-format --dry-run --Werror` passes on every touched file.
- [ ] ...

# How You're Graded
- ...
```

## Refiner-specific rules

- **Verifiable checkboxes.** Every checkbox under *Definition Of Done*, *Functional Requirements*,
  *Technical Requirements* and *Verification* must be a statement a reviewer can mark true/false
  from code, test output or command output — never a vague aspiration.
- **Assumptions are isolated.** A requirement that does not come from the supplied task must appear
  **only** under a new `# Assumptions` section, inserted between `# Variables` and `# Workflow`,
  one bullet each in the form:
  `ASSUMPTION: <statement> — basis: <repo path or "none, needs confirmation"> — if wrong: <impact>`.
  This is the single additive section beyond the canonical template; it exists because the
  template otherwise has nowhere to put an assumption. Omit the section entirely when there are
  none — do not write an empty heading.
- **No solution design.** The Refiner may read the repository to resolve technical context, and
  **must** cite the paths it relied on inside `# Problem`/`# Solution` — but must not design the
  solution's internals: no file-by-file change lists, no class/function names it invented, no test
  plan. Those belong to the Architect, not the Refiner.
- **`PLAN_DIR` convention.** Always `.claude/tasks/<task-id>/plan/` — this is the single
  plan-artifact-path convention used throughout the factory; do not invent an alternative.
- **`CONVERSATION_LANGUAGE` convention.** Always present: the English name of the task's
  conversation language, determined per `CLAUDE.md` § "Conversation language". It is metadata for
  the session that talks to the human and never puts non-English text into the spec.
- **GitHub issues are the primary source.** Tasks live as issues in this repository's GitHub
  tracker (`joanTrave/tetonor`), titled `TETO-NNN — <summary>`; ingest them per
  *GitHub issue ingestion* below.
- **Jira ingestion is optional.** If Atlassian MCP tools are available in the session, the Refiner
  may fetch the issue directly from a Jira key. If they are not available, or the input is neither
  a `TETO-NNN` key, a GitHub issue nor a Jira key, ask the human to paste the task text instead.
  Either way, the resulting mission spec looks identical — the template does not vary by input
  source.

## GitHub issue ingestion

Input `TETO-<n>` (case-insensitive, any zero padding, e.g. `teto-4`, `TETO-004`):

1. Normalise the key to upper case with a three-digit number (`TETO-004`). This is the task id.
2. Resolve it to an issue number by searching open **and** closed issues by title:
   `gh issue list --state all --search '"TETO-004" in:title' --json number,title`.
   The search is fuzzy, so keep only results whose title starts with `TETO-004`. Exactly one must
   remain; on zero or several matches, stop and report to the human.
3. Read the issue:
   `gh issue view <number> --json number,title,body,labels,state,url,comments`.
   The body (typically `## Objetivo`, `## Criterios de aceptación` and a priority/value/effort
   line) plus any comments are the supplied task: acceptance criteria become *Functional
   Requirements*; anything else the spec needs is an `ASSUMPTION:`.
4. Cite the issue URL in `# Problem`. If the issue is `CLOSED`, warn the human before writing.

Input `#<n>` or a GitHub issue URL: go straight to step 3; use the `TETO-NNN` key from its title as
the task id when present, otherwise the kebab-case slug of the title.

Issues are written in Spanish; write the mission spec in English, translating acceptance criteria
faithfully. If `gh` is missing or not authenticated, stop and ask the human to run
`gh auth login`.

## Validation

A mission spec is well-formed when every canonical `#` section from the template above is present,
in order, and the `# <task-id>` title equals the filename stem.
