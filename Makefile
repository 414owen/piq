headers := vec.h util.h term.h
common := util.c tokenizer.c

CFLAGS ?= -O1

test: $(common) $(headers) test.c
	cc $(CFLAGS) -o test test.c $(common)
