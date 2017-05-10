/*
 ********************************************************************************************
 *
 *	Copyright (c) 2017  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	L2_monitor.c
 *	�T�v
 *  1200bps�L�����f���ʐM �჌�C���ʐM���䕔
 *
 *  �ʐM�f�[�^�Ɂu�K���v��������O�����肷��
 *  (PPP like)
 *
 ********************************************************************************************
 *
 *
 * �t���[���\��
 *  55h AAh 55h AAh 7Eh    [body]  [C1] [C2] 7Eh
 *  <- preamble --> start  �C�Ӓ�  CCITT     end
 *                  flag           CRC-16    flag
 *                                 hi   low
 *
 *  data escape
 *    7Eh -> 7Dh, 5Eh
 *    7Dh -> 7Dh, 5Dh
 */

/*
 *===========================================================================================
 *					Includes
 *===========================================================================================
 */

#include <stdio.h>

#include "mitaka_common.h"
#include "define.h"
#include "app.h"
#include "maintenance.h"
#include "queue.h"
#include "monitor.h"
#include "common.h"
#include "L2_monitor.h"

/*
 *===========================================================================================
 *					�����萔��`�E�^��`�E�}�N����`
 *===========================================================================================
 */
#define	UART3_RCV_BUF_MAX	(128)
#define	UART3_TX_BUF_MAX	(256)
/*
param.preamble_ptn �Ɉڍs
#define	UART_PRE1			(0x55)
#define	UART_PRE2			(0xaa)
#define	UART_PRE3			(0x55)
#define	UART_PRE4			(0xaa)
*/
#define	UART_FLAG_CHAR	(0x7e)
#define	UART_ESC_CHAR	(0x7d)
#define	UART_ESC_7E		(0x5e)
#define	UART_ESC_7D		(0x5d)

enum _ST_UART_num {
	ST_UART_HUNT_FLAG = 0,
	ST_UART_BODY_NORMAL,
	ST_UART_BODY_ESC_CODE
};

/*
 *===========================================================================================
 *					�����ϐ���`
 *===========================================================================================
 */
unsigned int	uart3_rcv_timer;
unsigned int	uart3_state;
unsigned char	uart3_rcv_buf[UART3_RCV_BUF_MAX];
unsigned int	uart3_rcv_buf_w;
unsigned int	uart3_rcv_buf_r;
unsigned int	uart3_rcv_buf_len;
unsigned short	uart3_rcv_crc;

#define	UART3_RCV_TIMEOUT	(100)

unsigned int	uart3_tx_p;
unsigned int	uart3_tx_len;
unsigned char	uart3_tx_buf[UART3_TX_BUF_MAX];


#define	UART_INIT_CRC_DATA	(0xFFFF)

// CRC16�̃e�[�u�����
static const unsigned short CRC16Table[ 256 ] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0,
};

#define DI()            __builtin_disable_interrupts()
#define EI()            __builtin_enable_interrupts()

/*
 *===========================================================================================
 *					�O���ϐ���`
 *===========================================================================================
 */
extern PARAM param;/* �ݒ�l */
extern HANDLE hComm1;       /* �V���A���|�[�g�̃n���h�� */

/*
 *===========================================================================================
 *					����	�֐���`
 *===========================================================================================
 */
static int EscapeChar( unsigned char buf[], int c );
static void put_rcvbuf3(int c);
static int get_rcvbuf3(void);
static void flash_rcvbuf3(void);
static int len_rcvbuf3(void);/* kaji20170313 */
static int cmp_rcvbuf3_CRC(void);
static unsigned short getMemCRC16( unsigned short init_data, unsigned const char buf[], size_t len );
static unsigned short getStrCRC16( const char in_data[] );

#ifndef windows
#endif

/*
 *===========================================================================================
 *					�O���[�o���֐���`
 *===========================================================================================
 */
int ChkSendCom3(void);/* buffer�Ɏc�����f�[�^�̑��M���� & ���M�c��byte�����m�F���� */
void SendCom3(HANDLE h, unsigned char *p, int size);/* ����@�ԒʐM(USART_ID_3 1200bps ODD)�ւ̑��M���� */
void RcvCom3Init(void);/* �ϐ����̏����� */
void RcvCom3Srv(void);/* UART3�̎�M�f�[�^����������MONITOR_QUEUE�ɐς݂Ȃ��������@���C�����[�v�p */
void TimerIntL2_monitor(int count);/* ��M�f�[�^�r�؂�^�C�}�[���� */


/*
 *===========================================================================================
 *					�O��	�֐���`
 *===========================================================================================
 */

/*
 *===========================================================================================
 *					�O���[�o���֐�
 *===========================================================================================
 */

/**
*	@brief ��M�f�[�^�r�؂�^�C�}�[����
 *
 *	@param [count]  �J�E���g�A�b�vms�l
 *
 *	@retval �Ȃ�
 */
void TimerIntL2_monitor( int count )
{
	uart3_rcv_timer += count;
}

/**
*	@brief ����@�ԒʐM�ւ̃f�[�^�ςݍ��ݏ���
 *
 *	@param [src_data] ���M�f�[�^�i�[�|�C���^�Z���TID
 *	@param [size]     �i�[�T�C�Y
 *
 *	@retval �Ȃ�
 */
void SendCom3(HANDLE h, unsigned char *src_data, int size) {
	DWORD writesize;    /* �|�[�g�֏������񂾃o�C�g�� */
	//DebugPrint("", "����@�ԒʐM", 0x20);
	//int i;
	//for(i=0;i<size;i++) {
	//	printf("%02X ",p[i]);
	//}
	//printf("\n");

	unsigned char	c;
	int				i, len;
	unsigned short	crc;
	
	if (size > ((UART3_TX_BUF_MAX-2)/2-6)){
		printf("size over!");
		return;
	}

	len = 0;
/*	uart3_tx_buf[len++] = UART_PRE1;
	uart3_tx_buf[len++] = UART_PRE2;
	uart3_tx_buf[len++] = UART_PRE3;
	uart3_tx_buf[len++] = UART_PRE4;
*/
	uart3_tx_buf[len++] = (unsigned char)((param.preamble_ptn & 0xff00UL) >> 8);
	uart3_tx_buf[len++] = (unsigned char)((param.preamble_ptn & 0x00ffUL) >> 0);
	uart3_tx_buf[len++] = (unsigned char)((param.preamble_ptn & 0xff00UL) >> 8);
	uart3_tx_buf[len++] = (unsigned char)((param.preamble_ptn & 0x00ffUL) >> 0);
	uart3_tx_buf[len++] = UART_FLAG_CHAR;
	
	for ( i=0; i<size; i++ ) {
		len += EscapeChar( &uart3_tx_buf[len], src_data[i] );
	}
	
	crc = getMemCRC16( UART_INIT_CRC_DATA, src_data, size );
	c = (unsigned char)((crc & 0xff00UL) >> 8);
	len += EscapeChar( &uart3_tx_buf[len], c );
	c = (unsigned char)((crc & 0x00ffUL)     );
	len += EscapeChar( &uart3_tx_buf[len], c );
	
	uart3_tx_buf[len++] = UART_FLAG_CHAR;
//	printf("TX-CRC: %04x\n", crc);
	uart3_tx_len = len;

#ifdef windows	
	WriteFile(hComm1, uart3_tx_buf, len, &writesize, NULL);
#else
	/* �����Ńf�[�^�𑗐M���� */
	uart3_tx_p = 0;
//	uart3_tx_len = size;
//	memmove(uart3_tx_buf, src_data, len);
	//printf("SendCom3 size=%d\n",size);
	/* Write a character at a time, only if transmitter is empty */
	while (PLIB_USART_TransmitterIsEmpty(USART_ID_3)) {
		/* Send character */
//		PLIB_USART_TransmitterByteSend(USART_ID_3, src_data[i]);
		PLIB_USART_TransmitterByteSend(USART_ID_3, uart3_tx_buf[uart3_tx_p]);
		//printf("%02x ",p[i]);
		/* Increment to address of next character */
		uart3_tx_p++;
//		if (i == size) {
		if (uart3_tx_p == uart3_tx_len) {
			//printf("\n");
			return;
		}
	}
//	printf("mdm:%d,%d,%d\n", uart3_tx_p,uart3_tx_len,size);
#endif
	
	
}

int ChkSendCom3(void)
{
#ifndef windows	
	while (PLIB_USART_TransmitterIsEmpty(USART_ID_3)) {
		/* Send character */
		if (uart3_tx_p == uart3_tx_len) {
			return 0;
		}
		PLIB_USART_TransmitterByteSend(USART_ID_3, uart3_tx_buf[uart3_tx_p]);
		uart3_tx_p++;
	}
//	printf("mdm:%d\n", uart3_tx_p);
#endif
	return	(uart3_tx_len - uart3_tx_p);
}

void RcvCom3Init(void)
{
	uart3_rcv_timer = 0;
	flash_rcvbuf3();
	uart3_tx_p = 0;
	uart3_tx_len = 0;
}

/*
	��M�f�[�^����t���O�����o���ăp�P�b�g�̓��o�����s���A
	�G�X�P�[�v�������������A
	�I���̃t���O������������CRC��r����OK�ł����
	��ʑw��CONTROLER_QUEUE�o�R�Ńp�P�b�g��n��
*/
void RcvCom3Srv(void)
{
	int	rx_char, c, l;

//	printf("%d ", lenqueue(UART3_QUEUE) );
	while(!empty(UART3_QUEUE)) {
//		printf("*");
		rx_char = dequeue(UART3_QUEUE);
//		enqueue(MONITOR_QUEUE, rx_char);
//		continue;

		if (UART3_RCV_TIMEOUT < uart3_rcv_timer) {
//			printf("*** uart3_rx_timeout\n");
			flash_rcvbuf3();
		}
		uart3_rcv_timer = 0;
		
		switch (uart3_state) {
			case ST_UART_HUNT_FLAG:
				if (UART_FLAG_CHAR == rx_char) {
//					printf("*** uart3_rx_hunt flag1\n");
					uart3_state = ST_UART_BODY_NORMAL;
				}
				break;
			
			case ST_UART_BODY_NORMAL:
				switch (rx_char) {
					case UART_FLAG_CHAR:
						// ������CRC check��ǉ�
						if (len_rcvbuf3() >= 12) {/* kaji20170313 �Œᒷ��header+CRC��12byte�Ƃ��� */
							if (cmp_rcvbuf3_CRC() != 0) {
								l = 0;
								while ((c = get_rcvbuf3()) != EOF) {
									enqueue(MONITOR_QUEUE, c);
									l++;
//									printf("%02X ", c);
								}
//								printf("  len:%d\n", l);
// kaji20170313��
							} else {
								/* --- CRC error --- */
								l = 0;
								while ((c = get_rcvbuf3()) != EOF) {
									l++;
									printf("%02X ", c);
								}
								printf("  len:%d (CRC error)\n", l);
// kaji20170313��
							}
						}
						flash_rcvbuf3();
//						uart3_state = ST_UART_HUNT_FLAG;
						uart3_state = ST_UART_BODY_NORMAL;/* kaji20170313 */
						break;
					case UART_ESC_CHAR:
						uart3_state = ST_UART_BODY_ESC_CODE;
						break;
					default:
						put_rcvbuf3(rx_char);
						break;
				}
				break;
			
			case ST_UART_BODY_ESC_CODE:
				switch (rx_char) {
					case UART_ESC_7E:
						put_rcvbuf3(0x7e);
						uart3_state = ST_UART_BODY_NORMAL;
						break;
					case UART_ESC_7D:
						put_rcvbuf3(0x7d);
						uart3_state = ST_UART_BODY_NORMAL;
						break;
					case ST_UART_HUNT_FLAG:
						printf("*** uart3_rx_illigal_esc_data\n");
						flash_rcvbuf3();
//						uart3_state = ST_UART_HUNT_FLAG;
						break;
					default:
						printf("*** uart3_rx_illigal_esc_data\n");
						flash_rcvbuf3();
//						uart3_state = ST_UART_BODY_NORMAL;
						break;
				}
				break;
			
			default:
				printf("*** uart3_rx_illigal_state\n");
				flash_rcvbuf3();
//				uart3_state = ST_UART_HUNT_FLAG;
				break;
		}
	}
}


/*
 *===========================================================================================
 *					�����֐�
 *===========================================================================================
 */
static int EscapeChar( unsigned char buf[], int c )
{
	int		len;

	len = 0;
	switch (c) {
		case UART_FLAG_CHAR:
			buf[len++] = UART_ESC_CHAR;
			buf[len++] = UART_ESC_7E;
			break;
		case UART_ESC_CHAR:
			buf[len++] = UART_ESC_CHAR;
			buf[len++] = UART_ESC_7D;
			break;
		default:
			buf[len++] = c;
			break;
	}
	return	len;
}

static void flash_rcvbuf3(void)
{
	uart3_state			= ST_UART_HUNT_FLAG;
	uart3_rcv_buf_w		= 0;
	uart3_rcv_buf_r		= 0;
	uart3_rcv_buf_len	= 0;
	uart3_rcv_crc		= UART_INIT_CRC_DATA;
}

static void put_rcvbuf3(int c)
{
	if (uart3_rcv_buf_len < UART3_RCV_BUF_MAX) {
		uart3_rcv_buf[uart3_rcv_buf_w] = (unsigned char)c;
		uart3_rcv_buf_w++;
		uart3_rcv_crc = CRC16Table[ ((uart3_rcv_crc >> 8) ^ ((unsigned char)c)) & 0xff ] ^ (uart3_rcv_crc << 8);
		if (UART3_RCV_BUF_MAX <= uart3_rcv_buf_w) {
			uart3_rcv_buf_w = 0;
		}
		uart3_rcv_buf_len++;
	} else {
		/* buffer over flow �΍� */
		flash_rcvbuf3();
	}
}

static int get_rcvbuf3(void)
{
	int	c;
	if (uart3_rcv_buf_len > 0) {
		c = uart3_rcv_buf[uart3_rcv_buf_r];
		uart3_rcv_buf_r++;
		if (UART3_RCV_BUF_MAX <= uart3_rcv_buf_r) {
			uart3_rcv_buf_r = 0;
		}
		uart3_rcv_buf_len--;
	} else {
		c = EOF;
	}
	return	c;
}

static int len_rcvbuf3(void)/* kaji20170313 */
{
	return	uart3_rcv_buf_len;
}

static int cmp_rcvbuf3_CRC(void)
{
	int		ret_code;
	unsigned char	str[80];
	
	if (uart3_rcv_crc == 0) {
		ret_code = 1;
	} else {
		ret_code = 0;
		sprintf( str, "RCV_CRC_ERROR: %04x", uart3_rcv_crc );
		DebugPrint("L2-monitor", str, 1);
	}
	return	ret_code;
}
	
// ��������CRC16�R�[�h���v�Z
static unsigned short getMemCRC16( unsigned short init_data, unsigned const char buf[], size_t len )
{
	unsigned short crc;
	
	crc = init_data;
    while( len != 0 ) {
    	crc = CRC16Table[ ((crc >> 8) ^ *buf) & 0xff ] ^ (crc << 8);
        buf++;
        len--;
    }
    return crc;
}

// �������CRC16�R�[�h���v�Z
static unsigned short getStrCRC16( const char in_data[] )
{
    unsigned const char *buf = (unsigned const char *)in_data;
    
    return getMemCRC16( 0xFFFF, buf, strlen(in_data) ) ^ 0x0000;
}

