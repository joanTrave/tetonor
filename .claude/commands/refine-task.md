---
description: Transform a technical task (TETO-NNN GitHub issue key, Jira key, raw text, or URL) into the canonical mission spec at .claude/tasks/<task-id>/specs/<task-id>.md, via the refiner agent.
---

Dispatch the `refiner` agent (`.claude/agents/delivery/refiner.md`) with the task supplied below as
its input. The refiner agent determines the task identifier, resolves any technical context it
needs from the repository, and writes `.claude/tasks/<task-id>/specs/<task-id>.md` following
`.claude/skills/mission-spec/SKILL.md` exactly.

Address the human in the task's conversation language and keep every generated file in English,
per `CLAUDE.md` § "Conversation language". Before the refiner runs, that is the language of the
supplied task text (or of the human's message); once the spec is written, it is the spec's
`CONVERSATION_LANGUAGE` bullet.

If no task is supplied below, ask the human for it before dispatching — a `TETO-NNN` key (read from
this repository's GitHub issues), a GitHub issue number or URL, a Jira key, pasted task text, or a
URL are all acceptable.

Once the mission spec is written, report its path and the files the refiner read (and why)
to the human, and stop. Do not proceed to the
Architect automatically — the next step in the pipeline (see `CLAUDE.md`) starts only when the
human decides to continue.

$ARGUMENTS
