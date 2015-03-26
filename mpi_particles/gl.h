#ifndef GL_H__
#define GL_H__

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Geometry.hpp"
#include "View.hpp"
#include "Input.hpp"
#include "Textures.hpp"
#include "Vec.inl"



#define MIN(a,b) (a > b ? b : a)
#define MAX(a,b) (a > b ? a : b)




// collected state for access in callbacks
struct AppContext {
	Geometry geom;				  // drawing data
	View view;					  // viewing data
	Input input;				  // user interface data
	bool redraw;				  // true if we need to redraw

	AppContext(GLFWwindow *win) : view(win) {
		redraw = true;
		
		// store context pointer to access application data from callbacks
		if(win){
			glfwSetWindowUserPointer(win, this);
		}
	}
};


int draw_data(FILE *fp, bool from_stdin);

bool str_equals(char *a, char *b);

void setup_gl(GLFWwindow **win);

void initialize_spheres_and_gl(GLFWwindow *win, Textures tex, AppContext *appctx, unsigned int **sphere_draw_ids, Vec<3> *points, int num_particles, double radius, Vec<3> **current_points);

void redraw_spheres(AppContext *appctx, GLFWwindow *win, Vec<3> *points, int num_particles, unsigned int *sphere_draw_ids, Vec<3> **current_points);

bool poll_input(GLFWwindow *win, AppContext *appctx);

#endif
