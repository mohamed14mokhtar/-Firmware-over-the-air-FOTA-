
#ifndef BOOTLOADER_H
#define BOOTLOADER_H

/* ----------------- Includes -----------------*/
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "usart.h"
#include "crc.h"

/* ----------------- Macro Declarations -----------------*/
#define BL_DEBUG_UART                &huart1
#define BL_HOST_COMMUNICATION_UART   &huart2
#define CRC_ENGINE_OBJ               &hcrc

#define DEBUG_INFO_DISABLE           0
#define DEBUG_INFO_ENABLE            1
#define BL_DEBUG_ENABLE              DEBUG_INFO_ENABLE

#define BL_HOST_BUFFER_RX_LENGTH     80

#define BL_ENABLE_UART_DEBUG_MESSAGE 0x00
#define BL_ENABLE_SPI_DEBUG_MESSAGE  0x01
#define BL_ENABLE_CAN_DEBUG_MESSAGE  0x02
#define BL_DEBUG_METHOD (BL_ENABLE_UART_DEBUG_MESSAGE)


/* Get Read Protection Status */
#define CBL_GO_TO_ADDR_CMD           0x14
#define CBL_FLASH_ERASE_CMD          0x15
#define CBL_MEM_WRITE_CMD            0x16


/* CRC_VERIFICATION */
#define CRC_TYPE_SIZE_BYTE           4

#define CRC_VERIFICATION_FAILED      0x00
#define CRC_VERIFICATION_PASSED      0x01

#define CBL_SEND_NACK                0xAB
#define CBL_SEND_ACK                 0xCD

/* Start address of sector 2 */
#define FLASH_SECTOR2_BASE_ADDRESS   0x08008000U

#define ADDRESS_IS_INVALID           0x00
#define ADDRESS_IS_VALID             0x01

#define STM32F407_SRAM1_SIZE         (64 * 1024)

#define STM32F407_FLASH_SIZE         (1024 * 1024)
#define STM32F407_SRAM1_END          (SRAM1_BASE + STM32F407_SRAM1_SIZE)
#define STM32F407_FLASH_END          (FLASH_BASE + STM32F407_FLASH_SIZE)

/* CBL_FLASH_ERASE_CMD */
#define CBL_FLASH_MAX_SECTOR_NUMBER  12
#define CBL_FLASH_MASS_ERASE         0xFF

#define INVALID_SECTOR_NUMBER        0x00
#define VALID_SECTOR_NUMBER          0x01
#define UNSUCCESSFUL_ERASE           0x02
#define SUCCESSFUL_ERASE             0x03

#define HAL_SUCCESSFUL_ERASE         0xFFFFFFFFU

/* CBL_MEM_WRITE_CMD */
#define FLASH_PAYLOAD_WRITE_FAILED   0x00
#define FLASH_PAYLOAD_WRITE_PASSED   0x01

#define FLASH_LOCK_WRITE_FAILED      0x00
#define FLASH_LOCK_WRITE_PASSED      0x01



/* ----------------- Macro Functions Declarations -----------------*/


/* ----------------- Data Type Declarations -----------------*/

typedef enum{
	BL_NACK = 0,
	BL_OK
}BL_Status;

typedef void (*pMainApp)(void);
typedef void (*Jump_Ptr)(void);

/* ----------------- Software Interfaces Declarations -----------------*/
void BL_Print_Message(char *format, ...);
BL_Status BL_UART_Fetch_Host_Command(void);

#endif /* BOOTLOADER_H */
