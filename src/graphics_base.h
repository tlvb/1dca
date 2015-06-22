#ifndef graphics_base_h
#define graphics_base_h
#include <stdbool.h>
#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

typedef struct {
	SDL_Window *window;
	SDL_GLContext glcontext;
	char *title;
	int width;
	int height;
	int flags;
} windowdata;
#define WINDOW_DATA_DEFAULT {.window=NULL,.glcontext=0,.title=NULL,.width=0,.height=0,.flags=0}

typedef struct {
	char *filename;
	uint8_t *source;
	GLint ref;
} shader;
#define SHADER_DEFAULT {.filename=NULL,.source=NULL,.ref=0}

typedef struct {
	shader vertex;
	shader fragment;
	GLint program;
} shaderdata;
#define SHADER_DATA_DEFAULT {.vertex=SHADER_DEFAULT,.fragment=SHADER_DEFAULT,.program=0}

bool create_gl_window(windowdata *wd);
void destroy_gl_window(windowdata *wd);
bool mk_shader_program(shaderdata *sd);
void rm_shader_program(shaderdata *sd);
#endif
