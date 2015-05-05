#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "common.h"
#include "gl.h"
#include <thread>
#include <chrono>

#define TIMESTAMPS 10000

#define SEND_INITIAL_PARTICLE_COUNT 100
#define SEND_INITIAL_PARTICLES 101

#define GHOST_ZONE_PADDING 0.1

void usage(){
	printf( "Example run: mpirun -np 4 ./run -p 20 -o stdout | ./run -i stdin\n\n");
	printf( "Options:\n" );
	printf( "-h                        : this text\n" );
	printf( "\nOptions for particle simulator:\n");
	printf( "-p <filename>             : Read particle config\n" );
	printf( "-o <filename>             : specify the output file name for logging instead of drawing 3d (can be \"stdout\" for stdout)\n" );
	printf( "-t <int>                  : set the number of timesteps to calculate, default infinite, but %u with -o present\n", NSTEPS );
	printf( "-c <filename>             : Use map config for simulator, defaults to map.cfg (plain old square).\n");
	printf( "-x <filename>	           : Load particle starting configuration.\n");
	printf( "-y <agents number>        : Number of agents in the -p file.\n");
	printf( "-r <random agents number> : Number of additional random agents to generate (default 2 if no -y arg).\n");

	printf( "\nOptions for OpenGL Visualizer:\n");
	printf( "-s <int>      : Frame skip, skips <int> frames every draw. Will speed up simulation visualization.\n");
	printf( "-i <filename> : Load points from this file instead of generating flow (can be \"stdin\" for stdin) (overrides all other settings)\n");
	
	
	printf( "\nOptions for either:\n");
	printf( "-c <filename> : Use map config, defaults to map.cfg (plain old square). Visualizer uses this to draw walls around points.\n");
	
	printf("\n\nEither -o (simulator) or -i (visualizer) must be set.\n");
	printf("\n\nSimulator requires power-of-4 cores (1, 4, 16, etc) for area subdivision.\n");
	
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
	unsigned int cells_rows = 0;
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
				//fprintf(stderr,"goal: row[%u] col[%u]\n",cells_rows,i);
				int check = (int)(cell - '0');
				//printf("---result: %u\n", check);

				if (check == (int) 3) {
					map_cfg->goal_col = i;
  					map_cfg->goal_row = cells_rows;

  					fprintf(stderr,"goal: row[%u] col[%u]\n",cells_rows,i);
				}
			}

			//fprintf(stderr,"cell rows: %u\n",cells_rows);
			cells_rows++;
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


bool isPowerOfFour(int n){
	// borrowed from http://www.geeksforgeeks.org/find-whether-a-given-number-is-a-power-of-4-or-not/
	if(n == 0){
		return 0;
	}
	while(n != 1){
		if(n%4 != 0){
			return 0;
		}
		n = n/4;
	}
	return 1;
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
	
	char *map_cfg_file = NULL;
	map_cfg_file = read_string( argc, argv, "-c", "map.cfg" );
	
	// Read map config by rank 0, process it, and broadcast it out
	struct map map_cfg = {0,0,0};
	if(rank == 0){
		FILE *fp = fopen(map_cfg_file, "r");
		read_map(fp, &map_cfg);
	}
	
	int num_particles = 2;
	
	if(input_file){
		
		if(rank == 0){
		
			unsigned int frame_skip = read_int( argc, argv, "-s", 0 );
			if(frame_skip){
				fprintf(stderr, "%s Skipping %u frames\n", VIZ_PREPEND, frame_skip);
			}
			
			FILE *fp = read_from_stdin ? stdin : fopen(input_file, "r");
			draw_data(fp, read_from_stdin, frame_skip, &map_cfg);
			if(!read_from_stdin){
				fclose(fp);
			}

		}
		
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Finalize();
		return 0;
	}
	
	// Won't get here if '-i' is provided.
	
	struct subdivision *areas = (struct subdivision *) malloc(n_proc * sizeof(struct subdivision));
	memset(areas, 0, n_proc * sizeof(struct subdivision));
	
	if(rank == 0){
		if(!isPowerOfFour(n_proc)){
			fprintf(stderr, "Must use a power-of-four number of cores (1, 4, 16, etc) for subdivision\n");
			usage();
			exit(0);
		}
		
		// calculate subdivisions for processing of specific areas
		int sqrt_proc = sqrt(n_proc);
		double range = 1.0 / sqrt_proc; // this makes it easy with 4 cores, just keep making squares (rectangles aren't AS easy)
		int core = 0;
		fprintf(stderr, "%s layout: \n", MPI_PREPEND);
		for(int row = 0; row < sqrt_proc; row++){
			
			for(int col = 0; col < sqrt_proc; col++){
				
				areas[core].min_x = col * range;
				areas[core].max_x = col * range + range;
				areas[core].min_y = row * range;
				areas[core].max_y = row * range + range;
				
				fprintf(stderr, "\t%i", core);
				core++;
			}
			fprintf(stderr, "\n");
		}
		
		// tests
		
//		fprintf(stderr,"test: %i -0.1\n", rank_for_location(-0.1, -0.1, n_proc, areas));
//		fprintf(stderr,"test: %i\n", rank_for_location(0.0, 0.1, n_proc, areas));
//		fprintf(stderr,"test: %i\n", rank_for_location(0.1, 0.1, n_proc, areas));
//		fprintf(stderr,"test: %i\n", rank_for_location(0.2, 0.1, n_proc, areas));
//		fprintf(stderr,"test: %i\n", rank_for_location(0.3, -0.1, n_proc, areas));
//		fprintf(stderr,"test: %i 0.4\n", rank_for_location(0.4, 0.1, n_proc, areas));
//		fprintf(stderr,"test: %i 0.5\n", rank_for_location(0.5, -0.1, n_proc, areas));
//		fprintf(stderr,"test: %i 0.6\n", rank_for_location(0.6, 0.1, n_proc, areas));
//		fprintf(stderr,"test: %i\n", rank_for_location(0.7, 0.1, n_proc, areas));
//		fprintf(stderr,"test: %i\n", rank_for_location(0.8, -0.1, n_proc, areas));
//		fprintf(stderr,"test: %i 0.9\n", rank_for_location(0.9, 0.1, n_proc, areas));
//		fprintf(stderr,"test: %i 1\n", rank_for_location(1.0, 0.1, n_proc, areas));
//		fprintf(stderr,"test: %i 1.1\n", rank_for_location(1.1, -0.1, n_proc, areas));
		
	}
	
	//MPI_Scatter(areas, 4, MPI_DOUBLE, &my_area, 4, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(areas, 4 * n_proc, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	
	struct subdivision *my_area = &(areas[rank]);
	fprintf(stderr, "%s Assigning rank %i to (%lf, %lf), (%lf, %lf)\n",MPI_PREPEND, rank, my_area->min_x, my_area->min_y, my_area->max_x, my_area->max_y);
	
//	MPI_Barrier(MPI_COMM_WORLD);
	
//	exit(0);
	
	// Read in the special agents
	char *input_agents = NULL;
	if(find_option(argc, argv, "-p") >= 0){
		input_agents = read_string( argc, argv, "-p", NULL );
	}
	
    int num_random_particles = read_int( argc, argv, "-r", (input_agents ? 0 : 2));
	
	
	char line [ 256 ];
	double t1=0.0, t2=0.0, t3=0.0, t4=0.0;

	int special_agents_count = read_int( argc, argv, "-y", 0);
	num_particles = num_random_particles + special_agents_count;
	double agents[special_agents_count][4];
	if(rank == 0){
		fprintf(stderr,"%s total: %i, special: %i, random: %i\n", MPI_PREPEND, num_particles, special_agents_count, num_random_particles);
	}
	
	// Do some basic argument checking
	if(rank == 0 && input_agents && special_agents_count == 0){
		fprintf(stderr, "Use of agent config requires -y for special agent count\n");
		usage();
		exit(1);
	}else if(rank == 0 && special_agents_count > 0 && !input_agents){
		fprintf(stderr, "Special agent count requires agent config via -p\n");
		usage();
		exit(1);
	}
	
	
	int row = 0;
	
	if(input_agents && rank == 0 && special_agents_count > 0){
		fprintf(stderr, "here\n");
		FILE *fp = fopen(input_agents, "r");
		
		while ( fgets ( line, sizeof(line), fp ) != NULL && row < special_agents_count ) {
			//printf("line: %s", line);
			
			char * pch = strtok (line," ");
			int agent_input_size = sscanf(pch, "%lf,%lf,%lf,%lf", &agents[row][0], &agents[row][1], 
				   &agents[row][2], &agents[row][3]);

			if (agent_input_size != 4) {
				printf("Error parsing file.\n");
				exit(0);
			}
			row++;
		}
		
		//int x, y ;
		//for(x=0; x < row; x++){
		//	for(y=0; y<4; y++){
		//		printf("Test[%d][%d]: %lf\n", x,y,agents[x][y]);
		//	}
		//}
		fclose(fp);
	}
	
	if(rank == 0 && row != special_agents_count){
		fprintf(stderr,"Read %i rows, but expected %i special agents.\n", row, special_agents_count);fflush(stderr);
		exit(0);
	}

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

	MPI_Datatype PARTICLE;
	int ints_per_particle = sizeof(particle_t) / sizeof(int);
	MPI_Type_contiguous( ints_per_particle, MPI_INT, &PARTICLE );
	MPI_Type_commit( &PARTICLE );
	
	MPI_Datatype MIN_PARTICLE;
	int ints_per_min_particle = sizeof(struct minimum_particle) / sizeof(int);
	MPI_Type_contiguous( ints_per_min_particle, MPI_INT, &MIN_PARTICLE );
	MPI_Type_commit( &MIN_PARTICLE );

	//
	//  set up the data partitioning across processors
	//
	
	if(!savename && !write_to_stdout){
		usage();
		exit(1);
	}
	
	//
	//  initialize and distribute the particles
	//
	set_size( num_particles, &map_cfg );
	int local_count;
	MPI_Status status;
	particle_t *local = NULL;
	int counts[n_proc], offsets[n_proc];
	
	if( rank == 0 ){
		init_particles( num_particles, special_agents_count, agents, particles, &map_cfg );
		
		particle_t *batches[n_proc];
		// figure out what particles belong to what cores
		for(int i = 0; i < n_proc; i++){
			// making room for enough for each core to receive ALL particles. Definitely overshooting, but it'll work later on when things are shared
			batches[i] = (particle_t *) malloc(num_particles * sizeof(particle_t));
			counts[i] = 0;
		}
		int index;
		particle_t *particle;
		for(int i = 0; i < num_particles; i++){
			particle = &particles[i];
			index = rank_for_location(particle->x, particle->y, n_proc, areas);
			memcpy(&batches[index][counts[index]], particle, sizeof(particle_t));
			
			fprintf(stderr,"%s Assigned particle at (%lf,%lf) to core %i\n", MPI_PREPEND,batches[index][counts[index]].x,batches[index][counts[index]].y,index );
			counts[index]++;
		}
		
		// now batches[i] holds array of counts[i] particles
		
		// as rank 0, just point locals pointer to array directly. done.
		local_count = counts[0];
		local = batches[0];
		
		for(int i = 1; i < n_proc; i++){
			
			MPI_Send(&counts[i], 1, MPI_INT, i, SEND_INITIAL_PARTICLE_COUNT, MPI_COMM_WORLD);
			MPI_Send(batches[i], counts[i], PARTICLE, i, SEND_INITIAL_PARTICLES, MPI_COMM_WORLD);
			
		}
		
	}else{
		MPI_Recv(&local_count, 1, MPI_INT, 0, SEND_INITIAL_PARTICLE_COUNT, MPI_COMM_WORLD, &status);
		local = (particle_t*) malloc( num_particles * sizeof(particle_t) );
		
		MPI_Recv(local, local_count, PARTICLE, 0, SEND_INITIAL_PARTICLES, MPI_COMM_WORLD, &status);
	}
	
	fprintf(stderr, "%s Rank %i got %i particles out of %i\n", MPI_PREPEND, rank, local_count, num_particles);
	if(local_count > 0){
		fprintf(stderr, "%s Rank %i point 0: (%lf, %lf) (of %i)\n", MPI_PREPEND, rank, local[0].x, local[0].y, local_count);
	}
	
	
	
	
	bool first = true;
	GLFWwindow *win = NULL;
	//Textures *tex;
	unsigned int *sphere_draw_ids;
	Vec<3> *points;
	Vec<3> *last_points;
	
	double radius = 15;
	double now = glfwGetTime();
	
	FILE *fsave = savename && rank == 0 ? (write_to_stdout ? stdout : fopen( savename, "w" )) : NULL;
	
    //
    //  simulate a number of time steps
    //
    double simulation_time = read_timer( );
	struct minimum_particle *minimum_particles = (struct minimum_particle *) malloc (num_particles * sizeof(struct minimum_particle));
	
	int nearby_core;
	int t, temp;
	particle_t *local_temp = (particle_t *) malloc (num_particles * sizeof(particle_t));
	particle_t *another_temp;
	
	int num_directions = 8;  // up, down, left, right, upleft, upright, downleft, downright
	int directions[num_directions];
	memset(directions, -1, num_directions);
	
	particle_t *to_send[n_proc];
	int to_send_counts[n_proc];
	for(int i = 0; i < n_proc; i++){
		to_send[i] = NULL;
		to_send_counts[i] = 0;
	}
	
	
    for( int step = 0; !timesteps || step < timesteps; step++ ){
		
		//
		//  compute all forces
		//
		for( int i = 0; i < local_count; i++ ){
			local[i].ax = local[i].ay = 0;
		}
		for( int i = 0; i < local_count-1; i++ ){
			for (int j = i+1; j < local_count; j++ ){
				fprintf(stderr,"%s rank %i computing between %i and %i of %i\n", MPI_PREPEND, rank, i, j, local_count);
				apply_force( local[i], local[j] );
			}
		}
		
		// compute forces against nearby ghost zones
		
		
		
		//
		//  move particles
		//
		t = 0;
		//fprintf(stderr, "%s rank %i starting with local_count at %i\n", MPI_PREPEND, rank, local_count);
		for( int i = 0; i < local_count; i++ ){
			move( local[i], &map_cfg );
			
			// check if this core should forget about this particle now.
			temp = rank_for_location(local[i].x, local[i].y, n_proc, areas);
			if(temp != rank){
				//fprintf(stderr,"%s rank %i forgot particle %i, at (%lf, %lf)\n", MPI_PREPEND, rank, i, local[i].x,local[i].y);
			}else{
				memcpy(&local_temp[t], &local[i], sizeof(particle_t));
				t++;
			}
		}
		
		//swap pointers, since we now have a condensed local_temp list of things to care about (no extra mallocing)
		another_temp = local;
		local = local_temp;
		local_temp = another_temp;
		
		local_count = t;
		for(int i = 0; i < local_count; i++){
			minimum_particles[i].x = local[i].x;
			minimum_particles[i].y = local[i].y;
			minimum_particles[i].color_r = local[i].color_r;
			minimum_particles[i].color_g = local[i].color_g;
			minimum_particles[i].color_b = local[i].color_b;
		}
		
		
		// tell root how many points each rank has
		MPI_Gather(&local_count, 1, MPI_INT, counts, 1, MPI_INT, 0, MPI_COMM_WORLD);
		
		if(rank == 0){
			for(int i = 0; i < n_proc; i++){
				offsets[i] = (i == 0 ? 0 : offsets[i-1] + counts[i-1]);
				//fprintf(stderr, "%s root says rank %i has %i mins, offset: %i\n", MPI_PREPEND, i, counts[i], offsets[i]);
			}
		}
		
		// send points to rank 0 to be written (only x,y & color)
		MPI_Gatherv(minimum_particles, local_count, MIN_PARTICLE, minimum_particles, counts, offsets, MIN_PARTICLE, 0, MPI_COMM_WORLD);
		
		if(rank == 0){
			save( fsave, num_particles, minimum_particles, &map_cfg );
		}
		
		
		// find particles nearby other cores:
		bool up = false, down = false, left = false, right = false;
		int recipient;
		for( int i = 0; i < local_count; i++ ){
			memset(directions, -1, num_directions * sizeof(int));
			
			// moving right:
			if(my_area->max_x - local[i].x < GHOST_ZONE_PADDING){
				right = true;
				directions[0] = rank_for_location(local[i].x + GHOST_ZONE_PADDING, local[i].y, n_proc, areas);
			}
			
			// moving left
			if(local[i].x - my_area->min_x < GHOST_ZONE_PADDING){
				left = true;
				directions[1] = rank_for_location(local[i].x - GHOST_ZONE_PADDING, local[i].y, n_proc, areas);
			}
			
			// moving up
			if(my_area->max_y - local[i].y < GHOST_ZONE_PADDING){
				up = true;
				directions[2] = rank_for_location(local[i].x, local[i].y + GHOST_ZONE_PADDING, n_proc, areas);
			}
			
			// moving down
			if(local[i].y - my_area->min_y < GHOST_ZONE_PADDING){
				down = true;
				directions[3] = rank_for_location(local[i].x, local[i].y - GHOST_ZONE_PADDING, n_proc, areas);
			}
			
			// moving up-right:
			if(up && right){
				directions[4] = rank_for_location(local[i].x + GHOST_ZONE_PADDING, local[i].y + GHOST_ZONE_PADDING, n_proc, areas);
			}
			
			// moving up-left
			if(up && left){
				directions[5] = rank_for_location(local[i].x - GHOST_ZONE_PADDING, local[i].y + GHOST_ZONE_PADDING, n_proc, areas);
			}
			
			// moving down-right:
			if(down && right){
				directions[6] = rank_for_location(local[i].x + GHOST_ZONE_PADDING, local[i].y - GHOST_ZONE_PADDING, n_proc, areas);
			}
			
			// moving down-left
			if(down && left){
				directions[7] = rank_for_location(local[i].x - GHOST_ZONE_PADDING, local[i].y - GHOST_ZONE_PADDING, n_proc, areas);
			}
			
			int sent_to[n_proc];
			memset(sent_to, 0, n_proc * sizeof(int));
			for(int j = 0; j < num_directions; j++){
				recipient = directions[j];
				if(recipient == -1 || recipient == rank || sent_to[recipient]){
					continue;
				}
				// ensure each core only gets one copy at worst
				sent_to[recipient] = 1;
				
				if(to_send[recipient] == NULL){
					// if i CAN send to rank t, make room for stuff.
					to_send[recipient] = (particle_t *) malloc(local_count * sizeof(particle_t));
				}
				//fprintf(stderr, "%s rank %i sending (%lf,%lf) direction %i to recipient %i\n", MPI_PREPEND, rank, local[i].x, local[i].y, j, recipient);
				memcpy(&to_send[recipient][to_send_counts[recipient]], &local[i], sizeof(particle_t));
				to_send_counts[recipient]++;
			}
		}
		
		// send stuff around
		
		int to_receive[n_proc];
		memset(to_receive, 0, n_proc * sizeof(int));
		MPI_Request recv_request[n_proc];
		MPI_Request particle_recv_request[n_proc];
		for(int i = 0; i < n_proc; i++){
			// Set up receives first, then sends
			
			MPI_Irecv(&to_receive[i], 1, MPI_INT, i, SEND_INITIAL_PARTICLE_COUNT, MPI_COMM_WORLD, &recv_request[i]);
		}
		for(int i = 0; i < n_proc; i++){
			MPI_Send(&to_send_counts[i], 1, MPI_INT, i, SEND_INITIAL_PARTICLE_COUNT, MPI_COMM_WORLD);
			if(to_send_counts[i] > 0){
				fprintf(stderr, "%s rank %i prepping to send %i to %i\n", MPI_PREPEND, rank, to_send_counts[i], i);fflush(stderr);
			}
		}
		
		for(int i = 0; i < n_proc; i++){
			if(to_receive[i] > 0){
				MPI_Irecv(& (local[local_count]), to_receive[i], PARTICLE, i, SEND_INITIAL_PARTICLES, MPI_COMM_WORLD, &particle_recv_request[i] );
				fprintf(stderr, "%s rank %i prepping to receive %i from %i\n", MPI_PREPEND, rank, to_receive[i], i);fflush(stderr);
				local_count += to_receive[i];
			}
		}
		
		for(int i = 0; i < n_proc; i++){
			if(to_send_counts[i] > 0){
				MPI_Send(to_send[i], to_send_counts[i], PARTICLE, i, SEND_INITIAL_PARTICLES, MPI_COMM_WORLD);
				fprintf(stderr, "%s rank %i sent %i to %i, (%lf,%lf)\n", MPI_PREPEND, rank, to_send_counts[i], i, to_send[i][0].x, to_send[i][0].y);fflush(stderr);
			}
		}
		MPI_Barrier(MPI_COMM_WORLD);
		
		if(local_count > 0){
			fprintf(stderr, "%s rank %i newest: (%lf,%lf)\n", MPI_PREPEND, rank, local[local_count-1].x, local[local_count-1].y);fflush(stderr);
		}
		// reset buffers
		for(int i = 0; i < n_proc; i++){
			if(to_send[i]){
				free(to_send[i]);
				to_send[i] = NULL;
			}
			to_send_counts[i] = 0;
		}
		//fprintf(stderr,"%s Rank %i finished %i\n",MPI_PREPEND, rank, step);
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
	
    free( local );
    free( particles );
    if( fsave )
        fclose( fsave );
    
    MPI_Finalize( );
    
    return 0;
}
