/*
 ********************************************************************************************
 *
 *	Copyright (c) 2017  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	L2.c
 *	概要
 *  光無線通信 低レイヤ通信制御部
 *
 *  通信データに「ガワ」をつけたり外したりする
 *  (PPP like)
 *
 ********************************************************************************************
 *
 *
 * フレーム構成
 *  55h AAh 55h AAh 7Eh    [body]  [C1] [C2] 7Eh
 *  <- preamble --> start  任意長  CCITT     end
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
#include "L2.h"

/*
 *===========================================================================================
 *					内部定数定義・型定義・マクロ定義
 *===========================================================================================
 */
#define	UART1_RCV_BUF_MAX	(128)
/*
param.preamble_ptn に移行
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
 *					内部変数定義
 *===========================================================================================
 */
 unsigned int	uart1_rcv_timer;
 unsigned int	uart1_state;
 unsigned char	uart1_rcv_buf[UART1_RCV_BUF_MAX];
 unsigned int	uart1_rcv_buf_w;
 unsigned int	uart1_rcv_buf_r;
 unsigned int	uart1_rcv_buf_len;
 unsigned short	uart1_rcv_crc;

#define	UART1_RCV_TIMEOUT	(20)

#define	UART_INIT_CRC_DATA	(0xFFFF)

// CRC16のテーブル情報
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
 *					外部変数定義
 *===========================================================================================
 */
extern PARAM param;/* 設定値 */
extern HANDLE hComm1;       /* シリアルポートのハンドル */

/*
 *===========================================================================================
 *					内部	関数定義
 *===========================================================================================
 */
static int EscapeChar( unsigned char buf[], int c );
static void put_rcvbuf1(int c);
static int get_rcvbuf1(void);
static void flash_rcvbuf1(void);
static int len_rcvbuf1(void);/* kaji20170313 */
static int cmp_rcvbuf1_CRC(void);
static unsigned short getMemCRC16( unsigned short init_data, unsigned const char buf[], size_t len );
static unsigned short getStrCRC16( const char in_data[] );

#ifndef windows
#endif

/*
 *===========================================================================================
 *					グローバル関数定義
 *===========================================================================================
 */
//void SendCom1(HANDLE h, unsigned char *p, int size);/* 制御機間通信(USART_ID_1 64000bps ODD)への送信処理 */
void SendCom1(HANDLE h, unsigned char *p, int size);/* 制御機間通信(USART_ID_1 64000bps ODD)への送信処理 */
void RcvCom1Init(void);/* 変数等の初期化 */
void RcvCom1Srv(void);/* UART1の受信データを処理してCONTROLER_QUEUEに積みなおす処理　メインループ用 */
void TimerIntL2(int count);/* 受信データ途切れタイマー処理 */


/*
 *===========================================================================================
 *					外部	関数定義
 *===========================================================================================
 */

/*
 *===========================================================================================
 *					グローバル関数
 *===========================================================================================
 */

/**
*	@brief 受信データ途切れタイマー処理
 *
 *	@param [count]  カウントアップms値
 *
 *	@retval なし
 */
void TimerIntL2( int count )
{
	uart1_rcv_timer += count;
}

/**
*	@brief 制御機間通信へのデータ積み込み処理
 *
 *	@param [src_data] 送信データ格納ポインタセンサID
 *	@param [size]     格納サイズ
 *
 *	@retval なし
 */
void SendCom1(HANDLE h, unsigned char *src_data, int size) {
	DWORD writesize;    /* ポートへ書き込んだバイト数 */
	//DebugPrint("", "制御機間通信", 0x20);
	//int i;
	//for(i=0;i<size;i++) {
	//	printf("%02X ",p[i]);
	//}
	//printf("\n");

	unsigned char	buf[128], *p, c;
	int				i, len;
	unsigned short	crc;
	
	if (size > 64)
		return;
	len = 0;
/*	buf[len++] = UART_PRE1;
	buf[len++] = UART_PRE2;
	buf[len++] = UART_PRE3;
	buf[len++] = UART_PRE4;
*/
	buf[len++] = (unsigned char)((param.preamble_ptn & 0xff00UL) >> 8);
	buf[len++] = (unsigned char)((param.preamble_ptn & 0x00ffUL) >> 0);
	buf[len++] = (unsigned char)((param.preamble_ptn & 0xff00UL) >> 8);
	buf[len++] = (unsigned char)((param.preamble_ptn & 0x00ffUL) >> 0);
	buf[len++] = UART_FLAG_CHAR;
	
	for ( i=0; i<size; i++ ) {
		len += EscapeChar( &buf[len], src_data[i] );
	}
	
	crc = getMemCRC16( UART_INIT_CRC_DATA, src_data, size );
	c = (unsigned char)((crc & 0xff00UL) >> 8);
	len += EscapeChar( &buf[len], c );
	c = (unsigned char)((crc & 0x00ffUL)     );
	len += EscapeChar( &buf[len], c );
	
	buf[len++] = UART_FLAG_CHAR;
//	printf("TX-CRC: %04x\n", crc);

#ifdef windows	
	WriteFile(hComm1, buf, len, &writesize, NULL);
#else
	/* ここでデータを送信する */
	i = 0;
	//printf("SendCom1 size=%d\n",size);
	while(1) {//全部送りきるにはこれが必要
		/* Write a character at a time, only if transmitter is empty */
		while (PLIB_USART_TransmitterIsEmpty(USART_ID_1)) {
			/* Send character */
//			PLIB_USART_TransmitterByteSend(USART_ID_1, src_data[i]);
			PLIB_USART_TransmitterByteSend(USART_ID_1, buf[i]);
			//printf("%02x ",p[i]);
			/* Increment to address of next character */
			i++;
//			if (i == size) {
			if (i == len) {
				//printf("\n");
				return;
			}
		}
	}
#endif
	
	
}


void RcvCom1Init(void)
{
	uart1_rcv_timer = 0;
	flash_rcvbuf1();
}

/*
	受信データからフラグを検出してパケットの頭出しを行い、
	エスケープ文字を処理しつつ、
	終わりのフラグが見つかったらCRC比較してOKであれば
	上位層にCONTROLER_QUEUE経由でパケットを渡す
*/
void RcvCom1Srv(void)
{
	int	rx_char, c, l;

//	printf("%d ", lenqueue(UART1_QUEUE) );
	while(!empty(UART1_QUEUE)) {
//		printf("*");
		rx_char = dequeue(UART1_QUEUE);
//		enqueue(CONTROLER_QUEUE, rx_char);
//		continue;

		if (UART1_RCV_TIMEOUT < uart1_rcv_timer) {
			flash_rcvbuf1();
		}
		uart1_rcv_timer = 0;
		
		switch (uart1_state) {
			case ST_UART_HUNT_FLAG:
				if (UART_FLAG_CHAR == rx_char) {
//					printf("*** uart1_rx_hunt flag\n");
					uart1_state = ST_UART_BODY_NORMAL;
				}
				break;
			
			case ST_UART_BODY_NORMAL:
				switch (rx_char) {
					case UART_FLAG_CHAR:
						// ここにCRC checkを追加
						if (len_rcvbuf1() >= 12) {/* kaji20170313 最低長をheader+CRCで12byteとする */
							if (cmp_rcvbuf1_CRC() != 0) {
								l = 0;
								while ((c = get_rcvbuf1()) != EOF) {
									enqueue(CONTROLER_QUEUE, c);
									l++;
								}
//								printf("Len:%d\n", l);
// kaji20170313↓
							} else {
								/* --- CRC error --- */
								l = 0;
								while ((c = get_rcvbuf1()) != EOF) {
									l++;
									printf("%02X ", c);
								}
								printf("  len:%d (CRC error)\n", l);
// kaji20170313↑
							}
						}
						flash_rcvbuf1();
//						uart1_state = ST_UART_HUNT_FLAG;
						uart1_state = ST_UART_BODY_NORMAL;/* kaji20170313 */
						break;
					case UART_ESC_CHAR:
						uart1_state = ST_UART_BODY_ESC_CODE;
						break;
					default:
						put_rcvbuf1(rx_char);
						break;
				}
				break;
			
			case ST_UART_BODY_ESC_CODE:
				switch (rx_char) {
					case UART_ESC_7E:
						put_rcvbuf1(0x7e);
						uart1_state = ST_UART_BODY_NORMAL;
						break;
					case UART_ESC_7D:
						put_rcvbuf1(0x7d);
						uart1_state = ST_UART_BODY_NORMAL;
						break;
					case ST_UART_HUNT_FLAG:
						printf("*** uart1_rx_illigal_esc_data\n");
						flash_rcvbuf1();
//						uart1_state = ST_UART_HUNT_FLAG;
						break;
					default:
						printf("*** uart1_rx_illigal_esc_data\n");
						flash_rcvbuf1();
//						uart1_state = ST_UART_BODY_NORMAL;
						break;
				}
				break;
			
			default:
				printf("*** uart1_rx_illigal_state\n");
				flash_rcvbuf1();
//				uart1_state = ST_UART_HUNT_FLAG;
				break;
		}
	}
}


/*
 *===========================================================================================
 *					内部関数
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

static void flash_rcvbuf1(void)
{
	uart1_state			= ST_UART_HUNT_FLAG;
	uart1_rcv_buf_w		= 0;
	uart1_rcv_buf_r		= 0;
	uart1_rcv_buf_len	= 0;
	uart1_rcv_crc		= UART_INIT_CRC_DATA;
}

static void put_rcvbuf1(int c)
{
	if (uart1_rcv_buf_len < UART1_RCV_BUF_MAX) {
		uart1_rcv_buf[uart1_rcv_buf_w] = (unsigned char)c;
		uart1_rcv_buf_w++;
		uart1_rcv_crc = CRC16Table[ ((uart1_rcv_crc >> 8) ^ ((unsigned char)c)) & 0xff ] ^ (uart1_rcv_crc << 8);
		if (UART1_RCV_BUF_MAX <= uart1_rcv_buf_w) {
			uart1_rcv_buf_w = 0;
		}
		uart1_rcv_buf_len++;
	} else {
		/* buffer over flow 対策 */
		flash_rcvbuf1();
	}
}

static int get_rcvbuf1(void)
{
	int	c;
	if (uart1_rcv_buf_len > 0) {
		c = uart1_rcv_buf[uart1_rcv_buf_r];
		uart1_rcv_buf_r++;
		if (UART1_RCV_BUF_MAX <= uart1_rcv_buf_r) {
			uart1_rcv_buf_r = 0;
		}
		uart1_rcv_buf_len--;
	} else {
		c = EOF;
	}
	return	c;
}

static int len_rcvbuf1(void)/* kaji20170313 */
{
	return	uart1_rcv_buf_len;
}

static int cmp_rcvbuf1_CRC(void)
{
	int		ret_code;
	unsigned char	str[80];
	
	if (uart1_rcv_crc == 0) {
		ret_code = 1;
	} else {
		ret_code = 0;
		sprintf( str, "RCV_CRC_ERROR: %04x", uart1_rcv_crc );
		DebugPrint("L2", str, 1);
	}
	return	ret_code;
}
	
// メモリのCRC16コードを計算
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

// 文字列のCRC16コードを計算
static unsigned short getStrCRC16( const char in_data[] )
{
    unsigned const char *buf = (unsigned const char *)in_data;
    
    return getMemCRC16( 0xFFFF, buf, strlen(in_data) ) ^ 0x0000;
}

