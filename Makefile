CC=gcc
CFLAGS=-std=gnu11 -Wall -lm -g
vpath %.c src
texturetest2: texturetest2.c graphics_base.o
	$(CC) $(CFLAGS) -o $@ $^ -lSDL2 -lGL
texturetest: texturetest.c graphics_base.o
	$(CC) $(CFLAGS) -o $@ $^ -lSDL2 -lGL
1dca5: 1dca5.c
	$(CC) $(CFLAGS) -o $@ $^
1dca4: 1dca4.c
	$(CC) $(CFLAGS) -o $@ $^
1dca3: 1dca3.c
	$(CC) $(CFLAGS) -o $@ $^
1dca2: 1dca2.c
	$(CC) $(CFLAGS) -o $@ $^
1dca: 1dca.c
	$(CC) $(CFLAGS) -o $@ $^
clean:
	rm *.o 1dca 1dca2 1dca3 1dca4 texturetest texturetest2 || true

%o: %.c
	$(CC) $(CFLAGS) -c $^
