#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <stdint.h>
#include "lwip/ip_addr.h"

// sector 12
#define IDENTIFICATION_ADDR             ((uint32_t)0x08100000)

/*
// sector 14
#define CONFIGURATION_ADDR              ((uint32_t)0x08108000)
#define CONFIGURATION_SECTOR            ((uint8_t)14)
#define CONFIGURATION_SECTOR_SIZE       ((uint32_t)0x4000)
*/

// sector 15
#define STATE_ADDR                      ((uint32_t)0x0810C000)

// sector 16
#define CONFIGURATION_ADDR              ((uint32_t)0x08110000)
#define CONFIGURATION_SECTOR            ((uint8_t)16)
#define CONFIGURATION_SECTOR_SIZE       ((uint32_t)0x10000)

typedef struct {
  ip_addr_t self_ip_addr;
  ip_addr_t self_mask_addr;
  ip_addr_t self_gateway_addr;
  
  uint16_t mb_rate;
  uint16_t mb_timeout;
  
  uint16_t sens2_min_val;
  uint16_t sens2_max_val;
  
  uint16_t sens3_min_lim;
  uint16_t sens3_max_lim;
  
  uint16_t sens3_min_val;
  uint16_t sens3_max_val;
  
  uint8_t self_id;
  uint8_t urm_id;
  uint8_t umvh_id; 
  uint8_t sens1_id;  
  
  uint8_t sens2_id;
  uint8_t valve1_io;
  uint8_t valve2_io;
  uint8_t state_out_io;
  
  uint8_t sens3_io;
  uint8_t state_in_io;
  uint16_t pressure_set_point;

  uint32_t crc32;
} settings_t;

typedef union u_identification {
  uint32_t ToUint32;
  struct {
    uint16_t serial_number;
    uint8_t mac_octet_6;
    uint8_t mac_octet_5;
  };
} identification_t;

typedef struct {
  uint16_t hysteresis_window;
  uint16_t self_test_duration;
  
  uint16_t min_stroke_rod;
  uint16_t min_stroke;
  
  uint16_t adc_min_stroke;  
  uint16_t max_stroke;
  
  uint16_t adc_max_stroke;
  int16_t zero_offset;
  
  uint16_t debounce_in;
  uint16_t delay_out;
  
} tuning_t;

extern tuning_t global_tuning;

extern identification_t identification;

extern const settings_t default_settings;

extern settings_t settings;

void load_settings();
int get_index_last_record(uint32_t sector_addr, uint32_t record_size, uint32_t sector_size);
uint32_t* get_addr_last_record(uint32_t sector_addr, uint32_t record_size, uint32_t sector_size);
uint32_t sector_addr_by_index(uint8_t index);
int Flash_WriteStruct(const void* record, uint8_t sector_index, uint32_t record_size, uint32_t sector_size);

#endif /* __SETTINGS__ */