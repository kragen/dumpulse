AVRGCC=avr-gcc
CFLAGS=-g -Os -Wall -std=c89

all: dumpulse.o udpserver udpclient dumpulse-attiny88.o dumpulse-atmega328.o dumpulse-i386.o
clean:
	rm *.o udpserver udpclient

dumpulse.o: dumpulse.c dumpulse.h

udpserver: udpserver.o dumpulse.o

dumpulse-attiny88.o: dumpulse.c dumpulse.h
	$(AVRGCC) -mmcu=attiny88 $(CFLAGS) -c $< -o $@

dumpulse-atmega328.o: dumpulse.c dumpulse.h
	$(AVRGCC) -mmcu=atmega328 $(CFLAGS) -c $< -o $@

dumpulse-i386.o: dumpulse.c dumpulse.h
	$(CC) -m32 $(CFLAGS) -c $< -o $@
