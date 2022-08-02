CFLAGS ?= -O2 -Wall -Wextra -pipe
ALLCFLAGS ?= $(CFLAGS) -I./hedley
CXXFLAGS ?= $(ALLCFLAGS) -fno-exceptions

SRCS := $(wildcard *.c)
DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

COMPILE.c = $(CC) $(DEPFLAGS) $(ALLCFLAGS) $(TARGET_ARCH) -c
COMPILE.cpp = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(TARGET_ARCH) -c

$(DEPDIR): ; @mkdir -p $@

parser.o : parser.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<

%.o : %.c
%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

llvm.o : llvm.cpp
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)
$(DEPFILES):

LDFLAGS=`llvm-config --cflags --ldflags --libs core executionengine mcjit interpreter analysis native bitwriter --system-libs`

COMMON_OBJS := \
	dir_exists.o \
	mkdir_p.o \
	consts.o \
	bitset.o \
	parser.o \
	parse_tree.o \
	diagnostic.o \
	rm_r.o \
	scope.o \
	type.o \
	typecheck.o \
	tokenizer.o \
	util.o \
	vec.o

REPL_OBJS := \
	$(COMMON_OBJS) \
	llvm.o \
	repl.o

repl : $(DEPDIR) repl.c $(REPL_OBJS)
	$(CXX) $(CXXFLAGS) -DDEBUG -ledit $(LDFLAGS) -o repl $(REPL_OBJS)

TEST_OBJS := \
	$(COMMON_OBJS) \
	run_tests.o \
	test.o \
	test_bitset.o \
	test_parser.o \
	test_scanner.o \
	test_utils.o \
	test_vec.o \
	test_typecheck.o

test: $(DEPDIR) run_tests.c $(TEST_OBJS)
	$(CC) $(CFLAGS) -DDEBUG $(LDFLAGS) -o test $(TEST_OBJS)

check: test
	./test

# requires --coverage CFLAG
reports/coverage.info: test
	mkdir -p reports
	./test >/dev/null
	rm *test*.gc*
	lcov --capture --directory . --output-file reports/coverage.info

clean:
	rm -rf *.so *.o test parser.c tokenizer.c *.gcda *.gcno *.gcov reports repl

reports/coverage/index.html: reports/coverage.info
	mkdir -p reports/coverage
	genhtml reports/coverage.info --output-directory reports/coverage

parser.c parser.h &: parser.y
	lemon -c parser.y
	sed -i 's/^static \(const char \*.*yyTokenName\[\].*\)$$/\1/g' parser.c
	sed -i 's/assert(/debug_assert(/g' parser.c
	./scripts/add-pragmas.sh

tokenizer.o : tokenizer.c parser.h
	$(COMPILE.c) $(OUTPUT_OPTION) $<

ifeq ($(GITHUB_ACTIONS),)
RE2C_OPTIONS := --case-ranges
endif

tokenizer.c: tokenizer.re
	re2c $(RE2C_OPTIONS) -o tokenizer.c tokenizer.re

include $(wildcard $(DEPFILES))
