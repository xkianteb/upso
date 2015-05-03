#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <float.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <cmath>
#include "common.h"


double size;
double height;
double width;


//
//  tuned constants
//
#define density 0.0005
#define mass    0.01
#define cutoff  0.01
#define min_r   (cutoff/100)
#define dt      0.0005 // 0.0005
#define precision 2 // Precision to how close the coordinates should be to goal

#define RANDOM_COLOR true

//
//  timer
//
double read_timer( )
{
    static bool initialized = false;
    static struct timeval start;
    struct timeval end;
    if( !initialized )
    {
        gettimeofday( &start, NULL );
        initialized = true;
    }
    gettimeofday( &end, NULL );
    return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}

//
//  keep density constant
//
void set_size( int n, struct map *map_cfg){
	if(map_cfg->height > 0 && map_cfg->width > 0){
		size = 1.0; // keep all coords floating point < 1
		height = map_cfg->height;
		width = map_cfg->width;
	}else{
		size = sqrt( density * n );
		height = 1.0;
		width = 1.0;
	}
}


bool is_valid_location(double x, double y, struct map *map_cfg){
	unsigned int highest_dim = MAX(map_cfg->height, map_cfg->width);
	
	unsigned int col = (int) floor(x * highest_dim);
	unsigned int row = (int) floor(y * highest_dim);
	
	unsigned int cell = map_cfg->width * row + col;
	//printf("%lf %lf\t\t%u %u\t\t%u\t\t%u %u\n", x,y,col,row, cell, map_cfg->width,map_cfg->height);fflush(stdout);
	
	if(col >= map_cfg->width || row >= map_cfg->height || cell >= map_cfg->height * map_cfg->width){
		return false;
	}
	
	return map_cfg->data[cell] != 0;
}

// Checks to see if you are movning toward goal
double is_valid_direction_y(double vy, double y, double goal_y)
{
	using std::abs;
    //unsigned int highest_dim = MAX(map_cfg->height, map_cfg->width);
	//double row = (double) floor(y * highest_dim);
	//double goal = map_cfg->goal_row;
	
	//double direction = (goal-row);
	int test = (abs(y - goal_y) < pow(0.1, precision) * fmax(abs(y), abs(goal_y)));

	if(test==0){
		//printf("y direction: %d\n", sign(goal_y-y));
		return sign(goal_y-y);
	} else {
		return 0;
	}
}

// Checks to see if you are movning toward goal
double is_valid_direction_x(double vx, double x, double goal_x)
{
	using std::abs;
	//unsigned int highest_dim = MAX(map_cfg->height, map_cfg->width);
	//double col = (double) floor(x * highest_dim);
	//double goal = map_cfg->goal_col;
	
	//double direction = (goal-col);
	int test = (abs(x - goal_x) < pow(0.1, precision) * fmax(abs(x), abs(goal_x)));

	if(test==0){
		//printf("x direction: %d\n", sign(goal_x-x));
		return sign(goal_x-x);
	} else {
		return 0;
	}
}

//int break_check = 0;

bool at_goal(double x, double y, double goal_x, double goal_y)
{
    using std::abs;
    
    int x_result = abs(x - goal_x) < pow(0.1, precision) * fmax(abs(x), abs(goal_x));
    int y_result = abs(y - goal_y) < pow(0.1, precision) * fmax(abs(y), abs(goal_y));

    /*break_check+=1;
    if(break_check == 3) {
    	printf("---------------------------------------------\n");
    	break_check = 0;
    }*/
	
    return (x_result & y_result);
}


//
//  Initialize the particle positions and velocities
//
void init_particles( int n, int sn, double agents[][4], particle_t *p, struct map *map_cfg ){
	unsigned int highest_dim = MAX(map_cfg->height, map_cfg->width);
    srand48( time( NULL ) );
	
	int sx = (int)ceil(sqrt((double)n));
    int sy = (n+sx-1)/sx;

    int z = 0;
    
    int *shuffle = (int*)malloc( n * sizeof(int) );
    for( int i = 0; i < n; i++ )
        shuffle[i] = i;
	
	for( int i = 0; i < n; i++ ){
        //
        //  make sure particles are not spatially sorted
        //
        int j = lrand48()%(n-i);
        int k = shuffle[j];
        shuffle[j] = shuffle[n-i-1];
        
        // compute x/y until they lie inside the walkable area
		do{
			if ( i >= 5) { 
				//printf("Values: (%lf,%lf,%lf,%lf)\n", agents[z][0], agents[z][1], 
				//	     agents[z][2], agents[z][3]);
				p[i].x = agents[z][0]; //size*(1.+(k%sx))/(1+sx);
				p[i].y = agents[z][1]; //size*(1.+(k/sx))/(1+sy);
	            p[i].goal_x = agents[z][2];
	            p[i].goal_y = agents[z][3];
	            z +=1;

	            if (!is_valid_location(p[i].x, p[i].y, map_cfg)) {
	            	printf("Error agent location is not valid\n");
	            	exit(0);
	            }

	            if(!is_valid_location(p[i].goal_x, p[i].goal_y, map_cfg)) {
	            	printf("Error agent goal location is not valid\n");
	            	exit(0);
	            }

	       } else {
	        	p[i].x = drand48() * size; //size*(1.+(k%sx))/(1+sx);
				p[i].y = drand48() * size; //size*(1.+(k/sx))/(1+sy);
	            p[i].goal_x = drand48() * size;
	            p[i].goal_y = drand48() * size;
	        }
		}while(!is_valid_location(p[i].x, p[i].y, map_cfg) || !is_valid_location(p[i].goal_x, p[i].goal_y, map_cfg));
		
        //
        //  assign random velocities within a bound
        p[i].vx = (drand48() * 2.0) + 1.0;
        p[i].vy = (drand48() * 2.0) + 1.0;

        double x_direction = is_valid_direction_x(p[i].vx, p[i].x, p[i].goal_x);
    	double y_direction = is_valid_direction_y(p[i].vy, p[i].y, p[i].goal_y);
    	p[i].vx *= sign(x_direction);
        p[i].vy *= sign(y_direction);
		
		// set color:
		p[i].color_r = RANDOM_COLOR ? drand48() : 0.0;
		p[i].color_g = RANDOM_COLOR ? drand48() : 0.0;
		p[i].color_b = RANDOM_COLOR ? drand48() : 0.0;
		//p[i].goal = 3;
		
		//fprintf(stderr, "%s particle %d at %lf %lf, vel (%lf,%lf)\n", MPI_PREPEND, i, p[i].x, p[i].y, p[i].vx, p[i].vy );fflush(stderr);
    }
    free( shuffle );
}

//
//  interact two particles
//
void apply_force( particle_t &particle, particle_t &neighbor )
{
	// Add .1 percent to avoid dealing with collisions
	// So everyone just rushes pass each other
    double dx = (neighbor.x - particle.x) + (sign(neighbor.x - particle.x) *.1);
    double dy = (neighbor.y - particle.y) + (sign(neighbor.y - particle.y) *.1);
    double r2 = dx * dx + dy * dy;
    if( r2 > cutoff*cutoff )
        return;
    r2 = MAX( r2, min_r*min_r );
    double r = sqrt( r2 );

    //
    //  very simple short-range repulsive force
    //
    double coef = ( 1 - cutoff / r ) / r2 / mass;
	
	double max_speedup = 1000.0;
    particle.ax += sign(coef * dx) * MIN(max_speedup, fabs(coef*dx));
    particle.ay += sign(coef * dy) * MIN(max_speedup, fabs(coef*dy));
}

unsigned int cell_for_pos(double x, double y, struct map *map_cfg){
	unsigned int highest_dim = MAX(map_cfg->height, map_cfg->width);
	unsigned int col = (unsigned int) floor(x * highest_dim);
	unsigned int row = (unsigned int) floor(y * highest_dim);
	
	unsigned int cell = map_cfg->width * row + col;
	return cell;
}

// returns:
// true: wall between x1 and x2
// *wall_x: will have the x coord of the wall
bool wall_between_x(double new_x, double orig_x, double orig_y, double *wall_x, struct map *map_cfg){
	unsigned int highest_dim = MAX(map_cfg->height, map_cfg->width);
	unsigned int new_col = (unsigned int) floor(new_x * highest_dim);
	unsigned int old_col = (unsigned int) floor(orig_x * highest_dim);
	unsigned int row = (int) floor(orig_y * highest_dim);
	
	unsigned int new_cell = cell_for_pos(new_x, orig_y, map_cfg); //map_cfg->width * row + new_col;
	unsigned int old_cell = cell_for_pos(orig_x, orig_y, map_cfg); //map_cfg->width * row + old_col;
	
    // Old cell should only be one
    assert(map_cfg->data[old_cell] == 1);
    // new cell should be valid
    assert(map_cfg->height > row && map_cfg->width > new_col && new_cell >= 0);
	
	// return false if both are one, so no wall, walkable.
	if(map_cfg->data[new_cell] != 0){
		return false;
	}
	
	// there is a wall present
	(*wall_x) = ((double) (new_col > old_col ? new_col : old_col)) / highest_dim;
	
	return true;
	
}

bool wall_between_y(double new_y, double orig_y, double orig_x, double *wall_y, struct map *map_cfg){
	unsigned int highest_dim = MAX(map_cfg->height, map_cfg->width);
	unsigned int new_row = (unsigned int) floor(new_y * highest_dim);
	unsigned int old_row = (unsigned int) floor(orig_y * highest_dim);
	unsigned int col = (int) floor(orig_x * highest_dim);
	
	unsigned int old_cell = cell_for_pos(orig_x, orig_y, map_cfg); // map_cfg->width * old_row + col;
	unsigned int new_cell = cell_for_pos(orig_x, new_y, map_cfg); // map_cfg->width * new_row + col;
	
    // Old cell should only be one
    assert(map_cfg->data[old_cell] == 1);
    // new cell should be valid
    assert(map_cfg->height > new_row && map_cfg->width > col && new_cell >= 0);
	
	// return false if both are one, so no wall, walkable.
	if(map_cfg->data[new_cell] != 0){
		return false;
	}
	
	// there is a wall present
	(*wall_y) = ((double) (new_row > old_row ? new_row : old_row)) / highest_dim;
	
	return true;
}

//
//  integrate the ODE
//
void move( particle_t &p, struct map *map_cfg ){
    //
    //  slightly simplified Velocity Verlet integration
    //  conserves energy better than explicit Euler method
    //
	double orig_x = p.x;
	double orig_y = p.y;
	double orig_vx = p.vx;
	double orig_vy = p.vy;

	//printf("Goal Row: [%u] and Goal Col: [%u]\n", map_cfg->goal_row, map_cfg->goal_col);

	// Consider removing these lines later. Used to avoid speed explosions.
	p.vx = ((double) sign(p.vx)) * ((double) MIN(2.0, fabs(p.vx)));
	p.vy = ((double) sign(p.vy)) * ((double) MIN(2.0, fabs(p.vy)));

	double x_direction = is_valid_direction_x(p.vx, p.x, p.goal_x);
	if (x_direction > 0) {
    	p.vx += p.ax * dt;
    } else if (x_direction < 0) {
    	p.vx += p.ax * dt * -1.0;
    	//p.vy += sign(p.vy);
    } else {
    	p.vx = 0.0;
    }

    //fprintf(stderr,"direction: %u\n",is_valid_direction_y(p.vy, p.y, map_cfg));
    double y_direction = is_valid_direction_y(p.vy, p.y, p.goal_y);
    if(y_direction > 0) {
    	p.vy += p.ay * dt;
	} else if(y_direction < 0){
		p.vy += p.ay * dt * -1.0;
		//p.vx += sign(p.vx);
		//p.vy *= p.vy;
	} else {
		p.vy = 0.0;
	}

	//printf("Velocity y: [%f] vs Veclocity x: [%f]\n", p.vy, p.vx);

	//fprintf(stderr,"velocity x: %f\n",p.vx);
	//fprintf(stderr,"velocity y: %f\n",p.vy);

	if(!at_goal(p.x, p.y, p.goal_x, p.goal_y)) {
    	p.x  += p.vx * dt;
    	p.y  += p.vy * dt;
    	//printf("x: %f | goal: %f| p.vx: %f\n", p.x, p.goal_x, p.vx );
    	//printf("y: %f | goal: %f| p.vy: %f\n", p.y, p.goal_y, p.vy );
    } 
    //else {
    //	printf("****Done x: %f | goal: %f| p.vx: %f\n", p.x, p.goal_x, p.vx );
    //	printf("****Done y: %f | goal: %f| p.vy: %f\n", p.y, p.goal_y, p.vy );
    //}


    //
    //  bounce from walls
    //
	double wall_x;
	double wall_y;
	
	if(wall_between_x(p.x, orig_x, orig_y, &wall_x, map_cfg)){

		while( (p.x > wall_x && wall_x > orig_x ) || (orig_x > wall_x && wall_x > p.x)){

			p.x  = 2*wall_x - p.x;
			x_direction = is_valid_direction_x(p.vx, p.x, p.goal_x);
			//printf("---X Here: Velocity x,y: [%f,%f] Postion: [%f,%f] Direction: [%f]\n", p.vx, p.vy, p.x,p.y, x_direction);
			if(x_direction < 0 ){
				p.vx *= -1.0;
				p.vx += p.ax * dt;
			} else if(x_direction == 0) {
				p.vx = 0.0;
				p.vy += (p.ay*p.ay);
			}
			//p.vx = -p.vx;
		}
	}

	
	if(wall_between_y(p.y, orig_y, p.x, &wall_y, map_cfg)){
		while( (p.y > wall_y && wall_y > orig_y ) || (orig_y > wall_y && wall_y > p.y)){
			p.y  = 2*wall_y - p.y;
			y_direction = is_valid_direction_y(p.vy, p.y, p.goal_y);
			//printf("---Y Here: Velocity x,y: [%f,%f] Postion: [%f,%f] Direction: [%f]\n", p.vx, p.vy, p.x,p.y, y_direction);
			if(y_direction < 0 ){
				p.vy *= -1.0;
				p.vy += p.ay * dt;
			} else if(x_direction == 0) {
				//printf("---Y Here: Velocity x: [%f] vs Veclocity y: [%f]\n", p.vy, p.vx);
				p.vy = 0.0;
				p.vx += (p.ax*p.ax);
			}
			
			//p.vy = -p.vy;
		}
	}
	
	unsigned int cell = cell_for_pos(p.x,p.y, map_cfg);
	assert(map_cfg->data[ cell ] != 0);
}

//
//  I/O routines
//
void save( FILE *f, int n, particle_t *p, struct map *map_cfg ){

	double velocity = 0.0;
    static bool first = true;
    if( first ){
        fprintf( f, "n %d\nr %lf\ns %lf\na %u\n", n, cutoff, size, MAX(map_cfg->height, map_cfg->width) );
    }
	
	for( int i = 0; i < n; i++ ){
	
		if(first){
			fprintf(f, "c %u %lf %lf %lf\n", i, p[i].color_r,p[i].color_g,p[i].color_b);
		}
		
        fprintf( f, "p %g %g\n", p[i].x, p[i].y );
		
		velocity += fabs(p[i].vx) + fabs(p[i].vy);
	}
	
	// For monitoring overall velocity speeds.
	//fprintf(stderr, "Vel: %lf\n",velocity);fflush(stderr);
	
	first = false;
	fflush(f);
}

//
//  command line option processing
//
int find_option( int argc, char **argv, const char *option )
{
    for( int i = 1; i < argc; i++ )
        if( strcmp( argv[i], option ) == 0 )
            return i;
    return -1;
}

int read_int( int argc, char **argv, const char *option, int default_value )
{
    int iplace = find_option( argc, argv, option );
    if( iplace >= 0 && iplace < argc-1 )
        return atoi( argv[iplace+1] );
    return default_value;
}

double read_double( int argc, char **argv, const char *option, int default_value ){
    int iplace = find_option( argc, argv, option );
    if( iplace >= 0 && iplace < argc-1 )
        return atof( argv[iplace+1] );
    return default_value;
}


char *read_string( int argc, char **argv, const char *option, char *default_value )
{
    int iplace = find_option( argc, argv, option );
    if( iplace >= 0 && iplace < argc-1 )
        return argv[iplace+1];
    return default_value;
}
