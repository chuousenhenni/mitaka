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

//#include <xc.h>                 /* For Nop() macro use  */
//#include "app.h"
//#include "system/debug/sys_debug.h"
//#include "system_definitions.h"
#include "mitaka_common.h"

// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************
typedef signed char _SBYTE;
typedef unsigned char _UBYTE;
typedef signed short _SWORD;
typedef unsigned short _UWORD;
typedef signed int _SINT;
typedef unsigned int _UINT;
typedef signed long _SDWORD;
typedef unsigned long _UDWORD;
typedef signed long long _SQWORD;
typedef unsigned long long _UQWORD;

// *****************************************************************************
// *****************************************************************************
// Section: Global Variable Definitions
// *****************************************************************************
// *****************************************************************************
#define FROM_ADDR_CS0  0xE0000000
#define FPGA_ADDR_CS1  0xE3FF0000
#define FPGA_ADDR_CS2  0xE3FFF000
#define FROM_SIZE       1024*1024
#define FPGA_DPRAM_SIZE	2048
#define LED_PANEL_SIZE	16*2
#define FPGA_CNT_SIZE	4096
#define FPGA_SIZE       256

#define LED_START_RDATA	0x1111
#define LED_START_GDATA	0x8888

//volatile _UWORD *flash_p1 = (_UWORD *)0xE0001554;		// Ver.1 board command addr1
//volatile _UWORD *flash_p2 = (_UWORD *)0xE0000AA8;		// Ver.1 board command addr2
volatile _UWORD *flash_p1 = (_UWORD *)0xE0000AAA;		// Ver.2 board command addr1
volatile _UWORD *flash_p2 = (_UWORD *)0xE0000554;		// Ver.2 board command addr2

volatile _UWORD *fpga_dpram_p1 = (_UWORD *)0xE3FF0000;
volatile _UWORD *fpga_dpram_p2 = (_UWORD *)0xE3FF0800;
volatile _UWORD *fpga_dpram_p3 = (_UWORD *)0xE3FF1000;
volatile _UWORD *fpga_dpram_p4 = (_UWORD *)0xE3FF1800;
volatile _UWORD *fpga_cnt_p    = (_UWORD *)0xE3FFF000;

/*
APP_Initializeにセットする
*/
short	flash_init(void)
{
    /* /EBIRP is set */
    PLIB_EBI_FlashPowerDownModeSet(EBI_ID_0,false);
    Nop();
    PLIB_EBI_FlashPowerDownModeSet(EBI_ID_0,true);
}

short	flash_write(volatile _UWORD *adr, _UWORD data)
{
	long	wait, wait2;
	short	ret;
	volatile _UWORD	v;
	ret = -1;
	*flash_p1 = 0x00AA;		/* write unlock cycle 1 */
	*flash_p2 = 0x0055;		/* write unlock cycle 2 */
	*flash_p1 = 0x00A0;		/* write program setup  */
	*adr = data;			/* write data to be programmed */
//	wait = 10000000;
	wait = 500000;
	while( wait>0 ){	/* ????? */
		v = *adr;
		if( ((v ^ data) & 0x0080) == 0 ){	/* ????DQ7?????? */
			wait = -1;
		}
		else {
			wait--;
			for( wait2=0; wait2<1000; wait2++)
				;
		}
	}
	if(wait != 0){
		ret = 0;
	}
	if (ret != 0) {
		printf("flash_write fail  adr=%X data = %X,ret=%d",adr,data,ret);
	}
	return	ret;
}

short	flash_write_buf(volatile _UWORD *adr, _UWORD *data, int size)
{
	int i;
	for( i = 0; i < size; i++) {
		flash_write(&adr[i], data[i]);
		//printf("flash_write_buf adr=%X data = %X,i=%d",adr,data[i],i);
	    PLIB_WDT_TimerClear(WDT_ID_0);/* ウォッチドッグクリア */
	}
	("flash_write_buf end\n");
}

short	flash_sector_erase(volatile _UWORD *adr)
{
	long	wait, wait2;
	_UWORD	data;
	short	ret;
	_UWORD	v;

	ret = -1;
	data = 0x0030;
	*flash_p1 = 0x00AA;		/* write unlock cycle 1 */
	*flash_p2 = 0x0055;		/* write unlock cycle 2 */
	*flash_p1 = 0x0080;		/* write setup command  */
	*flash_p1 = 0x00AA;		/* write additional unlock cycle 1 */
	*flash_p2 = 0x0055;		/* write additional unlock cycle 1 */
	*adr      = data;		/* write sector erase command */
	wait = 500000;
	while( wait>0 ){	/* ????? */
        v = *adr;
		if( (v & 0x0080) != 0 ){	/* ?????????'1' */
			wait = -1;
		}
		else {
			wait--;
			for( wait2=0; wait2<2048; wait2++)
				;
		}
	}
	if(wait != 0){
		ret = 0;
	}
	if (ret != 0) {
		printf("flash_sector_erase %X ret=%d\n",adr,ret);
	}
	return	ret;
}

short	flash_chip_erase(void)
{
		long	wait, wait2;
		_UWORD	data;
		short	ret;
		_UWORD	v;
volatile _UWORD	*adr;

	ret = -1;
	data = 0x0030;
	*flash_p1 = 0x00AA;		/* write unlock cycle 1 */
	*flash_p2 = 0x0055;		/* write unlock cycle 2 */
	*flash_p1 = 0x0080;		/* write setup command  */
	*flash_p1 = 0x00AA;		/* write additional unlock cycle 1 */
	*flash_p2 = 0x0055;		/* write additional unlock cycle 1 */
	*flash_p1 = 0x0010;		/* write chip erase command */

	adr = (_UWORD *)FROM_ADDR_CS0;
	for( wait=0; wait<1000; wait++)
		;
	wait = 500000;
//	wait = 100000;
	while( wait>0 ){	/* ????? */
		v = *adr;
		if( (v & 0x0080) != 0 ){	/* ?????????'1' */
			wait = -1;
		}
		else {
			wait--;
			for( wait2=0; wait2<8192; wait2++)
				;
		}
	}
	if(wait != 0){
		ret = 0;
	}
	return	ret;
}
