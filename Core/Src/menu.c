#include "menu.h"
#include "flash_if.h"
#include "global_types.h"
#include "manage_settings.h"
#include "crc.h"

pin_t default_pin = {
  .pin0 = 3,
  .pin1 = 3,
  .pin2 = 3,
  .pin4 = 0
};

Menu_Item_t NULL_MENU = {0};

Menu_Item_t* CurrentMenuItem = &NULL_MENU;

void Menu_SelectCurrentItem(void);
void Menu_EnterCurrentItem(uint8_t dir);
void Menu_Navigate(Menu_Item_t* const NewMenu);

uint8_t save_state = 0;
extern osTimerId_t timer_delay_save;


//static void Menu_1_1_Enter(uint8_t dir);
//static void Menu_1_2_Enter(uint8_t dir);
static void M_1_4_Select(void);
//static void M_2_4_Select(void);
//static void M_2_4_Select(void);
//static void M_3_3_Select(void);

static void M_16_Select(void);

//static void Menu_6_1_Enter(uint8_t dir);
//static void M_6_2_Select(void);

static void PIN_Enter(uint8_t dir);

static void Menu_7_1_Enter(uint8_t dir);
//static void Menu_8_1_Enter(uint8_t dir);
//static void M_8_1_Select(void);

/*---------------*/
#define M01 1

#define M02 2
#define M15 15

#define M03 3
#define M16 16
/*---------------*/
#define M04 4

#define M05 5
#define M17 17

#define M06 6
#define M18 18
/*---------------*/
#define M07 7

#define M08 8
#define M19 19
/*---------------*/
#define M09 9
/*---------------*/
#define M10 10
/*---------------*/
#define M11 11

#define M12 12
#define M20 20
/*---------------*/
#define M13 13
/*---------------*/
#define M14 14

#define M21 21
/*---------------*/

/*--------------------------------------------------------------------------------------------------------------------*/
/*        Name      Next       Previous   Parent     Child      SelectFunc    EnterFunc       Text                 id */
MENU_ITEM(Menu_10,  Menu_11,   Menu_12,   Menu_1_1,  NULL_MENU,  NULL,         PIN_Enter,      "PIN",               70);
MENU_ITEM(Menu_11,  Menu_12,   Menu_10,   Menu_1_1,  NULL_MENU,  NULL,         PIN_Enter,      "PIN",               71);
MENU_ITEM(Menu_12,  Menu_10,   Menu_11,   Menu_1_1,  NULL_MENU,  NULL,         PIN_Enter,      "PIN",               72);

/*--------------------------------------------------------------------------------------------------------------------*/
/*        Name      Next       Previous   Parent     Child      SelectFunc    EnterFunc       Text                 id */
//MENU_ITEM(Menu_1,   Menu_2,    Menu_7,    Menu_7,    Menu_10,   NULL,         NULL,           "НАСТРОЙКА",         M01);
MENU_ITEM(Menu_1,   Menu_4,    Menu_5,    Menu_7,    Menu_10,   NULL,         NULL,           "НАСТРОЙКА",         M01);
/*--------------------------------------------------------------------------------------------------------------------*/
// pin
MENU_ITEM(Menu_16,  NULL_MENU, NULL_MENU, Menu_1,    Menu_1_1,  M_16_Select,  NULL,           "",                  M17);

/*        Name      Next       Previous   Parent     Child      SelectFunc    EnterFunc       Text                 id */
MENU_ITEM(Menu_1_1, Menu_1_2,  Menu_1_2,  Menu_1,    Me_1_1_1,  NULL,         Menu_7_1_Enter, "Порог низ",         M02);
//MENU_ITEM(Me_1_1_1, NULL_MENU, NULL_MENU, Menu_1_1,  Menu_10,   NULL,         NULL,           "",                  66);
// pin
//MENU_ITEM(Me_1_1_2, NULL_MENU, NULL_MENU, Menu_1_1,  NULL_MENU, M_1_4_Select, NULL,           "Сохранить",         M15);
MENU_ITEM(Me_1_1_1, NULL_MENU, NULL_MENU, Menu_1_1,  NULL_MENU, M_1_4_Select, NULL,           "Сохранить",         M15);
/*--------------------------------------------------------------------------------------------------------------------*/
/*        Name      Next       Previous   Parent     Child      SelectFunc    EnterFunc       Text                 id */
MENU_ITEM(Menu_1_2, Menu_1_1,  Menu_1_1,  Menu_1,    Me_1_2_1,  NULL,         Menu_7_1_Enter, "Порог верх",        M03);
//MENU_ITEM(Me_1_2_1, NULL_MENU, NULL_MENU, Menu_1_2,  Menu_10,   NULL,         NULL,           "",                  66);
// pin
//MENU_ITEM(Me_1_2_2, NULL_MENU, NULL_MENU, Menu_1_2,  NULL_MENU, M_1_4_Select, NULL,           "Сохранить",         M16);
MENU_ITEM(Me_1_2_1, NULL_MENU, NULL_MENU, Menu_1_2,  NULL_MENU, M_1_4_Select, NULL,           "Сохранить",         M16);
/*--------------------------------------------------------------------------------------------------------------------*/
/*        Name      Next       Previous   Parent     Child      SelectFunc    EnterFunc       Text                 id */
//MENU_ITEM(Menu_2,   Menu_6,    Menu_1,    Menu_7,    Menu_2_2,  NULL,         NULL,           "Перемещение Настр", M04);
/*--------------------------------------------------------------------------------------------------------------------*/
/*        Name      Next       Previous   Parent     Child      SelectFunc    EnterFunc       Text                 id */
//MENU_ITEM(Menu_2_1, Menu_2_2,  Menu_2_2,  Menu_2,    Me_2_1_1,  NULL,         Menu_1_1_Enter, "Порог низ",         M05);
//MENU_ITEM(Me_2_1_1, NULL_MENU, NULL_MENU, Menu_2_1,  Menu_10,   NULL,         NULL,           "",                  66);
// pin
//MENU_ITEM(Me_2_1_2, NULL_MENU, NULL_MENU, Menu_2_1,  NULL_MENU, M_2_4_Select, NULL,           "Сохранить",         M17);
/*--------------------------------------------------------------------------------------------------------------------*/
/*        Name      Next       Previous   Parent     Child      SelectFunc    EnterFunc       Text                 id */
//MENU_ITEM(Menu_2_2, Menu_2_1,  Menu_2_1,  Menu_2,    Me_2_2_1,  NULL,         Menu_1_2_Enter, "Порог верх",        M06);
//MENU_ITEM(Me_2_2_1, NULL_MENU, NULL_MENU, Menu_2_2,  Menu_10,   NULL,         NULL,           "",                  66);
// pin
//MENU_ITEM(Me_2_2_2, NULL_MENU, NULL_MENU, Menu_2_2,  NULL_MENU, M_2_4_Select, NULL,           "Сохранить",         M18);
/*--------------------------------------------------------------------------------------------------------------------*/
/*        Name      Next       Previous   Parent     Child      SelectFunc    EnterFunc       Text                 id */
//MENU_ITEM(Menu_3,   Menu_8,    Menu_8,    Menu_7,    Menu_3_1,  NULL,         NULL,           "Давление Уставка",  M07);

//MENU_ITEM(Menu_3_1, NULL_MENU, NULL_MENU, Menu_3,    Me_3_1_1,  NULL,         Menu_1_1_Enter, "Уставка",           M08);
//MENU_ITEM(Me_3_1_1, NULL_MENU, NULL_MENU, Menu_3_1,  Menu_10,   NULL,         NULL,           "",                  66);
// pin
//MENU_ITEM(Me_3_1_2, NULL_MENU, NULL_MENU, Menu_3,    NULL_MENU, M_3_3_Select, NULL,           "Сохранить",         0);
/*--------------------------------------------------------------------------------------------------------------------*/
/*        Name      Next       Previous   Parent     Child      SelectFunc    EnterFunc       Text                 id */
//MENU_ITEM(Menu_4,   Menu_5,    Menu_6,    Menu_7,    NULL_MENU, NULL,         NULL,           "УРМ",               M09);
MENU_ITEM(Menu_4,   Menu_5,    Menu_1,    Menu_7,    NULL_MENU, NULL,         NULL,           "УРМ",               M09);
/*--------------------------------------------------------------------------------------------------------------------*/
/*        Name      Next       Previous   Parent     Child      SelectFunc    EnterFunc       Text                 id */
MENU_ITEM(Menu_5,   Menu_7,    Menu_4,    Menu_7,    NULL_MENU, NULL,         NULL,           "УМВХ",              M10);
/*--------------------------------------------------------------------------------------------------------------------*/
/*        Name      Next       Previous   Parent     Child      SelectFunc    EnterFunc       Text                 id */
//MENU_ITEM(Menu_6,   Menu_4,    Menu_2,    Menu_7,    Menu_6_1,  NULL,         NULL,           "Калибровка ДЛП",    M11);

//MENU_ITEM(Menu_6_1, NULL_MENU, NULL_MENU, Menu_6,    Me_6_1_1,  NULL,         Menu_6_1_Enter, "Установка нуля",    M12);
//MENU_ITEM(Me_6_1_1, NULL_MENU, NULL_MENU, Menu_6_1,  Menu_10,   NULL,         NULL,           "",                  66);
// pin
//MENU_ITEM(Me_6_1_2, NULL_MENU, NULL_MENU, Menu_6_1,  NULL_MENU, M_6_2_Select, NULL,           "Сохранить",         0);
/*--------------------------------------------------------------------------------------------------------------------*/
/*        Name      Next       Previous   Parent     Child      SelectFunc    EnterFunc       Text                 id */
//MENU_ITEM(Menu_7,   Menu_1,    Menu_5,    NULL_MENU, Menu_3,    NULL,         Menu_7_1_Enter, "МЕСТНЫЙ",           M13);
MENU_ITEM(Menu_7,   Menu_1,    Menu_5,    NULL_MENU, NULL_MENU,    NULL,         Menu_7_1_Enter, "МЕСТНЫЙ",           M13);
/*--------------------------------------------------------------------------------------------------------------------*/
/*        Name      Next       Previous   Parent     Child      SelectFunc    EnterFunc       Text                 id */
//MENU_ITEM(Menu_8,   Menu_3,    Menu_3,    Menu_7,    Menu_8_1,  NULL,         NULL,           "Управление",        M14);
//MENU_ITEM(Menu_8_1, NULL_MENU, NULL_MENU, Menu_8,    Me_8_1_1,  NULL,         Menu_8_1_Enter, "Режим",             M21);
//MENU_ITEM(Me_8_1_1, NULL_MENU, NULL_MENU, Menu_8_1,  Menu_10,   NULL,         NULL,           "",                  66);
// pin
//MENU_ITEM(Me_8_1_2, NULL_MENU, NULL_MENU, Menu_8,    NULL_MENU, M_8_1_Select, NULL,           "Сохранить",         0);
/*--------------------------------------------------------------------------------------------------------------------*/

extern valve_control_t local_valve_control;

extern osSemaphoreId_t valve_controlSemaphore;

extern osSemaphoreId_t flash_semaphore;

static void Menu_7_1_Enter(uint8_t dir) {
  if (oled_msg.system_control_mode == LOCAL_SYS_CONTROL) {
    if (osSemaphoreAcquire(valve_controlSemaphore, 2) == osOK) {
      local_valve_control.tick = osKernelGetTickCount() + 500;
      if (dir) {
        local_valve_control.state = VALVE_UP;
      } else {
        local_valve_control.state = VALVE_DOWN;
      }
      osSemaphoreRelease(valve_controlSemaphore);
    }
  }
}
/*
static void Menu_8_1_Enter(uint8_t dir) {
  oled_msg.value1 = (oled_msg.value1 == AUTO_SYS_CONTROL) ? LOCAL_SYS_CONTROL : AUTO_SYS_CONTROL;
}
*/

/*
static void M_8_1_Select(void) {
  if (oled_msg.pin.value == default_pin.value) {
    oled_msg.system_control_mode = (uint8_t)oled_msg.value1;
  } else {
    oled_msg.value1 = oled_msg.system_control_mode;
  }
  Menu_Navigate(CurrentMenuItem->Parent);
}
*/

void Menu_SelectCurrentItem(void) {
  if ((CurrentMenuItem == &NULL_MENU) || (CurrentMenuItem == NULL)) {
    return;
  }  
  SelectCallback_t func = CurrentMenuItem->SelectCallback;  
  if (func != NULL) {
    func();
  }
}

void Menu_EnterCurrentItem(uint8_t dir) {
  if ((CurrentMenuItem == &NULL_MENU) || (CurrentMenuItem == NULL)) {
    return;
  }  
  EnterCallback_t func = CurrentMenuItem->EnterCallback;  
  if (func != NULL) {
    func(dir);
  }
}

void replace_menu(Menu_Item_t* child, Menu_Item_t* parent) {
  // pin 0
  Menu_10.Child = child;
  Menu_10.Parent = parent;
  // pin 1
  Menu_11.Child = child;
  Menu_11.Parent = parent;
  // pin 2
  Menu_12.Child = child;
  Menu_12.Parent = parent;
}

void Menu_Navigate(Menu_Item_t* const NewMenu) {
  if ((NewMenu == &NULL_MENU) || (NewMenu == NULL)) {
    return;
  }  
  CurrentMenuItem = NewMenu;  
  if (CurrentMenuItem->Parent == &NULL_MENU) {
    oled_msg.title = NewMenu->Text;
    if (CurrentMenuItem->id == M09 || CurrentMenuItem->id == M10) {
      oled_msg.sub_title = "Информация";
    } else {
      oled_msg.sub_title = " ";
    }
  } else {
    oled_msg.title = NewMenu->Parent->Text;
    oled_msg.sub_title = NewMenu->Text;
  }  
  oled_msg.menu_id = CurrentMenuItem->id;  
  switch (CurrentMenuItem->id) {
    case M01: { // pressure min && max
      //oled_msg.value1 = settings.sens2_min_val / 100.0f;
      //oled_msg.value2 = settings.sens2_max_val / 100.0f;

      oled_msg.pin.value = 0;
      replace_menu(&Menu_16, &Menu_1);
      
      break;
    }
    case M02: { // pressure min
      
      //oled_msg.value1 = settings.sens2_min_val / 100.0f;
      //oled_msg.value2 = settings.sens2_max_val / 100.0f;
      break;
    }
    case M03: { // pressure max
     
      //oled_msg.value1 = settings.sens2_min_val / 100.0f;
      //oled_msg.value2 = settings.sens2_max_val / 100.0f;
      break;
    }
    case M04: { // position min max
      //oled_msg.value1 = settings.sens3_min_val;
      //oled_msg.value2 = settings.sens3_max_val;
      break;
    }
    case M05: { // position min max
      //oled_msg.value1 = settings.sens3_min_val;
      //oled_msg.value2 = settings.sens3_max_val;
      break;
    }
    case M06: { // position min max
      //oled_msg.value1 = settings.sens3_min_val;
      //oled_msg.value2 = settings.sens3_max_val;
      break;
    }    
    case M07: { // pressure set point
      //oled_msg.value1 = settings.pressure_set_point / 100.0f;
      break;
    }
    case M09: { // urm info
      oled_msg.value1 = settings.urm_id;
      oled_msg.value2 = settings.valve1_io;
      oled_msg.value3 = settings.valve2_io;
      oled_msg.value4 = settings.state_out_io;
      break;
    }
    case M10: { // umvh info
      oled_msg.value1 = settings.umvh_id;
      oled_msg.value2 = settings.sens3_io;
      oled_msg.value3 = settings.state_in_io;
      break;
    }
    case M11: { // position zero offset
      //oled_msg.value5 = global_tuning.zero_offset;
      break;
    }
    case M14: { //
      //oled_msg.value1 = oled_msg.system_control_mode;
      break;
    }
    case 66: { // save
      oled_msg.pin.value = 0;
      
      switch (CurrentMenuItem->Parent->id) {
        case M02: {
          replace_menu(&Me_1_1_1, &Menu_1_1);
          break;
        }
        case M03: {
          replace_menu(&Me_1_2_1, &Menu_1_2);
          break;
        }
        case M05: {
          //replace_menu(&Me_2_1_2, &Menu_2_1);
          break;
        }
        case M06: {
          //replace_menu(&Me_2_2_2, &Menu_2_2);
          break;
        }
        case M08: {
          //replace_menu(&Me_3_1_2, &Menu_3_1);
          break;
        }
        case M12: {
          //replace_menu(&Me_6_1_2, &Menu_6_1);
          break;
        }
        case M21: {
          //replace_menu(&Me_8_1_2, &Menu_8_1);
          break;
        }
        default: {
          break;
        }
      }
/*
      if (CurrentMenuItem->Parent->id == M02) {
        replace_menu(&Me_1_1_2, &Menu_1_1);
      } else if (CurrentMenuItem->Parent->id == M03) {
        replace_menu(&Me_1_2_2, &Menu_1_2);
      } else if (CurrentMenuItem->Parent->id == M05) {
        //replace_menu(&Me_2_1_2, &Menu_2_1);
      } else if (CurrentMenuItem->Parent->id == M06) {
        //replace_menu(&Me_2_2_2, &Menu_2_2);
      } else if (CurrentMenuItem->Parent->id == M08) {
        //replace_menu(&Me_3_1_2, &Menu_3_1);
      } else if (CurrentMenuItem->Parent->id == M12) {
        //replace_menu(&Me_6_1_2, &Menu_6_1);
      } else if (CurrentMenuItem->Parent->id == M21) {
        replace_menu(&Me_8_1_2, &Menu_8_1);
      }
*/
      break;
    }
    default: {
      break;
    }
  }

  SelectCallback_t func = CurrentMenuItem->SelectCallback;
  if (func != NULL) {
    func();
  }
}

/*
static void Menu_1_1_Enter(uint8_t dir) {
  float max_threshold = 0;
  float increment = 0;
  if (CurrentMenuItem->id == M02 || CurrentMenuItem->id == M08) {
    //m = 99;
    max_threshold = (settings.sens2_max_val < 9900) ? settings.sens2_max_val : 9900;
    max_threshold /= 100.0f;
    increment = 0.1;
  }  
  if (CurrentMenuItem->id == M05) {
    //m = 9999;
    max_threshold = (settings.sens3_max_val < 9999) ? settings.sens3_max_val : 9999;
    increment = 1;
  }  
  if (dir) {
    if (oled_msg.value1 < max_threshold) {
      oled_msg.value1 += increment;
    }
  } else {
    if (oled_msg.value1 > 0) {
      oled_msg.value1 -= increment;
    }
  }
}
*/

/*
static void Menu_1_2_Enter(uint8_t dir) {
  uint16_t m = 0;
  float min_threshold = 0;
  float increment = 0;
  if (CurrentMenuItem->id == M03) {
    m = 99;
    min_threshold = (settings.sens2_min_val > 0) ? settings.sens2_min_val : 0;
    min_threshold /= 100.0f;
    increment = 0.1;
  }  
  if (CurrentMenuItem->id == M06) {
    m = 9999;
    min_threshold = (settings.sens3_min_val > 0) ? settings.sens3_min_val : 0;
    increment = 1;
  }
  if (dir) {
    if (oled_msg.value2 < m) {
      oled_msg.value2 += increment;
    }
  } else {
    if (oled_msg.value2 > min_threshold) {
      oled_msg.value2 -= increment;
    }
  }
}
*/

static void M_1_4_Select(void) {
  if (oled_msg.pin.value == default_pin.value) {
    if (CurrentMenuItem->id == M15) {
      uint16_t current_val = (uint16_t)(oled_msg.pressure_sensor[HYDRAULIC].min_value * 100.0f);
      if (current_val != settings.sens2_min_val && current_val < settings.sens2_max_val) {
        if (osSemaphoreAcquire(flash_semaphore, 5) == osOK) {
          if (save_state == 0 && !osTimerIsRunning(timer_delay_save)) {
            save_state = 1;
            oled_msg.save_state = 1;
            osTimerStart(timer_delay_save, 5000);
          }  
          settings.sens2_min_val = current_val;
#if WFLASH == 1
          //FlashWORD(CONFIGURATION_ADDR, (uint32_t *)&settings, sizeof(settings_t));
          settings.crc32 = hw_crc32((uint8_t *)&settings, sizeof(settings_t) - 4);
          Flash_WriteStruct(&settings, CONFIGURATION_SECTOR, sizeof(settings_t), CONFIGURATION_SECTOR_SIZE);
#endif
          osSemaphoreRelease(flash_semaphore);
        }
      }
    }
    else if (CurrentMenuItem->id == M16) {
      uint16_t current_val = (uint16_t)(oled_msg.pressure_sensor[HYDRAULIC].max_value * 100.0f);
      if (current_val != settings.sens2_max_val && current_val > settings.sens2_min_val) {
        if (osSemaphoreAcquire(flash_semaphore, 5) == osOK) {
          if (save_state == 0 && !osTimerIsRunning(timer_delay_save)) {
            save_state = 1;
            oled_msg.save_state = 1;
            osTimerStart(timer_delay_save, 5000);
          }
          settings.sens2_max_val = current_val;
#if WFLASH == 1
          //FlashWORD(CONFIGURATION_ADDR, (uint32_t *)&settings, sizeof(settings_t));
          settings.crc32 = hw_crc32((uint8_t *)&settings, sizeof(settings_t) - 4);
          Flash_WriteStruct(&settings, CONFIGURATION_SECTOR, sizeof(settings_t), CONFIGURATION_SECTOR_SIZE);
#endif
          osSemaphoreRelease(flash_semaphore);
        }
      }
    }
    //oled_msg.save_state = 1;
  } else {
    //if (CurrentMenuItem->id == M15) {
    //  oled_msg.value1 = settings.sens2_min_val / 100.0f;
    //}
    //if (CurrentMenuItem->id == M16) {
    //  oled_msg.value2 = settings.sens2_max_val / 100.0f;
    //}
    oled_msg.save_state = 0;
  }
  Menu_Navigate(CurrentMenuItem->Parent);
}

void delay_save_callback(void *argument) {
  save_state = 0;
  oled_msg.save_state = 0;
}

static void M_16_Select(void) {
  if (oled_msg.pin.value == default_pin.value) {
    Menu_Navigate(CurrentMenuItem->Child);
  } else {
    Menu_Navigate(CurrentMenuItem->Parent);
  }
}

/*
static void M_2_4_Select(void) {
  if (oled_msg.pin.value == default_pin.value) {
    if (CurrentMenuItem->id == M17 && settings.sens3_min_val != oled_msg.value1) {
      settings.sens3_min_val = (uint16_t)oled_msg.value1;
#if WFLASH == 1
      FlashWORD(CONFIGURATION_ADDR, (uint32_t *)&settings, sizeof(settings_t));
#endif
    }
    if (CurrentMenuItem->id == M18 && settings.sens3_max_val != oled_msg.value2) {
      settings.sens3_max_val = (uint16_t)oled_msg.value2;
#if WFLASH == 1
      FlashWORD(CONFIGURATION_ADDR, (uint32_t *)&settings, sizeof(settings_t));
#endif
    }
  } else {
    if (CurrentMenuItem->id == M17) {
      oled_msg.value1 = settings.sens3_min_val;
    }
    if (CurrentMenuItem->id == M18) {
      oled_msg.value2 = settings.sens3_max_val;
    }
  }
  Menu_Navigate(CurrentMenuItem->Parent);
}
*/

/*
static void M_3_3_Select(void) {
  if (oled_msg.pin.value == default_pin.value) {
    if (settings.pressure_set_point != oled_msg.value1 * 100.0f) {
      settings.pressure_set_point = (uint16_t)(oled_msg.value1 * 100.0f);
#if WFLASH == 1
      FlashWORD(CONFIGURATION_ADDR, (uint32_t *)&settings, sizeof(settings_t));
#endif
    }
  } else {
    oled_msg.value1 = settings.pressure_set_point / 100.0f;
  }
  Menu_Navigate(CurrentMenuItem->Parent);
}
*/

/*
static void Menu_6_1_Enter(uint8_t dir) {
  if (dir) {
    oled_msg.value5++;
  } else {
    oled_msg.value5--;
  }
}
*/

/*
static void M_6_2_Select(void) {
  if (oled_msg.pin.value == default_pin.value) {
    if (global_tuning.zero_offset != oled_msg.value5) {
      global_tuning.zero_offset = oled_msg.value5;
#if WFLASH == 1
      FlashWORD(STATE_ADDR, (uint32_t *)&global_tuning, sizeof(tuning_t));
#endif
    }
  } else {
    oled_msg.value5 = global_tuning.zero_offset;
  }
  Menu_Navigate(CurrentMenuItem->Parent);
}
*/

static void PIN_Enter(uint8_t dir) {
  if (dir) {
    if (CurrentMenuItem->id == 70) {
      if (oled_msg.pin.pin0 == 9) {
        oled_msg.pin.pin0 = 0;
      } else {
        oled_msg.pin.pin0++;
      }
    } else if (CurrentMenuItem->id == 71) {
      if (oled_msg.pin.pin1 == 9) {
        oled_msg.pin.pin1 = 0;
      } else {
        oled_msg.pin.pin1++;
      }
    } else if (CurrentMenuItem->id == 72) {
      if (oled_msg.pin.pin2 == 9) {
        oled_msg.pin.pin2 = 0;
      } else {
        oled_msg.pin.pin2++;
      }
    }
  } else {
    if (CurrentMenuItem->id == 70) {
      if (oled_msg.pin.pin0 == 0) {
        oled_msg.pin.pin0 = 9;
      } else {
        oled_msg.pin.pin0--;
      }
    } else if (CurrentMenuItem->id == 71) {
      if (oled_msg.pin.pin1 == 0) {
        oled_msg.pin.pin1 = 9;
      } else {
        oled_msg.pin.pin1--;
      }
    } else if (CurrentMenuItem->id == 72) {
      if (oled_msg.pin.pin2 == 0) {
        oled_msg.pin.pin2 = 9;
      } else {
        oled_msg.pin.pin2--;
      }
    }
  }
}