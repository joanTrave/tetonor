# Tetonor build system. Recipes need a POSIX shell (MSYS2 make on Windows).

CC          = clang
BASE_CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic
CFLAGS      = $(BASE_CFLAGS)
DEPFLAGS    = -MMD -MP
LDFLAGS     ?=

# Windows: PDCurses (wincon backend) from MSYS2 UCRT64. Plain -lpdcurses is
# the GUI backend and does not link here without extra setup. Defined before
# the generic defaults so Linux/macOS keep ncurses.
UNAME_S := $(shell uname -s 2>/dev/null)
ifneq ($(filter Windows_NT,$(OS))$(filter MINGW% MSYS%,$(UNAME_S)),)
EXE := .exe
CURSES_CFLAGS ?= -isystem /ucrt64/include/pdcurses -DPDC_FORCE_UTF8
CURSES_LIB    ?= -lpdcurses_wincon -lwinmm
endif
CURSES_CFLAGS ?=
CURSES_LIB    ?= -lncurses

GCOV                    ?= llvm-cov gcov
COMPARE_BRANCH          ?= origin/main
DIFF_COVERAGE_THRESHOLD ?= 90
SANITIZE_FLAGS          ?= -g -fsanitize=address,undefined

SRC     := $(wildcard src/*.c src/game/*.c src/ui/*.c src/storage/*.c \
                      src/platform/*.c)
OBJ     := $(SRC:%.c=build/obj/%.o)
UI_OBJ  := $(filter build/obj/src/ui/%.o,$(OBJ))

# Units linked into the tests: the portable core, the storage and platform
# adapters, and the pure UI modules.
TESTABLE_OBJ := $(patsubst %.c,build/obj/%.o,$(wildcard src/game/*.c \
                                                         src/storage/*.c \
                                                         src/platform/*.c)) \
                build/obj/src/ui/difficulty_menu.o \
                build/obj/src/ui/game_session.o \
                build/obj/src/ui/keys.o build/obj/src/ui/layout.o \
                build/obj/src/ui/palette_style.o \
                build/obj/src/ui/pause_menu.o \
                build/obj/src/ui/read_timeout.o \
                build/obj/src/ui/stats_menu.o

TEST_SRC := $(wildcard tests/*/test_*.c)
TEST_BIN := $(TEST_SRC:tests/%.c=build/tests/%$(EXE))

FORMAT_FILES = $(shell find src tests -type f \( -name '*.c' -o -name '*.h' \))

.PHONY: all test coverage diff-coverage sanitize format-check clean

all: tetonor$(EXE)

tetonor$(EXE): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS) $(CURSES_LIB)

build/obj/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(DEPFLAGS) $(EXTRA_CFLAGS) -c -o $@ $<

# Curses flags apply to the UI adapter only, never to the core or the tests.
$(UI_OBJ): EXTRA_CFLAGS = $(CURSES_CFLAGS)

build/tests/%$(EXE): tests/%.c $(TESTABLE_OBJ)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(DEPFLAGS) -MF $@.d -o $@ $< $(TESTABLE_OBJ) $(LDFLAGS)

test: $(TEST_BIN)
	@rm -rf build/tests/tmp && mkdir -p build/tests/tmp
	@failed=0; \
	for test_bin in $(TEST_BIN); do \
		echo "== $$test_bin"; \
		./$$test_bin || failed=1; \
	done; \
	exit $$failed

coverage:
	$(MAKE) clean
	$(MAKE) test CFLAGS="$(BASE_CFLAGS) --coverage"
	gcovr --gcov-executable "$(GCOV)" -r . --filter 'src/' --print-summary \
		--txt --cobertura coverage.xml

diff-coverage: coverage
	diff-cover coverage.xml --compare-branch=$(COMPARE_BRANCH) \
		--include-untracked --fail-under=$(DIFF_COVERAGE_THRESHOLD)

sanitize:
	$(MAKE) clean
	$(MAKE) test CFLAGS="$(BASE_CFLAGS) $(SANITIZE_FLAGS)"

format-check:
	clang-format --dry-run --Werror $(FORMAT_FILES)

clean:
	rm -rf build coverage.xml tetonor tetonor.exe

-include $(OBJ:.o=.d) $(TEST_BIN:=.d)
