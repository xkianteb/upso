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

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <assert.h>
#include <map>

#include "run.h"

#define MIN(a,b) (a > b ? b : a)
#define MAX(a,b) (a > b ? a : b)

#define SCALAR 1.0
#define POINT_SCALAR 1.0
#define DRAW_DELAY 0.1

// collected state for access in callbacks
struct AppContext {
	Geometry geom;				  // drawing data
	View view;					  // viewing data
	Input input;				  // user interface data
	bool redraw;				  // true if we need to redraw

	AppContext(GLFWwindow *win) : view(win) {
		redraw = true;

		// store context pointer to access application data from callbacks
		glfwSetWindowUserPointer(win, this);
	}
};

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
	if (! glfwInit()) return 0;

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

void process_args(int argc, char **argv, bool *stdin, bool *show_usage, int *file_name_index, double *speed){
    if(argc < 2){
        return;
    }
    (*stdin) = strcmp("-", argv[1]) == 0;
    int i = 0;
    bool error = false;
    // skip ./trace
    for(i = 1; i < argc; i++){
        if(strcmp("-f", argv[i]) == 0){
            if(i+1 < argc){
                (*file_name_index) = i+1;
            }else{
                error = true;
            }
        }else if(strcmp("-s", argv[i]) == 0){
            if(i+1 < argc){
                sscanf(argv[i+1], "%lf", speed);
            }else{
                error = true;
			}
		}else if(strcmp("-h", argv[i]) == 0){
			(*show_usage) = true;
			return;
		}
    }
    if(error){
        fprintf(stderr, "invalid arguments\n");
        usage();
        exit(1);
    }
}

bool str_equals(char *a, char *b){
    return strcmp(a, b) == 0;
}

void get_space_for_items(unsigned int *space_for_items, unsigned int *num_items, void **items, unsigned int item_size, unsigned int batch_size, bool verbose){
    
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

void read_input(bool verbose, FILE *fp, AppContext *appctx, Textures tex, Vec<3> **points, unsigned int *num_points, unsigned int *num_particles, double *min_x, double *max_x, double *min_y, double *max_y, double *density){
    
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
    
    unsigned int batch_malloc_size = 100;
	unsigned int space_for_points = 0;
	double x,y = 0;
	int values_read = 0;
	
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
			printf("got num particles: %u\n", *num_particles);
			
		}else if(str_equals("d",first_letter)){
			if(verbose)
				printf("Checking d %s\n", first_term);
			
            sscanf(line, "d %lf\n", density);
			printf("density: %lf\n", *density);
	
		}else{
			//printf("0 %u %u %x\n", space_for_points, *num_points, *points);
			get_space_for_items(&space_for_points, num_points, (void **) points, sizeof(Vec<3>), batch_malloc_size, verbose);
			//printf("1 %u %u %x\n", space_for_points, *num_points, *points);
			//fflush(stdout);
			sscanf(line, "%lf %lf\n", &x, &y );
			(*points)[*num_points] = Vec3(x, y, 1) * POINT_SCALAR;
			(*min_x) = MIN( *min_x, (*points)[*num_points].x );
			(*max_x) = MAX( *max_x, (*points)[*num_points].x );
			(*min_y) = MIN( *min_y, (*points)[*num_points].y );
			(*max_y) = MAX( *max_y, (*points)[*num_points].y );
			//printf("2\n");
			//fflush(stdout);
			//printf("added point %lf %lf\n",(*points)[*num_points].x, (*points)[*num_points].y);
			//fflush(stdout);
			(*num_points)++;
		}
		if(verbose)
			printf("Finished with line.\n");
	}
	if(verbose){
		printf("Done with initial read\n");
		fflush(stdout);
	}
	
}
/*

int main(int argc, char *argv[]) {
	
	bool read_stdin = false;
    int file_name_index = -1;
	bool show_usage = false;
	bool verbose = false;
	unsigned int base_path_size = 256;
	char *base_path = (char *) malloc(base_path_size * sizeof(char));
	char *full_read_file = (char *) malloc(base_path_size * sizeof(char));
	memset(base_path, 0, base_path_size * sizeof(char));
	memset(full_read_file, 0, base_path_size * sizeof(char));
	bool interactive = false;
	unsigned int points_used_so_far = 0;
	double speed = 10.0;
	double min_x = INT_MAX;
	double max_x = INT_MIN;
	double min_y = INT_MAX;
	double max_y = INT_MIN;
	double density = 0;
	
	// set up window
	GLFWwindow *win = initGLFW();
	assert(win);				// window must exist

	// drawing infrastructure
	AppContext appctx(win);
	Textures tex;
	
    if(argc > 1){
        process_args(argc, argv, &read_stdin, &show_usage, &file_name_index, &speed);
    }

	if(show_usage || (interactive && file_name_index == -1)){
		usage();
		exit(0);
	}
    
    FILE *read_pointer = NULL;
    
    if(read_stdin){
        fprintf(stderr, "Reading from stdin\n");
        read_pointer = stdin;
	}else if(file_name_index > -1){
		
		memcpy(full_read_file, base_path, strlen(base_path) * sizeof(char));
		memcpy(&full_read_file[strlen(base_path)], argv[file_name_index], strlen(argv[file_name_index]) * sizeof(char));
		
        fprintf(stderr, "reading from %s\n", full_read_file);
        read_pointer = fopen(full_read_file, "r");
    }else{
		usage();
		exit(0);
	}
	
    Vec<3> *points = 0;
	unsigned int num_points = 0;
	unsigned int num_particles = 0;
    
	read_input(verbose, read_pointer, &appctx, tex, &points, &num_points, &num_particles, &min_x, &max_x, &min_y, &max_y, &density);
	printf("Finished reading file\n");
	fflush(stdout);

	if(!read_stdin && read_pointer != stdin){
		fclose(read_pointer);
	}

	//printf("Read %u verts, %u texture_coords, %u norms, %u faces\n", num_vertices, num_texture_coords, num_normal_lines, num_faces);
	printf("Read %u points\n", num_points);
	
	unsigned int i = 0;
	
	// do some processing on the points
	printf("x range: (%lf,%lf), y range: (%lf,%lf)\n", min_x, max_x, min_y, max_y);
	double x_width = min_x + (max_x - min_x) * 0.5;
	max_x -= x_width;
	min_x -= x_width;
	double y_width = min_y + (max_y - min_y) * 0.5;
	max_y -= y_width;
	min_y -= y_width;
	
	double desired_height = 150.0;
	double scale = max_x > max_y ? desired_height / max_x : desired_height / max_y;
	printf("scale: %lf\n",scale);
	
	for(i = 0; i < num_points; i++){
		points[i].x -= x_width;
		points[i].y -= y_width;
		points[i].x *= scale;
		points[i].y *= scale;
		max_x = MAX(max_x, points[i].x);
		max_y = MAX(max_y, points[i].y);
		min_x = MIN(min_x, points[i].x);
		min_y = MIN(min_y, points[i].y);
	}
	
	Vec<3> current_points[num_particles];
	unsigned int sphere_draw_ids[num_particles];
	
	double radius = 2;
	//printf("radius: %lf\n",radius);
	
	for(points_used_so_far = 0; points_used_so_far < num_particles; points_used_so_far++){
		Sphere sph1(appctx.geom, tex, radius, points[points_used_so_far], SCALAR);
		sphere_draw_ids[points_used_so_far] = sph1.drawID;
		current_points[points_used_so_far].x = points[points_used_so_far].x;
		current_points[points_used_so_far].y = points[points_used_so_far].y;
		//printf("added sphere %u %u at (%f,%f)\n", points_used_so_far, sphere_draw_ids[points_used_so_far], points[points_used_so_far].x,points[points_used_so_far].y);
	}
	
	double now = glfwGetTime();
	
	appctx.geom.finalizeDrawData();
	
	appctx.view.update(win);
	

	// loop until GLFW says it's time to quit
	while (!glfwWindowShouldClose(win)) {
		// check for updates while a key is pressed
		
		if(glfwGetTime() - now > DRAW_DELAY){
			appctx.redraw = true;
			
			for(i = 0; i < num_particles && points_used_so_far < num_points; ){
			
				Xform &xform = appctx.geom.getModelUniforms( sphere_draw_ids[i] ).modelMats;
				Vec<3> m = Vec3(points[points_used_so_far].x, points[points_used_so_far].y, 0);
				//printf("drawid %u, prior: %lf, %lf\t\t%lf %lf\n", sphere_draw_ids[i], current_points[i].x,current_points[i].y, m.x,m.y);
				m.xy = m.xy - current_points[i].xy;
				current_points[i].xy = points[points_used_so_far].xy;
				//printf("\tafter: %lf, %lf\n", m.x, m.y);
				xform = xform * Xform::translate( m );
				
				i++;
				(points_used_so_far)++;
			}
			if(points_used_so_far >= num_points){
				printf("Drawn all %u points, restarting\n", num_points);
				points_used_so_far = 0;
			}
			
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

		glfwPollEvents();		// wait for user input
	}

	glfwDestroyWindow(win);
	glfwTerminate();

	return 0;
}
*/