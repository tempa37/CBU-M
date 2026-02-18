#ifndef __GLOBAL_H__
#define __GLOBAL_H__

//#include "lwip/ip_addr.h"
#include "cmsis_os.h"

#define HYDRAULIC 0
#define MAINLINE 1

#define AUTO_SYS_CONTROL 1
#define LOCAL_SYS_CONTROL 0

#define DO_NRUN 0
#define DO_RUN 1

#define DI_NRUN 0
#define DI_RUN 1
#define DI_UNDEFINED 2

#if CHECK_LOAD_SYSTEM == 1
#define DISPLAY_LOAD_BIT (1 << 0)
#define MAIN_LOAD_BIT (1 << 1)
//#define DATA_BIT (1 << 2)
//#define MODBUS_BIT (1 << 3)
//#define nRING_LINE_BIT (1 << 4)
//#define nBAN_TEST_BIT (1 << 5)
#endif

//#define STROKE_UP 1
//#define STROKE_DOWN -1
//#define STROKE_STOP 0

#ifdef DEBUG
extern UBaseType_t WM_MBSlaveRTU;
extern UBaseType_t WM_MBSlaveTCP;
extern UBaseType_t WM_MBMasterRTU;
extern UBaseType_t WM_MBMasterTCP;
extern UBaseType_t WM_LWIP;
extern UBaseType_t WM_Scheduler;
extern UBaseType_t WM_OLED;
extern UBaseType_t WM_HTTPD;
#endif

typedef enum valve_state {
  VALVE_STOP,
  VALVE_UP,
  VALVE_DOWN
} valve_state_t;

typedef struct valve_control {
  uint32_t tick;
  valve_state_t state;
} valve_control_t;

typedef enum {
  STATE_UNDEFINED = 0,
  STATE_OFFLINE = 1,
  STATE_ONLINE = 2,
  STATE_MIN_LIM = 3,
  STATE_BELOW_NORMAL = 4,
  STATE_NORMAL = 5,
  STATE_ABOVE_NORMAL = 6,
  STATE_MAX_LIM = 7,
} state_t;

typedef struct st_sensor {
  uint8_t state;
  float value;
  float min_value;
  float max_value;
} sensor_t;

//typedef union {
//  uint8_t value;
//  struct {
//    uint8_t in1 :1;
//    uint8_t in2 :1;
//    uint8_t in3 :1;
//    uint8_t in4 :1;
//    uint8_t in5 :1;
//    uint8_t in6 :1;
//    uint8_t in7 :1;
//    uint8_t in8 :1;
//  };
//} module_t;
    
typedef struct {
  uint8_t state;
  uint8_t value_out;
  uint8_t out_up;
  uint8_t out_down;
} um_out_t;

typedef union s_pin {
  uint16_t value;
  struct {
    uint8_t pin0:4;
    uint8_t pin1:4;
    uint8_t pin2:4;
    uint8_t pin4:4;
  };
} pin_t;

typedef struct {
  uint8_t menu_id;
  
  float value1;
  float value2;
  uint16_t value3;
  uint16_t value4;
  int16_t value5;
  
  pin_t pin;

  const char* title;
  const char* sub_title;
  
  sensor_t pressure_sensor[2];

  um_out_t valve;
  
  uint8_t emer_state;
  
  uint8_t state_in;
  uint8_t state_out;
  
  uint16_t position;
  uint8_t position_state;
  
  uint8_t umvh_state;
  uint8_t urm_state;
  uint8_t save_state;
  
  uint8_t system_control_mode;
} oled_t;

extern oled_t oled_msg;

#endif /* __GLOBAL_H__ */