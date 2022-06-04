/*
 * Excerpt from libcafe/include/wiiu/dynlib/coreinit/mem.h
 *
 * The whole file is not compatible with the wut headers, therefore
 * only import the required parts.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MEMResourceElement MEMResourceElement;
typedef struct MEMResource MEMResource;

typedef enum MEMResourceFlag
{
    MEM_RESOURCE_FLAG_DMA         = 1 << 0,
    MEM_RESOURCE_FLAG_ITD         = 1 << 1,
    MEM_RESOURCE_FLAG_ITD_PAYLOAD = 1 << 2,
    MEM_RESOURCE_FLAG_IPC         = 1 << 3,
    MEM_RESOURCE_FLAG_INST        = 1 << 4,
    MEM_RESOURCE_FLAG_OVERPROVIDE = 1 << 5
} MEMResourceFlag;

struct MEMResourceElement {
    uint32_t        size;
    uint32_t        alignment;
    uint32_t        quantum;
    uint32_t        numQuanta;
    MEMResourceFlag flags;
    void*           pv;
};

struct MEMResource {
    uint32_t            numMemRes;
    MEMResourceElement* memRes;
};

#ifdef __cplusplus
}
#endif

