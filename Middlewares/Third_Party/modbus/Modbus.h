#ifndef THIRD_PARTY_MODBUS_INC_MODBUS_H_
#define THIRD_PARTY_MODBUS_INC_MODBUS_H_

#include "ModbusConfig.h"
#include <inttypes.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

typedef enum {
  USART_HW = 1,
  USB_CDC_HW = 2,
  TCP_HW = 3,
  USART_HW_DMA = 4,
} mb_hardware_t;

typedef enum {
  MB_SLAVE = 3,
  MB_MASTER = 4
} mb_masterslave_t;

typedef enum {
  ADDRESS_BROADCAST = 0,  //!< broadcast mode -> modH->u8Buffer[ID] == 0
  ADDRESS_NORMAL = 1,     //!< normal mode -> modH->u8Buffer[ID] > 0
} mb_address_t;

/**
 * @enum MB_FC
 * @brief
 * Modbus function codes summary.
 * These are the implement function codes either for Master or for Slave.
 *
 * @see also fctsupported
 * @see also modbus_t
 */
typedef enum MB_FC {
  MB_FC_READ_COILS               = 1,	 /*!< FCT=1  -> read coils or digital outputs */
  MB_FC_READ_DISCRETE_INPUT      = 2,	 /*!< FCT=2  -> read digital inputs */
  MB_FC_READ_REGISTERS           = 3,	 /*!< FCT=3  -> read registers or analog outputs */
  MB_FC_READ_INPUT_REGISTER      = 4,	 /*!< FCT=4  -> read analog inputs */
  MB_FC_WRITE_COIL               = 5,	 /*!< FCT=5  -> write single coil or output */
  MB_FC_WRITE_REGISTER           = 6,	 /*!< FCT=6  -> write single register */
  MB_FC_WRITE_MULTIPLE_COILS     = 15,   /*!< FCT=15 -> write multiple coils or outputs */
  MB_FC_WRITE_MULTIPLE_REGISTERS = 16	 /*!< FCT=16 -> write multiple registers */
} mb_functioncode_t;

typedef struct {
  uint8_t uxBuffer[MAX_BUFFER];
  uint8_t u8start;
  uint8_t u8end;
  uint8_t u8available;
  bool overflow;
} modbusRingBuffer_t;

/**
 * @enum MESSAGE
 * @brief
 * Indexes to telegram frame positions
 */
typedef enum MESSAGE {
  ID = 0,  //!< ID field
  FUNC,    //!< Function code position
  ADD_HI,  //!< Address high byte
  ADD_LO,  //!< Address low byte
  NB_HI,   //!< Number of coils or registers high byte
  NB_LO,   //!< Number of coils or registers low byte
  BYTE_CNT //!< byte counter
} mb_message_t;

typedef enum COM_STATES {
  COM_IDLE = 0,
  COM_WAITING = 1
} mb_com_state_t;

typedef enum ERROR_LIST {
  MB_ERR_OK = 20,
  MB_ERR_NOT_MASTER = 1,
  MB_ERR_POLLING = 2,
  MB_ERR_BUFF_OVERFLOW = 3,
  MB_ERR_BAD_CRC = 4,
  MB_ERR_EXCEPTION = 5,
  MB_ERR_BAD_SIZE = 6,
  MB_ERR_TIMEOUT = 8,
  MB_ERR_BAD_SLAVE_ID = 9,
  MB_ERR_BAD_TCP_ID = 10,
  MB_ERR_TCP_CON = 11,
  EXC_FUNC_CODE = 12,
  EXC_ADDR_RANGE = 13,
  EXC_REGS_QUANT = 14,
  //EXC_EXECUTE = 15
} mb_error_t;

typedef union {
  uint8_t u8[4];
  uint16_t u16[2];
  uint32_t u32;
} bytesFields;

/**
 * @struct modbus_t
 * @brief
 * Master query structure:
 * This structure contains all the necessary fields to make the Master generate a Modbus query.
 * A Master may keep several of these structures and send them cyclically or
 * use them according to program needs.
 */
typedef struct {
  uint8_t u8id;             /*!< Slave address between 1 and 247. 0 means broadcast */
  mb_functioncode_t u8fct;  /*!< Function code: 1, 2, 3, 4, 5, 6, 15 or 16 */
  uint16_t u16RegAdd;       /*!< Address of the first register to access at slave/s */
  uint16_t u16CoilsNo;      /*!< Number of coils or registers to access */
  uint16_t *u16reg;         /*!< Pointer to memory image in master */
  uint32_t *u32CurrentTask; /*!< Pointer to the task that will receive notifications from Modbus */
#if ENABLE_TCP == 1
  uint32_t xIpAddress;
  uint16_t u16Port;
  uint8_t u8clientID;
#endif
} modbus_t;

#if ENABLE_TCP == 1

typedef struct {
  struct netconn *conn;
  uint32_t aging;
} tcpclients_t;

#endif

/**
 * @struct modbusHandler_t
 * @brief
 * Modbus handler structure
 * Contains all the variables required for Modbus daemon operation
 */
typedef struct {
  mb_masterslave_t uModbusType;
  // HAL Serial Port handler
  UART_HandleTypeDef *port; 
  uint8_t u8id; //!< 0=master, 1..247=slave number
  GPIO_TypeDef* EN_Port; //!< flow control pin: 0=USB or RS-232 mode, >1=RS-485 mode
  uint16_t EN_Pin;  //!< flow control pin: 0=USB or RS-232 mode, >1=RS-485 mode
  mb_error_t i8lastError;
  // Modbus buffer for communication
  uint8_t u8Buffer[MAX_BUFFER]; 
  uint8_t u8BufferSize;
  uint8_t u8lastRec;
  uint16_t *u16regs;
  // keep statistics of Modbus traffic
  uint16_t u16InCnt; 
  uint16_t u16OutCnt;
  uint16_t u16ErrCnt;
  
  uint16_t u16ErrUndef;
  
  uint16_t sendtimeout;
  uint16_t recvtimeout;
  
  uint16_t u16regsize;
  uint8_t dataRX;
  int8_t i8state;
  
  uint16_t lost;
  
  mb_address_t u8AddressMode; //!< 0=broadcast, 1..247=normal

  //FreeRTOS components

  //Queue Modbus Telegram
  osMessageQueueId_t QueueTelegramHandle;
  //Task Modbus slave
  osThreadId_t myTaskModbusAHandle;
  //Timer RX Modbus
  xTimerHandle xTimerT35;
  //Timer MasterTimeout
  xTimerHandle xTimerTimeout;
  //Semaphore for Modbus data
  osSemaphoreId_t ModBusSphrHandle;
  // RX ring buffer for USART
  modbusRingBuffer_t xBufferRX;
  // type of hardware  TCP, USB CDC, USART
  mb_hardware_t xTypeHW;

#if ENABLE_TCP == 1
  uint8_t num_tcp_conn;
  //tcpclients_t newconns[NUMBERTCPCONN];
  tcpclients_t *newconns;
  struct netconn *conn;
  uint32_t xIpAddress;
  uint16_t u16TransactionID;
  // this is only used for the slave (i.e., the server)
  uint16_t uTcpPort;
  uint8_t newconnIndex;
#endif

} modbusHandler_t;

enum {
  RESPONSE_SIZE = 6,
  EXCEPTION_SIZE = 3,
  CHECKSUM_SIZE = 2
};

extern modbusHandler_t *mHandlers[MAX_M_HANDLERS];

// Function prototypes
void ModbusInit(modbusHandler_t * modH);
void ModbusStart(modbusHandler_t * modH);

#if ENABLE_USB_CDC == 1
void ModbusStartCDC(modbusHandler_t * modH);
#endif

//void setTimeOut(modbusHandler_t* modH, uint16_t u16timeOut); //!<write communication watch-dog timer

void set_timeout(modbusHandler_t* modH, uint16_t recv, uint16_t send);

//uint16_t getTimeOut(); //!<get communication watch-dog timer value
//bool getTimeOutState(); //!<get communication watch-dog timer state
// put a query in the queue tail
void ModbusQuery(modbusHandler_t * modH, modbus_t telegram ); 
// put a query in the queue head
void ModbusQueryInject(modbusHandler_t * modH, modbus_t telegram); 
// slave
void StartTaskModbusSlave(void *argument); 
// master
void StartTaskModbusMaster(void *argument); 
uint16_t calcCRC(uint8_t *Buffer, uint8_t u8length);

#if ENABLE_TCP == 1
//close the TCP connection
void ModbusCloseConn(struct netconn *conn);
//close the TCP connection and cleans the modbus handler
void ModbusCloseConnNull(modbusHandler_t * modH); 
#endif


//Function prototypes for ModbusRingBuffer

// adds a byte to the ring buffer
void RingAdd(modbusRingBuffer_t *xRingBuffer, uint8_t u8Val);
// gets all the available bytes into buffer and return the number of bytes read
uint8_t RingGetAllBytes(modbusRingBuffer_t *xRingBuffer, uint8_t *buffer);
// gets uNumber of bytes from ring buffer, returns the actual number of bytes read
uint8_t RingGetNBytes(modbusRingBuffer_t *xRingBuffer, uint8_t *buffer, uint8_t uNumber);
// return the number of available bytes
uint8_t RingCountBytes(modbusRingBuffer_t *xRingBuffer);
// flushes the ring buffer
void RingClear(modbusRingBuffer_t *xRingBuffer);

//global variable to maintain the number of concurrent handlers
extern uint8_t numberHandlers; 

uint32_t modbus_query(modbusHandler_t* mbHandler, modbus_t* mbTelegram, uint8_t retry);

/* prototypes of the original library not implemented

uint16_t getInCnt(); //!<number of incoming messages
uint16_t getOutCnt(); //!<number of outcoming messages
uint16_t getErrCnt(); //!<error counter
uint8_t getID(); //!<get slave ID between 1 and 247
uint8_t getState();
uint8_t getLastError(); //!<get last error message
void setID( uint8_t u8id ); //!<write new ID for the slave
void setTxendPinOverTime( uint32_t u32overTime );
void ModbusEnd(); //!<finish any communication and release serial communication port

*/

#endif /* THIRD_PARTY_MODBUS_INC_MODBUS_H_ */
