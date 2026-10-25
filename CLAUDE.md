# Tetonor

Terminal (TUI) implementation of the Tetonor number puzzle, written in portable C11 with curses.
It must build and run on Linux, macOS and Windows without OS-specific code, except in `src/platform/` (see Layout).

## Stack and commands

- C11, clang (`-std=c11 -Wall -Wextra -Werror -pedantic`), curses (ncurses on Unix, PDCurses on Windows).
- Build: `make` · Clean: `make clean` · Tests: `make test` · Sanitizers: `make sanitize` · Coverage: `make coverage` · Diff coverage (>= 90%): `make diff-coverage` · Format check: `make format-check`. See `.claude/skills/testing/SKILL.md`.
- Windows: run every make target through MSYS2 UCRT64 bash (see `README.md`), e.g. `MSYSTEM=UCRT64 CHERE_INVOKING=1 MSYS2_PATH_TYPE=inherit C:/msys64/usr/bin/bash.exe -lc 'export PATH="$HOME/.venvs/tetonor-tools/bin:$PATH"; make test'`.
- Format: `clang-format` with the repository `.clang-format`.

## Layout

- `src/main.c` — composition root (seed RNG, start UI).
- `src/game/` — portable game core, standard C only; `game_port.h` is the UI's only entry point.
- `src/ui/` — curses screens and input; the only place curses is used.
- `src/storage/` — statistics file I/O, standard `<stdio.h>` only.
- `src/platform/` — the only place OS-specific code is allowed (per-user data directory lookup and creation), kept minimal behind an OS-neutral header.
- `.claude/` — agent workflow configuration (agents, commands, skills, scripts), versioned; `.claude/tasks/` holds per-task artifacts and is not versioned.
- `CONTRIBUTING.md` — human contributor guide.

Coding standards: `docs/best_practices.md`. Stack skills: `.claude/skills/c/`, `.claude/skills/tui/`,
`.claude/skills/architecture/`, `.claude/skills/naming/`, `.claude/skills/testing/`.

# Task Workflow

When starting work on a new task, follow this pipeline. Each agent is invoked with only the minimum context it needs for its step (the ticket details/requirements and the specific prior artifact it must act on) — not the full conversation history.

0. **Refiner** (`/refine-task` command → `refiner` agent): transform a technical task (a `TETO-NNN` key read from this repository's GitHub issues, a Jira key, raw text, or a URL) into the canonical mission spec at `.claude/tasks/<task-id>/specs/<task-id>.md`.
1. **Architect** (`/architect-plan <task-id>` command → `architect` agent): from `.claude/tasks/<task-id>/specs/<task-id>.md`, produce the implementation plan at `.claude/tasks/<task-id>/plan/plan.md` **and** the architecture gate report at `.claude/tasks/<task-id>/plan/architecture-gate.md`.
2. **Approval gate**: stop and wait for the user to approve the architecture before proceeding. Read `architecture-gate.md` to approve — it is the deterministic decision document; `plan.md` is what the downstream agents read, not what the human reviews line by line.
3. Once approved, `/implement <task-id>` first prepares (or safely reuses) the task's git worktree at `../tetonor.worktrees/<task-id>` on `feature/<task-id>`. It never touches the main checkout. Then it runs the following in order, all inside that worktree; task artifacts stay in the main checkout's `.claude/tasks/` (see `.claude/skills/git/SKILL.md` § "Task worktrees"):
   - **Tester** (`tester` agent): write tests derived from the approved plan. These tests are Tester-owned once written.
   - **Developer** (`developer` agent): implement the feature per the approved plan and run the tests written by the tester until they pass. The Developer must not modify or delete Tester-owned tests to make an implementation pass (see `.claude/agents/delivery/developer.md`).
   - **Reviewer** (`reviewer` agent): review the implementation (diff) for correctness, architecture, maintainability, security, and test quality; produce `.claude/tasks/<task-id>/reviews/review.json` (authoritative) and `.claude/tasks/<task-id>/reviews/review.md` (rendered from it); return an explicit approve/reject recommendation.
4. **Approval gate**: once the reviewer approves, stop and wait for the user to approve before committing.
5. Once the user approves, the **Developer** commits the change (Conventional Commits) from the task worktree. The worktree is kept afterwards; removing it is a separate, explicit human request.

Do not skip either approval gate, and do not let the developer commit before the second approval is given.

## Artifact locations

| Artifact | Path | Produced by |
|---|---|---|
| Mission spec | `.claude/tasks/<task-id>/specs/<task-id>.md` | Refiner |
| Implementation plan | `.claude/tasks/<task-id>/plan/plan.md` | Architect |
| Architecture gate report | `.claude/tasks/<task-id>/plan/architecture-gate.md` | Architect |
| Review (machine-readable) | `.claude/tasks/<task-id>/reviews/review.json` | Reviewer |
| Review (human-readable) | `.claude/tasks/<task-id>/reviews/review.md` | Reviewer |

`<task-id>` is the tracker key when there is one (`TETO-004` for a GitHub issue, `BOF-3006` for Jira), otherwise a kebab-case slug. See
`.claude/skills/artifacts/SKILL.md` for the full layout and the report templates/schemas, and
`.claude/skills/mission-spec/SKILL.md` for the mission-spec template.
