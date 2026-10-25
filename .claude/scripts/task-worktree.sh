#!/bin/sh
# Task worktree manager used by /implement. Policy: .claude/skills/git/SKILL.md, "Task worktrees".
#
# Usage: task-worktree.sh <prepare|status|remove|path> <task-id> [--branch <name>] [--no-fetch]
#
# Output is KEY=value lines on stdout; diagnostics go to stderr. Exit codes:
#   0 ok · 2 usage · 3 environment (git, repository) · 4 missing task artifacts
#   5 git state needs a human decision (nothing was changed)
#
# Safety contract: this script never resets, cleans, force-removes, prunes, deletes branches,
# switches the main checkout's branch, merges or pushes. Every refusal leaves the repository as it
# was and says what a human must resolve.
set -u

GIT=${TASK_WORKTREE_GIT:-git}
MIN_GIT_MAJOR=2
MIN_GIT_MINOR=31
BASE_BRANCH=main
REMOTE=origin

E_USAGE=2
E_ENV=3
E_PRECONDITION=4
E_CONFLICT=5

die() {
	code=$1
	shift
	printf 'task-worktree: error: %s\n' "$*" >&2
	exit "$code"
}

warn() {
	printf 'task-worktree: warning: %s\n' "$*" >&2
}

usage() {
	die $E_USAGE "usage: task-worktree.sh <prepare|status|remove|path> <task-id> [--branch <name>] [--no-fetch]"
}

check_git() {
	command -v "$GIT" >/dev/null 2>&1 || die $E_ENV "git is not available on PATH"
	version=$("$GIT" --version 2>/dev/null |
		sed -n 's/^git version \([0-9][0-9]*\)\.\([0-9][0-9]*\).*/\1 \2/p')
	[ -n "$version" ] || die $E_ENV "cannot determine the git version"
	# shellcheck disable=SC2086
	set -- $version
	if [ "$1" -lt $MIN_GIT_MAJOR ] || { [ "$1" -eq $MIN_GIT_MAJOR ] && [ "$2" -lt $MIN_GIT_MINOR ]; }; then
		die $E_ENV "git $1.$2 is too old; $MIN_GIT_MAJOR.$MIN_GIT_MINOR or newer is required"
	fi
}

# Resolves the main checkout from any directory of the repository, including a task worktree.
resolve_main() {
	common=$("$GIT" rev-parse --path-format=absolute --git-common-dir 2>/dev/null) ||
		die $E_ENV "the current directory is not inside a git repository"
	case $common in
	*/.git) MAIN=${common%/.git} ;;
	*) die $E_ENV "unsupported repository layout (git common dir '$common' is not '<checkout>/.git')" ;;
	esac
	[ -f "$MAIN/.claude/commands/implement.md" ] ||
		die $E_ENV "'$MAIN' is not the expected repository: .claude/commands/implement.md is missing"
	# Task artifacts live only in the main checkout's .claude/tasks/ (ignored by git), even though the rest of .claude/ is versioned.
	ARTIFACTS="$MAIN/.claude/tasks/$TASK"
	WORKTREE="$(dirname "$MAIN")/$(basename "$MAIN").worktrees/$TASK"
}

check_task_id() {
	case $1 in
	'' | [!A-Za-z0-9]* | *[!A-Za-z0-9._-]* | *..*) die $E_USAGE "invalid task id '$1'" ;;
	esac
}

# One line per registered worktree: <path> TAB <branch or -> TAB <flags or -> (flags: detached,
# bare, locked, prunable). Placeholders keep empty fields from collapsing under tab IFS splitting.
list_worktrees() {
	"$GIT" -C "$MAIN" worktree list --porcelain | awk '
		function emit() {
			if (path != "")
				print path "\t" (branch == "" ? "-" : branch) "\t" (flags == "" ? "-" : flags)
		}
		/^worktree / { emit(); path = substr($0, 10); branch = ""; flags = ""; next }
		/^branch /   { branch = substr($0, 8); sub(/^refs\/heads\//, "", branch); next }
		/^detached/  { flags = flags "detached,"; next }
		/^bare/      { flags = flags "bare,"; next }
		/^locked/    { flags = flags "locked,"; next }
		/^prunable/  { flags = flags "prunable,"; next }
		END { emit() }'
}

record_field() {
	printf '%s\n' "$1" | awk -F '\t' -v n="$2" '{ print $n }'
}

record_for_path() {
	list_worktrees | awk -F '\t' -v p="$1" '$1 == p { print; exit }'
}

record_for_branch() {
	list_worktrees | awk -F '\t' -v b="$1" '$2 == b { print; exit }'
}

has_ref() {
	"$GIT" -C "$MAIN" show-ref --verify --quiet "$1"
}

# Prints the git operation left half-done in worktree $1 (merge, rebase, ...), if any.
in_progress_operation() {
	for marker in MERGE_HEAD rebase-merge rebase-apply CHERRY_PICK_HEAD REVERT_HEAD; do
		marker_path=$("$GIT" -C "$1" rev-parse --path-format=absolute --git-path "$marker")
		if [ -e "$marker_path" ]; then
			echo "$marker"
			return
		fi
	done
	if [ -n "$("$GIT" -C "$1" diff --name-only --diff-filter=U)" ]; then
		echo "unmerged-paths"
	fi
}

is_dirty() {
	[ -n "$("$GIT" -C "$1" status --porcelain)" ]
}

yes_no() {
	if "$@"; then echo yes; else echo no; fi
}

# Sets BASE to the ref new task branches start from; fetching only updates remote-tracking refs.
resolve_base() {
	BASE_FRESH=no
	if "$GIT" -C "$MAIN" remote get-url "$REMOTE" >/dev/null 2>&1; then
		if [ "$FETCH" = yes ]; then
			if "$GIT" -C "$MAIN" fetch --quiet "$REMOTE"; then
				BASE_FRESH=yes
			else
				warn "could not fetch '$REMOTE'; using the last known $REMOTE/$BASE_BRANCH"
			fi
		fi
	fi
	if has_ref "refs/remotes/$REMOTE/$BASE_BRANCH"; then
		BASE="$REMOTE/$BASE_BRANCH"
	elif has_ref "refs/heads/$BASE_BRANCH"; then
		BASE=$BASE_BRANCH
	else
		die $E_PRECONDITION "cannot determine the base: neither $REMOTE/$BASE_BRANCH nor $BASE_BRANCH exists"
	fi
}

check_task_artifacts() {
	for artifact in plan/plan.md plan/architecture-gate.md; do
		[ -f "$ARTIFACTS/$artifact" ] ||
			die $E_PRECONDITION "missing $ARTIFACTS/$artifact; run /architect-plan $TASK and approve it first"
	done
	SPEC=missing
	if [ -f "$ARTIFACTS/specs/$TASK.md" ]; then
		SPEC=present
	else
		warn "mission spec $ARTIFACTS/specs/$TASK.md is missing"
	fi
}

cmd_path() {
	echo "WORKTREE=$WORKTREE"
}

cmd_prepare() {
	check_task_artifacts
	"$GIT" check-ref-format --branch "$BRANCH" >/dev/null 2>&1 || die $E_USAGE "invalid branch name '$BRANCH'"

	record=$(record_for_path "$WORKTREE")
	if [ -n "$record" ]; then
		case $(record_field "$record" 3) in
		*prunable*)
			die $E_CONFLICT "'$WORKTREE' is registered but its directory is missing (stale worktree)." \
				"Check 'git worktree list'; once nothing there is needed, a human may run" \
				"'git worktree prune' and re-run /implement $TASK"
			;;
		esac
		registered_branch=$(record_field "$record" 2)
		[ "$registered_branch" = "$BRANCH" ] ||
			die $E_CONFLICT "'$WORKTREE' has '$registered_branch' checked out, expected '$BRANCH'; resolve it manually"
		operation=$(in_progress_operation "$WORKTREE")
		[ -z "$operation" ] ||
			die $E_CONFLICT "'$WORKTREE' has an unfinished git operation ($operation); finish or abort it manually"
		resolve_base
		ACTION=reused
	else
		owner=$(record_for_branch "$BRANCH")
		if [ -n "$owner" ]; then
			owner_path=$(record_field "$owner" 1)
			if [ "$owner_path" = "$MAIN" ]; then
				die $E_CONFLICT "'$BRANCH' is checked out in the main checkout '$MAIN'." \
					"/implement never switches the main checkout: commit or stash its changes and" \
					"switch it to another branch yourself, then re-run /implement $TASK"
			fi
			die $E_CONFLICT "'$BRANCH' is already checked out in another worktree at '$owner_path'"
		fi
		if [ -e "$WORKTREE" ] && [ -n "$(ls -A "$WORKTREE" 2>/dev/null || echo occupied)" ]; then
			die $E_CONFLICT "'$WORKTREE' already exists and is not a worktree of this repository; move it away first"
		fi
		resolve_base
		mkdir -p "$(dirname "$WORKTREE")" || die $E_ENV "cannot create '$(dirname "$WORKTREE")'"
		if has_ref "refs/heads/$BRANCH"; then
			ACTION=attached
			set -- "$WORKTREE" "$BRANCH"
		elif has_ref "refs/remotes/$REMOTE/$BRANCH"; then
			ACTION=attached-remote
			set -- --track -b "$BRANCH" "$WORKTREE" "$REMOTE/$BRANCH"
		else
			ACTION=created
			set -- --no-track -b "$BRANCH" "$WORKTREE" "$BASE"
		fi
		"$GIT" -C "$MAIN" worktree add --quiet "$@" >&2 ||
			die $E_CONFLICT "git worktree add failed; nothing was removed"
	fi

	checked_out=$("$GIT" -C "$WORKTREE" symbolic-ref --quiet --short HEAD) || checked_out=""
	[ "$checked_out" = "$BRANCH" ] ||
		die $E_CONFLICT "'$WORKTREE' is on '${checked_out:-detached HEAD}', expected '$BRANCH'"
	main_branch=$("$GIT" -C "$MAIN" symbolic-ref --quiet --short HEAD) || main_branch="(detached)"

	echo "TASK_ID=$TASK"
	echo "MAIN_CHECKOUT=$MAIN"
	echo "ARTIFACTS_DIR=$ARTIFACTS"
	echo "WORKTREE=$WORKTREE"
	echo "BRANCH=$BRANCH"
	echo "BASE=$BASE"
	echo "BASE_FRESH=$BASE_FRESH"
	echo "ACTION=$ACTION"
	echo "WORKTREE_DIRTY=$(yes_no is_dirty "$WORKTREE")"
	echo "MAIN_BRANCH=$main_branch"
	echo "MAIN_DIRTY=$(yes_no is_dirty "$MAIN")"
	echo "SPEC=$SPEC"
}

cmd_status() {
	echo "TASK_ID=$TASK"
	echo "WORKTREE=$WORKTREE"
	record=$(record_for_path "$WORKTREE")
	if [ -z "$record" ]; then
		echo "REGISTERED=no"
		return
	fi
	case $(record_field "$record" 3) in
	*prunable*)
		echo "REGISTERED=stale"
		return
		;;
	esac
	echo "REGISTERED=yes"
	branch=$(record_field "$record" 2)
	echo "BRANCH=$branch"
	echo "DIRTY=$(yes_no is_dirty "$WORKTREE")"
	echo "CHANGED_FILES=$("$GIT" -C "$WORKTREE" status --porcelain | wc -l | tr -d ' ')"
	operation=$(in_progress_operation "$WORKTREE")
	echo "IN_PROGRESS=${operation:-none}"
	FETCH=no
	resolve_base
	echo "BASE=$BASE"
	echo "COMMITS_AHEAD=$("$GIT" -C "$WORKTREE" rev-list --count "$BASE..HEAD")"
	upstream=$("$GIT" -C "$WORKTREE" rev-parse --abbrev-ref --symbolic-full-name '@{upstream}' 2>/dev/null) ||
		upstream=""
	echo "UPSTREAM=${upstream:-none}"
	if [ -n "$upstream" ]; then
		echo "UNPUSHED=$("$GIT" -C "$WORKTREE" rev-list --count "$upstream..HEAD")"
	else
		echo "UNPUSHED=n/a"
	fi
}

cmd_remove() {
	record=$(record_for_path "$WORKTREE")
	[ -n "$record" ] || die $E_CONFLICT "no worktree is registered at '$WORKTREE'"
	case $(record_field "$record" 3) in
	*prunable*) die $E_CONFLICT "'$WORKTREE' is stale; pruning is a human decision ('git worktree prune')" ;;
	esac
	operation=$(in_progress_operation "$WORKTREE")
	[ -z "$operation" ] || die $E_CONFLICT "'$WORKTREE' has an unfinished git operation ($operation); not removed"
	if is_dirty "$WORKTREE"; then
		die $E_CONFLICT "'$WORKTREE' has uncommitted or untracked changes; not removed"
	fi
	branch=$(record_field "$record" 2)
	"$GIT" -C "$MAIN" worktree remove "$WORKTREE" ||
		die $E_CONFLICT "git worktree remove refused '$WORKTREE'; nothing was removed"
	echo "REMOVED=$WORKTREE"
	echo "BRANCH_KEPT=$branch"
}

[ $# -ge 2 ] || usage
COMMAND=$1
TASK=$2
shift 2
BRANCH=""
FETCH=yes
while [ $# -gt 0 ]; do
	case $1 in
	--branch)
		[ $# -ge 2 ] || usage
		BRANCH=$2
		shift 2
		;;
	--no-fetch)
		FETCH=no
		shift
		;;
	*) usage ;;
	esac
done
check_task_id "$TASK"
BRANCH=${BRANCH:-feature/$TASK}

check_git
resolve_main
case $COMMAND in
path) cmd_path ;;
prepare) cmd_prepare ;;
status) cmd_status ;;
remove) cmd_remove ;;
*) usage ;;
esac
