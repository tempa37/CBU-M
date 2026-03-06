#include "ring_line.h"

/* Contour 1 state (legacy global) and contour 2 state. */
volatile ring_line_state_t g_ring_line_state = RING_LINE_BREAK_CIRCUIT;
volatile ring_line_state_t g_ring_line_state_2 = RING_LINE_BREAK_CIRCUIT;

volatile uint16_t g_ring_line_opto_1_1_count = 0;
volatile uint16_t g_ring_line_opto_1_2_count = 0;
volatile uint16_t g_ring_line_opto_2_1_count = 0;
volatile uint16_t g_ring_line_opto_2_2_count = 0;

#define RING_LINE_BUF_SIZE           320U
#define RING_LINE_BUF_WORDS          (RING_LINE_BUF_SIZE / 64U)
#define RING_LINE_MID_MIN_COUNT      128U
#define RING_LINE_MID_MAX_COUNT      192U
#define RING_LINE_HIGH_LEVEL_COUNT   288U

static volatile uint64_t ring_line_opto_1_1[RING_LINE_BUF_WORDS] = {0};
static volatile uint64_t ring_line_opto_1_2[RING_LINE_BUF_WORDS] = {0};
static volatile uint64_t ring_line_opto_2_1[RING_LINE_BUF_WORDS] = {0};
static volatile uint64_t ring_line_opto_2_2[RING_LINE_BUF_WORDS] = {0};

static volatile uint16_t ring_line_sample_index = 0;
static volatile uint8_t ring_line_window_ready = 0;
static volatile uint8_t ring_line_collect_enabled = 1;

static uint16_t ring_line_count_bits_set_parallel(uint64_t x) {
  x -= (x >> 1U) & 0x5555555555555555ULL;
  x = (x & 0x3333333333333333ULL) + ((x >> 2U) & 0x3333333333333333ULL);
  x = (x + (x >> 4U)) & 0x0f0f0f0f0f0f0f0fULL;
  return (uint16_t)((x * 0x0101010101010101ULL) >> 56U);
}

static ring_line_state_t ring_line_resolve_state(uint16_t count_1, uint16_t count_2, ring_line_state_t prev_state) {
  uint8_t line_1_is_mid = (count_1 >= RING_LINE_MID_MIN_COUNT) && (count_1 <= RING_LINE_MID_MAX_COUNT);
  uint8_t line_2_is_mid = (count_2 >= RING_LINE_MID_MIN_COUNT) && (count_2 <= RING_LINE_MID_MAX_COUNT);
  uint8_t line_1_is_high = (count_1 >= RING_LINE_HIGH_LEVEL_COUNT);
  uint8_t line_2_is_high = (count_2 >= RING_LINE_HIGH_LEVEL_COUNT);

  if ((line_1_is_mid && line_2_is_high) || (line_2_is_mid && line_1_is_high)) {
    return RING_LINE_DIODE_CIRCUIT;
  }

  if (line_1_is_high && line_2_is_high) {
    return RING_LINE_BREAK_CIRCUIT;
  }

  if (line_1_is_mid && line_2_is_mid) {
    return RING_LINE_SHORT_CIRCUIT;
  }

  return prev_state;
}

static void ring_line_reset_window(void) {
  uint16_t i;

  for (i = 0; i < RING_LINE_BUF_WORDS; i++) {
    ring_line_opto_1_1[i] = 0;
    ring_line_opto_1_2[i] = 0;
    ring_line_opto_2_1[i] = 0;
    ring_line_opto_2_2[i] = 0;
  }

  ring_line_sample_index = 0;
  ring_line_window_ready = 0;
  ring_line_collect_enabled = 1;
}

void ring_line_capture_isr(uint8_t opto_1_1, uint8_t opto_1_2, uint8_t opto_2_1, uint8_t opto_2_2) {
  uint16_t word_index;
  uint16_t bit_index;

  if ((ring_line_collect_enabled == 0U) || (ring_line_window_ready != 0U)) {
    return;
  }

  word_index = ring_line_sample_index / 64U;
  bit_index = ring_line_sample_index % 64U;

  if (opto_1_1 != 0U) {
    ring_line_opto_1_1[word_index] |= (1ULL << bit_index);
  } else {
    ring_line_opto_1_1[word_index] &= ~(1ULL << bit_index);
  }

  if (opto_1_2 != 0U) {
    ring_line_opto_1_2[word_index] |= (1ULL << bit_index);
  } else {
    ring_line_opto_1_2[word_index] &= ~(1ULL << bit_index);
  }

  if (opto_2_1 != 0U) {
    ring_line_opto_2_1[word_index] |= (1ULL << bit_index);
  } else {
    ring_line_opto_2_1[word_index] &= ~(1ULL << bit_index);
  }

  if (opto_2_2 != 0U) {
    ring_line_opto_2_2[word_index] |= (1ULL << bit_index);
  } else {
    ring_line_opto_2_2[word_index] &= ~(1ULL << bit_index);
  }

  ring_line_sample_index++;

  if (ring_line_sample_index >= RING_LINE_BUF_SIZE) {
    ring_line_collect_enabled = 0;
    ring_line_window_ready = 1;
  }
}

void ring_line_process_task_step(void) {
  uint16_t i;
  uint16_t count_1_1 = 0;
  uint16_t count_1_2 = 0;
  uint16_t count_2_1 = 0;
  uint16_t count_2_2 = 0;

  if (ring_line_window_ready == 0U) {
    return;
  }

  for (i = 0; i < RING_LINE_BUF_WORDS; i++) {
    count_1_1 += ring_line_count_bits_set_parallel(ring_line_opto_1_1[i]);
    count_1_2 += ring_line_count_bits_set_parallel(ring_line_opto_1_2[i]);
    count_2_1 += ring_line_count_bits_set_parallel(ring_line_opto_2_1[i]);
    count_2_2 += ring_line_count_bits_set_parallel(ring_line_opto_2_2[i]);
  }

  g_ring_line_opto_1_1_count = count_1_1;
  g_ring_line_opto_1_2_count = count_1_2;
  g_ring_line_opto_2_1_count = count_2_1;
  g_ring_line_opto_2_2_count = count_2_2;

  g_ring_line_state = ring_line_resolve_state(count_1_1, count_1_2, g_ring_line_state);
  g_ring_line_state_2 = ring_line_resolve_state(count_2_1, count_2_2, g_ring_line_state_2);

  ring_line_reset_window();
}
