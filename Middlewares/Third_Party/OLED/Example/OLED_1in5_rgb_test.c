#include "test.h"
#include "OLED_1in5_rgb.h"
#include "ImageData.h"
#include <stdbool.h>
#include "cmsis_os2.h"
#include "string.h"
#include "version.h"
#include "global_types.h"
#include "manage_settings.h"

// Queue OLED
extern osMessageQueueId_t oled_msg_queue;
extern void WriteReg(uint8_t Reg);
extern uint8_t BlackImage[32768];
extern osEventFlagsId_t load_system_flags;

void oled_thread(void *argument);

#define HOLDING_TIME 3000
#define INIT_TIME 10000
#define CLEAR_TIMEOUT 5
#define FONT_COLOR WHITE

#define DEBUG_OLED 2

const char* mpa = "MPa";
const char* mm = "mm";

const char* main_line = "МАГИСТРАЛЬ, МПа";
const char* hydraulic_line = "ЦИЛИНДР, МПа";
const char* emer_stop = "СТОП";
const char* set_point_min = "УСТАВКА MIN, МПа";
const char* set_point_max = "УСТАВКА MAX, МПа";

void paint_pin(uint8_t num, pin_t* p) {
  // clear all
  Paint_ClearWindows(0, 35, 128, 105, BLACK);
  
#define x1 53
#define x2 75
  switch (num) {
    case 0: {
      Paint_DrawNum(57, 37, p->pin0 , &Font20, 0, BLACK, BLUE);
      Paint_DrawLine(57, 56, 71, 56, WHITE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
      Paint_DrawNum(57, 60, p->pin1, &Font20, 0, BLACK, FONT_COLOR);
      Paint_DrawNum(57, 83, p->pin2, &Font20, 0, BLACK, FONT_COLOR);
      break;
    }
    case 1: {
      Paint_DrawNum(57, 37, p->pin0, &Font20, 0, BLACK, FONT_COLOR);      
      Paint_DrawNum(57, 60, p->pin1, &Font20, 0, BLACK, BLUE);
      Paint_DrawLine(57, 79, 71, 79, WHITE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
      Paint_DrawNum(57, 83, p->pin2, &Font20, 0, BLACK, FONT_COLOR);
      break;
    }
    case 2 : {
      Paint_DrawNum(57, 37, p->pin0, &Font20, 0, BLACK, FONT_COLOR);
      Paint_DrawNum(57, 60, p->pin1, &Font20, 0, BLACK, FONT_COLOR);      
      Paint_DrawNum(57, 83, p->pin2, &Font20, 0, BLACK, BLUE);
      Paint_DrawLine(57, 102, 71, 102, WHITE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
      break;
    }
  }
}

void paint_pressure(uint8_t num, float* v1, float* v2, const char* unit) {
  // clear all
  Paint_ClearWindows(0, 35, 128, 105, BLACK);
#define x3 6
#define x4 85
#define x5 3
  
  switch (num) {
    case 0: {
      // value 1
      Paint_DrawNum(x3, 35, (double)*v1, &Font20, 1, BLACK, FONT_COLOR);
      Paint_DrawString_EN(x4, 35, unit, &Font20, FONT_COLOR, BLACK);
      // value 2
      Paint_DrawNum(x3, 60, (double)*v2, &Font20, 1, BLACK, FONT_COLOR);
      Paint_DrawString_EN(x4, 60, unit, &Font20, FONT_COLOR, BLACK);
      //
      break;
    }
    case 1: {
      // selector
      Paint_DrawCircle(x5, 68, 3, RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);
      // value 1
      Paint_DrawNum(x3, 35, (double)*v1, &Font20, 1, BLACK, FONT_COLOR);
      Paint_DrawString_EN(x4, 35, unit, &Font20, FONT_COLOR, BLACK);
      // value 2
      Paint_DrawNum(x3, 60, (double)*v2, &Font20, 1, BLACK, FONT_COLOR);
      Paint_DrawString_EN(x4, 60, unit, &Font20, FONT_COLOR, BLACK);
      //
      break;
    }
    case 2: {
      // value 1
      Paint_DrawNum(x3, 35, (double)*v1, &Font20, 1, BLACK, FONT_COLOR);
      Paint_DrawString_EN(x4, 35, unit, &Font20, FONT_COLOR, BLACK);
      // selector
      Paint_DrawCircle(x5, 43, 3, RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);
      // value 2
      Paint_DrawNum(x3, 60, (double)*v2, &Font20, 1, BLACK, FONT_COLOR);
      Paint_DrawString_EN(x4, 60, unit, &Font20, FONT_COLOR, BLACK);
      //
      break;
    }
  }
}

void Paint_triangle_horizontal(UWORD Xstart, UWORD Ystart, UWORD Color, uint8_t type) {
  if (type == 0) {
    Paint_DrawLine(Xstart,     Ystart,     Xstart, Ystart + 8, Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(Xstart - 4, Ystart + 4, Xstart, Ystart,     Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(Xstart - 4, Ystart + 4, Xstart, Ystart + 8, Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    
    Paint_DrawLine(Xstart - 1, Ystart + 1, Xstart - 1, Ystart + 6, Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(Xstart - 2, Ystart + 2, Xstart - 2, Ystart + 5, Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(Xstart - 3, Ystart + 3, Xstart - 3, Ystart + 4, Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    
  } else if (type == 1) {
    Paint_DrawLine(Xstart, Ystart, Xstart, Ystart + 8, Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(Xstart, Ystart, Xstart + 5, Ystart + 4, Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(Xstart, Ystart + 8, Xstart + 5, Ystart + 4, Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    
    Paint_DrawLine(Xstart + 1, Ystart + 1, Xstart + 1, Ystart + 6, Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(Xstart + 2, Ystart + 2, Xstart + 2, Ystart + 5, Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(Xstart + 3, Ystart + 3, Xstart + 3, Ystart + 4, Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  }
}

void Paint_triangle_vertical(UWORD Xstart, UWORD Ystart, UWORD Color, uint8_t type) {
  if (type == 0) {
    Paint_DrawLine(Xstart - 10, Ystart + 10, Xstart,      Ystart,      Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(Xstart,      Ystart,      Xstart + 10, Ystart + 10, Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(Xstart - 10, Ystart + 10, Xstart + 10, Ystart + 10, Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    for (uint8_t i = 1; i < 10; i++) {
      Paint_DrawLine(Xstart - i,  Ystart + i,  Xstart + i,  Ystart + i,  Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }
  } else if (type == 1) {
    Paint_DrawLine(Xstart - 10, Ystart - 10, Xstart,      Ystart,      Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(Xstart,      Ystart,      Xstart + 10, Ystart - 10, Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(Xstart - 10, Ystart - 10, Xstart + 10, Ystart - 10, Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    for (uint8_t i = 1; i < 10; i++) {
      Paint_DrawLine(Xstart - i,  Ystart - i,  Xstart + i,  Ystart - i,  Color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }
  }
}


void oled_thread(void *argument) {
  (void) argument;
  osStatus_t status;
  oled_t msg = {0};
  uint32_t time_init_display = 0;
  bool update = false;
  //
  char cStr[28] = {0};

  // display initialization
  OLED_1in5_rgb_Init();
  osDelay(100);
  OLED_1in5_rgb_Clear();
  osDelay(100);
  OLED_1in5_rgb_Init();
  osDelay(100);
  OLED_1in5_rgb_Clear();
  
  // create image
  Paint_NewImage(BlackImage, OLED_1in5_RGB_WIDTH, OLED_1in5_RGB_HEIGHT, 0, BLACK);
  // set scale
  Paint_SetScale(65);
  // Select Image
  Paint_SelectImage(BlackImage);

#if CHECK_LOAD_SYSTEM == 1
  // draw image on the screen
  OLED_1in5_rgb_Display(BlackImage);
  
  osDelay(3000);
  
  Paint_ClearWindows(0, 0, OLED_1in5_RGB_WIDTH, OLED_1in5_RGB_HEIGHT, BLACK);
  
  Paint_DrawString_EN(10, 5, "IP адрес", &Courier12R, WHITE, BLACK);
  memset(cStr, 0, 22);
  sprintf(cStr, "%s", ip4addr_ntoa(&settings.usk_ip_addr));  
  Paint_DrawString_EN(10, 21, cStr, &Courier12R, WHITE, BLACK);
  
  Paint_DrawString_EN(10, 41, "Маска", &Courier12R, WHITE, BLACK);
  memset(cStr, 0, 22);
  sprintf(cStr, "%s", ip4addr_ntoa(&settings.usk_mask_addr));
  Paint_DrawString_EN(10, 57, cStr, &Courier12R, WHITE, BLACK);
  
  Paint_DrawString_EN(10, 77, "Шлюз", &Courier12R, WHITE, BLACK);
  memset(cStr, 0, 22);
  sprintf(cStr, "%s", ip4addr_ntoa(&settings.usk_gateway_addr));
  Paint_DrawString_EN(10, 93, cStr, &Courier12R, WHITE, BLACK);
  
  OLED_1in5_rgb_Display(BlackImage);
  
  osDelay(10000);
  osEventFlagsSet(load_system_flags, DISPLAY_LOAD_BIT);
  osEventFlagsWait(load_system_flags, MAIN_LOAD_BIT, osFlagsWaitAny|osFlagsNoClear, osWaitForever);
#endif

  Paint_ClearWindows(0, 0, OLED_1in5_RGB_WIDTH, OLED_1in5_RGB_HEIGHT, BLACK);

  memset(cStr, 0, 22);
  sprintf(cStr, "Версия %d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);
  Paint_DrawString_EN(0, 113, cStr, &Courier12R, RED, BLACK);
  // draw image on the screen
  OLED_1in5_rgb_Display(BlackImage);
  
  uint16_t valve_down_color = 0;
  uint16_t valve_up_color = 0;
  
  uint32_t tick = osKernelGetTickCount();
  
  while (1) {
#ifdef DEBUG
    WM_OLED = uxTaskGetStackHighWaterMark(NULL);
#endif
    tick += pdMS_TO_TICKS(100);
    
    // osWaitForever
    status = osMessageQueueGet(oled_msg_queue, &msg, NULL, 10U);    
    if (status == osOK) {
      update = true;
    }
    
    if (time_init_display < HAL_GetTick()) {
      OLED_1in5_rgb_Init();
      OLED_1in5_rgb_Clear();
      time_init_display = HAL_GetTick() + INIT_TIME;
      update = true;
    }
    
    if (update == true) {
      
      time_init_display = HAL_GetTick() + INIT_TIME;
      
      Paint_ClearWindows(0, 0, 94, 18, BLACK);
      // title
      Paint_DrawString_EN(4, 4, msg.title, &Courier12R, FONT_COLOR, BLACK);
      // sub title
      //Paint_DrawString_EN(0, 17, msg.sub_title, &Courier12R, FONT_COLOR, BLACK);
      
      if (msg.emer_state) {
        Paint_DrawRectangle(95, 3, 127, 18, RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawString_EN(96, 4, emer_stop, &Courier12R, FONT_COLOR, RED);
      } else {
        Paint_ClearWindows(94, 2, 128, 19, BLACK);
      }
      
      switch (msg.menu_id) {
        case 0: {
          // clear all
          Paint_ClearWindows(0, 20, 128, 128, BLACK);
          break;
        }
        // pressure, threshold min, threshold max
        case 1: {
          Paint_ClearWindows(0, 20, 128, 128, BLACK);
          //paint_pressure(0, &msg.value2, &msg.value1, mpa);
          Paint_DrawString_EN(4, 58, "УСТАВКА ПОРОГОВ", &Courier12R, FONT_COLOR, BLACK);
          break;
        }
        case 2: { // min
          
          // clear all
          Paint_ClearWindows(0, 20, 128, 128, BLACK);
          
          Paint_DrawString_EN(4, 20, main_line, &Courier12R, FONT_COLOR, BLACK);
          
          if (msg.pressure_sensor[MAINLINE].state == STATE_ONLINE) {
            Paint_DrawNum(44, 37, (double)msg.pressure_sensor[MAINLINE].value, &Courier16R, 1, BLACK, FONT_COLOR);
          } else {
            Paint_DrawString_EN(44, 37, "Н.Д.", &Courier16R, RED, BLACK);
          }

          Paint_DrawString_EN(4, 58, set_point_min, &Courier12R, FONT_COLOR, BLACK);
          
          valve_down_color = 0;
          // DOWN valve decrease pressure
          if (msg.valve.state == STATE_ONLINE) {
            if (msg.valve.value_out & (1 << (settings.valve2_io - 1))) {
              valve_down_color = GREEN;
            } else {
              valve_down_color = GRAY;
            }
          } else {
            valve_down_color = RED;
          }
          
          Paint_DrawRectangle(0, 112, 12, 124, valve_down_color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
          
          uint16_t set_color = 0;
          
          if (msg.save_state) {
            set_color = GREEN;
          } else {
            set_color = FONT_COLOR;
          }
          
          Paint_DrawNum(16, 112, (double)settings.sens2_min_val/100.0, &Courier12R, 1, BLACK, set_color);
          
          Paint_ClearWindows(44, 74, 84, 124, BLACK);
          if (msg.pressure_sensor[HYDRAULIC].state > STATE_OFFLINE) {
            uint16_t sensor_color = FONT_COLOR;
/*
            if (msg.pressure_sensor[HYDRAULIC].state == STATE_NORMAL) {
              sensor_color = FONT_COLOR;
              Paint_triangle_vertical(58, 79, RED, 0);              
              Paint_triangle_vertical(58, 119, RED, 1);
            } else if (msg.pressure_sensor[HYDRAULIC].state == STATE_BELOW_NORMAL) {
              Paint_triangle_vertical(58, 79, RED, 0); // down
              sensor_color = RED;
            } else if (msg.pressure_sensor[HYDRAULIC].state == STATE_ABOVE_NORMAL) {
              Paint_triangle_vertical(58, 119, RED, 1); // up
              sensor_color = RED;
            }
*/
            Paint_DrawNum(44, 91, (double)msg.pressure_sensor[HYDRAULIC].value, &Courier16R, 1, BLACK, sensor_color);
          } else {
            Paint_DrawString_EN(44, 91, "Н.Д.", &Courier16R, RED, BLACK);
          }
          
          oled_msg.pressure_sensor[HYDRAULIC].min_value = msg.pressure_sensor[HYDRAULIC].value;
          
          // UP valve increase pressure 
          valve_up_color = 0;
          if (msg.valve.state == STATE_ONLINE) {
            if (msg.valve.value_out & (1 << (settings.valve1_io - 1))) {
              valve_up_color = GREEN;
            } else {
              valve_up_color = GRAY;
            }
          } else {
            valve_up_color = RED;
          }
          Paint_DrawRectangle(0, 74, 12, 86, valve_up_color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
          
          //Paint_DrawNum(20, 74, (double)settings.sens2_max_val/100.0, &Courier12R, 0, BLACK, FONT_COLOR);
          
          break;
        }
        case 3: { // max
          //paint_pressure(2, &msg.value2, &msg.value1, mpa);

          // clear all
          Paint_ClearWindows(0, 20, 128, 128, BLACK);
          
          Paint_DrawString_EN(4, 20, main_line, &Courier12R, FONT_COLOR, BLACK);
          
          if (msg.pressure_sensor[MAINLINE].state == STATE_ONLINE) {
            Paint_DrawNum(44, 37, (double)msg.pressure_sensor[MAINLINE].value, &Courier16R, 1, BLACK, FONT_COLOR);
          } else {
            Paint_DrawString_EN(44, 37, "Н.Д.", &Courier16R, RED, BLACK);
          }

          Paint_DrawString_EN(4, 58, set_point_max, &Courier12R, FONT_COLOR, BLACK);
          
          valve_down_color = 0;
          // DOWN valve decrease pressure
          if (msg.valve.state == STATE_ONLINE) {
            if (msg.valve.value_out & (1 << (settings.valve2_io - 1))) {
              valve_down_color = GREEN;
            } else {
              valve_down_color = GRAY;
            }
          } else {
            valve_down_color = RED;
          }
          
          Paint_DrawRectangle(0, 112, 12, 124, valve_down_color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
          
          //Paint_DrawNum(20, 112, (double)settings.sens2_min_val/100.0, &Courier12R, 0, BLACK, FONT_COLOR);
          
          Paint_ClearWindows(44, 74, 84, 124, BLACK);
          if (msg.pressure_sensor[HYDRAULIC].state > STATE_OFFLINE) {
            uint16_t sensor_color = FONT_COLOR;
/*
            if (msg.pressure_sensor[HYDRAULIC].state == STATE_NORMAL) {
              sensor_color = FONT_COLOR;
              Paint_triangle_vertical(58, 79, RED, 0);              
              Paint_triangle_vertical(58, 119, RED, 1);
            } else if (msg.pressure_sensor[HYDRAULIC].state == STATE_BELOW_NORMAL) {
              Paint_triangle_vertical(58, 79, RED, 0); // down
              sensor_color = RED;
            } else if (msg.pressure_sensor[HYDRAULIC].state == STATE_ABOVE_NORMAL) {
              Paint_triangle_vertical(58, 119, RED, 1); // up
              sensor_color = RED;
            }
*/
            Paint_DrawNum(44, 91, (double)msg.pressure_sensor[HYDRAULIC].value, &Courier16R, 1, BLACK, sensor_color);
          } else {
            Paint_DrawString_EN(44, 91, "Н.Д.", &Courier16R, RED, BLACK);
          }
          
          oled_msg.pressure_sensor[HYDRAULIC].max_value = msg.pressure_sensor[HYDRAULIC].value;
          
          // UP valve increase pressure 
          valve_up_color = 0;
          if (msg.valve.state == STATE_ONLINE) {
            if (msg.valve.value_out & (1 << (settings.valve1_io - 1))) {
              valve_up_color = GREEN;
            } else {
              valve_up_color = GRAY;
            }
          } else {
            valve_up_color = RED;
          }
          Paint_DrawRectangle(0, 74, 12, 86, valve_up_color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
          
          uint16_t set_color = 0;
          
          if (msg.save_state) {
            set_color = GREEN;
          } else {
            set_color = FONT_COLOR;
          }
          
          Paint_DrawNum(20, 74, (double)settings.sens2_max_val/100.0, &Courier12R, 1, BLACK, set_color);
          
          break;
          
        }
        // position, threshold min, threshold max
        case 4: {
          paint_pressure(0, &msg.value2, &msg.value1, mm);
          break;
        }
        case 5: {
          paint_pressure(1, &msg.value2, &msg.value1, mm);
          break;
        }
        case 6: {
          paint_pressure(2, &msg.value2, &msg.value1, mm);
          break;
        }
        // pressure set point
        case 7: {
          // clear all
          Paint_ClearWindows(0, 35, 128, 105, BLACK);
          // value 1
          Paint_DrawNum(10, 35, (double)msg.value1, &Font20, 1, BLACK, FONT_COLOR);
          Paint_DrawString_EN(80, 35, mpa, &Font20, FONT_COLOR, BLACK);
          // value 2
          Paint_DrawNum(10, 60, (double)msg.pressure_sensor[MAINLINE].value, &Font20, 1, BLACK, FONT_COLOR);
          Paint_DrawString_EN(80, 60, mpa, &Font20, FONT_COLOR, BLACK);
          // value 3
          Paint_DrawNum(10, 85, msg.position, &Font20, 0, BLACK, FONT_COLOR);
          Paint_DrawString_EN(80, 85, mm, &Font20, FONT_COLOR, BLACK);
          break;
        }
        case 8: {
          // clear all
          Paint_ClearWindows(0, 35, 128, 105, BLACK);
          // selector
          Paint_DrawCircle(3, 43, 3, RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);
          // value 1
          Paint_DrawNum(10, 35, (double)msg.value1, &Font20, 1, BLACK, FONT_COLOR);
          Paint_DrawString_EN(80, 35, mpa, &Font20, FONT_COLOR, BLACK);
          // value 2
          Paint_DrawNum(10, 60, (double)msg.pressure_sensor[MAINLINE].value, &Font20, 1, BLACK, FONT_COLOR);
          Paint_DrawString_EN(80, 60, mpa, &Font20, FONT_COLOR, BLACK);
          // value 3
          Paint_DrawNum(10, 85, msg.position, &Font20, 0, BLACK, FONT_COLOR);
          Paint_DrawString_EN(80, 85, mm, &Font20, FONT_COLOR, BLACK);
          break;
        }
        // URM info
        case 9: {
          // clear all
          Paint_ClearWindows(0, 35, 128, 105, BLACK);
          // value 1
          Paint_DrawString_EN(0, 35, "Адрес", &Courier12R, FONT_COLOR, BLACK);
          Paint_DrawNum(105, 35, (double)msg.value1, &Courier12R, 0, BLACK, FONT_COLOR);
          // value 2
          Paint_DrawString_EN(0, 52, "Вых Клапан+", &Courier12R, FONT_COLOR, BLACK);
          Paint_DrawNum(105, 52, (double)msg.value2, &Courier12R, 0, BLACK, FONT_COLOR);
          // value 3
          Paint_DrawString_EN(0, 69, "Вых Клапан-", &Courier12R, FONT_COLOR, BLACK);
          Paint_DrawNum(105, 69, msg.value3, &Courier12R, 0, BLACK, FONT_COLOR);
          // value 4
          Paint_DrawString_EN(0, 86, "Вых Состояние", &Courier12R, FONT_COLOR, BLACK);
          Paint_DrawNum(105, 86, msg.value4, &Courier12R, 0, BLACK, FONT_COLOR);
          break;
        }
        // UMVH info
        case 10: {
          // clear all
          Paint_ClearWindows(0, 35, 128, 105, BLACK);
          // value 1
          Paint_DrawString_EN(0, 35, "Адрес", &Courier12R, FONT_COLOR, BLACK);
          Paint_DrawNum(105, 35, (double)msg.value1, &Courier12R, 0, BLACK, FONT_COLOR);
          // value 2
          Paint_DrawString_EN(0, 52, "Вх ДЛП", &Courier12R, FONT_COLOR, BLACK);
          Paint_DrawNum(105, 52, (double)msg.value2, &Courier12R, 0, BLACK, FONT_COLOR);
          // value 3
          Paint_DrawString_EN(0, 69, "Вх Упрв", &Courier12R, FONT_COLOR, BLACK);
          Paint_DrawNum(105, 69, msg.value3, &Courier12R, 0, BLACK, FONT_COLOR);
          break;
        }
        // position zero offset
        case 11 :
        case 12 : {
          // clear all
          Paint_ClearWindows(0, 35, 128, 105, BLACK);
          // value 1 <-> position zero offset
          if (msg.value5 >= 0) {
            Paint_DrawString_EN(0, 35, "+", &Font20, FONT_COLOR, BLACK);
          } else {
            Paint_DrawString_EN(0, 35, "-", &Font20, FONT_COLOR, BLACK);
          }
          Paint_DrawNum(14, 35, abs(msg.value5), &Font20, 0, BLACK, FONT_COLOR);
          Paint_DrawString_EN(80, 35, mm, &Font20, FONT_COLOR, BLACK);
          // value 2 <-> current position value
          Paint_DrawNum(0, 60, msg.position, &Font20, 0, BLACK, FONT_COLOR);
          Paint_DrawString_EN(80, 60, mm, &Font20, FONT_COLOR, BLACK);
          break;
        }
        case 13 : {
          // clear all
          Paint_ClearWindows(0, 20, 128, 128, BLACK);
          
          Paint_DrawString_EN(4, 20, main_line, &Courier12R, FONT_COLOR, BLACK);
          
          if (msg.pressure_sensor[MAINLINE].state == STATE_ONLINE) {
            Paint_DrawNum(44, 37, (double)msg.pressure_sensor[MAINLINE].value, &Courier16R, 1, BLACK, FONT_COLOR);
          } else {
            Paint_DrawString_EN(44, 37, "Н.Д.", &Courier16R, RED, BLACK);
          }

          Paint_DrawString_EN(4, 58, hydraulic_line, &Courier12R, FONT_COLOR, BLACK);
          
          valve_down_color = 0;
          // DOWN valve decrease pressure
          if (msg.valve.state == STATE_ONLINE) {
            if (msg.valve.value_out & (1 << (settings.valve2_io - 1))) {
              valve_down_color = GREEN;
            } else {
              valve_down_color = GRAY;
            }
          } else {
            valve_down_color = RED;
          }
          
          Paint_DrawRectangle(0, 112, 12, 124, valve_down_color, DOT_PIXEL_1X1, DRAW_FILL_FULL);

          Paint_DrawNum(16, 112, (double)settings.sens2_min_val/100.0, &Courier12R, 1, BLACK, FONT_COLOR);

          Paint_ClearWindows(44, 74, 84, 124, BLACK);
          if (msg.pressure_sensor[HYDRAULIC].state > STATE_OFFLINE) {
            uint16_t sensor_color = FONT_COLOR;
            if (msg.pressure_sensor[HYDRAULIC].state == STATE_NORMAL) {
              sensor_color = FONT_COLOR;
              Paint_triangle_vertical(58, 79, RED, 0);              
              Paint_triangle_vertical(58, 119, RED, 1);
            } else if (msg.pressure_sensor[HYDRAULIC].state == STATE_BELOW_NORMAL) {
              Paint_triangle_vertical(58, 79, RED, 0); // down
              sensor_color = RED;
            } else if (msg.pressure_sensor[HYDRAULIC].state == STATE_ABOVE_NORMAL) {
              Paint_triangle_vertical(58, 119, RED, 1); // up
              sensor_color = RED;
            }
            Paint_DrawNum(44, 91, (double)msg.pressure_sensor[HYDRAULIC].value, &Courier16R, 1, BLACK, sensor_color);
          } else {
            Paint_DrawString_EN(44, 91, "Н.Д.", &Courier16R, RED, BLACK);
          }

          // UP valve increase pressure 
          valve_up_color = 0;
          if (msg.valve.state == STATE_ONLINE) {
            if (msg.valve.value_out & (1 << (settings.valve1_io - 1))) {
              valve_up_color = GREEN;
            } else {
              valve_up_color = GRAY;
            }
          } else {
            valve_up_color = RED;
          }
          Paint_DrawRectangle(0, 74, 12, 86, valve_up_color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
          
          Paint_DrawNum(16, 74, (double)settings.sens2_max_val/100.0, &Courier12R, 1, BLACK, FONT_COLOR);
          
          break;
        }
        case 14: {
          // clear all
          Paint_ClearWindows(0, 35, 128, 105, BLACK);
          
          if (msg.system_control_mode == LOCAL_SYS_CONTROL) {
            Paint_DrawString_EN(29, 62, "МЕСТНЫЙ", &Courier16R, FONT_COLOR, BLACK);
          } else {
            Paint_DrawString_EN(44, 62, "АВТО", &Courier16R, FONT_COLOR, BLACK);
          }
          break;
        }
        case 21: {
          // clear all
          Paint_ClearWindows(0, 35, 128, 105, BLACK);
          
          if (msg.value1 == LOCAL_SYS_CONTROL) {
            Paint_DrawString_EN(29, 62, "МЕСТНЫЙ", &Courier16R, FONT_COLOR, BLACK);
          } else {
            Paint_DrawString_EN(44, 62, "АВТО", &Courier16R, FONT_COLOR, BLACK);
          }
          break;
        }
        case 66 : {
          // clear all
          Paint_ClearWindows(0, 35, 128, 105, BLACK);
          Paint_DrawString_EN(14, 45, "Сохранить?", &Courier16R, FONT_COLOR, BLACK);          
          Paint_DrawString_EN(14, 66, "Да", &Courier16R, FONT_COLOR, BLACK);
          Paint_DrawString_EN(83, 66, "Нет", &Courier16R, FONT_COLOR, BLACK);
          break;
        }
        case 70 : {
          paint_pin(0, &msg.pin);
          break;
        }
        case 71 : {
          paint_pin(1, &msg.pin);
          break;
        }
        case 72 : {
          paint_pin(2, &msg.pin);
          break;
        }
        default: {
          break;
        }
      }
      // draw image on the screen
      OLED_1in5_rgb_Display(BlackImage);
      
      update = false;
    }
    osDelayUntil(tick);
  }  
}
