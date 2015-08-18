#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
void build_rulemap(uint8_t *map, unsigned int rule) { /*{{{*/
	/*
	 * 111 110 101 100 011 010 001 000
	 * 128  64  32  16   8   4   2   1
	 *
	 */
	for (unsigned int i=0; i<256; ++i) {
		map[i] = 0;
		for (unsigned int j=1; j<7; ++j) {
			/*
			 * ... [j+1] [ j ] [j-1] ...
			 *          \  |  /
			 *             V
			 *           [ j ]
			 */
			const uint8_t key = (i >> (j-1)) & 7;
			if ((rule & (1<<key)) != 0) {
				map[i] |= 1<<(j);
			}
		}
	}
} /*}}}*/
void apply_rulemap_0(uint8_t *new, const uint8_t *old, unsigned int n, const uint8_t *map) { /*{{{*/
	/*
	 * for the byte non-edges (bit 1-6)
	 */
	for (unsigned int i=0; i<n; ++i) {
		new[i] = map[old[i]];
	}
} /*}}}*/
void apply_rulemap_1(uint8_t *new, const uint8_t *old, unsigned int n, const uint8_t *map) { /*{{{*/
	/*
	 * for the byte edges (bit 0 and 7) ...since they depend on the neighbour byte's bits
	 * left     right
	 * ------LL RR------
	 * ----LLRR
	 * -----MS-
	 * -------M S-------
	 *
	 */
	int i=n-1;
	for (int j=0; j<n; ++j) {
		uint8_t i_left = old[i];
		uint8_t i_right = old[j];
		uint8_t i_combined = ((i_left<<2)&0xc) | ((i_right>>6)&0x3);
		uint8_t o_result = map[i_combined];
		uint8_t o_left = (o_result>>2) & 1;
		uint8_t o_right = (o_result<<6) & 128;
		new[i] |= o_left;
		new[j] |= o_right;
		i=j;
	}
} /*}}}*/
void apply_rulemap(uint8_t *new, const uint8_t *old, unsigned int n, const uint8_t *map) { /*{{{*/
	apply_rulemap_0(new, old, n, map);
	apply_rulemap_1(new, old, n, map);
} /*}}}*/
void permutate(uint8_t *world, unsigned int w) {
	int i = rand()%w;
	world[i] = rand();
}
void output_pbm(FILE *fh, const uint8_t *data, unsigned int w, unsigned int h) { /*{{{*/
	fprintf(fh, "P4\n#1dca\n%u %u\n", w*8, h);
	fwrite(data, sizeof(uint8_t), w*h, fh);
} /*}}}*/
int main(int argc, const char *const*argv) { /*{{{*/
	if (argc < 4) {
		fprintf(
			stderr,
			"lookup table based 1d cellular automaton with pbm output\n"
			"%s rule width height [chance of permutation] > somefile\n"
			"the width will be truncated to a multiple of eight\n"
			"example: %s 184 1024 768\n",
			argv[0], argv[0]
		);
		return 1;
	}
	srand(time(NULL)+getpid());
	const unsigned int r = atoi(argv[1]);
	const unsigned int w = atoi(argv[2])>>3;
	const unsigned int h = atoi(argv[3]);
	unsigned int rate = argc==5?atoi(argv[4]):0;
	const unsigned int m = h>>1;
	uint8_t map[256];
	build_rulemap(map, r);
	uint8_t *world = calloc(sizeof(uint8_t), w*h); // yes we could allocate just two rows instead
	if (world == NULL) { return -1; }
	for (unsigned int i=0; i<w; ++i) {
		world[i+w*m] = rand(); // initialization
	}
	for (unsigned int y=m; y>0; --y) {
		apply_rulemap(&world[(y-1)*w], &world[y*w], w, map);
		for (int chance=rate; chance>rand()%100; chance-=100) {
			permutate(&world[(y-1)*w], w);
		}
	}
	for (unsigned int y=(m+1); y<h; ++y) {
		apply_rulemap(&world[y*w], &world[(y-1)*w], w, map);
		for (int chance=rate; chance>rand()%100; chance-=100) {
			permutate(&world[y*w], w);
		}
	}
	output_pbm(stdout, world, w, h);
	free(world);
	return 0;
} /*}}}*/
