#include "graphics_base.h"
#include <assert.h>
bool create_gl_window(windowdata *wd) { /*{{{*/
	SDL_Init(SDL_INIT_VIDEO);
	// window creation {{{
	if ((wd->width <= 0 || wd->height <= 0) && ((wd->flags & SDL_WINDOW_FULLSCREEN_DESKTOP) == 0)) {
		fprintf(stderr, "unreasonable window dimensions: %dx%d\n", wd->width, wd->height);
		return false;
	}
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	wd->window = SDL_CreateWindow(
		wd->title,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		wd->width,
		wd->height,
		wd->flags | SDL_WINDOW_OPENGL
	);
	if (wd->window == NULL) {
		fprintf(stderr, "window creation error\n---\n%s\n---\n", SDL_GetError());
		return false;
	}
	// }}}
	// gl context creation {{{
	wd->glcontext = SDL_GL_CreateContext(wd->window);
	if (wd->glcontext == NULL) {
		fprintf(stderr, "gl context creation error\n---\n%s\n---\n", SDL_GetError());
		return false;
	}
	// }}}
	// gl environment setup {{{
	glViewport(0,0, (GLsizei)wd->width, (GLsizei)wd->height);
	glClearDepth(1.0f);
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glBlendEquation(GL_FUNC_ADD);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	// }}}
	return true;
} /*}}}*/
void destroy_gl_window(windowdata *wd) { /*{{{*/
	if (wd->glcontext != NULL) {
		SDL_GL_DeleteContext(wd->glcontext);
	}
	if (wd->window != NULL) {
		SDL_DestroyWindow(wd->window);
	}
} /*}}}*/
static void load_shader_source_from_file(shader *sh) { /*{{{*/
	FILE *fd = fopen(sh->filename, "r");
	if (fd == NULL) {
		fprintf(stderr, "null file descriptor for %s\n", sh->filename);
	}
	fseek(fd, 0, SEEK_END);
	long len = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	sh->source = malloc(sizeof(uint8_t)*(len+1));
	assert(sh->source);
	fread(sh->source, sizeof(uint8_t), len, fd);
	fclose(fd);
} /*}}}*/
static GLint mk_shader_from_source(GLenum type, const uint8_t *source) { /*{{{*/
	long len = strlen((const char*)source);
	// create and compile shader from source {{{
	GLint shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&source, (GLint*)&len);
	glCompileShader(shader);
	/// }}}
	// check for compile errors {{{
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		fprintf(stderr, "%s shader compilation failed:\n================\n", type==GL_VERTEX_SHADER?"vertex":"fragment");
		GLint loglen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &loglen);
		char *log = malloc(loglen);
		assert(log);
		glGetShaderInfoLog(shader, loglen, NULL, log);
		fprintf(stderr, log);
		free(log);
		fprintf(stderr, "----------------\n");
		fprintf(stderr, (const char*)source);
		fprintf(stderr, "================\n");

		glDeleteShader(shader);
		return 0;
	}
	// }}}
	return shader;
} /*}}}*/
bool mk_shader_program(shaderdata *sd) { /*{{{*/
	// load and compile shader sources {{{
	if (sd->vertex.source == NULL) {
		load_shader_source_from_file(&sd->vertex);
	}
	sd->vertex.ref = mk_shader_from_source(GL_VERTEX_SHADER, sd->vertex.source);
	if (sd->vertex.ref == 0) {
		return false;
	}
	if (sd->fragment.source == NULL) {
		load_shader_source_from_file(&sd->fragment);
	}
	sd->fragment.ref = mk_shader_from_source(GL_FRAGMENT_SHADER, sd->fragment.source);
	if (sd->fragment.ref == 0)  {
		glDeleteShader(sd->vertex.ref);
		return false;
	}
	// }}}
	// link programs {{{
	sd->program = glCreateProgram();
	glAttachShader(sd->program, sd->vertex.ref);
	glAttachShader(sd->program, sd->fragment.ref);
	glLinkProgram(sd->program);
	// }}}
	// check for linker errors {{{
	GLint status;
	glGetProgramiv(sd->program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		fprintf(stderr, "shader linking failed:\n----------------\n");
		GLint loglen;
		glGetProgramiv(sd->program, GL_INFO_LOG_LENGTH, &loglen);
		char *log = malloc(loglen);
		assert(log);
		glGetProgramInfoLog(sd->program, loglen, NULL, log);
		fprintf(stderr, log);
		fprintf(stderr, "----------------\n");
		free(log);
		rm_shader_program(sd);
		return false;
	}
	// }}}
	return true;
} /*}}}*/
void rm_shader_program(shaderdata *sd) { /*{{{*/
	glDetachShader(sd->program, sd->vertex.ref);
	glDetachShader(sd->program, sd->fragment.ref);
	glDeleteShader(sd->vertex.ref);
	glDeleteShader(sd->fragment.ref);
	glDeleteProgram(sd->program);
	sd->vertex.ref = 0;
	sd->fragment.ref = 0;
	sd->program = 0;
} /*}}}*/
