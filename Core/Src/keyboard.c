#include "keyboard.h"
#include "gpio.h"
#include <stdint.h>

// queue keyboard
osMessageQueueId_t keyboard_msg_queue = NULL;

osThreadId_t keyboard_task_handle = NULL;

const osThreadAttr_t keyboard_task_handle_attr = {
  .name = "KeyTask",
  .stack_size = 128 * 4,
  .priority = osPriorityBelowNormal,
};

static volatile uint16_t keyboard_web_mask = 0U;

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
void keyboard_routine(uint8_t* id, uint8_t key_id, uint32_t* event_tick, uint8_t* button_id) {
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

  uint32_t port_state = 0;

  button_t button = {EVENT_ONE_CLICK, KEY_ID_NO};

  uint32_t threshold_time = 0;

  uint8_t previous_id = KEY_ID_NO;

  uint32_t previous_tick = 0;

  uint8_t flag = 0;

  uint32_t hal_tick;

  uint16_t web_mask = 0U;

  while(1) {

    hal_tick = HAL_GetTick();

    web_mask = 0U;

    port_state = GPIOD->IDR;

    if (!(port_state & GPIO_PIN_0)) {
      web_mask |= KEYBOARD_WEB_BIT_RIGHT;
      keyboard_routine(&id, KEY_ID_RIGHT, &event_tick, &button_id);
    }
    if (!(port_state & GPIO_PIN_1)) {
      web_mask |= KEYBOARD_WEB_BIT_EMERGENCY_STOP;
      keyboard_routine(&id, KEY_ID_EMER, &event_tick, &button_id);
    }
    if (!(port_state & GPIO_PIN_2)) {
      web_mask |= KEYBOARD_WEB_BIT_UP;
      keyboard_routine(&id, KEY_ID_UP, &event_tick, &button_id);
    }
    if (!(port_state & GPIO_PIN_4)) {
      web_mask |= KEYBOARD_WEB_BIT_LEFT;
      keyboard_routine(&id, KEY_ID_LEFT, &event_tick, &button_id);
    }
    if (!(port_state & GPIO_PIN_5)) {
      web_mask |= KEYBOARD_WEB_BIT_APPLY;
      keyboard_routine(&id, KEY_ID_INPUT, &event_tick, &button_id);
    }
    if (!(port_state & GPIO_PIN_6)) {
      web_mask |= KEYBOARD_WEB_BIT_DOWN;
      keyboard_routine(&id, KEY_ID_DOWN, &event_tick, &button_id);
    }
    if (!(port_state & GPIO_PIN_7)) {
      web_mask |= KEYBOARD_WEB_BIT_CANCEL;
      keyboard_routine(&id, KEY_ID_CANSEL, &event_tick, &button_id);
    }

    port_state = GPIOB->IDR;
    if (!(port_state & GPIO_PIN_3)) {
      web_mask |= KEYBOARD_WEB_BIT_START;
      keyboard_routine(&id, KEY_ID_START, &event_tick, &button_id);
    }
    if (!(port_state & GPIO_PIN_4)) {
      web_mask |= KEYBOARD_WEB_BIT_STOP;
      keyboard_routine(&id, KEY_ID_STOP, &event_tick, &button_id);
    }

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
