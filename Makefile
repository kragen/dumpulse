AVRGCC=avr-gcc

all: dumpulse.o dumpulse-attiny88.o dumpulse-atmega328.o dumpulse-i386.o

dumpulse.o: dumpulse.c dumpulse.h
	$(CC) -Os -Wall -std=c89 -c $< -o $@

dumpulse-attiny88.o: dumpulse.c dumpulse.h
	$(AVRGCC) -Os -mmcu=attiny88 -Os -Wall -std=c89 -c $< -o $@

dumpulse-atmega328.o: dumpulse.c dumpulse.h
	$(AVRGCC) -Os -mmcu=atmega328 -Os -Wall -std=c89 -c $< -o $@

dumpulse-i386.o: dumpulse.c dumpulse.h
	$(CC) -Os -m32 -Os -Wall -std=c89 -c $< -o $@
