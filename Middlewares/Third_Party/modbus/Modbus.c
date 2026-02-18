#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"
#include "main.h"
#include "Modbus.h"
#include "timers.h"
#include "semphr.h"

#include "global_types.h"

#if ENABLE_TCP == 1
#include "api.h"
#include "ip4_addr.h"
#include "netif.h"
#include <string.h>
#endif

#ifndef ENABLE_USART_DMA
#define ENABLE_USART_DMA 0
#endif

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

#define lowByte(w) ((w) & 0xff)
#define highByte(w) ((w) >> 8)

modbusHandler_t *mHandlers[MAX_M_HANDLERS];

// Queue Modbus telegrams for master
const osMessageQueueAttr_t QueueTelegram_attributes = {
  .name = "QueueModbusTelegram"
};

const osThreadAttr_t myTaskModbusA_attributes = {
  .name = "TaskModbusSlave",
  .priority = (osPriority_t) osPriorityHigh,
  .stack_size = 128 * 4
};

const osThreadAttr_t myTaskModbusA_attributesTCP = {
  .name = "TaskModbusSlave",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 14
};

// Task Modbus Master
// osThreadId_t myTaskModbusAHandle;
const osThreadAttr_t myTaskModbusB_attributes = {
  .name = "TaskModbusMaster",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 5
};

const osThreadAttr_t myTaskModbusB_attributesTCP = {
  .name = "TaskModbusMaster",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 14
};

// Semaphore to access the Modbus Data
const osSemaphoreAttr_t ModBusSphr_attributes = {
  .name = "ModBusSphr"
};

uint8_t numberHandlers = 0;

static void sendTxBuffer(modbusHandler_t *modH);
static int16_t getRxBuffer(modbusHandler_t *modH);
static uint8_t validateAnswer(modbusHandler_t *modH);
static void buildException( uint8_t u8exception, modbusHandler_t *modH );
static uint8_t validateRequest(modbusHandler_t * modH);
static uint16_t word(uint8_t H, uint8_t l);
static void get_FC1(modbusHandler_t *modH);
static void get_FC3(modbusHandler_t *modH);
static int8_t process_FC1(modbusHandler_t *modH );
static int8_t process_FC3(modbusHandler_t *modH );
static int8_t process_FC5( modbusHandler_t *modH);
static int8_t process_FC6(modbusHandler_t *modH );
static int8_t process_FC15(modbusHandler_t *modH );
static int8_t process_FC16(modbusHandler_t *modH);
static void vTimerCallbackT35(TimerHandle_t *pxTimer);
static void vTimerCallbackTimeout(TimerHandle_t *pxTimer);
static int8_t SendQuery(modbusHandler_t *modH ,  modbus_t telegram);

#if ENABLE_TCP == 1

static bool TCPwaitConnData(modbusHandler_t *modH);
static uint8_t TCPinitserver(modbusHandler_t *modH);
static mb_error_t TCPconnectserver(modbusHandler_t * modH, modbus_t *telegram);
static mb_error_t TCPgetRxBuffer(modbusHandler_t * modH);

#endif

// Ring Buffer functions

// This function must be called only after disabling USART RX interrupt or inside of the RX interrupt
void RingAdd(modbusRingBuffer_t *xRingBuffer, uint8_t u8Val) {
  xRingBuffer->uxBuffer[xRingBuffer->u8end] = u8Val;
  xRingBuffer->u8end = (xRingBuffer->u8end + 1) % MAX_BUFFER;
  if (xRingBuffer->u8available == MAX_BUFFER) {
    xRingBuffer->overflow = true;
    xRingBuffer->u8start = (xRingBuffer->u8start + 1) % MAX_BUFFER;
  } else {
    xRingBuffer->overflow = false;
    xRingBuffer->u8available++;
  }
}

// This function must be called only after disabling USART RX interrupt
uint8_t RingGetAllBytes(modbusRingBuffer_t *xRingBuffer, uint8_t *buffer) {
  return RingGetNBytes(xRingBuffer, buffer, xRingBuffer->u8available);
}

// This function must be called only after disabling USART RX interrupt
uint8_t RingGetNBytes(modbusRingBuffer_t *xRingBuffer, uint8_t *buffer, uint8_t uNumber) {
  uint8_t uCounter;
  if (xRingBuffer->u8available == 0 || uNumber == 0 ) return 0;
  if (uNumber > MAX_BUFFER) return 0;
  
  for (uCounter = 0; uCounter < uNumber && uCounter < xRingBuffer->u8available; uCounter++) {
    buffer[uCounter] = xRingBuffer->uxBuffer[xRingBuffer->u8start];
    xRingBuffer->u8start = (xRingBuffer->u8start + 1) % MAX_BUFFER;
  }
  
  xRingBuffer->u8available = xRingBuffer->u8available - uCounter;
  xRingBuffer->overflow = false;
  RingClear(xRingBuffer);
  
  return uCounter;
}

uint8_t RingCountBytes(modbusRingBuffer_t *xRingBuffer) {
  return xRingBuffer->u8available;
}

void RingClear(modbusRingBuffer_t *xRingBuffer) {
  xRingBuffer->u8start = 0;
  xRingBuffer->u8end = 0;
  xRingBuffer->u8available = 0;
  xRingBuffer->overflow = false;
}

// End of Ring Buffer functions

const unsigned char fctsupported[] = {
  MB_FC_READ_COILS,
  MB_FC_READ_DISCRETE_INPUT,
  MB_FC_READ_REGISTERS,
  MB_FC_READ_INPUT_REGISTER,
  MB_FC_WRITE_COIL,
  MB_FC_WRITE_REGISTER,
  MB_FC_WRITE_MULTIPLE_COILS,
  MB_FC_WRITE_MULTIPLE_REGISTERS
};

/**
 * @brief
 * Initialization for a Master/Slave.
 * this function will check the configuration parameters
 * of the modbus handler
 *
 * @param modH   modbus handler
 */
void ModbusInit(modbusHandler_t * modH) {
  if (numberHandlers < MAX_M_HANDLERS) {
    // Initialize the ring buffer
    RingClear(&modH->xBufferRX);
    if (modH->uModbusType == MB_SLAVE) {
      // Create Modbus task slave
#if ENABLE_TCP == 1
      if (modH->xTypeHW == TCP_HW) {
        modH->myTaskModbusAHandle = osThreadNew(StartTaskModbusSlave, modH, &myTaskModbusA_attributesTCP);
      } else {
        modH->myTaskModbusAHandle = osThreadNew(StartTaskModbusSlave, modH, &myTaskModbusA_attributes);
      }
#else
      modH->myTaskModbusAHandle = osThreadNew(StartTaskModbusSlave, modH, &myTaskModbusA_attributes);
#endif
    } else if (modH->uModbusType == MB_MASTER) {
      // Create Modbus task Master and Queue for telegrams      
#if ENABLE_TCP == 1
      if (modH->xTypeHW == TCP_HW) {
        modH->myTaskModbusAHandle = osThreadNew(StartTaskModbusMaster, modH, &myTaskModbusB_attributesTCP);
      } else {
        modH->myTaskModbusAHandle = osThreadNew(StartTaskModbusMaster, modH, &myTaskModbusB_attributes);
      }
#else
      modH->myTaskModbusAHandle = osThreadNew(StartTaskModbusMaster, modH, &myTaskModbusB_attributes);
#endif
      
      // Just a text name, not used by the kernel.
      // The timer period in ticks.
      // The timers will auto-reload themselves when they expire.
      // Assign each timer a unique id equal to its array index.
      // Each timer calls the same callback when it expires.
      modH->xTimerTimeout = xTimerCreate("xTimerTimeout", modH->sendtimeout, pdFALSE, (void*)modH->xTimerTimeout, (TimerCallbackFunction_t)vTimerCallbackTimeout);
      if (modH->xTimerTimeout == NULL) {
        // error creating timer, check heap and stack size
        //while(1);
        Error_Handler();
      }
      modH->QueueTelegramHandle = osMessageQueueNew(MAX_TELEGRAMS, sizeof(modbus_t), &QueueTelegram_attributes);
      if (modH->QueueTelegramHandle == NULL) {
        // error creating queue for telegrams, check heap and stack size
        //while(1);
        Error_Handler();
      }
    } else {
      // error Modbus type not supported choose a valid Type
      //while(1);
      Error_Handler();
    }
    if (modH->myTaskModbusAHandle == NULL) {
      // error creating Modbus task, check heap and stack size
      //while(1);
      Error_Handler();
    }
    // Just a text name, not used by the kernel.
    // The timer period in ticks.
    // The timers will auto-reload themselves when they expire.
    // Assign each timer a unique id equal to its array index.
    // Each timer calls the same callback when it expires.
    modH->xTimerT35 = xTimerCreate("TimerT35", T35, pdFALSE, (void*)modH->xTimerT35, (TimerCallbackFunction_t)vTimerCallbackT35);
    if (modH->xTimerT35 == NULL) {
      while(1); // error creating the timer, check heap and stack size
    }
    modH->ModBusSphrHandle = osSemaphoreNew(1, 1, &ModBusSphr_attributes);
    if (modH->ModBusSphrHandle == NULL) {
      while(1); // error creating the semaphore, check heap and stack size
    }
    mHandlers[numberHandlers] = modH;
    numberHandlers++;
  } else {
    // error no more Modbus handlers supported
    //while(1);
    Error_Handler();
  }
}

void set_timeout(modbusHandler_t* modH, uint16_t recv, uint16_t send) {  
  modH->recvtimeout = recv;
  modH->sendtimeout = send;
  if (modH->xTimerTimeout != NULL) {
    xTimerChangePeriod(modH->xTimerTimeout, pdMS_TO_TICKS(modH->sendtimeout), 5);
  }
}

/**
  * @brief Modbus query function
  * @param Modbus handler
  * @param Modbus telegram
  * @param Query retry count
  * @retval Result notification
  */
uint32_t modbus_query(modbusHandler_t* mbHandler, modbus_t* mbTelegram, uint8_t retry) {
  #define TIME_BETWEEN_ATTEMPTS 100
  // number of modbus connection attempts
  //uint8_t connection_attempts = retry;
  uint32_t notice;
  do {
    // execute Modbus request
    ModbusQuery(mbHandler, *mbTelegram);
    notice = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (notice != MB_ERR_OK) {
      retry--;
      if (retry == 0) {
        break;
      }
      if (notice != MB_ERR_OK) {
        osDelay(pdMS_TO_TICKS(TIME_BETWEEN_ATTEMPTS));
      }
    }
  } while (notice != MB_ERR_OK);
  
  return notice;
}

/**
 * @brief
 * Start object.
 *
 * Call this AFTER calling begin() on the serial port, typically within setup().
 *
 * (If you call this function, then you should NOT call any of
 * ModbusRtu's own begin() functions.)
 *
 * @ingroup setup
 */
void ModbusStart(modbusHandler_t *modH) {
  if (modH->xTypeHW != USART_HW && modH->xTypeHW != TCP_HW && modH->xTypeHW != USB_CDC_HW  && modH->xTypeHW != USART_HW_DMA) {
    // ERROR select the type of hardware
    //while(1); 
    Error_Handler();
  }
  if (modH->xTypeHW == USART_HW_DMA && ENABLE_USART_DMA == 0) {
    // ERROR To use USART_HW_DMA you need to enable it in the ModbusConfig.h file
    //while(1); 
    Error_Handler();
  }
  if (modH->xTypeHW == USART_HW || modH->xTypeHW == USART_HW_DMA) {
    if (modH->EN_Port != NULL ) {
      // return RS485 transceiver to transmit mode
      HAL_GPIO_WritePin(modH->EN_Port, modH->EN_Pin, GPIO_PIN_RESET);
    }
    if (modH->uModbusType == MB_SLAVE &&  modH->u16regs == NULL) {
      // ERROR define the DATA pointer shared through Modbus
      //while(1); 
      Error_Handler();
    }
    // check that port is initialized
    while(HAL_UART_GetState(modH->port) != HAL_UART_STATE_READY);
    
#if ENABLE_USART_DMA == 1
    if (modH->xTypeHW == USART_HW_DMA) {
      if (HAL_UARTEx_ReceiveToIdle_DMA(modH->port, modH->xBufferRX.uxBuffer, MAX_BUFFER) != HAL_OK) {
        // error in your initialization code
        //while(1);
        Error_Handler();
      }
      __HAL_DMA_DISABLE_IT(modH->port->hdmarx, DMA_IT_HT); // we don't need half-transfer interrupt
    } else {
      // Receive data from serial port for Modbus using interrupt
      if (HAL_UART_Receive_IT(modH->port, &modH->dataRX, 1) != HAL_OK) {
        // error in your initialization code
        //while(1);
        Error_Handler();
      }
    }
#else
    // Receive data from serial port for Modbus using interrupt
    if (HAL_UART_Receive_IT(modH->port, &modH->dataRX, 1) != HAL_OK) {
      // error in your initialization code
      //while(1);
      Error_Handler();
    }
#endif
    if (modH->u8id != 0 && modH->uModbusType == MB_MASTER) {
      // error Master ID must be zero
      //while(1);
      Error_Handler();
    }
    if (modH->u8id == 0 && modH->uModbusType == MB_SLAVE) {
      // error Master ID must be zero
      //while(1);
      Error_Handler();
    }
  }
  
  modH->lost = 0;

  modH->u8lastRec = modH->u8BufferSize = 0;
  modH->u16InCnt = modH->u16OutCnt = modH->u16ErrCnt = 0;  
}

#if ENABLE_USB_CDC == 1
extern void MX_USB_DEVICE_Init(void);
void ModbusStartCDC(modbusHandler_t * modH) {
  if (modH->uModbusType == MB_SLAVE &&  modH->u16regs == NULL ) {
    while(1); //ERROR define the DATA pointer shared through Modbus
  }  
  modH->u8lastRec = modH->u8BufferSize = 0;
  modH->u32InCnt = modH->u32OutCnt = modH->u32errCnt = 0;
}
#endif

void vTimerCallbackT35(TimerHandle_t *pxTimer) {
  // Notify that a stream has just arrived
  // TimerHandle_t aux;
  for (uint8_t i = 0; i < numberHandlers; i++) {
    if ((TimerHandle_t *)mHandlers[i]->xTimerT35 == pxTimer) {
      if (mHandlers[i]->uModbusType == MB_MASTER) {
        xTimerStop(mHandlers[i]->xTimerTimeout, 0);
      }
      xTaskNotify(mHandlers[i]->myTaskModbusAHandle, MB_ERR_OK, eSetValueWithOverwrite);
    }
  }
}

void vTimerCallbackTimeout(TimerHandle_t *pxTimer) {
  // Notify that a stream has just arrived
  // TimerHandle_t aux;
  for (uint8_t i = 0; i < numberHandlers; i++) {
    if ((TimerHandle_t *)mHandlers[i]->xTimerTimeout == pxTimer) {      
      xTaskNotify(mHandlers[i]->myTaskModbusAHandle, MB_ERR_TIMEOUT, eSetValueWithOverwrite);
    }
  }
}

#if ENABLE_TCP == 1

#include "socket.h"
#include "tcp.h"

#define ENABLE_KEEP_ALIVE 1
#define DISABLE_KEEP_ALIVE 0

osSemaphoreId_t modbus_semaphore;

uint32_t count_modbus_semaphore;

bool TCPwaitConnData(modbusHandler_t *modH) {
  struct netbuf *inbuf;
  err_t err;
  char* buf;
  uint16_t buflen;
  uint16_t uLength;
  bool xTCPvalid = false;
  tcpclients_t *clientconn;
  
  // select the next connection slot to work with using round-robin
  modH->newconnIndex++;
  if (modH->newconnIndex >= modH->num_tcp_conn) {
    modH->newconnIndex = 0;
  }
  
  //modH->newconnIndex = (modH->newconnIndex >= modH->num_tcp_conn) ? 0 : modH->newconnIndex + 1;
  
  clientconn = &modH->newconns[modH->newconnIndex];
  
  count_modbus_semaphore = osSemaphoreGetCount(modbus_semaphore);
  
  // NULL means there is a free connection slot, so we can accept an 
  // incoming client connection
  if (clientconn->conn == NULL) {
    if (osSemaphoreAcquire(modbus_semaphore, 50) == osOK) {
      // accept any incoming connection
      err = netconn_accept(modH->conn, &clientconn->conn);
    } else {
      return xTCPvalid = false;
    }
    if (err != ERR_OK) {
      if (osSemaphoreRelease(modbus_semaphore) != osOK) {
        modH->u16ErrUndef++;
      }
      // not valid incoming connection at this time
      ModbusCloseConnNull(modH);
      return xTCPvalid = false;
    } else {
      clientconn->aging = 0;
      
      netconn_set_recvtimeout(clientconn->conn, modH->recvtimeout);
      
      tcp_nagle_disable(clientconn->conn->pcb.tcp);

      int keep_alive = DISABLE_KEEP_ALIVE;
      // Send first keepalive probe after keep_idle seconds of inactivity
      int keep_idle = 5;
      // Sending subsequent keepalive probes after keep_interval seconds
      int keep_interval = 5;
      // Time out after keep_count unsuccessful keepalive probes
      int keep_count = 3;

      setsockopt(clientconn->conn->socket, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(keep_alive));
      setsockopt(clientconn->conn->socket, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle));
      setsockopt(clientconn->conn->socket, IPPROTO_TCP, TCP_KEEPINTVL, &keep_interval, sizeof(keep_interval));
      setsockopt(clientconn->conn->socket, IPPROTO_TCP, TCP_KEEPCNT, &keep_count, sizeof(keep_count));
    }
  }
  
  err = netconn_recv(clientconn->conn, &inbuf);
  // the connection was closed
  if (err == ERR_CLSD || err == ERR_ABRT || err == ERR_RST) {
    if (osSemaphoreRelease(modbus_semaphore) != osOK) {
      modH->u16ErrUndef++;
    }
    // delete the buffer always
    netbuf_delete(inbuf); 
    // Close and clean the connection
    ModbusCloseConnNull(modH);
    clientconn->aging = 0;
    return xTCPvalid = false;
  } else if (err == ERR_TIMEOUT) {
    // delete the buffer always
    netbuf_delete(inbuf);
    // continue the aging process
    modH->newconns[modH->newconnIndex].aging++;
    // if the connection is old enough and inactive close and clean it up
    //if (modH->newconns[modH->newconnIndex].aging >= TCPAGINGCYCLES) {
      // Close and clean the connection
      ModbusCloseConnNull(modH);
      //clientconn->aging = 0;
      if (osSemaphoreRelease(modbus_semaphore) != osOK) {
        modH->u16ErrUndef++;
      }
    //}
    return xTCPvalid = false;
  } else if (err == ERR_OK) {
    if (netconn_err(clientconn->conn) == ERR_OK) {
      // Read the data from the port, blocking if nothing yet there
      // We assume the request (the part we care about) is in one netbuf
      netbuf_data(inbuf, (void**)&buf, &buflen);
      // minimum frame size for modbus TCP
      if (buflen > 11) {
        // validate protocol ID
        if (buf[2] == 0 || buf[3] == 0) {
          uLength = (buf[4]<<8 & 0xff00) | buf[5];
          if (uLength < (MAX_BUFFER-2) && (uLength + 6) <= buflen) {
            for (uint16_t i = 0; i < uLength; i++) {
              modH->u8Buffer[i] = buf[i+6];
            }
            modH->u16TransactionID = (buf[0]<<8 & 0xff00) | buf[1];
            // add 2 dummy bytes for CRC
            modH->u8BufferSize = uLength + 2;
            // we have data for the modbus slave
            xTCPvalid = true;
            modH->u16InCnt++;
          }
        }
      }
      // reset the aging counter
      clientconn->aging = 0;
    } else {
      modH->u16ErrUndef++;
      if (osSemaphoreRelease(modbus_semaphore) != osOK) {
        modH->u16ErrUndef++;
      }
      // delete the buffer always
      netbuf_delete(inbuf); 
      // Close and clean the connection
      ModbusCloseConnNull(modH);
      clientconn->aging = 0;
      return xTCPvalid = false;
    }
  } else {
    modH->u16ErrUndef++;
  }
  // always delete buffer
  netbuf_delete(inbuf);
  
  return xTCPvalid;
}

uint8_t TCPinitserver(modbusHandler_t *modH) {
  // Create a new TCP connection handle
  modH->conn = netconn_new(NETCONN_TCP);
  if (modH->conn == NULL) {
    // error creating new connection
    return 1;
  }
  // if port not defined
  if (modH->uTcpPort == 0) {
    modH->uTcpPort = 502;
  }
  err_t err = netconn_bind(modH->conn, NULL, modH->uTcpPort);
  if (err != ERR_OK) {
    ModbusCloseConn(modH->conn);
    // error binding
    return 2;
  }
  // Put the connection into LISTEN state
  err = netconn_listen(modH->conn);
  //err = netconn_listen_with_backlog(modH->conn, 2);
  if (err != ERR_OK) {
    ModbusCloseConn(modH->conn);
    // error listen
    return 3;
  }
  // this is necessary to make it non blocking
  netconn_set_recvtimeout(modH->conn, 1);
  //
  return 0;
}

#endif

tcpclients_t slave_newconns[3];

void StartTaskModbusSlave(void *argument) {
  modbusHandler_t *modH = (modbusHandler_t *)argument;

  if (modH->xTypeHW == TCP_HW) {
    modH->newconns = slave_newconns;
    //memset(&modH->newconns, 0, sizeof(tcpclients_t) * NUMBERTCPCONN);
    memset(slave_newconns, 0, sizeof(tcpclients_t) * modH->num_tcp_conn);
    
    modbus_semaphore = osSemaphoreNew(2, 2, NULL);
  }
 
  while (1) {

#ifdef DEBUG
    if (modH->xTypeHW == TCP_HW) {
      WM_MBSlaveTCP = uxTaskGetStackHighWaterMark(NULL);
    }
    if (modH->xTypeHW == USART_HW || modH->xTypeHW == USART_HW_DMA) {
      WM_MBSlaveRTU = uxTaskGetStackHighWaterMark(NULL);
    }
#endif

    modH->i8lastError = MB_ERR_OK;

#if ENABLE_USB_CDC == 1
    if (modH->xTypeHW == USB_CDC_HW) {
      // Block indefinitely until a Modbus Frame arrives
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
      // is this necessary?
      if (modH->u8BufferSize == ERR_BUFF_OVERFLOW) {
        modH->i8lastError = MB_ERR_BUFF_OVERFLOW;
        modH->u32errCnt++;
        continue;
      }
    }
#endif
    
#if ENABLE_TCP == 1
    if (modH->xTypeHW == TCP_HW) {
      
      if (modH->conn == NULL) {
        uint8_t res;
        do {
          // start the Modbus server slave
          res = TCPinitserver(modH);
          osDelay(50);
        } while (res != 0);
      }
      
      // wait for connection and receive data
      if (TCPwaitConnData(modH) == false) {
        continue; // TCP package was not validated
      }
    }
#endif
    if (modH->xTypeHW == USART_HW || modH->xTypeHW == USART_HW_DMA) {
      // Block until a Modbus Frame arrives
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
      if (getRxBuffer(modH) < 0) {
        modH->i8lastError = MB_ERR_BUFF_OVERFLOW;
        modH->u16ErrCnt++;
        continue;
      }
    }
    if (modH->u8BufferSize < 7) {
      // The size of the frame is invalid
      modH->i8lastError = MB_ERR_BAD_SIZE;
      modH->u16ErrCnt++;
      continue;
    }
    
    //check broadcast mode
    modH->u8AddressMode = ADDRESS_NORMAL;
    if (modH->u8Buffer[ID] == ADDRESS_BROADCAST) {
        modH->u8AddressMode = ADDRESS_BROADCAST;
    }
    
    // check slave id
    if (modH->u8Buffer[ID] != modH->u8id && modH->u8AddressMode != ADDRESS_BROADCAST) {
#if ENABLE_TCP == 0
      continue; // continue this is not for us
#else
      if (modH->xTypeHW != TCP_HW) {
        continue; //for Modbus TCP this is not validated, user should modify accordingly if needed
      }
#endif
    }
    // validate message: CRC, FCT, address and size
    uint8_t u8exception = validateRequest(modH);
    if (u8exception > 0) {
      buildException(u8exception, modH);
      sendTxBuffer(modH);
      modH->i8lastError = (mb_error_t)u8exception;
      //return u8exception
      continue;
    }
    modH->i8lastError = MB_ERR_OK;
    // before processing the message get the semaphore
    xSemaphoreTake(modH->ModBusSphrHandle, portMAX_DELAY);
    // process message
    switch (modH->u8Buffer[FUNC]) {
      case MB_FC_READ_COILS:
      case MB_FC_READ_DISCRETE_INPUT: {
        if (modH->u8AddressMode == ADDRESS_BROADCAST) {
          // broadcast mode should ignore read function
          break;
        }
        modH->i8state = process_FC1(modH);
        break;
      }
      case MB_FC_READ_INPUT_REGISTER:
      case MB_FC_READ_REGISTERS: {
        if (modH->u8AddressMode == ADDRESS_BROADCAST) {
          // broadcast mode should ignore read function
          break;
        }
        modH->i8state = process_FC3(modH);
        break;
      }
      case MB_FC_WRITE_COIL: {
        modH->i8state = process_FC5(modH);
        break;
      }
      case MB_FC_WRITE_REGISTER: {
        modH->i8state = process_FC6(modH);
        break;
      }
      case MB_FC_WRITE_MULTIPLE_COILS: {
        modH->i8state = process_FC15(modH);
        break;
      }
      case MB_FC_WRITE_MULTIPLE_REGISTERS: {
        modH->i8state = process_FC16(modH);
        break;
      }
      default: {
        break;
      }
    }
    // Release the semaphore
    xSemaphoreGive(modH->ModBusSphrHandle); 
  }
}

void ModbusQuery(modbusHandler_t * modH, modbus_t telegram) {
  // Add the telegram to the TX tail Queue of Modbus
  if (modH->uModbusType == MB_MASTER) {
    telegram.u32CurrentTask = (uint32_t *) osThreadGetId();
    xQueueSendToBack(modH->QueueTelegramHandle, &telegram, 0);
  } else {
    // error a slave cannot send queries as a master
    while(1);
  }
}

void ModbusQueryInject(modbusHandler_t *modH, modbus_t telegram) {
  // Add the telegram to the TX head Queue of Modbus
  xQueueReset(modH->QueueTelegramHandle);
  telegram.u32CurrentTask = (uint32_t *) osThreadGetId();
  xQueueSendToFront(modH->QueueTelegramHandle, &telegram, 0);
}

#if ENABLE_TCP == 1
void ModbusCloseConn(struct netconn *conn) {
  if (conn != NULL) {
    netconn_close(conn);
    netconn_delete(conn);
  }
}

void ModbusCloseConnNull(modbusHandler_t * modH) {
  if (modH->newconns[modH->newconnIndex].conn != NULL) {
    netconn_close(modH->newconns[modH->newconnIndex].conn);
    netconn_delete(modH->newconns[modH->newconnIndex].conn);
    modH->newconns[modH->newconnIndex].conn = NULL;
  }
}
#endif

/**
 * @brief
 * *** Only Modbus Master ***
 * Generate a query to an slave with a modbus_t telegram structure
 * The Master must be in COM_IDLE mode. After it, its state would be COM_WAITING.
 * This method has to be called only in loop() section.
 *
 * @see modbus_t
 * @param modH  modbus handler
 * @param modbus_t  modbus telegram structure (id, fct, ...)
 * @ingroup loop
 */
int8_t SendQuery(modbusHandler_t *modH, modbus_t telegram ) {
  uint8_t u8regsno, u8bytesno;
  uint8_t error = 0;
  // before processing the message get the semaphore
  xSemaphoreTake(modH->ModBusSphrHandle, portMAX_DELAY);
  if (modH->u8id != 0) {
    error = MB_ERR_NOT_MASTER;
  }
  if (modH->i8state != COM_IDLE) {
    error = MB_ERR_POLLING;
  }
  if ((telegram.u8id == 0) || (telegram.u8id > 247)) { 
    error = MB_ERR_BAD_SLAVE_ID;
  }
  if (error) {
    modH->i8lastError = (mb_error_t)error;
    xSemaphoreGive(modH->ModBusSphrHandle);
    return error;
  }
  modH->u16regs = telegram.u16reg;
  // telegram header
  modH->u8Buffer[ID] = telegram.u8id;
  modH->u8Buffer[FUNC] = telegram.u8fct;
  modH->u8Buffer[ADD_HI] = highByte(telegram.u16RegAdd);
  modH->u8Buffer[ADD_LO] = lowByte(telegram.u16RegAdd);
  
  switch (telegram.u8fct) {
    case MB_FC_READ_COILS:
    case MB_FC_READ_DISCRETE_INPUT:
    case MB_FC_READ_REGISTERS:
    case MB_FC_READ_INPUT_REGISTER: {
      modH->u8Buffer[NB_HI] = highByte(telegram.u16CoilsNo);
      modH->u8Buffer[NB_LO] = lowByte(telegram.u16CoilsNo);
      modH->u8BufferSize = 6;
      break;
    }
    case MB_FC_WRITE_COIL: {
      modH->u8Buffer[NB_HI] = ((telegram.u16reg[0]> 0) ? 0xff : 0);
      modH->u8Buffer[NB_LO] = 0;
      modH->u8BufferSize = 6;
      break;
    }
    case MB_FC_WRITE_REGISTER: {
      modH->u8Buffer[NB_HI] = highByte(telegram.u16reg[0]);
      modH->u8Buffer[NB_LO] = lowByte(telegram.u16reg[0]);
      modH->u8BufferSize = 6;
      break;
    }
    case MB_FC_WRITE_MULTIPLE_COILS: {// TODO: implement "sending coils"
      u8regsno = telegram.u16CoilsNo / 16;
      u8bytesno = u8regsno * 2;
      if ((telegram.u16CoilsNo % 16) != 0) {
        u8bytesno++;
        u8regsno++;
      }
      modH->u8Buffer[NB_HI] = highByte(telegram.u16CoilsNo);
      modH->u8Buffer[NB_LO] = lowByte(telegram.u16CoilsNo);
      modH->u8Buffer[BYTE_CNT] = u8bytesno;
      modH->u8BufferSize = 7;
      // if you generate a response with a sequence Hi, Lo the protocol does not work
      for (uint16_t i = 0; i < u8bytesno; i++) {
        if (i % 2) {
          modH->u8Buffer[modH->u8BufferSize] = highByte(telegram.u16reg[i/2]);
          //modH->u8Buffer[modH->u8BufferSize] = lowByte( telegram.u16reg[i/2]);
        } else {
          modH->u8Buffer[modH->u8BufferSize] = lowByte(telegram.u16reg[i/2]);
          //modH->u8Buffer[modH->u8BufferSize] = highByte(telegram.u16reg[i/2]);
        }
        modH->u8BufferSize++;
      }
      break;
    }
    case MB_FC_WRITE_MULTIPLE_REGISTERS: {
      modH->u8Buffer[NB_HI] = highByte(telegram.u16CoilsNo);
      modH->u8Buffer[NB_LO] = lowByte( telegram.u16CoilsNo);
      modH->u8Buffer[BYTE_CNT] = (uint8_t)(telegram.u16CoilsNo * 2);
      modH->u8BufferSize = 7; 
      for (uint16_t i = 0; i < telegram.u16CoilsNo; i++) {
        modH->u8Buffer[modH->u8BufferSize] = highByte(telegram.u16reg[i]);
        modH->u8BufferSize++;
        modH->u8Buffer[modH->u8BufferSize] = lowByte(telegram.u16reg[i]);
        modH->u8BufferSize++;
      }
      break;
    }
  }
  sendTxBuffer(modH);
  xSemaphoreGive(modH->ModBusSphrHandle);
  modH->i8state = COM_WAITING;
  modH->i8lastError = MB_ERR_OK;
  return 0;
}

#if ENABLE_TCP == 1

uint8_t connect_flag;

void fn(struct netconn *conn, enum netconn_evt evt, u16_t len) {
  if (evt == NETCONN_EVT_SENDPLUS) {
    connect_flag = 2;
  } else if (evt == NETCONN_EVT_ERROR) {
    connect_flag = 1;
  }
}

static mb_error_t TCPconnectserver(modbusHandler_t *modH, modbus_t *telegram) {
  err_t err;
  tcpclients_t *clientconn;
  // select the current connection slot to work with
  clientconn = &modH->newconns[modH->newconnIndex];
  //
  if (telegram->u8clientID >= modH->num_tcp_conn) {
  //if (telegram->u8clientID >= NUMBERTCPCONN) {
    return MB_ERR_BAD_TCP_ID;
  }
  
  if (clientconn->conn == NULL) {
    clientconn->conn = netconn_new_with_callback(NETCONN_TCP, fn);
  }
  
  // if the connection is null open a new connection
  //if (clientconn->conn == NULL) {
  //else {
    //clientconn->conn = netconn_new(NETCONN_TCP);    
    //clientconn->conn = netconn_new_with_callback(NETCONN_TCP, fn);
    if (clientconn->conn != NULL) {
      
      tcp_nagle_disable(clientconn->conn->pcb.tcp);
      
      netconn_set_nonblocking(clientconn->conn, true);
      err = netconn_connect(clientconn->conn, (ip_addr_t *)&telegram->xIpAddress, telegram->u16Port);
      netconn_set_nonblocking(clientconn->conn, false);
      if (err == ERR_INPROGRESS) {
        connect_flag = 0;
        uint32_t time = HAL_GetTick() + modH->recvtimeout;
        do {
          if (connect_flag == 1) {
            return MB_ERR_TCP_CON;
          }
          if (time < HAL_GetTick()) {            
            return MB_ERR_TIMEOUT;
          }
          osDelay(10);
        } while (connect_flag == 0);
      }
      
      err = netconn_err(clientconn->conn);

      if (err != ERR_OK) {        
        return MB_ERR_TIMEOUT;
      }
    } else {
      return MB_ERR_TCP_CON;
    }
  //}
  return MB_ERR_OK;
}

static mb_error_t TCPgetRxBuffer(modbusHandler_t *modH) {
  struct netbuf *inbuf;
  err_t err;
  char* buf;
  uint16_t buflen;
  uint16_t uLength = 0;
  tcpclients_t *clientconn;
  // select the current connection slot to work with
  clientconn = &modH->newconns[modH->newconnIndex];
  
  if (clientconn->conn == NULL) {
    return MB_ERR_TCP_CON;
  }
  
  // reset buffer
  memset(modH->u8Buffer, 0, MAX_BUFFER);
  modH->u8BufferSize = 0;
  
  netconn_set_recvtimeout(clientconn->conn, modH->recvtimeout);
  err = netconn_recv(clientconn->conn, &inbuf);
  if (err == ERR_OK) {
    err = netconn_err(clientconn->conn);
    if (err == ERR_OK) {
      // Read the data from the port, blocking if nothing yet there.
      // We assume the request (the part we care about) is in one netbuf
      err = netbuf_data(inbuf, (void**)&buf, &buflen);
      if (err == ERR_OK) {
        // minimum frame size for modbus TCP
        if ((buflen > 11 && (modH->uModbusType == MB_SLAVE)) || (buflen >= 10 && (modH->uModbusType == MB_MASTER))) {
          // validate protocol ID
          if (buf[2] == 0 || buf[3] == 0) {
            uLength = (buf[4] << 8 & 0xff00)|buf[5];
            if (uLength < (MAX_BUFFER-2) && (uLength + 6) <= buflen) {
              for (int i = 0; i < uLength; i++) {
                modH->u8Buffer[i] = buf[i+6];
              }
              modH->u16TransactionID = (buf[0]<<8 & 0xff00)|buf[1];
              // include 2 dummy bytes for CRC
              modH->u8BufferSize = uLength + 2;              
            }
          }
        }
      } // netbuf_data
      //netbuf_delete(inbuf);
    }
  }
  // always delete buffer
  netbuf_delete(inbuf);
  
  if (err != ERR_OK) {
    return MB_ERR_TCP_CON;
  }
  return MB_ERR_OK;
}

#endif

tcpclients_t master_newconns[20];

void StartTaskModbusMaster(void *argument) {
  modbusHandler_t *modH = (modbusHandler_t *)argument;
  uint32_t ulNotificationValue;
  modbus_t telegram;
  
  memset(&telegram, 0, sizeof(modbus_t));

  if (modH->xTypeHW == TCP_HW) {
    
    modH->newconns = master_newconns;
    
    memset(master_newconns, 0, sizeof(tcpclients_t) * modH->num_tcp_conn);
    //memset(&modH->newconns, 0, sizeof(tcpclients_t) * NUMBERTCPCONN);
  }
  
   if (modH->xTypeHW != TCP_HW) {
     modH->newconns = NULL;
   }
  
  while (1) {

#ifdef DEBUG
    if (modH->xTypeHW == TCP_HW) {
      WM_MBMasterTCP = uxTaskGetStackHighWaterMark(NULL);
    }
    if (modH->xTypeHW == USART_HW || modH->xTypeHW == USART_HW_DMA) {
      WM_MBMasterRTU = uxTaskGetStackHighWaterMark(NULL);
    }
#endif

    // Wait indefinitely for a telegram to send
    xQueueReceive(modH->QueueTelegramHandle, &telegram, portMAX_DELAY);

#if ENABLE_TCP == 1
    if (modH->xTypeHW == TCP_HW) {
      modH->newconnIndex = telegram.u8clientID;
      //
      ulNotificationValue = TCPconnectserver(modH, &telegram);
      if (ulNotificationValue == MB_ERR_OK) {
        SendQuery(modH, telegram);
        // Block until a Modbus Frame arrives or query timeouts
        // TCP receives the data and the notification simultaneously since it is synchronous
        ulNotificationValue = TCPgetRxBuffer(modH);
        // close the TCP connection
        if (ulNotificationValue != MB_ERR_OK) {
          ModbusCloseConnNull(modH);
        }
      } else {
        ModbusCloseConnNull(modH);
      }
    } else {
      // send a query for USART and USB_CDC
      SendQuery(modH, telegram);
      // Block until a Modbus Frame arrives or query timeouts
      ulNotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
#else

// Merge pull request #93 from ZaTeeman/fix_silent_time
// Wait period of silence between modbus frame
    if (modH->port->Init.BaudRate <= 19200) {
      osDelay((int)(35000/modH->port->Init.BaudRate) + 2);
    } else {
      osDelay(3);
    }
// Merge pull request #93 from ZaTeeman/fix_silent_time

    // This is the case for implementations with only USART support
    SendQuery(modH, telegram);
    // Block indefinitely until a Modbus Frame arrives or query timeouts
    ulNotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
/*
    TickType_t xBlockTime = pdMS_TO_TICKS(modH->u16timeOut);
    ulNotificationValue = ulTaskNotifyTake(pdTRUE, xBlockTime);
*/
#endif
    
    // notify the task the request timeout
    modH->i8lastError = MB_ERR_OK;
    if (ulNotificationValue == MB_ERR_TIMEOUT) {
      modH->i8state = COM_IDLE;
      modH->i8lastError = MB_ERR_TIMEOUT;
      modH->u16ErrCnt++;
      xTaskNotify((TaskHandle_t)telegram.u32CurrentTask, modH->i8lastError, eSetValueWithOverwrite);
      continue;
    } else if (ulNotificationValue == MB_ERR_TCP_CON) {
      modH->i8state = COM_IDLE;
      modH->i8lastError = MB_ERR_TCP_CON;
      modH->u16ErrCnt++;
      xTaskNotify((TaskHandle_t)telegram.u32CurrentTask, modH->i8lastError, eSetValueWithOverwrite);
      continue;
    }
#if ENABLE_USB_CDC == 1 || ENABLE_TCP == 1
    // TCP and USB_CDC use different methods to get the buffer
    if (modH->xTypeHW == USART_HW || modH->xTypeHW == USART_HW_DMA) {
      
      if (modH->port->Init.BaudRate <= 19200) {
        osDelay((int)(35000/modH->port->Init.BaudRate) + 2);
      } else {
        osDelay(2);
      }
    
      modH->u8BufferSize = 0;
      getRxBuffer(modH);
    }
#else
    getRxBuffer(modH);
#endif
    if (modH->u8BufferSize < 6) {
      modH->i8state = COM_IDLE;
      modH->i8lastError = MB_ERR_BAD_SIZE;
      modH->u16ErrCnt++;
      xTaskNotify((TaskHandle_t)telegram.u32CurrentTask, modH->i8lastError, eSetValueWithOverwrite);
      continue;
    }
    // cancel timeout timer
    xTimerStop(modH->xTimerTimeout, 0);
    // validate message: id, CRC, FCT, exception
    uint8_t u8exception = validateAnswer(modH);
    
    if (u8exception != MB_ERR_OK) {
      modH->i8state = COM_IDLE;
      modH->i8lastError = (mb_error_t)u8exception;
      xTaskNotify((TaskHandle_t)telegram.u32CurrentTask, modH->i8lastError, eSetValueWithOverwrite);
      continue;
    }
    
    modH->i8lastError = (mb_error_t)u8exception;
    // before processing the message get the semaphore
    xSemaphoreTake(modH->ModBusSphrHandle, portMAX_DELAY);
    // process answer
    switch (modH->u8Buffer[FUNC]) {
      case MB_FC_READ_COILS:
      case MB_FC_READ_DISCRETE_INPUT: {
        // call get_FC1 to transfer the incoming message to u16regs buffer
        get_FC1(modH);
        break;
      }
      case MB_FC_READ_INPUT_REGISTER:
      case MB_FC_READ_REGISTERS: {
        // call get_FC3 to transfer the incoming message to u16regs buffer
        get_FC3(modH);
        break;
      }
      case MB_FC_WRITE_COIL:
      case MB_FC_WRITE_REGISTER:
      case MB_FC_WRITE_MULTIPLE_COILS:
      case MB_FC_WRITE_MULTIPLE_REGISTERS: {
        // nothing to do
        break;
      }
      default: {
        break;
      }
    }
    modH->i8state = COM_IDLE;
    xSemaphoreGive(modH->ModBusSphrHandle);
    xTaskNotify((TaskHandle_t)telegram.u32CurrentTask, modH->i8lastError, eSetValueWithOverwrite);
    
/*    
    modH->i8state = COM_IDLE;
    // no error the error_OK, we need to use a different value than 0 to detect the timeout
    if (modH->i8lastError == MB_ERR_OK) {
      //Release the semaphore
      xSemaphoreGive(modH->ModBusSphrHandle);
      xTaskNotify((TaskHandle_t)telegram.u32CurrentTask, modH->i8lastError, eSetValueWithOverwrite);
    }
    continue;
*/
  }
}

/**
 * This method processes functions 1 & 2 (for master)
 * This method puts the slave answer into master data buffer
 *
 * @ingroup register
 */
void get_FC1(modbusHandler_t *modH) {
  uint8_t u8byte = 3;
  for (uint8_t i = 0; i < modH->u8Buffer[2]; i++) {
    if (i % 2) {
      modH->u16regs[i/2] = word(modH->u8Buffer[i+u8byte], lowByte(modH->u16regs[i/2]));
    } else {
      modH->u16regs[i/2] = word(highByte(modH->u16regs[i/2]), modH->u8Buffer[i+u8byte]);
    }
  }
}

/**
 * This method processes functions 3 & 4 (for master)
 * This method puts the slave answer into master data buffer
 *
 * @ingroup register
 */
void get_FC3(modbusHandler_t *modH) {
  uint8_t u8byte = 3;
  for (uint8_t i = 0; i < modH->u8Buffer[2] / 2; i++) {
    modH->u16regs[i] = word(modH->u8Buffer[u8byte], modH->u8Buffer[u8byte + 1]);
    u8byte += 2;
  }
}

/**
 * @brief
 * This method validates master incoming messages
 *
 * @return 0 if OK, EXCEPTION if anything fails
 * @ingroup buffer
 */
uint8_t validateAnswer(modbusHandler_t *modH) {
  // check message crc vs calculated crc
#if ENABLE_TCP == 1
  if (modH->xTypeHW != TCP_HW) {
    if (modH->u8Buffer[FUNC] == MB_FC_WRITE_COIL ||
        modH->u8Buffer[FUNC] == MB_FC_WRITE_REGISTER ||
        modH->u8Buffer[FUNC] == MB_FC_WRITE_MULTIPLE_COILS ||
        modH->u8Buffer[FUNC] == MB_FC_WRITE_MULTIPLE_REGISTERS        
        )
    {
      modH->u8BufferSize = 8;
    }    
#endif
    // combine the crc Low & High bytes
	uint16_t u16MsgCRC =
      ((modH->u8Buffer[modH->u8BufferSize - 2] << 8)
       | modH->u8Buffer[modH->u8BufferSize - 1]);
    
    if (calcCRC(modH->u8Buffer, modH->u8BufferSize - 2) != u16MsgCRC) {
      modH->u16ErrCnt++;
      return MB_ERR_BAD_CRC;
    }
#if ENABLE_TCP == 1
  }
#endif
  // check exception
  if ((modH->u8Buffer[FUNC] & 0x80) != 0) {
    modH->u16ErrCnt++;
    return MB_ERR_EXCEPTION;
  }
  
  // check fct code
  bool isSupported = false;
  for (uint8_t i = 0; i < sizeof(fctsupported); i++) {
    if (fctsupported[i] == modH->u8Buffer[FUNC]) {
      isSupported = 1;
      break;
    }
  }
  if (!isSupported) {
    modH->u16ErrCnt++;
    return EXC_FUNC_CODE;
    //return MB_ERR_EXCEPTION;
  }
  // OK, no exception code thrown
  return MB_ERR_OK;
}

/**
 * @brief
 * This method moves Serial buffer data to the Modbus u8Buffer.
 *
 * @return buffer size if OK, ERR_BUFF_OVERFLOW if u8BufferSize >= MAX_BUFFER
 * @ingroup buffer
 */
int16_t getRxBuffer(modbusHandler_t *modH) {
  int16_t i16result;  
  if (modH->xTypeHW == USART_HW) {
    // disable interrupts to avoid race conditions on serial port
    HAL_UART_AbortReceive_IT(modH->port);
  }
  if (modH->xBufferRX.overflow) {
    // clean up the overflowed buffer
    RingClear(&modH->xBufferRX); 
    i16result = -1;
  } else {
    modH->u8BufferSize = RingGetAllBytes(&modH->xBufferRX, modH->u8Buffer);
    modH->u16InCnt++;
    i16result = modH->u8BufferSize;
  }
  if (modH->xTypeHW == USART_HW) {
    HAL_UART_Receive_IT(modH->port, &modH->dataRX, 1);
  }
  return i16result;
}

/**
 * @brief
 * This method validates slave incoming messages
 *
 * @return 0 if OK, EXCEPTION if anything fails
 * @ingroup modH Modbus handler
 */
uint8_t validateRequest(modbusHandler_t *modH) {
  // check message crc vs calculated crc
#if ENABLE_TCP == 1
  uint16_t u16MsgCRC;
  u16MsgCRC = ((modH->u8Buffer[modH->u8BufferSize - 2] << 8)
              | modH->u8Buffer[modH->u8BufferSize - 1]); // combine the crc Low & High bytes
  if (modH->xTypeHW != TCP_HW) {
    if (calcCRC(modH->u8Buffer,  modH->u8BufferSize - 2) != u16MsgCRC) {
      modH->u16ErrCnt++;
      return MB_ERR_BAD_CRC;
    }
  }
#else
  uint16_t u16MsgCRC;
  u16MsgCRC = ((modH->u8Buffer[modH->u8BufferSize - 2] << 8)
              | modH->u8Buffer[modH->u8BufferSize - 1]); // combine the crc Low & High bytes
  if (calcCRC( modH->u8Buffer,  modH->u8BufferSize-2 ) != u16MsgCRC) {
    modH->u16ErrCnt++;
    return MB_ERR_BAD_CRC;
  }
#endif
  // check fct code
  bool isSupported = false;
  for (uint8_t i = 0; i < sizeof(fctsupported); i++) {
    if (fctsupported[i] == modH->u8Buffer[FUNC]) {
      isSupported = 1;
      break;
    }
  }
  if (!isSupported) {
    modH->u16ErrCnt++;
    return EXC_FUNC_CODE;
  }
  // check start address & nb range
  uint16_t u16AdRegs = 0;
  uint16_t u16NRegs = 0;
  //uint8_t u8regs;
  switch (modH->u8Buffer[FUNC]) {
    case MB_FC_READ_COILS:
    case MB_FC_READ_DISCRETE_INPUT:
    case MB_FC_WRITE_MULTIPLE_COILS: {
      u16AdRegs = word( modH->u8Buffer[ADD_HI], modH->u8Buffer[ADD_LO]) / 16;
      u16NRegs = word( modH->u8Buffer[NB_HI], modH->u8Buffer[NB_LO]) / 16;
      // check for incomplete words
      if (word( modH->u8Buffer[NB_HI], modH->u8Buffer[NB_LO]) % 16) { 
        u16NRegs++;
      }
      // verify address range
      if ((u16AdRegs + u16NRegs) > modH->u16regsize) {
        return EXC_ADDR_RANGE;
      }
      //verify answer frame size in bytes
      u16NRegs = word(modH->u8Buffer[NB_HI], modH->u8Buffer[NB_LO]) / 8;
      if (word( modH->u8Buffer[NB_HI], modH->u8Buffer[NB_LO]) % 8) {
        u16NRegs++;
      }
      // adding the header  and CRC ( Slave address + Function code  + number of data bytes to follow + 2-byte CRC )
      u16NRegs = u16NRegs + 5; 
      if (u16NRegs > 256) {
        return EXC_REGS_QUANT;
      }
      break;
    }
    case MB_FC_WRITE_COIL: {
      u16AdRegs = word( modH->u8Buffer[ADD_HI], modH->u8Buffer[ADD_LO]) / 16;
      // check for incomplete words
      if (word( modH->u8Buffer[ADD_HI], modH->u8Buffer[ADD_LO]) % 16) {
        u16AdRegs++;
      }
      if (u16AdRegs > modH->u16regsize) {
        return EXC_ADDR_RANGE;
      }
      break;
    }
    case MB_FC_WRITE_REGISTER: {
      u16AdRegs = word( modH->u8Buffer[ADD_HI], modH->u8Buffer[ADD_LO]);
      if (u16AdRegs > modH-> u16regsize) {
        return EXC_ADDR_RANGE;
      }
      break;
    }
    case MB_FC_READ_REGISTERS:
    case MB_FC_READ_INPUT_REGISTER:
    case MB_FC_WRITE_MULTIPLE_REGISTERS: {
      u16AdRegs = word(modH->u8Buffer[ADD_HI], modH->u8Buffer[ADD_LO]);
      u16NRegs = word(modH->u8Buffer[NB_HI], modH->u8Buffer[NB_LO]);
      if ((u16AdRegs + u16NRegs) > modH->u16regsize) {
        return EXC_ADDR_RANGE;
      }
      // verify answer frame size in bytes
      // adding the header  and CRC
      u16NRegs = u16NRegs * 2 + 5; 
      if (u16NRegs > 256) {
        return EXC_REGS_QUANT;
      }
      break;
    }
  }
  // OK, no exception code thrown
  return 0; 
}

/**
 * @brief
 * This method creates a word from 2 bytes
 *
 * @return uint16_t (word)
 * @ingroup H  Most significant byte
 * @ingroup L  Less significant byte
 */
uint16_t word(uint8_t H, uint8_t L) {
  bytesFields W;
  W.u8[0] = L;
  W.u8[1] = H;
  return W.u16[0];
}

/**
 * @brief
 * This method calculates CRC
 *
 * @return uint16_t calculated CRC value for the message
 * @ingroup Buffer
 * @ingroup u8length
 */
uint16_t calcCRC(uint8_t *Buffer, uint8_t u8length) {
  unsigned int temp, temp2, flag;
  temp = 0xFFFF;
  for (unsigned char i = 0; i < u8length; i++) {
    temp = temp ^ Buffer[i];
    for (unsigned char j = 1; j <= 8; j++) {
      flag = temp & 0x0001;
      temp >>=1;
      if (flag) {
        temp ^= 0xA001;
      }
    }
  }
  // Reverse byte order
  temp2 = temp >> 8;
  temp = (temp << 8) | temp2;
  temp &= 0xFFFF;
  // the returned value is already swapped
  // crcLo byte is first & crcHi byte is last
  return temp;
}

/**
 * @brief
 * This method builds an exception message
 *
 * @ingroup u8exception exception number
 * @ingroup modH modbus handler
 */
void buildException(uint8_t u8exception, modbusHandler_t *modH) {
  // get the original FUNC code
  uint8_t u8func = modH->u8Buffer[FUNC];
  modH->u8Buffer[ID] = modH->u8id;
  modH->u8Buffer[FUNC] = u8func + 0x80;
  modH->u8Buffer[2] = u8exception;
  modH->u8BufferSize = EXCEPTION_SIZE;
}

#if ENABLE_USB_CDC == 1
extern uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
#endif

/**
 * @brief
 * This method transmits u8Buffer to Serial line.
 * Only if u8txenpin != 0, there is a flow handling in order to keep
 * the RS485 transceiver in output state as long as the message is being sent.
 * This is done with TC bit.
 * The CRC is appended to the buffer before starting to send it.
 *
 * @return nothing
 * @ingroup modH Modbus handler
 */
static void sendTxBuffer(modbusHandler_t *modH) {
  // when in slaveType and u8AddressMode == ADDRESS_BROADCAST, do not send anything
  if (modH->uModbusType == MB_SLAVE && modH->u8AddressMode == ADDRESS_BROADCAST) {
    modH->u8BufferSize = 0;
    // increase message counter
    modH->u16OutCnt++;
    return;
  }
  
  // append CRC to message
#if  ENABLE_TCP == 1
  if (modH->xTypeHW != TCP_HW) {
#endif
    uint16_t u16crc = calcCRC(modH->u8Buffer, modH->u8BufferSize);
    modH->u8Buffer[modH->u8BufferSize] = u16crc >> 8;
    modH->u8BufferSize++;
    modH->u8Buffer[modH->u8BufferSize] = u16crc & 0x00ff;
    modH->u8BufferSize++;
#if ENABLE_TCP == 1
  }
#endif

#if ENABLE_USB_CDC == 1 || ENABLE_TCP == 1
  if (modH->xTypeHW == USART_HW || modH->xTypeHW == USART_HW_DMA) {
#endif
    if (modH->EN_Port != NULL) {
      // enable transmitter, disable receiver to avoid echo on RS485 transceivers
      HAL_HalfDuplex_EnableTransmitter(modH->port);
      HAL_GPIO_WritePin(modH->EN_Port, modH->EN_Pin, GPIO_PIN_SET);
    }

#if ENABLE_USART_DMA == 1
    if (modH->xTypeHW == USART_HW) {
#endif
      // transfer buffer to serial line IT
      HAL_UART_Transmit_IT(modH->port, modH->u8Buffer, modH->u8BufferSize);

#if ENABLE_USART_DMA == 1
    } else {
      // transfer buffer to serial line DMA
      HAL_UART_Transmit_DMA(modH->port, modH->u8Buffer, modH->u8BufferSize);
    }
#endif
    // wait notification from TXE interrupt
    ulTaskNotifyTake(pdTRUE, 250);
/*
* If you are porting the library to a different MCU check the 
* USART datasheet and add the corresponding family in the following
* preprocessor conditions
*/
#if defined(STM32H7) || defined(STM32F3) || defined(STM32L4) || defined(STM32L082xx) || defined(STM32F7) || defined(STM32WB) || defined(STM32G070xx) || defined(STM32F0) || defined(STM32G431xx)
    while((modH->port->Instance->ISR & USART_ISR_TC) == 0)
#else
    // F429, F103, L152 ...
    while((modH->port->Instance->SR & USART_SR_TC) == 0)
#endif
    {
      //block the task until the the last byte is send out of the shifting buffer in USART
    }
    
    if (modH->EN_Port != NULL) {
      // return RS485 transceiver to receive mode
      HAL_GPIO_WritePin(modH->EN_Port, modH->EN_Pin, GPIO_PIN_RESET);
      // enable receiver, disable transmitter
      HAL_HalfDuplex_EnableReceiver(modH->port);
    }
    // set timeout for master query
    if (modH->uModbusType == MB_MASTER) {
      xTimerReset(modH->xTimerTimeout, 0);
    }
#if ENABLE_USB_CDC == 1 || ENABLE_TCP == 1
  }

#if ENABLE_USB_CDC == 1
  else if(modH->xTypeHW == USB_CDC_HW) {
    CDC_Transmit_FS(modH->u8Buffer,  modH->u8BufferSize);
    // set timeout for master query
    if (modH->uModbusType == MB_MASTER) {
      xTimerReset(modH->xTimerTimeout, 0);
    }
  }
#endif

#if ENABLE_TCP == 1
  else if (modH->xTypeHW == TCP_HW) {
    uint8_t u8MBAPheader[6];
    // this might need improvement the transaction ID could be validated
    u8MBAPheader[0] = highByte(modH->u16TransactionID);
    u8MBAPheader[1] = lowByte(modH->u16TransactionID);
    u8MBAPheader[2] = 0; // protocol ID
    u8MBAPheader[3] = 0; // protocol ID
    u8MBAPheader[4] = 0; // highbyte data length always 0
    u8MBAPheader[5] = modH->u8BufferSize; // highbyte data length
    
    struct netvector xNetVectors[2];
    xNetVectors[0].len = 6;
    xNetVectors[0].ptr = (void *)u8MBAPheader;
    
    xNetVectors[1].len = modH->u8BufferSize;
    xNetVectors[1].ptr = (void *)modH->u8Buffer;
    
    //netconn_set_sendtimeout(modH->newconns[modH->newconnIndex].conn, modH->sendtimeout);
    
    size_t uBytesWritten;
    err_t err = netconn_write_vectors_partly(modH->newconns[modH->newconnIndex].conn, xNetVectors, 2, NETCONN_COPY, &uBytesWritten);
    if (err != ERR_OK ) {
      modH->u16ErrCnt++;
      ModbusCloseConnNull(modH);
    }
    
    if (modH->uModbusType == MB_MASTER) {
      xTimerReset(modH->xTimerTimeout, 0);
    }
  }
#endif

#endif
  modH->u8BufferSize = 0;
  // increase message counter
  modH->u16OutCnt++;
}

/**
 * @brief
 * This method processes functions 1 & 2
 * This method reads a bit array and transfers it to the master
 *
 * @return u8BufferSize Response to master length
 * @ingroup discrete
 */
int8_t process_FC1(modbusHandler_t *modH) {
  uint16_t u16currentRegister;
  uint8_t u8currentBit, u8bytesno, u8bitsno;
  uint8_t u8CopyBufferSize;
  uint16_t u16currentCoil, u16coil;
  // get the first and last coil from the message
  uint16_t u16StartCoil = word( modH->u8Buffer[ADD_HI], modH->u8Buffer[ADD_LO]);
  uint16_t u16Coilno = word( modH->u8Buffer[NB_HI], modH->u8Buffer[NB_LO]);
  // put the number of bytes in the outcoming message
  u8bytesno = (uint8_t) (u16Coilno / 8);
  if (u16Coilno % 8 != 0) 
    u8bytesno ++;
  modH->u8Buffer[ADD_HI] = u8bytesno;
  modH->u8BufferSize = ADD_LO;
  modH->u8Buffer[modH->u8BufferSize + u8bytesno - 1] = 0;
  // read each coil from the register map and put its value inside the outcoming message
  u8bitsno = 0;
  for (u16currentCoil = 0; u16currentCoil < u16Coilno; u16currentCoil++) {
    u16coil = u16StartCoil + u16currentCoil;
    u16currentRegister = (u16coil / 16);
    u8currentBit = (uint8_t)(u16coil % 16);
    bitWrite(modH->u8Buffer[modH->u8BufferSize], u8bitsno, bitRead(modH->u16regs[u16currentRegister], u8currentBit));
    u8bitsno++;
    if(u8bitsno > 7) {
      u8bitsno = 0;
      modH->u8BufferSize++;
    }
  }
  // send outcoming message
  if (u16Coilno % 8 != 0) {
    modH->u8BufferSize++;
  }
  u8CopyBufferSize = modH->u8BufferSize + 2;
  sendTxBuffer(modH);
  return u8CopyBufferSize;
}

/**
 * @brief
 * This method processes functions 3 & 4
 * This method reads a word array and transfers it to the master
 *
 * @return u8BufferSize Response to master length
 * @ingroup register
 */
int8_t process_FC3(modbusHandler_t *modH) {
  uint16_t u16StartAdd = word( modH->u8Buffer[ADD_HI], modH->u8Buffer[ADD_LO]);
  uint8_t u8regsno = word(modH->u8Buffer[NB_HI], modH->u8Buffer[NB_LO]);
  uint8_t u8CopyBufferSize;
  uint16_t i;
  modH->u8Buffer[2] = u8regsno * 2;
  modH->u8BufferSize = 3;
  for (i = u16StartAdd; i < u16StartAdd + u8regsno; i++) {
    modH->u8Buffer[modH->u8BufferSize] = highByte(modH->u16regs[i]);
    modH->u8BufferSize++;
    modH->u8Buffer[modH->u8BufferSize] = lowByte(modH->u16regs[i]);
    modH->u8BufferSize++;
  }
  u8CopyBufferSize = modH->u8BufferSize + 2;
  sendTxBuffer(modH);
  return u8CopyBufferSize;
}

/**
 * @brief
 * This method processes function 5
 * This method writes a value assigned by the master to a single bit
 *
 * @return u8BufferSize Response to master length
 * @ingroup discrete
 */
int8_t process_FC5( modbusHandler_t *modH) {
  uint8_t u8currentBit;
  uint16_t u16currentRegister;
  uint8_t u8CopyBufferSize;
  uint16_t u16coil = word(modH->u8Buffer[ADD_HI], modH->u8Buffer[ADD_LO]);
  // point to the register and its bit
  u16currentRegister = (u16coil / 16);
  u8currentBit = (uint8_t)(u16coil % 16);
  // write to coil
  bitWrite(modH->u16regs[u16currentRegister], u8currentBit,modH->u8Buffer[NB_HI] == 0xff);
  // send answer to master
  modH->u8BufferSize = 6;
  u8CopyBufferSize = modH->u8BufferSize + 2;
  sendTxBuffer(modH);
  return u8CopyBufferSize;
}

/**
 * @brief
 * This method processes function 6
 * This method writes a value assigned by the master to a single word
 *
 * @return u8BufferSize Response to master length
 * @ingroup register
 */
int8_t process_FC6(modbusHandler_t *modH) {
  uint16_t u16add = word(modH->u8Buffer[ADD_HI], modH->u8Buffer[ADD_LO]);
  uint8_t u8CopyBufferSize;
  uint16_t u16val = word(modH->u8Buffer[NB_HI], modH->u8Buffer[NB_LO]);
  modH->u16regs[u16add] = u16val;
  // keep the same header
  modH->u8BufferSize = RESPONSE_SIZE;
  u8CopyBufferSize = modH->u8BufferSize + 2;
  sendTxBuffer(modH);
  return u8CopyBufferSize;
}

/**
 * @brief
 * This method processes function 15
 * This method writes a bit array assigned by the master
 *
 * @return u8BufferSize Response to master length
 * @ingroup discrete
 */
int8_t process_FC15(modbusHandler_t *modH) {
  uint8_t u8currentBit, u8frameByte, u8bitsno;
  uint16_t u16currentRegister;
  uint8_t u8CopyBufferSize;
  uint16_t u16currentCoil, u16coil;
  bool bTemp;
  // get the first and last coil from the message
  uint16_t u16StartCoil = word(modH->u8Buffer[ADD_HI], modH->u8Buffer[ADD_LO]);
  uint16_t u16Coilno = word(modH->u8Buffer[NB_HI], modH->u8Buffer[NB_LO]);
  // read each coil from the register map and put its value inside the outcoming message
  u8bitsno = 0;
  u8frameByte = 7;
  for (u16currentCoil = 0; u16currentCoil < u16Coilno; u16currentCoil++) {
    u16coil = u16StartCoil + u16currentCoil;
    u16currentRegister = (u16coil / 16);
    u8currentBit = (uint8_t) (u16coil % 16);
    bTemp = bitRead(modH->u8Buffer[u8frameByte], u8bitsno);
    bitWrite(modH->u16regs[u16currentRegister], u8currentBit, bTemp);
    u8bitsno++;
    if (u8bitsno > 7) {
      u8bitsno = 0;
      u8frameByte++;
    }
  }
  // send outcoming message
  // it's just a copy of the incomping frame until 6th byte
  modH->u8BufferSize = 6;
  u8CopyBufferSize = modH->u8BufferSize + 2;
  sendTxBuffer(modH);
  return u8CopyBufferSize;
}

/**
 * @brief
 * This method processes function 16
 * This method writes a word array assigned by the master
 *
 * @return u8BufferSize Response to master length
 * @ingroup register
 */
int8_t process_FC16(modbusHandler_t *modH) {
  uint16_t u16StartAdd = modH->u8Buffer[ADD_HI] << 8 | modH->u8Buffer[ADD_LO];
  uint16_t u16regsno = modH->u8Buffer[NB_HI] << 8 | modH->u8Buffer[NB_LO];
  uint8_t u8CopyBufferSize;
  uint16_t i;
  uint16_t temp;
  // build header
  modH->u8Buffer[NB_HI] = 0;
  // answer is always 256 or less bytes
  modH->u8Buffer[NB_LO] = (uint8_t)u16regsno; 
  modH->u8BufferSize = RESPONSE_SIZE;
  // write registers
  for (i = 0; i < u16regsno; i++) {
    temp = word(modH->u8Buffer[(BYTE_CNT + 1) + i * 2], modH->u8Buffer[(BYTE_CNT + 2) + i * 2]);
    modH->u16regs[ u16StartAdd + i ] = temp;
  }
  u8CopyBufferSize = modH->u8BufferSize + 2;
  sendTxBuffer(modH);
  return u8CopyBufferSize;
}
