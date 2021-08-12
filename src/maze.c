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
void print_map(int row, int col, MAP_NODE map[row][col]){
	printf(" ");
	for (int i = 0; i < col; i++){
		printf("%d", i);
	}

	printf("\n");

	for (int i = 0; i < row; i++){
		printf("%d", i);
		for (int k = 0; k < col; k++){
			printf("%c", map[i][k].cell);
		}
		printf("\n");
	}
}

/*
	function: this is a function that moves a piece on the board
	parameters:  map of characters, size of map row, size of map column, position of character to move, choice of movement
	returns: returns the result of the move either player won, player lost, or not end
*/
RESULT move(int row, int col, MAP_NODE map[row][col], int position[], int creatures[MAX_CREATURES][2], int end[], char choice, int num_creatures){
	RESULT is_end = NOT_END;
	int x = position[0];
	int y = position[1];
	int x1 = x;
	int y1 = y;

	char replacement_char = map[x][y].replacement;

	//check to see if replacement is empty and end is the same position
	//this would mean that all the creatures on the end have moved away
	if (map[x][y].replacement == ' '){
		if (x == end[0] && y == end[1])
			replacement_char = '*';
		else
			replacement_char = map[x][y].replacement;
	}
	else {
		replacement_char = map[x][y].replacement;
		map[x][y].replacement = ' ';
	}

	//check to see if object is player or creature
	if (map[x][y].cell == '#' || map[x][y].cell == '*' || map[x][y].cell == ' '){
		printf("This is an immovable object\n");
		return is_end;
	}

	//determines the position that needs to be checked
	switch (choice){
		case 'u':
			if (map[x][y].weight_u != -1)
				x1--;
			break;
		case 'd':
			if (map[x][y].weight_d != -1)
				x1++;
			break;
		case 'l':
			if (map[x][y].weight_l != -1)
				y1--;
			break;
		case 'r':
			if (map[x][y].weight_r != -1)
				y1++;
			break;
	}
	//if the position has not changed then it cant move
	if (x == x1 && y == y1)
		return;

	//update graphs
	if (isupper(map[x1][y1].cell)){
		if (map[x][y].cell == '0')
			is_end = PL_LOST;
		else
			map[x1][y1].replacement = map[x][y].cell;
		map[x][y].cell = replacement_char;
	}
	else {
		if (map[x1][y1].cell == '*'){
			if (map[x][y].cell == '0')
				is_end = PL_WON;
		}
		else if (map[x1][y1].cell == '0'){
			is_end = PL_LOST;
		}

		map[x1][y1].cell = map[x][y].cell;
		map[x][y].cell = replacement_char;
	}

	position[0] = x1;
	position[1] = y1;

	return is_end;
}

/*
	function: this is a function that plays one round where the player moves, then all the creatures move in alphabetical order
	parameters: map of characters, size of map row, size of map column
	returns: returns an array of graph nodes that contain the player, the creature, and the end
*/
RESULT round(int row, int col, MAP_NODE map[row][col], int end[], int creatures[MAX_CREATURES][2], int player[], int num_creatures){
	char choice;
	RESULT is_end;

	print_map(row, col, map);
	printf("Player %c, please enter your move [u(p), d(own), l(eft), or r(ight)]: ", map[play_pos[0]][play_pos[1]].cell);
	scanf(" %c", &choice);

	while (choice != 'u' && choice != 'd' && choice != 'l' && choice != 'r'){
		printf("Please choose either (u, d, l, or r)\n");
		printf("Player %c, please enter your move [u(p), d(own), l(eft), or r(ight)]: ", map[play_pos[0]][play_pos[1]].cell);
		scanf(" %c", &choice);
	}

	//let player move
	is_end = move(row, col, map, play_pos, creatures, end, choice, num_creatures);
	print_map(row, col, map);
	if (is_end == PL_WON || is_end == PL_LOST)
		return is_end;

	set_weights(row, col, map, play_pos);

	//let creatures move in alphabetical order
	for (int i = 0; i < num_creatures; i++){

		choice = shortest_path_dij(row, col, map, creatures[i], play_pos, '0');

		//choice = shortest_path(row, col, map, creatures[i], '0');
		is_end = move(row, col, map, creatures[i], creatures, end, choice, num_creatures);

		if (is_end == PL_LOST){
			print_map(row, col, map);
			return is_end;
		}
	}

	return is_end;
}

/*
	function: this is a function that uses breadth first search to find the shortest path from position to node with target. it works by putting items into a queue based on their level.
	when the target is reached, the path is recorded by a tree and the first path to reach the target is the shortest path	
	parameters: size of map row, size of map column, map , position of starting object, target of object to be found
	returns: the directions the object needs to follow for the shortest path in char format (u, d, l, r)
*/
STACK* shortest_path(int row, int col, MAP_NODE map[row][col], int object[], char target){
	int x = p[0]; //shortcuts for making indexes easier to understand
	int y = p[1]; //shortcuts for making indexes easier to understand
	int move[2];  //position of objects first move
	char choice;
	QUEUE *queue = NULL; //head of queue for breadth first search
	QUEUE temp;
	STACK *pos_stack = NULL; //queue to return the order from a to b
	STACK *temp_pos;		 //temp to traverse stack

	MAP_NODE temp_m = map[x][y]; //temp for creating map nodes

	TREE *new_parent = (TREE *)malloc(sizeof(TREE));
	new_parent->map_node = temp_m;

	queue = enqueue(queue, temp_m, new_parent);

	//for each map node check if it is target then get all neighbors and set neighbors parent
	//to map node
	while (queue != NULL){
		temp = dequeue(&queue);

		if (temp.map_node.label != 'v'){

			x = temp.map_node.pos[0];
			y = temp.map_node.pos[1];
			//visit temp
			//check if temp is target
			if (temp.map_node.cell == target){
				//stop and go up in tree and reverse the path from target to
				while (temp.tree_node != NULL){
					pos_stack = push(pos_stack, temp.tree_node->map_node.pos[0], temp.tree_node->map_node.pos[1]);
					temp.tree_node = temp.tree_node->parent;
				}

				break;
			}
			//set temp to visited
			map[x][y].label = 'v';

			//visit up neighbor
			if (map[x][y].weight_u != -1 && !isupper(map[x - 1][y].cell)){
				temp_m = map[x - 1][y];

				//create new tree and set parent to current node being visited
				TREE *new_child = (TREE *)malloc(sizeof(TREE));
				new_child->map_node = temp_m;
				new_child->parent = temp.tree_node;
				queue = enqueue(queue, temp_m, new_child);
			}
			//visit down neighbor
			if (map[x][y].weight_d != -1 && !isupper(map[x + 1][y].cell)){
				temp_m = map[x + 1][y];
				//create new tree and set parent to current node being visited
				TREE *new_child = (TREE *)malloc(sizeof(TREE));
				new_child->map_node = temp_m;
				new_child->parent = temp.tree_node;
				queue = enqueue(queue, temp_m, new_child);
			}
			//check left
			if (map[x][y].weight_l != -1 && !isupper(map[x][y - 1].cell)){
				temp_m = map[x][y - 1];

				//create new tree and set parent to current node being visited
				TREE *new_child = (TREE *)malloc(sizeof(TREE));
				new_child->map_node = temp_m;
				new_child->parent = temp.tree_node;
				queue = enqueue(queue, temp_m, new_child);
			}
			//check right
			if (map[x][y].weight_r != -1 && !isupper(map[x][y + 1].cell)){
				temp_m = map[x][y + 1];
				//create new tree and set parent to current node being visited
				TREE *new_child = (TREE *)malloc(sizeof(TREE));
				new_child->map_node = temp_m;
				new_child->parent = temp.tree_node;
				queue = enqueue(queue, temp_m, new_child);
			}
		}
	}

	//set back all map nodes to unvisited
	for (int i = 0; i < row; i++){
		for (int k = 0; k < col; k++){
			map[i][k].label = 'u';
		}
	}

	while (queue != NULL)
		dequeue(&queue);

	return pos_stack;
}

/*
	function: this function sets the specific weights for the map by finding the shortest path from the player to the end and then using dijkstra's algorithm
	parameters: size of map row, size of map column, map, position of player
	returns: the map

*/
void set_weights(int row, int col, MAP_NODE map[row][col], int player[]){
	STACK *path = NULL;
	STACK *temp = NULL;
	int is_on_path, is_on_path_u, is_on_path_d, is_on_path_l, is_on_path_r;
	path = shortest_path(row, col, map, player, '*');

	//set the specific weights for the graph
	for (int i = 0; i < row; i++){
		for (int k = 0; k < col; k++){
			is_on_path = 0;
			is_on_path_u = 0;
			is_on_path_d = 0;
			is_on_path_l = 0;
			is_on_path_r = 0;

			temp = path;

			while (temp != NULL){
				if (temp->pos[0] == i && temp->pos[1] == k)
					is_on_path = 1;
				if (temp->pos[0] == i - 1 && temp->pos[1] == k)
					is_on_path_u = 1;
				if (temp->pos[0] == i + 1 && temp->pos[1] == k)
					is_on_path_d = 1;
				if (temp->pos[0] == i && temp->pos[1] == k - 1)
					is_on_path_l = 1;
				if (temp->pos[0] == i && temp->pos[1] == k + 1)
					is_on_path_r = 1;

				temp = temp->next;
			}

			//check up
			if (map[i][k].weight_u != -1){
				if (is_on_path && is_on_path_u)
					map[i][k].weight_u = 1;
				else if (is_on_path || is_on_path_u)
					map[i][k].weight_u = 2;
				else
					map[i][k].weight_u = 3;
			}

			//check down
			if (map[i][k].weight_d != -1){
				if (is_on_path && is_on_path_d)
					map[i][k].weight_d = 1;
				else if (is_on_path || is_on_path_d)
					map[i][k].weight_d = 2;
				else
					map[i][k].weight_d = 3;
			}

			//check left
			if (map[i][k].weight_l != -1){
				if (is_on_path && is_on_path_l)
					map[i][k].weight_l = 1;
				else if (is_on_path || is_on_path_l)
					map[i][k].weight_l = 2;
				else
					map[i][k].weight_l = 3;
			}

			//check right
			if (map[i][k].weight_r != -1){
				if (is_on_path && is_on_path_r)
					map[i][k].weight_r = 1;
				else if (is_on_path || is_on_path_r)
					map[i][k].weight_r = 2;
				else
					map[i][k].weight_r = 3;
			}
		}
	}

	return path;
}

/*
	function: this is a function that finds the shortest path by using dijkstra's algorithm which is similar to the breadth first search algorithm except the way the priority queue is organized
	parameters: size of map row, size of map column, map, position of starting object, target
	return: returns the char of choice for the creature
*/
char dijkstra_path(int row, int col, MAP_NODE map[row][col], int object[], char target){}

 

























