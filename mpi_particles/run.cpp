#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "common.h"
#include "gl.h"
#include <thread>
#include <chrono>

#define TIMESTAMPS 10000
#define DRAW_DELAY 1

void usage(){
	printf( "Options:\n" );
	printf( "-h            : this text\n" );
	printf( "-p <int>      : set the number of particles (default 2)\n" );
	printf( "-o <filename> : specify the output file name for logging instead of drawing 3d (can be \"stdout\" for stdout)\n" );
	printf( "-i <filename> : Load points from this file instead of generating flow (can be \"stdin\" for stdin) (overrides all other settings)\n");
	printf( "-t <int>      : set the number of timesteps to calculate, default infinite, but %u with -o present\n", NSTEPS );
	
	printf("\n\nEither -o or -i must be set.\n");
	
}

int main( int argc, char **argv ){

    if( find_option( argc, argv, "-h" ) >= 0 ){
		usage();
		exit(0);
    }
	
	//
    //  set up MPI
    //
    int n_proc, rank;
    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &n_proc );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	
    char *input_file = NULL;
	if(find_option(argc, argv, "-i") >= 0){
		input_file = read_string( argc, argv, "-i", NULL );
	}
	bool read_from_stdin = input_file && str_equals(input_file, "stdin");
	
	
	int num_particles = 2;
	
	double min_x = INT_MAX;
	double max_x = INT_MIN;
	double min_y = INT_MAX;
	double max_y = INT_MIN;
	
	if(input_file){
		
		if(rank == 0){
			int num_points;
			FILE *fp = read_from_stdin ? stdin : fopen(input_file, "r");
			Vec<3> *points = 0;
			double radius;
			read_input(false, fp, &points, &num_particles, &num_points, &min_x, &max_x, &min_y, &max_y, &radius);
			if(!read_from_stdin){
				fclose(fp);
			}
			draw_data(points, num_particles, num_points, min_x, min_y, max_x, max_y, radius);
		}
		
		MPI_Barrier(MPI_COMM_WORLD);
		
		return 0;
	}
	
	
	// Won't get here if '-i' is provided.
	
	
    num_particles = read_int( argc, argv, "-p", 2 );
	
    char *savename = NULL;
	if(find_option(argc, argv, "-o") >= 0){
		savename = read_string( argc, argv, "-o", NULL );
	}
	bool write_to_stdout = savename && str_equals(savename, "stdout");
	
	int timesteps = savename ? NSTEPS : 0;
	
	if(find_option(argc, argv, "-t") >= 0){
		timesteps = read_int( argc, argv, "-t", NSTEPS );
	}
	
    particle_t *particles = (particle_t*) malloc( num_particles * sizeof(particle_t) );
	
	
    //
    //  allocate generic resources
    //
    FILE *fsave = savename && rank == 0 ? (write_to_stdout ? stdout : fopen( savename, "w" )) : NULL;
    
    MPI_Datatype PARTICLE;
    MPI_Type_contiguous( 6, MPI_DOUBLE, &PARTICLE );
    MPI_Type_commit( &PARTICLE );
    
    //
    //  set up the data partitioning across processors
    //
	
	int computing_procs = n_proc - 1;
	int gl_proc_rank = n_proc-1;
	
	if(computing_procs == 0){
		fprintf(stderr, "Error, need to assign at least 2 cores. 1 for GL, 1+ for MPI\n");
		exit(1);
	}
	
    int particle_per_proc = (num_particles + computing_procs - 1) / computing_procs;
	if(rank == 0){
		printf("computing procs: %i\tparticles: %i\tparticles per proc: %i\n", computing_procs, num_particles, particle_per_proc);
		fflush(stdout);
	}
	
    int *partition_offsets = (int*) malloc( (n_proc) * sizeof(int) );
    for( int i = 0; i < n_proc+1; i++ ){
        partition_offsets[i] = min( i * particle_per_proc, num_particles );
		//if(rank == 0) {	printf("offsets: %i %i\n", i, partition_offsets[i]);}
	}
    
    int *partition_sizes = (int*) malloc( n_proc * sizeof(int) );
    for( int i = 0; i < n_proc; i++ ){
	
        partition_sizes[i] = (i == n_proc-1) ? 0 : partition_offsets[i+1] - partition_offsets[i];
		//if(rank == 0) {	printf("sizes: %i %i\n", i, partition_sizes[i]);}
	}
    
    //
    //  allocate storage for local partition
    //
    int nlocal = partition_sizes[rank];
    particle_t *local = (particle_t*) malloc( nlocal * sizeof(particle_t) );
    
    //
    //  initialize and distribute the particles
    //
    set_size( num_particles );
    if( rank == 0 ){
        init_particles( num_particles, particles );
	}
	
	bool first = true;
	GLFWwindow *win = NULL;
	
	Textures *tex;
	unsigned int *sphere_draw_ids;
	Vec<3> *points;
	Vec<3> *last_points;
	AppContext *appctx = NULL;
	bool should_break = false;
	
	double radius = 15;
	double now = glfwGetTime();
	
	if(rank == gl_proc_rank){
		setup_gl(&win);
		
		AppContext ac(win);
		appctx = &ac;
		
		Textures tex1;
		tex = &tex1;
		
		points = (Vec<3> *) malloc(num_particles * sizeof(Vec<3>));
		last_points = (Vec<3> *) malloc(num_particles * sizeof(Vec<3>));
		sphere_draw_ids = (unsigned int *) malloc(num_particles * sizeof(unsigned int));
		
	}
	
    MPI_Scatterv( particles, partition_sizes, partition_offsets, PARTICLE, local, nlocal, PARTICLE, 0, MPI_COMM_WORLD );
	
    //
    //  simulate a number of time steps
    //
    double simulation_time = read_timer( );
    for( int step = 0; !timesteps || step < timesteps; step++ ){
        // 
        //  collect all global data locally (not good idea to do)
        //
        MPI_Allgatherv( local, nlocal, PARTICLE, particles, partition_sizes, partition_offsets, PARTICLE, MPI_COMM_WORLD );
		
        if(rank == gl_proc_rank){
			
			
			if(fsave){
				save( fsave, num_particles, particles );
			}else{
				// write to GL
				
				
				while(glfwGetTime() - now < DRAW_DELAY){
					should_break = poll_input(win, appctx);
					
					if(should_break){
						break;
					}
					// sleep for a microsecond
					std::this_thread::sleep_for(std::chrono::nanoseconds(1000));
				}
				
				for(int i = 0; i < num_particles; i++){
					points[i].x = particles[i].x;
					points[i].y = particles[i].y;
				}
				
				if(first){
					printf("Initializing spheres\n");
					initialize_spheres_and_gl(win, *tex, appctx, &sphere_draw_ids, points, num_particles, radius, &last_points);
					printf("draws : %u\n", appctx->geom.numDraws);
					first = false;
				}
				redraw_spheres(appctx, win, points, num_particles, sphere_draw_ids, &last_points);
				
				
				should_break = poll_input(win, appctx);
				now = glfwGetTime();
				
			}
		}else{
			// rank n-1 has to only do GL/output
        
			//
			//  compute all fnum_particlesrces
			//
			for( int i = 0; i < nlocal; i++ )
			{
				local[i].ax = local[i].ay = 0;
				for (int j = 0; j < num_particles; j++ )
					apply_force( local[i], particles[j] );
			}
			
			//
			//  move particles
			//
			for( int i = 0; i < nlocal; i++ )
				move( local[i] );
		}
		
		MPI_Bcast(&should_break, 1, MPI::BOOL, gl_proc_rank, MPI_COMM_WORLD);
		
		if(should_break){
			break;
		}
    }
    simulation_time = read_timer( ) - simulation_time;
    
    if( rank == 0 ){
        printf( "n = %d, n_procs = %d (MPI procs = %d), simulation time = %g s\n", num_particles, n_proc, computing_procs, simulation_time );
	}
    
    //
    //  release resources
    //
	
	if(rank == gl_proc_rank && win){
		glfwDestroyWindow(win);
		glfwTerminate();
	}
    free( partition_offsets );
    free( partition_sizes );
    free( local );
    free( particles );
    if( fsave )
        fclose( fsave );
    
    MPI_Finalize( );
    
    return 0;
}
