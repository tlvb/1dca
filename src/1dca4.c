#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#define LAYERS 11
#define WIDTH 1920
#define HEIGHT 1080
#define LAYERWIDTH (WIDTH*2)
#define FRAMES 150
#define BASERATE 1000
#define RULE 184
#define SCALE 0
typedef struct { /*{{{*/
	int w;
	int h;
	uint8_t *layer[LAYERS];
} layerset; /*}}}*/
layerset *alloc_layerset(int w,  int h) { /*{{{*/
	layerset *ls = malloc(sizeof(layerset));
	for (int i=0; i<LAYERS; ++i) {
		fprintf(stderr, " [%u]", i);
		ls->layer[i] = malloc(sizeof(uint8_t)*w*h);
	}
	ls->w = w;
	ls->h = h;
	return ls;
} /*}}}*/
void free_layerset(layerset *ls) { /*{{{*/
	for (int i=0; i<LAYERS; ++i) {
		free(ls->layer[i]);
	}
	free(ls);
} /*}}}*/
void mapcolor(float a, float b, float c, float *rgb) {
	a/=255.0;
	b/=255.0;
	c/=255.0;
	a=(0.1+0.9*a);
	rgb[0] = a*(0.8*b+0.2*c);
	rgb[1] = a*c*c*c;
	rgb[2] = a*c*(0.7*b+0.3*c);
	rgb[0] *= 255;
	rgb[1] *= 255;
	rgb[2] *= 255;
}
void output_ppm(FILE *fh, const uint8_t *c0, const uint8_t *c1, const uint8_t *c2, int w, int h) { /*{{{*/
	fprintf(fh, "P6\n#1dca\n%u %u\n255\n", w<<SCALE, h<<SCALE);
	for (int y=0; y<(h<<SCALE); ++y) {
		for (int x=0; x<(w<<SCALE); ++x) {
			int i = (x>>SCALE) + (y>>SCALE)*w;
			float rgb[3];
			mapcolor(
				((float)c0[i]),
				((float)c1[i]),
				((float)c2[i]),
				rgb
			);
			fputc(rgb[0], fh);
			fputc(rgb[1], fh);
			fputc(rgb[2], fh);
		}
	}
} /*}}}*/
void output_enumerated_png(const char *fmt, int i, uint8_t **layers, int w, int h) { /*{{{*/
	char namestr[2048];
	char cmdstr[2048];
	if (snprintf(namestr, 2048, fmt, i) == 2047) {
		fprintf(stderr, "too long path\n");
		return;
	}
	if(snprintf(cmdstr, 2048, "pnmtopng > %s", namestr) == 2047) {
		fprintf(stderr, "too long command\n");
		return;
	}
	FILE *fh = popen(cmdstr, "w");
	output_ppm(fh, layers[0], layers[1], layers[2], w, h);
	pclose(fh);
} /*}}}*/
#define apply_rule(_rule_, _ila_, _ica_, _ira_) \
	(((_rule_) & (1u<<(((*(_ila_)!=0)?4:0) | ((*(_ica_)!=0)?2:0) | ((*(_ira_)!=0)?1:0)))) != 0 ? 255 : 0)
int mod(int a, int b) { /*{{{*/
	int r = a%b;
	return r<0 ? r+b : r;
} /*}}}*/
void apply_rule_row(int rule, uint8_t *orow, const uint8_t *irow, int n) { /*{{{*/
	orow[0] = apply_rule(rule, irow+n-1, irow, irow+1);
	for (int i=1; i<n-1; ++i) {
		orow[i] = apply_rule(rule, irow+i-1, irow+i, irow+i+1);
	}
	orow[n-1] = apply_rule(rule, irow+n-2, irow+n-1, irow);
} /*}}}*/
void seed_world(uint8_t *w, int n) { /*{{{*/
	for (int i=0; i<n; ++i) {
		w[i] = (rand()%2)==0?255:0;
	}
} /*}}}*/
void build_world(uint8_t *world, int w, int h, int rule, int rate, int how) { /*{{{*/
	if (how == 0) {
		// from middle
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
	}
	else if (how == 1) {
		// from top
		seed_world(world, w);
		for (int y=1; y<h; ++y) {
			apply_rule_row(rule, world+y*w, world+(y-1)*w, w);
			for (int c=rate; c>rand()%100; c-=100) {
				seed_world(world+y*w+((rand()%w)&~7), 8);
			}
		}
	}
	else if (how == 2) {
		seed_world(world+(h-1)*w, w);
		for (int y=h-1; y>0; --y) {
			apply_rule_row(rule, world+(y-1)*w, world+y*w, w);
			for (int c=rate; c>rand()%100; c-=100) {
				seed_world(world+(y-1)*w+((rand()%w)&~7), 8);
			}
		}
	}
} /*}}}*/
void build_layerset(layerset *ls, int rule, int baserate) { /*{{{*/
	fprintf(stderr, "building layerset\n");
	int ratedelta = baserate/(LAYERS-1);
	for (int i=0; i<LAYERS; ++i) {
		build_world(ls->layer[i], ls->w, ls->h, rule, baserate-ratedelta*i, i==LAYERS-1?0:i%3);
	}
} /*}}}*/
void blit_subsect(uint8_t *dst, uint8_t *src, int dw, int h, int sw, int deltax) { /*{{{*/
	for (int y=0; y<h; ++y) {
		for (int dx=0; dx<dw; ++dx) {
			int sx = mod((dx+deltax),sw);
			dst[dx+dw*y] = src[sx+sw*y];
		}
	}
} /*}}}*/
void combine_add_subsect(uint8_t *dst, uint8_t *src, int dw, int h, int sw, int deltax, uint8_t c) { /*{{{*/
	for (int y=0; y<h; ++y) {
		for (int dx=0; dx<dw; ++dx) {
			int sx = mod((dx+deltax),sw);
			if (src[sx+sw*y] != 0) {
				if (255-dst[dx+dw*y] < c) {
					dst[dx+dw*y] += c;
				}
				else {
					dst[dx+dw*y] = 255;
				}
			}
		}
	}
} /*}}}*/
void combine_subtract_subsect(uint8_t *dst, uint8_t *src, int dw, int h, int sw, int deltax, uint8_t c) { /*{{{*/
	for (int y=0; y<h; ++y) {
		for (int dx=0; dx<dw; ++dx) {
			int sx = mod((dx+deltax),sw);
			if (src[sx+sw*y] != 0) {
				if (dst[dx+dw*y] > c) {
					dst[dx+dw*y] -= c;
				}
				else {
					dst[dx+dw*y] = 0;
				}
			}
		}
	}
} /*}}}*/
void combine_crop_transform_layerset(uint8_t *dst, int dw, int dh, layerset *ls, int *deltax) { /*{{{*/
	blit_subsect(dst, ls->layer[0], dw, dh, ls->w, deltax[0]);
	for (int i=0; i<4; ++i) {
		combine_subtract_subsect(dst, ls->layer[i*2+1], dw, dh, ls->w, deltax[i*2+1], 191);
		combine_add_subsect(dst, ls->layer[i*2+2], dw, dh, ls->w, deltax[i*2+2], 191);
	}
	combine_subtract_subsect(dst, ls->layer[10], dw, dh, ls->w, deltax[10], 255);
} /*}}}*/
int main(void) { /*{{{*/
	srand(time(NULL)+getpid());
	uint8_t *dst[4];
	int deltax[LAYERS];
	fprintf(stderr, "allocating memory..."); /*{{{*/
	layerset *ls[3];
	for (int i=0; i<3; ++i) {
		ls[i] = alloc_layerset(LAYERWIDTH, HEIGHT);
		dst[i] = malloc(sizeof(uint8_t)*WIDTH*HEIGHT);
	}
	fprintf(stderr, " ok\n"); /*}}}*/
	fprintf(stderr, "building base layers..."); /*{{{*/
	for (int i=0; i<3; ++i) {
		build_layerset(ls[i], RULE, BASERATE);
	}
	fprintf(stderr, " ok\n");/*}}}*/
	fprintf(stderr, "generating images..."); /*{{{*/
	deltax[LAYERS-1] = 0;
	for (int i=0; i<FRAMES; ++i) {
		fprintf(stderr, " [%u", i);
		fprintf(stderr, "\n");
		for (int j=0; j<5; ++j) {
			deltax[j*2] = -2*(i>>(4-j));
			deltax[j*2+1] = 2*(i>>(4-j));
		}
		for (int j=0; j<LAYERS-1; ++j) {
			deltax[j] = (j&1?2:-2)*(i>>(4-(j>>1)));
			fprintf(stderr, "%d ",deltax[j]);
		}
		fprintf(stderr, "\n");
		for (int j=0; j<3; ++j) {
			fprintf(stderr, ".");
			combine_crop_transform_layerset(dst[j], WIDTH, HEIGHT, ls[j], deltax);
		}
		fprintf(stderr, "!");
		output_enumerated_png("outrg/rg_%06u.png", i, dst, WIDTH, HEIGHT);
		fprintf(stderr, "]");
	}
	fprintf(stderr, " ok\n"); /*}}}*/
	fprintf(stderr, "freeing memory..."); /*{{{*/
	for (int i=0; i<3; ++i) {
		free(dst[i]);
		free_layerset(ls[i]);
	}
	fprintf(stderr, " ok\n"); /*}}}*/
	return 0;
} /*}}}*/
