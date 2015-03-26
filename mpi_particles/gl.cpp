//
// Simple GL example
//

#include "Geometry.hpp"
#include "Textures.hpp"
#include "Input.hpp"
#include "View.hpp"
#include "Generic.hpp"
#include "Sphere.hpp"
#include "Vec.inl"

#include <stdio.h>
#include <assert.h>
#include <map>

#include "gl.h"
#include "run.h"
#include "common.h"

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
	GLFWwindow *win = glfwCreateWindow(800, 800, "OpenGL Demo", 0, 0);
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
		if(verbose)
			printf("starting malloc\n");
		fflush(stdout);
        // out of space, or zero
        void *temp = (void *) malloc ((*num_items + batch_size) * item_size);
		memset(temp, 0, ((*num_items + batch_size) * item_size));
        memcpy(temp, *items, *num_items * item_size);
		if(*items){
        	free(*items);
		}
        (*items) = temp;
        (*space_for_items) = *num_items + batch_size;
		
		if(verbose)
	        printf("Now room for %i items, stored: %i\n", *space_for_items, *num_items);
		fflush(stdout);
    }
}

int read_input(bool verbose, FILE *fp, Vec<3> **points, int *num_particles, double *radius, double *size){
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    short temp_str_len = 50;
    char first_term[temp_str_len];
	char first_letter[temp_str_len];
	memset(&first_letter, 0, temp_str_len);
    
    if(fp == NULL){
        fprintf(stderr, "Error reading file.\n");
        exit(1);
    }
	
    unsigned int batch_malloc_size = *num_particles;
	unsigned int space_for_points = *num_particles;
	double x,y = 0;
	
	int num_points = 0;
	// get space for one to start

	while((read = getline(&line, &len, fp)) != -1){
		if(verbose)
        	printf("read line of length %zu: %s\n", read, line);
        first_term[0] = 0;
		first_letter[0] = 0;
		
        sscanf(line, "%s", first_term);
		
		sscanf(line, "%c", first_letter);
		
        if(str_equals("n",first_letter)){
			if(verbose)
				printf("Checking n %s\n", first_term);
			
            sscanf(line, "n %u\n", num_particles);
			fprintf(stderr,"got num particles: %u\n", *num_particles);
			batch_malloc_size = *num_particles;
		}else if(str_equals("r",first_letter)){
			if(verbose)
				printf("Checking r %s\n", first_term);
			
            sscanf(line, "r %lf\n", radius);
			printf("radius: %lf\n", *radius);
		}else if(str_equals("s",first_letter)){
			if(verbose)
				printf("Checking s %s\n", first_term);
			
            sscanf(line, "s %lf\n", size);
			fprintf(stderr,"size: %lf\n", *size);
	
		}else{
			//printf("0 %u %u %x\n", space_for_points, *num_points, *points);
			get_space_for_items(&space_for_points, &num_points, (void **) points, sizeof(Vec<3>), batch_malloc_size, verbose);
			sscanf(line, "%lf %lf\n", &x, &y );
			(*points)[num_points].x = x;
			(*points)[num_points].y = y;
			//printf("stored point %i as %lf %lf\n",num_points, (*points)[num_points].x,(*points)[num_points].y);
			num_points++;
			
			if(num_points == *num_particles){
				break;
			}
		}
		if(verbose)
			printf("Finished with line.\n");
	}
	if(verbose){
		printf("Done with read\n");
		fflush(stdout);
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

void scale_points(Vec<3> *points, int num_particles, double *scale, double *radius, double size, bool first_run){
	
	// do some processing on the particles
	//printf("x range: (%lf,%lf), y range: (%lf,%lf)\n", min_x, max_x, min_y, max_y);
	double x_width = size * 0.5;
	double y_width = size * 0.5;
	int i = 0;
	
	if(first_run){
		double desired_height = 150.0;
		(*scale) = desired_height / x_width;
		// This math is kinda hacked, but it gets good results
		(*radius) = 0.5 * (*radius) * (*scale);
		//printf("new radius: %lf\n",(*radius));
	}
	
	for(i = 0; i < num_particles; i++){
		points[i].x -= x_width;
		points[i].y -= y_width;
		points[i].x *= (*scale);
		points[i].y *= (*scale);
	}
	
	//printf("new min max: (%lf, %lf) (%lf, %lf)\n", min_x, min_y, max_x, max_y);
	
}

int draw_data(FILE *fp, bool from_stdin){

	GLFWwindow *win = initGLFW();
	assert(win);				// window must exist
	
	AppContext appctx(win);
	Textures tex;
	
	
	Vec<3> *points = 0;
	double radius = 0.01;
	int num_particles = 0;
	int points_read = 0;
	double size = 0;
	points_read = read_input(false, fp, &points, &num_particles, &radius, &size);
	if(points_read < num_particles){
		fprintf(stderr,"Out of points\n");
		return 0;
	}

	double scale = 1;
	scale_points(points, num_particles, &scale, &radius, size, true);
	
	Vec<3> current_points[num_particles];
	unsigned int sphere_draw_ids[num_particles];
	unsigned int i = 0;
	
	
	for(i = 0; i < num_particles; i++){
		Sphere sph1(appctx.geom, tex, radius, Vec3(points[i].x, points[i].y, 1) , 1);
		sphere_draw_ids[i] = sph1.drawID;
		current_points[i].x = points[i].x;
		current_points[i].y = points[i].y;
	}
	
	int fps_seconds = 5;
	double now = glfwGetTime();
	double duration = 0;
	
	appctx.geom.finalizeDrawData();
	
	appctx.view.update(win);
	
	bool never_drawn = true;
	unsigned int frames = 0;

	// loop until GLFW says it's time to quit
	while (!glfwWindowShouldClose(win)) {
		// check for updates while a key is pressed
	
		// get new points to draw
		points_read = read_input(false, fp, &points, &num_particles, &radius, &size);
		if(points_read < num_particles){
			fprintf(stderr, "Out of points\n");
			break;
		}
		scale_points(points, num_particles, &scale, &radius, size, false);
		
		appctx.redraw = true;
		for(i = 0; i < num_particles; i++){
		
			Xform &xform = appctx.geom.getModelUniforms( sphere_draw_ids[i] ).modelMats;
			Vec<3> m = Vec3(points[i].x, points[i].y, 0);
			m.xy = m.xy - current_points[i].xy;
			current_points[i].xy = points[i].xy;
			xform = xform * Xform::translate( m );
		}
		
		duration = glfwGetTime() - now;
		if(duration > fps_seconds){
			fprintf(stderr,"FPS:\t%lf\n", frames / duration);
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
		}
		frames++;
		glfwPollEvents();		// wait for user input
	}

	glfwDestroyWindow(win);
	glfwTerminate();
	
	return 0;
}

