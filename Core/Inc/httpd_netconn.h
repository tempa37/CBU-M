#ifndef __HTTPSERVER_NETCONN_H__
#define __HTTPSERVER_NETCONN_H__

#include <stdint.h>

typedef struct {
  uint8_t state;
  uint8_t success;
  uint8_t step;
  uint8_t error;
  uint32_t started_tick;
  uint32_t finished_tick;
} uart_test_result_t;

void http_server_thread(void *argument);
void uart_test_request_start(void);
void uart_test_get_result(uart_test_result_t *result);

#endif /* __HTTPSERVER_NETCONN_H__ */
