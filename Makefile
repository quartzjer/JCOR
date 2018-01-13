CC=gcc
CFLAGS+=-g -std=c99 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
INCLUDE+=-Iinclude
HEADERS=$(wildcard include/*.h)
SOURCES=$(wildcard src/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))
BINS=$(wildcard bin/*.c)
TESTSC=$(wildcard test/*.c)
ALL=$(SOURCES) $(BINS) $(TESTSC)
DEPS=$(patsubst %.c,%.o,$(ALL))
TESTS=$(patsubst test/%.c,%,$(TESTSC))

GOPATH?=/usr/local/gopath
MMARK?=$(GOPATH)/bin/mmark
XML2RFC?=xml2rfc

all: jcor jwt2json

%.o : %.c $(HEADERS)
	$(CC) $(INCLUDE) $(CFLAGS) -c $< -o $@

bin/% : test/%.o $(DEPS)
	$(CC) $(INCLUDE) $(CFLAGS) $(patsubst bin/%,test/%.o,$@) -o $@ $(OBJECTS)

jcor: $(DEPS)
	$(CC) $(CFLAGS) -o bin/jcor bin/jcor.o $(OBJECTS)

jwt2json: $(DEPS)
	$(CC) $(CFLAGS) -o bin/jwt2json bin/jwt2json.o $(OBJECTS)

test: jcor
	./bin/jcor test/test1.json test/test1.jcor
	hexdump test/test1.jcor  | cut -c 8-
	./bin/jcor test/test1.json test/test1d.jcor test/refs1.jcor true
	hexdump test/test1d.jcor  | cut -c 8-
	./bin/jcor test/test1d.jcor test/test1d.json test/refs1.jcor
	diff test/test1.json test/test1d.json

spec: draft-miller-json-constrained-representation-00.html

%.xml: %.md
	$(MMARK) --xml2 --page $< > $@
%.html %.txt: %.xml
	$(XML2RFC) --html --text $<

clean:
	rm -f bin/jcor
	rm -f bin/jwt2json

req: 
	@echo brew install go
	@echo export GOPATH=~/go
	@echo go get github.com/miekg/mmark/mmark
	@echo pip3 install xml2rfc

.PHONY: all jcor clean test req spec
