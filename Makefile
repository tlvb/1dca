CC=gcc
CFLAGS=-std=gnu11 -Wall -lm
vpath %.c src
1dca4: 1dca4.c
	$(CC) $(CFLAGS) -o $@ $^
1dca3: 1dca3.c
	$(CC) $(CFLAGS) -o $@ $^
1dca2: 1dca2.c
	$(CC) $(CFLAGS) -o $@ $^
1dca: 1dca.c
	$(CC) $(CFLAGS) -o $@ $^
clean:
	rm 1dca 1dca2 1dca3 1dca4 || true
