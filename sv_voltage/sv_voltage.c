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

        // ASDU : Application Service Data Unit

        SVPublisher_ASDU asdu = SVPublisher_addASDU(svPublisher, "RandomVoltage", NULL, 1); //Null : Extra Configuration, 1 : Number of Copies

        int voltage = SVPublisher_ASDU_addFLOAT(asdu);
        int timestampPoint = SVPublisher_ASDU_addTimestamp(asdu); //Milliseconds

        SVPublisher_setupComplete(svPublisher);

        srand(time(NULL)); // Pseudo-Random Number Generator + Seed 

        while (running) {
            Timestamp timestamp;
            Timestamp_clearFlags(&timestamp);
            Timestamp_setTimeInMilliseconds(&timestamp, Hal_getTimeInMs());

            // Between 370-390
            float randVoltage = 370.0f + (rand() % 21);

            SVPublisher_ASDU_setFLOAT(asdu, voltage, randVoltage);
            SVPublisher_ASDU_setTimestamp(asdu, timestampPoint, timestamp);

            SVPublisher_ASDU_increaseSmpCnt(asdu);

            SVPublisher_publish(svPublisher);

	    printf("Published Voltage : %f\n", randVoltage);

            Thread_sleep(1000);
        }

        SVPublisher_destroy(svPublisher);
    }
    else {
        printf("Failed to create SV publisher\n");
    }

    return 0;
}

