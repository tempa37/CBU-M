#ifndef __RING_LINE_H__
#define __RING_LINE_H__

#include "stdint.h"

typedef enum {
  RING_LINE_SHORT_CIRCUIT = 0,
  RING_LINE_BREAK_CIRCUIT = 1,
  RING_LINE_DIODE_CIRCUIT = 2,
} ring_line_state_t;

extern volatile ring_line_state_t g_ring_line_state;
extern volatile ring_line_state_t g_ring_line_state_2;

extern volatile uint16_t g_ring_line_opto_1_1_count;
extern volatile uint16_t g_ring_line_opto_1_2_count;
extern volatile uint16_t g_ring_line_opto_2_1_count;
extern volatile uint16_t g_ring_line_opto_2_2_count;

void ring_line_capture_isr(uint8_t opto_1_1, uint8_t opto_1_2, uint8_t opto_2_1, uint8_t opto_2_2);
void ring_line_process_task_step(void);

#endif /* __RING_LINE_H__ */
