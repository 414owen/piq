CFLAGS ?= -O1 -Wall -Wno-unused-result

tokenizer.c: tokenizer.re
	re2c -o tokenizer.c tokenizer.re

util.so: util.h
	cc $(CFLAGS) -fpic -shared -o util.so util.c

tokenizer.so: token.h consts.h source.h term.h tokenizer.c vec.h parser.h
	cc $(CFLAGS) -fpic -shared -o tokenizer.so tokenizer.c

test_scanner.so: tokenizer.so test.h test_scanner.c consts.h
	cc $(CFLAGS) -fpic -shared -o test_scanner.so test_scanner.c

test.so: util.so term.h test.h test.h test.c
	cc $(CFLAGS) -fpic -shared -o test.so test.c

parser.c: parser.y
	lemon parser.y
	# clang-format -i parser.c
	# clang-format -i parser.h

parser.h: parser.c

diagnostic.so: source.h

test: $(common) test.so test_scanner.so tokenizer.so util.so test.c vec.h term.h test.h run_tests.c
	cc $(CFLAGS) -L. -o test run_tests.c -l:test.so -l:test_scanner.so -l:util.so -l:tokenizer.so -Wl,-rpath=$(PWD)

clean:
	rm -f *.so test
