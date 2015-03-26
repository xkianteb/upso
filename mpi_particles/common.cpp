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
	
	
	// returns true if cell has 1
	/*
	if(map_cfg->data[cell] == 1){
		fprintf(stderr,"%s particle at col,row (%u, %u)\n",MPI_PREPEND, col,row);
	}
	*/
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

//
//  integrate the ODE
//
void move( particle_t &p )
{
    //
    //  slightly simplified Velocity Verlet integration
    //  conserves energy better than explicit Euler method
    //
    p.vx += p.ax * dt;
    p.vy += p.ay * dt;
    p.x  += p.vx * dt;
    p.y  += p.vy * dt;

    //
    //  bounce from walls
    //
    while( p.x < 0 || p.x > size )
    {
        p.x  = p.x < 0 ? -p.x : 2*size-p.x;
        p.vx = -p.vx;
    }
    while( p.y < 0 || p.y > size )
    {
        p.y  = p.y < 0 ? -p.y : 2*size-p.y;
        p.vy = -p.vy;
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
