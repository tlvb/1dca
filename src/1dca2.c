#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
void output_pgm(FILE *fh, const uint8_t *data, unsigned int w, unsigned int h) { /*{{{*/
	fprintf(fh, "P5\n#1dca\n%u %u\n255\n", w, h);
	fwrite(data, sizeof(uint8_t), w*h, fh);
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
int main(void) { /*{{{*/
	srand(time(NULL)+getpid());
	unsigned int rule = 184;
	unsigned int w = 1920;
	unsigned int h = 1080;
	uint8_t *world = calloc(sizeof(uint8_t), w*h);
	uint8_t *xpsme = calloc(sizeof(uint8_t), w*h);
	build_world(world, w, h, rule, 1000);
	for (int i=0; i<4; ++i) {
		build_world(xpsme, w, h, rule, 900-200*i);
		combine_subtract_worlds(world, xpsme, w*h, 191);
		build_world(xpsme, w, h, rule, 900-200*i-100);
		combine_add_worlds(world, xpsme, w*h, 191);
	}
	build_world(xpsme, w, h, rule, 0);
	combine_subtract_worlds(world, xpsme, w*h, 255);
	output_pgm(stdout, world, w, h);
	free(xpsme);
	free(world);
	return 0;
} /*}}}*/
