/*******************************************************************************
  MPLAB Harmony Application 
  
  Company:
    Microchip Technology Inc.
  
  File Name:
    app.c

  Summary:
    Application Template

  Description:
    This file contains the application logic.
 *******************************************************************************/


// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
// DOM-IGNORE-END


// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************

#include "flash_modify.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Variable Definitions
// *****************************************************************************
// *****************************************************************************

/* Row size for MZ device is 2Kbytes */
#define DEVICE_ROW_SIZE_DIVIDED_BY_4            512
/* Page size for MZ device is 16Kbytes */
#define DEVICE_PAGE_SIZE_DIVIDED_BY_4            4096

/*****************************************************
 * Initialize the application data structure. All
 * application related variables are stored in this
 * data structure.
 *****************************************************/

/* Array in the KSEG1 RAM to store the data */
uint32_t databuff[DEVICE_ROW_SIZE_DIVIDED_BY_4] __attribute__((coherent));

/* Array in the program memory with starting address PROGRAM_FLASH_BASE_ADDRESS_VALUE to write the data */
const uint32_t dataFlash[DEVICE_PAGE_SIZE_DIVIDED_BY_4] __attribute__(( address(PROGRAM_FLASH_BASE_ADDRESS_VALUE))) =  {0};
const uint32_t dataFlash2[DEVICE_PAGE_SIZE_DIVIDED_BY_4] __attribute__(( address(PROGRAM_FLASH_BASE_ADDRESS2_VALUE))) =  {0};

APP_DATA appData = 
{

    //TODO - Initialize appData structure. 
};

// *****************************************************************************
/* Driver objects.

  Summary:
    Contains driver objects.

  Description:
    This structure contains driver objects returned by the driver init routines
    to the application. These objects are passed to the driver tasks routines.
*/


APP_DRV_OBJECTS appDrvObject;

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Routines
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
/*
  Function:
    uint32_t virtualToPhysical (uint32_t address)

  Summary:
    Converts a virtual memory address to a physical one
*/
uint32_t virtualToPhysical(uint32_t address)
{
   return (address & 0x1FFFFFFF);
}

/*******************************************************************************
/*
  Function:
    void NVMpageErase (uint32_t address)

  Summary:
    Erases a page in flash memory (16KB)
*/
void NVMpageErase(uint32_t address)
{
   // Base address of page to be erased
   PLIB_NVM_FlashAddressToModify(NVM_ID_0, virtualToPhysical(address));

   // Disable flash write/erase operations
   PLIB_NVM_MemoryModifyInhibit(NVM_ID_0);

   // Select page erase function & enable flash write/erase operations
   PLIB_NVM_MemoryOperationSelect(NVM_ID_0, PAGE_ERASE_OPERATION);

   // Allow memory modifications
   PLIB_NVM_MemoryModifyEnable(NVM_ID_0);

   // Write the unlock key sequence
   PLIB_NVM_FlashWriteKeySequence(NVM_ID_0, 0x0);
   PLIB_NVM_FlashWriteKeySequence(NVM_ID_0, 0xAA996655);
   PLIB_NVM_FlashWriteKeySequence(NVM_ID_0, 0x556699AA);

   // Start the operation
   PLIB_NVM_FlashWriteStart(NVM_ID_0);

      // Wait until the operation has completed
   while (!PLIB_NVM_FlashWriteCycleHasCompleted(NVM_ID_0));

   // Disable flash write/erase operations
   PLIB_NVM_MemoryModifyInhibit(NVM_ID_0);
   
}

/*******************************************************************************
/*
  Function:
    void NVMwriteRow(uint32_t address, uint32_t dataAddress)

  Summary:
    Writes a row in flash memory (2KB)
*/
void NVMwriteRow(uint32_t destAddr, uint32_t srcAddr)
{
   // Base address of row to be written to (destination)
   PLIB_NVM_FlashAddressToModify(NVM_ID_0, virtualToPhysical(destAddr));
   // Data buffer address (source)
   PLIB_NVM_DataBlockSourceAddress(NVM_ID_0, virtualToPhysical(srcAddr));

   // Disable flash write/erase operations
   PLIB_NVM_MemoryModifyInhibit(NVM_ID_0);

   // Select row write function & enable flash write/erase operations
   PLIB_NVM_MemoryOperationSelect(NVM_ID_0, ROW_PROGRAM_OPERATION);

   // Allow memory modifications
    PLIB_NVM_MemoryModifyEnable(NVM_ID_0);

   // Write the unlock key sequence
   PLIB_NVM_FlashWriteKeySequence(NVM_ID_0, 0xAA996655);
   PLIB_NVM_FlashWriteKeySequence(NVM_ID_0, 0x556699AA);

   // Start the operation
   PLIB_NVM_FlashWriteStart(NVM_ID_0);

}

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Routines
// *****************************************************************************
// *****************************************************************************



// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine
// *****************************************************************************
// *****************************************************************************

/******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void flash_modify_APP_Initialize ( void )
{
    /* TODO: Initialize your application's state machine and other
     * parameters.
     */

    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;
}

/********************************************************
 * Application switch press routine
 ********************************************************/



/**********************************************************
 * Application tasks routine. This function implements the
 * application state machine.
 ***********************************************************/

static unsigned int task_state = NVM_FILL_DATABUF_AND_ERASE_STATE;

//static unsigned int task_state = NVM_FILL_DATABUF_AND_ERASE_STATE;
unsigned int nowbuf[128];
int nowsize;
int flash_modify_APP_Tasks_init ( int *p , int size)
{
	task_state = NVM_FILL_DATABUF_AND_ERASE_STATE;
	int i;
	nowsize = size;
	if (size > 128) {
		printf("nowsize is over\n");
		return -1;
	}
	for(i=0;i<size;i++){
		nowbuf[i]=p[i];
	}
	return 0;
}

int flash_modify_APP_Tasks ( void )
{
   static unsigned int runCount = 0;
	int ret = -1;
   unsigned int x;
   switch(task_state)
   {
      case NVM_FILL_DATABUF_AND_ERASE_STATE:
        for (x = 0; x < nowsize; x++)
        {
            databuff[x] = nowbuf[x];
        	//printf("%.02x ",nowbuf[x]);
        }
	   	//printf("\n");

        /* Erase the page which consist of the row to be written */
        NVMpageErase(PROGRAM_FLASH_BASE_ADDRESS);
        task_state = NVM_ERASE_COMPLETION_CHECK;
        break;

      case NVM_ERASE_COMPLETION_CHECK:
        if(PLIB_NVM_FlashWriteCycleHasCompleted(NVM_ID_0))
        {
            // Disable flash write/erase operations
            PLIB_NVM_MemoryModifyInhibit(NVM_ID_0);
            task_state = NVM_ERASE_ERROR_CHECK;
        }
        break;

      case NVM_ERASE_ERROR_CHECK:
        if(PLIB_NVM_LowVoltageIsDetected(NVM_ID_0) || PLIB_NVM_WriteOperationHasTerminated(NVM_ID_0))
        {
            /* Write Failed */
            task_state = NVM_ERROR_STATE;
        }
        else
        {
            /* Erase Success */
            /* Write a row of data to PROGRAM_FLASH_BASE_ADDRESS, using databuff array as the source */
            NVMwriteRow(PROGRAM_FLASH_BASE_ADDRESS, (uint32_t)databuff);
            task_state = NVM_WRITE_COMPLETION_CHECK;
        }
        break;

      case NVM_WRITE_COMPLETION_CHECK:
        if(PLIB_NVM_FlashWriteCycleHasCompleted(NVM_ID_0))
        {
            // Disable flash write/erase operations
            PLIB_NVM_MemoryModifyInhibit(NVM_ID_0);
            task_state = NVM_WRITE_ERROR_CHECK;
        }
        break;

      case NVM_WRITE_ERROR_CHECK:
        if(!(PLIB_NVM_LowVoltageIsDetected(NVM_ID_0) || PLIB_NVM_WriteOperationHasTerminated(NVM_ID_0)))
        {            
            /* Write Success */
            /* Verify that data written to flash memory is valid (databuff array read from kseg1) */
        	//printf("nowsize =%d\n", nowsize);
//            if (!memcmp(databuff, (void *)KVA0_TO_KVA1(PROGRAM_FLASH_BASE_ADDRESS), sizeof(databuff)))
            if (!memcmp(databuff, (void *)KVA0_TO_KVA1(PROGRAM_FLASH_BASE_ADDRESS), nowsize))
            {
                task_state = NVM_SUCCESS_STATE;
            }
            else
            {
                task_state = NVM_ERROR_STATE;
            }
        }
        break;
         
      case NVM_ERROR_STATE:
        /*stay here, nvm had a failure*/
        //printf("NVM_ERROR_STATE\r\n");
		ret = 0;
        break;

      case NVM_SUCCESS_STATE:
        //printf("NVM_SUCCESS_STATE\r\n");
		ret = 1;
        break;
   }
   runCount++;
   return ret;
} 

int flash_modify_APP_Tasks2 ( void )
{
   static unsigned int runCount = 0;
	int ret = -1;
   unsigned int x;
   switch(task_state)
   {
      case NVM_FILL_DATABUF_AND_ERASE_STATE:
        for (x = 0; x < nowsize; x++)
        {
            databuff[x] = nowbuf[x];
        	//printf("%.02x ",nowbuf[x] & 0xff);
        }
	   	//printf("\n");

        /* Erase the page which consist of the row to be written */
        NVMpageErase(PROGRAM_FLASH_BASE_ADDRESS2);
        task_state = NVM_ERASE_COMPLETION_CHECK;
        break;

      case NVM_ERASE_COMPLETION_CHECK:
        if(PLIB_NVM_FlashWriteCycleHasCompleted(NVM_ID_0))
        {
            // Disable flash write/erase operations
            PLIB_NVM_MemoryModifyInhibit(NVM_ID_0);
            task_state = NVM_ERASE_ERROR_CHECK;
        }
        break;

      case NVM_ERASE_ERROR_CHECK:
        if(PLIB_NVM_LowVoltageIsDetected(NVM_ID_0) || PLIB_NVM_WriteOperationHasTerminated(NVM_ID_0))
        {
            /* Write Failed */
            task_state = NVM_ERROR_STATE;
        }
        else
        {
            /* Erase Success */
            /* Write a row of data to PROGRAM_FLASH_BASE_ADDRESS2, using databuff array as the source */
            NVMwriteRow(PROGRAM_FLASH_BASE_ADDRESS2, (uint32_t)databuff);
            task_state = NVM_WRITE_COMPLETION_CHECK;
        }
        break;

      case NVM_WRITE_COMPLETION_CHECK:
        if(PLIB_NVM_FlashWriteCycleHasCompleted(NVM_ID_0))
        {
            // Disable flash write/erase operations
            PLIB_NVM_MemoryModifyInhibit(NVM_ID_0);
            task_state = NVM_WRITE_ERROR_CHECK;
        }
        break;

      case NVM_WRITE_ERROR_CHECK:
        if(!(PLIB_NVM_LowVoltageIsDetected(NVM_ID_0) || PLIB_NVM_WriteOperationHasTerminated(NVM_ID_0)))
        {            
            /* Write Success */
            /* Verify that data written to flash memory is valid (databuff array read from kseg1) */
        	//printf("nowsize =%d\n", nowsize);
//            if (!memcmp(databuff, (void *)KVA0_TO_KVA1(PROGRAM_FLASH_BASE_ADDRESS), sizeof(databuff)))
            if (!memcmp(databuff, (void *)KVA0_TO_KVA1(PROGRAM_FLASH_BASE_ADDRESS), nowsize))
            {
                task_state = NVM_SUCCESS_STATE;
            }
            else
            {
                task_state = NVM_ERROR_STATE;
            }
        }
        break;
         
      case NVM_ERROR_STATE:
        /*stay here, nvm had a failure*/
        //printf("NVM_ERROR_STATE\r\n");
		ret = 0;
        break;

      case NVM_SUCCESS_STATE:
        //printf("NVM_SUCCESS_STATE\r\n");
		ret = 1;
        break;
   }
   runCount++;
   return ret;
} 

/*******************************************************************************
 End of File
 */

