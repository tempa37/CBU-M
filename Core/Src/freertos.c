/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "main.h"
#include "cmsis_os2.h"
#include "string.h"

#include "global_types.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "httpd_netconn.h"

// modbus master/slave rtu, tcp
#include "Modbus.h"
#include "flash_if.h"
#include "version.h"
#include "tim.h"
#include "usart.h"
#include "menu.h"
#include "keyboard.h"

#include "manage_settings.h"
#include "ring_line.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

// "usart.c"
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;

// "OLED_1in5_rgb_test.c"
extern void oled_thread(void *argument);

// "lwip.c"
extern void MX_LWIP_Init(void);
extern struct netif gnetif;
extern ip4_addr_t ipaddr;
extern ip4_addr_t netmask;
extern ip4_addr_t gw;

/* USER CODE END PTD */

#ifdef DEBUG
UBaseType_t WM_MBSlaveRTU;
UBaseType_t WM_MBSlaveTCP;

UBaseType_t WM_MBMasterRTU;
UBaseType_t WM_MBMasterTCP;

UBaseType_t WM_LWIP;
UBaseType_t WM_HTTPD;
UBaseType_t HighWaterMarkETH;
UBaseType_t HighWaterMarkETH_IN;

UBaseType_t WM_Scheduler;
UBaseType_t WM_OLED;
#endif

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

#ifdef DEBUG
uint32_t wd_tick;
uint32_t zap[12] = {0};
#endif

oled_t oled_msg = {0};

// osPriorityBelowNormal
osThreadId_t oled_task_handle;
const osThreadAttr_t oled_task_handle_attr = {
  .name = "Display",
  .stack_size = 128 * 16,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
// osPriorityBelowNormal

// osPriorityNormal
osThreadId_t main_task_handle;
const osThreadAttr_t main_task_handle_attr = {
  .name = "Main",
  .stack_size = 128 * 12,
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t ring_line_task_handle;
const osThreadAttr_t ring_line_task_handle_attr = {
  .name = "RingLine",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
//
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 16,
  .priority = (osPriority_t) osPriorityNormal,
};
//
osThreadId_t wdi_task_handle;
const osThreadAttr_t wdi_task_handle_attr = {  
  .name = "WDI",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
// osPriorityNormal

// osPriorityHigh
osThreadId_t httpd_task_handle;
const osThreadAttr_t httpd_task_handle_attr = {  
  .name = "HTTP server",
  .stack_size = 128 * 64,
  .priority = (osPriority_t) osPriorityHigh,
};
// osPriorityHigh

osSemaphoreId_t msgSemaphore;
osSemaphoreId_t valve_controlSemaphore;

osSemaphoreId_t flash_semaphore;

osMessageQueueId_t oled_msg_queue;
osMessageQueueId_t control_msg_queue;

osEventFlagsId_t load_system_flags;

//osTimerId_t timer_delayed_start;
osTimerId_t timer_delay_save;

uint16_t mb_reg_pressure[2] = {0};
uint16_t mb_reg_umvh[9] = {0};
uint16_t mb_reg_urm = 0;
uint8_t MACAddr[6] = {0};

valve_control_t local_valve_control = {0, VALVE_STOP};

uint64_t bit_map;
uint8_t index_map;

uint8_t switch_map;
uint8_t switch_state;

typedef enum control_type {
  CTRL_UNDEFINED,
  CTRL_RESET,
  CTRL_HAND,
  CTRL_STOP,
} control_type_t;

button_t button;

//#define TIME_BETWEEN_ATTEMPTS 10

typedef enum {
  CURRENT_IN = 1,
  NAMUR_IN = 2,
  PT100_IN = 3,
  LOGICAL_IN = 4,
  VOLTAGE_IN = 5,
  FREQ_IN = 6,
} input_type_t;

typedef enum {
  NCONNECTED = 0,
  CLOSE_CIRCUIT = 1,
  OPEN_CIRCUIT = 2,
  SHORT_CIRCUIT = 3,
} logical_type_t;


// internal connection
modbusHandler_t ModbusRS2;
uint16_t ModbusDATA_RS2[12];

// modbus RTU slave
modbusHandler_t ModbusRS1;

// modbus TCP master
//modbusHandler_t ModbusTCPm;
//uint16_t ModbusDATA_TCPm[30];

// modbus TCP slave
modbusHandler_t ModbusTCPs;

uint16_t ModbusDATA_Slave[64];

static const uint8_t uart_test_magic[4] = {0xA5, 0x5A, 0x3C, 0xC3};
static uint8_t uart_test_rx_it1[4];
static uint8_t uart_test_rx_it3[4];
static uint8_t uart_test_rx_copy1[4];
static uint8_t uart_test_rx_copy3[4];
static volatile uint8_t uart_test_rx_ready1 = 0;
static volatile uint8_t uart_test_rx_ready3 = 0;
static volatile uint8_t uart_test_mask = 0;
static volatile uint8_t uart_test_done = 0;
static volatile uint8_t uart_test_stage = 0;
static volatile uint8_t uart_test_stage2_pending = 0;
static volatile uint8_t uart_test_active = 0;

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
// Function Prototypes

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */
static void startDefaultTask(void *argument);
static void wdi_thread(void *argument);
static void main_task_thread(void *argument);
static void ring_line_thread(void *argument);
static void modbus_tcp_start(void);
static void uart_test_set_rs485(uint8_t tx1, uint8_t tx3);
static void uart_test_full_reset(void);
static void uart_test_start_stage(uint8_t stage);
bool uart_test_run(uint32_t timeout_ms, uint8_t *result_mask);
bool uart_test_handle_rx(UART_HandleTypeDef *huart);
//void delayed_start_callback(void *argument);

// Function Prototypes

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  load_settings();

  MACAddr[0] = 0x00;
  MACAddr[1] = 0x10;
  MACAddr[2] = 0xa1;
  MACAddr[3] = 0x71;
  MACAddr[4] = identification.mac_octet_5;
  MACAddr[5] = identification.mac_octet_6;
  
  //readFlash(CONFIGURATION_ADDR, (uint32_t *)&settings, sizeof(settings_t));
  
  if (settings.pressure_set_point > 10000) {
    settings.pressure_set_point = 10000;
  }  

  // button "to the right" and button "to the left"
  if ((HAL_GPIO_ReadPin(K1_Port, K1_Pin) == 0 && HAL_GPIO_ReadPin(K3_Port, K3_Pin) == 0) || 
      settings.self_ip_addr.addr == IPADDR_NONE || 
        settings.self_ip_addr.addr == IPADDR_ANY) 
  {
    ip4_addr_set_u32(&settings.self_ip_addr, default_settings.self_ip_addr.addr);
    ip4_addr_set_u32(&settings.self_mask_addr, default_settings.self_mask_addr.addr);
    ip4_addr_set_u32(&settings.self_gateway_addr, default_settings.self_gateway_addr.addr);
  }
  
  ip4_addr_set(&ipaddr, &settings.self_ip_addr);
  ip4_addr_set(&netmask, &settings.self_mask_addr);
  ip4_addr_set(&gw, &settings.self_gateway_addr);
  
//  if (settings.urm_id == 0) {
//    settings.urm_id = 4;
//    settings.self_id = 1;
//  }

  // add events, ...
  
  load_system_flags = osEventFlagsNew(NULL);
  
  // add mutexes, ...

  // add semaphores, ...
  
  msgSemaphore = osSemaphoreNew(1, 1, NULL);
  
  valve_controlSemaphore = osSemaphoreNew(1, 1, NULL);
  
  flash_semaphore = osSemaphoreNew(1, 1, NULL);
  
  // start timers, add new ones, ...
  
  timer_delay_save = osTimerNew(delay_save_callback, osTimerOnce, (void *)0, NULL);

  // add queues, ...
  
  oled_msg_queue = osMessageQueueNew(1, sizeof(oled_t), NULL);
  
  keyboard_msg_queue = osMessageQueueNew(1, sizeof(button_t), NULL);
  
  control_msg_queue = osMessageQueueNew(1, sizeof(uint8_t), NULL);

  // Create the thread(s)
  
  // creation of defaultTask
  defaultTaskHandle = osThreadNew(startDefaultTask, NULL, &defaultTask_attributes);

  wdi_task_handle = osThreadNew(wdi_thread, NULL, &wdi_task_handle_attr);  

  oled_task_handle = osThreadNew(oled_thread, NULL, &oled_task_handle_attr);
  
  keyboard_task_handle = osThreadNew(keyboard_thread, NULL, &keyboard_task_handle_attr);

  httpd_task_handle = osThreadNew(http_server_thread, NULL, &httpd_task_handle_attr);
 
  modbus_tcp_start();

  main_task_handle = osThreadNew(main_task_thread, NULL, &main_task_handle_attr);
  ring_line_task_handle = osThreadNew(ring_line_thread, NULL, &ring_line_task_handle_attr);
}

/**
 * @brief Watchdog  
 * @param argument: Not used
 * @retval None
 */
static void wdi_thread(void *argument) {
  (void) argument;
#ifdef DEBUG
  wd_tick = 0;
#endif
  while (1) {
#ifdef DEBUG
    wd_tick++;
#endif
    osDelay(2000);
    HAL_GPIO_TogglePin(WDI_GPIO_Port, WDI_Pin);
  }
}

/**
 * @brief Modbus TCP Master and Slave
 * @retval None
 */
static void modbus_tcp_start(void) {
  /*
  ModbusTCPm.uModbusType = MB_MASTER;
  ModbusTCPm.u8id = 0;
  // for a master it could be higher depending on the slave speed
  ModbusTCPm.u16timeOut = 1000;
  // No RS485
  ModbusTCPm.EN_Port = NULL;
  ModbusTCPm.u16regs = ModbusDATA_TCPm;
  ModbusTCPm.u16regsize = sizeof(ModbusDATA_TCPm)/sizeof(ModbusDATA_TCPm[0]);
  // TCP hardware
  ModbusTCPm.xTypeHW = TCP_HW;
  // Initialize Modbus library
  ModbusInit(&ModbusTCPm);
  // Start capturing traffic on serial Port and initialize counters
  ModbusStart(&ModbusTCPm);
  */
  
  // Modbus TCP slave - external connection
  ModbusTCPs.uModbusType = MB_SLAVE;
  
  ModbusTCPs.num_tcp_conn = 3;
  
  ModbusTCPs.u8id = settings.self_id;
  // for a master it could be higher depending on the slave speed
  
  //ModbusTCPs.u16timeOut = 1000;  
  ModbusTCPs.sendtimeout = 1000;
  ModbusTCPs.recvtimeout = 1000;
  
  // No RS485
  ModbusTCPs.EN_Port = NULL;
  ModbusTCPs.u16regs = ModbusDATA_Slave;
  ModbusTCPs.u16regsize = sizeof(ModbusDATA_Slave)/sizeof(ModbusDATA_Slave[0]);
  // TCP hardware
  ModbusTCPs.xTypeHW = TCP_HW;
  ModbusTCPs.uTcpPort = 502;
  // Initialize Modbus library
  ModbusInit(&ModbusTCPs);
  // Start capturing traffic on serial Port and initialize counters
  ModbusStart(&ModbusTCPs);
}

/**
 * @brief Modbus RTU Master and Slave
 * @retval None
 */

/**
  * @brief Function implementing the defaultTask thread.
  * @param argument: Not used
  * @retval None
  */
static void startDefaultTask(void *argument) {
  (void) argument;

  // reset lan8710
  HAL_GPIO_WritePin(RST_PHYLAN_Port, RST_PHYLAN_Pin, GPIO_PIN_RESET);
  osDelay(100);
  HAL_GPIO_WritePin(RST_PHYLAN_Port, RST_PHYLAN_Pin, GPIO_PIN_SET);

  // init code for LWIP
  MX_LWIP_Init();
  
  osStatus_t status;
  
  //button_t button = {0};
  control_type_t control_id = CTRL_UNDEFINED;
  
  oled_t oled_msg_upd = {0};
  
  uint8_t click = 1;
  uint32_t click_tick = 0;
  
  // infinite loop
  while (1) {
#ifdef DEBUG
    WM_LWIP = uxTaskGetStackHighWaterMark(NULL);
#endif
    status = osMessageQueueGet(keyboard_msg_queue, &button, NULL, 2U);    
    if (status == osOK) {
      click_tick = HAL_GetTick() + 5000;
      if (osSemaphoreAcquire(msgSemaphore, 2) == osOK) {
        switch (button.id) {
          case KEY_ID_NO : {
            local_valve_control.state = VALVE_STOP;
            oled_msg.emer_state = 0;
            click = 1;
            break;
          }
          case KEY_ID_RIGHT : {
            //if (CurrentMenuItem->Child == &Me_1_1_2 || CurrentMenuItem->Child == &Me_1_2_2) {
            //  Menu_EnterCurrentItem(1);
            //}
            break;
          }
          case KEY_ID_LEFT : {
            //if (CurrentMenuItem->Child == &Me_1_1_2 || CurrentMenuItem->Child == &Me_1_2_2) {
            //  Menu_EnterCurrentItem(0);
            //}
            break;
          }
          case KEY_ID_INPUT : {
            if (click == 1) {
              Menu_Navigate(CurrentMenuItem->Child);
              click = 0;
            }
            break;
          }
          case KEY_ID_CANSEL : {
            if (click == 1) {
              Menu_Navigate(CurrentMenuItem->Parent);
              click = 0;
            }
            break;
          }
          case KEY_ID_UP : {
            if (click == 1) {
              Menu_Navigate(CurrentMenuItem->Previous);
              click = 0;
            }
            break;
          }
          case KEY_ID_DOWN : {
            if (click == 1) {
              Menu_Navigate(CurrentMenuItem->Next);
              click = 0;
            }
            break;
          }
          case KEY_ID_START : {
            // always allow
            if (CurrentMenuItem->Parent == &Menu_1) {
              // pressure valve UP
              Menu_EnterCurrentItem(1);
            } else {
              if (oled_msg.pressure_sensor[HYDRAULIC].state == STATE_NORMAL ||
                  oled_msg.pressure_sensor[HYDRAULIC].state == STATE_BELOW_NORMAL)
              {
                // pressure valve UP
                Menu_EnterCurrentItem(1);
              }
            }
            break;
          }
          case KEY_ID_STOP : {
            // always allow
            if (CurrentMenuItem->Parent == &Menu_1) {
              // pressure valve DOWN
              Menu_EnterCurrentItem(0);
            } else {
              if (oled_msg.pressure_sensor[HYDRAULIC].state == STATE_NORMAL ||
                  oled_msg.pressure_sensor[HYDRAULIC].state == STATE_ABOVE_NORMAL)
              {
                // pressure valve DOWN
                Menu_EnterCurrentItem(0);
              }
            }
            break;
          }
          case KEY_ID_EMER : {
            control_id = CTRL_STOP;
            break;
          }
          default: {
            break;
          }
        }
        osSemaphoreRelease(msgSemaphore);
      }
    }
    
    if (memcmp(&oled_msg_upd, &oled_msg, sizeof(oled_t)) != 0) {
      memcpy(&oled_msg_upd, &oled_msg, sizeof(oled_t));
      osMessageQueuePut(oled_msg_queue, &oled_msg_upd, 0U, 10U);
    }
    
    if (control_id != CTRL_UNDEFINED) {
      osMessageQueuePut(control_msg_queue, &control_id, 0U, 2U);
      control_id = CTRL_UNDEFINED;
    }

    if (click_tick < HAL_GetTick() && 
        (CurrentMenuItem == &Menu_1||CurrentMenuItem == &Menu_4||CurrentMenuItem == &Menu_5)) {
      Menu_Navigate(&Menu_7);
    }

    osDelay(20);
  }
}


/**
  * @brief 
  * @param Direction
  * @retval None
  */
/*
void stroke(int8_t dir) {
  switch (dir) {
    case -1: { // decrease coordinate
      // open valve1 
      bitClear(mb_reg_urm, settings.valve1_io - 1);
      // close valve2
      bitSet(mb_reg_urm, settings.valve2_io - 1);
      break;
    }
    case 0: { // stop
      // open valve1 
      bitClear(mb_reg_urm, settings.valve1_io - 1);
      // close valve2
      bitClear(mb_reg_urm, settings.valve2_io - 1);
      break;
    }
    case 1: { // increase coordinate
      // open valve1 
      bitSet(mb_reg_urm, settings.valve1_io - 1);
      // close valve2
      bitClear(mb_reg_urm, settings.valve2_io - 1);
      break;
    }
  }
}
*/
/*
#define SELFTEST_NRUN 0
#define SELFTEST_RUN 1
#define SELFTEST_OK 2
#define SELFTEST_NOK 3

typedef struct {
  uint8_t count;
  uint8_t state_statistics;
  uint8_t threshold_change;
  uint8_t state_umvh;
} debounce_t;

debounce_t debounce_logic = {0};

typedef struct {
  uint8_t state;
  uint32_t tick;
  int8_t dir;
  uint16_t start_position;
} selftest_t;

selftest_t test_sensor = {0};
*/

void modbus_set_timeout(modbusHandler_t *mb) {
  if (osSemaphoreAcquire(mb->ModBusSphrHandle, 100) == osOK) {
    //setTimeOut(mb, settings.mb_timeout);
    set_timeout(mb, settings.mb_timeout, settings.mb_timeout);
    osSemaphoreRelease(mb->ModBusSphrHandle);
  }
}

uint16_t count_bits_set_parallel(uint64_t x) {
  // put count of each 2 bits into those 2 bits
  x -= (x >> 1) & 0x5555555555555555UL;
  // put count of each 4 bits into those 4 bits
  x = (x & 0x3333333333333333UL) + ((x >> 2) & 0x3333333333333333UL);
  // put count of each 8 bits into those 8 bits
  x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fUL;
  // returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ...
  return (x * 0x0101010101010101UL) >> 56;
}

/**
 * @brief  
 * @param argument: Not used
 * @retval None
 */

static void uart_test_set_rs485(uint8_t tx1, uint8_t tx3) {
  HAL_GPIO_WritePin(RS485_1_ON_Port, RS485_1_ON_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(RS485_2_ON_Port, RS485_2_ON_Pin, GPIO_PIN_SET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(UART1_RE_DE_Port, UART1_RE_DE_Pin, tx1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(UART2_RE_DE_Port, UART2_RE_DE_Pin, tx3 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void uart_test_full_reset(void) {
  HAL_UART_Abort(&huart1);
  HAL_UART_Abort(&huart3);
  HAL_UART_DeInit(&huart1);
  HAL_UART_DeInit(&huart3);
  HAL_Delay(20);
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  HAL_Delay(20);
}

static void uart_test_start_stage(uint8_t stage) {
  uart_test_stage = stage;
  uart_test_full_reset();

  if (stage == 0) {
    uart_test_set_rs485(1, 0);
    uart_test_rx_ready3 = 0;
    memset(uart_test_rx_it3, 0, sizeof(uart_test_rx_it3));
    memset(uart_test_rx_copy3, 0, sizeof(uart_test_rx_copy3));
    if (HAL_UART_Receive_IT(&huart3, uart_test_rx_it3, sizeof(uart_test_rx_it3)) != HAL_OK) {
      uart_test_done = 1;
      return;
    }
    if (HAL_UART_Transmit(&huart1, (uint8_t *)uart_test_magic, sizeof(uart_test_magic), 100) != HAL_OK) {
      uart_test_done = 1;
    }
  } else {
    uart_test_set_rs485(0, 1);
    uart_test_rx_ready1 = 0;
    memset(uart_test_rx_it1, 0, sizeof(uart_test_rx_it1));
    memset(uart_test_rx_copy1, 0, sizeof(uart_test_rx_copy1));
    if (HAL_UART_Receive_IT(&huart1, uart_test_rx_it1, sizeof(uart_test_rx_it1)) != HAL_OK) {
      uart_test_done = 1;
      return;
    }
    if (HAL_UART_Transmit(&huart3, (uint8_t *)uart_test_magic, sizeof(uart_test_magic), 100) != HAL_OK) {
      uart_test_done = 1;
    }
  }
}

bool uart_test_run(uint32_t timeout_ms, uint8_t *result_mask) {
  uint32_t start_tick = HAL_GetTick();

  uart_test_mask = 0;
  uart_test_done = 0;
  uart_test_stage = 0;
  uart_test_stage2_pending = 0;
  uart_test_rx_ready1 = 0;
  uart_test_rx_ready3 = 0;
  uart_test_active = 1;

  uart_test_start_stage(0);

  while (!uart_test_done && (HAL_GetTick() - start_tick) < timeout_ms) {
    if (uart_test_stage2_pending) {
      uart_test_stage2_pending = 0;
      HAL_Delay(100);
      uart_test_start_stage(1);
    }
    osDelay(1);
  }

  uart_test_active = 0;
  uart_test_set_rs485(0, 0);
  HAL_GPIO_WritePin(RS485_1_ON_Port, RS485_1_ON_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RS485_2_ON_Port, RS485_2_ON_Pin, GPIO_PIN_RESET);

  if (result_mask != NULL) {
    *result_mask = uart_test_mask;
  }
  return (uart_test_mask == 0x03);
}

bool uart_test_handle_rx(UART_HandleTypeDef *huart) {
  if (!uart_test_active) {
    return false;
  }

  if ((huart == &huart3) && (uart_test_stage == 0)) {
    memcpy(uart_test_rx_copy3, uart_test_rx_it3, sizeof(uart_test_rx_copy3));
    uart_test_rx_ready3 = 1;
    if (memcmp(uart_test_rx_copy3, uart_test_magic, sizeof(uart_test_magic)) == 0) {
      uart_test_mask |= 0x01;
    }
    uart_test_stage2_pending = 1;
    return true;
  }

  if ((huart == &huart1) && (uart_test_stage == 1)) {
    memcpy(uart_test_rx_copy1, uart_test_rx_it1, sizeof(uart_test_rx_copy1));
    uart_test_rx_ready1 = 1;
    if (memcmp(uart_test_rx_copy1, uart_test_magic, sizeof(uart_test_magic)) == 0) {
      uart_test_mask |= 0x02;
    }
    uart_test_done = 1;
    return true;
  }

  return false;
}

static void main_task_thread(void *argument) {
  (void) argument;

#ifdef DEBUG
  uint32_t runtime[3] = {0};
#endif

  Menu_Navigate(&Menu_7);

  uint32_t tick = osKernelGetTickCount();
  osStatus_t status;
  control_type_t control_id = CTRL_UNDEFINED;

  while (1) {
#ifdef DEBUG
    runtime[0] = HAL_GetTick();
#endif

    tick += pdMS_TO_TICKS(settings.mb_rate);

    status = osMessageQueueGet(control_msg_queue, &control_id, NULL, 2U);
    if (status == osOK) {
      if (control_id == CTRL_STOP) {
        oled_msg.emer_state = 1;
      }
    }

    if (osSemaphoreAcquire(ModbusTCPs.ModBusSphrHandle, 2) == osOK) {
      ModbusTCPs.u16regs[0] = VERSION_MAJOR;
      ModbusTCPs.u16regs[1] = VERSION_MINOR;
      ModbusTCPs.u16regs[2] = VERSION_BUILD;
      ModbusTCPs.u16regs[3] = identification.serial_number;
      osSemaphoreRelease(ModbusTCPs.ModBusSphrHandle);
    }

#ifdef DEBUG
    runtime[1] = HAL_GetTick();
    runtime[2] = runtime[1] - runtime[0];
#endif

#if CHECK_LOAD_SYSTEM == 1
    osEventFlagsSet(load_system_flags, MAIN_LOAD_BIT);
#endif

    osDelayUntil(tick);
  }
}

/*
void delayed_start_callback(void *argument) {
  (void) argument;  
  switch_state = 1;
}
*/
static void ring_line_thread(void *argument) {
  (void) argument;

  while (1) {
    ring_line_process_task_step();
    osDelay(500);
  }
}
