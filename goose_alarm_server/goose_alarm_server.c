#include <signal.h> 
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "hal_thread.h"
#include "iec61850_server.h"
#include "static_model.h"
#include "sv_subscriber.h"
#include "goose_publisher.h"

static int running = 0;
static IedServer iedServer = NULL;
static GoosePublisher publisher = NULL;

void sigint_handler(int signalId)
{
    running = 0;
}

static void goose_publish_v(float voltage) {
    LinkedList dataSetValues = LinkedList_create();
    LinkedList_add(dataSetValues, MmsValue_newVisibleString("Voltage Overshoot"));
    LinkedList_add(dataSetValues, MmsValue_newFloat(voltage));
    
    GoosePublisher_publish(publisher, dataSetValues);

    LinkedList_destroyDeep(dataSetValues, (LinkedListValueDeleteFunction) MmsValue_delete);
}

static void goose_publish_c(float current){
    LinkedList dataSetValues = LinkedList_create();
    LinkedList_add(dataSetValues, MmsValue_newVisibleString("Current Overshoot"));
    LinkedList_add(dataSetValues, MmsValue_newFloat(current));
    
    GoosePublisher_publish(publisher, dataSetValues);

    LinkedList_destroyDeep(dataSetValues, (LinkedListValueDeleteFunction) MmsValue_delete);
}

static void sv_update_listener(SVSubscriber subscriber, void* parameter, SVSubscriber_ASDU asdu)
{
    const char* svID = SVSubscriber_ASDU_getSvId(asdu);

    if (svID != NULL)
        printf("Received SV with ID: %s\n", svID);

    if (strcmp(svID, "RandomCurrent") == 0) {
        if (SVSubscriber_ASDU_getDataSize(asdu) >= 4) {
            float current = SVSubscriber_ASDU_getFLOAT32(asdu, 0);
            printf("Received SV (Current): current=%f\n", current);

            if (current > 1100.0) {
                printf("Current exceeded threshold. Publishing GOOSE message.\n");
                goose_publish_c(current);
            }
        }
    }
    else if (strcmp(svID, "RandomVoltage") == 0) {
        if (SVSubscriber_ASDU_getDataSize(asdu) >= 4) {
            float voltage = SVSubscriber_ASDU_getFLOAT32(asdu, 0);
            printf("Received SV (Voltage): voltage=%f\n", voltage);

            if (voltage > 380.0) {
                printf("Voltage exceeded threshold. Publishing GOOSE message.\n");
                goose_publish_v(voltage); // Current is not applicable here
            }
        }
    }
    else {
        printf("Unknown SV ID: %s\n", svID);
    }
}

int main(int argc, char** argv)
{
    iedServer = IedServer_create(&iedModel);
    int tcpPort = 102;

    if (argc > 1) {
        tcpPort = atoi(argv[1]);
    }

    CommParameters gooseCommParameters;
    gooseCommParameters.appId = 1000;
    gooseCommParameters.vlanId = 0;
    gooseCommParameters.vlanPriority = 4;
    gooseCommParameters.dstAddress[0] = 0x01;
    gooseCommParameters.dstAddress[1] = 0x0c;
    gooseCommParameters.dstAddress[2] = 0xcd;
    gooseCommParameters.dstAddress[3] = 0x01;
    gooseCommParameters.dstAddress[4] = 0x00;
    gooseCommParameters.dstAddress[5] = 0x01;

    publisher = GoosePublisher_create(&gooseCommParameters, "eth0");

    if (publisher) {
        GoosePublisher_setGoCbRef(publisher, "simpleIOGenericIO/LLN0$GO$gcbAnalogValues");
        GoosePublisher_setConfRev(publisher, 1);
        GoosePublisher_setDataSetRef(publisher, "simpleIOGenericIO/LLN0$AnalogValues");
        GoosePublisher_setTimeAllowedToLive(publisher, 5000);
    } else {
        printf("Failed to create GOOSE publisher. Exiting.\n");
        return -1;
    }

    SVReceiver receiver = SVReceiver_create();

    if (argc > 2) {
        SVReceiver_setInterfaceId(receiver, argv[2]);
        printf("Set interface id: %s\n", argv[2]);
    } else {
        printf("Using interface eth0\n");
        SVReceiver_setInterfaceId(receiver, "eth0");
    }

    SVSubscriber subscriber_amper = SVSubscriber_create(NULL, 0x4000); //NULL : Default Solve Function, 0x4000 : Unique Subscriber Id
    SVSubscriber_setListener(subscriber_amper, sv_update_listener, NULL); // NULL : User Id
    SVReceiver_addSubscriber(receiver, subscriber_amper);

    SVSubscriber subscriber_voltage = SVSubscriber_create(NULL, 0x4001);
    SVSubscriber_setListener(subscriber_voltage, sv_update_listener, NULL);
    SVReceiver_addSubscriber(receiver, subscriber_voltage);

    SVReceiver_start(receiver);

    IedServer_start(iedServer, tcpPort);

    if (!IedServer_isRunning(iedServer)) {
        printf("Starting server failed! Exit.\n");
        IedServer_destroy(iedServer);
        return -1;
    }

    running = 1;

    signal(SIGINT, sigint_handler);

    while (running) {
        Thread_sleep(100);
    }

    IedServer_stop(iedServer);

    SVReceiver_stop(receiver);

    IedServer_destroy(iedServer);
    GoosePublisher_destroy(publisher);
    SVReceiver_destroy(receiver);

    return 0;
}

