//
// Simple GL example
//

#include "Geometry.hpp"
#include "Textures.hpp"
#include "Input.hpp"
#include "View.hpp"
#include "Generic.hpp"
#include "Sphere.hpp"
#include "MultiSphere.hpp"
#include "Vec.inl"

#include <stdio.h>
#include <assert.h>
#include <map>

#include "gl.h"
#include "run.h"
#include "common.h"

#define Z_AXIS_DEPTH 1

///////
// GLFW callbacks must use extern "C"
extern "C" {
	// called for GLFW error
	void winError(int error, const char *description) {
		fprintf(stderr, "GLFW error: %s\n", description);
	}

	// called whenever the window size changes
	void reshape(GLFWwindow *win, int width, int height) {
		AppContext *appctx = (AppContext*)glfwGetWindowUserPointer(win);
		appctx->view.update(win);
		appctx->redraw = true;
	}

	// called when mouse button is pressed
	void mousePress(GLFWwindow *win, int button, int action, int mods) {
		AppContext *appctx = (AppContext*)glfwGetWindowUserPointer(win);
		appctx->redraw |= appctx->input.mousePress(*win, button, action);
	}

	// called when mouse is moved
	void mouseMove(GLFWwindow *win, double x, double y) {
		AppContext *appctx = (AppContext*)glfwGetWindowUserPointer(win);
		appctx->redraw |= appctx->input.mouseMove(appctx->view, *win, x,y);
	}

	// called on any key press or release
	void keyPress(GLFWwindow *win, int key, int scancode, int action, int mods) {
		AppContext *appctx = (AppContext*)glfwGetWindowUserPointer(win);
		appctx->redraw |= appctx->input.keyPress(appctx->geom, *win, key, action, mods);
	}
}


// initialize GLFW - windows and interaction
GLFWwindow *initGLFW() {
	// set error callback before init
	glfwSetErrorCallback(winError);
	if (! glfwInit()){
					printf("returning zero\n");fflush(stdout);
		return 0;
	}

	// OpenGL version: YOU MAY NEED TO ADJUST VERSION OR OPTIONS!
	// When figuring out the settings that will work for you, make
	// sure you can see error messages on console output.
	//
	// My driver needs FORWARD_COMPAT, but others will need to comment that out.
	// Likely changes for other versions:
	//	 OpenGL 4.0 (2010): configured for this already
	//	 OpenGL 3.3 (2010): change VERSION_MAJOR to 3, MINOR to 3
	//	   change "400 core" to "330 core" in the .vert and .frag files
	//	 OpenGL 3.2 (2009): change VERSION_MAJOR to 3, MINOR to 2
	//	   change "400 core" to "150 core" in the .vert and .frag files
	//	 OpenGL 3.1 (2009): change VERSION_MAJOR to 3, MINOR to 1
	//	   comment out PROFILE line
	//	   change "400 core" to "140" in the .vert and .frag files
	//	 OpenGL 3.0 (2008): does not support features we need
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	// create window
	GLFWwindow *win = glfwCreateWindow(800, 800, "OpenGL Viz", 0, 0);
	if (! win) {
		glfwTerminate();
		return 0;
	}
	glfwMakeContextCurrent(win);

	// GLEW (GL Extension Wrangler) eases access to modern OpenGL functions
	glewExperimental = true;
	glewInit();

	// set functions to be called by GLFW when events happen
	glfwSetFramebufferSizeCallback(win, reshape);
	glfwSetKeyCallback(win, keyPress);
	glfwSetMouseButtonCallback(win, mousePress);
	glfwSetCursorPosCallback(win, mouseMove);

	return win;
}

bool str_equals(char *a, char *b){
    return strcmp(a, b) == 0;
}

void get_space_for_items(unsigned int *space_for_items, int *num_items, void **items, unsigned int item_size, unsigned int batch_size, bool verbose){
    
    if(*space_for_items == 0 || *space_for_items == *num_items){
        // out of space, or zero
        void *temp = (void *) malloc ((*num_items + batch_size) * item_size);
		memset(temp, 0, ((*num_items + batch_size) * item_size));
        memcpy(temp, *items, *num_items * item_size);
		if(*items){
        	free(*items);
		}
        (*items) = temp;
        (*space_for_items) = *num_items + batch_size;
    }
}

int read_input(bool verbose, FILE *fp, Vec<3> **points, int *num_particles, double *radius, double *size, unsigned int *actual_size, Vec<3> **colors){
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    short temp_str_len = 50;
    char first_term[temp_str_len];
	char first_letter[temp_str_len];
	memset(&first_letter, 0, temp_str_len);
    
    if(fp == NULL){
        fprintf(stderr, "%s Error reading file.\n", VIZ_PREPEND);
        exit(1);
    }
	
    unsigned int batch_malloc_size = *num_particles;
	unsigned int space_for_points = *num_particles;
	double x,y = 0;
	
	int num_points = 0;
	// get space for one to start
	unsigned int t;
	Vec<3> t_color;

	while((read = getline(&line, &len, fp)) != -1){
        first_term[0] = 0;
		first_letter[0] = 0;
		
		sscanf(line, "%s", first_term);
		
		sscanf(line, "%c", first_letter);
		
		//fprintf(stderr, "%s got line: %s\n", VIZ_PREPEND, line);
		
		if(str_equals("n",first_letter)){
            sscanf(line, "n %u\n", num_particles);
			fprintf(stderr,"%s got num particles: %u\n",VIZ_PREPEND, *num_particles);
			batch_malloc_size = *num_particles;
			
			if(*colors){
				free(*colors);
			}
			(*colors) = (Vec<3> *) malloc (*num_particles * sizeof(Vec<4>));
			for(int i = 0; i < *num_particles; i++){
				(*colors)[i] = Vec3(0,0,0);
			}
			
		}else if(str_equals("r",first_letter)){
            sscanf(line, "r %lf\n", radius);
			fprintf(stderr,"%s radius: %lf\n", VIZ_PREPEND, *radius);
		}else if(str_equals("s",first_letter)){
			sscanf(line, "s %lf\n", size);
			fprintf(stderr,"%s size: %lf\n", VIZ_PREPEND, *size);
		}else if(str_equals("a",first_letter)){
			sscanf(line, "a %u\n", actual_size);
			fprintf(stderr,"%s actual_size: %u\n", VIZ_PREPEND, *actual_size);
		}else if(str_equals("c", first_letter)){
			sscanf(line, "c %u %f %f %f\n", &t, &t_color.x,&t_color.y,&t_color.z);
			(*colors)[t] = Vec3(t_color.x, t_color.y, t_color.z);
			//fprintf(stderr, "%s got color for particle %u (%f,%f,%f)\n", VIZ_PREPEND, t, (*colors)[t].x,(*colors)[t].y,(*colors)[t].z);
		}else if(str_equals("p",first_letter)){
			get_space_for_items(&space_for_points, &num_points, (void **) points, sizeof(Vec<3>), batch_malloc_size, verbose);
			sscanf(line, "p %lf %lf\n", &x, &y );
			(*points)[num_points].x = x;
			(*points)[num_points].y = y;
			(*points)[num_points].z = Z_AXIS_DEPTH;
			num_points++;
			
			if(num_points == *num_particles){
				break;
			}
		}else{
			fprintf(stderr, "%s got unhandled line: %s\n", VIZ_PREPEND, line);
		}

	}

	return num_points;
}

bool poll_input(GLFWwindow *win, AppContext *appctx){
	glfwPollEvents();		// wait for user input
	appctx->input.keyUpdate(appctx->geom, *win);
	if(glfwWindowShouldClose(win)){
		
		return true;
	}
	return false;
}

void scale_points(Vec<3> *points, int num_particles, double *scale, double *radius, double size, unsigned int actual_size, bool first_run){
	
	int i;
	
	// center around origin
	for(i = 0; i < num_particles; i++){
		points[i].x -= 0.5 * size;
		points[i].y -= 0.5 * size;
	}
	
	if(first_run){
		// This math is kinda hacked, but it gets good results
		double desired_height = 400;
		(*scale) = desired_height; // / actual_size;
		(*radius) = 0.5 * (*radius) * (*scale);
		//printf("new radius: %lf\n",(*radius));
		printf("size: %lf, scale: %lf, actual: %u, radius: %lf\n", size, *scale, actual_size, *radius);
	}
	
	for(i = 0; i < num_particles; i++){
		points[i].x *= (*scale);
		points[i].y *= (*scale);
	}
	
}

void update_geom_color(Geometry::Vertex *verts, unsigned int num_verts, Vec<3> colors){
	for(int i = 0; i < num_verts; i++){
		verts[i].color = colors;
	}
}

int draw_data(FILE *fp, bool from_stdin, unsigned int frame_skip, struct map *map_cfg){

	GLFWwindow *win = initGLFW();
	assert(win);				// window must exist
	
	
	Vec<3> *points = 0;
	Vec<3> *colors = 0;
	double radius = 0.01;
	int num_particles = 0;
	int points_read = 0;
	double size = 0;
	unsigned int actual_size = 0;
	points_read = read_input(false, fp, &points, &num_particles, &radius, &size, &actual_size, &colors);
	if(points_read < num_particles){
		fprintf(stderr,"%s Out of points\n", VIZ_PREPEND);
		return 0;
	}

	double scale = 1;
	scale_points(points, num_particles, &scale, &radius, size, actual_size, true);
	
	Vec<3> current_points[num_particles];
	unsigned int i = 0;
	
	
	double distance = 500;
	
	AppContext appctx(win, distance);
	Textures tex;
	
	// MultiSphere(class Geometry &geom, class Textures &tex, float radius, unsigned int num_spheres)
	MultiSphere spheres(appctx.geom, tex, radius, num_particles);
	unsigned int spheres_draw_id = spheres.drawID;
	unsigned int verts_per_sphere = spheres.num_verts_per_sphere;
	
	Geometry::Vertex *verts = appctx.geom.getVertices(spheres_draw_id);
	Vec<3> dxdydz;
	unsigned int so_far = 0;
	for(i = 0; i < num_particles; i++){
		
		dxdydz = Vec3(0,0,0) - points[i];
		for(int j = 0; j < verts_per_sphere; j++){
			verts[so_far].pos.xy = verts[so_far].pos.xy - dxdydz.xy;
			so_far++;
		}
		current_points[i] = points[i];
		update_geom_color(&(verts[verts_per_sphere * i]), verts_per_sphere, colors[i]);
	}
	
	// draw walls: Not done yet.
	
	
	
	int fps_seconds = 5;
	double now = glfwGetTime();
	double duration = 0;
	
	appctx.geom.updateShaders();
	appctx.geom.finalizeDrawData();
	
	appctx.view.update(win);
	
	bool never_drawn = true;
	unsigned int frames = 0;
	unsigned int since_last_draw = 0;
	
	// loop until GLFW says it's time to quit
	while (!glfwWindowShouldClose(win)) {
		since_last_draw++;
		// check for updates while a key is pressed
	
		// get new points to draw
		points_read = read_input(false, fp, &points, &num_particles, &radius, &size, &actual_size, &colors);
		if(points_read < num_particles){
			fprintf(stderr, "%s Out of points\n", VIZ_PREPEND);
			break;
		}
		
		
		if(!frame_skip || since_last_draw >= frame_skip){
			
			
			scale_points(points, num_particles, &scale, &radius, size, actual_size, false);
			appctx.redraw = true;
			so_far = 0;
			for(i = 0; i < num_particles; i++){
				
				dxdydz = current_points[i] - points[i];
				for(int j = 0; j < verts_per_sphere; j++){
					verts[so_far].pos.xy = verts[so_far].pos.xy - dxdydz.xy;
					so_far++;
				}
				current_points[i] = points[i];
			}
			
			appctx.geom.finalizeDrawData();
		}
		
			
		duration = glfwGetTime() - now;
		if(duration > fps_seconds){
			fprintf(stderr,"%s FPS:\t%lf\n", VIZ_PREPEND, frames / duration);
			frames = 0;
			now = glfwGetTime();
		}
		
		appctx.redraw |= appctx.input.keyUpdate(appctx.geom, *win);
		
		// do we need to redraw?
		if (appctx.redraw) {
			appctx.redraw = false; // don't draw again until something changes
			
			// clear old screen contents then draw
			glClearColor(1.f, 1.f, 1.f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			appctx.geom.draw();
			glfwSwapBuffers(win);
			since_last_draw = 0;
			frames++;
		}
		glfwPollEvents();		// wait for user input
	}

	glfwDestroyWindow(win);
	glfwTerminate();
	
	return 0;
}

