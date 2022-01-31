CFLAGS ?= -O1 -Wall -Wno-unused-result

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

TEST_OBJS := parser.o tokenizer.o test.o test_scanner.o test_parser.o parse_tree.o test_utils.o util.o vec.o

test: $(DEPDIR) run_tests.c $(TEST_OBJS)
	$(CC) $(CFLAGS) -o test run_tests.c $(TEST_OBJS)

check: test
	./test

clean:
	rm -f *.so *.o test parser.c tokenizer.c

parser.c parser.h &: parser.y
	lemon -c parser.y
	sed -i 's/^static \(const char \*.*yyTokenName\[\].*\)$$/\1/g' parser.c
	./scripts/add-pragmas.sh

tokenizer.o : tokenizer.c parser.h
	$(COMPILE.c) $(OUTPUT_OPTION) $<

tokenizer.c: tokenizer.re
	re2c -o tokenizer.c tokenizer.re

include $(wildcard $(DEPFILES))
