#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "cmsis_os2.h"

#define KEYBOARD_DEBOUNCE_TIME 60
#define KEYBOARD_IDLE_TIME 180

#define BUTTON_LONG_PRESS_THRESHOLD 500
#define BUTTON_SHORT_PRESS_THRESHOLD 600

typedef enum key {
  KEY_ID_NO,
  KEY_ID_RIGHT,
  KEY_ID_UP,
  KEY_ID_LEFT,
  KEY_ID_INPUT,
  KEY_ID_DOWN,
  KEY_ID_CANSEL,
  KEY_ID_START,
  KEY_ID_STOP,
  KEY_ID_EMER,
} key_type_t;

typedef enum _button_event {
  EVENT_ONE_CLICK,
  EVENT_SHORT_PRESS,
  EVENT_LONG_PRESS,
} button_event_t;

typedef struct _button {
  button_event_t event;
  uint8_t id;
} button_t;

// queue keyboard
extern osMessageQueueId_t keyboard_msg_queue;
extern osThreadId_t keyboard_task_handle;
extern const osThreadAttr_t keyboard_task_handle_attr;

void keyboard_thread(void *argument);

#endif /* __KEYBOARD_H__ */