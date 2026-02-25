#include "ring_line.h"

/* ===================== RING LINE RESULT (VISIBLE GLOBAL STATE) ===================== */
volatile ring_line_state_t g_ring_line_state = RING_LINE_BREAK_CIRCUIT;
/* ============================================================================= */

#define RING_LINE_BUF_SIZE           320U
#define RING_LINE_BUF_WORDS          (RING_LINE_BUF_SIZE / 64U)
#define RING_LINE_MID_MIN_COUNT      128U
#define RING_LINE_MID_MAX_COUNT      192U
#define RING_LINE_HIGH_LEVEL_COUNT   288U

static volatile uint64_t ring_line_ch1[RING_LINE_BUF_WORDS] = {0};
static volatile uint64_t ring_line_ch2[RING_LINE_BUF_WORDS] = {0};

static volatile uint16_t ring_line_sample_index = 0;
static volatile uint8_t ring_line_window_ready = 0;
static volatile uint8_t ring_line_collect_enabled = 1;

static uint16_t ring_line_count_bits_set_parallel(uint64_t x) {
  x -= (x >> 1U) & 0x5555555555555555ULL;
  x = (x & 0x3333333333333333ULL) + ((x >> 2U) & 0x3333333333333333ULL);
  x = (x + (x >> 4U)) & 0x0f0f0f0f0f0f0f0fULL;
  return (uint16_t)((x * 0x0101010101010101ULL) >> 56U);
}

static void ring_line_reset_window(void) {
  uint16_t i;

  for (i = 0; i < RING_LINE_BUF_WORDS; i++) {
    ring_line_ch1[i] = 0;
    ring_line_ch2[i] = 0;
  }

  ring_line_sample_index = 0;
  ring_line_window_ready = 0;
  ring_line_collect_enabled = 1;
}

void ring_line_capture_isr(uint8_t ch1, uint8_t ch2) {
  uint16_t word_index;
  uint16_t bit_index;

  if ((ring_line_collect_enabled == 0U) || (ring_line_window_ready != 0U)) {
    return;
  }

  word_index = ring_line_sample_index / 64U;
  bit_index = ring_line_sample_index % 64U;

  if (ch1 != 0U) {
    ring_line_ch1[word_index] |= (1ULL << bit_index);
  } else {
    ring_line_ch1[word_index] &= ~(1ULL << bit_index);
  }

  if (ch2 != 0U) {
    ring_line_ch2[word_index] |= (1ULL << bit_index);
  } else {
    ring_line_ch2[word_index] &= ~(1ULL << bit_index);
  }

  ring_line_sample_index++;

  if (ring_line_sample_index >= RING_LINE_BUF_SIZE) {
    ring_line_collect_enabled = 0;
    ring_line_window_ready = 1;
  }
}

void ring_line_process_task_step(void) {
  uint16_t i;
  uint16_t line1_count = 0;
  uint16_t line2_count = 0;
  uint8_t line1_is_mid;
  uint8_t line2_is_mid;
  uint8_t line1_is_high;
  uint8_t line2_is_high;

  if (ring_line_window_ready == 0U) {
    return;
  }

  for (i = 0; i < RING_LINE_BUF_WORDS; i++) {
    line1_count += ring_line_count_bits_set_parallel(ring_line_ch1[i]);
    line2_count += ring_line_count_bits_set_parallel(ring_line_ch2[i]);
  }

  line1_is_mid = (line1_count >= RING_LINE_MID_MIN_COUNT) && (line1_count <= RING_LINE_MID_MAX_COUNT);
  line2_is_mid = (line2_count >= RING_LINE_MID_MIN_COUNT) && (line2_count <= RING_LINE_MID_MAX_COUNT);
  line1_is_high = (line1_count >= RING_LINE_HIGH_LEVEL_COUNT);
  line2_is_high = (line2_count >= RING_LINE_HIGH_LEVEL_COUNT);

  if ((line1_is_mid && line2_is_high) || (line2_is_mid && line1_is_high)) {
    g_ring_line_state = RING_LINE_DIODE_CIRCUIT;
  } else if (line1_is_high && line2_is_high) {
    g_ring_line_state = RING_LINE_BREAK_CIRCUIT;
  } else if (line1_is_mid && line2_is_mid) {
    g_ring_line_state = RING_LINE_SHORT_CIRCUIT;
  } else {
    // Between thresholds: keep previous state.
  }

  ring_line_reset_window();
}
