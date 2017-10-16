AVRGCC=avr-gcc
CFLAGS=-g -Os -Wall -std=c89
PYTEST=py.test-3

all-native: dumpulse.o udpserver dumpulse.so
clean:
	-rm *.o *.so udpserver
all: all-native dumpulse-i386.o dumpulse-attiny88.o dumpulse-atmega328.o

test: test-prereqs
	$(PYTEST) test.py
test-prereqs: dumpulse.so


dumpulse.o: dumpulse.c dumpulse.h
	$(CC) -fPIC $(CFLAGS) -o $@ -c $<

dumpulse_so.o: dumpulse_so.c dumpulse.h
	$(CC) -fPIC $(CFLAGS) -o $@ -c $<

dumpulse.so: dumpulse_so.o dumpulse.o
	$(CC) -shared $^ -o $@

udpserver: udpserver.o dumpulse.o

dumpulse-attiny88.o: dumpulse.c dumpulse.h
	$(AVRGCC) -mmcu=attiny88 $(CFLAGS) -c $< -o $@

dumpulse-atmega328.o: dumpulse.c dumpulse.h
	$(AVRGCC) -mmcu=atmega328 $(CFLAGS) -c $< -o $@

dumpulse-i386.o: dumpulse.c dumpulse.h
	$(CC) -m32 $(CFLAGS) -c $< -o $@
