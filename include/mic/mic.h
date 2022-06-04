#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "memresource.h"

typedef int MICHandle;

typedef struct MICRingBuffer MICRingBuffer;
typedef struct MICStatus MICStatus;

typedef enum MICError
{
    MIC_ERROR_NONE          = 0,
    MIC_ERROR_NOT_SUP       = -1,
    MIC_ERROR_INV_ARG       = -2,
    MIC_ERROR_INV_STATE     = -3,
    MIC_ERROR_NO_MEM        = -4,
    MIC_ERROR_ALREADY_OPEN  = -5,
    MIC_ERROR_NOT_OPEN      = -6,
    MIC_ERROR_NOT_INIT      = -7,
    MIC_ERROR_NOT_CONNECTED = -8
} MICError;

typedef enum MICStatusFlag
{
    MIC_STATUS_FLAG_PCM16     = 1 << 0,
    MIC_STATUS_FLAG_OPEN      = 1 << 1,
    MIC_STATUS_FLAG_CONNECTED = 1 << 2
} MICStatusFlag;

typedef enum MICState
{
    MIC_STATE_SAMPLE_RATE       = 0,
    MIC_STATE_GAIN_DB           = 1,
    MIC_STATE_GAIN_MIN          = 2,
    MIC_STATE_GAIN_MAX          = 3,
    MIC_STATE_GAIN_STEP         = 4,
    MIC_STATE_MUTE              = 5,
    MIC_STATE_ECHO_CANCELLATION = 7,
    MIC_STATE_AUTO_SELECTION    = 8,
    MIC_STATE_DIGITAL_GAIN_DB   = 9
} MICState;

typedef enum MICInstance
{
    MIC_INSTANCE_0 = 0,
    MIC_INSTANCE_1 = 1
} MICInstance;

struct MICRingBuffer {
    uint32_t modulus;
    int16_t* base;
};

struct MICStatus {
    MICStatusFlag flags;
    uint32_t available;
    uint32_t readPos;
};

MICHandle MICInit(MICInstance instance, MEMResource* res, MICRingBuffer* rb, MICError* err);
MICError MICUninit(MICHandle mic);
MICError MICOpen(MICHandle mic);
MICError MICGetStatus(MICHandle mic, MICStatus* status);
MICError MICSetDataConsumed(MICHandle mic, uint32_t consumedSamples);
MICError MICSetState(MICHandle mic, MICState state, uint32_t value);
MICError MICGetState(MICHandle mic, MICState state, uint32_t* value);
MICError MICClose(MICHandle mic);

#ifdef __cplusplus
}
#endif
