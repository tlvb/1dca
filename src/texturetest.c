#include "graphics_base.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef __LINUX__
#include <unistd.h>
#endif
#include <time.h>
/*{{{*/
#define vertex_shader_source \
"#version 110\n" \
"attribute vec2 position;\n" \
"attribute vec2 texcoord;\n" \
"varying vec2 fragtc;\n" \
"void main(void) {\n" \
"	fragtc = texcoord;\n" \
"	gl_Position = vec4(position.x, position.y, 0.0, 1);\n" \
"}\n"
/*}}}*/
/*{{{*/
#define fragment_shader_source \
"#version 110\n" \
"uniform sampler2D texture;\n" \
"uniform int texi;\n" \
"uniform vec2 texo;\n" \
"varying vec2 fragtc;\n" \
"void main(void) {\n" \
"   vec4 clr = texture2D(texture, fragtc+texo);\n" \
"   gl_FragColor = vec4(clr.r, clr.g, clr.b, clr.a);\n" \
"}\n"
/*}}}*/
#define LAYERS 15
#define TEXTURE_WIDTH 4096
#define TEXTURE_HEIGHT 2048
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define RULE 184
#define BASERATE 1000
#define apply_rule(_rule_, _ila_, _ica_, _ira_, _c_) \
	(((_rule_) & (1u<<(((*(_ila_)!=0)?4:0) | ((*(_ica_)!=0)?2:0) | ((*(_ira_)!=0)?1:0)))) != 0 ? (_c_) : 0)

void apply_rule_row(int rule, uint8_t *orow, const uint8_t *irow, int n, int o, int strength) { /*{{{*/
	orow[o] = apply_rule(rule, irow+(n-1)*3+o, irow+o, irow+3+o, strength);
	for (int i=1; i<n-1; ++i) {
		orow[i*3+o] = apply_rule(rule, irow+(i-1)*3+o, irow+i*3+o, irow+(i+1)*3+o, strength);
	}
	orow[(n-1)*3+o] = apply_rule(rule, irow+(n-2)*3+o, irow+(n-1)*3+o, irow+o, strength);
} /*}}}*/
void seed_world(uint8_t *world, int x, int y, int w, int n, int o, int strength) { /*{{{*/
	for (int i=0; i<n; ++i) {
		world[(x+i)*3+o+y*w*3] = (rand()%2)==0?strength:0;
	}
} /*}}}*/
void build_world(uint8_t *world, int w, int h, int rule, int rate, int how, int strength) { /*{{{*/
	for (int o=0; o<3; ++o) {
		if (how == 0) {
			// from middle
			unsigned int m = h/2;
			seed_world(world, 0, m, w, w, o, strength);
			for (unsigned int y=m; y>0; --y) {
				apply_rule_row(rule, world+(y-1)*w*3, world+y*w*3, w, o, strength);
				for (int c=rate; c>rand()%100; c-=100) {
					seed_world(world, ((rand()%w)&~7), y-1, w, 8, o, strength);
				}
			}
			for (unsigned int y=m+1; y<h; ++y) {
				apply_rule_row(rule, world+y*w*3, world+(y-1)*w*3, w, o, strength);
				for (int c=rate; c>rand()%100; c-=100) {
					seed_world(world, ((rand()%w)&~7), y, w, 8, o, strength);
				}
			}
		}
		else if (how == 1) {
			// from top
			seed_world(world, 0, 0, w, w, o, strength);
			for (int y=1; y<h; ++y) {
				apply_rule_row(rule, world+y*w*3, world+(y-1)*w*3, w, o, strength);
				for (int c=rate; c>rand()%100; c-=100) {
					seed_world(world, ((rand()%w)&~7), y, w, 8, o, strength);
				}
			}
		}
		else if (how == 2) {
			// from bottom
			seed_world(world, 0, h-1, w, w, o, strength);
			for (int y=h-1; y>0; --y) {
				apply_rule_row(rule, world+(y-1)*w*3, world+y*w*3, w, o, strength);
				for (int c=rate; c>rand()%100; c-=100) {
					seed_world(world, ((rand()%w)&~7), y-1, w, 8, o, strength);
				}
			}
		}
	}
} /*}}}*/
int main(void) {

#ifdef __LINUX__
	srand(time(NULL)+getpid());
#else
	srand(time(NULL));
#endif

	uint8_t *textures[LAYERS];
	for (int i=0; i<LAYERS; ++i) {
		textures[i] = malloc(sizeof(uint8_t)*TEXTURE_WIDTH*TEXTURE_HEIGHT*3);
		if (textures[i] == NULL) {
			for (int j=0; j<i; ++j) {
				free(textures[j]);
			}
			fprintf(stderr, "could not allocate enough memory\n");
			return -1;
		}
	}
#if (LAYERS > 1)
	int ratedelta = BASERATE/(LAYERS-1);
#else
	int ratedelta = 0;
#endif
	for (int i=0; i<LAYERS; ++i) {
		build_world(textures[i], TEXTURE_WIDTH, SCREEN_HEIGHT, RULE, BASERATE-ratedelta*i, i==LAYERS-1?0:i%3, ((i==LAYERS-1)||(i==0))?255:255);
	}

	windowdata wd = WINDOW_DATA_DEFAULT;
	wd.width = SCREEN_WIDTH;
	wd.height = SCREEN_HEIGHT;
	wd.flags = SDL_WINDOW_FULLSCREEN;
	if (!create_gl_window(&wd)) { /*{{{*/
		return -1;
	} /*}}}*/
	shaderdata sd = SHADER_DATA_DEFAULT;
	sd.vertex.source = (uint8_t*) vertex_shader_source;
	sd.fragment.source = (uint8_t*) fragment_shader_source;

	if (!mk_shader_program(&sd)) { /*{{{*/
		destroy_gl_window(&wd);
		return -1;
	} /*}}}*/
	glUseProgram(sd.program);

	/*\
	 *
	 *  0----1
	 *  |   /| ^
	 *  |  / | |
	 *  | /  | +-->
	 *  |/   |
	 *  2----3
	 *
	\*/
	GLfloat mesh[8] = { /*{{{*/
		-1.0f, +1.0f,
		+1.0f, +1.0f,
		-1.0f, -1.0f,
		+1.0f, -1.0f,
	}; /*}}}*/
	GLfloat sm = 1920.0f / 4096.0f;
	GLfloat tm = 1080.0f / 2048.0f;
	//sm = 1;
	//tm = 1;
	GLfloat texc[8] = { /*{{{*/
		0.0f, tm,
		sm,   tm,
		0.0f, 0.0f,
		sm,   0
	}; /*}}}*/

	GLuint position_ref = glGetAttribLocation(sd.program, "position");
	GLuint texcoord_ref = glGetAttribLocation(sd.program, "texcoord");
	GLuint texi_ref = glGetUniformLocation(sd.program, "texi");
	GLuint texo_ref = glGetUniformLocation(sd.program, "texo");
	//GLuint texture_ref = glGetUniformLocation(sd.program, "texture");
	GLuint position_buf_ref;
	GLuint texcoord_buf_ref;
	GLuint texture_tex_ref[LAYERS];

	/*{{{*/
	glGenBuffers(1, &position_buf_ref);
	glBindBuffer(GL_ARRAY_BUFFER, position_buf_ref);
	glBufferData( /*{{{*/
		GL_ARRAY_BUFFER,
		sizeof(mesh),
		mesh,
		GL_STATIC_DRAW
	); /*}}}*/
	glVertexAttribPointer( /*{{{*/
		position_ref,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(GLfloat)*2,
		NULL
	); /*}}}*/
	glEnableVertexAttribArray(position_ref);
	/*}}}*/
	/*{{{*/
	glGenBuffers(1, &texcoord_buf_ref);
	glBindBuffer(GL_ARRAY_BUFFER, texcoord_buf_ref);
	glBufferData( /*{{{*/
		GL_ARRAY_BUFFER,
		sizeof(texc),
		texc,
		GL_STATIC_DRAW
	); /*}}}*/
	glVertexAttribPointer( /*{{{*/
		texcoord_ref,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(GLfloat)*2,
		NULL
	); /*}}}*/
	glEnableVertexAttribArray(texcoord_ref);
	/*}}}*/

	glGenTextures(LAYERS, texture_tex_ref);
	for (int i=0; i<LAYERS; ++i) { /*{{{*/
		glBindTexture(GL_TEXTURE_2D, texture_tex_ref[i]);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGB8,
			TEXTURE_WIDTH,
			TEXTURE_HEIGHT,
			0,
			GL_RGB,
			GL_UNSIGNED_BYTE,
			textures[i]
		);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
	} /*}}}*/

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	bool done = false;
	int o = 0;
	while (!done) { /*{{{*/
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		for (int i=0; i<LAYERS; ++i) {
			glBindTexture(GL_TEXTURE_2D, texture_tex_ref[i]);
			if (i==LAYERS-1) {
				//glBlendEquation(GL_MIN);
				glBlendEquation(GL_FUNC_SUBTRACT);
				glUniform2f(texo_ref, 0.0f, 0.0f);
			}
			else if ((i==0) || ((i&1) != 0)) {
				//glBlendEquation(GL_MAX);
				glBlendEquation(GL_FUNC_ADD);
				glUniform2f(texo_ref, (float)(-o<<((LAYERS-1-i)>>1))/TEXTURE_WIDTH, 0.0f);
			}
			else {
				//glBlendEquation(GL_MIN);
				glBlendEquation(GL_FUNC_SUBTRACT);
				glUniform2f(texo_ref, (float)(o<<((LAYERS-1-i)>>1))/TEXTURE_WIDTH, 0.0f);
			}
			glUniform1i(texi_ref, i);
			glDrawArrays(
				GL_TRIANGLE_STRIP,
				0,
				4
			);
		}
		SDL_GL_SwapWindow(wd.window);
		SDL_Event e;
		if (SDL_PollEvent(&e)) {
			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_q) {
					done = true;
				}
			}
		}
		o = (o+2)%TEXTURE_WIDTH;
	} /*}}}*/

	rm_shader_program(&sd);
	destroy_gl_window(&wd);
	for (int i=0; i<LAYERS; ++i) {
		free(textures[i]);
	}
	return 0;


}
