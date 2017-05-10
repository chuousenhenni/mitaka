/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	GPS.c
 *	概要
 *  GPS制御部
 *
 *
 ********************************************************************************************
 */
/*
 *===========================================================================================
 *					Includes
 *===========================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/time.h>

#include <stdint.h>

#define STRICT
#include <stdio.h>
#include <conio.h> //for kbhit
#include <string.h>
#include <time.h>

#ifdef windows
#include <unistd.h>
#include <windows.h>
#else //yamazaki*
#include "app.h"
//#include "app_commands.h"
#include "system_definitions.h"
#include "peripheral/devcon/plib_devcon.h"
#include "peripheral/rtcc/plib_rtcc.h"
#include <xc.h>
#include <stdio.h>
#endif

#include "define.h"
#include "app.h"
#include "maintenance.h"
#include "queue.h"
#include "monitor.h"
#include "common.h"

/*
 *===========================================================================================
 *					内部定数定義・型定義・マクロ定義
 *===========================================================================================
 */
 
/* GPSへのデータ送信処理の状態変数 */
enum GPSSendStatus{
	MASTER_RESET_SEND = 0,
	WAITING_MASTER_RESET_ACK,
	WAITING_GPS_DATA,
	GP2_SEND,
	WAITING_GP2_ACK,
	LOCATION_SEND,
	WAITING_LOCATION_ACK,
	IDLE
};

/* GPSデータ受信処理の状態変数 */
enum GPSRcvStatus{
	WAITING_HEADER = 0,
	WAITING_CRLF
};


/*
 *===========================================================================================
 *					内部変数定義
 *===========================================================================================
 */

int gps_send_stage_no = MASTER_RESET_SEND;/* GPSへの送信状態 */
int check_gps_timer;/* GPS受信タイムアウト判定用タイマ */
int gps_rcv_count = 0;
int gps_master_reset_rcv_count = 0;
int gps_gp2_rcv_count = 0;
int gps_location_rcv_count = 0;
char gps_rcv_buff[256];
int gps_rcv_buff_p = 0;/* 受信バッファ格納ポインタ */
int gps_rcv_stage_no = WAITING_HEADER;/* 受信待ち状態変数 */

char maser_reset_send_data[256];
char gp2_send_data[256];
char location_send_data[256];

/*
 *===========================================================================================
 *					外部変数定義
 *===========================================================================================
 */

extern HANDLE hComm1;       /* シリアルポートのハンドル */

/*
 *===========================================================================================
 *					内部	関数定義
 *===========================================================================================
 */

void GPSSend(void);/* GPSに対しての送信処理 */
void GPSRcv(void);/* GPSからのデータの受信処理 */
int CalcBcc(char *p, int size);/* BCCの計算処理 */
int CheckGPSData(char *p);/* GPSデータ受信処理 */

/*
 *===========================================================================================
 *					外部	関数定義
 *===========================================================================================
 */

extern void Send(HANDLE h, unsigned char *p, int size);

void GPSInit(void)
{
	check_gps_timer = 0;/* GPS受信タイムアウト判定用タイマ */
}


/**
 *	@brief GPS読み込み処理
 *
 *	@retval
 */
void GPSSrv(void)
{
	GPSSend();	
	GPSRcv();
}

/**
 *	@brief GPSに対しての送信処理
 *
 *	@retval なし
 */
void GPSSend(void)
{
	char str[256];
			char str2[10];
	/* ＰＪＲＣＩ，ＧＰ － 初期位置、時刻設定（固定長センテンス 71 バイト） */
	//char s1[] = "$PJRCI,GP,llll.ll,a,yyyyy.yy,a,uxxxx,hhmmss,xx,xx,xxxx,A,A,A,A,A,A*hh\r\n";
	/* マスターリセット */
	char maser_reset[] = "PJRCI,GP,0000.00,0,00000.00,0,00000,000000,00,00,0000,V,V,V,A,V,V";
	/* ＰＪＲＣＥ，ＧＰ，２ － 任意センテンス出力設定（固定長センテンス 21 バイト）*/
	//char s1[] = "$PJRCE,GP,2,xx,x*hh\r\n";
	/* 1秒間隔の送信をGPSに設定する */
	char gp2[] = "PJRCE,GP,2,00,1";
	/* 一回データ受信後に位置時刻を設定する */
	char location[] = "PJRCI,GP,llll.ll,a,yyyyy.yy,a,uxxxx,hhmmss,xx,xx,xxxx,A,A,A,V,V,V";
	
	int bcc;
	switch (gps_send_stage_no) {
		case MASTER_RESET_SEND:
			bcc = CalcBcc(maser_reset,strlen(maser_reset));
			sprintf(str2, "%.2X", bcc);
			str2[2] = '\0';
			strcpy(str,"$");
			strcat(str,maser_reset);
			strcat(str,"*");
			strcat(str,str2);
			strcat(str,"\r\n");
			strcpy(maser_reset_send_data, str);
			Send(hComm1, (unsigned char *)&str, strlen(str));
			check_gps_timer = 0;
			DebugPrint("","GPSSend マスターリセット送信", 1);
			gps_send_stage_no = WAITING_MASTER_RESET_ACK;
			break;
		case WAITING_MASTER_RESET_ACK:
			if (gps_master_reset_rcv_count != 0) {
				gps_send_stage_no = WAITING_GPS_DATA;
				DebugPrint("","GPSSend マスターリセット受信", 1);
			} else if (check_gps_timer >= 2) {
				gps_send_stage_no = MASTER_RESET_SEND;
			}
			break;
		case WAITING_GPS_DATA:
			if (gps_rcv_count != 0) {
				gps_send_stage_no = GP2_SEND;
			DebugPrint("","GPSSend GPSデータ受信", 1);
			}
			break;
		case GP2_SEND:
			bcc = CalcBcc(gp2,strlen(gp2));
			sprintf(str2, "%.2X", bcc);
			str2[2] = '\0';
			strcpy(str,"$");
			strcat(str,gp2);
			strcat(str,"*");
			strcat(str,str2);
			strcat(str,"\r\n");
			strcpy(gp2_send_data, str);
			Send(hComm1, (unsigned char *)&str, strlen(str));
			check_gps_timer = 0;
			DebugPrint("","GPSSend GP2送信", 1);
			gps_send_stage_no = WAITING_GP2_ACK;
			break;
		case WAITING_GP2_ACK:
			if (gps_location_rcv_count != 0) {
				gps_send_stage_no = LOCATION_SEND;
				DebugPrint("","GPSSend GP2受信", 1);
			}
			break;
		case LOCATION_SEND:
			bcc = CalcBcc(location,strlen(location));
			sprintf(str2, "%.2X", bcc);
			str2[2] = '\0';
			strcpy(str,"$");
			strcat(str,location);
			strcat(str,"*");
			strcat(str,str2);
			strcat(str,"\r\n");
			strcpy(location_send_data, str);
			Send(hComm1, (unsigned char *)&str, strlen(str));
			check_gps_timer = 0;
			DebugPrint("","GPSSend 位置情報送信", 1);
			gps_send_stage_no = WAITING_LOCATION_ACK;
			break;
		case WAITING_LOCATION_ACK:
			if (gps_location_rcv_count != 0) {
				gps_send_stage_no = IDLE;
				DebugPrint("","GPSSend 位置情報受信", 1);
			}
			break;
		case IDLE:
			break;
		default:
			break;
	}
}

/**
 *	@brief GPSデータ受信処理
 *
 *	@retval なし
 */
int CheckGPSData(char *p)
{
//  char s1[] = "$PJRCI,GP,llll.ll,a,yyyyy.yy,a,uxxxx,hhmmss,xx,xx,xxxx,A,A,A,A,A,A*hh\r\n";
//    char s1[] = "$PJRCI,GP,llll.ll,a,yyyyy.yy,a,uxxxx,123456,20,08,2016,A,A,A,A,A,A*hh\r\n";
//12時34分56秒 2016年8月20日
//	char s2[] = ",";  /* 空白+ハイフン+ピリオド */

	int command_count = 0;
	char command [ 20 ][ 20 ];
	char str [ 20 ][ 20 ];
	char *tok = strtok( p, "," );
	while( tok != NULL ){
		strcpy(command[command_count], tok );
		printf("--%s--\n",command[command_count]);
		command_count++;
		if (command_count > 20) {
			printf("CheckGPSData buffer over flow\n");
			command_count--;
		}
		tok = strtok( NULL, ",*" );
	}
	str[0][2]=0;
	str[1][2]=0;
	str[2][2]=0;
	strncpy(str[0],&command[7][0],2);
	strncpy(str[1],&command[7][2],2);
	strncpy(str[2],&command[7][4],2);
	printf( "command_count=%d\n",command_count);
	printf("%s年%s月%s日%s時%s分%s秒\n"
		,command[10]
		,command[9]
		,command[8]
		,str[0]
		,str[1]
		,str[2]
	);
	
	int year=strtol(command[10], 0, 16);
	int month=strtol(command[9], 0, 16);
	int day=strtol(command[8], 0, 16);
	int hour=strtol(str[0], 0, 16);
	int min=strtol(str[1], 0, 16);
	int sec=strtol(str[2], 0, 16);
	printf("%04X年%0X月%02X日%02X時%02X分%02X秒\n"
		,year
		,month
		,day
		,hour
		,min
		,sec
	);
	
	
  return 0;
}
	
/**
 *	@brief GPSからのデータの受信処理
 *
 *	@retval なし
 */
void GPSRcv(void)
{
	char str[256];
	
	while(!empty(CONTROLER_QUEUE)) {
		unsigned char d = dequeue(CONTROLER_QUEUE);//受信データ取り出し
		switch (gps_rcv_stage_no) {
		case WAITING_HEADER:
			if ( d == '$') {
				gps_rcv_buff_p = 0;
				gps_rcv_buff[ gps_rcv_buff_p ]= d ;
				gps_rcv_buff_p++ ;
				gps_rcv_stage_no = WAITING_CRLF;
			} else {
				sprintf(str, "$ error [%.2X]", d);
				DebugPrint("", str, 1);
			}
			break;
		case WAITING_CRLF:
			gps_rcv_buff[ gps_rcv_buff_p ]= d ;
			gps_rcv_buff_p++;
			if ( d == '\n'){
				/* GPSデータ受信 */
				gps_rcv_buff[gps_rcv_buff_p] = '\0';
				strcpy(str, "GPSデータ受信 ");
				strcat(str, gps_rcv_buff);
				DebugPrint("", str, 1);
				CalcBcc(gps_rcv_buff,gps_rcv_buff_p-5);
				if (strcmp(gps_rcv_buff, maser_reset_send_data) == 0) {
					gps_master_reset_rcv_count++;
				} else if (strcmp(gps_rcv_buff, gp2_send_data) == 0) {
					gps_gp2_rcv_count++;
				} else if (strcmp(gps_rcv_buff, location_send_data) == 0) {
					gps_location_rcv_count++;
				} else {
					CheckGPSData(gps_rcv_buff);/* GPSデータ受信処理 */
					gps_rcv_count++;
				}
				gps_rcv_stage_no = WAITING_HEADER;
				gps_rcv_buff_p = 0;
			}
			break;
		default:
				break;
		}
		if( gps_rcv_buff_p > RCV_BUFF_SIZE - 5 ) {
			DebugPrint("", "RCV_BUFF_SIZE buffer overflow error", 2);
			str[0] = '\0';
			gps_rcv_buff_p = 0;
		}
	}
}
	
