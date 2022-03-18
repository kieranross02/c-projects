

#pragma once

/* Preprocessor directives to define macros */
#define MAZE1     "maze1.txt"
#define MAZE2     "maze2.txt"
#define MAZE3     "maze3.txt"
#define MAZE119   "maze119.txt"
#define MAZE3877  "maze3877.txt"
#define MAZET     "mazet.txt"
#define MAZE_WALL '*'
#define VISITED   'Y'
#define UNVISITED 'N'
#define BUFFER    128

#include <stdio.h>
#include <stdlib.h>

/* Structure used as the cell for the maze representation */
typedef struct maze_cell {
  char character;
  char visited;
} maze_cell; 

/* Function prototypes */
void         print_generated_paths ( char** pathset, int numpaths ); /* not implemented, can be used for debugging if needed */
maze_cell**  parse_maze            ( FILE* maze_file, int dimension );
int          get_maze_dimension    ( FILE* maze_file );
void         generate_all_paths    ( char*** pathsetref, int* numpathsref, maze_cell** maze, int dimension, int row, int column, char* path );
int          path_cost             ( char* path_string );
void	     construct_shortest_path_info ( char** pathset, int numpaths, char* outputbuffer );
void		 construct_cheapest_path_info ( char** pathset, int numpaths, char* outputbuffer );
void         process               ( );