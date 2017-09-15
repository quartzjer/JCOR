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

all: jscn jwt2json

%.o : %.c $(HEADERS)
	$(CC) $(INCLUDE) $(CFLAGS) -c $< -o $@

bin/% : test/%.o $(DEPS)
	$(CC) $(INCLUDE) $(CFLAGS) $(patsubst bin/%,test/%.o,$@) -o $@ $(OBJECTS)

jscn: $(DEPS)
	$(CC) $(CFLAGS) -o bin/jscn bin/jscn.o $(OBJECTS)

jwt2json: $(DEPS)
	$(CC) $(CFLAGS) -o bin/jwt2json bin/jwt2json.o $(OBJECTS)

test: jscn
	./bin/jscn test/test1.json test/test1.jscn
	hexdump test/test1.jscn  | cut -c 8-
	./bin/jscn test/test1.json test/test1d.jscn test/refs1.jscn true
	hexdump test/test1d.jscn  | cut -c 8-
	./bin/jscn test/test1d.jscn test/test1d.json test/refs1.jscn
	diff test/test1.json test/test1d.json

spec: draft-miller-json-constrained-notation-00.html

%.xml: %.md
	$(MMARK) --xml2 --page $< > $@
%.html %.txt: %.xml
	$(XML2RFC) --html --text $<

clean:
	rm -f bin/jscn
	rm -f bin/jwt2json

req: 
	@echo brew install go
	@echo export GOPATH=~/go
	@echo go get github.com/miekg/mmark/mmark
	@echo pip3 install xml2rfc

.PHONY: all jscn test req spec
