
all: cjs

cjs: 
	gcc -Wall -g -Isrc -o bin/cjs bin/cjs.c  src/cb0r.c src/js0n.c src/base64.c

test: 
	gcc -Wall -g -Isrc -o bin/test bin/cjs.c  src/cb0r.c src/js0n.c src/base64.c
	./bin/test test/test1.json test/test1.cjs

clean:
	rm bin/cjs bin/test

.PHONY: all cjs clean test
