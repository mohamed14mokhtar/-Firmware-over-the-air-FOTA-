
/* ----------------- Includes -----------------*/
#include "bootloader.h"

/* ----------------- Static Functions Decleration -----------------*/

static void Bootloader_Jump_To_Address(uint8_t *Host_Buffer);
static void Bootloader_Erase_Flash(uint8_t *Host_Buffer);
static void Bootloader_Memory_Write(uint8_t *Host_Buffer);


static uint8_t Bootloader_CRC_Verify(uint8_t *pData, uint32_t Data_Len, uint32_t Host_CRC);
static void Bootloader_Send_ACK(uint8_t Replay_Len);
static void Bootloader_Send_NACK(void);
static uint8_t Host_Address_Verification(uint32_t Jump_Address);
static uint8_t Perform_Flash_Erase(uint8_t Sector_Numebr, uint8_t Number_Of_Sectors);


/* ----------------- Global Variables Definitions -----------------*/
static uint8_t BL_Host_Buffer[BL_HOST_BUFFER_RX_LENGTH];



/* -----------------  Software Interfaces Definitions -----------------*/

uint8_t Data_Length = 0;
BL_Status BL_UART_Fetch_Host_Command(void){
	BL_Status Status = BL_NACK;
	HAL_StatusTypeDef HAL_Status = HAL_ERROR;

	__HAL_UART_CLEAR_OREFLAG(BL_HOST_COMMUNICATION_UART);              // Clear overrun
	__HAL_UART_CLEAR_FLAG(BL_HOST_COMMUNICATION_UART, UART_FLAG_RXNE); // Clear RX buffer
	memset(BL_Host_Buffer, 0, BL_HOST_BUFFER_RX_LENGTH);               // set all value at array to 0
	/* Read the length of the command packet received from the HOST */
	//first byte receive should be one of this values
	while(!((BL_Host_Buffer[0] == 71) || (BL_Host_Buffer[0] == 6) || (BL_Host_Buffer[0] == 4))){
		HAL_Status = HAL_UART_Receive(BL_HOST_COMMUNICATION_UART, BL_Host_Buffer, 1, HAL_MAX_DELAY);

	}


	if(HAL_Status != HAL_OK){
		Status = BL_NACK;
	}
	else{
		Data_Length = BL_Host_Buffer[0];  //length of the packet
		/* Read the command packet received from the HOST */
		HAL_Status = HAL_UART_Receive(BL_HOST_COMMUNICATION_UART, &BL_Host_Buffer[1], Data_Length-1, HAL_MAX_DELAY);

		if(HAL_Status != HAL_OK){
			Status = BL_NACK;
		}
		else{
			switch(BL_Host_Buffer[1]){
				case CBL_GO_TO_ADDR_CMD:
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
					BL_Print_Message("CBL_GO_TO_ADDR_CMD \r\n");
#endif
					Bootloader_Jump_To_Address(BL_Host_Buffer);
					Status = BL_OK;
					break;
				case CBL_FLASH_ERASE_CMD:
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
					BL_Print_Message("CBL_FLASH_ERASE_CMD \r\n");
#endif
					Bootloader_Erase_Flash(BL_Host_Buffer);
					Status = BL_OK;
					break;
				case CBL_MEM_WRITE_CMD:
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
					BL_Print_Message("CBL_MEM_WRITE_CMD \r\n");
#endif
					Bootloader_Memory_Write(BL_Host_Buffer);
					Status = BL_OK;
					break;
				default:
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
					BL_Print_Message("Invalid command code received from host !! \r\n");
					BL_Print_Message("Received Command Code: 0x%02X\r\n", BL_Host_Buffer[1]);
					break;
#endif
			}
		}
	}

	return Status;
}

void BL_Print_Message(char *format, ...){
	char Messsage[100] = {0};
	/* holds the information needed by va_start, va_arg, va_end */
	va_list args;
	/* Enables access to the variable arguments */
	va_start(args, format);
	/* Write formatted data from variable argument list to string */
	vsprintf(Messsage, format, args);
#if (BL_DEBUG_METHOD == BL_ENABLE_UART_DEBUG_MESSAGE)
	/* Trasmit the formatted data through the defined UART */
	HAL_UART_Transmit(BL_DEBUG_UART, (uint8_t *)Messsage, sizeof(Messsage), HAL_MAX_DELAY);
#elif (BL_DEBUG_METHOD == BL_ENABLE_SPI_DEBUG_MESSAGE)
	/* Trasmit the formatted data through the defined SPI */
#elif (BL_DEBUG_METHOD == BL_ENABLE_CAN_DEBUG_MESSAGE)
	/* Trasmit the formatted data through the defined CAN */
#endif
	/* Performs cleanup for an ap object initialized by a call to va_start */
	va_end(args);
}

/* ----------------- Static Functions Definitions -----------------*/

static void Bootloader_Jump_To_Address(uint8_t *Host_Buffer) {
    uint32_t HOST_Jump_Address = 0;
    uint32_t APP_MSP_Value = 0;
    uint32_t APP_Reset_Hndler_Addr = 0;

    uint8_t Address_Verification = ADDRESS_IS_INVALID;

    HOST_Jump_Address = *((uint32_t *)(&Host_Buffer[2]));

    Address_Verification = Host_Address_Verification(HOST_Jump_Address);

    if (ADDRESS_IS_VALID == Address_Verification) {
        Bootloader_Send_ACK(1);

        // Extract stack pointer and reset handler from the vector table
        uint32_t APP_MSP_Value = *((uint32_t*)HOST_Jump_Address);         // MSP
        uint32_t APP_Reset_Hndler_Addr = *((uint32_t*)HOST_Jump_Address) | 1;     // Reset Handler with Thumb bit set

        __set_MSP(APP_MSP_Value);  // Set MSP

        HAL_UART_DeInit(&huart2);
        HAL_UART_DeInit(&huart1);
        HAL_RCC_DeInit();                //very important and should delete weak function
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_13);

        SysTick->CTRL = 0;
        SysTick->LOAD = 0;
        SysTick->VAL  = 0;

        // Jump to application
        void (*AppResetHandler)(void) = (void (*)(void)) APP_Reset_Hndler_Addr;
        AppResetHandler();
    }
}




static uint8_t Perform_Flash_Erase(uint8_t Sector_Numebr, uint8_t Number_Of_Sectors){
	uint8_t Sector_Validity_Status = INVALID_SECTOR_NUMBER;
	FLASH_EraseInitTypeDef pEraseInit;
	uint8_t Remaining_Sectors = 0;
	uint32_t SectorError = 0;
	HAL_StatusTypeDef HAL_Status = HAL_ERROR;

	if(Number_Of_Sectors > CBL_FLASH_MAX_SECTOR_NUMBER){
		/* Number Of sectors is out of range */
		Sector_Validity_Status = INVALID_SECTOR_NUMBER;
	}
	else{
		if((Sector_Numebr <= (CBL_FLASH_MAX_SECTOR_NUMBER - 1)) || (CBL_FLASH_MASS_ERASE == Sector_Numebr)){
			/* Check if user needs Mass erase */
			if(CBL_FLASH_MASS_ERASE == Sector_Numebr){
				pEraseInit.TypeErase = FLASH_TYPEERASE_MASSERASE; /* Flash Mass erase activation */
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
				BL_Print_Message("Flash Mass erase activation \r\n");
#endif
			}
			else{
				/* User needs Sector erase */
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
				BL_Print_Message("User needs Sector erase \r\n");
#endif
				Remaining_Sectors = CBL_FLASH_MAX_SECTOR_NUMBER - Sector_Numebr;
				if(Number_Of_Sectors > Remaining_Sectors){
					Number_Of_Sectors = Remaining_Sectors;
				}
				else { /* Nothing */ }

				pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS; /* Sectors erase only */
				pEraseInit.Sector = Sector_Numebr;        /* Initial FLASH sector to erase when Mass erase is disabled */
				pEraseInit.NbSectors = Number_Of_Sectors; /* Number of sectors to be erased. */
			}

			pEraseInit.Banks = FLASH_BANK_1; /* Bank 1  */
			pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3; /* Device operating range: 2.7V to 3.6V */

			/* Unlock the FLASH control register access */
            HAL_Status = HAL_FLASH_Unlock();
			/* Perform a mass erase or erase the specified FLASH memory sectors */
			HAL_Status = HAL_FLASHEx_Erase(&pEraseInit, &SectorError);
			if(HAL_SUCCESSFUL_ERASE == SectorError){
				Sector_Validity_Status = SUCCESSFUL_ERASE;
			}
			else{
				Sector_Validity_Status = UNSUCCESSFUL_ERASE;
			}
			/* Locks the FLASH control register access */
      HAL_Status = HAL_FLASH_Lock();
		}
		else{
			Sector_Validity_Status = UNSUCCESSFUL_ERASE;
		}
	}
	return Sector_Validity_Status;
}

static void Bootloader_Erase_Flash(uint8_t *Host_Buffer){
	uint8_t Erase_Status = 0;

#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Mass erase or sector erase of the user flash \r\n");
#endif
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
		BL_Print_Message("CRC Verification Passed \r\n");
#endif
		/* Send acknowledgement to the HOST */
		Bootloader_Send_ACK(1);
		/* Perform Mass erase or sector erase of the user flash */
		Erase_Status = Perform_Flash_Erase(Host_Buffer[2], Host_Buffer[3]);
		if(SUCCESSFUL_ERASE == Erase_Status){

#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
			BL_Print_Message("Successful Erase \r\n");
#endif
		}
		else{
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
			BL_Print_Message("Erase request failed !!\r\n");
#endif
		}
}


static uint8_t Flash_Memory_Write_Payload(uint8_t *Host_Payload, uint32_t Payload_Start_Address, uint16_t Payload_Len){
    HAL_StatusTypeDef HAL_Status = HAL_ERROR;
    uint8_t Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
    uint16_t Payload_Counter = 0;

    /* Unlock the FLASH control register access */
    HAL_Status = HAL_FLASH_Unlock();

    if(HAL_Status != HAL_OK){
        Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("FLASH_PAYLOAD_WRITE_FAILED \r\n");
#endif
    }
    else{
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("FLASH_PAYLOAD_WRITE_PASS \r\n");
#endif
        for(Payload_Counter = 0; Payload_Counter < Payload_Len; Payload_Counter += 4){  // Step by 4 to write 32-bit words

       	            HAL_Status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
       	                                                       Payload_Start_Address + Payload_Counter,
															   *((uint32_t *)(Host_Payload + Payload_Counter)));

            if(HAL_Status != HAL_OK){
                Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
                break;
            }
            else{
                Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_PASSED;
            }
        }
    }

    if((FLASH_PAYLOAD_WRITE_PASSED == Flash_Payload_Write_Status) && (HAL_OK == HAL_Status)){
        /* Locks the FLASH control register access */
        HAL_Status = HAL_FLASH_Lock();
        if(HAL_Status != HAL_OK){
            Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
        }
        else{
            Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_PASSED;
        }
    }
    else{
        Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
    }

    return Flash_Payload_Write_Status;
}


static void Bootloader_Memory_Write(uint8_t *Host_Buffer){
	uint16_t Host_CMD_Packet_Len = 0;
	uint32_t HOST_Address = 0;
	uint8_t Payload_Len = 0;
	uint8_t Address_Verification = ADDRESS_IS_INVALID;
	uint8_t Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;

#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
	BL_Print_Message("Write data into different memories of the MCU \r\n");
#endif
	Host_CMD_Packet_Len = Host_Buffer[0] + 1;
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
		BL_Print_Message("CRC Verification Passed \r\n");
#endif
		/* Send acknowledgement to the HOST */

		/* Extract the start address from the Host packet */
		HOST_Address = *((uint32_t *)(&Host_Buffer[2]));
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
		BL_Print_Message("HOST_Address = 0x%X \r\n", HOST_Address);
#endif
		/* Extract the payload length from the Host packet */
		Payload_Len = Host_Buffer[6];
		/* Verify the Extracted address to be valid address */
		Address_Verification = Host_Address_Verification(HOST_Address);
		if(ADDRESS_IS_VALID == Address_Verification){
			/* Write the payload to the Flash memory */
			Flash_Payload_Write_Status = Flash_Memory_Write_Payload((uint8_t *)&Host_Buffer[7], HOST_Address, Payload_Len);
			if(FLASH_PAYLOAD_WRITE_PASSED == Flash_Payload_Write_Status){
				/* Report payload write passed */
				Bootloader_Send_ACK(1);
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
				BL_Print_Message("Payload Valid \r\n");
#endif
			}
			else{
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
				BL_Print_Message("Payload InValid \r\n");
#endif
			}
		}
		else{
			/* Report address verification failed */
			Address_Verification = ADDRESS_IS_INVALID;
		}

}


/* ----------------- will use it later -----------------*/
static uint8_t Bootloader_CRC_Verify(uint8_t *pData, uint32_t Data_Len, uint32_t Host_CRC){
	uint8_t CRC_Status = CRC_VERIFICATION_FAILED;
	uint32_t MCU_CRC_Calculated = 0;
	uint8_t Data_Counter = 0;
	uint32_t Data_Buffer = 0;
	/* Calculate CRC32 */
	for(Data_Counter = 0; Data_Counter < Data_Len; Data_Counter++){
		Data_Buffer = (uint32_t)pData[Data_Counter];
		MCU_CRC_Calculated = HAL_CRC_Accumulate(CRC_ENGINE_OBJ, &Data_Buffer, 1);
	}
	/* Reset the CRC Calculation Unit */
  __HAL_CRC_DR_RESET(CRC_ENGINE_OBJ);
	/* Compare the Host CRC and Calculated CRC */
	if(MCU_CRC_Calculated == Host_CRC){
		CRC_Status = CRC_VERIFICATION_PASSED;
	}
	else{
		CRC_Status = CRC_VERIFICATION_FAILED;
	}

	return CRC_Status;
}

static void Bootloader_Send_ACK(uint8_t Replay_Len){
	uint8_t Ack_Value[2] = {0};
	Ack_Value[0] = CBL_SEND_ACK;
	Ack_Value[1] = Replay_Len;
	HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, (uint8_t *)Ack_Value, 2, HAL_MAX_DELAY);
}

static void Bootloader_Send_NACK(void){
	uint8_t Ack_Value = CBL_SEND_NACK;
	HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, &Ack_Value, 1, HAL_MAX_DELAY);
}


static uint8_t Host_Address_Verification(uint32_t Jump_Address){
	uint8_t Address_Verification = ADDRESS_IS_INVALID;
	if((Jump_Address >= SRAM1_BASE) && (Jump_Address <= STM32F407_SRAM1_END)){
		Address_Verification = ADDRESS_IS_VALID;
	}
	else if((Jump_Address >= FLASH_BASE) && (Jump_Address <= STM32F407_FLASH_END)){
		Address_Verification = ADDRESS_IS_VALID;
	}
	else{
		Address_Verification = ADDRESS_IS_INVALID;
	}
	return Address_Verification;
}


