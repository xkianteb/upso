#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <float.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
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
#define dt      0.0005

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
	unsigned int highest_dim = max(map_cfg->height, map_cfg->width);
	
	unsigned int col = (int) floor(x * highest_dim);
	unsigned int row = (int) floor(y * highest_dim);
	
	unsigned int cell = map_cfg->width * row + col;
	//printf("%lf %lf\t\t%u %u\t\t%u\t\t%u %u\n", x,y,col,row, cell, map_cfg->width,map_cfg->height);fflush(stdout);
	
	if(col >= map_cfg->width || row >= map_cfg->height || cell >= map_cfg->height * map_cfg->width){
		return false;
	}
	
	return map_cfg->data[cell] == 1;
}


//
//  Initialize the particle positions and velocities
//
void init_particles( int n, particle_t *p, struct map *map_cfg )
{
    srand48( time( NULL ) );
        
    int sx = (int)ceil(sqrt((double)n));
    int sy = (n+sx-1)/sx;
    
    int *shuffle = (int*)malloc( n * sizeof(int) );
    for( int i = 0; i < n; i++ )
        shuffle[i] = i;
    
    for( int i = 0; i < n; i++ ) 
    {
        //
        //  make sure particles are not spatially sorted
        //
        int j = lrand48()%(n-i);
        int k = shuffle[j];
        shuffle[j] = shuffle[n-i-1];
        
        // compute x/y until they lie inside the walkable area
		do{
			p[i].x = drand48() * size; //size*(1.+(k%sx))/(1+sx);
			p[i].y = drand48() * size; //size*(1.+(k/sx))/(1+sy);
		}while(!is_valid_location(p[i].x, p[i].y, map_cfg));
		
		fprintf(stderr, "%s particle %d at %lf %lf\n", MPI_PREPEND, i, p[i].x, p[i].y );fflush(stderr);
        //
        //  assign random velocities within a bound
        //
        p[i].vx = drand48()*2-1;
        p[i].vy = drand48()*2-1;
    }
    free( shuffle );
}

//
//  interact two particles
//
void apply_force( particle_t &particle, particle_t &neighbor )
{

    double dx = neighbor.x - particle.x;
    double dy = neighbor.y - particle.y;
    double r2 = dx * dx + dy * dy;
    if( r2 > cutoff*cutoff )
        return;
    r2 = fmax( r2, min_r*min_r );
    double r = sqrt( r2 );

    //
    //  very simple short-range repulsive force
    //
    double coef = ( 1 - cutoff / r ) / r2 / mass;
    particle.ax += coef * dx;
    particle.ay += coef * dy;
}


// returns:
// true: wall between x1 and x2
// *wall_x: will have the x coord of the wall
bool wall_between_x(double new_x, double orig_x, double orig_y, double *wall_x, struct map *map_cfg){
	unsigned int highest_dim = max(map_cfg->height, map_cfg->width);
	unsigned int new_col = (int) floor(new_x * highest_dim);
	unsigned int old_col = (int) floor(orig_x * highest_dim);
	unsigned int row = (int) floor(orig_y * highest_dim);
	
	unsigned int cell = map_cfg->width * row + new_col;
	unsigned int cell2 = map_cfg->width * row + old_col;
	
	
	// Both cells shouldn't be zero, else a particle is in a bad place
	//fprintf(stderr, "%s particle at %lf %lf, new_x: %lf\n", MPI_PREPEND, orig_x, orig_y ,new_x);fflush(stderr);
	//fprintf(stderr, "%u %u %u\t\t%u %u\t\tcell values: %u %u\t%u %u\n", old_col,new_col, row, cell, cell2,map_cfg->data[cell] ,map_cfg->data[cell2], map_cfg->width,map_cfg->height);
	//fflush(stderr);
	assert(map_cfg->data[cell] != 0 || map_cfg->data[cell2] != 0);
	
	//fprintf(stderr,"\tpassed\n");
	// return false if both are one, so no wall, walkable.
	if(map_cfg->data[cell] == 1 && map_cfg->data[cell2] == 1){
		//fprintf(stderr,"x: (%lf, %lf) -> %lf, both are 1\n", orig_x, orig_y, new_x);
		return false;
	}
	
	// there is a wall present
	(*wall_x) = ((double) (new_col > old_col ? new_col : old_col)) / highest_dim;
	
	//fprintf(stderr,"x: (%lf, %lf) -> %lf, wall at %lf\n", orig_x, orig_y, new_x, (*wall_x));
	return true;
	
}

bool wall_between_y(double new_y, double orig_y, double orig_x, double *wall_y, struct map *map_cfg){
	unsigned int highest_dim = max(map_cfg->height, map_cfg->width);
	unsigned int new_row = (int) floor(new_y * highest_dim);
	unsigned int old_row = (int) floor(orig_y * highest_dim);
	unsigned int col = (int) floor(orig_x * highest_dim);
	
	unsigned int cell = map_cfg->width * old_row + col;
	unsigned int cell2 = map_cfg->width * new_row + col;
	
	
	// Both cells shouldn't be zero, else a particle is in a bad place
	assert(map_cfg->data[cell] != 0 || map_cfg->data[cell2] != 0);
	
	// return false if both are one, so no wall, walkable.
	if(map_cfg->data[cell] == 1 && map_cfg->data[cell2] == 1){
		//fprintf(stderr,"y: (%lf, %lf) -> %lf, both are 1\n", orig_x, orig_y, new_y);
		return false;
	}
	
	// there is a wall present
	(*wall_y) = ((double) (new_row > old_row ? new_row : old_row)) / highest_dim;
	
	//fprintf(stderr,"y: (%lf, %lf) -> %lf, wall at %lf\n", orig_x, orig_y, new_y, (*wall_y));
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
	
    p.vx += p.ax * dt;
    p.vy += p.ay * dt;
    p.x  += p.vx * dt;
    p.y  += p.vy * dt;

    //
    //  bounce from walls
    //
	double wall_x;
	double wall_y;
	
	if(wall_between_x(p.x, orig_x, orig_y, &wall_x, map_cfg)){
		//while( p.x < 0 || p.x > wall_x ){
		//fprintf(stderr,"** Calculating x wall: %lf %lf\t\twall: %lf\t\t(%u || %u)\n",orig_x, p.x, wall_x, ((p.x > wall_x && wall_x > orig_x ) ? 1 : 0), ((orig_x > wall_x && wall_x > p.x) ? 1 : 0) );
		while( (p.x > wall_x && wall_x > orig_x ) || (orig_x > wall_x && wall_x > p.x)){

			//fprintf(stderr,"** Calculating x wall: %lf %lf\t\twall: %lf\t\t(%u || %u)\n",orig_x, p.x, wall_x, ((p.x > wall_x && wall_x > orig_x ) ? 1 : 0), ((orig_x > wall_x && wall_x > p.x) ? 1 : 0) );
			// p.x  = p.x < wall_x ? orig_x - p.vx*dt : 2*size-p.x;
			p.x  = 2*wall_x - p.x; //p.x < wall_x ? orig_x - p.vx*dt : 2*wall_x-p.x;
			p.vx = -p.vx;
		}
	}
	
	if(wall_between_y(p.y, orig_y, orig_x, &wall_y, map_cfg)){
		//while( p.y < 0 || p.y > size ){
		//fprintf(stderr,"** Calculating y wall: %lf %lf\t\twall: %lf\t\t(%u || %u)\n",orig_y, p.y, wall_y, ((p.y > wall_y && wall_y > orig_y )  ? 1 : 0), ((orig_y > wall_y && wall_y > p.y) ? 1 : 0));
		
		while( (p.y > wall_y && wall_y > orig_y ) || (orig_y > wall_y && wall_y > p.y)){
		//fprintf(stderr,"** Calculating y wall: %lf %lf\t\twall: %lf\t\t(%u || %u)\n",orig_y, p.y, wall_y, ((p.y > wall_y && wall_y > orig_y )  ? 1 : 0), ((orig_y > wall_y && wall_y > p.y) ? 1 : 0));
			p.y  = 2*wall_y - p.y; //p.y < wall_y ? wall_y + (wall_y - p.y) : 2*wall_y-p.y;
			p.vy = -p.vy;
		}
	}
	
}

//
//  I/O routines
//
void save( FILE *f, int n, particle_t *p )
{

    static bool first = true;
    if( first )
    {
        fprintf( f, "n %d\nr %lf\ns %lf\n", n, cutoff, size );fflush(f);
        first = false;
    }
	for( int i = 0; i < n; i++ ){
        fprintf( f, "%g %g\n", p[i].x, p[i].y );fflush(f);
	}
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

char *read_string( int argc, char **argv, const char *option, char *default_value )
{
    int iplace = find_option( argc, argv, option );
    if( iplace >= 0 && iplace < argc-1 )
        return argv[iplace+1];
    return default_value;
}
