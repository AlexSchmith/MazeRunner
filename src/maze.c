/*
	Author: Alex Schmith
	Email: aschmith2019@my.fit.edu
	Date:

	Description: This is a game where the objective is to get to the finish *
	while not getting eaten by monsters. # are obstacles that both players and creatures
	cannot pass through and 0 is the player. The creatures can be any letter from A-Z. 


*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_CREATURES 10
#define INFINITE 100000

typedef enum result {
	NOT_END,
	PL_WON,
	PL_LOST
} RESULT;

typedef struct map_node {
	char cell; //character for map either 0 for player, A-Z for a creature, * for the end, and # for an obstacle
	char replacement; // character for two items on the same 
	char label; //either unvisited 'u' or visited 'v' and automatically set to 'u'
	char dist_lab; //either completed 'c' or not completed 'n'
	int weight_u, weight_d, weight_l, weight_r;
	int pos[2];
	int distance;
} MAP_NODE;

typedef struct map_tree {
	MAP_NODE map_node;
	struct map_tree *parent;
} TREE;

typedef struct map_queue {
	MAP_NODE map_node;
	TREE *tree_node;
	struct map_queue *next;
} QUEUE;

typedef struct stack {
	int pos[2];
	struct stack *next;
} STACK;


/*
	Variables:
		* map is the game map 
		* creatures is a two dimensional array that carries the coordinates of all of the creatures
		* player carries the coordinates of the player on the map
		* end carries the coordinates of the finish
*/




//Data Structure Functions
QUEUE *insert(QUEUE *queue, MAP_NODE map, TREE *tree_node);
QUEUE *enqueue(QUEUE *queue, MAP_NODE map_node, TREE *tree_node);
QUEUE *update(QUEUE *queue, QUEUE *node);
QUEUE dequeue(QUEUE **queue);
STACK *push(STACK *stack, int x, int y);
int *pop(STACK **stack);
//Game Functions
void create_map(FILE *graph, int row, int col, MAP_NODE map[row][col], int creatures[MAX_CREATURES][2], int player[], int end[], int *num_creatures);
void print_map(int row, int col, MAP_NODE map[row][col]);
RESULT move(int row, int col, MAP_NODE map[row][col], int position[], int creatures[MAX_CREATURES][2], int end[], char choice, int num_creatures);
RESULT round(int row, int col, MAP_NODE map[row][col], int end[], int creatures[MAX_CREATURES][2], int play_pos[], int num_creatures);
STACK* shortest_path(int row, int col, MAP_NODE map[row][col], int start_pos[], char target);
void set_weights(int row, int col, MAP_NODE map[row][col], int player[]);

int main(int argc, char *argv[]){
	int player[2];
	int end[2];
	int creatures[MAX_CREATURES][2];
	int num_creatures = 0;
	int col, row;
	char cont = 'y';

	FILE *graph = fopen(argv[1], "r");
	RESULT is_end = NOT_END;

	fscanf(graph, "%d %d\n", &row, &col);

	MAP_NODE map[row][col];
	create_map(graph, row, col, map, creatures, player, end, &num_creatures);

	while (is_end == NOT_END)
		is_end = round(row, col, map, end, creatures, player, num_creatures);

	if (is_end != 0){
		if (is_end == PL_WON)
			printf("Player 0 beats the hungry creatures!\n");
		else if (is_end == PL_LOST)
			printf("One creature is not hungry anymore!\n");
	}
	return 0;
}


//Data Structure Functions

/*
	function: this is a function that inserts an element into a priority queue
	parameters: the head of the priority queue, a map node, and a tree node.
	return: returns the head of the priority queue
*/
QUEUE *insert(QUEUE *queue, MAP_NODE map, TREE *node){
	QUEUE *new_node = (QUEUE *)malloc(sizeof(QUEUE));
	QUEUE *temp = NULL;

	new_node->map_node = map;
	new_node->tree_node = node;
	new_node->next = NULL;

	if (queue == NULL)
	{
		queue = new_node;
	}
	else if (queue->map_node.distance > new_node->map_node.distance)
	{
		new_node->next = queue;
		queue = new_node;
	}
	else
	{
		temp = queue;

		while (temp->next != NULL && temp->next->map_node.distance <= new_node->map_node.distance)
		{
			temp = temp->next;
		}

		new_node->next = temp->next;
		temp->next = new_node;
	}

	return queue;
}

/*
	function: this is a function that finds a specific node and deletes  it from the priority queue
	parameters: the head of the priority queue, and the node to be deleted
	returns: the head of the priority queue
*/
QUEUE *delete(QUEUE *queue, QUEUE *node){
	QUEUE *insert = queue;
	QUEUE *temp = queue;
	int dist = node->map_node.distance;

	if (queue == NULL || node == NULL)
	{
		return queue;
	}
	else if (queue == node)
	{
		queue = queue->next;
		free(node);
		node = NULL;
	}
	else
	{
		//find and remove node from list
		while (temp->next != NULL && temp->next != node)
			temp = temp->next;

		temp->next = node->next;

		free(node);
		node = NULL;
	}

	return queue;
}

/*
	function: this is a function that adds a pair of coordinates to a stack.
	it will be used to reverse the direction of the coordinates for the shortest path algorithm
	parameters: stack, x coordinate, y coordinate
*/
STACK *push(STACK *stack, int x, int y){
	STACK *new_node = (STACK *)malloc(sizeof(STACK));
	STACK *temp = stack;

	if (new_node == NULL)
	{
		printf("errror\n");
	}

	new_node->next = NULL;
	new_node->pos[0] = x;
	new_node->pos[1] = y;

	if (stack == NULL)
	{
		stack = new_node;
	}
	else
	{
		new_node->next = stack;
		stack = new_node;
	}

	return stack;
}

/*
	function: this is a stack implementation that returns a pair of coordinates and removes it from the top of the stack
	parameters: stack
	returns: the position of the top stack and returns new stack indirectly
*/
int *pop(STACK **stack){
	STACK *temp = *stack;
	int value[2];

	value[0] = (*stack)->pos[0];
	value[1] = (*stack)->pos[1];

	*stack = (*stack)->next;
	free(temp);
	temp = NULL;

	return temp;
}

/*
	function: a queue implementation used for the breadth first search and dijkstra's algorithm that contains bot the map node and a parent tree node
	parameters: the queue, a map node, a tree node
	returns: the queue of the order which to visit the nodes
*/
QUEUE *enqueue(QUEUE *queue, MAP_NODE map_node, TREE *tree_node){
	QUEUE *new_node = (QUEUE *)malloc(sizeof(QUEUE));
	QUEUE *temp = NULL;

	if (new_node == NULL)
	{
		printf("Error allocating memory\n");
	}

	new_node->next = NULL;
	new_node->map_node = map_node;
	new_node->tree_node = tree_node;

	temp = queue;

	if (queue == NULL)
	{
		queue = new_node;
	}
	else
	{
		while (temp->next != NULL)
		{
			temp = temp->next;
		}
		temp->next = new_node;
	}

	return queue;
}

/*
	function: this is a function that dequeues for a queue implementation
	parameters: a pointer to the queue
	returns: returns the queue value at the front and the new queue through indirect referencing
*/
QUEUE dequeue(QUEUE **queue){
	QUEUE *temp_q = *queue;
	QUEUE ret_val = **queue;

	*queue = (*queue)->next;
	free(temp_q);
	temp_q = NULL;

	return ret_val;
}


// Game Functions

/*
	function: this function records a map and sets the defualt weights to 1
	parameters: the file for the map, a double array for the positions of all creatures, the position for the player, the position of the end, and the number of creatures
	returns: returns the game map, and through indirect referencing returns the positions for the player, the creatures, and the end
*/
void create_map(FILE *graph, int row, int col, MAP_NODE map[row][col], int creatures[MAX_CREATURES][2], int player[], int end[], int *num_creatures){
	*num_creatures = 0;
	STACK *path = NULL;
	STACK *temp;
	QUEUE *queue = NULL;
	QUEUE *temp_q;
	TREE *filler = NULL;
	char cell;
	int index = 0;
	int is_on_path, is_on_path_u, is_on_path_d, is_on_path_l, is_on_path_r;
	int weight = 1;

	//create each map node
	for (int i = 0; i < row; i++)
	{
		for (int k = 0; k < col; k++)
		{
			map[i][k].cell = '\0';
			fscanf(graph, "%c", &cell);

			map[i][k].cell = cell;
			map[i][k].label = 'u';
			map[i][k].dist_lab = 'n';
			map[i][k].pos[0] = i;
			map[i][k].pos[1] = k;
			map[i][k].replacement = ' ';
			map[i][k].distance = INFINITE;

			if (i == 0 || k == 0 || i == row - 1 || k == col - 1)
				weight = -1;
			else
				weight = 1;

			map[i][k].weight_u = weight;
			map[i][k].weight_d = weight;
			map[i][k].weight_l = weight;
			map[i][k].weight_r = weight;

			//set positions for player, end, and creatures
			if (map[i][k].cell == '0')
			{
				player[0] = i;
				player[1] = k;
			}
			else if (map[i][k].cell == '*')
			{
				end[0] = i;
				end[1] = k;
			}
			else if (map[i][k].cell == '#')
			{
				map[i][k].distance = -1;
			}
			else if (isalpha(map[i][k].cell) && isupper(map[i][k].cell))
			{
				index = 0;

				if (*num_creatures != 0)
				{
					while (index < *num_creatures && cell > map[creatures[index][0]][creatures[index][1]].cell)
					{
						index++;
					}
				}
				if (*num_creatures >= MAX_CREATURES)
					continue;

				for (int i = *num_creatures; i >= index; i--)
				{
					creatures[i][0] = creatures[i - 1][0];
					creatures[i][1] = creatures[i - 1][1];
				}

				creatures[index][0] = i;
				creatures[index][1] = k;
				(*num_creatures)++;
			}
		}
		fscanf(graph, "\n");
	}

	//set boundaries for the graph
	for (int i = 0; i < row; i++)
	{
		for (int k = 0; k < col; k++)
		{
			if (i == 0 || map[i - 1][k].cell == '#')
				map[i][k].weight_u = -1;
			if (i == row - 1 || map[i + 1][k].cell == '#')
				map[i][k].weight_d = -1;
			if (k == 0 || map[i][k - 1].cell == '#')
				map[i][k].weight_l = -1;
			if (k == col - 1 || map[i][k + 1].cell == '#')
				map[i][k].weight_r = -1;
		}
	}

	fclose(graph);
}

/*
	function: this is a function that prints the 2d array for the map
	parameters: map of characters, size of map row, size of map column
*/
void print_map(int row, int col, MAP_NODE map[row][col]){}

/*
	function: this is a function that moves a piece on the board
	parameters:  map of characters, size of map row, size of map column, position of character to move, choice of movement
	returns: returns the result of the move either player won, player lost, or not end
*/
RESULT move(int row, int col, MAP_NODE map[row][col], int position[], int creatures[MAX_CREATURES][2], int end[], char choice, int num_creatures){}

/*
	function: this is a function that plays one round where the player moves, then all the creatures move in alphabetical order
	parameters: map of characters, size of map row, size of map column
	returns: returns an array of graph nodes that contain the player, the creature, and the end
*/
RESULT round(int row, int col, MAP_NODE map[row][col], int end[], int creatures[MAX_CREATURES][2], int player[], int num_creatures){}

/*
	function: this is a function that uses breadth first search to find the shortest path from position to node with target. it works by putting items into a queue based on their level.
	when the target is reached, the path is recorded by a tree and the first path to reach the target is the shortest path	
	parameters: size of map row, size of map column, map , position of starting object, target of object to be found
	returns: the directions the object needs to follow for the shortest path in char format (u, d, l, r)
*/
STACK* shortest_path(int row, int col, MAP_NODE map[row][col], int object[], char target){}

/*
	function: this function sets the specific weights for the map by finding the shortest path from the player to the end and then using dijkstra's algorithm
	parameters: size of map row, size of map column, map, position of player
	returns: the map

*/
void set_weights(int row, int col, MAP_NODE map[row][col], int player[]){}

/*
	function: this is a function that finds the shortest path by using dijkstra's algorithm which is similar to the breadth first search algorithm except the way the priority queue is organized
	parameters: size of map row, size of map column, map, position of starting object, target
	return: returns the char of choice for the creature
*/
char dijkstra_path(int row, int col, MAP_NODE map[row][col], int object[], char target){}

 

























