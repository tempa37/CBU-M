#include "manage_settings.h"
#include "stm32f4xx_hal.h"
#include "crc.h"
#include "string.h"
#include "cmsis_os2.h"

__root const settings_t default_settings = {
  .self_ip_addr.addr = 54962368, // 192.168.70.3
  .self_mask_addr.addr = 16777215, // 255.255.255.0
  .self_gateway_addr.addr = 21407936, // 192.168.70.1
  .mb_rate = 100,
  .mb_timeout = 50,
  .sens2_min_val = 100,
  .sens2_max_val = 3000,
  .sens3_min_lim = 50, 
  .sens3_max_lim = 750,
  .sens3_min_val = 150,
  .sens3_max_val = 650,
  .self_id = 1,
  .urm_id = 2,
  .umvh_id = 3,
  .sens1_id = 22,
  .sens2_id = 21,
  .valve1_io = 2,
  .valve2_io = 3,
  .state_out_io = 4,
  .sens3_io = 2,
  .state_in_io = 1,
  .pressure_set_point = 1000,
};

settings_t settings @ ".ccmram" = {0};

identification_t identification = {0};

tuning_t global_tuning = {0};

uint32_t sector_addr_by_index(uint8_t index) {
  switch(index) {
    case 0:  return 0x08000000;
    case 1:  return 0x08004000;
    case 2:  return 0x08008000;
    case 3:  return 0x0800C000;
    case 4:  return 0x08010000;
    case 5:  return 0x08020000;
    case 6:  return 0x08040000;
    case 7:  return 0x08060000;
    case 8:  return 0x08080000;
    case 9:  return 0x080A0000;
    case 10: return 0x080C0000;
    case 11: return 0x080E0000;
    case 12: return 0x08100000;
    case 13: return 0x08104000;
    case 14: return 0x08108000;
    case 15: return 0x0810C000;
    case 16: return 0x08110000;
    case 17: return 0x08120000;
    case 18: return 0x08140000;
    case 19: return 0x08160000;
    case 20: return 0x08180000;
    case 21: return 0x081A0000;
    case 22: return 0x081C0000;
    case 23: return 0x081E0000;
  };
  return 0;
}

#define CLEAR_WORD 0xffffffff

int get_index_last_record(uint32_t sector_addr, uint32_t record_size, uint32_t sector_size) {
  uint8_t* ptr = (uint8_t*)sector_addr;
  uint32_t* check_ptr;
  uint32_t offset = 0;
  int last_index = -1;
  
  while (offset + record_size <= sector_size) {
    check_ptr = (uint32_t*)(ptr + offset);
    if (*check_ptr == CLEAR_WORD) {
      return last_index;
    }
    last_index++;
    offset += record_size;
  }
  return last_index;
}

uint32_t* get_addr_last_record(uint32_t sector_addr, uint32_t record_size, uint32_t sector_size) {
  uint32_t addr = sector_addr;
  uint32_t last_valid_addr = 0;
  uint32_t last_sector_addr = sector_addr + sector_size;
  
  while (addr + record_size <= last_sector_addr) {
    if (*(uint32_t*)addr == CLEAR_WORD) {
      break;
    }
    last_valid_addr = addr;
    addr += record_size;
  }
  return (uint32_t*)last_valid_addr;
}

#define FLASH_CLEAR_ANY_FLAG __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP|FLASH_FLAG_OPERR|FLASH_FLAG_WRPERR|FLASH_FLAG_PGAERR|FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR)

int Flash_WriteStruct(const void* record, uint8_t sector_index, uint32_t record_size, uint32_t sector_size) {
  uint32_t count_word = record_size / 4;
  uint32_t sector_addr = sector_addr_by_index(sector_index);
  int index_record = get_index_last_record(sector_addr, record_size, sector_size);
  int next_index_record = index_record + 1;

  uint32_t next_addr = sector_addr + next_index_record * record_size;

  if (next_addr + record_size > sector_addr + sector_size) {
    FLASH_EraseInitTypeDef FLASH_EraseInitStruct;
    FLASH_EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    FLASH_EraseInitStruct.Sector = sector_index;
    FLASH_EraseInitStruct.NbSectors = 1;
    FLASH_EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    // Clear pending flags (if any)
    FLASH_CLEAR_ANY_FLAG;
    // Enable the Flash control register access
    HAL_FLASH_Unlock();
    uint32_t sector_error = 0;
    if (HAL_FLASHEx_Erase(&FLASH_EraseInitStruct, &sector_error) != HAL_OK) {
      // sector_error
      // if HAL_ERROR -> contains the configuration information on faulty sector in case of error
      // if HAL_OK -> 0xFFFFFFFFU means that all the sectors have been correctly erased
      HAL_FLASH_Lock();
      return HAL_FLASH_GetError();
    }
    HAL_FLASH_Lock();
    next_addr = sector_addr;
  }
  
  uint8_t try_count = 0;
  
  uint32_t* data = (uint32_t *)record;
  
  HAL_FLASH_Unlock();
  
  // program the Flash area word by word
  for (uint32_t i = 0; i < count_word; i++) {
    // Clear pending flags (if any)
    FLASH_CLEAR_ANY_FLAG;
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, next_addr, data[i]) == HAL_OK) {      
      if (*(__IO uint32_t *)next_addr != data[i]) {
        // Flash content doesn't match SRAM content
        HAL_FLASH_Lock();
        return(5);
      }
      next_addr += 4;
    } else {
      // Error occurred while writing data in Flash memory
      osDelay(10);
      if (try_count < 3) {
        i--;
        try_count++;
      } else {
        HAL_FLASH_Lock();
        return HAL_FLASH_GetError();        
      }
    }
  }
  HAL_FLASH_Lock();
  return 0;
}

void read_flash(uint32_t addr, uint32_t *data, uint32_t length) {
  for (int i = 0; i < length / 4; i++) {
    data[i] = *(__IO uint32_t*)addr;
    addr += 4;
  }
}

void read_flash_addr(uint32_t* addr, uint32_t *data, uint32_t length) {
  for (int i = 0; i < length / 4; i++) {
    data[i] = *addr;
    addr += 1;
  }
}

void load_settings() {
  
  identification.ToUint32 = (*(__IO uint32_t *)IDENTIFICATION_ADDR);
  
  read_flash(STATE_ADDR, (uint32_t *)&global_tuning, sizeof(tuning_t));
  
  uint32_t* current_addr = get_addr_last_record(CONFIGURATION_ADDR, sizeof(settings_t), CONFIGURATION_SECTOR_SIZE);
  uint32_t calc_crc32;
  if (current_addr == NULL) {
    memcpy(&settings, &default_settings, sizeof(settings_t));
    calc_crc32 = hw_crc32((uint8_t *)&settings, sizeof(settings_t) - 4);
    settings.crc32 = calc_crc32;
    Flash_WriteStruct(&settings, CONFIGURATION_SECTOR, sizeof(settings_t), CONFIGURATION_SECTOR_SIZE);
  } else {
    //memcpy(&settings, &current_addr, sizeof(settings_t));
    
    read_flash_addr(current_addr , (uint32_t *)&settings, sizeof(settings_t));
  }
  calc_crc32 = hw_crc32((uint8_t *)&settings, sizeof(settings_t) - 4);
  
  if (calc_crc32 != settings.crc32) {
    memcpy(&settings, &default_settings, sizeof(settings_t));
  }
}