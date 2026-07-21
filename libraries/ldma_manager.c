#include "ldma_manager.h"

#include "em_core.h"
#include "em_ldma.h"
#include <stddef.h>

#if (LDMA_MANAGER_CHANNEL_COUNT < 1u)
#error "LDMA_MANAGER_CHANNEL_COUNT must be at least 1"
#endif

#if (LDMA_MANAGER_CHANNEL_COUNT > 32u)
#error "LDMA_MANAGER_CHANNEL_COUNT must be 32 or less"
#endif

static uint32_t s_allocated_channels_mask = 0u;
static unsigned int s_next_search_channel = 0u;
static ldma_manager_callback_t s_callbacks[LDMA_MANAGER_CHANNEL_COUNT] = { 0 };
static void *s_callback_contexts[LDMA_MANAGER_CHANNEL_COUNT] = { 0 };
static unsigned int s_callback_sequence_numbers[LDMA_MANAGER_CHANNEL_COUNT] = { 0 };

// -----------------------------------------------------------------------------
//                     LDMA Manager Helpers
// -----------------------------------------------------------------------------

static bool ldma_manager__channel_is_valid(unsigned int channel)
{
  return channel < LDMA_MANAGER_CHANNEL_COUNT;
}

// -----------------------------------------------------------------------------
//                     LDMA Manager Helpers End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     LDMA Manager General
// -----------------------------------------------------------------------------

bool ldma_manager__init(void)
{
  LDMA_Init_t init = LDMA_INIT_DEFAULT;

  LDMA_Init(&init);

  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  s_allocated_channels_mask = 0u;
  s_next_search_channel = 0u;
  for (unsigned int channel = 0u; channel < LDMA_MANAGER_CHANNEL_COUNT; ++channel) {
    s_callbacks[channel] = NULL;
    s_callback_contexts[channel] = NULL;
    s_callback_sequence_numbers[channel] = 0u;
  }
  CORE_EXIT_CRITICAL();
  return true;
}

unsigned int ldma_manager__channel_count(void)
{
  return LDMA_MANAGER_CHANNEL_COUNT;
}

bool ldma_manager__is_allocated(unsigned int channel)
{
  bool is_allocated = false;

  if (!ldma_manager__channel_is_valid(channel)) {
    return false;
  }

  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  is_allocated = ((s_allocated_channels_mask & (1u << channel)) != 0u);
  CORE_EXIT_CRITICAL();

  return is_allocated;
}

bool ldma_manager__allocate_specific(unsigned int channel)
{
  bool allocated = false;

  if (!ldma_manager__channel_is_valid(channel)) {
    return false;
  }

  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  if ((s_allocated_channels_mask & (1u << channel)) == 0u) {
    s_allocated_channels_mask |= (1u << channel);
    s_next_search_channel = (channel + 1u) % LDMA_MANAGER_CHANNEL_COUNT;
    allocated = true;
  }
  CORE_EXIT_CRITICAL();

  return allocated;
}

bool ldma_manager__allocate(unsigned int *channel_out)
{
  bool allocated = false;

  if (channel_out == NULL) {
    return false;
  }

  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  for (unsigned int attempt = 0u; attempt < LDMA_MANAGER_CHANNEL_COUNT; ++attempt) {
    const unsigned int channel =
        (s_next_search_channel + attempt) % LDMA_MANAGER_CHANNEL_COUNT;
    const uint32_t channel_mask = (1u << channel);

    if ((s_allocated_channels_mask & channel_mask) == 0u) {
      s_allocated_channels_mask |= channel_mask;
      s_next_search_channel = (channel + 1u) % LDMA_MANAGER_CHANNEL_COUNT;
      *channel_out = channel;
      allocated = true;
      break;
    }
  }
  CORE_EXIT_CRITICAL();

  return allocated;
}

void ldma_manager__free(unsigned int channel)
{
  if (!ldma_manager__channel_is_valid(channel)) {
    return;
  }

  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  LDMA_StopTransfer((int)channel);
  LDMA_IntDisable(1u << channel);
  LDMA_IntClear(1u << channel);
  s_allocated_channels_mask &= ~(1u << channel);
  s_callbacks[channel] = NULL;
  s_callback_contexts[channel] = NULL;
  s_callback_sequence_numbers[channel] = 0u;
  if (s_next_search_channel >= LDMA_MANAGER_CHANNEL_COUNT) {
    s_next_search_channel = 0u;
  }
  CORE_EXIT_CRITICAL();
}

bool ldma_manager__set_callback(unsigned int channel,
                                ldma_manager_callback_t callback,
                                void *user_context)
{
  if (!ldma_manager__channel_is_valid(channel)) {
    return false;
  }

  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();
  s_callbacks[channel] = callback;
  s_callback_contexts[channel] = user_context;
  s_callback_sequence_numbers[channel] = 0u;
  CORE_EXIT_CRITICAL();

  return true;
}

// -----------------------------------------------------------------------------
//                     LDMA Manager General End
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                     Interrupt Handlers
// -----------------------------------------------------------------------------

void LDMA_IRQHandler(void)
{
  uint32_t pending = LDMA_IntGetEnabled();

  if ((pending & LDMA_IF_ERROR) != 0u) {
    LDMA_IntClear(LDMA_IF_ERROR);
  }

  for (unsigned int channel = 0u; channel < LDMA_MANAGER_CHANNEL_COUNT; ++channel) {
    const uint32_t channel_mask = (1u << channel);

    if ((pending & channel_mask) == 0u) {
      continue;
    }

    LDMA_IntClear(channel_mask);

    if (s_callbacks[channel] != NULL) {
      const unsigned int sequence_no = s_callback_sequence_numbers[channel]++;
      (void)s_callbacks[channel](channel,
                                 sequence_no,
                                 s_callback_contexts[channel]);
    }
  }
}

// -----------------------------------------------------------------------------
//                     Interrupt Handlers End
// -----------------------------------------------------------------------------
