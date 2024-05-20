/*
 * sv_publisher_example.c
 *
 * Example program for Sampled Values (SV) publisher
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "hal_thread.h"
#include "sv_publisher.h"

static bool running = true;

void sigint_handler(int signalId)
{
    running = 0;
}

int main(int argc, char** argv)
{
    char* interface;
  
    if (argc > 1)
        interface = argv[1];
    else
        interface = "eth0";
  
    printf("Using interface %s\n", interface);

    signal(SIGINT, sigint_handler);

    SVPublisher svPublisher = SVPublisher_create(NULL, interface);

    if (svPublisher) {

        /* Create ASDU and add data points */

        SVPublisher_ASDU asdu = SVPublisher_addASDU(svPublisher, "RandomVoltage", NULL, 1);

        int voltage = SVPublisher_ASDU_addFLOAT(asdu);
        int timestampPoint = SVPublisher_ASDU_addTimestamp(asdu); // Use the timestamp in milliseconds

        SVPublisher_setupComplete(svPublisher);

        srand(time(NULL)); // Seed the random number generator

        while (running) {
            Timestamp timestamp;
            Timestamp_clearFlags(&timestamp);
            Timestamp_setTimeInMilliseconds(&timestamp, Hal_getTimeInMs());

            /* Generate random voltage value between 370 and 390 */
            float randVoltage = 370.0f + (rand() % 21);

            /* Update the values in the SV ASDU */
            SVPublisher_ASDU_setFLOAT(asdu, voltage, randVoltage);
            SVPublisher_ASDU_setTimestamp(asdu, timestampPoint, timestamp);

            /* Update the sample counter */
            SVPublisher_ASDU_increaseSmpCnt(asdu);

            /* Send the SV message */
            SVPublisher_publish(svPublisher);

	    printf("Published Voltage : %f\n", randVoltage);

            /* Sleep for 500 milliseconds (0.5 seconds) */
            Thread_sleep(1000);
        }

        SVPublisher_destroy(svPublisher);
    }
    else {
        printf("Failed to create SV publisher\n");
    }

    return 0;
}

