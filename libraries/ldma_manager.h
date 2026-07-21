#ifndef LDMA_MANAGER_H_
#define LDMA_MANAGER_H_

#include <stdbool.h>
#include <stdint.h>

#if defined(EMDRV_DMADRV_DMA_CH_COUNT)
#define LDMA_MANAGER_CHANNEL_COUNT ((unsigned int)EMDRV_DMADRV_DMA_CH_COUNT)
#else
#define LDMA_MANAGER_CHANNEL_COUNT (8u)
#endif

#define LDMA_MANAGER_INVALID_CHANNEL ((unsigned int)UINT32_MAX)

typedef bool (*ldma_manager_callback_t)(unsigned int channel,
                                        unsigned int sequence_no,
                                        void *user_context);

bool ldma_manager__init(void);
bool ldma_manager__allocate(unsigned int *channel_out);
bool ldma_manager__allocate_specific(unsigned int channel);
void ldma_manager__free(unsigned int channel);
bool ldma_manager__is_allocated(unsigned int channel);
unsigned int ldma_manager__channel_count(void);
bool ldma_manager__set_callback(unsigned int channel,
                                ldma_manager_callback_t callback,
                                void *user_context);

#endif /* LDMA_MANAGER_H_ */
