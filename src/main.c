#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#define WIDTH 400
#define HEIGHT 800

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20

#define INIT_X 5
#define INIT_Y 21

#define VERTEX_ELEMENTS 6
#define QUAD_VERTICES 4
#define QUAD_INDICES 6

enum Tetromino
{
	LONG,
	RIGHT_L,
	LEFT_L,
	SQUARE,
	RIGHT_S,
	LEFT_S,
	T
};

enum Colors
{
	NONE,
	LIGHT_BLUE,
	BLUE,
	ORANGE,
	YELLOW,
	GREEN,
	PURPLE,
	RED
};

struct ActiveShape
{
	int x, y, rotation;
	enum Colors color;
	enum Tetromino shape;
};

struct Color
{
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat a;
};

#define COLOR_NONE (struct Color){.r=0.2f, .g=0.2f, .b=0.2f, .a=1.0f}
#define COLOR_LIGHT_BLUE (struct Color){.r=0.0f, .g=1.0f, .b=1.0f, .a=1.0f}
#define COLOR_BLUE (struct Color){.r=0.0f, .g=0.0f, .b=1.0f, .a=1.0f}
#define COLOR_ORANGE (struct Color){.r=1.0f, .g=0.5f, .b=0.0f, .a=1.0f}
#define COLOR_YELLOW (struct Color){.r=1.0f, .g=1.0f, .b=0.0f, .a=1.0f}
#define COLOR_GREEN (struct Color){.r=0.0f, .g=1.0f, .b=0.0f, .a=1.0f}
#define COLOR_PURPLE (struct Color){.r=1.0f, .g=0.0f, .b=1.0f, .a=1.0f}
#define COLOR_RED (struct Color){.r=1.0f, .g=0.0f, .b=0.0f, .a=1.0f}

typedef struct Shaders 
{
	GLuint VBO, VAO, EBO, program_id;
} Shaders;

int64_t fsize(FILE *f)
{
    fseek(f, 0L, SEEK_END);
    int size = (int64_t)ftell(f);
    fseek(f, 0L, SEEK_SET);

    return size;
}

char *read_file(const char * const path)
{
	FILE *file = fopen(path, "rb"); 

	if(!file)
	{
		printf("ERROR: Failed to open %s.\n", path);
		abort();
	}

	int64_t size = fsize(file);
	
	char *str = malloc(sizeof(char) * size);
	fread(str, sizeof(char), size, file);

	return str;
}

Shaders *init_shaders(const char * const path_to_vertex_shader, const char * const path_to_fragment_shader)
{
	Shaders *shaders = malloc(sizeof(Shaders));

	const char *vertex_shader_code = read_file(path_to_vertex_shader);
	const char *fragment_shader_code = read_file(path_to_fragment_shader);

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_code, NULL);
	glCompileShader(vertex_shader);

	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_code, NULL);
	glCompileShader(fragment_shader);

	shaders->program_id = glCreateProgram();
	glAttachShader(shaders->program_id, vertex_shader);
	glAttachShader(shaders->program_id, fragment_shader);
	glLinkProgram(shaders->program_id);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return shaders;
}

void activate_shaders(Shaders shaders)
{
	glUseProgram(shaders.program_id);
}

void delete_shaders(Shaders shaders)
{
    glDeleteProgram(shaders.program_id);
}

void init_VBO(Shaders *shaders, GLfloat vertices[], size_t n)
{
	glGenBuffers(1, &shaders->VBO);
    glBindBuffer(GL_ARRAY_BUFFER, shaders->VBO);
    glBufferData(GL_ARRAY_BUFFER, n, vertices, GL_STATIC_DRAW);
}

void bind_VBO(Shaders shaders)
{
    glBindBuffer(GL_ARRAY_BUFFER, shaders.VBO);
}

void unbind_VBO(Shaders shaders)
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void delete_VBO(Shaders *shaders)
{
	glDeleteBuffers(1, &shaders->VBO);
}

void init_EBO(Shaders *shaders, GLuint indices[], size_t n)
{
	glGenBuffers(1, &shaders->EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shaders->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, n, indices, GL_STATIC_DRAW);
}

void bind_EBO(Shaders shaders)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shaders.EBO);
}

void unbind_EBO(Shaders shaders)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void delete_EBO(Shaders *shaders)
{
	glDeleteBuffers(1, &shaders->EBO);
}

void init_VAO(Shaders *shaders)
{
    glGenVertexArrays(1, &shaders->VAO);
}

void link_attribute(Shaders shaders, GLuint layout, GLuint n_components, GLenum type, GLsizeiptr stride, void *offset)
{
	bind_VBO(shaders);

    glVertexAttribPointer(layout, n_components, type, GL_FALSE, stride, offset);
    glEnableVertexAttribArray(layout);

	unbind_VBO(shaders);
}

void bind_VAO(Shaders shaders)
{
    glBindVertexArray(shaders.VAO);
}

void unbind_VAO(Shaders shaders)
{
    glBindVertexArray(0);
}

void delete_VAO(Shaders *shaders)
{
	glDeleteBuffers(1, &shaders->VAO);
}

int xy_to_index(int x, int y, int width)
{
	return x + (width % y);
}

float scale_x(int x)
{
	return ((float)(x * 2) / (float)BOARD_WIDTH) - 1.0f;
}

float scale_y(int y)
{
	return ((float)(y * 2) / (float)BOARD_HEIGHT) - 1.0f;
}

void add_quad(int x, int y, struct Color color, GLfloat vertices[], size_t *v_i, GLuint indices[], size_t *i_i)
{
	vertices[*v_i * VERTEX_ELEMENTS] = scale_x(x);
	vertices[*v_i * VERTEX_ELEMENTS + 1] = scale_y(y);
	vertices[*v_i * VERTEX_ELEMENTS + 2] = 0.0f;
	vertices[*v_i * VERTEX_ELEMENTS + 3] = color.r;
	vertices[*v_i * VERTEX_ELEMENTS + 4] = color.g;
	vertices[*v_i * VERTEX_ELEMENTS + 5] = color.b;

	vertices[(*v_i + 1) * VERTEX_ELEMENTS] = scale_x(x + 1);
	vertices[(*v_i + 1) * VERTEX_ELEMENTS + 1] = scale_y(y);
	vertices[(*v_i + 1) * VERTEX_ELEMENTS + 2] = 0.0f;
	vertices[(*v_i + 1) * VERTEX_ELEMENTS + 3] = color.r;
	vertices[(*v_i + 1) * VERTEX_ELEMENTS + 4] = color.g;
	vertices[(*v_i + 1) * VERTEX_ELEMENTS + 5] = color.b;

	vertices[(*v_i + 2) * VERTEX_ELEMENTS] = scale_x(x + 1);
	vertices[(*v_i + 2) * VERTEX_ELEMENTS + 1] = scale_y(y + 1);
	vertices[(*v_i + 2) * VERTEX_ELEMENTS + 2] = 0.0f;
	vertices[(*v_i + 2) * VERTEX_ELEMENTS + 3] = color.r;
	vertices[(*v_i + 2) * VERTEX_ELEMENTS + 4] = color.g;
	vertices[(*v_i + 2) * VERTEX_ELEMENTS + 5] = color.b;

	vertices[(*v_i + 3) * VERTEX_ELEMENTS] = scale_x(x);
	vertices[(*v_i + 3) * VERTEX_ELEMENTS + 1] = scale_y(y + 1);
	vertices[(*v_i + 3) * VERTEX_ELEMENTS + 2] = 0.0f;
	vertices[(*v_i + 3) * VERTEX_ELEMENTS + 3] = color.r;
	vertices[(*v_i + 3) * VERTEX_ELEMENTS + 4] = color.g;
	vertices[(*v_i + 3) * VERTEX_ELEMENTS + 5] = color.b;

	indices[*i_i] = *v_i;
	indices[*i_i + 1] = *v_i + 1;
	indices[*i_i + 2] = *v_i + 2;
	indices[*i_i + 3] = *v_i;
	indices[*i_i + 4] = *v_i + 3;
	indices[*i_i + 5] = *v_i + 2;

	*v_i += QUAD_VERTICES;
	*i_i += QUAD_INDICES;
}

struct Point
{
	float x, y;
};

void matrix_mulitply(struct Point *point, int rotation)
{
	float x = point->x;
	float y = point->y;

	point->x = (x * cos(rotation * (M_PI / 180.0f))) - (y * sin(rotation * (M_PI / 180.0f)));
	point->y = (x * sin(rotation * (M_PI / 180.0f))) + (y * cos(rotation * (M_PI / 180.0f)));
}

void get_points(struct ActiveShape *shape, struct Point points[])
{
	switch(shape->shape)
	{
		case LONG:
			points[0] = (struct Point){.x=-1.5f, .y=0.5f};
			points[1] = (struct Point){.x=-0.5f, .y=0.5f};
			points[2] = (struct Point){.x=0.5f, .y=0.5f};
			points[3] = (struct Point){.x=1.5f, .y=0.5f};

			for(int i = 0; i < 4; i++)
			{
				matrix_mulitply(&points[i], shape->rotation);

				points[i].x = points[i].x + 0.5;
				points[i].y = points[i].y + 0.5;
			}

			break;
		case RIGHT_L:
			points[0] = (struct Point){.x=0.0f, .y=0.0f};
			points[1] = (struct Point){.x=1.0f, .y=0.0f};
			points[2] = (struct Point){.x=-1.0f, .y=0.0f};
			points[3] = (struct Point){.x=-1.0f, .y=1.0f};

			for(int i = 0; i < 4; i++)
			{
				matrix_mulitply(&points[i], shape->rotation);
			}

			break;
		case LEFT_L:
			points[0] = (struct Point){.x=0.0f, .y=0.0f};
			points[1] = (struct Point){.x=-1.0f, .y=0.0f};
			points[2] = (struct Point){.x=1.0f, .y=0.0f};
			points[3] = (struct Point){.x=1.0f, .y=1.0f};

			for(int i = 0; i < 4; i++)
			{
				matrix_mulitply(&points[i], shape->rotation);
			}

			break;
		case SQUARE:
			points[0] = (struct Point){.x=0.5f, .y=0.5f};
			points[1] = (struct Point){.x=0.5f, .y=-0.5f};
			points[2] = (struct Point){.x=-0.5f, .y=-0.5f};
			points[3] = (struct Point){.x=-0.5f, .y=0.5f};

			for(int i = 0; i < 4; i++)
			{
				matrix_mulitply(&points[i], shape->rotation);

				points[i].x = points[i].x + 0.5f;
				points[i].y = points[i].y + 0.5f;
			}

			break;
		case RIGHT_S:
			points[0] = (struct Point){.x=0.0f, .y=0.0f};
			points[1] = (struct Point){.x=0.0f, .y=1.0f};
			points[2] = (struct Point){.x=1.0f, .y=1.0f};
			points[3] = (struct Point){.x=-1.0f, .y=0.0f};

			for(int i = 0; i < 4; i++)
			{
				matrix_mulitply(&points[i], shape->rotation);
			}

			break;
		case LEFT_S:
			points[0] = (struct Point){.x=0.0f, .y=0.0f};
			points[1] = (struct Point){.x=0.0f, .y=1.0f};
			points[2] = (struct Point){.x=-1.0f, .y=1.0f};
			points[3] = (struct Point){.x=1.0f, .y=0.0f};

			for(int i = 0; i < 4; i++)
			{
				matrix_mulitply(&points[i], shape->rotation);
			}

			break;
		case T:
			points[0] = (struct Point){.x=0.0f, .y=0.0f};
			points[1] = (struct Point){.x=0.0f, .y=1.0f};
			points[2] = (struct Point){.x=1.0f, .y=0.0f};
			points[3] = (struct Point){.x=-1.0f, .y=0.0f};

			for(int i = 0; i < 4; i++)
			{
				matrix_mulitply(&points[i], shape->rotation);
			}

			break;
	}
}

void add_active_shape(struct ActiveShape *shape, GLfloat vertices[], size_t *v_i, GLuint indices[], size_t *i_i)
{
	struct Point points[4];
	get_points(shape, points);

	switch(shape->shape) 
	{
		case LONG:
			add_quad(shape->x + points[0].x, shape->y + points[0].y, COLOR_LIGHT_BLUE, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[1].x, shape->y + points[1].y, COLOR_LIGHT_BLUE, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[2].x, shape->y + points[2].y, COLOR_LIGHT_BLUE, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[3].x, shape->y + points[3].y, COLOR_LIGHT_BLUE, vertices, v_i, indices, i_i);

			break;
		case RIGHT_L:
			add_quad(shape->x + points[0].x, shape->y + points[0].y, COLOR_BLUE, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[1].x, shape->y + points[1].y, COLOR_BLUE, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[2].x, shape->y + points[2].y, COLOR_BLUE, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[3].x, shape->y + points[3].y, COLOR_BLUE, vertices, v_i, indices, i_i);

			break;
		case LEFT_L:
			add_quad(shape->x + points[0].x, shape->y + points[0].y, COLOR_ORANGE, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[1].x, shape->y + points[1].y, COLOR_ORANGE, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[2].x, shape->y + points[2].y, COLOR_ORANGE, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[3].x, shape->y + points[3].y, COLOR_ORANGE, vertices, v_i, indices, i_i);

			break;
		case SQUARE:
			add_quad(shape->x + points[0].x, shape->y + points[0].y, COLOR_YELLOW, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[1].x, shape->y + points[1].y, COLOR_YELLOW, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[2].x, shape->y + points[2].y, COLOR_YELLOW, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[3].x, shape->y + points[3].y, COLOR_YELLOW, vertices, v_i, indices, i_i);

			break;
		case RIGHT_S:
			add_quad(shape->x + points[0].x, shape->y + points[0].y, COLOR_GREEN, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[1].x, shape->y + points[1].y, COLOR_GREEN, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[2].x, shape->y + points[2].y, COLOR_GREEN, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[3].x, shape->y + points[3].y, COLOR_GREEN, vertices, v_i, indices, i_i);

			break;
		case LEFT_S:
			add_quad(shape->x + points[0].x, shape->y + points[0].y, COLOR_RED, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[1].x, shape->y + points[1].y, COLOR_RED, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[2].x, shape->y + points[2].y, COLOR_RED, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[3].x, shape->y + points[3].y, COLOR_RED, vertices, v_i, indices, i_i);
		
			break;
		case T:
			add_quad(shape->x + points[0].x, shape->y + points[0].y, COLOR_PURPLE, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[1].x, shape->y + points[1].y, COLOR_PURPLE, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[2].x, shape->y + points[2].y, COLOR_PURPLE, vertices, v_i, indices, i_i);
			add_quad(shape->x + points[3].x, shape->y + points[3].y, COLOR_PURPLE, vertices, v_i, indices, i_i);

			break;
	}
}

int check_side_collision(struct ActiveShape *shape, enum Colors board[], int change_x)
{
	struct Point points[4];
	get_points(shape, points);

	for(int i = 0; i < 4; i++)
	{
		int x = shape->x + points[i].x + change_x;
		int y = shape->y + points[i].y;
		int xy_to_i = x + (y * BOARD_WIDTH);

		if(x < 0 || x > BOARD_WIDTH - 1 || board[xy_to_i] != NONE)
		{
			return 1;
		}
	}

	return 0;
}

int check_bottom_collision(struct ActiveShape *shape, enum Colors board[])
{
	struct Point points[4];
	get_points(shape, points);

	for(int i = 0; i < 4; i++)
	{
		int x = shape->x + points[i].x;
		int y = shape->y + points[i].y - 1;
		int xy_to_i = x + (y * BOARD_WIDTH);


		if(y < 0)
		{
			return 1;
		}
		else if(y < BOARD_HEIGHT)
		{
			if(board[xy_to_i])
			{
				return 1;
			}
		}
	}

	return 0;
}

#define INITIAL_AUTO_DROP_DELAY 60

#define ARR 2
#define DAS 10
#define LOCK_DELAY 30
#define LOCK_DELAY_LIMIT 10

#define ACTIVE 1
#define INACTIVE 0

#define RELEASED 0
#define PRESSED 1
#define HELD 2

struct KeyState
{
	int is_active;
	int mode;
	int frame_delta;
};

void manage_key_state(struct KeyState *key_state, GLFWwindow *window, int key, int can_hold)
{
	if(glfwGetKey(window, key) == GLFW_RELEASE)
	{
		key_state->is_active = INACTIVE;
		key_state->mode = RELEASED;
	}

	if(glfwGetKey(window, key) == GLFW_PRESS)
	{
		switch(key_state->mode) 
		{
			case RELEASED:
				key_state->frame_delta = 0;

				key_state->is_active = ACTIVE;
				key_state->mode = PRESSED;

				break;
			case PRESSED:
				if(key_state->frame_delta < DAS)
				{
					key_state->is_active = INACTIVE;
				}
				else 
				{
					key_state->frame_delta = 0;

					if(can_hold)
					{
						key_state->is_active = ACTIVE;
						key_state->mode = HELD;
					}
				}

				break;
			case HELD:
				if(key_state->frame_delta < ARR)
				{
					key_state->is_active = INACTIVE;
				}
				else 
				{
					key_state->frame_delta = 0;

					key_state->is_active = ACTIVE;
				}

				break;
		}
	}
}

void generate_new_active_shape(struct ActiveShape *shape)
{
	int tetromino_type = rand() % ((int)T + 1);

	shape->x = INIT_X;
	shape->y = INIT_Y;
	shape->rotation = 0;
	shape->shape = (enum Tetromino)tetromino_type;

	switch (shape->shape)
	{
		case LONG:
			shape->color = LIGHT_BLUE;
			break;
		case RIGHT_L:
			shape->color = BLUE;
			break;
		case LEFT_L:
			shape->color = ORANGE;
			break;
		case SQUARE:
			shape->color = YELLOW;
			break;
		case RIGHT_S:
			shape->color = GREEN;
			break;
		case LEFT_S:
			shape->color = RED;
			break;
		case T:
			shape->color = PURPLE;
			break;
	}
}

int check_and_transform(struct ActiveShape *shape, struct Point shape_points[], struct Point transform_points[], enum Colors board[])
{
	for(int t = 0; t < 5; t++)
	{
		int valid_transformation = 1;

		for(int p = 0; p < 4; p++)
		{
			int x = shape->x + (int)shape_points[p].x + (int)transform_points[t].x;
			int y = shape->y + (int)shape_points[p].y + (int)transform_points[t].y;
			int i = x + (y * BOARD_WIDTH);

			if(board[i] != NONE || x < 0 || y < 0 || x >= BOARD_WIDTH)
			{
				valid_transformation = 0;
			}
		}

		if(valid_transformation)
		{
			shape->x += (int)transform_points[t].x;
			shape->y += (int)transform_points[t].y;

			return 1;
		}
	}

	return 0;
}

int rotate(struct ActiveShape *shape, enum Colors board[])
{
	// Wall kicks for L, S, Square, and T pieces
	struct Point rot_0_to_1LSST[5] = {
		(struct Point){ .x = 0.0f, .y = 0.0f },
		(struct Point){ .x = -1.0f, .y = 0.0f },
		(struct Point){ .x = -1.0f, .y = 1.0f },
		(struct Point){ .x = 0.0f, .y = -2.0f },
		(struct Point){ .x = -1.0f, .y = -2.0f }
	};

	struct Point rot_1_to_0LSST[5] = {
		(struct Point){ .x = 0.0f, .y = 0.0f },
		(struct Point){ .x = 1.0f, .y = 0.0f },
		(struct Point){ .x = 1.0f, .y = -1.0f },
		(struct Point){ .x = 0.0f, .y = 2.0f },
		(struct Point){ .x = 1.0f, .y = 2.0f }
	};

	struct Point rot_1_to_2LSST[5] = {
		(struct Point){ .x = 0.0f, .y = 0.0f },
		(struct Point){ .x = 1.0f, .y = 0.0f },
		(struct Point){ .x = 1.0f, .y = -1.0f },
		(struct Point){ .x = 0.0f, .y = 2.0f },
		(struct Point){ .x = 1.0f, .y = 2.0f }
	};

	struct Point rot_2_to_1LSST[5] = {
		(struct Point){ .x = 0.0f, .y = 0.0f },
		(struct Point){ .x = -1.0f, .y = 0.0f },
		(struct Point){ .x = -1.0f, .y = 1.0f },
		(struct Point){ .x = 0.0f, .y = -2.0f },
		(struct Point){ .x = -1.0f, .y = -2.0f }
	};

	struct Point rot_2_to_3LSST[5] = {
		(struct Point){ .x = 0.0f, .y = 0.0f },
		(struct Point){ .x = 1.0f, .y = 0.0f },
		(struct Point){ .x = 1.0f, .y = 1.0f },
		(struct Point){ .x = 0.0f, .y = -2.0f },
		(struct Point){ .x = 1.0f, .y = -2.0f }
	};

	struct Point rot_3_to_2LSST[5] = {
		(struct Point){ .x = 0.0f, .y = 0.0f },
		(struct Point){ .x = -1.0f, .y = 0.0f },
		(struct Point){ .x = -1.0f, .y = -1.0f },
		(struct Point){ .x = 0.0f, .y = 2.0f },
		(struct Point){ .x = -1.0f, .y = 2.0f }
	};

	struct Point rot_3_to_0LSST[5] = {
		(struct Point){ .x = 0.0f, .y = 0.0f },
		(struct Point){ .x = -1.0f, .y = 0.0f },
		(struct Point){ .x = -1.0f, .y = -1.0f },
		(struct Point){ .x = 0.0f, .y = 2.0f },
		(struct Point){ .x = -1.0f, .y = 2.0f }
	};

	struct Point rot_0_to_3LSST[5] = {
		(struct Point){ .x = 0.0f, .y = 0.0f },
		(struct Point){ .x = 1.0f, .y = 0.0f },
		(struct Point){ .x = 1.0f, .y = 1.0f },
		(struct Point){ .x = 0.0f, .y = -2.0f },
		(struct Point){ .x = 1.0f, .y = -2.0f }
	};

	// Wall kicks for Long piece

	struct Point rot_0_to_1LONG[5] = {
		(struct Point){ .x = 0.0f, .y = 0.0f },
		(struct Point){ .x = -2.0f, .y = 0.0f },
		(struct Point){ .x = 1.0f, .y = 0.0f },
		(struct Point){ .x = -2.0f, .y = -1.0f },
		(struct Point){ .x = 1.0f, .y = 2.0f }
	};

	struct Point rot_1_to_0LONG[5] = {
		(struct Point){ .x = 0.0f, .y = 0.0f },
		(struct Point){ .x = 2.0f, .y = 0.0f },
		(struct Point){ .x = -1.0f, .y = 0.0f },
		(struct Point){ .x = 2.0f, .y = 1.0f },
		(struct Point){ .x = -1.0f, .y = -2.0f }
	};

	struct Point rot_1_to_2LONG[5] = {
		(struct Point){ .x = 0.0f, .y = 0.0f },
		(struct Point){ .x = -1.0f, .y = 0.0f },
		(struct Point){ .x = 2.0f, .y = 0.0f },
		(struct Point){ .x = -1.0f, .y = 2.0f },
		(struct Point){ .x = 2.0f, .y = -1.0f }
	};

	struct Point rot_2_to_1LONG[5] = {
		(struct Point){ .x = 0.0f, .y = 0.0f },
		(struct Point){ .x = 1.0f, .y = 0.0f },
		(struct Point){ .x = -2.0f, .y = 0.0f },
		(struct Point){ .x = 1.0f, .y = -2.0f },
		(struct Point){ .x = -2.0f, .y = 1.0f }
	};

	struct Point rot_2_to_3LONG[5] = {
		(struct Point){ .x = 0.0f, .y = 0.0f },
		(struct Point){ .x = 2.0f, .y = 0.0f },
		(struct Point){ .x = -1.0f, .y = 0.0f },
		(struct Point){ .x = 2.0f, .y = 1.0f },
		(struct Point){ .x = -1.0f, .y = -2.0f }
	};

	struct Point rot_3_to_2LONG[5] = {
		(struct Point){ .x = 0.0f, .y = 0.0f },
		(struct Point){ .x = -2.0f, .y = 0.0f },
		(struct Point){ .x = 1.0f, .y = 0.0f },
		(struct Point){ .x = -2.0f, .y = -1.0f },
		(struct Point){ .x = 1.0f, .y = 2.0f }
	};

	struct Point rot_3_to_0LONG[5] = {
		(struct Point){ .x = 0.0f, .y = 0.0f },
		(struct Point){ .x = 1.0f, .y = 0.0f },
		(struct Point){ .x = -2.0f, .y = 0.0f },
		(struct Point){ .x = 1.0f, .y = -2.0f },
		(struct Point){ .x = -2.0f, .y = 1.0f }
	};

	struct Point rot_0_to_3LONG[5] = {
		(struct Point){ .x = 0.0f, .y = 0.0f },
		(struct Point){ .x = -1.0f, .y = 0.0f },
		(struct Point){ .x = 2.0f, .y = 0.0f },
		(struct Point){ .x = -1.0f, .y = 2.0f },
		(struct Point){ .x = 2.0f, .y = -1.0f }
	};

	char dir = 'r';
	int original_rotation = shape->rotation;

	if(dir == 'l')
	{
		shape->rotation = (shape->rotation + 90) % 360;
	}
	else 
	{
		shape->rotation = (shape->rotation - 90) % 360;
	}

	if(shape->rotation < 0)
	{
		shape->rotation = shape->rotation + 360;
	}

	struct Point points[4];
	get_points(shape, points);

	if(shape->shape == LONG)
	{
		if(shape->rotation == 0)
		{
			if(dir == 'l')
			{
				if(check_and_transform(shape, points, rot_1_to_0LONG, board)) // this functions mutates the shape
				{
					return 1;
				}
			}
			else 
			{
				if(check_and_transform(shape, points, rot_3_to_0LONG, board)) // this functions mutates the shape
				{
					return 1;
				}
			}
		}
		else if(shape->rotation == 90)
		{
			if(dir == 'l')
			{
				if(check_and_transform(shape, points, rot_2_to_1LONG, board)) // this functions mutates the shape
				{
					return 1;
				}
			}
			else 
			{
				if(check_and_transform(shape, points, rot_0_to_1LONG, board)) // this functions mutates the shape
				{
					return 1;
				}
			}
		}
		else if(shape->rotation == 180)
		{
			if(dir == 'l')
			{
				if(check_and_transform(shape, points, rot_3_to_2LONG, board)) // this functions mutates the shape
				{
					return 1;
				}
			}
			else 
			{
				if(check_and_transform(shape, points, rot_1_to_2LONG, board)) // this functions mutates the shape
				{
					return 1;
				}
			}
		}
		else if(shape->rotation == 270)
		{
			if(dir == 'l')
			{
				if(check_and_transform(shape, points, rot_0_to_3LONG, board)) // this functions mutates the shape
				{
					return 1;
				}
			}
			else 
			{
				if(check_and_transform(shape, points, rot_2_to_3LONG, board)) // this functions mutates the shape
				{
					return 1;
				}
			}
		}
	}
	else 
	{
		if(shape->rotation == 0)
		{
			if(dir == 'l')
			{
				if(check_and_transform(shape, points, rot_1_to_0LSST, board)) // this functions mutates the shape
				{
					return 1;
				}
			}
			else 
			{
				if(check_and_transform(shape, points, rot_3_to_0LSST, board)) // this functions mutates the shape
				{
					return 1;
				}
			}
		}
		else if(shape->rotation == 90)
		{
			if(dir == 'l')
			{
				if(check_and_transform(shape, points, rot_2_to_1LSST, board)) // this functions mutates the shape
				{
					return 1;
				}
			}
			else 
			{
				if(check_and_transform(shape, points, rot_0_to_1LSST, board)) // this functions mutates the shape
				{
					return 1;
				}
			}
		}
		else if(shape->rotation == 180)
		{
			if(dir == 'l')
			{
				if(check_and_transform(shape, points, rot_3_to_2LSST, board)) // this functions mutates the shape
				{
					return 1;
				}
			}
			else 
			{
				if(check_and_transform(shape, points, rot_1_to_2LSST, board)) // this functions mutates the shape
				{
					return 1;
				}
			}
		}
		else if(shape->rotation == 270)
		{
			if(dir == 'l')
			{
				if(check_and_transform(shape, points, rot_0_to_3LSST, board)) // this functions mutates the shape
				{
					return 1;
				}
			}
			else 
			{
				if(check_and_transform(shape, points, rot_2_to_3LSST, board)) // this functions mutates the shape
				{
					return 1;
				}
			}
		}

	}

	shape->rotation = original_rotation;

	return 0;
}

int check_row(enum Colors board[], int y)
{
	int y_to_i = y * BOARD_WIDTH;

	for(int x = 0; x < BOARD_WIDTH; x++)
	{
		if(board[y_to_i + x] == NONE)
		{
			return 0;
		}
	}

	return 1;
}

void remove_row(enum Colors board[], int y_to_remove)
{
	for(int y = y_to_remove; y < BOARD_HEIGHT - 1; y++)
	{
		for(int x = 0; x < BOARD_WIDTH; x++)
		{
			int row_current_i = x + (y * BOARD_WIDTH);
			int row_above_i = x + ((y + 1) * BOARD_WIDTH);

			board[row_current_i] = board[row_above_i];
		}
	}

	for(int x = 0; x < BOARD_WIDTH; x++)
	{
		int i = x + ((BOARD_HEIGHT - 1) * BOARD_WIDTH);

		board[i] = NONE;
	}
}

void remove_rows(enum Colors board[], GLfloat vertices[], size_t *v_i, GLuint indices[], size_t *i_i)
{
	for(int y = BOARD_HEIGHT - 1; y >= 0; y--)
	{
		if(check_row(board, y))
		{
			remove_row(board, y);
		}
	}

	*v_i = 0;
	*i_i = 0;

	for(int x = 0; x < BOARD_WIDTH; x++)
	{
		for(int y = 0; y < BOARD_HEIGHT; y++)
		{
			int i = x + (y * BOARD_WIDTH);

			struct Color color;

			if(board[i] != NONE)
			{
				switch (board[i])
				{
					case LIGHT_BLUE:
						color = COLOR_LIGHT_BLUE;
						break;
					case BLUE:
						color = COLOR_BLUE;
						break;
					case ORANGE:
						color = COLOR_ORANGE;
						break;
					case YELLOW:
						color = COLOR_YELLOW;
						break;
					case GREEN:
						color = COLOR_GREEN;
						break;
					case PURPLE:
						color = COLOR_PURPLE;
						break;
					case RED:
						color = COLOR_RED;
						break;
					default:
						printf("ERROR: unknown color\n");
						abort();
						break;
				}

				add_quad(x, y, color, vertices, v_i, indices, i_i);
			}
		}
	}

}

int main(void)
{
	srand(time(NULL));
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Tetris", NULL, NULL);

	int true_width, true_height;
	glfwGetFramebufferSize(window, &true_width, &true_height);

    if(window == NULL)
    {
        printf("Failed to create GLFW window\n");
        glfwTerminate();

        return -1;
    }

    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to initialize GLAD\n");
        
        return -1;
    }

    gladLoadGL();
    glViewport(0, 0, true_width, true_height);
    
	Shaders *shaders = init_shaders("shader/default.vert", "shader/default.frag");

	// 10 x 20 board, 4 vertices for each square
	enum Colors board[BOARD_WIDTH * BOARD_HEIGHT] = {NONE};

    GLfloat vertices[BOARD_WIDTH * BOARD_HEIGHT * QUAD_VERTICES * VERTEX_ELEMENTS];
	GLuint indices[BOARD_WIDTH * BOARD_HEIGHT * QUAD_INDICES];

	size_t VBO_len = 0;
	size_t EBO_len = 0;

	struct ActiveShape shape;
	generate_new_active_shape(&shape);
	add_active_shape(&shape, vertices, &VBO_len, indices, &EBO_len);

	init_VAO(shaders);
	bind_VAO(*shaders);

	init_VBO(shaders, vertices, sizeof(vertices));
	init_EBO(shaders, indices, sizeof(indices));

	link_attribute(*shaders, 0, 3, GL_FLOAT, VERTEX_ELEMENTS * sizeof(float), (void *)0);
	link_attribute(*shaders, 1, 3, GL_FLOAT, VERTEX_ELEMENTS * sizeof(float), (void *)(3 * sizeof(float)));

	unbind_VAO(*shaders);
	unbind_VBO(*shaders);
	unbind_EBO(*shaders);

	int game_over = 0;

	int lock_delay_delta = 0;
	int lock_delay_rotations = 0;
	int drop_frame_delta = 0;
	int drop_delay = INITIAL_AUTO_DROP_DELAY;

	double t_ahead = glfwGetTime();
	double t_behind = 0.0f;

	struct KeyState up = { .is_active = INACTIVE, .mode = RELEASED, .frame_delta = 0 };
	struct KeyState down = { .is_active = INACTIVE, .mode = RELEASED, .frame_delta = 0 };
	struct KeyState left = { .is_active = INACTIVE, .mode = RELEASED, .frame_delta = 0 };
	struct KeyState right = { .is_active = INACTIVE, .mode = RELEASED, .frame_delta = 0 };

    while(!glfwWindowShouldClose(window) && !game_over)
    {
		if(t_ahead - t_behind >= 1.0f / 60.0f)
		{
			struct Color color = COLOR_NONE;

			glClearColor(color.r, color.g, color.b, color.a);
			glClear(GL_COLOR_BUFFER_BIT);

			if(up.is_active == ACTIVE) 
			{
				if(rotate(&shape, board)) // this function alters the state
				{
					lock_delay_rotations++;
				}
				else 
				{
					lock_delay_rotations = LOCK_DELAY_LIMIT;
				}

				if(lock_delay_rotations <= LOCK_DELAY_LIMIT)
				{
					lock_delay_delta = 0;
				}
			}
			if(down.is_active == ACTIVE) 
			{
				if(!check_bottom_collision(&shape, board))
				{
					shape.y -= 1;
				}
				else 
				{
					lock_delay_delta = LOCK_DELAY;
				}
			}
			if(left.is_active == ACTIVE) 
			{
				if(!check_side_collision(&shape, board, -1))
				{
					shape.x -= 1;
				}
			}
			if(right.is_active == ACTIVE) 
			{
				if(!check_side_collision(&shape, board, 1))
				{
					shape.x += 1;
				}
			}

			if(drop_frame_delta >= drop_delay)
			{
				if(check_bottom_collision(&shape, board))
				{
					if(lock_delay_delta >= LOCK_DELAY)
					{
						struct Point points[4];
						get_points(&shape, points);

						for(int i = 0; i < 4; i++)
						{
							int board_index = (shape.x + (int)points[i].x) + ((shape.y + (int)points[i].y) * BOARD_WIDTH);
							board[board_index] = shape.color;

							if(shape.y + points[i].y >= BOARD_HEIGHT)
							{
								game_over = 1;
							}
						}

						if(!game_over)
						{
							remove_rows(board, vertices, &VBO_len, indices, &EBO_len);

							generate_new_active_shape(&shape);
							add_active_shape(&shape, vertices, &VBO_len, indices, &EBO_len);
						}
					}
				}
				else 
				{
					drop_frame_delta = 0;
					lock_delay_delta = 0;
					lock_delay_rotations = 0;
					shape.y--;
				}
			}
	
			if(!game_over)
			{
				VBO_len -= 4 * QUAD_VERTICES;
				EBO_len -= 4 * QUAD_INDICES;

				add_active_shape(&shape, vertices, &VBO_len, indices, &EBO_len);
				bind_VBO(*shaders);

				glBufferSubData(GL_ARRAY_BUFFER, 0, VBO_len * VERTEX_ELEMENTS * sizeof(GLfloat), vertices);
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, EBO_len * sizeof(GLuint), indices);

				unbind_VBO(*shaders);

				activate_shaders(*shaders);

				bind_VAO(*shaders);
				glDrawElements(GL_TRIANGLES, EBO_len, GL_UNSIGNED_INT, 0);

				glfwSwapBuffers(window);
				glfwPollEvents();

				t_behind = t_ahead;

				manage_key_state(&up, window, GLFW_KEY_UP, 0);
				manage_key_state(&down, window, GLFW_KEY_DOWN, 1);
				manage_key_state(&left, window, GLFW_KEY_LEFT, 1);
				manage_key_state(&right, window, GLFW_KEY_RIGHT, 1);

				up.frame_delta++;
				down.frame_delta++;
				left.frame_delta++;
				right.frame_delta++;
				lock_delay_delta++;

				drop_frame_delta++;
			}
		}

		t_ahead = glfwGetTime();
    }


	delete_VAO(shaders);
	delete_VBO(shaders);
	delete_EBO(shaders);

	delete_shaders(*shaders);

    glfwTerminate();
    return 0;
}
