/*
 File:				mazesolver.c
 Author:			  Alex Gu and Kieran Ross
 Student #s:		89802680 and 25550245
 CWLs:      		alexgu6 and ktmross
 Date:				Nov 21, 2021
 */

 
#define _CRT_SECURE_NO_WARNINGS

/* Preprocessor directives to include libraries */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mazesolver.h"

/*
 Contains the majority of the maze solver logic
 PRE:       The file specified by the MACRO in lines 47 and 60 exists in the project
 POST:      Prints data about maze solutions.
 RETURN:    None
 */
void process()
{
  /* Variables */
  int dimension = 0;
  FILE* maze_file = NULL;
  maze_cell** maze = NULL;
  char outputstring[BUFFER]; // holds output for shortest/cheapest path

  /* Declare variables for holding generated path information */
  char** paths = NULL;
  int paths_found = 0;

  /* Opens and parses the maze file.  Replace the first parameter of fopen with
    different file names defined in the preprocessor section of the header file
    to test your mazesolver with our sample mazes. */
  maze_file = fopen(MAZET, "r");

  if (maze_file) {

    /* Calls the functions that:
      a) get the size of the maze and stores it in the dimension variable
      b) copies the maze into memory */
      // INSERT YOUR CODE HERE (2 lines)
	  dimension = get_maze_dimension(maze_file);
	  maze = parse_maze(maze_file, dimension);

  }
  else {
    fprintf(stderr, "Unable to parse maze file: %s\n", MAZE1);
    system("pause");
  }

  /* Traverses maze and generates all solutions */
  // INSERT YOUR CODE HERE (1 line)
  generate_all_paths(&paths, &paths_found, maze, dimension, 0, 0, "");

  /* Calculates and displays required data */
  char temp[BUFFER];
  sprintf(outputstring, "Total number of solutions: %i\n\0", paths_found);
  construct_shortest_path_info(paths, paths_found, &temp);
  strcat(outputstring, temp);
  construct_cheapest_path_info(paths, paths_found, &temp);
  strcat(outputstring, temp);
  printf("%s", outputstring);
}

/*
 Acquires and returns the maze size.  Since the maze is always a square, all we
 need to do is find the length of the top row!
 PARM:      maze_file is a pointer to a filestream
 PRE:       maze_file is an initialized pointer to a correctly-formatted maze file
 POST:      maze_file's internal pointer is set to beginning of stream
 RETURN:    length of the first line of text in the maze file EXCLUDING any EOL characters
            ('\n' or '\r') and EXCLUDING the string-terminating null ('\0') character.
 */
int get_maze_dimension( FILE* maze_file )  {

  int  dimension = 0;
  char line_buffer[BUFFER];

	dimension = strlen( fgets ( line_buffer, BUFFER, maze_file ) );

	/* You don't need to know this.  It 'resets' the file's internal pointer to the
	   beginning of the file. */
	fseek( maze_file, 0, SEEK_SET );

    /* Checks if text file was created in Windows and contains '\r'
	   IF TRUE reduce strlen by 2 in order to omit '\r' and '\n' from each line
	   ELSE    reduce strlen by 1 in order to omit '\n' from each line */
  if ( strchr( line_buffer, '\r' ) != NULL ) {
    // INSERT CODE HERE (1 line)
	  return (dimension - 2);
  } else {
    // INSERT CODE HERE (1 line)
	  return (dimension - 1);
  }
}

/*
 Parses and stores maze as a 2D array of maze_cell.  This requires a few steps:
 1) Allocating memory for a 2D array of maze_cell, e.g., maze_cell[rows][columns]
    a) Dynamically allocates memory for 'dimension' pointers to maze_cell, and assign
	   the memory (case as a double pointer to maze_cell) to maze, which is a
	   double pointer to maze_cell (this makes the maze[rows] headers)
	b) For each row of the maze, dynamically allocate memory for 'dimension' maze_cells
	   and assign it (cast as a pointer to maze_cell) to maze[row]
 2) Copying the file to the allocated space
    a) For each row obtained from the file using fgets and temporarily stored in line_buffer
	   i) For each of the 'dimension' columns in that row
	   ii)Assign the character from the file to the struct, and set the struct to unvisited
 3) Returns the double pointer called maze.
 PARAM:  maze_file pointer to an open filestream
 PARAM:  dimension pointer to an int
 PRE:    maze_file is a pointer to a correctly-formatted maze file
 POST:   dimension contains the correct size of the square maze
 POST:   maze contains a dynamically allocated copy of the maze stored in the maze_file
 RETURN: maze, a maze_cell double pointer, which points to 'dimension' single pointers
         to maze_cell, each of which points to 'dimension' maze_cell structs.
 */
maze_cell** parse_maze( FILE* maze_file, int dimension )
{
	/* Variables */
  char        line_buffer[BUFFER];
  int         row = 0;
	int         column = 0;
	maze_cell** maze = NULL; 

  /* Allocates memory for correctly-sized maze */
	maze = (maze_cell**)calloc(dimension, sizeof(maze_cell*));

  for ( row = 0; row < dimension; ++row ) {
	  maze[row] = (maze_cell*)calloc(dimension, sizeof(maze_cell));
  }

  /* Copies maze file to memory */
	row = 0;
  while ( fgets ( line_buffer, BUFFER, maze_file ) ) {
    for ( column = 0; column < dimension; ++column ) {
		maze[row][column].character = line_buffer[column];
		maze[row][column].visited = 'N';
	  }
    row++;
  }
	return maze;
}

/**
 Generates all paths through a maze recursively.
 PARAM:     pointer to a 2D array of all generated paths
            (pass the 2D array by reference - actual parameter to be modified by function)
 PARAM:     pointer to number of paths found
            (pass the integer by reference - actual parameter to be modified by function)
 PARAM:     pointer to a 2D array of maze_cell
 PARAM:     dimension of the square maze
 PARAM:     starting position row
 PARAM:     starting position column
 PARAM:     pointer to a string of chars which contains the 'current' path
 PRE:       maze contains a representation of the maze
 PRE:       dimension contains the correct dimension of the maze
 PRE:       row contains a valid row coordinate
 PRE:       column contains a valid column coordinate
 PRE:       path contains a sequence of characters (the first time you invoke this
            function, the passed parameter should be an empty string, e.g., "" (not NULL)
 POST:      IF current coordinate is not out of maze bounds &&
               current coordinate is not a wall
            THEN path contains current coordinate
 POST:      IF current coordinate is at maze finish line (right boundary)
            THEN paths contains the path from start to finish.
 POST:      dereferenced pathset contains all paths found
 POST:      dereferences numpaths contains the number of paths found
 */
void generate_all_paths( char*** pathsetref, int* numpathsref, maze_cell** maze, int dimension, int row, int column, char* path )
{
	/* Variables */
	int path_length   = 0;
	char* new_point  = NULL;
	char* new_path   = NULL;

  /* Checks for base cases */
	if (row >= dimension || row < 0 || column < 0) {
    return;
	}
	else if (maze[row][column].visited == 'Y' || maze[row][column].character == '*') {
		return;
	}

  /* Otherwise deals with the recursive case.  Pushes the current coordinate onto the path
	  and checks to see if the right boundary of the maze has been reached
	  IF   right boundary reached
	  THEN path is added to the list as a successful path and function returns
	  ELSE the current location is marked as used and the search continues
	      in each cardinal direction using a recursive call using these steps:
		1. get length of path
		2. allocate space for a larger new path
		3. allocate space for a new point to add to the path
		4. assign the value in the maze cell to the new point
		5. concatenate old path to new path
		6. concatenate new point to new path */	
  else {
	  path_length = strlen( path );
	  new_path = ( char * ) calloc( path_length + 2, sizeof( char ) );
	  new_point = ( char * ) calloc( 2, sizeof( char ) );
	  new_point[0] = maze[row][column].character;
	  if ( path_length ) {
		  new_path = strcat( new_path, path );
	  }
	  new_path = strcat( new_path, new_point );
    free(new_point);

    if ( column == ( dimension - 1 ) ) {
	    /* 1. Reallocate memory in global paths array to make room
			    for a new solution string
			 2. Copy the solution path to the location of new string
			 3. Increment paths counter */
	    *pathsetref = ( char** ) realloc ( *pathsetref, ( (*numpathsref) + 1 ) * sizeof( char* ) );
		(*pathsetref)[*numpathsref] = ( char* ) calloc( strlen( new_path ) + 1, sizeof( char ));
	    strcpy( (*pathsetref)[*numpathsref], new_path );
	    (*numpathsref)++;
      return;
    } else {
		  /* 1. Mark point as visited
			   2. Recursively search in each direction using the new path, and the same pathsetref and numpathsref
			   3. Mark point as unvisited */
		maze[row][column].visited = 'Y';
		generate_all_paths(pathsetref, numpathsref, maze, dimension, ++row, column, new_path);
		generate_all_paths(pathsetref, numpathsref, maze, dimension, --row, column, new_path);
		generate_all_paths(pathsetref, numpathsref, maze, dimension, row, ++column, new_path);
		generate_all_paths(pathsetref, numpathsref, maze, dimension, row, --column, new_path);
		maze[row][column].visited = 'N';
		  return;
    }
  }
}

/*
 Calculates the cost for a given path.
 Examples:
 ""    -> 0
 "2"   -> 2
 "871" -> 16
 PARM:   path_string is a pointer to an array of char
 PRE:    path_string is a pointer to a null-terminated array of char (a string)
 RETURN: the 'cost' of the path.
 */
int path_cost ( char* path_string )
{
  int cost = 0;
  int value = 0;
  if (path_string == NULL) {
	  return 0;
  }
  else {
	  int length = strlen(path_string);
	  for (int i = 0; i < length; i++) {
		  //sscanf(&path_string[i], &value);
		  value = path_string[i] - '0';
		  cost += value;
	  }
	  return cost;
  }
}

/*
 Writes the shortest path in the paths global variable into the outputbuffer parameter,
 where the shortest path has the fewest cells.
 In the event of a tie, use the first 'shortest' path found.
 Uses this format:
 "Shortest path: XXXXXXXX"
 Don't forget to add a null character at the end.
 PARAM: 2D array containing all paths found
 PARAM: number of paths in pathset
 PARAM: outputbuffer is a character array of sufficient length
 POST:  outputbuffer contains information about the shortest path
 */
void construct_shortest_path_info(char** pathset, int numpaths, char* outputbuffer)
{
	//int i = numpaths - 1; //utlize this as index
	//if(strlen(*pathset))
	int shortest_path = 0;
	for (int i = 0; i < numpaths; i++) {
		if (strlen(pathset[i]) < strlen(pathset[shortest_path])) {
			shortest_path = i;
		}
	}
	sprintf(outputbuffer, "Shortest path: %s\n\0", pathset[shortest_path]);
}

/*
 Writes the cheapest path in the paths global variable into the outputbuffer parameter,
 where the cheapest path has the lowest sum value of its cells.
 In the event of a tie, use the first 'cheapest' path found.
 Uses this format:
 "Cheapest path: XXXXXXXX
  Cheapest path cost: 9999"
 Don't forget to use a newline and to add a null character at the end.
 Use sprintf to put an integer into a buffer which can then be concatenated using strcat.
 PARAM: 2D array containing all paths found
 PARAM: number of paths in pathset
 PARAM: outputbuffer is a character array of sufficient length
 POST:  outputbuffer contains information about the cheapest path and its cost
 */
void construct_cheapest_path_info ( char** pathset, int numpaths, char* outputbuffer )
{
	int cheapest_path = 0;
	for (int i = 0; i < numpaths; i++) {
		if (path_cost(pathset[i]) < path_cost(pathset[cheapest_path])) {
			cheapest_path = i;
		}
	}
	//char* second_line = "";
	sprintf(outputbuffer, "Cheapest path: %s\nCheapest path cost: %i\n\0", pathset[cheapest_path], path_cost(pathset[cheapest_path]));
}

/* End of file */