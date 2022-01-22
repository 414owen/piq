CFLAGS ?= -O1

util.so: util.h
	cc $(CFLAGS) -fpic -shared -o util.so util.c

tokenizer.so: token.h consts.h source.h term.h
	cc $(CFLAGS) -fpic -shared -o tokenizer.so tokenizer.c

test_scanner.so: tokenizer.so test.h
	cc $(CFLAGS) -fpic -shared -o test_scanner.so test_scanner.c

test.so: util.so term.h test.h test.h
	cc $(CFLAGS) -fpic -shared -o test.so test.c

test: $(common) test.so test_scanner.so tokenizer.so util.so test.c vec.h term.h test.h
	cc $(CFLAGS) -L. -o test run_tests.c -l:test.so -l:test_scanner.so -l:util.so -l:tokenizer.so -Wl,-rpath=$(PWD)

clean:
	rm -f *.so test
