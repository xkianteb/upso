#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "common.h"
#include "gl.h"

#define TIMESTAMPS 10000

void usage(){
	printf( "Options:\n" );
	printf( "-h            : this text\n" );
	printf( "-p <int>      : set the number of particles (default 2)\n" );
	printf( "-o <filename> : specify the output file name for logging in background (can be \"stdout\" for stdout)\n" );
	printf( "-i <filename> : Load points from this file instead of generating flow (can be \"stdin\" for stdin) (overrides all other settings)\n");
	printf( "-t <int>      : set the number of timesteps to calculate, default infinite, but %u with -o present\n", NSTEPS );
	
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
	
	
	int num_particles;
	
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
    int particle_per_proc = (num_particles + n_proc - 1) / n_proc;
    int *partition_offsets = (int*) malloc( (n_proc+1) * sizeof(int) );
    for( int i = 0; i < n_proc+1; i++ )
        partition_offsets[i] = min( i * particle_per_proc, num_particles );
    
    int *partition_sizes = (int*) malloc( n_proc * sizeof(int) );
    for( int i = 0; i < n_proc; i++ )
        partition_sizes[i] = partition_offsets[i+1] - partition_offsets[i];
    
    //
    //  allocate storage for local partition
    //
    int nlocal = partition_sizes[rank];
    particle_t *local = (particle_t*) malloc( nlocal * sizeof(particle_t) );
    
    //
    //  initialize and distribute the particles
    //
    set_size( num_particles );
    if( rank == 0 )
        init_particles( num_particles, particles );
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
        
        //
        //  save current step if necessary (slightly different semantics than in other codes)
        //
        if( fsave && (step%SAVEFREQ) == 0 )
            save( fsave, num_particles, particles );
        
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
    simulation_time = read_timer( ) - simulation_time;
    
    if( rank == 0 )
        printf( "n = %d, n_procs = %d, simulation time = %g s\n", num_particles, n_proc, simulation_time );
    
    //
    //  release resources
    //
    free( partition_offsets );
    free( partition_sizes );
    free( local );
    free( particles );
    if( fsave )
        fclose( fsave );
    
    MPI_Finalize( );
    
    return 0;
}
