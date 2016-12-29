
all: cjs

cjs: 
	gcc -Wall -o cjs cjs.c  cb0r.c js0n.c

clean:
	rm cjs

.PHONY: all cjs clean 
