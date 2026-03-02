#ifndef __HTTPSERVER_NETCONN_H__
#define __HTTPSERVER_NETCONN_H__

#include <stdint.h>

void http_server_thread(void *argument);
void uart_test_request_start(void);
uint8_t uart_test_get_status(uint8_t *running, uint8_t *done, uint8_t *success, const char **message);

#endif /* __HTTPSERVER_NETCONN_H__ */
