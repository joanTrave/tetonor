#!/bin/sh
# Tests for .claude/scripts/task-worktree.sh. Every case builds a throwaway origin + clone under a
# temporary directory; the real repository is never touched.
#
# Run: sh .claude/scripts/tests/test_task_worktree.sh
set -u

SCRIPT_DIR=$(cd "$(dirname "$0")/.." && pwd)
SCRIPT="$SCRIPT_DIR/task-worktree.sh"
TMP_ROOT=$(mktemp -d)
trap 'rm -rf "$TMP_ROOT"' EXIT

# Isolate the tests from the user's git configuration (hooks, signing, templates).
: >"$TMP_ROOT/gitconfig"
export GIT_CONFIG_GLOBAL="$TMP_ROOT/gitconfig" GIT_CONFIG_NOSYSTEM=1
export GIT_AUTHOR_NAME=tester GIT_AUTHOR_EMAIL=tester@example.com
export GIT_COMMITTER_NAME=tester GIT_COMMITTER_EMAIL=tester@example.com

FAILURES=0
CASES=0
CURRENT=""

fail() {
	printf '  FAIL [%s]: %s\n' "$CURRENT" "$*"
	FAILURES=$((FAILURES + 1))
}

assert_eq() {
	[ "$1" = "$2" ] || fail "$3: expected '$1', got '$2'"
}

assert_true() {
	msg=$1
	shift
	"$@" || fail "$msg"
}

field() {
	printf '%s\n' "$2" | sed -n "s/^$1=//p"
}

# Runs the script from directory $1; sets OUT, ERR and CODE.
run() {
	dir=$1
	shift
	OUT=$(cd "$dir" && sh "$SCRIPT" "$@" 2>"$TMP_ROOT/stderr")
	CODE=$?
	ERR=$(cat "$TMP_ROOT/stderr")
}

add_task_artifacts() {
	mkdir -p "$REPO/.claude/tasks/$1/plan" "$REPO/.claude/tasks/$1/specs"
	echo plan >"$REPO/.claude/tasks/$1/plan/plan.md"
	echo gate >"$REPO/.claude/tasks/$1/plan/architecture-gate.md"
	echo spec >"$REPO/.claude/tasks/$1/specs/$1.md"
}

# Fresh origin + main checkout with the factory files and two tasks. Sets REPO and WT_ROOT.
new_repo() {
	CASES=$((CASES + 1))
	CURRENT=$1
	case_dir="$TMP_ROOT/case$CASES"
	mkdir -p "$case_dir"
	git init --quiet --bare -b main "$case_dir/origin.git"
	git clone --quiet "$case_dir/origin.git" "$case_dir/repo" 2>/dev/null
	REPO=$(git -C "$case_dir/repo" rev-parse --path-format=absolute --show-toplevel)
	WT_ROOT="$(dirname "$REPO")/repo.worktrees"
	printf '.claude\nbuild/\n' >"$REPO/.gitignore"
	echo 'int main(void) { return 0; }' >"$REPO/main.c"
	git -C "$REPO" add .gitignore main.c
	git -C "$REPO" commit --quiet -m "chore: initial"
	git -C "$REPO" push --quiet -u origin main 2>/dev/null
	mkdir -p "$REPO/.claude/commands"
	echo implement >"$REPO/.claude/commands/implement.md"
	add_task_artifacts T-1
	add_task_artifacts T-2
}

# Everything observable about the main checkout: HEAD, branch, index, working tree, untracked files.
main_snapshot() {
	git -C "$REPO" rev-parse HEAD
	git -C "$REPO" symbolic-ref --quiet HEAD
	git -C "$REPO" status --porcelain --untracked-files=all
	git -C "$REPO" diff
	git -C "$REPO" diff --cached
	git -C "$REPO" ls-files --others --exclude-standard | while read -r f; do cat "$REPO/$f"; done
}

worktree_count() {
	git -C "$REPO" worktree list --porcelain | grep -c '^worktree '
}

test_new_task_creates_worktree_outside_main_checkout() {
	new_repo new_task
	before=$(main_snapshot)
	run "$REPO" prepare T-1
	assert_eq 0 "$CODE" "exit code ($ERR)"
	assert_eq created "$(field ACTION "$OUT")" ACTION
	assert_eq "$WT_ROOT/T-1" "$(field WORKTREE "$OUT")" WORKTREE
	assert_eq feature/T-1 "$(field BRANCH "$OUT")" BRANCH
	assert_eq origin/main "$(field BASE "$OUT")" BASE
	assert_eq yes "$(field BASE_FRESH "$OUT")" BASE_FRESH
	assert_eq feature/T-1 "$(git -C "$WT_ROOT/T-1" symbolic-ref --short HEAD)" "worktree branch"
	assert_eq "$(git -C "$REPO" rev-parse origin/main)" "$(git -C "$WT_ROOT/T-1" rev-parse HEAD)" "branch starts at base"
	assert_eq "" "$(git -C "$WT_ROOT/T-1" rev-parse --abbrev-ref '@{upstream}' 2>/dev/null)" "no upstream yet"
	assert_eq 2 "$(worktree_count)" "registered worktrees"
	assert_eq "$before" "$(main_snapshot)" "main checkout untouched"
}

test_rerun_reuses_worktree_and_keeps_progress() {
	new_repo rerun
	run "$REPO" prepare T-1
	echo progress >"$WT_ROOT/T-1/progress.c"
	before=$(main_snapshot)
	run "$REPO" prepare T-1
	assert_eq 0 "$CODE" "exit code ($ERR)"
	assert_eq reused "$(field ACTION "$OUT")" ACTION
	assert_eq yes "$(field WORKTREE_DIRTY "$OUT")" WORKTREE_DIRTY
	assert_eq progress "$(cat "$WT_ROOT/T-1/progress.c")" "uncommitted progress kept"
	assert_eq 2 "$(worktree_count)" "no duplicate worktree"
	assert_eq 1 "$(git -C "$REPO" branch --list 'feature/*' | wc -l | tr -d ' ')" "no duplicate branch"
	assert_eq "$before" "$(main_snapshot)" "main checkout untouched"
}

test_two_tasks_get_independent_worktrees_and_branches() {
	new_repo two_tasks
	run "$REPO" prepare T-1
	echo one >"$WT_ROOT/T-1/one.c"
	run "$REPO" prepare T-2
	assert_eq 0 "$CODE" "exit code ($ERR)"
	assert_eq "$WT_ROOT/T-2" "$(field WORKTREE "$OUT")" WORKTREE
	assert_eq feature/T-2 "$(git -C "$WT_ROOT/T-2" symbolic-ref --short HEAD)" "second branch"
	assert_eq 3 "$(worktree_count)" "registered worktrees"
	assert_true "first task's file stays out of the second worktree" test ! -e "$WT_ROOT/T-2/one.c"
	assert_eq one "$(cat "$WT_ROOT/T-1/one.c")" "first worktree untouched"
}

test_dirty_main_checkout_is_preserved_and_not_carried_over() {
	new_repo dirty_main
	echo 'int main(void) { return 1; }' >"$REPO/main.c"
	echo draft >"$REPO/draft.c"
	echo staged >"$REPO/staged.c"
	git -C "$REPO" add staged.c
	before=$(main_snapshot)
	run "$REPO" prepare T-1
	assert_eq 0 "$CODE" "exit code ($ERR)"
	assert_eq yes "$(field MAIN_DIRTY "$OUT")" MAIN_DIRTY
	assert_eq main "$(field MAIN_BRANCH "$OUT")" MAIN_BRANCH
	assert_eq "$before" "$(main_snapshot)" "local changes kept exactly"
	assert_eq 'int main(void) { return 0; }' "$(cat "$WT_ROOT/T-1/main.c")" "modification not carried over"
	assert_true "untracked file not carried over" test ! -e "$WT_ROOT/T-1/draft.c"
	assert_true "staged file not carried over" test ! -e "$WT_ROOT/T-1/staged.c"
}

test_paths_are_independent_of_the_calling_directory() {
	new_repo paths
	run "$REPO" prepare T-1
	from_main=$OUT
	run "$WT_ROOT/T-1" prepare T-1
	assert_eq 0 "$CODE" "exit code from inside the worktree ($ERR)"
	assert_eq "$(field MAIN_CHECKOUT "$from_main")" "$(field MAIN_CHECKOUT "$OUT")" MAIN_CHECKOUT
	assert_eq "$(field WORKTREE "$from_main")" "$(field WORKTREE "$OUT")" WORKTREE
	assert_eq "$REPO" "$(field MAIN_CHECKOUT "$OUT")" "MAIN_CHECKOUT is the main checkout"
	assert_eq "$REPO/.claude/tasks/T-1" "$(field ARTIFACTS_DIR "$OUT")" "artifacts stay in the main checkout"
	case $(field WORKTREE "$OUT") in
	"$REPO"/*) fail "worktree must live outside the main checkout" ;;
	esac
	assert_true "no second copy of the factory in the worktree" test ! -e "$WT_ROOT/T-1/.claude"
	run "$REPO/.claude" path T-1
	assert_eq "$WT_ROOT/T-1" "$(field WORKTREE "$OUT")" "path from a subdirectory"
}

test_interrupted_run_resumes_with_commits_and_changes() {
	new_repo resume
	run "$REPO" prepare T-1
	echo committed >"$WT_ROOT/T-1/feature.c"
	git -C "$WT_ROOT/T-1" add feature.c
	git -C "$WT_ROOT/T-1" commit --quiet -m "feat: partial"
	echo wip >>"$WT_ROOT/T-1/feature.c"
	head=$(git -C "$WT_ROOT/T-1" rev-parse HEAD)
	run "$REPO" prepare T-1
	assert_eq reused "$(field ACTION "$OUT")" ACTION
	assert_eq "$head" "$(git -C "$WT_ROOT/T-1" rev-parse HEAD)" "commits kept"
	assert_eq "committed
wip" "$(cat "$WT_ROOT/T-1/feature.c")" "uncommitted change kept"
	run "$REPO" status T-1
	assert_eq yes "$(field REGISTERED "$OUT")" REGISTERED
	assert_eq 1 "$(field COMMITS_AHEAD "$OUT")" COMMITS_AHEAD
	assert_eq yes "$(field DIRTY "$OUT")" DIRTY
}

test_removed_worktree_reattaches_existing_branch() {
	new_repo reattach
	run "$REPO" prepare T-1
	echo kept >"$WT_ROOT/T-1/kept.c"
	git -C "$WT_ROOT/T-1" add kept.c
	git -C "$WT_ROOT/T-1" commit --quiet -m "feat: kept"
	run "$REPO" remove T-1
	assert_eq 0 "$CODE" "remove exit code ($ERR)"
	assert_eq feature/T-1 "$(field BRANCH_KEPT "$OUT")" BRANCH_KEPT
	assert_true "branch survives removal" git -C "$REPO" show-ref --verify --quiet refs/heads/feature/T-1
	run "$REPO" prepare T-1
	assert_eq 0 "$CODE" "exit code ($ERR)"
	assert_eq attached "$(field ACTION "$OUT")" ACTION
	assert_eq kept "$(cat "$WT_ROOT/T-1/kept.c")" "branch work restored"
}

test_pushed_branch_without_local_copy_is_attached_from_remote() {
	new_repo remote_branch
	git -C "$REPO" branch feature/T-1
	git -C "$REPO" push --quiet origin feature/T-1 2>/dev/null
	git -C "$REPO" branch --quiet -d feature/T-1
	run "$REPO" prepare T-1
	assert_eq 0 "$CODE" "exit code ($ERR)"
	assert_eq attached-remote "$(field ACTION "$OUT")" ACTION
	assert_eq origin/feature/T-1 "$(git -C "$WT_ROOT/T-1" rev-parse --abbrev-ref '@{upstream}')" upstream
}

test_occupied_path_is_refused_without_changes() {
	new_repo occupied
	mkdir -p "$WT_ROOT/T-1"
	echo mine >"$WT_ROOT/T-1/notes.txt"
	run "$REPO" prepare T-1
	assert_eq 5 "$CODE" "exit code"
	assert_eq mine "$(cat "$WT_ROOT/T-1/notes.txt")" "foreign directory untouched"
	assert_true "no branch created" sh -c "! git -C '$REPO' show-ref --verify --quiet refs/heads/feature/T-1"
	assert_eq 1 "$(worktree_count)" "no worktree registered"
}

test_branch_checked_out_in_main_checkout_is_refused() {
	new_repo branch_in_main
	git -C "$REPO" switch --quiet -c feature/T-1
	echo wip >"$REPO/wip.c"
	before=$(main_snapshot)
	run "$REPO" prepare T-1
	assert_eq 5 "$CODE" "exit code"
	case $ERR in *"main checkout"*) ;; *) fail "error names the main checkout: $ERR" ;; esac
	assert_eq "$before" "$(main_snapshot)" "main checkout untouched"
	assert_eq 1 "$(worktree_count)" "no worktree registered"
}

test_branch_checked_out_in_another_worktree_is_refused() {
	new_repo branch_elsewhere
	git -C "$REPO" worktree add --quiet -b feature/T-1 "$(dirname "$REPO")/elsewhere" 2>/dev/null
	run "$REPO" prepare T-1
	assert_eq 5 "$CODE" "exit code"
	case $ERR in *elsewhere*) ;; *) fail "error names the other worktree: $ERR" ;; esac
	assert_eq 2 "$(worktree_count)" "no extra worktree"
}

test_worktree_on_unexpected_branch_is_refused() {
	new_repo wrong_branch
	run "$REPO" prepare T-1
	git -C "$WT_ROOT/T-1" switch --quiet -c experiment
	run "$REPO" prepare T-1
	assert_eq 5 "$CODE" "exit code"
	assert_eq experiment "$(git -C "$WT_ROOT/T-1" symbolic-ref --short HEAD)" "worktree branch left alone"
}

test_stale_worktree_is_reported_not_pruned() {
	new_repo stale
	run "$REPO" prepare T-1
	rm -rf "$WT_ROOT/T-1"
	run "$REPO" prepare T-1
	assert_eq 5 "$CODE" "exit code"
	case $ERR in *"git worktree prune"*) ;; *) fail "error explains the manual prune: $ERR" ;; esac
	assert_eq 2 "$(worktree_count)" "stale registration not pruned automatically"
	run "$REPO" status T-1
	assert_eq stale "$(field REGISTERED "$OUT")" REGISTERED
}

test_unfinished_merge_is_refused_and_left_in_place() {
	new_repo merge_conflict
	run "$REPO" prepare T-1
	echo 'int main(void) { return 2; }' >"$WT_ROOT/T-1/main.c"
	git -C "$WT_ROOT/T-1" commit --quiet -am "feat: two"
	git -C "$REPO" branch --quiet --no-track other origin/main
	git -C "$REPO" worktree add --quiet "$(dirname "$REPO")/other" other 2>/dev/null
	echo 'int main(void) { return 3; }' >"$(dirname "$REPO")/other/main.c"
	git -C "$(dirname "$REPO")/other" commit --quiet -am "feat: three"
	git -C "$WT_ROOT/T-1" merge --quiet other >/dev/null 2>&1
	run "$REPO" prepare T-1
	assert_eq 5 "$CODE" "exit code"
	assert_true "merge left for a human" test -n "$(git -C "$WT_ROOT/T-1" diff --name-only --diff-filter=U)"
	run "$REPO" remove T-1
	assert_eq 5 "$CODE" "remove refuses a conflicted worktree"
}

test_remove_refuses_dirty_worktree() {
	new_repo remove_dirty
	run "$REPO" prepare T-1
	echo unsaved >"$WT_ROOT/T-1/unsaved.c"
	run "$REPO" remove T-1
	assert_eq 5 "$CODE" "exit code"
	assert_eq unsaved "$(cat "$WT_ROOT/T-1/unsaved.c")" "untracked work kept"
	assert_eq 2 "$(worktree_count)" "worktree still registered"
}

test_missing_approved_artifacts_stop_before_any_change() {
	new_repo missing_artifacts
	rm "$REPO/.claude/tasks/T-1/plan/architecture-gate.md"
	run "$REPO" prepare T-1
	assert_eq 4 "$CODE" "missing gate"
	rm -rf "$REPO/.claude/tasks/T-2/plan"
	run "$REPO" prepare T-2
	assert_eq 4 "$CODE" "missing plan"
	run "$REPO" prepare T-3
	assert_eq 4 "$CODE" "unknown task"
	assert_eq 1 "$(worktree_count)" "nothing created"
	assert_true "no worktree directory created" test ! -e "$WT_ROOT"
}

test_missing_spec_is_a_warning() {
	new_repo missing_spec
	rm "$REPO/.claude/tasks/T-1/specs/T-1.md"
	run "$REPO" prepare T-1
	assert_eq 0 "$CODE" "exit code ($ERR)"
	assert_eq missing "$(field SPEC "$OUT")" SPEC
}

test_invalid_input_and_environment_are_rejected() {
	new_repo invalid_input
	for bad in ../T-1 T/1 .hidden "T 1" a..b; do
		run "$REPO" prepare "$bad"
		assert_eq 2 "$CODE" "task id '$bad'"
	done
	run "$REPO" prepare T-1 --branch 'bad..name'
	assert_eq 2 "$CODE" "invalid branch name"
	run "$REPO" prepare
	assert_eq 2 "$CODE" "missing task id"
	mkdir -p "$TMP_ROOT/not-a-repo"
	run "$TMP_ROOT/not-a-repo" prepare T-1
	assert_eq 3 "$CODE" "outside a repository"
	rm "$REPO/.claude/commands/implement.md"
	run "$REPO" prepare T-1
	assert_eq 3 "$CODE" "repository without the factory"
	OUT=$(cd "$REPO" && TASK_WORKTREE_GIT=git-does-not-exist sh "$SCRIPT" prepare T-1 2>/dev/null)
	assert_eq 3 "$?" "git not available"
	assert_eq 1 "$(worktree_count)" "nothing created"
}

test_offline_base_falls_back_to_last_known_remote() {
	new_repo offline
	git -C "$REPO" remote set-url origin "$TMP_ROOT/missing-origin.git"
	run "$REPO" prepare T-1
	assert_eq 0 "$CODE" "exit code ($ERR)"
	assert_eq origin/main "$(field BASE "$OUT")" BASE
	assert_eq no "$(field BASE_FRESH "$OUT")" BASE_FRESH
}

test_tracked_instructions_keep_artifacts_in_main_checkout() {
	new_repo tracked_instructions
	# Layout after the instructions become versioned: the factory is tracked, only the task
	# artifacts (and personal files) stay ignored.
	printf '.claude/tasks/\n.claude/settings.local.json\nbuild/\n' >"$REPO/.gitignore"
	git -C "$REPO" add .gitignore .claude/commands/implement.md
	git -C "$REPO" commit --quiet -m "chore: track instructions"
	git -C "$REPO" push --quiet origin main 2>/dev/null
	before=$(main_snapshot)
	run "$REPO" prepare T-1
	assert_eq 0 "$CODE" "exit code ($ERR)"
	assert_eq created "$(field ACTION "$OUT")" ACTION
	assert_eq "$REPO/.claude/tasks/T-1" "$(field ARTIFACTS_DIR "$OUT")" "artifacts stay in the main checkout"
	assert_eq no "$(field MAIN_DIRTY "$OUT")" MAIN_DIRTY
	assert_true "tracked instructions are checked out in the worktree" test -f "$WT_ROOT/T-1/.claude/commands/implement.md"
	assert_true "checked-out instructions are tracked" \
		git -C "$WT_ROOT/T-1" ls-files --error-unmatch .claude/commands/implement.md >/dev/null
	assert_true "no second copy of the task artifacts in the worktree" test ! -e "$WT_ROOT/T-1/.claude/tasks"
	assert_eq "$before" "$(main_snapshot)" "main checkout untouched"
	run "$REPO" remove T-1
	assert_eq 0 "$CODE" "a worktree with tracked instructions is clean and removable ($ERR)"
	assert_true "artifacts survive removal" test -f "$REPO/.claude/tasks/T-1/plan/plan.md"
}

test_script_contains_no_destructive_or_publishing_git_operation() {
	CURRENT=static_safety
	if grep -nE 'reset --hard|clean -[a-z]*f|--force|branch -D|worktree prune *$|"\$GIT"[^#]* (push|merge|rebase|switch|checkout|stash)( |$)' \
		"$SCRIPT" | grep -v '^[0-9]*:[[:space:]]*#' | grep -v "die \$E_" | grep -v 'warn ' >"$TMP_ROOT/static"; then
		fail "forbidden operation: $(cat "$TMP_ROOT/static")"
	fi
}

for test_case in $(sed -n 's/^\(test_[a-z_]*\)() {$/\1/p' "$0"); do
	"$test_case"
	cd "$TMP_ROOT" || exit 1
done

if [ "$FAILURES" -eq 0 ]; then
	echo "task-worktree: all tests passed"
	exit 0
fi
echo "task-worktree: $FAILURES assertion(s) failed"
exit 1
