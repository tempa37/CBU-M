#include "keyboard.h"
#include "gpio.h"
#ifdef i2c
#include "i2c.h"
#endif
#include <stdint.h>



//#define KEYBOARD_PCF8575_ADDR  0x25
//#define KEYBOARD_PCF8575_ADDR  (0x25 << 1)
#define KEYBOARD_PCF8575_ADDR        (0x23U << 1)
//#define KEYBOARD_PCF8575_ADDR        0x23 
#define KEYBOARD_I2C_TIMEOUT_MS      200U
#define KEYBOARD_I2C_READY_TRIES     3U
#define KEYBOARD_PCF8575_INPUT_STATE 0xFFFFU

typedef struct {
  uint16_t web_bit;
  uint8_t key_id;
} keyboard_binding_t;

static const keyboard_binding_t keyboard_bindings[] = {
  { KEYBOARD_WEB_BIT_RIGHT, KEY_ID_RIGHT },
  { KEYBOARD_WEB_BIT_EMERGENCY_STOP, KEY_ID_EMER },
  { KEYBOARD_WEB_BIT_UP, KEY_ID_UP },
  { KEYBOARD_WEB_BIT_LEFT, KEY_ID_LEFT },
  { KEYBOARD_WEB_BIT_APPLY, KEY_ID_INPUT },
  { KEYBOARD_WEB_BIT_DOWN, KEY_ID_DOWN },
  { KEYBOARD_WEB_BIT_CANCEL, KEY_ID_CANSEL },
  { KEYBOARD_WEB_BIT_START, KEY_ID_START },
  { KEYBOARD_WEB_BIT_STOP, KEY_ID_STOP },
};

// queue keyboard
osMessageQueueId_t keyboard_msg_queue = NULL;

osThreadId_t keyboard_task_handle = NULL;

const osThreadAttr_t keyboard_task_handle_attr = {
  .name = "KeyTask",
  .stack_size = 128 * 4,
  .priority = osPriorityBelowNormal,
};

static volatile uint16_t keyboard_web_mask = 0U;
static volatile uint8_t keyboard_use_expander = 0U;
volatile uint8_t keyboard_i2c_ready = 0U;

static void keyboard_routine(uint8_t* id, uint8_t key_id, uint32_t* event_tick, uint8_t* button_id);

static uint16_t keyboard_read_gpio_mask(void) {
  uint16_t web_mask = 0U;
  uint32_t port_state = GPIOD->IDR;

  if ((port_state & K1_Pin) == 0U) {
    web_mask |= KEYBOARD_WEB_BIT_RIGHT;
  }
  if ((port_state & K9_Pin) == 0U) {
    web_mask |= KEYBOARD_WEB_BIT_EMERGENCY_STOP;
  }
  if ((port_state & K2_Pin) == 0U) {
    web_mask |= KEYBOARD_WEB_BIT_UP;
  }
  if ((port_state & K3_Pin) == 0U) {
    web_mask |= KEYBOARD_WEB_BIT_LEFT;
  }
  if ((port_state & K4_Pin) == 0U) {
    web_mask |= KEYBOARD_WEB_BIT_APPLY;
  }
  if ((port_state & K5_Pin) == 0U) {
    web_mask |= KEYBOARD_WEB_BIT_DOWN;
  }
  if ((port_state & K6_Pin) == 0U) {
    web_mask |= KEYBOARD_WEB_BIT_CANCEL;
  }

  port_state = GPIOB->IDR;
  if ((port_state & K7_Pin) == 0U) {
    web_mask |= KEYBOARD_WEB_BIT_START;
  }
  if ((port_state & K8_Pin) == 0U) {
    web_mask |= KEYBOARD_WEB_BIT_STOP;
  }

  return web_mask;
}

#ifdef i2c
static HAL_StatusTypeDef keyboard_read_expander_raw(uint16_t *raw_state) {
  uint8_t rx_buffer[2] = {0};

  if (HAL_I2C_Master_Receive(&hi2c1, KEYBOARD_PCF8575_ADDR, rx_buffer, sizeof(rx_buffer), KEYBOARD_I2C_TIMEOUT_MS) != HAL_OK) {
    return HAL_ERROR;
  }

  *raw_state = (uint16_t)rx_buffer[0] | ((uint16_t)rx_buffer[1] << 8);
  return HAL_OK;
}

static uint16_t keyboard_read_expander_mask(void) {
  uint16_t raw_state = KEYBOARD_PCF8575_INPUT_STATE;

  if (keyboard_read_expander_raw(&raw_state) != HAL_OK) {
    return 0U;
  }

  // keyboard.html already uses the physical PCF8575 bit numbers.
  return (uint16_t)(~raw_state) & KEYBOARD_WEB_MASK_ALL;
}
#endif

static void keyboard_scan_mask(uint16_t web_mask, uint8_t* id, uint32_t* event_tick, uint8_t* button_id) {
  uint32_t i = 0U;

  for (i = 0U; i < (sizeof(keyboard_bindings) / sizeof(keyboard_bindings[0])); i++) {
    if ((web_mask & keyboard_bindings[i].web_bit) != 0U) {
      keyboard_routine(id, keyboard_bindings[i].key_id, event_tick, button_id);
    }
  }
}

void keyboard_probe(void) {
#ifdef i2c
  uint8_t tx_buffer[2] = {
    (uint8_t)(KEYBOARD_PCF8575_INPUT_STATE & 0xFFU),
    (uint8_t)(KEYBOARD_PCF8575_INPUT_STATE >> 8),
  };
  uint16_t raw_state = 0U;
  HAL_StatusTypeDef state = HAL_ERROR;

  keyboard_use_expander = 0U;
  keyboard_i2c_ready = 0U;

  state = HAL_I2C_IsDeviceReady(&hi2c1, KEYBOARD_PCF8575_ADDR, KEYBOARD_I2C_READY_TRIES, 500U);
  if (state != HAL_OK) {
    return;
  }

  if (HAL_I2C_Master_Transmit(&hi2c1, KEYBOARD_PCF8575_ADDR, tx_buffer, sizeof(tx_buffer), KEYBOARD_I2C_TIMEOUT_MS) != HAL_OK) {
    return;
  }

  if (keyboard_read_expander_raw(&raw_state) != HAL_OK) {
    return;
  }

  keyboard_use_expander = 1U;
  keyboard_i2c_ready = 1U;
#else
  keyboard_use_expander = 0U;
  keyboard_i2c_ready = 0U;
#endif
}

uint16_t keyboard_read_state_mask(void) {
#ifdef i2c
  if (keyboard_use_expander != 0U) {
    return keyboard_read_expander_mask();
  }
#endif

  return keyboard_read_gpio_mask();
}

uint16_t keyboard_get_web_mask(void) {
  return keyboard_web_mask;
}

/**
 * @brief keyboard routine
 * @param id
 * @param key_id
 * @param event_tick
 * @param button_id
 * @retval None
 */
static void keyboard_routine(uint8_t* id, uint8_t key_id, uint32_t* event_tick, uint8_t* button_id) {
  uint32_t hal_tick = HAL_GetTick();
  if (*id == KEY_ID_NO) {
    *event_tick = hal_tick;
    *id = key_id;
  }
  if (*id == key_id && (hal_tick - *event_tick) >= KEYBOARD_DEBOUNCE_TIME) {
    *button_id = key_id;
  }
}

/**
 * @brief keyboard handler
 * @param argument: Not used
 * @retval None
 */
void keyboard_thread(void *argument) {
  (void) argument;

  uint8_t button_id = KEY_ID_NO;

  uint8_t id = KEY_ID_NO;

  uint32_t event_tick = 0;

  button_t button = {EVENT_ONE_CLICK, KEY_ID_NO};

  uint32_t threshold_time = 0;

  uint8_t previous_id = KEY_ID_NO;

  uint32_t previous_tick = 0;

  uint8_t flag = 0;

  uint32_t hal_tick;
  uint16_t web_mask;

  while(1) {

    hal_tick = HAL_GetTick();

    web_mask = keyboard_read_state_mask();
    keyboard_scan_mask(web_mask, &id, &event_tick, &button_id);

    keyboard_web_mask = web_mask;

    if (button_id != KEY_ID_NO) {
      button.id = button_id;

      if (button_id == previous_id) {
        threshold_time = hal_tick - previous_tick;
        if (threshold_time < BUTTON_SHORT_PRESS_THRESHOLD) {
          button.event = EVENT_LONG_PRESS;
        } else if (threshold_time >= BUTTON_SHORT_PRESS_THRESHOLD) {
          button.event = EVENT_SHORT_PRESS;
        }
      } else {
        button.event = EVENT_ONE_CLICK;
      }

      previous_tick = hal_tick;
      previous_id = button_id;

      osMessageQueuePut(keyboard_msg_queue, &button, 0U, 0U);

      //osDelay(KEYBOARD_IDLE_TIME - KEYBOARD_DEBOUNCE_TIME);
      osDelay(KEYBOARD_DEBOUNCE_TIME);
      button_id = KEY_ID_NO;
      id = KEY_ID_NO;
      flag = 1;
    }

    if ((previous_tick + KEYBOARD_IDLE_TIME + KEYBOARD_DEBOUNCE_TIME) < hal_tick && flag) {
      flag = 0;
      button.id = KEY_ID_NO;
      id = KEY_ID_NO;
      osMessageQueuePut(keyboard_msg_queue, &button, 0U, 0U);
    }

    if ((id > KEY_ID_NO) && ((hal_tick - event_tick) >= KEYBOARD_IDLE_TIME)) {
      id = KEY_ID_NO;
    }

    osDelay(10);
  }
}
