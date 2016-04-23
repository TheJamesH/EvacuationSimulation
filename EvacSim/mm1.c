/* Evacuation simulation prgram. */

#include <stdio.h>  
#include <math.h>
#include "lcgrand.h"  /* Header file for random-number generator. */
#include <windows.h>

//#define NChannels 100
//#define N_LIMIT 100

int   actorNum;
float mean_interarrival, mean_service, sim_time, time_arrival[N_LIMIT + 1], time_last_event, time_next_event[4];
double area_channel_utilization;
FILE  *infile, *outfile;
time_t seconds;

main();

void  initialize(void);
void  timing(void);
void  arrive(void);
void  depart(void);
void  report(void);
void  update_time_avg_stats(void);
float expon(float mean);


main()  /* Main function. */
{
	seconds = time(NULL);
    /* Open input and output files. */
    infile = fopen("C:\\Users\\James\\Documents\\Visual Studio 2015\\Projects\\380Lab2\\Debug\\mm1.in", "r");
	if (infile == NULL) {
		perror("Error opening infile");
		return;			
	}
    outfile = fopen("C:\\Users\\James\\Documents\\Visual Studio 2015\\Projects\\380Lab2\\Debug\\mm1.out", "w");
	if (outfile == NULL) {
		perror("Error opening outfile");
		return;
	}

	/* Set up Outside node. */

    /* Read input simulation. */

    //fscanf(infile, "%f %f %d", &mean_interarrival, &mean_service, &num_calls_required);

    /* Write report heading and input parameters. */

    /*fprintf(outfile, "Cellular System\n\n");
    fprintf(outfile, "Mean interarrival time%11.3f minutes\n\n",
            mean_interarrival);
    fprintf(outfile, "Mean service time%16.3f minutes\n\n", mean_service);
    fprintf(outfile, "Number of calls%14d\n\n", num_calls_required); */

    /* Initialize the simulation. */

    initialize();

    /* Run the simulation while all actors have not reached outside node. */

    while ()
    {
        /* Determine the next event. */

        timing();

        /* Update time-average statistical accumulators. */

        update_time_avg_stats();

        /* Invoke the appropriate event function. */

        switch (next_event_type) 
        {
            case 1: //new call arrival
				num_call_arrived++;
				if (num_channels > 0) {
					num_channels--;
					time_next_event[3] = sim_time + expon(mean_service);
				}
				else {
					num_call_rejected++;
				}
				time_next_event[1] = sim_time + expon(mean_interarrival);
				break;
            case 2: //handoff call arrival
				num_handoff_arrived++;
				if (num_channels > 0) {
					num_channels--;
					time_next_event[3] = sim_time + expon(mean_service);
				}
				else {
					num_hadoff_rejected++;
				}
				time_next_event[2] = sim_time + expon(mean_interarrival);
				break;
			case 3: //call finishing
				num_channels++;
				time_next_event[3] = 1.0e+30;
				break;
        }
    }

    /* Invoke the report generator and end the simulation. */

    report();

    fclose(infile);
    fclose(outfile);

    return 0;
}


void initialize(void)  /* Initialization function. */
{
    /* Initialize the simulation clock. */

    sim_time = 0.0;

    /* Initialize the state variables. */

    time_last_event = 0.0;

    /* Initialize the statistical counters. */

	num_call_arrived = 0;
	num_call_rejected = 0;
	num_handoff_arrived = 0;
	num_hadoff_rejected = 0;
	area_channel_utilization = 0.0;

    /* Initialize event list. */

    time_next_event[1] = sim_time + expon(mean_interarrival);
    time_next_event[2] = sim_time + expon(mean_interarrival);
	time_next_event[3] = 1.0e+30;
}


void timing(void)  /* Timing function. */
{
    int   i;
    float min_time_next_event = 1.0e+29;

    next_event_type = 0;

    /* Determine the event type of the next event to occur. */

    for (i = 1; i <= num_events; ++i)
        if (time_next_event[i] < min_time_next_event)
        {   
            min_time_next_event = time_next_event[i];
            next_event_type     = i;
        }

    /* Check to see whether the event list is empty. */

    if (next_event_type == 0)
    {
        /* The event list is empty, so stop the simulation. */

        fprintf(outfile, "\nEvent list empty at time %f", sim_time);
		fclose(infile);
		fclose(outfile);
        exit(1);
    }

    /* The event list is not empty, so advance the simulation clock. */

    sim_time = min_time_next_event;
}


void report(void)  /* Report generator function. */
{
    /* Compute and write estimates of desired measures of performance. */
	fprintf(outfile, "num_call_arrived: %d \n", num_call_arrived);
	fprintf(outfile, "num_call_rejected: %d \n", num_call_rejected);
	fprintf(outfile, "num_handoff_arrived: %d \n", num_handoff_arrived);
	fprintf(outfile, "num_hadoff_rejected: %d \n\n", num_hadoff_rejected);

	// u =  1/mean_service or  calls/sec
	fprintf(outfile, "u: %12.3f calls per minute \n", (1 / mean_service));
	// p = (rate of new + rate of handoff)/(N * u)
	fprintf(outfile, "p: %12.3f \n", ((2*mean_interarrival) / ((float)N_LIMIT* (1 / mean_service))));
	// U = avg occupide channels / N
	fprintf(outfile, "U: %12.3f \n", ((area_channel_utilization) / (sim_time * (float)N_LIMIT)));
	// CBP = total rejected/ total new call arrivals
	if (num_call_arrived != 0) {
		fprintf(outfile, "CBP: %12.3f \n", ((float)(num_call_rejected) / (float)num_call_arrived));
	}
	// HDP = total rejected/ total new handoff arrivals
	if (num_handoff_arrived != 0) {
		fprintf(outfile, "HDP: %12.3f \n", ((float)(num_hadoff_rejected) / (float)num_handoff_arrived));
	}

	fprintf(outfile, "Time simulation ended%12.3f minutes\n", sim_time);
	fprintf(outfile, "area_channel_utilization%12.3f \n", area_channel_utilization);
}


void update_time_avg_stats(void)  /* Update area accumulators for time-average
                                     statistics. */
{
    float time_since_last_event;

    /* Compute time since last event, and update last-event-time marker. */

    time_since_last_event = sim_time - time_last_event;
    time_last_event       = sim_time;

    /* Update area under channel utilization. */

	area_channel_utilization += (N_LIMIT - num_channels) * time_since_last_event;
}


float expon(float mean)  /* Exponential variate generation function. */
{
    /* Return an exponential random variate with mean "mean". */
    return -mean * log(lcgrand(seconds%60));
}

