CC=g++
CFLAGS=-O3 -c -Wall -g
LDFLAGS=-g
SOURCES=bitmap_hashmap.cc shadow_hashmap.cc probing_hashmap.cc tombstone_hashmap.cc backshift_hashmap.cc testcase.cc monitoring.cc murmurhash3.cc hamming.cc
SOURCES_MAIN=main.cc
OBJECTS=$(SOURCES:.cc=.o)
OBJECTS_MAIN=$(SOURCES_MAIN:.cc=.o)
EXECUTABLE=hashmap

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(OBJECTS_MAIN)
	$(CC) $(LDFLAGS) $(OBJECTS) $(OBJECTS_MAIN) -o $@

.cc.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *~ *.o $(EXECUTABLE)

