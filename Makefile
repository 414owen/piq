CFLAGS ?= -O2 -Wall -Wextra -Wno-unused-result

SRCS := $(wildcard *.c)
DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

$(DEPDIR): ; @mkdir -p $@

%.o : %.c
%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

parser.o : parser.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<

DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)
$(DEPFILES):

TEST_OBJS := \
    bitset.o \
	parser.o \
	parse_tree.o \
	test.o \
    test_bitset.c \
	test_parser.o \
	test_scanner.o \
	test_utils.o \
	test_vec.o \
	typecheck.o \
	test_typecheck.o \
	tokenizer.o \
	util.o \
	vec.o \

test: $(DEPDIR) run_tests.c $(TEST_OBJS)
	$(CC) $(CFLAGS) -o test run_tests.c $(TEST_OBJS)

check: test
	./test

# requires --coverage CFLAG
reports/coverage.info: test
	mkdir -p reports
	./test >/dev/null
	rm *test*.gc*
	lcov --capture --directory . --output-file reports/coverage.info

clean:
	rm -rf *.so *.o test parser.c tokenizer.c *.gcda *.gcno *.gcov reports

reports/coverage/index.html: reports/coverage.info
	mkdir -p reports/coverage
	genhtml reports/coverage.info --output-directory reports/coverage

parser.c parser.h &: parser.y
	lemon -c parser.y
	sed -i 's/^static \(const char \*.*yyTokenName\[\].*\)$$/\1/g' parser.c
	./scripts/add-pragmas.sh

tokenizer.o : tokenizer.c parser.h
	$(COMPILE.c) $(OUTPUT_OPTION) $<

tokenizer.c: tokenizer.re
	re2c -o tokenizer.c tokenizer.re

include $(wildcard $(DEPFILES))
