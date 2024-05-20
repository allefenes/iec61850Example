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

        SVPublisher_ASDU asdu = SVPublisher_addASDU(svPublisher, "RandomCurrent", NULL, 1);

        int current = SVPublisher_ASDU_addFLOAT(asdu);
        int ts = SVPublisher_ASDU_addTimestamp(asdu);

        SVPublisher_setupComplete(svPublisher);

        srand(time(NULL)); // Seed the random number generator

        while (running) {
            Timestamp timestamp;
            Timestamp_clearFlags(&timestamp);
            Timestamp_setTimeInMilliseconds(&timestamp, Hal_getTimeInMs());

            /* Generate random current value between 1000 and 1200 */
            float randCurrent = 1000.0f + (rand() % 201);

            /* Update the values in the SV ASDU */
            SVPublisher_ASDU_setFLOAT(asdu, current, randCurrent);
            SVPublisher_ASDU_setTimestamp(asdu, ts, timestamp);

            /* Update the sample counter */
            SVPublisher_ASDU_increaseSmpCnt(asdu);

            /* Send the SV message */
            SVPublisher_publish(svPublisher);

	    printf("Published Current : %f\n", randCurrent);

            /* Adjust sleep time to match the SV sample rate */
            Thread_sleep(1000);
        }

        SVPublisher_destroy(svPublisher);
    }
    else {
        printf("Failed to create SV publisher\n");
    }

    return 0;
}

