/*******************************************************************************
 System Interrupts File

  File Name:
    system_int.c

  Summary:
    Raw ISR definitions.

  Description:
    This file contains a definitions of the raw ISRs required to support the
    interrupt sub-system.

  Summary:
    This file contains source code for the interrupt vector functions in the
    system.

  Description:
    This file contains source code for the interrupt vector functions in the
    system.  It implements the system and part specific vector "stub" functions
    from which the individual "Tasks" functions are called for any modules
    executing interrupt-driven in the MPLAB Harmony system.

  Remarks:
    This file requires access to the systemObjects global data structure that
    contains the object handles to all MPLAB Harmony module objects executing
    interrupt-driven in the system.  These handles are passed into the individual
    module "Tasks" functions to identify the instance of the module to maintain.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2011-2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
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

#include <xc.h>
#include <sys/attribs.h>
#include "app.h"
#include "system_definitions.h"
#include "peripheral/rtcc/plib_rtcc.h"

//enum _Que{//��M�f�[�^�L���[�Ɏg�p����z��ԍ�
//	CONTROLER_QUEUE = 0,/* ����@�ԒʐM uart1 */
//	PC_QUEUE,           /* �^�p�Ǘ�PC�ԒʐM uart 2 */
//	MONITOR_QUEUE,      /* �Ď��ՊԒʐM uart 3 */
//	CONSOLE_QUEUE,      /* �����e�i���XPC�ԒʐM uart4 */
//	IO_QUEUE
//};
#define CONTROLER_QUEUE (0)	/* ����@�ԒʐM uart1 */
#define PC_QUEUE (1)        /* �^�p�Ǘ�PC�ԒʐM uart 2 */
#define MONITOR_QUEUE (2)   /* �Ď��ՊԒʐM uart 3 */
#define CONSOLE_QUEUE (3)   /* �����e�i���XPC�ԒʐM uart4 */
#define IO_QUEUE (4)
#define UART1_QUEUE (5)		/* UART1�̎�M�f�[�^ kaji20170305 */
#define UART3_QUEUE (6)		/* UART3�̎�M�f�[�^ kaji20170310 */

// *****************************************************************************
// *****************************************************************************
// Section: System Interrupt Vector Functions
// *****************************************************************************
// *****************************************************************************
void __ISR(_TIMER_2_VECTOR, ipl1AUTO) _IntHandlerDrvTmrInstance0(void)
{
    tmrIntTriggered = 1;
extern volatile int timer_count;
timer_count++;
extern TimerInt(void);//yamazaki*
TimerInt();

    DRV_TMR_Tasks_ISR(sysObj.drvTmr0);

}


/**
 *	@brief ����@�ԒʐM(USART_ID_1)�̑��M���荞�ݏ���
 *
 *	@retval �Ȃ�
 */
void __ISR(_UART1_TX_VECTOR, ipl1AUTO) _IntHandlerDrvUsartTransmitInstance1(void)
{


    /* TODO: Add code to process interrupt here */
    if(PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT))
    {
        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);
        PLIB_INT_SourceDisable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);
    }

    /* Clear pending interrupt */
 //   PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);

}

/**
 *	@brief ����@�ԒʐM(USART_ID_1)�̎�M���荞�ݏ���
 *
 *	@retval �Ȃ�
 */
void __ISR(_UART1_RX_VECTOR, ipl1AUTO) _IntHandlerDrvUsartReceiveInstance1(void)
{


    /* TODO: Add code to process interrupt here */
        if (PLIB_USART_ReceiverDataIsAvailable(USART_ID_1))
        {
            /* Get the data from the buffer */
            appData.data1 = PLIB_USART_ReceiverByteReceive(USART_ID_1);
//   			enqueue(CONTROLER_QUEUE, appData.data1);//yamazaki*
   			enqueue(UART1_QUEUE, appData.data1);/* kaji20170306 */
       }

        appData.InterruptFlag1 = true;

    /* Clear pending interrupt */
    PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_RECEIVE);

}

/**
 *	@brief ����@�ԒʐM(USART_ID_1)�̃G���[���荞�ݏ���
 *
 *	@retval �Ȃ�
 */
void __ISR(_UART1_FAULT_VECTOR, ipl1AUTO) _IntHandlerDrvUsartErrorInstance1(void)
{


    /* TODO: Add code to process interrupt here */
        if (PLIB_USART_ReceiverDataIsAvailable(USART_ID_1))
        {
            /* Get the data from the buffer */
            appData.data1 = PLIB_USART_ReceiverByteReceive(USART_ID_1);
   			//enqueue(CONTROLER_QUEUE, appData.data1);//yamazaki*
       }

    /* Clear pending interrupt */
    PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_ERROR);

}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**
 *	@brief �^�p�Ǘ�PC�ԒʐM(USART_ID_2 115200bps)�̑��M���荞�ݏ���
 *
 *	@retval �Ȃ�
 */
void __ISR(_UART2_TX_VECTOR, ipl1AUTO) _IntHandlerDrvUsartTransmitInstance2(void)
{


    /* TODO: Add code to process interrupt here */
    if(PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_2_TRANSMIT))
    {
        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_2_TRANSMIT);
        PLIB_INT_SourceDisable(INT_ID_0, INT_SOURCE_USART_2_TRANSMIT);
    }

    /* Clear pending interrupt */
 //   PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_2_TRANSMIT);

}

/**
 *	@brief �^�p�Ǘ�PC�ԒʐM(USART_ID_2 115200bps)�̎�M���荞�ݏ���
 *
 *	@retval �Ȃ�
 */
void __ISR(_UART2_RX_VECTOR, ipl1AUTO) _IntHandlerDrvUsartReceiveInstance2(void)
{


    /* TODO: Add code to process interrupt here */
        if (PLIB_USART_ReceiverDataIsAvailable(USART_ID_2))
        {
            /* Get the data from the buffer */
            appData.data2 = PLIB_USART_ReceiverByteReceive(USART_ID_2);
 			enqueue(PC_QUEUE, appData.data2);
       }

        appData.InterruptFlag2 = true;

    /* Clear pending interrupt */
    PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_2_RECEIVE);

}

/**
 *	@brief �^�p�Ǘ�PC�ԒʐM(USART_ID_2 115200bps)�̃G���[���荞�ݏ���
 *
 *	@retval �Ȃ�
 */
void __ISR(_UART2_FAULT_VECTOR, ipl1AUTO) _IntHandlerDrvUsartErrorInstance2(void)
{


    /* TODO: Add code to process interrupt here */
        if (PLIB_USART_ReceiverDataIsAvailable(USART_ID_2))
        {
            /* Get the data from the buffer */
            appData.data2 = PLIB_USART_ReceiverByteReceive(USART_ID_2);
 			//enqueue(PC_QUEUE, appData.data2);
       }

    /* Clear pending interrupt */
    PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_2_ERROR);

}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**
 *	@brief �Ď��ՊԒʐM(USART_ID_3 1200bps)�̑��M���荞�ݏ���
 *
 *	@retval �Ȃ�
 */
void __ISR(_UART3_TX_VECTOR, ipl1AUTO) _IntHandlerDrvUsartTransmitInstance3(void)
{


    /* TODO: Add code to process interrupt here */
    if(PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_3_TRANSMIT))
    {
        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_3_TRANSMIT);
        PLIB_INT_SourceDisable(INT_ID_0, INT_SOURCE_USART_3_TRANSMIT);
    }

    /* Clear pending interrupt */
 //   PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_3_TRANSMIT);

}

/**
 *	@brief �Ď��ՊԒʐM(USART_ID_3 1200bps)�̎�M���荞�ݏ���
 *
 *	@retval �Ȃ�
 */
void __ISR(_UART3_RX_VECTOR, ipl1AUTO) _IntHandlerDrvUsartReceiveInstance3(void)
{


    /* TODO: Add code to process interrupt here */
        if (PLIB_USART_ReceiverDataIsAvailable(USART_ID_3))
        {
            /* Get the data from the buffer */
            appData.data3 = PLIB_USART_ReceiverByteReceive(USART_ID_3);
            //printf("%02x ",appData.data3&0xff);
// 			enqueue(MONITOR_QUEUE, appData.data3);//yamazaki*
 			enqueue(UART3_QUEUE, appData.data3);/* kaji20170310 */
       }

        appData.InterruptFlag3 = true;

    /* Clear pending interrupt */
    PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_3_RECEIVE);

}

/**
 *	@brief �Ď��ՊԒʐM(USART_ID_3 1200bps)�̃G���[���荞�ݏ���
 *
 *	@retval �Ȃ�
 */
void __ISR(_UART3_FAULT_VECTOR, ipl1AUTO) _IntHandlerDrvUsartErrorInstance3(void)
{


    /* TODO: Add code to process interrupt here */
    int error;
       if (PLIB_USART_ReceiverDataIsAvailable(USART_ID_3))
        {
            /* Get the data from the buffer */
            appData.data3 = PLIB_USART_ReceiverByteReceive(USART_ID_3);
            //printf("%02x ",appData.data3&0xff);
 			//enqueue(MONITOR_QUEUE, appData.data3);//yamazaki*
       }

    // Get the status of all errors.
    error = PLIB_USART_ErrorsGet(USART_ID_3);

    // Check if parity error is active
    if(error & USART_ERROR_PARITY)
    {
        // Parity error is active.
        SYS_MESSAGE("PARITY\r\n");
    }
    else if(error & USART_ERROR_FRAMING)
    {
        // Framing error is active.
        SYS_MESSAGE("FRAMING\r\n");
    }
    else if(error & USART_ERROR_RECEIVER_OVERRUN)
    {
        // Framing error is active.
        SYS_MESSAGE("OVERRUN\r\n");
    }
    

    /* Clear pending interrupt */
    PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_3_ERROR);

}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**
 *	@brief �����e�i���XPC�ԒʐM(USART_ID_4)�̑��M���荞�ݏ���
 *
 *	@retval �Ȃ�
 */
void __ISR(_UART4_TX_VECTOR, ipl1AUTO) _IntHandlerDrvUsartTransmitInstance0(void)
{


    /* TODO: Add code to process interrupt here */

    /* Clear pending interrupt */
    PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_4_TRANSMIT);
}

/**
 *	@brief �����e�i���XPC�ԒʐM(USART_ID_4)�̎�M���荞�ݏ���
 *
 *	@retval �Ȃ�
 */
void __ISR(_UART4_RX_VECTOR, ipl1AUTO) _IntHandlerDrvUsartReceiveInstance0(void)
{


    /* TODO: Add code to process interrupt here */
        if (PLIB_USART_ReceiverDataIsAvailable(USART_ID_4))
        {
            /* Get the data from the buffer */
            appData.data4 = PLIB_USART_ReceiverByteReceive(USART_ID_4);
 			enqueue(CONSOLE_QUEUE, appData.data4);//yamazaki*
       }

        appData.InterruptFlag4 = true;

    /* Clear pending interrupt */
    PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_4_RECEIVE);

}

/**
 *	@brief �����e�i���XPC�ԒʐM(USART_ID_4)�̃G���[���荞�ݏ���
 *
 *	@retval �Ȃ�
 */
void __ISR(_UART4_FAULT_VECTOR, ipl1AUTO) _IntHandlerDrvUsartErrorInstance0(void)
{


    /* TODO: Add code to process interrupt here */
        if (PLIB_USART_ReceiverDataIsAvailable(USART_ID_4))
        {
            /* Get the data from the buffer */
            appData.data4 = PLIB_USART_ReceiverByteReceive(USART_ID_4);
 			//enqueue(CONSOLE_QUEUE, appData.data4);//yamazaki*
       }

    /* Clear pending interrupt */
    PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_4_ERROR);

}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


 void __ISR(_RTCC_VECTOR, ipl1AUTO) _IntHandlerRTCC(void)
{
	extern void ClearSyncTimer(void);
	ClearSyncTimer();
    //SYS_MESSAGE("_RTCC_VECTOR\r\n");
    PLIB_RTCC_AlarmEnable(RTCC_ID_0); /* Enable alarm */

    PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_RTCC);


}

 void __ISR(_ETHERNET_VECTOR, ipl5AUTO) _IntHandler_ETHMAC(void)
{
    DRV_ETHMAC_Tasks_ISR((SYS_MODULE_OBJ)0);
}

/* This function is used by ETHMAC driver */
bool SYS_INT_SourceRestore(INT_SOURCE src, int level)
{
    if(level)
    {
        SYS_INT_SourceEnable(src);
    }

    return level;
}


 
/*******************************************************************************
 End of File
*/

