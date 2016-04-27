#include <stdio.h>
#include <math.h>
#include "simlib.h"

#define EVENT_ENTER_NODE		1	/* The Event for when a person enters a new node*/
#define EVENT_ENTER_DOOR		3	/* The Event for when an Agent enters the door pool*/
#define EVENT_EXIT_BUILDING		4	/* The Event for when an Agent exits the building */
#define EVENT_ALARM_ALL			5	/* The Event for when the alarm goes off alerting all the nodes*/
#define EVENT_END_SIMULATION	6	/* The Event type for end of the simulation */

typedef struct {
	int isDoor;					// 0 = is not a door; 1 = is a door
	int * adjacencies;			// List containing the rooms it connects to
	int * distance_adjacencies;	// List containing the distance to the rooms it connects to, adjancency[i]=distance[i]
	int number_of_connections;  // Number of connections made
	int isAlarmed;				// 0 = Node is not aware of the situation; 1 = Node is aware of the situation
	int door_pool;				// Only applicable if isDoor == 1; The amount of people at the door
	int door_flow;				// Amount of time it takes to travel through door
	int population;				// Number of people in the room
} Node;

/* List queues */
int * enter_node_queue;
int * enter_door_queue;
int * exit_building_queue;
int enter_node_head, enter_node_tail = 0;
int enter_door_head, enter_door_tail = 0; 
int exit_building_head, exit_node_tail = 0;

/* Input Variables and sutff used through out program */
int time_delay, alarm_delay, actor_num, node_num, total_people;
Node * nodelist;

/* Statistics to output */


void seedNodes(void);
void enterNode(void);
void enterDoor(void);
void exitBuilding(void);
void alarm(void);
void report(void);

int main() {
	/* Read in the input file */
	FILE * input_pointer = fopen("evacsim.in", "r");
	if (input_pointer == NULL) {
		fprintf(stderr, "File 'evacsim.in' was not opened successfully");
		return -1;
	}

	/* Instantiate the variables */
	fscanf(input_pointer, "%d\n%d\n%d\n%d\n", &time_delay, &alarm_delay, &actor_num, &node_num);
	total_people = actor_num;
	nodelist = malloc(sizeof(Node) * node_num);

	enter_door_queue = enter_node_queue = exit_building_queue = malloc(sizeof(int) * actor_num);

	/* Create the node adjacencies using the file */
	int i, j; Node curr_node;
	for (i = 0; i<node_num; i++) {
		curr_node = nodelist[i];
		curr_node.door_flow = 1;
		curr_node.population = 0;
		curr_node.door_pool = 0;

		fscanf(input_pointer, "%d\n%d\n", &(curr_node.isDoor), &(curr_node.number_of_connections));
		curr_node.adjacencies = malloc(sizeof(int) * curr_node.number_of_connections);

		for (j = 0; j < curr_node.number_of_connections; j++) {
			fscanf(input_pointer, "%d %d\n", curr_node.adjacencies[j],curr_node.distance_adjacencies[j]);
		}
	}

	/* Seed nodes with agents */
	seedNodes();

	/* Initialize simlib */
	init_simlib();

	/* set maxatr = max(maximum number of attributes per record, 4)*/
	maxatr = 4;

	/* Initialize the model */
	init_model();

	/* Schedule the Alarm*/
	event_schedule(alarm_delay, EVENT_ALARM_ALL);

	/* Schedule the end of the simulation */
	event_schedule(time_delay, EVENT_END_SIMULATION);

	/* Select a random to set to alert and have all people escape */
	set_first_event();

	/* Run the simulation until the time is up */
	do {
		timing();
		switch (next_event_type) {
		case EVENT_ENTER_NODE:
			enterNode();
			break;
		case EVENT_ENTER_DOOR:
			enterDoor();
			break;
		case EVENT_EXIT_BUILDING:
			exitBuilding();
			break;
		case EVENT_ALARM_ALL:
			alarm();
			break;
		case EVENT_END_SIMULATION:
			report();
			break;
		}

	} while (next_event_type != EVENT_END_SIMULATION && total_people != 0);

	fclose(input_pointer);
	free(enter_door_queue);
	free(enter_node_queue);
	free(exit_building_queue);
}

void seedNodes() /* Seed the nodes with random amounts of agents*/
{
	int i;
	srand(node_num);
	for (i = 0; i < actor_num; i++) 
	{
		int r = rand();
		nodelist[i].population++;
	}
}

void set_first_event() 
{
	srand(node_num);
	int r = rand();
	/* Set the node to alarmed */
	nodelist[r].isAlarmed = 1;

	/* Check if the room has a door */
	int i;
	for (i = 0; i < nodelist[r].number_of_connections; i++) {
		/* NOTE: It didn't like me not using the next two lines */
		int * adj = nodelist[r].adjacencies;
		int adjacent_node_num = adj[i];
		
		/* If there is a door, schedule an Enter Door event for each person in the room */
		if (nodelist[adjacent_node_num].isDoor == 1) {
			event_schedule(sim_time + nodelist[r].distance_adjacencies[i], EVENT_ENTER_DOOR);

			nodelist[adjacent_node_num].door_pool += nodelist[r].population;
			nodelist[r].population = 0;
			
			enter_door_queue[enter_door_tail] = r;
			enter_door_tail = (enter_door_tail + 1) % actor_num;
			return;
		}
	}

	/* There is no door, so each person is going to enter a random adjacent node */
	srand(nodelist[r].number_of_connections);
	int j;
	while(nodelist[r].population > 0) {
		int random_node_adj = rand();

		event_schedule(sim_time + nodelist[r].distance_adjacencies[random_node_adj], EVENT_ENTER_NODE);

		nodelist[random_node_adj].population++;
		nodelist[r].population--;

		enter_node_queue[enter_node_tail] = r;
		enter_node_tail = (enter_node_tail + 1) % actor_num;
	}
}

void enterNode() /* Enter a new node event function */
{
	int r = enter_node_head;
	enter_node_head = (enter_node_head + 1) % actor_num;

	/* Set the node to alarmed */
	nodelist[r].isAlarmed = 1;

	/* Check if the room has a door */
	int i;
	for (i = 0; i < nodelist[r].number_of_connections; i++) {
		/* NOTE: It didn't like me not using the next two lines */
		int * adj = nodelist[r].adjacencies;
		int adjacent_node_num = adj[i];

		/* If there is a door, schedule an Enter Door event */
		if (nodelist[adjacent_node_num].isDoor == 1) {
			event_schedule(sim_time + nodelist[r].distance_adjacencies[i], EVENT_ENTER_DOOR);

			/* Add to the enter door queue */
			enter_door_queue[enter_door_tail] = r;
			enter_door_tail = (enter_door_tail + 1) % actor_num;
			return;
		}
	}

	/* There is no door, so enter a random node */
	srand(nodelist[r].number_of_connections);
	int j;
	while(nodelist[r].population > 0) {
		int random_node_adj = rand();

		event_schedule(sim_time + nodelist[r].distance_adjacencies[random_node_adj], EVENT_ENTER_NODE);

		nodelist[random_node_adj].population++;
		nodelist[r].population--;

		enter_node_queue[enter_node_tail] = r;
		enter_node_tail = (enter_node_tail + 1) % actor_num;
	}
}

void enterDoor()
{
	/* Add new agent to */
}

void exitBuilding() {}

void alarm() {}

void report() {}