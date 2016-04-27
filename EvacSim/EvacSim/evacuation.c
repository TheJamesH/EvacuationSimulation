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
	int population;				// Number of people in the room
	int max_population;			// Maximum number of people in the room at any given time
} Node;

/* List queues */
int * enter_node_queue;
int * enter_door_queue;
int * exit_building_queue;
int enter_node_head, enter_node_tail = 0;
int enter_door_head, enter_door_tail = 0; 
int exit_building_head, exit_node_tail = 0;

/* Input Variables and sutff used through out program */
int time_delay, alarm_delay, actor_num, node_num, total_people, cumulative_time;
Node * nodelist;

/* Statistics to output */


void seedNodes(void);
void enterNode(void);
void enterDoor(void);
void alarm(void);
void report(void);

int main() 
{
	/* Read in the input file */
	FILE * input_pointer = fopen("evacsim.in", "r");
	if (input_pointer == NULL) 
	{
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
	for (i = 0; i<node_num; i++) 
	{
		curr_node = nodelist[i];
		curr_node.population = 0;

		fscanf(input_pointer, "%d\n%d\n", &(curr_node.isDoor), &(curr_node.number_of_connections));
		curr_node.adjacencies = malloc(sizeof(int) * curr_node.number_of_connections);

		for (j = 0; j < curr_node.number_of_connections; j++) 
		{
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
	int last_escape_time = 0;
	do 
	{
		timing();
		switch (next_event_type) {
		case EVENT_ENTER_NODE:
			enterNode();
			break;
		case EVENT_ENTER_DOOR:
			enterDoor();
			break;
		case EVENT_EXIT_BUILDING:
			total_people--;
			cumulative_time += sim_time-last_escape_time;
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
	event_schedule(sim_time + 1, EVENT_ENTER_NODE);

	enter_node_queue[enter_node_tail] = r;
	enter_node_tail = (enter_node_tail + 1) % actor_num;
}

void enterNode() /* Enter a new node event function */
{
	int r = enter_node_head;
	enter_node_head = (enter_node_head + 1) % actor_num;

	/* Set the node to alarmed */
	nodelist[r].isAlarmed = 1;

	/* Check if the room has a door */
	int i;
	for (i = 0; i < nodelist[r].number_of_connections; i++) 
	{
		/* NOTE: It didn't like me not using the next two lines */
		int * adj = nodelist[r].adjacencies;
		int adjacent_node_num = adj[i];

		/* If there is a door, schedule an Enter Door event */
		if (nodelist[adjacent_node_num].isDoor == 1) 
		{
			event_schedule(sim_time + nodelist[r].distance_adjacencies[i], EVENT_ENTER_DOOR);

			nodelist[adjacent_node_num].population += nodelist[r].population;
			nodelist[r].population = 0;

			/* Add to the enter door queue */
			enter_door_queue[enter_door_tail] = r;
			enter_door_tail = (enter_door_tail + 1) % actor_num;
			return;
		}
	}

	/* There is no door, so enter a random node */
	srand(nodelist[r].number_of_connections);
	int j;
	while(nodelist[r].population > 0) 
	{
		int random_node_adj = rand();

		event_schedule(sim_time + nodelist[r].distance_adjacencies[random_node_adj], EVENT_ENTER_NODE);

		nodelist[random_node_adj].population++;
		nodelist[r].population--;

		enter_node_queue[enter_node_tail] = r;
		enter_node_tail = (enter_node_tail + 1) % actor_num;

		check_max_pop(random_node_adj);
	}
}

void enterDoor()
{
	int doorNum = enter_door_queue[enter_door_head];
	enter_door_head = (enter_door_head + 1) % actor_num;

	check_max_pop(doorNum);

	nodelist[doorNum].population--;
	/* The actors can only exit the building one at a time */
	event_schedule(sim_time + 1, EVENT_EXIT_BUILDING);
	
	/* If there are more people in the node, schedule another door event*/
	if (nodelist[doorNum].population != 0) 
	{
		event_schedule(sim_time + 1, EVENT_ENTER_DOOR);
		
		enter_door_queue[enter_door_tail] = doorNum;
		enter_door_tail = (enter_door_tail + 1) % actor_num;
	}
}

void alarm() 
{	
	int i;
	for (i = 0; i < node_num; i++) 
	{
		if (nodelist[i].isAlarmed == 0) {
			nodelist[i].isAlarmed = 1;
			event_schedule(sim_time + 1, EVENT_ENTER_NODE);
			
			enter_node_queue[enter_door_tail] = i;
			enter_node_tail = (enter_node_tail + 1) % actor_num;
		}
	}
}

void report() 
{
	FILE * output = fopen("evac.out", "w");

	fprintf(output, "Total Number of Actors: %d \n", actor_num);
	fprintf(output, "Time for all people to escape: %d\n", sim_time);
	fprintf(output, "Average time to escape: %d\n", cumulative_time / sim_time);
	
	int i;
	for (i=0;i<node_num; i++)
	{
		fprintf(output, "Max Pop in Node %d: %d \n", i, nodelist[i].max_population);
		fprintf(output, "Max Pop in Node %d: ??? \n \n", i);
	}

	fclose(output);
}

void check_max_pop(int node) {
	if (nodelist[node].population > nodelist[node].max_population) {
		nodelist[node].max_population = nodelist[node].population;
	}
}