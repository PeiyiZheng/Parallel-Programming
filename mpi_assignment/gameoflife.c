// gameoflife.c
// Name: Peiyi Zheng
// JHED: pzheng4

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "mpi.h"

#define DEFAULT_ITERATIONS 64
#define GRID_WIDTH  256
#define DIM  16     // assume a square grid

int count_neighbors(int* region, int prev_row, int prev_col,
                                 int cur_row, int cur_col,
                                 int next_row, int next_col) {
  int cnt = 0;
  cnt += region[prev_row * DIM + prev_col];
  cnt += region[prev_row * DIM + cur_col];
  cnt += region[prev_row * DIM + next_col];
  cnt += region[cur_row * DIM + prev_col];
  cnt += region[cur_row * DIM + next_col];
  cnt += region[next_row * DIM + prev_col];
  cnt += region[next_row * DIM + cur_col];
  cnt += region[next_row * DIM + next_col];

  return cnt;
}

int main ( int argc, char** argv ) {

  int global_grid[ 256 ] = 
   {0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

  // MPI Standard variable
  int num_procs;
  int ID, i, j;
  int iters = 0;
  int num_iterations;

  // Setup number of iterations
  if (argc == 1) {
    num_iterations = DEFAULT_ITERATIONS;
  }
  else if (argc == 2) {
    num_iterations = atoi(argv[1]);
  }
  else {
    printf("Usage: ./gameoflife <num_iterations>\n");
    exit(1);
  }

  // Messaging variables
  MPI_Status stat;
  // TODO add other variables as necessary

  // MPI Setup
  if ( MPI_Init( &argc, &argv ) != MPI_SUCCESS )
  {
    printf ( "MPI_Init error\n" );
  }

  MPI_Comm_size ( MPI_COMM_WORLD, &num_procs ); // Set the num_procs
  MPI_Comm_rank ( MPI_COMM_WORLD, &ID );

  assert ( DIM % num_procs == 0 );

  // TODO Setup your environment as necessary
  // IDs for message passing
  int cur = ID;
  int prev = cur != 0 ? cur - 1 : num_procs - 1;
  int next = (cur + 1) % num_procs;

  // Slicing the array horizontally
  int slice_size = GRID_WIDTH / num_procs;
  int *slice = malloc(slice_size * sizeof(int)), *updated_slice = malloc(slice_size * sizeof(int));
  for (i = 0; i < slice_size; ++i) {
    slice[i] = global_grid[cur * slice_size + i];
  }

  // Arrays for message passing
  int cur_to_prev[DIM], cur_to_next[DIM];
  int prev_to_cur[DIM], next_to_cur[DIM];

  //  Region for each processor:
  //  prev_to_cur -- DIM
  //  slice       -- slice_size
  //  next_to_cur -- DIM
  int region_size = slice_size + DIM * 2;
  int *region = malloc(region_size * sizeof(int));

  for ( iters = 0; iters < num_iterations; iters++ ) {
    // TODO: Add Code here or a function call to you MPI code
    
    // Single processor: serial code -- test the correctness of GoL implementation
    if (num_procs == 1) {
      for (i = 0; i < DIM; ++i) {
        for (j = 0; j < DIM; ++j) {
          int living_neighbors = count_neighbors(slice, i == 0 ? DIM - 1 : i - 1, j == 0 ? DIM - 1 : j - 1,
                                                        i, j,
                                                        i == DIM - 1 ? 0 : i + 1, j == DIM - 1 ? 0 : j + 1);
          
          if (slice[i * DIM + j] == 0 && living_neighbors == 3) {
            updated_slice[i * DIM + j] = 1;
          }
          else if (slice[i * DIM + j] == 1 && (living_neighbors == 2 || living_neighbors == 3)) {
            updated_slice[i * DIM + j] = 1;
          }
          else {
            updated_slice[i * DIM + j] = 0;
          }
        }
      }

      // Update local slice
      for (i = 0; i < GRID_WIDTH; ++i) {
        slice[i] = updated_slice[i];
        global_grid[i] = updated_slice[i];
      }
    }
    // Multiple processors:
    else {
      for (i = 0; i < DIM; ++i) {
        cur_to_prev[i] = slice[i];
        cur_to_next[i] = slice[slice_size - DIM + i];
      }

      // 0 <--> 1 <--> 2 <--> 3 <--> 0
      // Rule: processors with even IDs send first
      if (cur % 2 == 0) {
        // data -->
        MPI_Send(&cur_to_next, DIM, MPI_INT, next, 0, MPI_COMM_WORLD);
        MPI_Recv(&prev_to_cur, DIM, MPI_INT, prev, 0, MPI_COMM_WORLD, &stat);


        // <-- data
        MPI_Send(&cur_to_prev, DIM, MPI_INT, prev, 1, MPI_COMM_WORLD);
        MPI_Recv(&next_to_cur, DIM, MPI_INT, next, 1, MPI_COMM_WORLD, &stat);
      }
      else {
        // --> data
        MPI_Recv(&prev_to_cur, DIM, MPI_INT, prev, 0, MPI_COMM_WORLD, &stat);
        MPI_Send(&cur_to_next, DIM, MPI_INT, next, 0, MPI_COMM_WORLD);

        // data <--
        MPI_Recv(&next_to_cur, DIM, MPI_INT, next, 1, MPI_COMM_WORLD, &stat);
        MPI_Send(&cur_to_prev, DIM, MPI_INT, prev, 1, MPI_COMM_WORLD);
      }

      // Merge data
      for (i = 0; i < DIM; ++i) {
        region[i] = prev_to_cur[i];
      }

      for (i = 0, j = DIM; i < slice_size; ++i, ++j) {
        region[j] = slice[i];
      }

      for (i = 0, j = DIM + slice_size; i < DIM; ++i, ++j) {
        region[j] = next_to_cur[i];
      }

      // GoL:
      for (i = 0; i < slice_size; ++i) {
        int row = (i / DIM) + 1, col = i % DIM;

        int living_neighbors = count_neighbors(region, row - 1, col == 0 ? DIM - 1 : col - 1,
                                                       row, col,
                                                       row + 1, col == DIM - 1 ? 0 : col + 1);

        if (slice[i] == 0 && living_neighbors == 3) {
          updated_slice[i] = 1;
        }
        else if (slice[i] == 1 && (living_neighbors == 2 || living_neighbors == 3)) {
          updated_slice[i] = 1;
        }
        else {
          updated_slice[i] = 0;
        }
      }

      // Gather the updated data to the master process: ID * slice_size = position in global_grid
      MPI_Gather(updated_slice, slice_size, MPI_INT, global_grid, slice_size, MPI_INT, 0, MPI_COMM_WORLD);

      // Update local slice
      for (i = 0; i < slice_size; ++i) {
        slice[i] = updated_slice[i];
      }
    }

    // Output the updated grid state
    if ( ID == 0 ) {
      printf ( "\nIteration %d: final grid:\n", iters );
      for ( j = 0; j < GRID_WIDTH; j++ ) {
        if ( j % DIM == 0 ) {
          printf( "\n" );
        }
        printf ( "%d  ", global_grid[j] );
      }
      printf( "\n" );
    }
  }

  // TODO: Clean up memory
  free(slice);
  free(updated_slice);
  free(region);
  MPI_Finalize(); // finalize so I can exit
}