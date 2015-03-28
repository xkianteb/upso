#ifndef COMMON_H__
#define COMMON_H__

#define MPI_PREPEND "MPI)"
#define VIZ_PREPEND "VIZ)"

inline int min( int a, int b ) { return a < b ? a : b; }
inline int max( int a, int b ) { return a > b ? a : b; }


struct map{
	unsigned int height;
	unsigned int width;
	// data as one long row-major sequence
	unsigned short *data;
};

//
//  saving parameters
//
const int NSTEPS = 1000;
const int SAVEFREQ = 1;

//
// particle data structure
//
typedef struct 
{
  double x;
  double y;
  double vx;
  double vy;
  double ax;
  double ay;
} particle_t;

//
//  timing routines
//
double read_timer( );

//
//  simulation routines
//
void set_size( int n, struct map *map_cfg);
void init_particles( int n, particle_t *p, struct map *map_cfg );
void apply_force( particle_t &particle, particle_t &neighbor );
void move( particle_t &p, struct map *map_cfg );

//
//  I/O routines
//
FILE *open_save( char *filename, int n );
void save( FILE *f, int n, particle_t *p );

//
//  argument processing routines
//
int find_option( int argc, char **argv, const char *option );
int read_int( int argc, char **argv, const char *option, int default_value );
char *read_string( int argc, char **argv, const char *option, char *default_value );

#endif
