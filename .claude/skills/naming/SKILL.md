# Naming and Project Structure Skill

## Purpose

Keep files, types, functions, macros and tests consistent across Tetonor's C code base.

## Files and directories

| Type | Convention | Example |
|---|---|---|
| Directories | `snake_case`, one per layer | `src/game/`, `src/ui/` |
| Source / header pairs | `snake_case`, same stem | `number_generation.c` / `number_generation.h` |
| Port headers | `<area>_port.h` | `game_port.h` |
| Test files | `test_` + stem of the unit under test | `tests/game/test_number_generation.c` |
| Entry point | `src/main.c` (only one) | `main.c` |

## Identifiers

| Kind | Convention | Example |
|---|---|---|
| Struct / typedef types | `PascalCase` | `NumberGeneration`, `GamePort` |
| Enum types | `PascalCase` | `MenuResult` |
| Enum constants | `UPPER_SNAKE_CASE`, prefixed with the type's concept | `MENU_START`, `MENU_EXIT` |
| Public functions | `snake_case`, verb first; UI entry points prefixed `ui_` | `generate_random_numbers`, `ui_show_start_menu` |
| Private (file-local) functions | `static`, `snake_case`, **no leading underscore** | `static void swap_ints(int* a, int* b)` |
| Function-pointer fields in a port | function name + `_f` | `generate_random_numbers_f` |
| Variables / fields / parameters | `snake_case`, descriptive | `available_array`, `game_port` |
| Macros and constants | `UPPER_SNAKE_CASE`, project-wide ones prefixed `TETONOR_` | `TETONOR_GRID_SIZE`, `CELL_W` |
| Include guards | `TETONOR_<NAME>_H` | `TETONOR_GAME_PORT_H` |
| Test functions | `test_<unit>_<scenario>` | `test_generate_random_numbers_masks_expected_count` |

Leading underscores are reserved by the C standard at file scope; the existing `_helper` names are
legacy — rename to `static` + plain `snake_case` when you touch that file, never add new ones.

## Lifecycle pairs

Allocation and release functions are named as pairs and live in the same module:

| Acquire | Release |
|---|---|
| `create_<thing>` | `destroy_<thing>` |
| `generate_<thing>` / `alloc_<thing>` | `free_<thing>` |
| `init_<thing>` | `end_<thing>` / `deinit_<thing>` |

## Rules

Naming is part of the architecture. Do not introduce a competing convention within the project. When
existing code has an established convention, follow it — except for the reserved-identifier rule
above, which is a correctness fix, not a style choice.
