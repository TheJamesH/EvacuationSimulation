#include <stdio.h>
#include <math.h>


struct Agent {
	int speed;		// How many units an agent travels during ticks
	int location;	// How many units an agent is away from the door
	int safe;		// 0 = agent is inside; 1 = agent is outside
	int alert;		// 0 = agent is unaware; 1 = agent is aware of the threat

};

typedef struct {
	int isDoor;					// 0 = is a door; 1 = is not a door
	int * adjacencies;			// List containing the rooms it connects to
	int number_of_connections;  // Number of connections made
} Node;

int alarm_delay, actor_num, node_num;
Node * nodelist;

int main() {
	/* Read in the input file */
	FILE * input_pointer = fopen("evacsim.in", "r");
	if (input_pointer == NULL) {
		fprintf(stderr, "File 'evacsim.in' was not opened successfully");
		return -1;
	}

	/* Instantiate the variables */
	fscanf(input_pointer, "%d\n%d\n%d\n", &alarm_delay, &actor_num, &node_num);
	nodelist = malloc(sizeof(Node) * node_num);

	/* Create the node adjacencies using the file */
	int i,j;
	Node curr_node;
	for (i = 0; i<node_num; i++) {
		curr_node = nodelist[i];
		fscanf(input_pointer, "%d\n%d\n", &(curr_node.isDoor), &(curr_node.number_of_connections));
		curr_node.adjacencies = malloc(sizeof(int) * curr_node.number_of_connections);
		j = 0;
		while (scanf("%d ", &curr_node.adjacencies[j]) == 1) { // THis might work
			++j;
		}
	}

	fclose(input_pointer);
}

