# Contributing to Tetonor

This guide is for people who contribute to Tetonor. It summarises how work flows through the
repository and links to the file that is the source of truth for each rule. When this guide and a
linked file disagree, the linked file wins.

## 1. Overview

Tetonor is a terminal (TUI) implementation of the Tetonor number puzzle, written in portable C11
with curses. Changes are delivered through an agent pipeline that a human drives and approves at
two gates: one on the architecture and one on the finished work. Start with
[`README.md`](README.md) for the game and the toolchain, and with [`CLAUDE.md`](CLAUDE.md) for the
project layout and the pipeline definition.

## 2. Getting set up

1. Install the toolchain for your platform by following [`README.md`](README.md) (clang, make, a
   curses library, clang-format, llvm-cov, and the Python coverage tools).
2. Clone the repository and build once with `make`, then run `make test`.
3. `CLAUDE.md` and the `.claude/` directory (agents, commands, skills, scripts) are versioned, so
   they come with the clone.
4. Personal files stay local and are ignored by git: `.claude/settings.local.json`,
   `.claude/metrics/agent-usage.jsonl` and `CLAUDE.local.md`. Put your own permissions and
   instruction overrides there.

## 3. The delivery pipeline

Every task moves through the same steps, in this order. The definition lives in
[`CLAUDE.md`](CLAUDE.md) under "Task Workflow".

1. **Refine.** A technical task (a `TETO-NNN` issue key, a Jira key, raw text or a URL) becomes a
   mission spec.
2. **Architect.** The architect turns the spec into an implementation plan and an architecture gate
   report.
3. **Gate 1.** You read `architecture-gate.md` and approve or reject it. Nothing is implemented
   before you approve.
4. **Implement**, in a task worktree (section 5): the tester writes tests from the plan, the
   developer implements until they pass, and the reviewer reviews the diff. The developer and
   reviewer repeat up to **3 rounds**. If the work is still not approved after that, the pipeline
   stops and reports to you.
5. **Gate 2.** You approve the reviewed work before anything is committed.
6. **Ship.** The developer commits, pushes and opens a draft pull request.

The artifacts of a task live under `.claude/tasks/<task-id>/`:

| Artifact | Path | Produced by |
|---|---|---|
| Mission spec | `specs/<task-id>.md` | refiner |
| Implementation plan | `plan/plan.md` | architect |
| Architecture gate report | `plan/architecture-gate.md` | architect |
| Review, machine-readable (authoritative) | `reviews/review.json` | reviewer |
| Review, human-readable | `reviews/review.md` | reviewer |

Artifacts are not versioned and never appear in a pull request. They exist once, in the main
checkout, and agents receive their absolute path. The `<task-id>` is the tracker key when there is
one (`TETO-004`, `BOF-3006`), otherwise a kebab-case slug.

Sources of truth: [`.claude/skills/artifacts/SKILL.md`](.claude/skills/artifacts/SKILL.md) (layout
and report templates), [`.claude/skills/mission-spec/SKILL.md`](.claude/skills/mission-spec/SKILL.md)
(spec template) and the agent definitions
[`refiner`](.claude/agents/delivery/refiner.md),
[`architect`](.claude/agents/delivery/architect.md),
[`tester`](.claude/agents/delivery/tester.md),
[`developer`](.claude/agents/delivery/developer.md) and
[`reviewer`](.claude/agents/delivery/reviewer.md).

## 4. Slash commands

| Command | Input | Output |
|---|---|---|
| `/refine-task <key\|text\|url>` | A task key, text or URL | The mission spec |
| `/architect-plan <task-id>` | The mission spec | `plan.md` and `architecture-gate.md`; stops at gate 1 |
| `/implement <task-id>` | An approved plan | A task worktree, tests, code and the review reports; after gate 2, the commit, push and draft PR |
| `/ship <task-id>` | Already reviewed and approved work | Commit, push and PR only |

Definitions: [`refine-task`](.claude/commands/refine-task.md),
[`architect-plan`](.claude/commands/architect-plan.md),
[`implement`](.claude/commands/implement.md) and [`ship`](.claude/commands/ship.md).

## 5. Task worktrees

`/implement` never works in the main checkout. It runs each task in its own git worktree through
`.claude/scripts/task-worktree.sh`.

- Location: `../tetonor.worktrees/<task-id>`, next to the main checkout.
- Branch: `feature/<task-id>`, or `hotfix/<task-id>` when you ask for it with `--branch`.
- Base: `origin/main` after a fetch.
- Commands: `prepare`, `status`, `remove` and `path`, as
  `sh .claude/scripts/task-worktree.sh <command> <task-id>`. On Windows use Git Bash.
- Exit codes: 0 ok; 2 usage error; 3 environment problem (git missing or older than 2.31, or not
  this repository); 4 the approved plan files are missing; 5 a git state that needs a human
  decision, with nothing changed.
- A worktree is kept after the run, whatever the outcome, so an interrupted run can resume. It is
  retired only when you ask, typically after the PR is merged.
- The script and the agents never reset, clean, force-remove, prune, delete branches or switch the
  main checkout's branch. Merging is never automated.

The instructions in effect for a run are the main checkout's `CLAUDE.md` and `.claude/`. The
worktree holds the branch's own versioned copy of those files, and task artifacts stay in the main
checkout.

Sources of truth: [`.claude/skills/git/SKILL.md`](.claude/skills/git/SKILL.md) (policy),
[`.claude/commands/implement.md`](.claude/commands/implement.md) (how it is used) and
[`.claude/scripts/task-worktree.sh`](.claude/scripts/task-worktree.sh) (behaviour).

## 6. Build, test and format

From the repository root:

```sh
make                 # build
make clean           # remove build outputs
make test            # build and run all tests
make sanitize        # tests under AddressSanitizer and UBSan
make coverage        # coverage report
make diff-coverage   # coverage of changed lines, must be >= 90%
make format-check    # clang-format check
sh .claude/scripts/tests/test_task_worktree.sh   # tests of the worktree script
```

On Windows, run every make target through MSYS2 UCRT64 bash:

```powershell
$env:MSYSTEM='UCRT64'; $env:CHERE_INVOKING='1'; $env:MSYS2_PATH_TYPE='inherit'
& 'C:\msys64\usr\bin\bash.exe' -lc 'export PATH="$HOME/.venvs/tetonor-tools/bin:$PATH"; make test'
```

Sources of truth: [`CLAUDE.md`](CLAUDE.md), [`README.md`](README.md) (installation) and
[`.claude/skills/testing/SKILL.md`](.claude/skills/testing/SKILL.md).

## 7. Branches, commits and pull requests

- Never commit on `main`. Use a `feature/<name>` or `hotfix/<name>` branch.
- Write commit messages in the Conventional Commits format (for example
  `fix(ui): release the game port when the player exits from the start menu`). The allowed types
  and scopes are in the git skill.
- One logical change per commit.
- Do not put an AI-authorship trailer or mention in a commit message or a pull request.
- Pull requests target `main` and are opened as drafts by default. The standing reviewers, if any,
  are listed in the git skill.
- Merging is never automated.
- Before committing, `make clean && make`, `make test` and the format check must pass.

Sources of truth: [`.claude/skills/git/SKILL.md`](.claude/skills/git/SKILL.md) and
[`.claude/commands/ship.md`](.claude/commands/ship.md).

## 8. Code and testing conventions

- The language is C11, compiled with `-std=c11 -Wall -Wextra -Werror -pedantic`. The build must
  have zero warnings. See [`.claude/skills/c/SKILL.md`](.claude/skills/c/SKILL.md).
- Code is portable. OS-specific code is allowed only in `src/platform/`.
- Layers are `ui` -> `game_port` -> `game`. Curses is used only in `src/ui/`, and storage uses
  standard `<stdio.h>`. See [`docs/best_practices.md`](docs/best_practices.md),
  [`.claude/skills/architecture/SKILL.md`](.claude/skills/architecture/SKILL.md) and
  [`.claude/skills/tui/SKILL.md`](.claude/skills/tui/SKILL.md).
- Naming and file layout follow
  [`.claude/skills/naming/SKILL.md`](.claude/skills/naming/SKILL.md).
- Tests are Tester-owned once written. The developer fixes the implementation, not the tests.
  Changed lines need at least 90% diff coverage. See
  [`.claude/skills/testing/SKILL.md`](.claude/skills/testing/SKILL.md).

## 9. Changing the workflow itself

`CLAUDE.md` and `.claude/` are ordinary versioned files. Change them through a normal task and a
pull request, like any other code. A local uncommitted edit in the main checkout affects your own
session only, and it does not reach a task worktree. See
[`.claude/skills/git/SKILL.md`](.claude/skills/git/SKILL.md).

## 10. Migrating an existing checkout

This applies once to a checkout created before `CLAUDE.md` and `.claude/` were versioned. In that
checkout they exist as untracked, ignored copies. When you pull a commit that tracks them, git
**overwrites ignored untracked copies without any warning**, and local edits to them are lost.
That is why the first step is a backup. Nothing in this procedure deletes anything.

Run it in Git Bash, from the checkout root, after the change has been merged to `main`.

1. Finish or commit your in-progress work on the current branch as usual. Do not stash or reset to
   make room.
2. Back up outside the repository (a copy, nothing is deleted):
   `B=../tetonor.claude-backup-$(date +%Y%m%d); mkdir "$B" && cp -a CLAUDE.md .claude "$B"/`
3. Run `git fetch origin`, then `git switch main && git pull --ff-only`. For a long-lived branch,
   merge `origin/main` into it instead. If git refuses with "untracked working tree files would
   be overwritten", move exactly the listed paths into `"$B/moved/"` (create the folders with
   `mkdir -p`, and keep their relative paths), then retry.
4. Reconcile with `diff -r "$B/.claude" .claude` and `diff "$B/CLAUDE.md" CLAUDE.md`. A difference
   in a versioned file is a local edit: propose the ones you still want through a normal task.
   Lines that only mention `tasks/`, `prompts/`, `settings.local.json` or `agent-usage.jsonl` mean
   nothing, because those ignored files were never touched.
5. Check that `git status` lists no `.claude/` path and that `.claude/tasks/` is still there.
6. Keep `$B` until you have reconciled. Deleting it is your decision.

Warnings:

- Do not edit the workflow files in a checkout between the moment you decide to migrate and the
  migration itself, or step 4 will show those edits as differences to re-apply.
- Switching a migrated checkout to a branch created **before** this change removes the versioned
  `CLAUDE.md` and `.claude/` files from the working tree while you are on that branch, so the slash
  commands disappear. Ignored state stays. Merge `main` into such branches first.
