#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "common.h"
#include "gl.h"
#include <thread>
#include <chrono>

#define TIMESTAMPS 10000


void usage(){
	printf( "Example run: mpirun -np 2 ./run -p 20 -o stdout | ./run -i stdin\n\n");
	printf( "Options:\n" );
	printf( "-h            : this text\n" );
	printf( "-p <int>      : set the number of particles (default 2)\n" );
	printf( "-o <filename> : specify the output file name for logging instead of drawing 3d (can be \"stdout\" for stdout)\n" );
	printf( "-i <filename> : Load points from this file instead of generating flow (can be \"stdin\" for stdin) (overrides all other settings)\n");
	printf( "-t <int>      : set the number of timesteps to calculate, default infinite, but %u with -o present\n", NSTEPS );
	printf( "-c <filename> : Use map config for simulator\n");
	
	printf("\n\nEither -o or -i must be set.\n");
	
}

void read_map(FILE *fp, struct map *map_cfg){
	char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    short temp_str_len = 50;
    char first_term[temp_str_len];
	char first_letter[temp_str_len];
	memset(&first_letter, 0, temp_str_len);
    
    if(fp == NULL){
        fprintf(stderr, "%s Error reading file.\n", MPI_PREPEND);
        exit(1);
    }
	
	map_cfg->height = 0;
	map_cfg->width = 0;
	map_cfg->data = NULL;
	unsigned int cells_read = 0;
	char cell;

	while((read = getline(&line, &len, fp)) != -1){
        first_term[0] = 0;
		first_letter[0] = 0;
		
        sscanf(line, "%s", first_term);
		
		sscanf(line, "%c", first_letter);
		
        if(str_equals("h",first_letter)){
            sscanf(line, "h %u\n", &map_cfg->height);
			fprintf(stderr,"%s map height: %u\n",MPI_PREPEND, map_cfg->height);
			
		}else if(str_equals("w",first_letter)){
            sscanf(line, "w %u\n", &map_cfg->width);
			fprintf(stderr,"%s map width: %u\n",MPI_PREPEND, map_cfg->width);
			
		}else{
			
			for(int i=0; i < map_cfg->width; i++){
				sscanf(&line[i], "%c", &cell);
				map_cfg->data[cells_read] = (unsigned short) (cell - '0');
				//printf("read cell %i, %c, %u\n", i, cell, map_cfg->data[cells_read]);
				cells_read++;
			}
			
		}

		// should only run this once, then it mallocs.
		if(!map_cfg->data && map_cfg->height > 0 && map_cfg->width > 0){
			map_cfg->data = (unsigned short *) malloc (map_cfg->height * map_cfg->width * sizeof(unsigned short));
			if(!map_cfg->data){
				fprintf(stderr, "%s Couldn't malloc for map config\n", MPI_PREPEND);
				exit(1);
			}
			memset(map_cfg->data, 0, map_cfg->height * map_cfg->width * sizeof(unsigned short));
		}
	}
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
	
	if(input_file){
		
		if(rank == 0){
			FILE *fp = read_from_stdin ? stdin : fopen(input_file, "r");
			draw_data(fp, read_from_stdin);
			if(!read_from_stdin){
				fclose(fp);
			}

		}
		
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Finalize();
		return 0;
	}
	
	// Won't get here if '-i' is provided.
	
    num_particles = read_int( argc, argv, "-p", 2 );
	
    char *savename = NULL;
	if(find_option(argc, argv, "-o") >= 0){
		savename = read_string( argc, argv, "-o", NULL );
	}
	bool write_to_stdout = savename && str_equals(savename, "stdout");
	
	int timesteps = write_to_stdout ? 0 : NSTEPS;
	
	if(find_option(argc, argv, "-t") >= 0){
		timesteps = read_int( argc, argv, "-t", timesteps );
	}
	if(rank == 0){
		fprintf(stderr, "%s Drawing %u timesteps\n",MPI_PREPEND, timesteps);
	}
    particle_t *particles = (particle_t*) malloc( num_particles * sizeof(particle_t) );
	
	char *map_cfg_file = NULL;
	if(find_option(argc, argv, "-c") >= 0){
		map_cfg_file = read_string( argc, argv, "-c", NULL );
	}
	
	
	// Read map config by rank 0, process it, and broadcast it out
	struct map map_cfg = {0,0,0};
	if(rank == 0 && map_cfg_file){
		FILE *fp = fopen(map_cfg_file, "r");
		read_map(fp, &map_cfg);
	}
	
	MPI_Bcast(&map_cfg.height, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD );
	MPI_Bcast(&map_cfg.width, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD );
	
	if(rank > 0 && map_cfg.height > 0 && map_cfg.width > 0){
		map_cfg.data = (unsigned short *) malloc (map_cfg.height * map_cfg.width * sizeof(unsigned short));
	}
	
	if(map_cfg.height > 0 && map_cfg.width > 0){
		MPI_Bcast(map_cfg.data, map_cfg.height * map_cfg.width, MPI_UNSIGNED_SHORT, 0, MPI_COMM_WORLD);
	}
	
	
	
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
	
	if(!savename && !write_to_stdout){
		usage();
		exit(1);
	}
	
    int particle_per_proc = (num_particles + n_proc - 1) / n_proc;
	if(rank == 0){
		fprintf(stderr, "%s particles: %i\tparticles per proc: %i\n", MPI_PREPEND, num_particles, particle_per_proc);
		fflush(stdout);
	}
	
    int *partition_offsets = (int*) malloc( (n_proc + 1) * sizeof(int) );
    for( int i = 0; i < n_proc+1; i++ ){
        partition_offsets[i] = min( i * particle_per_proc, num_particles );
	}
    
    int *partition_sizes = (int*) malloc( n_proc * sizeof(int) );
    for( int i = 0; i < n_proc; i++ ){
	
        partition_sizes[i] = partition_offsets[i+1] - partition_offsets[i];
	}
    
    //
    //  allocate storage for local partition
    //
    int nlocal = partition_sizes[rank];
    particle_t *local = (particle_t*) malloc( nlocal * sizeof(particle_t) );
    
    //
    //  initialize and distribute the particles
    //
    set_size( num_particles, &map_cfg );
    if( rank == 0 ){
        init_particles( num_particles, particles, &map_cfg );
	}
	
	bool first = true;
	GLFWwindow *win = NULL;
	//Textures *tex;
	unsigned int *sphere_draw_ids;
	Vec<3> *points;
	Vec<3> *last_points;
	
	double radius = 15;
	double now = glfwGetTime();
	
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

        if(rank == 0){
			save( fsave, num_particles, particles );
			
			// sleep for a few milliseconds
			//std::this_thread::sleep_for(std::chrono::nanoseconds(20000000));
		}
		
        
		//
		//  compute all fnum_particlesrces
		//
		for( int i = 0; i < nlocal; i++ ){
			local[i].ax = local[i].ay = 0;
			for (int j = 0; j < num_particles; j++ ){
				apply_force( local[i], particles[j] );
			}
		}
		
		//
		//  move particles
		//
		for( int i = 0; i < nlocal; i++ ){
			move( local[i], &map_cfg );
		}
    }
    simulation_time = read_timer( ) - simulation_time;
    
    if( rank == 0 ){
        fprintf(stderr, "%s n = %d, n_procs = %d, simulation time = %g s\n", MPI_PREPEND, num_particles, n_proc, simulation_time );
	}
    
    //
    //  release resources
    //
	
	if(map_cfg.data){
		free(map_cfg.data);
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
