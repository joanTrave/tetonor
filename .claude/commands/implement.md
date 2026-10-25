---
description: Prepare the task's git worktree, run Tester → Developer → Reviewer in it (looping Developer↔Reviewer until approved), then the second human approval gate, then Developer commits/pushes/opens the PR from the worktree.
---

Task id supplied below. This command assumes the first approval gate already passed — the human
has read `.claude/tasks/<task-id>/plan/architecture-gate.md` and approved the architecture. If you
have not seen that approval in this conversation, ask the human to confirm it before dispatching
anything; do not infer approval from the mere existence of the plan files. Preparing the worktree
does not count as approval.

Preconditions — stop and tell the human what's missing rather than fabricating it:
- `.claude/tasks/<task-id>/plan/plan.md` must exist (Architect's implementation + test plan).
- `.claude/tasks/<task-id>/plan/architecture-gate.md` must exist.

The implementation runs in a dedicated git worktree, never in the main checkout. Lifecycle,
location and cleanup policy: `.claude/skills/git/SKILL.md` § "Task worktrees".

## 0. Prepare the worktree

From the main checkout (the session's working directory), run:

```bash
sh .claude/scripts/task-worktree.sh prepare <task-id>
```

On Windows run it through Git Bash or MSYS2 bash. It checks git, the repository and the plan files;
it creates or safely reuses `../<repo>.worktrees/<task-id>` on branch `feature/<task-id>`. A new
branch starts from a freshly fetched `origin/main`. The main checkout's files and branch are never
touched. Pass `--branch hotfix/<task-id>` only when the human asked for a hotfix branch, and pass
the same value on every re-run.

- **Exit 0** — keep the printed `MAIN_CHECKOUT`, `ARTIFACTS_DIR`, `WORKTREE`, `BRANCH` and `BASE`
  values for the rest of this run. Tell the human the `ACTION` (`created`, `reused`, `attached`,
  `attached-remote`). If `MAIN_DIRTY=yes`, say that the main checkout's local changes were left as
  they are and are **not** part of this implementation. If `BASE_FRESH=no`, say the base could not
  be fetched.
- **`ACTION=reused`/`attached` with `WORKTREE_DIRTY=yes` or commits ahead of `BASE`** — this is a
  resumed run. Run `sh .claude/scripts/task-worktree.sh status <task-id>` and include the summary
  (changed files, commits ahead) in every dispatch below so agents continue the existing work
  instead of redoing it.
- **Exit 2–5** — stop. Relay the script's error verbatim; it says what the human must resolve.
  Never work around it: do not force, prune, reset, stash, switch the main checkout's branch or
  delete anything, and do not fall back to implementing in the main checkout.

## Working contexts

Every dispatch below passes these absolute paths. Subagents start in the main checkout, so a
relative path or a bare `make` acts on the **wrong tree**.

| Context | Path | Used for |
|---|---|---|
| Worktree | `WORKTREE` | All source/test reads and edits, `git`, `make`, `clang-format` |
| Task artifacts | `ARTIFACTS_DIR` (in the main checkout) | spec, `plan.md`, `architecture-gate.md`, `review.json`, `review.md` — the only copy. `.claude/tasks/` is ignored by git, so a copy in the worktree would be invisible to `git status` and lost on `remove` |
| Instructions | `MAIN_CHECKOUT/.claude/` and `MAIN_CHECKOUT/CLAUDE.md` | the agents, skills and commands in effect for this run. The worktree has the branch's versioned copy, which agents edit only when the plan says so; such edits take effect after merge |
| Main checkout | `MAIN_CHECKOUT` | Nothing in this pipeline edits, builds in, or commits from it |

Put this block, filled in, at the top of every agent prompt:

```text
Task: <task-id>
Worktree (all code changes, builds and git commands here): <WORKTREE>
Branch: <BRANCH> · Base: <BASE>
Task artifacts (read/write here, never copy into the worktree): <ARTIFACTS_DIR>
Instructions: <MAIN_CHECKOUT>/.claude/ and <MAIN_CHECKOUT>/CLAUDE.md
Do not modify files under <MAIN_CHECKOUT> other than <ARTIFACTS_DIR>/reviews/ (Reviewer only).
Run commands as `make -C "<WORKTREE>" <target>`, `git -C "<WORKTREE>" ...`, or
`cd "<WORKTREE>" && ...` in the same command; on Windows wrap make targets per CLAUDE.md with
`cd "<WORKTREE>" && make <target>` inside the MSYS2 `bash -lc` string.
```

Dispatch agents **without** `isolation: "worktree"`, which would give each agent a different tree.
Every agent works in the same task worktree, and agents run one after another, never in parallel.

## 1. Tester

Dispatch the `tester` agent (`.claude/agents/delivery/tester.md`) with the context block and the
path to `plan.md`. The Tester writes tests derived from the approved plan, in the worktree. Once written, these tests are
Tester-owned for the remainder of this task — the Developer must not modify or delete them (see
`.claude/agents/delivery/developer.md`). On a resumed run, the Tester completes or corrects the
tests already in the worktree rather than rewriting them.

## 2. Developer ↔ Reviewer loop

1. Dispatch the `developer` agent (`.claude/agents/delivery/developer.md`) with the context block, the
   plan, and (from the second iteration on) the prior `review.json`/`review.md` findings to
   address. The Developer implements per the plan in the worktree and runs the Tester's tests until they pass,
   without touching Tester-owned tests.
2. Dispatch the `reviewer` agent (`.claude/agents/delivery/reviewer.md`) with the context block. It
   independently reviews the worktree's diff against `BASE`
   (`git -C "<WORKTREE>" diff <BASE>...HEAD` plus the uncommitted changes), never edits the worktree, and writes
   `<ARTIFACTS_DIR>/reviews/review.json` + `<ARTIFACTS_DIR>/reviews/review.md`
   per `.claude/skills/artifacts/SKILL.md`.
3. Read `review.json`'s `status`:
   - `approved` or `approved_with_warnings` → exit the loop, proceed to the human gate below.
   - `changes_requested` → send the findings back to the Developer (step 1) for another pass.
   - `rejected` or `blocked` → stop and report to the human immediately; do not keep looping on a
     rejection that needs a human or architectural decision, and never silently relax the
     Reviewer's gate to force a pass.
4. Cap the loop at 3 Developer↔Reviewer rounds. If still not `approved`/`approved_with_warnings`
   after 3 rounds, stop and report the unresolved findings to the human instead of iterating
   further on your own judgement.

If an agent fails or a check cannot be made to pass, stop and report it. Leave the worktree and its
branch as they are, so the human can inspect them and re-run `/implement <task-id>` to resume.

## 3. Second approval gate

Stop here. Report the Reviewer's final `status`, `overall_score`, any `warnings`, and the paths to
`review.json`/`review.md` to the human. Per `CLAUDE.md`, do not let the Developer commit until the
human explicitly approves.

## 4. Commit, push, open PR

Only after explicit human approval, dispatch the `developer` agent with the context block to:
- Commit in the worktree on `BRANCH` (already the dedicated task branch — never commit from or
  switch the main checkout).
- Commit only the files relevant to this change, using Conventional Commits, with no AI-authorship
  trailer — see the git skill's full rules.
- Push the branch (`git -C "<WORKTREE>" push -u origin <BRANCH>`).
- Open a pull request targeting `main` with `gh pr create`, as a draft unless the human explicitly asked for a
  ready PR, with the standing reviewers from the git skill added (if any are configured).

Report back the branch name, commit hash, and PR URL. Do not merge, and do not remove the worktree.

## 5. Close

Whenever this command stops, at any step, run
`sh .claude/scripts/task-worktree.sh status <task-id>`. End with exactly one of these outcomes plus
the worktree path, branch, `DIRTY`, `COMMITS_AHEAD` and `UNPUSHED`:

| Outcome | When |
|---|---|
| `completed` | Step 4 done: committed, pushed, PR opened. |
| `awaiting-approval` | Reviewer approved; waiting at the second gate. |
| `review-failed` | Reviewer `rejected`/`blocked`, or the 3-round cap was hit. |
| `pending-review` | Implementation exists but no valid review was produced (Reviewer failed or did not run). |
| `interrupted` | Stopped before implementation finished: preparation refused, an agent failed, or tests/build could not pass. |

The worktree is always kept. Removing it is a separate, explicit human decision (git skill §
"Task worktrees").

$ARGUMENTS
