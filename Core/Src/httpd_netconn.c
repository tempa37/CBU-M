#include "httpd_netconn.h"
#include "lwip/apps/fs.h"
#include "cmsis_os2.h"
#include "lwip/api.h"
#include "string.h"

#include "jsmn.h"
#include "flash_if.h"
#include <stdbool.h>
#include "crc.h"
#include "version.h"
#include "socket.h"
#include "tcp.h"
#include "global_types.h"
#include "manage_settings.h"
#include "ring_line.h"
#include "main.h"
#include "keyboard.h"
#include "ktv.h"
#include <stdio.h>

// for function mktime() & difftime()
//#include <time.h>

#define DELAY_MS(x) (osDelay(pdMS_TO_TICKS(x)))

void http_server_netconn_init();

static void http_server(struct netconn *conn);
//uint8_t config(const char *json, jsmntok_t *t, uint8_t count, element_type_t tp, uint8_t sp);
static int fwupdate_state_machine(struct netconn* conn, char* buf, u16_t buflen);
static void fwupdate_send_success(struct netconn* conn, const char* str);
static int jsoneq(const char *json, jsmntok_t *tok, const char *s);
static void stage_set(struct netconn *conn, uint8_t config);
static void stage_pins(struct netconn *conn);
static void stage_keyboard_state(struct netconn *conn);
static void stage_ktv_state(struct netconn *conn);
static void manual_pin_write_by_name(const char *pin_name, uint8_t value);
static uint8_t manual_pin_read_pb15(void);
static void relay_test_set_mode(uint8_t enable);
static void relay_test_drive(uint8_t value);

volatile uint8_t manual_pins_mode = 0U;
volatile uint8_t relay_test_mode = 0U;
static uint8_t relay_test_prev_pd13 = 0U;
static uint8_t relay_test_prev_pb15 = 0U;
//void stage_load(struct netconn *conn);

static const unsigned char PAGE_HEADER_200_OK[] = {
  //"HTTP/1.1 200 OK"
  0x48, 0x54, 0x54, 0x50, 0x2F, 0x31, 0x2E, 0x31, 0x20, 0x32, 0x30, 0x30, 0x20, 
  0x4F, 0x4B, 0x0D, 0x0A,
  0x00
};

/*
static const unsigned char PAGE_HEADER_503[] = {
  //"HTTP/1.1 503 Service Unavailable"
  0x35, 0x30, 0x33, 0x20, 0x53, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x20, 0x55, 
  0x6E, 0x61, 0x76, 0x61, 0x69, 0x6C, 0x61, 0x62, 0x6C, 0x65, 0x0D, 0x0A,
  0x00
};
*/

static const unsigned char PAGE_HEADER_SERVER[] = {
  //"Server: lwIP"
  0x53, 0x65, 0x72, 0x76, 0x65, 0x72, 0x3A, 0x20, 0x6C, 0x77, 0x49, 0x50, 0x0D, 
  0x0A,
  0x00
};

static const unsigned char PAGE_HEADER_CONTENT_TEXT[] = {
  //"Content-type: text/html"
  0x43, 0x6F, 0x6E, 0x74, 0x65, 0x6E, 0x74, 0x2D, 0x74, 0x79, 0x70, 0x65, 0x3A, 
  0x20, 0x74, 0x65, 0x78, 0x74, 0x2F, 0x68, 0x74, 0x6D, 0x6C, 0x0D, 0x0A, 0x0D,
  0x0A,
  0x00
};

static const unsigned char PAGE_HEADER_CONTENT_JSON[] = {
  //"Content-type: application/json; charset=utf-8"
  0x43, 0x6F, 0x6E, 0x74, 0x65, 0x6E, 0x74, 0x2D, 0x74, 0x79, 0x70, 0x65, 0x3A, 
  0x20, 0x61, 0x70, 0x70, 0x6C, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6F, 0x6E, 0x2F, 
  0x6A, 0x73, 0x6F, 0x6E, 0x3B, 0x20, 0x63, 0x68, 0x61, 0x72, 0x73, 0x65, 0x74, 
  0x3D, 0x75, 0x74, 0x66, 0x2D, 0x38, 0x0D, 0x0A, 0x0D, 0x0A,
  0x00
};

static const unsigned char PAGE_HEADER_REQUIRED[] = {
  //"HTTP/1.1 401 Authorization Required"
  0x48, 0x54, 0x54, 0x50, 0x2F, 0x31, 0x2E, 0x31, 0x20, 0x34, 0x30, 0x31, 0x20, 
  0x41, 0x75, 0x74, 0x68, 0x6F, 0x72, 0x69, 0x7A, 0x61, 0x74, 0x69, 0x6F, 0x6E, 
  0x20, 0x52, 0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x64, 0x0D, 0x0A,
  0x00
};

static const unsigned char PAGE_HEADER_BASIC[] = {
  //"WWW-Authenticate: Basic realm="User Visible Realm""
  0x57, 0x57, 0x57, 0x2D, 0x41, 0x75, 0x74, 0x68, 0x65, 0x6E, 0x74, 0x69, 0x63, 
  0x61, 0x74, 0x65, 0x3A, 0x20, 0x42, 0x61, 0x73, 0x69, 0x63, 0x20, 0x72, 0x65, 
  0x61, 0x6C, 0x6D, 0x3D, 0x22, 0x55, 0x73, 0x65, 0x72, 0x20, 0x56, 0x69, 0x73, 
  0x69, 0x62, 0x6C, 0x65, 0x20, 0x52, 0x65, 0x61, 0x6C, 0x6D, 0x22, 0x0D, 0x0A,
  0x00
};

/*
static const unsigned char PAGE_HEADER_CONNECTION[] = {
  //"Connection: keep-alive"
  0x43, 0x6F, 0x6E, 0x6E, 0x65, 0x63, 0x74, 0x69, 0x6F, 0x6E, 0x3A, 0x20, 0x6B,
  0x65, 0x65, 0x70, 0x2D, 0x61, 0x6C, 0x69, 0x76, 0x65, 0x0D, 0x0A,
  0x00
};
*/

static const unsigned char PAGE_HEADER_CONNECTION_CLOSE[] = {
  //"Connection: close"
  0x43, 0x6F, 0x6E, 0x6E, 0x65, 0x63, 0x74, 0x69, 0x6F, 0x6E, 0x3A, 0x20, 0x63, 
  0x6C, 0x6F, 0x73, 0x65, 0x0D, 0x0A,
  0x00
};

#define CONTENT_LENGTH_TAG "Content-Length:"
#define OCTET_STREAM_TAG "application/octet-stream\r\n\r\n"
#define BOUNDARY_TAG "multipart/form-data; boundary="
#define EMPTY_LINE_TAG "\r\n\r\n"
#define AUTHORIZATION_TAG "Authorization: Basic"

#define STATUS_ERROR -1
#define STATUS_NONE 0
#define STATUS_INPROGRESS 1
#define STATUS_DONE 2
  
typedef enum {
  FWUPDATE_STATE_HEADER,
  FWUPDATE_STATE_OCTET_START,
  FWUPDATE_STATE_OCTET_STREAM,
} fwupdate_state_t;

typedef struct {
  fwupdate_state_t state;
  int content_length;
  int file_length; /*!< file length in byte*/
  int accum_length;
  //volatile uint32_t addr;
  uint8_t accum_buf[4];
  uint8_t accum_buf_len;
} fwupdate_t;

static fwupdate_t fwupdate;

typedef enum {
  POST_STATE_HEADER,
  POST_STATE_START,
  POST_STATE_BODY,
} post_state_t;

typedef struct {
  post_state_t state;
  uint32_t content_length;
  uint32_t accum_length;
  // 2048
  char content[3072];
  uint8_t url;
} post_t;

static post_t post_data @ ".ccmram";

#ifdef DEBUG_FW_UPDATE
char start_bin[16];
char stop_bin[16];

uint32_t status_w;
uint32_t status_e;
#endif

#define TOK_LENGTH 500
jsmntok_t token[TOK_LENGTH]  @ ".ccmram";

// 2304
#define HTML_LEN 3072
char html[3072] @ ".ccmram";

uint16_t length_html = 0;

uint16_t boundary;

static __IO uint32_t FlashWriteAddress;

uint32_t end_address;

uint32_t recv_count;

osTimerId_t timerTimeout;

osSemaphoreId_t httpdbufSemaphore;

osSemaphoreId_t httpdSemaphore;

extern osSemaphoreId_t flash_semaphore;

extern volatile uint8_t uart_test_request;
extern volatile uint8_t uart_test_ready;
extern volatile uint8_t uart_test_ok;
extern char uart_test_message[96];

void send_response(struct netconn *conn) {
  char content_length_header[32] = {0};
  char response_headers[128] = {0};
  uint8_t headers_length;
  
  sprintf((char*)content_length_header,"Content-Length: %d\r\n", length_html);
  headers_length = sprintf((char*)response_headers,"%s%s%s%s%s", PAGE_HEADER_200_OK, PAGE_HEADER_SERVER, PAGE_HEADER_CONNECTION_CLOSE, content_length_header, PAGE_HEADER_CONTENT_JSON);
  netconn_write(conn, (const unsigned char*)response_headers, (size_t)headers_length, NETCONN_COPY | NETCONN_MORE);
  netconn_write(conn, (const unsigned char*)html, (size_t)length_html, NETCONN_COPY);
}

/**
 * @brief initialization
 *
 *
 * detailed description
 */
void stage_serial(struct netconn *conn) {
  if (osSemaphoreAcquire(httpdbufSemaphore, 100) == osOK) {    
    memset(html, 0, HTML_LEN);
    length_html = 0;
    //length_html = sprintf((char*)html,"%s%s%s%s", PAGE_HEADER_200_OK, PAGE_HEADER_SERVER, PAGE_HEADER_CONNECTION_CLOSE, PAGE_HEADER_CONTENT_JSON);
    length_html += sprintf((char*)(html + length_html), "{\"serial\":\"%d\",", identification.serial_number);
    // int to string hex value
    length_html += sprintf((char*)(html + length_html), "\"mac0\":\"%X\",", identification.mac_octet_6);
    // int to string hex value
    length_html += sprintf((char*)(html + length_html), "\"mac1\":\"%X\"}", identification.mac_octet_5);
    //netconn_write(conn, (const unsigned char*)html, (size_t)length_html, NETCONN_COPY);
    
    send_response(conn);
    
    osSemaphoreRelease(httpdbufSemaphore);
  }
}


void status_sys(struct netconn *conn) {
  if (osSemaphoreAcquire(httpdbufSemaphore, 100) == osOK) {    
    memset(html, 0, HTML_LEN);
    length_html = 0;
   
    length_html += sprintf((char*)(html + length_html), "{\"sen1\":\"%.1f\",", oled_msg.pressure_sensor[MAINLINE].value);
    length_html += sprintf((char*)(html + length_html), "\"sen1s\":\"%d\",", oled_msg.pressure_sensor[MAINLINE].state);
    
    length_html += sprintf((char*)(html + length_html), "\"sen2\":\"%.1f\",", oled_msg.pressure_sensor[HYDRAULIC].value);
    length_html += sprintf((char*)(html + length_html), "\"sen2s\":\"%d\",", oled_msg.pressure_sensor[HYDRAULIC].state);
    length_html += sprintf((char*)(html + length_html), "\"sen2min\":\"%d\",", settings.sens2_min_val);
    length_html += sprintf((char*)(html + length_html), "\"sen2max\":\"%d\",", settings.sens2_max_val);
    
    length_html += sprintf((char*)(html + length_html), "\"emer\":\"%d\",", oled_msg.emer_state);
    length_html += sprintf((char*)(html + length_html), "\"urm\":\"%d\",", oled_msg.urm_state);
    length_html += sprintf((char*)(html + length_html), "\"umvh\":\"%d\",", oled_msg.umvh_state);
    
    length_html += sprintf((char*)(html + length_html), "\"uptime\":\"%d\"}", HAL_GetTick()/1000);
   
    send_response(conn);
    
    osSemaphoreRelease(httpdbufSemaphore);
  }
}

/**
 * @brief initialization
 *
 *
 * detailed description
 */
/*
void stage_state(struct netconn *conn) {
  if (osSemaphoreAcquire(httpdbufSemaphore, 100) == osOK) {
    
    memset(html, 0, HTML_LEN);
    length_html = sprintf((char*)html,"%s%s%s%s", PAGE_HEADER_200_OK, PAGE_HEADER_SERVER, PAGE_HEADER_CONNECTION, PAGE_HEADER_CONTENT_JSON);
    length_html += sprintf((char*)(html + length_html), "{\"state\":[");
    for (uint8_t i = 0; i < 4; i++) {
      length_html += sprintf((char*)(html + length_html), "{\"e\":\"%d\",\"t\":\"%d\"},", thres[i].extinction, thres[i].transporter);
    }
    html[length_html-- - 1] = 0;
    length_html += sprintf((char*)(html + length_html), "]}");
    netconn_write(conn, (const unsigned char*)html, (size_t)length_html, NETCONN_COPY);
    
    osSemaphoreRelease(httpdbufSemaphore);
  }
}
*/

/**
 * @brief initialization
 *
 *
 * detailed description
 */

static uint8_t manual_pin_read_pb15(void) {
  return (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) == GPIO_PIN_SET) ? 1U : 0U;
}

static void manual_pin_write_by_name(const char *pin_name, uint8_t value) {
  GPIO_PinState pin_state = (value != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;

  if (pin_name == NULL) {
    return;
  }

  if (strcmp(pin_name, "wdi") == 0) {
    HAL_GPIO_WritePin(WDI_GPIO_Port, WDI_Pin, pin_state);
  } else if (strcmp(pin_name, "uart1_re_de") == 0) {
    HAL_GPIO_WritePin(UART1_RE_DE_Port, UART1_RE_DE_Pin, pin_state);
  } else if (strcmp(pin_name, "uart2_re_de") == 0) {
    HAL_GPIO_WritePin(UART2_RE_DE_Port, UART2_RE_DE_Pin, pin_state);
  } else if (strcmp(pin_name, "pwr_ktv") == 0) {
    HAL_GPIO_WritePin(PWR_KTV_GPIO_Port, PWR_KTV_Pin, pin_state);
  } else if (strcmp(pin_name, "rst_phylan") == 0) {
    HAL_GPIO_WritePin(RST_PHYLAN_Port, RST_PHYLAN_Pin, pin_state);
  } else if (strcmp(pin_name, "stm_loop_link") == 0) {
    HAL_GPIO_WritePin(STM_LOOP_LINK_GPIO_Port, STM_LOOP_LINK_Pin, pin_state);
  } else if (strcmp(pin_name, "ana_rele_2") == 0) {
    HAL_GPIO_WritePin(ANA_RELE_2_GPIO_Port, ANA_RELE_2_Pin, pin_state);
  } else if (strcmp(pin_name, "oled_dc") == 0) {
    HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, pin_state);
  } else if (strcmp(pin_name, "oled_cs") == 0) {
    HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, pin_state);
  } else if (strcmp(pin_name, "oled_rst") == 0) {
    HAL_GPIO_WritePin(OLED_RST_GPIO_Port, OLED_RST_Pin, pin_state);
  } else if (strcmp(pin_name, "pb15") == 0) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, pin_state);
  }
}

static void relay_test_set_mode(uint8_t enable) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (enable != 0U) {
    relay_test_prev_pd13 = (HAL_GPIO_ReadPin(ANA_RELE_2_GPIO_Port, ANA_RELE_2_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    relay_test_prev_pb15 = manual_pin_read_pb15();

    GPIO_InitStruct.Pin = CON_1_Pin|CON_2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    HAL_GPIO_WritePin(CON_1_Port, CON_1_Pin|CON_2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ANA_RELE_2_GPIO_Port, ANA_RELE_2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);

    manual_pins_mode = 0U;
    relay_test_mode = 1U;
  } else {
    if (relay_test_mode != 0U) {
      HAL_GPIO_WritePin(ANA_RELE_2_GPIO_Port, ANA_RELE_2_Pin, relay_test_prev_pd13 != 0U ? GPIO_PIN_SET : GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, relay_test_prev_pb15 != 0U ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

    GPIO_InitStruct.Pin = CON_1_Pin|CON_2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    relay_test_mode = 0U;
  }
}

static void relay_test_drive(uint8_t value) {
  if (relay_test_mode == 0U) {
    return;
  }

  HAL_GPIO_WritePin(CON_1_Port, CON_1_Pin|CON_2_Pin, value != 0U ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void stage_pins(struct netconn *conn) {
  if (osSemaphoreAcquire(httpdbufSemaphore, 100) == osOK) {
    uint8_t pe3 = (HAL_GPIO_ReadPin(BDU1_M_S_GPIO_Port, BDU1_M_S_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t pe4 = (HAL_GPIO_ReadPin(BDU2_M_S_GPIO_Port, BDU2_M_S_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t pd14 = (HAL_GPIO_ReadPin(CON_1_Port, CON_1_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t pd12 = (HAL_GPIO_ReadPin(CON_2_Port, CON_2_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t pc6 = (HAL_GPIO_ReadPin(MCU_BLK_1_1_GPIO_Port, MCU_BLK_1_1_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t pd15 = (HAL_GPIO_ReadPin(MCU_BLK_1_2_GPIO_Port, MCU_BLK_1_2_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t pd11 = (HAL_GPIO_ReadPin(MCU_BLK_2_1_GPIO_Port, MCU_BLK_2_1_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t pd10 = (HAL_GPIO_ReadPin(MCU_BLK_2_2_GPIO_Port, MCU_BLK_2_2_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t pd8 = (HAL_GPIO_ReadPin(CON_ON_OFF_GPIO_Port, CON_ON_OFF_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t pa15 = (HAL_GPIO_ReadPin(RS485_1_ON_Port, RS485_1_ON_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t pc9 = (HAL_GPIO_ReadPin(RS485_2_ON_Port, RS485_2_ON_Pin) == GPIO_PIN_SET) ? 1U : 0U;

    uint8_t out_wdi = (HAL_GPIO_ReadPin(WDI_GPIO_Port, WDI_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t out_uart1_re_de = (HAL_GPIO_ReadPin(UART1_RE_DE_Port, UART1_RE_DE_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t out_uart2_re_de = (HAL_GPIO_ReadPin(UART2_RE_DE_Port, UART2_RE_DE_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t out_pwr_ktv = (HAL_GPIO_ReadPin(PWR_KTV_GPIO_Port, PWR_KTV_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t out_rst_phylan = (HAL_GPIO_ReadPin(RST_PHYLAN_Port, RST_PHYLAN_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t out_stm_loop_link = (HAL_GPIO_ReadPin(STM_LOOP_LINK_GPIO_Port, STM_LOOP_LINK_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t out_ana_rele_2 = (HAL_GPIO_ReadPin(ANA_RELE_2_GPIO_Port, ANA_RELE_2_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t out_oled_dc = (HAL_GPIO_ReadPin(OLED_DC_GPIO_Port, OLED_DC_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t out_oled_cs = (HAL_GPIO_ReadPin(OLED_CS_GPIO_Port, OLED_CS_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t out_oled_rst = (HAL_GPIO_ReadPin(OLED_RST_GPIO_Port, OLED_RST_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t out_pb15 = manual_pin_read_pb15();
    uint8_t ring1_state = (uint8_t)g_ring_line_state;
    uint8_t ring2_state = (uint8_t)g_ring_line_state_2;
    uint16_t ring1_opto11_count = g_ring_line_opto_1_1_count;
    uint16_t ring1_opto12_count = g_ring_line_opto_1_2_count;
    uint16_t ring2_opto21_count = g_ring_line_opto_2_1_count;
    uint16_t ring2_opto22_count = g_ring_line_opto_2_2_count;

    memset(html, 0, HTML_LEN);
    length_html = 0;

    length_html += sprintf((char*)(html + length_html), "{\"pe3\":%u,", pe3);
    length_html += sprintf((char*)(html + length_html), "\"pe4\":%u,", pe4);
    length_html += sprintf((char*)(html + length_html), "\"pd14\":%u,", pd14);
    length_html += sprintf((char*)(html + length_html), "\"pd12\":%u,", pd12);
    length_html += sprintf((char*)(html + length_html), "\"pc6\":%u,", pc6);
    length_html += sprintf((char*)(html + length_html), "\"pd15\":%u,", pd15);
    length_html += sprintf((char*)(html + length_html), "\"pd11\":%u,", pd11);
    length_html += sprintf((char*)(html + length_html), "\"pd10\":%u,", pd10);
    length_html += sprintf((char*)(html + length_html), "\"pd8\":%u,", pd8);
    length_html += sprintf((char*)(html + length_html), "\"pa15\":%u,", pa15);
    length_html += sprintf((char*)(html + length_html), "\"pc9\":%u,", pc9);

    length_html += sprintf((char*)(html + length_html), "\"manual_mode\":%u,", manual_pins_mode);
    length_html += sprintf((char*)(html + length_html), "\"relay_test_mode\":%u,", relay_test_mode);
    length_html += sprintf((char*)(html + length_html), "\"out_wdi\":%u,", out_wdi);
    length_html += sprintf((char*)(html + length_html), "\"out_uart1_re_de\":%u,", out_uart1_re_de);
    length_html += sprintf((char*)(html + length_html), "\"out_uart2_re_de\":%u,", out_uart2_re_de);
    length_html += sprintf((char*)(html + length_html), "\"out_pwr_ktv\":%u,", out_pwr_ktv);
    length_html += sprintf((char*)(html + length_html), "\"out_rst_phylan\":%u,", out_rst_phylan);
    length_html += sprintf((char*)(html + length_html), "\"out_stm_loop_link\":%u,", out_stm_loop_link);
    length_html += sprintf((char*)(html + length_html), "\"out_ana_rele_2\":%u,", out_ana_rele_2);
    length_html += sprintf((char*)(html + length_html), "\"out_oled_dc\":%u,", out_oled_dc);
    length_html += sprintf((char*)(html + length_html), "\"out_oled_cs\":%u,", out_oled_cs);
    length_html += sprintf((char*)(html + length_html), "\"out_oled_rst\":%u,", out_oled_rst);
    length_html += sprintf((char*)(html + length_html), "\"out_pb15\":%u,", out_pb15);
    length_html += sprintf((char*)(html + length_html), "\"ring1_state\":%u,", ring1_state);
    length_html += sprintf((char*)(html + length_html), "\"ring2_state\":%u,", ring2_state);
    length_html += sprintf((char*)(html + length_html), "\"ring1_opto11_count\":%u,", ring1_opto11_count);
    length_html += sprintf((char*)(html + length_html), "\"ring1_opto12_count\":%u,", ring1_opto12_count);
    length_html += sprintf((char*)(html + length_html), "\"ring2_opto21_count\":%u,", ring2_opto21_count);
    length_html += sprintf((char*)(html + length_html), "\"ring2_opto22_count\":%u}", ring2_opto22_count);

    send_response(conn);

    osSemaphoreRelease(httpdbufSemaphore);
  }
}
static void stage_keyboard_state(struct netconn *conn) {
  if (osSemaphoreAcquire(httpdbufSemaphore, 100) == osOK) {
    uint16_t keyboard_mask = keyboard_get_web_mask();

    memset(html, 0, HTML_LEN);
    length_html = sprintf((char*)(html), "{\"m\":\"%04X\"}", keyboard_mask);

    send_response(conn);

    osSemaphoreRelease(httpdbufSemaphore);
  }
}

void stage_set(struct netconn *conn, uint8_t config) {
  if (osSemaphoreAcquire(httpdbufSemaphore, 100) == osOK) {
    memset(html, 0, HTML_LEN);
    
    length_html = 0;
    //length_html = sprintf((char*)html,"%s%s%s%s", PAGE_HEADER_200_OK, PAGE_HEADER_SERVER, PAGE_HEADER_CONNECTION_CLOSE, PAGE_HEADER_CONTENT_JSON);

    char version[12] = {0};
    sprintf(version, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);
    //
    length_html += sprintf((char*)(html + length_html), "{\"usk_ip\":\"%s\",", ip4addr_ntoa(&settings.self_ip_addr)); // 0
    length_html += sprintf((char*)(html + length_html), "\"usk_mask\":\"%s\",", ip4addr_ntoa(&settings.self_mask_addr)); // 1
    length_html += sprintf((char*)(html + length_html), "\"usk_gateway\":\"%s\",", ip4addr_ntoa(&settings.self_gateway_addr)); // 2
    length_html += sprintf((char*)(html + length_html), "\"self_id\":\"%d\",", settings.self_id); // 3
    length_html += sprintf((char*)(html + length_html), "\"mb_rate\":\"%d\",", settings.mb_rate); // 4
    length_html += sprintf((char*)(html + length_html), "\"mb_timeout\":\"%d\",", settings.mb_timeout); // 5
    
    length_html += sprintf((char*)(html + length_html), "\"sens1_id\":\"%d\",", settings.sens1_id); // 6
    length_html += sprintf((char*)(html + length_html), "\"sens2_id\":\"%d\",", settings.sens2_id); // 7
    length_html += sprintf((char*)(html + length_html), "\"sens2_min_val\":\"%d\",", settings.sens2_min_val); // 8
    length_html += sprintf((char*)(html + length_html), "\"sens2_max_val\":\"%d\",", settings.sens2_max_val); // 9
    
    length_html += sprintf((char*)(html + length_html), "\"sens3_min_lim\":\"%d\",", settings.sens3_min_lim); // 10
    length_html += sprintf((char*)(html + length_html), "\"sens3_max_lim\":\"%d\",", settings.sens3_max_lim); // 11
    length_html += sprintf((char*)(html + length_html), "\"sens3_min_val\":\"%d\",", settings.sens3_min_val); // 12
    length_html += sprintf((char*)(html + length_html), "\"sens3_max_val\":\"%d\",", settings.sens3_max_val); // 13
    
    length_html += sprintf((char*)(html + length_html), "\"urm_id\":\"%d\",", settings.urm_id); // 14
    length_html += sprintf((char*)(html + length_html), "\"valve1_io\":\"%d\",", settings.valve1_io); // 15
    length_html += sprintf((char*)(html + length_html), "\"valve2_io\":\"%d\",", settings.valve2_io); // 16
    length_html += sprintf((char*)(html + length_html), "\"state_out_io\":\"%d\",", settings.state_out_io); // 17 
    
    length_html += sprintf((char*)(html + length_html), "\"umvh_id\":\"%d\",", settings.umvh_id); // 18
    length_html += sprintf((char*)(html + length_html), "\"sens3_io\":\"%d\",", settings.sens3_io); // 19
    length_html += sprintf((char*)(html + length_html), "\"state_in_io\":\"%d\",", settings.state_in_io); // 20
    
    length_html += sprintf((char*)(html + length_html), "\"version\":\"%s\",", version);
    length_html += sprintf((char*)(html + length_html), "\"serial\":\"%d\",", identification.serial_number);
    // int to string hex value
    length_html += sprintf((char*)(html + length_html), "\"mac1\":\"%X\",", identification.mac_octet_5);
    // int to string hex value
    length_html += sprintf((char*)(html + length_html), "\"mac0\":\"%X\",", identification.mac_octet_6);
    length_html += sprintf((char*)(html + length_html), "\"uptime\":\"%d\"}", HAL_GetTick()/1000);
    
    //
    //netconn_write(conn, (const unsigned char*)html, (size_t)length_html, NETCONN_COPY);
    
    send_response(conn);
    
    osSemaphoreRelease(httpdbufSemaphore);
  }
}

void tuning_set(struct netconn *conn) {
  if (osSemaphoreAcquire(httpdbufSemaphore, 100) == osOK) {
    memset(html, 0, HTML_LEN);
    
    length_html = 0;
    //length_html = sprintf((char*)html,"%s%s%s%s", PAGE_HEADER_200_OK, PAGE_HEADER_SERVER, PAGE_HEADER_CONNECTION_CLOSE, PAGE_HEADER_CONTENT_JSON);
    
    length_html += sprintf((char*)(html + length_html), "{\"hysteresis_window\":\"%d\",", global_tuning.hysteresis_window);
    length_html += sprintf((char*)(html + length_html), "\"self_test_duration\":\"%d\",", global_tuning.self_test_duration);
    length_html += sprintf((char*)(html + length_html), "\"min_stroke_rod\":\"%d\",", global_tuning.min_stroke_rod);
    length_html += sprintf((char*)(html + length_html), "\"min_stroke\":\"%d\",", global_tuning.min_stroke);
    length_html += sprintf((char*)(html + length_html), "\"adc_min_stroke\":\"%d\",", global_tuning.adc_min_stroke);
    length_html += sprintf((char*)(html + length_html), "\"max_stroke\":\"%d\",", global_tuning.max_stroke);
    length_html += sprintf((char*)(html + length_html), "\"adc_max_stroke\":\"%d\",", global_tuning.adc_max_stroke);
    length_html += sprintf((char*)(html + length_html), "\"zero_offset\":\"%d\",", global_tuning.zero_offset);
    length_html += sprintf((char*)(html + length_html), "\"debounce_in\":\"%d\",", global_tuning.debounce_in);
    length_html += sprintf((char*)(html + length_html), "\"delay_out\":\"%d\"}", global_tuning.delay_out);

    //netconn_write(conn, (const unsigned char*)html, (size_t)length_html, NETCONN_COPY);
    
    send_response(conn);
    
    osSemaphoreRelease(httpdbufSemaphore);
  }
}

/**
 * @brief initialization
 *
 *
 * detailed description
 */
static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

/**
 * @brief initialization
 *
 *
 * detailed description
 */
static void fwupdate_send_success(struct netconn* conn, const char* str) {
  if (osSemaphoreAcquire(httpdbufSemaphore, 100) == osOK) {
    memset(html, 0, HTML_LEN);
    length_html = 0;
    //length_html = sprintf(html,"%s%s%s%s", PAGE_HEADER_200_OK, PAGE_HEADER_SERVER, PAGE_HEADER_CONNECTION_CLOSE, PAGE_HEADER_CONTENT_TEXT);
    length_html += sprintf((char*)(html + length_html), "%s", str);
    //netconn_write(conn, (const unsigned char*)html, length_html, NETCONN_COPY);
    
    send_response(conn);
    
    osSemaphoreRelease(httpdbufSemaphore);
  }
}

/**
 * @brief initialization
 *
 *
 * detailed description
 */
static int post_state_machine(struct netconn* conn, char* buf, u16_t buflen) {
  char* buf_end = buf + buflen;
  int ret = STATUS_NONE;
  while (buf && buf < buf_end) {
    switch (post_data.state) {
    case POST_STATE_HEADER: {
      post_data.url = 0;
/*
      if ((buflen >= 19) && (strncmp(buf, "POST /configuration", 19) == 0)) {
        post_data.url = 1;
      } else 
*/
      if ((buflen >= 14) && (strncmp(buf, "POST /settings", 14) == 0)) {
        post_data.url = 2;
      } else if ((buflen >= 12) && (strncmp(buf, "POST /serial", 12) == 0)) {
        post_data.url = 3;
      } else if ((buflen >= 12) && (strncmp(buf, "POST /tuning", 12) == 0)) {
        post_data.url = 4;
      } else if ((buflen >= 18) && (strncmp(buf, "POST /pins_control", 18) == 0)) {
        post_data.url = 5;
      }
      if (post_data.url > 0) {
        ret = STATUS_ERROR;
        buf = strstr(buf, CONTENT_LENGTH_TAG);
        if (buf) {
          buf += strlen(CONTENT_LENGTH_TAG);
          post_data.content_length = atoi(buf);
          if (post_data.content_length > 0) {
            buf = strstr(buf, EMPTY_LINE_TAG);
            if (buf) {
              buf += strlen(EMPTY_LINE_TAG);
              post_data.state = POST_STATE_START;
              ret = STATUS_INPROGRESS;
            }
          } else {
            buf = 0;
          }
        }
      } else {
        buf = 0;
      }
      break;
    }
    case POST_STATE_START: {
      post_data.state = POST_STATE_HEADER;
      ret = STATUS_ERROR;
      if (buf) {
        post_data.state = POST_STATE_BODY;
        post_data.accum_length = 0;
        memset(post_data.content, 0, sizeof(post_data.content));
        ret = STATUS_INPROGRESS;
      }
      break;
    }
    case POST_STATE_BODY: {
      uint32_t len = (uint32_t)(buf_end - buf);
      uint32_t copy_len = len;
      uint32_t remaining = 0U;
      ret = STATUS_INPROGRESS;
      if (post_data.content_length > post_data.accum_length) {
        remaining = post_data.content_length - post_data.accum_length;
      }
      if (copy_len > remaining) {
        copy_len = remaining;
      }
      if ((post_data.accum_length + copy_len) >= sizeof(post_data.content)) {
        copy_len = sizeof(post_data.content) - 1U - post_data.accum_length;
      }
      if (copy_len > 0U) {
        memcpy(post_data.content + post_data.accum_length, buf, copy_len);
        post_data.accum_length += copy_len;
      }
      post_data.content[post_data.accum_length] = 0;
      if (ret == STATUS_INPROGRESS) {
        if (post_data.accum_length >= post_data.content_length) {
          if (osSemaphoreAcquire(httpdbufSemaphore, 100) == osOK) {
            memset(html, 0, HTML_LEN);
            length_html = 0;
            //length_html = sprintf(html,"%s%s%s%s", PAGE_HEADER_200_OK, PAGE_HEADER_SERVER, PAGE_HEADER_CONNECTION_CLOSE, PAGE_HEADER_CONTENT_TEXT);
            //netconn_write(conn, (const unsigned char*)html, length_html, NETCONN_COPY);
            
            send_response(conn);
            
            osSemaphoreRelease(httpdbufSemaphore);
          }
          ret = STATUS_DONE;
          post_data.state = POST_STATE_HEADER;
        }
      }
      buf = 0;
      break;
    }
    }
  } // while (buf && buf < buf_end)
  return ret;
}

/**
 * @brief initialization
 *
 *
 * detailed description
 */
static int fwupdate_state_machine(struct netconn* conn, char* buf, u16_t buflen) {
  char* buf_start = buf;
  char* buf_end = buf + buflen;
  int ret = STATUS_NONE;
  while (buf && buf < buf_end) {
    if ((buflen >= 12) && (strncmp(buf, "POST /upload", 12) == 0)) {
      memset(&fwupdate, 0, sizeof(fwupdate_t));
      fwupdate.state = FWUPDATE_STATE_HEADER;
    }
    switch (fwupdate.state) {
    case FWUPDATE_STATE_HEADER: {
        if ((buflen >= 12) && (strncmp(buf, "POST /upload", 12) == 0)) {
          ret = STATUS_ERROR;
          char* tmp = buf;
          buf = strstr(buf, CONTENT_LENGTH_TAG);
          if (buf) {
            buf += strlen(CONTENT_LENGTH_TAG);
            fwupdate.content_length = atoi(buf);
            if (fwupdate.content_length > 0) {
              buf = strstr(tmp, BOUNDARY_TAG);
              buf += strlen(BOUNDARY_TAG);
              char* boundary_end;              
              boundary_end = strstr(buf, "\r\n");              
              boundary = boundary_end - buf;
              buf = strstr(buf, EMPTY_LINE_TAG);
              if (buf) {
                buf += strlen(EMPTY_LINE_TAG);
                buf_start = buf;
                fwupdate.state = FWUPDATE_STATE_OCTET_START;
                ret = STATUS_INPROGRESS;
              }
            } else {
              buf = 0;
            }
          }
        } else {
          buf = 0;
        }
        break;
    } // case FWUPDATE_STATE_HEADER
    case FWUPDATE_STATE_OCTET_START: {
      fwupdate.state = FWUPDATE_STATE_HEADER;
      ret = STATUS_ERROR;
      
      buf = strstr(buf, OCTET_STREAM_TAG);
      if (buf) {
        buf += strlen(OCTET_STREAM_TAG);
        uint16_t multipart_length = buf - buf_start;
        fwupdate.file_length = fwupdate.content_length - multipart_length - boundary - 8;
        if (fwupdate.file_length == PARTITION_SIZE + 4) {
          fwupdate.state = FWUPDATE_STATE_OCTET_STREAM;
          fwupdate.accum_length = 0;
          fwupdate.accum_buf_len = 0;
          ret = STATUS_INPROGRESS;          
          // init flash
          HAL_FLASH_Unlock();
          FlashWriteAddress = PARTITION_2_ADDR;

#ifdef DEBUG_FW_UPDATE
            status_e = FLASH_If_Erase(FlashWriteAddress, 7);
#else
            FLASH_If_Erase(FlashWriteAddress, 7);
#endif

          //end_address = fwupdate.addr + length_partition;
          end_address = FlashWriteAddress + PARTITION_SIZE;
        }
      }
      break;
    } // case FWUPDATE_STATE_OCTET_START
    case FWUPDATE_STATE_OCTET_STREAM: {
      int len = buf_end - buf;
      ret = STATUS_INPROGRESS;

#ifdef DEBUG_FW_UPDATE
        if (fwupdate.accum_length == 0) {
          memcpy(start_bin, buf, 16);
        }
#endif

      if (fwupdate.accum_length + len > fwupdate.file_length) {
        len = fwupdate.file_length - fwupdate.accum_length;
      }
      // 
      if (fwupdate.accum_buf_len > 0) {
        uint32_t j = 0;
        while (fwupdate.accum_buf_len <= 3) {
          if (len > (j + 1)) {
            fwupdate.accum_buf[fwupdate.accum_buf_len++] = *(buf + j);
          } else {
            fwupdate.accum_buf[fwupdate.accum_buf_len++] = 0xFF;
          }
          j++;
        }

#ifdef DEBUG_FW_UPDATE
          //status_w = FLASH_If_Write(&fwupdate.addr, end_address, (uint32_t*)(fwupdate.accum_buf), 1);
          status_w = FLASH_If_Write(&FlashWriteAddress, end_address, (uint32_t*)(fwupdate.accum_buf), 1);
#else
          FLASH_If_Write(&FlashWriteAddress, end_address, (uint32_t*)(fwupdate.accum_buf), 1);
#endif

        fwupdate.accum_length += 4;
        fwupdate.accum_buf_len = 0;

#ifdef DEBUG_FW_UPDATE
          memset((void *)fwupdate.accum_buf, 0, 4);
#endif

        // update data pointer
        buf = (char*)(buf + j);
        len = len - j;
      }      
      uint32_t count = len / 4;
      uint32_t i = len % 4;
      if (i > 0) {        
        // store bytes in accum_buf
        fwupdate.accum_buf_len = 0;

#ifdef DEBUG_FW_UPDATE
          memset((void *)fwupdate.accum_buf, 0, 4);
#endif

        for(; i > 0; i--) {
          fwupdate.accum_buf[fwupdate.accum_buf_len++] = *(char*)(buf + len - i);
        }
      } 

#ifdef DEBUG_FW_UPDATE
        //status_w = FLASH_If_Write(&fwupdate.addr, end_address, (uint32_t*)buf, count);
        status_w = FLASH_If_Write(&FlashWriteAddress, end_address, (uint32_t*)buf, count);
#else
        FLASH_If_Write(&FlashWriteAddress, end_address, (uint32_t*)buf, count);
#endif

      fwupdate.accum_length += count * 4;
      
      if (ret == STATUS_INPROGRESS) {
        if (fwupdate.accum_length == fwupdate.file_length) {

#ifdef DEBUG_FW_UPDATE
            memcpy(stop_bin, buf + len - 16, 16);
#endif

          uint32_t sum = hw_crc32((unsigned char*)PARTITION_2_ADDR, PARTITION_SIZE);
          uint32_t crc = (*(__IO uint32_t *)PARTITION_2_CRC_ADDR);

          char send_status[12] = {0};

          if(sum == crc) {
            uint32_t part = 1;
            taskENTER_CRITICAL();
            FlashWORD(PARTITION_SELECT_ADDR, (uint32_t *)&part, sizeof(part));
            taskEXIT_CRITICAL();
            sprintf((char*)(send_status), "{\"crc\":\"%d\"}", 1);
            fwupdate_send_success(conn, send_status);
            ret = STATUS_DONE;
          } else {
            sprintf((char*)(send_status), "{\"crc\":\"%d\"}", 0);
            fwupdate_send_success(conn, send_status);
            ret = STATUS_ERROR;
          }
          HAL_FLASH_Lock();
          fwupdate.state = FWUPDATE_STATE_HEADER;
        }
      }
      buf = 0;
      break;
    } // case FWUPDATE_STATE_OCTET_STREAM
    } // switch (fwupdate.state)
  } // while (buf && buf < buf_end)
  return ret;
}

/**
 * @brief Timeout and reboot
 *
 */
void timeoutCallback(void *argument) {
  (void) argument;
  NVIC_SystemReset();
}

/**
 * @brief 
 *
 */
void close_conn(struct netconn *conn) {
  if (conn != NULL) {
    netconn_close(conn);
  }
  netconn_delete(conn);
  conn = NULL;
}

void authorization_process(struct netconn *conn, const char *buf, const char *url) {
  struct fs_file file;
  if (strstr(buf, AUTHORIZATION_TAG) > 0 && strstr(buf, "YWRtaW46YWRtaW4=") > 0) {
    fs_open(&file, url);
    netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
    fs_close(&file);
  } else {
    if (osSemaphoreAcquire(httpdbufSemaphore, 100) == osOK) {
      memset(html, 0, HTML_LEN);
      length_html = sprintf((char*)html, "%s%s%s%s", PAGE_HEADER_REQUIRED, PAGE_HEADER_BASIC, PAGE_HEADER_CONNECTION_CLOSE, PAGE_HEADER_CONTENT_TEXT);
      netconn_write(conn, (const unsigned char*)html, length_html, NETCONN_COPY);
      osSemaphoreRelease(httpdbufSemaphore);
    }
  }
}

/**
 * @brief Routing http data
 *
 */

static void uart_test_start(struct netconn *conn) {
  if (osSemaphoreAcquire(httpdbufSemaphore, 100) == osOK) {
    uart_test_request = 1;
    uart_test_ready = 0;
    memset(html, 0, HTML_LEN);
    length_html = sprintf((char *)html, "{\"started\":1}");
    send_response(conn);
    osSemaphoreRelease(httpdbufSemaphore);
  }
}

static void uart_test_result(struct netconn *conn) {
  if (osSemaphoreAcquire(httpdbufSemaphore, 100) == osOK) {
    memset(html, 0, HTML_LEN);
    length_html = sprintf((char *)html, "{\"ready\":%d,\"ok\":%d,\"message\":\"%s\"}", uart_test_ready, uart_test_ok, uart_test_message);
    send_response(conn);
    osSemaphoreRelease(httpdbufSemaphore);
  }
}
static void ktv_start(struct netconn *conn) {
  if (osSemaphoreAcquire(httpdbufSemaphore, 100) == osOK) {
    uint8_t started = KTV_RequestPoll();

    memset(html, 0, HTML_LEN);
    length_html = (uint16_t)snprintf((char *)html, HTML_LEN, "{\"started\":%u}", started);
    send_response(conn);
    osSemaphoreRelease(httpdbufSemaphore);
  }
}

static void stage_ktv_state(struct netconn *conn) {
  if (osSemaphoreAcquire(httpdbufSemaphore, 100) == osOK) {
    tsKtvWebSnapshot snapshot;
    uint16_t i;

    KTV_GetSnapshot(&snapshot);

    memset(html, 0, HTML_LEN);
    length_html = (uint16_t)snprintf(
      (char *)html,
      HTML_LEN,
      "{\"busy\":%u,\"ready\":%u,\"init\":%u,\"line\":%u,\"state\":%u,\"pending\":%u,\"poll_counter\":%lu,\"items\":[",
      snapshot.busy,
      snapshot.ready,
      snapshot.init_pin,
      snapshot.line_pin,
      snapshot.state,
      snapshot.poll_pending,
      (unsigned long)snapshot.poll_counter
    );

    for (i = 0U; i <= KTV_NUM_MAX; ++i) {
      int written = snprintf(
        (char *)(html + length_html),
        HTML_LEN - length_html,
        "%s{\"id\":%u,\"u\":%u,\"f\":%u,\"b\":%u,\"o\":%u,\"g\":%u,\"a\":%u}",
        (i == 0U) ? "" : ",",
        i,
        snapshot.items[i].used,
        snapshot.items[i].flags,
        snapshot.items[i].before,
        snapshot.items[i].online,
        snapshot.items[i].gap,
        snapshot.items[i].after
      );

      if ((written < 0) || ((uint16_t)written >= (HTML_LEN - length_html))) {
        break;
      }
      length_html += (uint16_t)written;
    }

    if (length_html < (HTML_LEN - 2U)) {
      length_html += (uint16_t)snprintf((char *)(html + length_html), HTML_LEN - length_html, "]}");
    }

    send_response(conn);
    osSemaphoreRelease(httpdbufSemaphore);
  }
}


static void http_server(struct netconn *conn) {
  struct netbuf *inbuf = NULL;
  char* buf;
  uint16_t buflen;
  struct fs_file file;

  char tmp[16];
  err_t err;
  bool close_connection = false;
/*  
  int keep_alive = 0;
  int keep_idle = 2;
  int keep_interval = 2;
  int keep_count = 2;
  
  setsockopt(conn->socket, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(keep_alive));
  setsockopt(conn->socket, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle));
  setsockopt(conn->socket, IPPROTO_TCP, TCP_KEEPINTVL, &keep_interval, sizeof(keep_interval));
  setsockopt(conn->socket, IPPROTO_TCP, TCP_KEEPCNT, &keep_count, sizeof(keep_count));
*/
  netconn_set_recvtimeout(conn, 10000);
  
  // Read the data from the port, blocking if nothing yet there. 
  // We assume the request (the part we care about) is in one netbuf
  err = netconn_recv(conn, &inbuf);
  
  do {
    err = netconn_err(conn);
    if (err == ERR_OK) {      
      recv_count++;
      do {
        bool request_in_progress = false;

        netbuf_data(inbuf, (void**)&buf, &buflen);
        // Is this an HTTP GET command?
        if ((buflen >= 5) && (strncmp(buf, "GET /", 5) == 0)) {
          if ((strncmp(buf, "GET / ", 6) == 0) || (strncmp(buf, "GET /index.html", 15) == 0)) {
            fs_open(&file, "/index.html");
            netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
            fs_close(&file);
          }
          else if((strncmp(buf, "GET /index.js", 13) == 0)) {
            fs_open(&file, "/index.js");
            netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
            fs_close(&file);
          }
          else if (strncmp(buf, "GET /tuning.html", 16) == 0) {
            authorization_process(conn, buf, "/tuning.html");
          }
          else if (strncmp(buf, "GET /serial.html", 16) == 0) {
            authorization_process(conn, buf, "/serial.html");
          }
          else if((strncmp(buf, "GET /tuning.js", 14) == 0)) {
            fs_open(&file, "/tuning.js");
            netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
            fs_close(&file);
          }
          else if (strncmp(buf, "GET /set0.json", 14) == 0) {
            stage_set(conn, 0);
          } 
          else if (strncmp(buf, "GET /set1.json", 14) == 0) {
            tuning_set(conn);
          }
          else if (strncmp(buf, "GET /serial.json", 16) == 0) {
            stage_serial(conn);
          }
          else if (strncmp(buf, "GET /info.json", 14) == 0) {
            status_sys(conn);
          }
          else if (strncmp(buf, "GET /pins.json", 14) == 0) {
            stage_pins(conn);
          }
          else if (strncmp(buf, "GET /keyboard_state.json", 24) == 0) {
            stage_keyboard_state(conn);
          }
          else if (strncmp(buf, "GET /ktv_start", 14) == 0) {
            ktv_start(conn);
          }
          else if (strncmp(buf, "GET /ktv_state.json", 19) == 0) {
            stage_ktv_state(conn);
          }
          else if (strncmp(buf, "GET /uart_test_start", 20) == 0) {
            uart_test_start(conn);
          }
          else if (strncmp(buf, "GET /uart_test_result", 21) == 0) {
            uart_test_result(conn);
          }
          else if (strncmp(buf, "GET /info.html", 14) == 0) {
            fs_open(&file, "/info.html");
            netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
            fs_close(&file);
          }
          else if ((strncmp(buf, "GET /keyboard.html", 18) == 0) || (strncmp(buf, "GET /\\keyboard.html", 19) == 0)) {
            if (fs_open(&file, "/keyboard.html") == ERR_OK) {
              netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
              fs_close(&file);
            } else {
              fs_open(&file, "/404.html");
              netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
              fs_close(&file);
            }
          }
          else if (strncmp(buf, "GET /ota.html", 13) == 0) {
            fs_open(&file, "/ota.html");
            netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
            fs_close(&file);
          }
          else {
            // Load Error page
            fs_open(&file, "/404.html");
            netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_COPY);
            fs_close(&file);
          }

          close_connection = true;
        }
        // Is this an HTTP POST command?
        else {
          int ret = fwupdate_state_machine(conn, buf, buflen);
          if (ret == STATUS_NONE) {
            // ignore
          } else if (ret == STATUS_INPROGRESS) {
            // Don't close the connection!
            request_in_progress = true;
            netconn_set_recvtimeout(conn, 10000);
            //tcp_nagle_disable(conn->pcb.tcp);
          } else {
            // Some result, we should close the connection now

            if (ret == STATUS_DONE) {
              // reboot after we close the connection              
              osTimerStart(timerTimeout, 5000);
            }
          }
          
          if (fwupdate.state != FWUPDATE_STATE_OCTET_STREAM) {
            ret = post_state_machine(conn, buf, buflen);
            if (ret == STATUS_NONE) {
              // ignore
            } else if (ret == STATUS_INPROGRESS) {
              // Don't close the connection!
              request_in_progress = true;
            } else {
              // Some result, we should close the connection now

              if (ret == STATUS_DONE) {
                // POST /settings
                if (post_data.url == 2) {
                  jsmn_parser p;
                  memset(token, 0, sizeof(jsmntok_t)*TOK_LENGTH);
                  jsmn_init(&p);
                  // parsing response in format json
                  int count = jsmn_parse(&p, post_data.content, strlen(post_data.content), token, TOK_LENGTH);
#ifdef DEBUG
                  WM_HTTPD = uxTaskGetStackHighWaterMark(NULL);
#endif
                  if (count > 0) {
                    for (uint8_t i = 1; i < count; i++) {
                      if (jsoneq(post_data.content, &token[i], "usk_ip") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.self_ip_addr.addr = ipaddr_addr(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "usk_mask") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.self_mask_addr.addr = ipaddr_addr(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "usk_gateway") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.self_gateway_addr.addr = ipaddr_addr(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "self_id") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.self_id = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "mb_rate") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.mb_rate = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "mb_timeout") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.mb_timeout = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "sens1_id") == 0) { // 2
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.sens1_id = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "sens2_id") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.sens2_id = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "sens2_min_val") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.sens2_min_val = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "sens2_max_val") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.sens2_max_val = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "sens3_min_lim") == 0) { // 3
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.sens3_min_lim = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "sens3_max_lim") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.sens3_max_lim = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "sens3_min_val") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.sens3_min_val = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "sens3_max_val") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.sens3_max_val = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "urm_id") == 0) { // 4
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.urm_id = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "valve1_io") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.valve1_io = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "valve2_io") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.valve2_io = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "state_out_io") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.state_out_io = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "umvh_id") == 0) { // 5
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.umvh_id = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "sens3_io") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.sens3_io = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "state_in_io") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        settings.state_in_io = atoi(tmp);
                      }
                    } // for (uint8_t i = 1; i < count; i++)

#ifdef DEBUG_FW_UPDATE
                      // sector 14
                      status_w = FlashWORD(CONFIGURATION_ADDR, (uint32_t *)&settings, sizeof(settings_t));
#else
                      //FlashWORD(CONFIGURATION_ADDR, (uint32_t *)&settings, sizeof(settings_t));
                    if (osSemaphoreAcquire(flash_semaphore, 5) == osOK) {
                      settings.crc32 = hw_crc32((uint8_t *)&settings, sizeof(settings_t) - 4);
                      Flash_WriteStruct(&settings, CONFIGURATION_SECTOR, sizeof(settings_t), CONFIGURATION_SECTOR_SIZE);
                      osSemaphoreRelease(flash_semaphore);
                    }
#endif

                    //configuration_change = true;
                    osTimerStart(timerTimeout, 5000);
                  }
                } // if (post_data.url == 2)
                // POST /serial
                if (post_data.url == 3) {
                  jsmn_parser p;
                  memset(token, 0, sizeof(jsmntok_t)*TOK_LENGTH);
                  jsmn_init(&p);
                  int count = jsmn_parse(&p, post_data.content, strlen(post_data.content), token, TOK_LENGTH);
                  if (count > 0) {
                    for (uint8_t i = 1; i < count; i++) {
                      if (jsoneq(post_data.content, &token[i], "serial") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        identification.serial_number = atoi(tmp);                        
                      } else if (jsoneq(post_data.content, &token[i], "mac0") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        // string hex value to int
                        identification.mac_octet_6 = strtol(tmp, NULL, 16);
                      } else if (jsoneq(post_data.content, &token[i], "mac1") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        // string hex value to int
                        identification.mac_octet_5 = strtol(tmp, NULL, 16);
                      }
                    } // for (uint8_t i = 1; i < count; i++) {

#ifdef DEBUG_FW_UPDATE
                      // sector 12
                      status_w = FlashWORD(IDENTIFICATION_ADDR, (uint32_t *)&identification, sizeof(identification_t));  
#else
                      FlashWORD(IDENTIFICATION_ADDR, (uint32_t *)&identification, sizeof(identification_t));                   
#endif                    

                    osTimerStart(timerTimeout, 5000);
                  }
                } // if (post_data.url == 3)
                // POST /tuning
                if (post_data.url == 4) {                  
                  jsmn_parser p;
                  memset(token, 0, sizeof(jsmntok_t)*TOK_LENGTH);
                  jsmn_init(&p);
                  int count = jsmn_parse(&p, post_data.content, strlen(post_data.content), token, TOK_LENGTH);
                  if (count > 0) {
                    for (uint8_t i = 0; i < count; i++) {
                      if (jsoneq(post_data.content, &token[i], "hysteresis_window") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        global_tuning.hysteresis_window = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "self_test_duration") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        global_tuning.self_test_duration = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "min_stroke_rod") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        global_tuning.min_stroke_rod = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "min_stroke") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        global_tuning.min_stroke = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "adc_min_stroke") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        global_tuning.adc_min_stroke = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "max_stroke") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        global_tuning.max_stroke = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "adc_max_stroke") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        global_tuning.adc_max_stroke = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "zero_offset") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        global_tuning.zero_offset = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "debounce_in") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        global_tuning.debounce_in = atoi(tmp);
                      } else if (jsoneq(post_data.content, &token[i], "delay_out") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        global_tuning.delay_out = atoi(tmp);
                      }
                    } // for (uint8_t i = 0; i < count; i++)

#ifdef DEBUG_FW_UPDATE
                      status_w = FlashWORD(STATE_ADDR, (uint32_t *)&global_tuning, sizeof(tuning_t));
#else
                      FlashWORD(STATE_ADDR, (uint32_t *)&global_tuning, sizeof(tuning_t));
#endif

                  }
                } // if (post_data.url == 4)
                // POST /pins_control
                if (post_data.url == 5) {
                  jsmn_parser p;
                  char pin_name[24] = {0};
                  uint8_t pin_value = 0U;
                  bool has_pin_name = false;
                  bool has_pin_value = false;
                  bool has_manual_mode = false;
                  uint8_t manual_mode_value = 0U;
                  bool has_relay_test_mode = false;
                  uint8_t relay_test_mode_value = 0U;
                  bool has_relay_test_drive = false;
                  uint8_t relay_test_drive_value = 0U;
                  uint16_t pin_name_len = 0U;

                  memset(token, 0, sizeof(jsmntok_t) * TOK_LENGTH);
                  jsmn_init(&p);
                  int count = jsmn_parse(&p, post_data.content, strlen(post_data.content), token, TOK_LENGTH);
                  if (count > 0) {
                    for (uint8_t i = 1; i < count; i++) {
                      if (jsoneq(post_data.content, &token[i], "manual_mode") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        manual_mode_value = (atoi(tmp) != 0) ? 1U : 0U;
                        has_manual_mode = true;
                      } else if (jsoneq(post_data.content, &token[i], "relay_test_mode") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        relay_test_mode_value = (atoi(tmp) != 0) ? 1U : 0U;
                        has_relay_test_mode = true;
                      } else if (jsoneq(post_data.content, &token[i], "relay_test_drive") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        relay_test_drive_value = (atoi(tmp) != 0) ? 1U : 0U;
                        has_relay_test_drive = true;
                      } else if (jsoneq(post_data.content, &token[i], "pin") == 0) {
                        memset((void *)pin_name, 0, sizeof(pin_name));
                        pin_name_len = (uint16_t)(token[i + 1].end - token[i + 1].start);
                        if (pin_name_len >= sizeof(pin_name)) {
                          pin_name_len = sizeof(pin_name) - 1U;
                        }
                        memcpy((void *)pin_name, (void *)(post_data.content + token[i + 1].start), pin_name_len);
                        pin_name[pin_name_len] = 0;
                        has_pin_name = true;
                      } else if (jsoneq(post_data.content, &token[i], "value") == 0) {
                        memset((void *)tmp, 0, 16);
                        memcpy((void *)tmp, (void *)(post_data.content + token[i + 1].start), token[i + 1].end - token[i + 1].start);
                        pin_value = (atoi(tmp) != 0) ? 1U : 0U;
                        has_pin_value = true;
                      }
                    }

                    if (has_relay_test_mode) {
                      relay_test_set_mode(relay_test_mode_value);
                    }

                    if (has_relay_test_drive) {
                      relay_test_drive(relay_test_drive_value);
                    }

                    if (relay_test_mode == 0U && has_manual_mode) {
                      manual_pins_mode = manual_mode_value;
                    }

                    if (relay_test_mode == 0U && manual_pins_mode != 0U && has_pin_name && has_pin_value) {
                      manual_pin_write_by_name(pin_name, pin_value);
                    }
                  }
                } // if (post_data.url == 5)
              } // (ret == POST_STATUS_DONE)
            }
          }

          if (!request_in_progress) {
            close_connection = true;
          }
        }
#ifdef DEBUG
        WM_HTTPD = uxTaskGetStackHighWaterMark(NULL);
#endif
      } while (!close_connection && (netbuf_next(inbuf) >= 0));
      //
      netbuf_delete(inbuf);
      inbuf = NULL;
      if (!close_connection) {
        err = netconn_recv(conn, &inbuf);
      }
    }
  } while ((err == ERR_OK) && !close_connection);
  if (inbuf != NULL) {
    netbuf_delete(inbuf);
  }
}

/**
 * @brief Function for handling TCP connections of HTTP server on port 80
 *
 */
void http_server_thread(void *argument) {
  (void) argument;
  // reset after timer expires
  timerTimeout = osTimerNew(timeoutCallback, osTimerOnce, (void *)0, NULL);
  //
  httpdbufSemaphore = osSemaphoreNew(1, 1, NULL);
  // set a limit on the number of connections no more than 1
  httpdSemaphore = osSemaphoreNew(1, 1, NULL);
  struct netconn *conn;
  struct netconn *newconn;
  err_t err;
  
#ifdef DEBUG_NETCONN  
  error_netconn_accept = 0;
#endif
  
  while (1) {
    conn = netconn_new(NETCONN_TCP);
    if (conn != NULL) {
      if (netconn_bind(conn, NULL, 80) == ERR_OK) {
        if (netconn_listen(conn) == ERR_OK) {
          netconn_set_recvtimeout(conn, 1);
          recv_count = 0;
#ifdef DEBUG
          WM_HTTPD = uxTaskGetStackHighWaterMark(NULL);
#endif
          do {
            if (osSemaphoreAcquire(httpdSemaphore, 50) == osOK) {
              err = netconn_accept(conn, &newconn);              
              if (err == ERR_OK) {
                http_server(newconn);
                close_conn(newconn);                
              }
              osSemaphoreRelease(httpdSemaphore);
            } else {
              DELAY_MS(50);
            }
          } while (err != ERR_CLSD);
#ifdef DEBUG_NETCONN
          error_netconn_accept++;
#endif
        }
      }     
    }
    close_conn(conn);
    DELAY_MS(50);
  }
}
