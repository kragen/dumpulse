dumpulse.o: dumpulse.c
	$(CC) -Os -Wall -std=c89 -c $< -o $@

