#!/bin/sh
# Structural tests for the conversation-language rule of the agent workflow: the rule lives in one
# authoritative section of CLAUDE.md, the files that talk to the human reference it, and the
# mission-spec template keeps its canonical shape. Read-only: nothing is modified, and the tree
# checked is the one this script lives in (the task worktree during /implement).
#
# Run: sh .claude/scripts/tests/test_language_rule.sh
set -u

ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
CLAUDE_MD="$ROOT/CLAUDE.md"
SKILL="$ROOT/.claude/skills/mission-spec/SKILL.md"
REFINE_TASK="$ROOT/.claude/commands/refine-task.md"
ARCHITECT_PLAN="$ROOT/.claude/commands/architect-plan.md"
IMPLEMENT="$ROOT/.claude/commands/implement.md"
REFINER="$ROOT/.claude/agents/delivery/refiner.md"
SECTION_HEADING='## Conversation language'
REFERENCE='`CLAUDE.md` § "Conversation language"'

FAILURES=0
CASES=0
CURRENT=""

begin_case() {
	CASES=$((CASES + 1))
	CURRENT=$1
}

fail() {
	printf '  FAIL [%s]: %s\n' "$CURRENT" "$*"
	FAILURES=$((FAILURES + 1))
}

assert_eq() {
	[ "$1" = "$2" ] || fail "$3: expected '$1', got '$2'"
}

# The lines of CLAUDE.md from the section heading up to (excluding) the next line starting with '#'.
section() {
	awk -v heading="$SECTION_HEADING" '
		$0 == heading { inside = 1; print; next }
		inside && /^#/ { exit }
		inside { print }
	' "$CLAUDE_MD" 2>/dev/null
}

# A file's text with every run of whitespace (including line breaks) collapsed to one space, so a
# phrase wrapped across lines still matches.
flatten() {
	tr '\n' ' ' <"$1" 2>/dev/null | tr -s '[:space:]' ' '
}

# Fails unless the text in $1 contains the literal $2.
assert_contains() {
	printf '%s\n' "$1" | grep -qF -- "$2" || fail "missing literal: $2"
}

# Fails unless every literal given after the text in $1 is contained in it.
assert_contains_all() {
	text=$1
	shift
	for literal in "$@"; do
		assert_contains "$text" "$literal"
	done
}

# Fails unless file $1 exists and contains the literal $2.
assert_file_contains() {
	if [ ! -f "$1" ]; then
		fail "file not found: $1"
		return
	fi
	grep -qF -- "$2" "$1" || fail "$1 does not contain: $2"
}

# Number of the first line of file $1 that matches the extended regex $2; empty when none does.
first_line_matching() {
	grep -n -E -- "$2" "$1" 2>/dev/null | head -n 1 | cut -d: -f1
}

# Paths of the instruction files that may hold the rule: CLAUDE.md and every *.md under .claude/
# except the per-task artifacts.
instruction_files() {
	printf '%s\n' "$CLAUDE_MD"
	find "$ROOT/.claude" -path "$ROOT/.claude/tasks" -prune -o -type f -name '*.md' -print
}

# How many instruction files contain the literal $1; sets COUNT, and ONLY to the sole file's path.
count_files_containing() {
	ONLY=$(instruction_files | while IFS= read -r file; do
		if [ -f "$file" ] && grep -qF -- "$1" "$file"; then
			printf '%s\n' "$file"
		fi
	done)
	COUNT=$(printf '%s\n' "$ONLY" | grep -c .)
}

test_claude_md_single_section() {
	begin_case claude_md_single_section
	assert_eq 1 "$(grep -cxF -- "$SECTION_HEADING" "$CLAUDE_MD")" "lines equal to '$SECTION_HEADING'"
	workflow=$(first_line_matching "$CLAUDE_MD" '^# Task Workflow$')
	heading=$(first_line_matching "$CLAUDE_MD" '^## Conversation language$')
	artifacts=$(first_line_matching "$CLAUDE_MD" '^## Artifact locations$')
	if [ -z "$workflow" ] || [ -z "$heading" ] || [ -z "$artifacts" ]; then
		fail "cannot order the headings (Task Workflow='$workflow', section='$heading', Artifact locations='$artifacts')"
	elif [ "$heading" -le "$workflow" ] || [ "$heading" -ge "$artifacts" ]; then
		fail "section (line $heading) must sit after '# Task Workflow' ($workflow) and before '## Artifact locations' ($artifacts)"
	fi
}

test_section_conversation_rule() {
	begin_case section_conversation_rule
	assert_contains_all "$(section)" 'conversation language' '/refine-task' 'TETO-NNN' \
		"human's own message" 'CONVERSATION_LANGUAGE'
}

test_section_fallback_english() {
	begin_case section_fallback_english
	assert_contains_all "$(section)" 'defaults to English'
}

test_section_persistence() {
	begin_case section_persistence
	assert_contains_all "$(section)" '/architect-plan' '/implement' 'approval gates'
}

test_section_explicit_override() {
	begin_case section_explicit_override
	assert_contains_all "$(section)" 'explicit request from the human'
}

test_section_scope_translated() {
	begin_case section_scope_translated
	assert_contains_all "$(section)" 'files read and why' 'exchanged between agents stay in English'
}

test_section_verbatim_items() {
	begin_case section_verbatim_items
	assert_contains_all "$(section)" 'File paths' 'marker keywords' 'outcome names'
}

test_section_english_artifacts() {
	begin_case section_english_artifacts
	assert_contains_all "$(section)" 'Mission specs' 'plans' 'architecture gate reports' 'review.json' \
		'review.md' 'code comments' 'commit messages' 'pull request titles and descriptions'
}

test_references_present() {
	begin_case references_present
	for file in "$REFINE_TASK" "$REFINER" "$ARCHITECT_PLAN" "$IMPLEMENT" "$SKILL"; do
		assert_file_contains "$file" "$REFERENCE"
	done
}

test_rule_not_duplicated() {
	begin_case rule_not_duplicated
	for literal in 'pull request titles and descriptions' 'defaults to English'; do
		count_files_containing "$literal"
		assert_eq 1 "$COUNT" "instruction files containing '$literal'"
		assert_eq "$CLAUDE_MD" "$ONLY" "the one file containing '$literal'"
	done
}

test_refiner_records_language() {
	begin_case refiner_records_language
	assert_file_contains "$REFINER" 'CONVERSATION_LANGUAGE'
	assert_file_contains "$REFINER" 'conversation language recorded'
}

test_refine_task_reports_files_read() {
	begin_case refine_task_reports_files_read
	assert_file_contains "$REFINE_TASK" 'the files the refiner read'
}

test_spec_template_variable() {
	begin_case spec_template_variable
	plan_dir=$(first_line_matching "$SKILL" '^- `PLAN_DIR: ')
	variable=$(first_line_matching "$SKILL" '^- `CONVERSATION_LANGUAGE: ')
	assumptions=$(first_line_matching "$SKILL" '^\[# Assumptions')
	if [ -z "$plan_dir" ] || [ -z "$assumptions" ]; then
		fail "template anchors not found (PLAN_DIR='$plan_dir', [# Assumptions='$assumptions')"
	elif [ -z "$variable" ]; then
		fail 'no template bullet starting with: - `CONVERSATION_LANGUAGE: '
	elif [ "$variable" -le "$plan_dir" ] || [ "$variable" -ge "$assumptions" ]; then
		fail "bullet (line $variable) must sit after PLAN_DIR ($plan_dir) and before [# Assumptions ($assumptions)"
	fi
}

test_spec_template_sections_unchanged() {
	begin_case spec_template_sections_unchanged
	expected='# <task-id>
# Problem
# Solution
# Variables
[# Assumptions
# Workflow
# Definition Of Done
# Functional Requirements
# Technical Requirements
# Verification
# How You'"'"'re Graded'
	# Top-level heading lines of the first markdown block after '## Canonical template', cut at the
	# first double space or arrow and trimmed.
	actual=$(awk '
		$0 == "## Canonical template" { seen = 1; next }
		seen && !inside && /^```markdown/ { inside = 1; next }
		inside && /^```/ { exit }
		inside && (/^# / || /^\[# /) { print }
	' "$SKILL" 2>/dev/null | sed -e 's/  .*//' -e 's/←.*//' -e 's/[[:space:]]*$//')
	assert_eq "$expected" "$actual" "ordered template section headings"
}

test_spec_spanish_issue_rule_unchanged() {
	begin_case spec_spanish_issue_rule_unchanged
	text=$(flatten "$SKILL")
	assert_contains "$text" 'Issues are written in Spanish; write the mission spec in English,'
	assert_contains "$text" 'translating acceptance criteria faithfully.'
}

test_spec_validation_unchanged() {
	begin_case spec_validation_unchanged
	assert_eq 1 "$(grep -cxF -- '## Validation' "$SKILL")" "lines equal to '## Validation'"
	assert_contains "$(flatten "$SKILL")" 'in order, and the `# <task-id>` title equals the filename stem'
}

for test_case in $(sed -n 's/^\(test_[a-z_]*\)() {$/\1/p' "$0"); do
	"$test_case"
done

if [ "$FAILURES" -eq 0 ]; then
	echo "language-rule: all $CASES cases passed"
	exit 0
fi
echo "language-rule: $FAILURES assertion(s) failed in $CASES cases"
exit 1
