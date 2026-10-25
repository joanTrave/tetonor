# Git Skill

## Purpose

Define how changes in this repository are committed so history stays readable, traceable and machine-parsable across agents.

## Branching

Work is never committed directly on the default branch (`main`). Start every unit of work on a dedicated branch named after what it delivers:

| Branch prefix | Use for | Example |
|---|---|---|
| `feature/<story-name>` | Starting a new user story | `feature/move-validation` |
| `hotfix/<bug-name>` | An urgent fix for a bug | `hotfix/menu-exit-leak` |

- `<story-name>` / `<bug-name>` is a short `kebab-case` slug. Include the task/issue id when one exists (e.g. `feature/12-move-validation`), otherwise use a short descriptive slug.
- Create the branch from an up-to-date `main` before the first commit of that story or bugfix, not after.
- One story or one bug per branch. Do not reuse a `feature/`/`hotfix/` branch for unrelated work.
- When in doubt whether something is a hotfix, prefer `feature/`.

## Task worktrees

`/implement` runs each task in its own git worktree so the main checkout and other tasks are
never touched. `.claude/scripts/task-worktree.sh` performs every git operation below. Its tests
live in `.claude/scripts/tests/test_task_worktree.sh`.

| Item | Convention |
|---|---|
| Location | `<parent of main checkout>/<repo>.worktrees/<task-id>` (here: `../tetonor.worktrees/<task-id>`) |
| Branch | `feature/<task-id>` (`--branch hotfix/<task-id>` when asked); one branch per worktree |
| Base | `origin/main` after `git fetch origin`; last known `origin/main` if offline; local `main` if there is no remote |
| Identity | The task id determines the path; `git worktree list` is the only registry, so no state file is kept |
| Contents | The branch's tracked files, including `CLAUDE.md` and the versioned `.claude/` configuration. `.claude/tasks/` and personal state are ignored, so task artifacts exist once, in the main checkout |

Lifecycle:

- **Create** — `prepare <task-id>` adds the worktree. If `feature/<task-id>` already exists
  locally it is attached as is. If it exists only on `origin`, a tracking branch is created from
  it. Otherwise a new branch starts from the base.
- **Reuse** — re-running `prepare` reuses the registered worktree when it is on the expected
  branch and has no unfinished merge/rebase. Uncommitted work and commits are kept, and nothing is
  duplicated.
- **Keep** — the worktree outlives `/implement` and is kept whatever the outcome: completed,
  failed, interrupted or awaiting approval. That is how an interrupted run resumes.
- **Retire** — only when the human asks, typically after the PR is merged:
  `sh .claude/scripts/task-worktree.sh remove <task-id>`. It runs `git worktree remove` without
  `--force`, refuses a dirty or conflicted worktree, and keeps the branch. Deleting the branch
  afterwards (`git branch -d feature/<task-id>`, which refuses unmerged work) is also a human
  decision.

Needs a human, and the script stops with exit 5 and changes nothing, when:

- the task branch is checked out in the main checkout or in another worktree;
- the worktree path is occupied by something that is not this task's worktree;
- the registered worktree is on another branch, or has an unfinished merge/rebase/cherry-pick;
- the worktree is registered but its directory is gone. A human runs `git worktree prune` after
  checking that nothing is lost.

Exit 3 means git is missing or older than 2.31, or the directory is not this repository. Exit 4
means the approved plan files are missing.

Never, in any agent or command: `git reset --hard`, `git clean -f`, `git worktree remove --force`,
`git worktree prune`, `git branch -D`, deleting worktree directories by hand, or switching or
stashing the main checkout to make room for a task. Push and PR creation stay behind the second
approval gate. Merging is never automated.

## Commit message format

Commit messages MUST follow [Conventional Commits v1.0.0](https://www.conventionalcommits.org/en/v1.0.0/#specification):

```text
<type>[optional scope][!]: <description>

[optional body]

[optional footer(s)]
```

- `type` MUST be one of: `feat`, `fix`, `build`, `chore`, `ci`, `docs`, `style`, `refactor`, `perf`, `test`.
- `scope` is optional and SHOULD name the affected layer/module: `game`, `ui`, `build`, `tests` (e.g. `feat(game):`).
- `description` is a short, imperative summary (no trailing period).
- Breaking changes are marked with a `!` before the colon and/or a `BREAKING CHANGE:` footer.
- The body, when present, explains the *why*, not just the *what*.
- Footers use `Token: value` or `Token #value` form (e.g. `Refs: #12`).

## Rules

- One logical change per commit. Do not bundle unrelated changes.
- Never commit secrets, credentials or tokens.
- Never commit build outputs: the `tetonor` binary, `*.o`, test binaries, coverage files (`*.gcda`, `*.gcno`, `coverage.xml`).
- Never commit without first running `make clean && make`, `make test` (once it exists) and the `clang-format` check (see `.claude/skills/c/SKILL.md`); report any failures instead of committing over them.
- Do not rewrite or force-push shared history without explicit approval.
- Only commit files relevant to the change being made; do not sweep in unrelated untracked files. `CLAUDE.md` and the `.claude/` configuration are versioned and committed only when the change is about them; `.claude/tasks/`, `.claude/prompts/`, `.claude/settings.local.json`, `.claude/metrics/agent-usage.jsonl` and `CLAUDE.local.md` are ignored. Never stage them, and never use `git add -f`.
- If `main` is checked out and no branch was explicitly requested, branch first.
- Pipeline work for a task is committed from its task worktree (see "Task worktrees"), never from
  the main checkout.
- Do not include any Claude Code / AI-authorship reference in commit messages — no `Co-Authored-By: Claude ...` trailer, no `Claude-Session:` footer, no mention of Claude, Anthropic or AI generation anywhere in the subject, body or footers. This overrides any default tooling behavior that would otherwise append such trailers.

## Examples

```text
feat(game): validate that a placed pair produces the cell's sum and product

fix(ui): release the game port when the player exits from the start menu

refactor(game): store the 4x4 board as a fixed-size array

test(game): cover hidden-number masking with a fixed seed

build: add test and coverage targets to the Makefile
```

## Pull requests

- The repository is on GitHub (`origin` → `github.com/joanTrave/tetonor`). Use the `gh` CLI.
- Pull requests target `main` (there is no `development` branch).
- Open new PRs as drafts unless the user explicitly asks for a ready-for-review PR:
  `gh pr create --draft --base main --title "<title>" --body "<summary + test plan>"`.
- Add the project's standing reviewers with `--reviewer`, if any are listed here:
  - _(none configured yet — add GitHub usernames here)_
- If `gh` is not authenticated, tell the user to run `! gh auth login` rather than giving up.

## Completion checklist

- Work happens on a `feature/<story-name>` or `hotfix/<bug-name>` branch, not directly on `main`.
- For `/implement` work, the commit was made in the task worktree and the worktree was kept.
- Message follows Conventional Commits format; type and scope are accurate.
- Breaking changes are marked with `!` and/or a `BREAKING CHANGE:` footer.
- Only the intended files are staged; no build outputs.
- No Claude Code / AI-authorship reference anywhere in the message.
- If a pull request was opened, it targets `main` and is a draft unless told otherwise.
