dumpulse.o: dumpulse.c dumpulse.h
	$(CC) -Os -Wall -std=c89 -c $< -o $@

