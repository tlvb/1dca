#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
void output_ppm(FILE *fh, const uint8_t *master, const uint8_t *red, const uint8_t *green, const uint8_t *blue, unsigned int w, unsigned int h) { /*{{{*/
	fprintf(fh, "P6\n#1dca\n%u %u\n255\n", w, h);
	for (int i=0; i<w*h; ++i) {
		fputc(((uint16_t)red[i]*(16+(uint16_t)master[i]))/271, fh);
		fputc(((uint16_t)blue[i]*(16+(uint16_t)master[i]))/271, fh);
		fputc(((uint16_t)blue[i]*(16+(uint16_t)master[i]))/271, fh);
	}
} /*}}}*/
#define apply_rule(_rule_, _ila_, _ica_, _ira_) \
	(((_rule_) & (1u<<(((*(_ila_)!=0)?4:0) | ((*(_ica_)!=0)?2:0) | ((*(_ira_)!=0)?1:0)))) != 0 ? 255 : 0)
void apply_rule_row(unsigned int rule, uint8_t *orow, const uint8_t *irow, unsigned int n) { /*{{{*/
	orow[0] = apply_rule(rule, irow+n-1, irow, irow+1);
	for (unsigned int i=1; i<n-1; ++i) {
		orow[i] = apply_rule(rule, irow+i-1, irow+i, irow+i+1);
	}
	orow[n-1] = apply_rule(rule, irow+n-2, irow+n-1, irow);
} /*}}}*/
void seed_world(uint8_t *w, unsigned int n) { /*{{{*/
	for (unsigned int i=0; i<n; ++i) {
		w[i] = (rand()%2)==0?255:0;
	}
} /*}}}*/
void build_world(uint8_t *world, unsigned int w, unsigned int h, unsigned int rule, unsigned int rate) { /*{{{*/
	unsigned int m = h/2;
	seed_world(world+m*w, w);
	for (unsigned int y=m; y>0; --y) {
		apply_rule_row(rule, world+(y-1)*w, world+y*w, w);
		for (int c=rate; c>rand()%100; c-=100) {
			seed_world(world+(y-1)*w+((rand()%w)&~7), 8);
		}
	}
	for (unsigned int y=m+1; y<h; ++y) {
		apply_rule_row(rule, world+y*w, world+(y-1)*w, w);
		for (int c=rate; c>rand()%100; c-=100) {
			seed_world(world+y*w+((rand()%w)&~7), 8);
		}
	}
} /*}}}*/
void combine_add_worlds(uint8_t *dst, uint8_t *src, unsigned int n, uint8_t c) { /*{{{*/
	for (unsigned int i=0; i<n; ++i) {
		if (src[i] != 0) {
			if (255-dst[i] < c) {
				dst[i] += c;
			}
			else {
				dst[i] = 255;
			}
		}
	}
} /*}}}*/
void combine_subtract_worlds(uint8_t *dst, uint8_t *src, unsigned int n, uint8_t c) { /*{{{*/
	for (unsigned int i=0; i<n; ++i) {
		if (src[i] != 0) {
			if (dst[i] > c) {
				dst[i] -= c;
			}
			else {
				dst[i] = 0;
			}
		}
	}
} /*}}}*/
void strangeland(uint8_t *land, unsigned int w, unsigned int h, unsigned int rule, uint8_t strength, unsigned int baserate) { /*{{{*/
	build_world(land, w, h, rule, baserate);
	unsigned int ratediff = baserate/10;
	uint8_t *layer = malloc(sizeof(uint8_t)*w*h);
	for (int i=0; i<4; ++i) {
		build_world(layer, w, h, rule, baserate-ratediff*2*(i+1));
		combine_subtract_worlds(land, layer, w*h, strength);
		build_world(layer, w, h, rule, baserate-ratediff*2*(i+2));
		combine_add_worlds(land, layer, w*h, strength);
	}
	build_world(layer, w, h, rule, 0);
	combine_subtract_worlds(land, layer, w*h, 255);
	free(layer);
} /*}}}*/
int main(int argc, const char **argv) { /*{{{*/
	if (argc != 4) {
		fprintf(
			stderr,
			"layers several iterations of rule 184 on top of each other, in color\n" \
			"run as %s outfile.ppm width height\n", argv[0]
		);
		return 1;
	}
	srand(time(NULL)+getpid());
	unsigned int rule = 184;
	unsigned int w = atoi(argv[2]);
	unsigned int h = atoi(argv[3]);;
	unsigned int baserate = 250;
	uint8_t *world[4];
	for (unsigned int i=0; i<4; ++i) {
		world[i] = malloc(sizeof(uint8_t)*w*h);
		strangeland(world[i], w, h, rule, 191, baserate);
	}
	FILE *f = fopen(argv[1], "wb");
	if (f != NULL) {
		output_ppm(f, world[0], world[1], world[2], world[3], w, h);
		fclose(f);
	}
	else {
		fprintf(stderr, "file opening error\n");
	}
	for (unsigned int i=0; i<4; ++i) {
		free(world[i]);
	}
	return 0;
} /*}}}*/
