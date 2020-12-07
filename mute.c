#include <stdlib.h>
#include <string.h>
#include <ladspa.h>

// Ports
#define PORT_COUNT       4
#define ML_MUTE_INTERVAL 0
#define ML_MUTE_LENGTH   1
#define ML_INPUT         2
#define ML_OUTPUT        3

// State
#define MUTED   0
#define UNMUTED !MUTED

// LADSPA_Handle
typedef struct {
    unsigned long sampleRate;

    unsigned int muteInterval; // in seconds
    unsigned int muteLength;   // in seconds

    LADSPA_Data *inputBuffer; // pointer to input buffer
    LADSPA_Data *outputBuffer; // pointer to output buffer

    unsigned long remaining; // how many samples are remained before we change the state
    unsigned int state; // muted or unmuted
} AnnoyingMuteLine;

// handle new instance
static LADSPA_Handle instantiateMuteLine(const LADSPA_Descriptor *descriptor,
                                      unsigned long sampleRate) {

    AnnoyingMuteLine *muteLine;

    muteLine = (AnnoyingMuteLine *)malloc(sizeof(AnnoyingMuteLine));

    if (muteLine == NULL) {
        return NULL;
    }

    muteLine->sampleRate = sampleRate;

    return muteLine;
}

// Assign given parameters accordingly in handle
static void connectPort(LADSPA_Handle handle, unsigned long port, LADSPA_Data *data) {
    AnnoyingMuteLine *muteLine = (AnnoyingMuteLine *) handle;

    switch (port) {
    case ML_MUTE_INTERVAL:
        muteLine->muteInterval = (unsigned int)*data;
        break;
    case ML_MUTE_LENGTH:
        muteLine->muteLength = (unsigned int)*data;
        break;
    case ML_INPUT:
        muteLine->inputBuffer = data;
        break;
    case ML_OUTPUT:
        muteLine->outputBuffer = data;
        break;
    }
}

// initialize the state
static void activateMuteLine(LADSPA_Handle handle) {
    AnnoyingMuteLine *muteLine = (AnnoyingMuteLine *)handle;

    muteLine->state = MUTED;
    muteLine->remaining = 0;
}

// main handler. forward samples or mute according to state
static void runMuteLine(LADSPA_Handle handle, unsigned long sampleCount) {
    AnnoyingMuteLine *muteLine = (AnnoyingMuteLine *) handle;

    for (unsigned long i = 0; i < sampleCount; i++) {
        if (muteLine->remaining == 0) {
            muteLine->state = !muteLine->state;

            if (muteLine->state == MUTED) {
                muteLine->remaining = muteLine->sampleRate * muteLine->muteLength;
            }
            else {
                muteLine->remaining = muteLine->sampleRate * muteLine->muteInterval;
            }
        }

        if (muteLine->state == MUTED) {
            muteLine->outputBuffer[i] = 0;
        }
        else {
            muteLine->outputBuffer[i] = muteLine->inputBuffer[i];
        }

        muteLine->remaining--;
    }
}

// free the handle
static void cleanupMuteLine(LADSPA_Handle handle) {
    AnnoyingMuteLine *muteLine = (AnnoyingMuteLine *) handle;

    free(handle);
}

static LADSPA_Descriptor *descriptor = NULL;

// On plugin load
static void __attribute__ ((constructor)) init() {
    LADSPA_PortDescriptor * portDescriptors;
    LADSPA_PortRangeHint * portRangeHints;
    char ** portNames;

    descriptor = (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

    if (!descriptor) {
        return;
    }

    descriptor->UniqueID  = 1094; // should be unique
    descriptor->Label     = strdup("mute_n");
    descriptor->Name      = strdup("Annoying Mute Line");
    descriptor->Maker     = strdup("paddlesteamer");
    descriptor->Copyright = strdup("None");

    descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;

    descriptor->PortCount = PORT_COUNT;
    
    portDescriptors = (LADSPA_PortDescriptor *) calloc(PORT_COUNT, sizeof(LADSPA_PortDescriptor));

    portDescriptors[ML_MUTE_INTERVAL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    portDescriptors[ML_MUTE_LENGTH] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    portDescriptors[ML_INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    portDescriptors[ML_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

    descriptor->PortDescriptors = portDescriptors;

    portNames = (char **) calloc(PORT_COUNT, sizeof(char *));
    portNames[ML_MUTE_INTERVAL] = strdup("Interval in seconds");
    portNames[ML_MUTE_LENGTH] = strdup("Mute length in seconds");
    portNames[ML_INPUT] = strdup("Input");
    portNames[ML_OUTPUT] = strdup("Output");

    descriptor->PortNames = (const char * const *) portNames;

    portRangeHints = (LADSPA_PortRangeHint *) calloc(PORT_COUNT, sizeof(LADSPA_PortRangeHint));

    portRangeHints[ML_MUTE_INTERVAL].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_1;
    portRangeHints[ML_MUTE_INTERVAL].LowerBound = 1;
    portRangeHints[ML_MUTE_LENGTH].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_1;
    portRangeHints[ML_MUTE_LENGTH].LowerBound = 1;
    portRangeHints[ML_INPUT].HintDescriptor = 0;
    portRangeHints[ML_OUTPUT].HintDescriptor = 0;

    descriptor->PortRangeHints = portRangeHints;

    descriptor->instantiate = instantiateMuteLine;
    descriptor->connect_port = connectPort;
    descriptor->activate = activateMuteLine;
    descriptor->run = runMuteLine;
    descriptor->run_adding = NULL;
    descriptor->set_run_adding_gain = NULL;
    descriptor->deactivate = NULL;
    descriptor->cleanup = cleanupMuteLine;
}

// On plugin unload
static void __attribute__ ((destructor)) fini() {
    if (descriptor == NULL) return;

    free((char *) descriptor->Label);
    free((char *) descriptor->Name);
    free((char *) descriptor->Maker);
    free((char *) descriptor->Copyright);
    free((char *) descriptor->PortDescriptors);
    for (int i=0; i<PORT_COUNT; i++) {
        free((char *) descriptor->PortNames[i]);
    }

    free((char **) descriptor->PortNames);
    free((LADSPA_PortRangeHint *) descriptor->PortRangeHints);
    free(descriptor);
}

// we only have one type of plugin
const LADSPA_Descriptor * ladspa_descriptor(unsigned long index) {
    if (index != 0) {
        return NULL;
    }

    return descriptor;
}
