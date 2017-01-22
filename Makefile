CC=gcc
CFLAGS+=-g -std=c99 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
INCLUDE+=-Isrc
HEADERS=$(wildcard src/*.h)
SOURCES=$(wildcard src/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))
BINS=$(wildcard bin/*.c)
TESTSC=$(wildcard test/*.c)
ALL=$(SOURCES) $(BINS) $(TESTSC)
DEPS=$(patsubst %.c,%.o,$(ALL))
TESTS=$(patsubst test/%.c,%,$(TESTSC))


all: jscn

%.o : %.c $(HEADERS)
	$(CC) $(INCLUDE) $(CFLAGS) -c $< -o $@

bin/% : test/%.o $(DEPS)
	$(CC) $(INCLUDE) $(CFLAGS) $(patsubst bin/%,test/%.o,$@) -o $@ $(OBJECTS)

jscn: $(DEPS)
	$(CC) $(CFLAGS) -o bin/jscn bin/jscn.o $(OBJECTS)

test: jscn
	./bin/jscn test/test1.json test/test1.jscn

clean:
	rm -f bin/jscn
	rm -f $(DEPS)


.PHONY: all jscn clean test
