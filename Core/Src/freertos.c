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

#include "flash_if.h"
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


volatile uint8_t uart_test_request = 0;
volatile uint8_t uart_test_ready = 0;
volatile uint8_t uart_test_ok = 0;
char uart_test_message[96] = "Тест не запускался";

typedef enum {
  UART_TEST_IDLE = 0,
  UART_TEST_RUN,
  UART_TEST_DONE,
} uart_test_state_t;

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
// Function Prototypes

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */
static void startDefaultTask(void *argument);
static void wdi_thread(void *argument);
static void main_task_thread(void *argument);
static void ring_line_thread(void *argument);
static void uart_test_set_rs485(uint8_t tx1, uint8_t tx3);
static void uart_test_run_once(UART_HandleTypeDef *tx_uart, UART_HandleTypeDef *rx_uart, uint8_t tx1, const uint8_t *packet, uint16_t len, const char *name, uint8_t *ok);
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

  
  //
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
  * @brief Function implementing the defaultTask thread.
  * @param argument: Not used
  * @retval None
  */
static void startDefaultTask(void *argument) {
  (void) argument;


  // init code for LWIP
  MX_LWIP_Init();

  httpd_task_handle = osThreadNew(http_server_thread, NULL, &httpd_task_handle_attr);
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

/**
 * @brief  
 * @param argument: Not used
 * @retval None
 */

static void uart_test_set_rs485(uint8_t tx1, uint8_t tx3) {
  HAL_GPIO_WritePin(RS485_1_ON_Port, RS485_1_ON_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(RS485_2_ON_Port, RS485_2_ON_Pin, GPIO_PIN_RESET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(UART1_RE_DE_Port, UART1_RE_DE_Pin, tx1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(UART2_RE_DE_Port, UART2_RE_DE_Pin, tx3 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void uart_test_prepare(UART_HandleTypeDef *uart) {
  volatile uint32_t sr = 0;
  volatile uint32_t dr = 0;

  HAL_UART_Abort(uart);

  __HAL_UART_CLEAR_PEFLAG(uart);
  __HAL_UART_CLEAR_FEFLAG(uart);
  __HAL_UART_CLEAR_NEFLAG(uart);
  __HAL_UART_CLEAR_OREFLAG(uart);
  __HAL_UART_CLEAR_IDLEFLAG(uart);
  __HAL_UART_CLEAR_FLAG(uart, UART_FLAG_TC);

#ifdef UART_FLAG_CTS
  __HAL_UART_CLEAR_FLAG(uart, UART_FLAG_CTS);
#endif

  while (__HAL_UART_GET_FLAG(uart, UART_FLAG_RXNE) != RESET) {
    dr = uart->Instance->DR;
    (void)dr;
  }

  if (__HAL_UART_GET_FLAG(uart, UART_FLAG_ORE) != RESET) {
    sr = uart->Instance->SR;
    dr = uart->Instance->DR;
    (void)sr;
    (void)dr;
  }

#if defined(USART_SR_WUF)
  __HAL_UART_CLEAR_FLAG(uart, USART_SR_WUF);
#endif
}

static void uart_test_reinit(void) {
  HAL_UART_DeInit(&huart1);
  HAL_UART_DeInit(&huart3);
  HAL_Delay(20);
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  
  HAL_NVIC_EnableIRQ(USART1_IRQn);
  HAL_NVIC_EnableIRQ(USART3_IRQn);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
  HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
  HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  
}

static void uart_test_run_once(UART_HandleTypeDef *tx_uart, UART_HandleTypeDef *rx_uart, uint8_t tx1, const uint8_t *packet, uint16_t len, const char *name, uint8_t *ok) {
  uint8_t rx_buf[8] = {0};
  uint32_t start_tick = 0;

  uart_test_prepare(rx_uart);
  uart_test_prepare(tx_uart);
  osDelay(20);

  uart_test_set_rs485(!tx1, tx1);
  osDelay(10);

  if (HAL_UART_Receive_DMA(rx_uart, rx_buf, len) != HAL_OK) {
    *ok = 0;
    snprintf(uart_test_message, sizeof(uart_test_message), "%s: ошибка запуска DMA приема", name);
    return;
  }

  osDelay(100);
  
  
  if (HAL_UART_Transmit_IT(tx_uart, (uint8_t *)packet, len) != HAL_OK) {
    *ok = 0;
    HAL_UART_AbortReceive(rx_uart);
    snprintf(uart_test_message, sizeof(uart_test_message), "%s: ошибка передачи", name);
    return;
  }

  osDelay(100);
  
  if (rx_uart->hdmarx == NULL) {
    HAL_UART_AbortReceive(rx_uart);
    *ok = 0;
    snprintf(uart_test_message, sizeof(uart_test_message), "%s: RX DMA не сконфигурирован", name);
    return;
  }

  start_tick = HAL_GetTick();
  while (__HAL_DMA_GET_COUNTER(rx_uart->hdmarx) > 0U) {
    if ((HAL_GetTick() - start_tick) > 1500U) {
      HAL_UART_AbortReceive(rx_uart);
      *ok = 0;
      snprintf(uart_test_message, sizeof(uart_test_message), "%s: нет приема", name);
      return;
    }
    osDelay(1);
  }

  if (memcmp(rx_buf, packet, len) != 0) {
    *ok = 0;
    snprintf(uart_test_message, sizeof(uart_test_message), "%s: пакет не совпал", name);
    return;
  }
  else
  {
    snprintf(uart_test_message, sizeof(uart_test_message), "%s: пакет совпал", name);
  }
}

static void main_task_thread(void *argument) {
  (void) argument;
  
#ifdef DEBUG  
  uint32_t runtime[3] = {0};
#endif
  
  Menu_Navigate(&Menu_7);
  
  uint32_t tick = osKernelGetTickCount();
  
  switch_state = 0;
  
  osStatus_t status;
  uart_test_state_t uart_test_state = UART_TEST_IDLE;
  const uint8_t uart_packet_a[6] = {0xA5, 0x5A, 0x01, 0x02, 0x03, 0x04};
  const uint8_t uart_packet_b[6] = {0x3C, 0xC3, 0x10, 0x20, 0x30, 0x40};
  uint32_t uart_test_resume_tick = 0;
  
  control_type_t control_id = CTRL_UNDEFINED;
  
#if CHECK_LOAD_SYSTEM == 1  
  //osEventFlagsWait(load_system_flags, DISPLAY_LOAD_BIT, osFlagsWaitAny|osFlagsNoClear, osWaitForever);
#endif
  
  // close valve1 
  //bitSet(mb_reg_urm, settings.valve1_io + 7);
  // open valve2
  //bitSet(mb_reg_urm, settings.valve2_io + 7);
  
  while (1) {
#ifdef DEBUG
    runtime[0] = HAL_GetTick();
#endif    

    tick += pdMS_TO_TICKS(settings.mb_rate);

    status = osMessageQueueGet(control_msg_queue, &control_id, NULL, 2U);
    if (status == osOK) {
      switch (control_id) {
        case CTRL_RESET : {
          break;
        }
        case CTRL_HAND : {
          break;
        }
        case CTRL_STOP : {
          oled_msg.emer_state = 1;
          break;
        }
        default: {
          break;
        }
      }
    }

    if (uart_test_request && uart_test_state != UART_TEST_RUN) {
      uart_test_request = 0;
      uart_test_ready = 0;
      uart_test_ok = 1;
      snprintf(uart_test_message, sizeof(uart_test_message), "Тест выполняется");
      uart_test_state = UART_TEST_RUN;
    }

    if (uart_test_state == UART_TEST_RUN) {
      uart_test_reinit();

      uart_test_run_once(&huart1, &huart3, 1, uart_packet_a, sizeof(uart_packet_a), "UART1->UART3", (uint8_t *)&uart_test_ok);
      if (uart_test_ok) {
        osDelay(50);
        uart_test_run_once(&huart3, &huart1, 0, uart_packet_b, sizeof(uart_packet_b), "UART3->UART1", (uint8_t *)&uart_test_ok);
      }
      uart_test_set_rs485(0, 0);
      HAL_GPIO_WritePin(RS485_1_ON_Port, RS485_1_ON_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(RS485_2_ON_Port, RS485_2_ON_Pin, GPIO_PIN_RESET);
      if (uart_test_ok) {
        snprintf(uart_test_message, sizeof(uart_test_message), "UART1<->UART3 OK");
      }
      uart_test_ready = 1;
      uart_test_state = UART_TEST_DONE;
      uart_test_resume_tick = osKernelGetTickCount() + pdMS_TO_TICKS(30000U);
    }

    if ((uart_test_state == UART_TEST_RUN) ||
        ((uart_test_state == UART_TEST_DONE) && ((int32_t)(osKernelGetTickCount() - uart_test_resume_tick) < 0))) {
      osDelay(20);
      continue;
    }

#ifdef DEBUG
    WM_Scheduler = uxTaskGetStackHighWaterMark(NULL);
    zap[0] = HAL_GetTick();
#endif
    
    if (osSemaphoreAcquire(msgSemaphore, 2) == osOK) {
      oled_msg.pressure_sensor[MAINLINE].state = STATE_OFFLINE;
      oled_msg.pressure_sensor[MAINLINE].value = 0;
      oled_msg.pressure_sensor[HYDRAULIC].state = STATE_OFFLINE;
      oled_msg.pressure_sensor[HYDRAULIC].value = 0;
      oled_msg.valve.state = STATE_OFFLINE;
      oled_msg.urm_state = STATE_OFFLINE;
      oled_msg.umvh_state = STATE_OFFLINE;
      oled_msg.position_state = STATE_OFFLINE;
      oled_msg.position = 0;
      oled_msg.state_in = DI_UNDEFINED;
      oled_msg.state_out = DO_NRUN;
      osSemaphoreRelease(msgSemaphore);
    }
    
    if (osSemaphoreAcquire(valve_controlSemaphore, 2) == osOK) {
      if (local_valve_control.tick < osKernelGetTickCount() || 
          local_valve_control.state == VALVE_STOP) {
        local_valve_control.state = VALVE_STOP;
      }
      oled_msg.valve.value_out = 0;
      osSemaphoreRelease(valve_controlSemaphore);
    }
    
/*
    if (((mb_reg_umvh[settings.sens3_io] >> 12) & 0xF) == VOLTAGE_IN && oled_msg.umvh_state == STATE_ONLINE) {
      float coef_a = (global_tuning.max_stroke - global_tuning.min_stroke) / ((global_tuning.adc_max_stroke - global_tuning.adc_min_stroke) / 100.f);
      float coef_b = global_tuning.adc_min_stroke / 100.0f * coef_a;
      
      if (osSemaphoreAcquire(msgSemaphore, 2) == osOK) {
        // convert volts -> mm and add zero offset
        oled_msg.position = (uint16_t)(coef_a * (mb_reg_umvh[settings.sens3_io] & 0xFFF) / 100.0f - coef_b + global_tuning.zero_offset);
        
        if (oled_msg.position > 9999) {
          oled_msg.position = 9999;
        }
        
        oled_msg.position_state = STATE_NORMAL;
        
        // position outside the working range?
        if (oled_msg.position < settings.sens3_min_val || oled_msg.position > settings.sens3_max_val) {
          // warning
          oled_msg.position_state = STATE_BELOW_NORMAL;
          // error position near extreme values
          if (oled_msg.position <= settings.sens3_min_lim ) {
            oled_msg.position_state = STATE_MIN_LIM;
          } else if (oled_msg.position >= settings.sens3_max_lim) {
            oled_msg.position_state = STATE_MAX_LIM;
          }
        }
        osSemaphoreRelease(msgSemaphore);
      }
    } else {
      if (osSemaphoreAcquire(msgSemaphore, 2) == osOK) {
        // error - input type mismatch
        oled_msg.position_state = STATE_UNDEFINED;
        osSemaphoreRelease(msgSemaphore);
      }
    }
*/
    
/*    
    debounce_logic.count = (global_tuning.debounce_in / settings.mb_rate) << 1;
    
    if (debounce_logic.count > 63) {
      debounce_logic.count = 63;
    }
*/
    
/*
    if (((mb_reg_umvh[settings.state_in_io] >> 12) & 0xF) == LOGICAL_IN && oled_msg.umvh_state == STATE_ONLINE) {
      debounce_logic.state_umvh = mb_reg_umvh[settings.state_in_io] & 0x3;
      if (debounce_logic.state_umvh == SHORT_CIRCUIT) {
        bit_map |= (1 << index_map);
      } else {
        bit_map &= ~(1 << index_map);
      }        
      index_map = (index_map == debounce_logic.count) ? 0 : index_map + 1;
    } else {
      debounce_logic.state_umvh = NCONNECTED;
    }
*/
    
/*    
    debounce_logic.state_statistics = count_bits_set_parallel(bit_map);
    
    debounce_logic.threshold_change = debounce_logic.count >> 1;

    if (osSemaphoreAcquire(msgSemaphore, 2) == osOK) {
      
      if ((debounce_logic.state_statistics > debounce_logic.threshold_change) && 
          (debounce_logic.state_umvh != NCONNECTED)) 
      {
        oled_msg.state_in = DI_RUN;
      } else if (debounce_logic.state_umvh == NCONNECTED) {
        oled_msg.state_in = DI_UNDEFINED;
        switch_state = 0;
        switch_map = 0;
      } else {
        oled_msg.state_in = DI_NRUN;
        switch_state = 0;
        switch_map = 0;
      }

      osSemaphoreRelease(msgSemaphore);
    }
*/
    
/*
    if (oled_msg.system_control_mode == AUTO_SYS_CONTROL && 
        (oled_msg.position_state == STATE_NORMAL || oled_msg.position_state == STATE_BELOW_NORMAL)) 
    {
      if (oled_msg.state_in == DI_RUN) {
        if (osSemaphoreAcquire(msgSemaphore, 2) == osOK) {
          if (test_sensor.state == SELFTEST_NRUN) {
            test_sensor.state = SELFTEST_RUN;
            test_sensor.tick = 0;
          } 
          if (test_sensor.state == SELFTEST_RUN) {
            //
            if (test_sensor.tick == 0) {
              test_sensor.start_position = oled_msg.position;
              if ((oled_msg.position - global_tuning.min_stroke_rod) > settings.sens3_min_val) {
                test_sensor.dir = STROKE_DOWN;
              } else {
                test_sensor.dir = STROKE_UP;
              }
            }

            stroke(test_sensor.dir);

            if (abs(oled_msg.position - test_sensor.start_position) > global_tuning.min_stroke_rod) {
              test_sensor.state = SELFTEST_OK;
            }
            if ((test_sensor.state != SELFTEST_OK) && 
                (test_sensor.tick > global_tuning.self_test_duration * 10)) 
            {
              test_sensor.state = SELFTEST_NOK;
              stroke(STROKE_STOP);
            }
            test_sensor.tick++;            
          }
          osSemaphoreRelease(msgSemaphore);
        }
      } else {
        test_sensor.state = SELFTEST_NRUN;
      }

      if (oled_msg.pressure1_state == STATE_ONLINE && 
          oled_msg.pressure2_state == STATE_NORMAL &&
          test_sensor.state == SELFTEST_OK && 
          oled_msg.state_in == DI_RUN && switch_state &&
         (oled_msg.position_state == STATE_NORMAL || oled_msg.position_state == STATE_BELOW_NORMAL))
      {
        oled_msg.state_out = DO_RUN;
        bitSet(mb_reg_urm, settings.state_out_io - 1);
      } else {
        oled_msg.state_out = DO_NRUN;
        bitClear(mb_reg_urm, settings.state_out_io - 1);
      }
      
      if (oled_msg.state_in == DI_RUN && test_sensor.state == SELFTEST_OK) {
        float error = global_tuning.hysteresis_window;      
        float delta = mb_reg_pressure[HYDRAULIC] - settings.pressure_set_point;

        if (delta > error) {
          stroke(STROKE_UP);
          switch_map = 0;
          osTimerStop(timer_delayed_start);
        } else if (delta < -error) {
          stroke(STROKE_DOWN);
          switch_map = 0;
          osTimerStop(timer_delayed_start);
        } else {
          stroke(STROKE_STOP);
          if (switch_map < 64) {
            switch_map++;
          }
        }
        if (switch_map == 64) {
          if (!switch_state && !osTimerIsRunning(timer_delayed_start)) {
            osTimerStart(timer_delayed_start, global_tuning.delay_out);
          }          
        }
      }
    } else { // LOCAL_SYS_CONTROL
      oled_msg.state_out = DO_NRUN;
      bitClear(mb_reg_urm, settings.state_out_io - 1);

      if (osSemaphoreAcquire(valve_controlSemaphore, 2) == osOK) {
        if (local_valve_control == 0) {
          control_count = 5;
        } else {
          if (control_count > 0) {
            control_count--;
          } else {
            local_valve_control = 0;
          }
          
          if ((oled_msg.position_state == STATE_MIN_LIM && local_valve_control == STROKE_UP) || 
              (oled_msg.position_state == STATE_MAX_LIM && local_valve_control == STROKE_DOWN) || 
              (oled_msg.position_state != STATE_MIN_LIM && oled_msg.position_state != STATE_MAX_LIM)) 
          {
            stroke(local_valve_control);
          }
        }
        osSemaphoreRelease(valve_controlSemaphore);
      }
    }
*/
    
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
