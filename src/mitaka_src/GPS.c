/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	GPS.c
 *	�T�v
 *  GPS���䕔
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
 *					�����萔��`�E�^��`�E�}�N����`
 *===========================================================================================
 */
 
/* GPS�ւ̃f�[�^���M�����̏�ԕϐ� */
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

/* GPS�f�[�^��M�����̏�ԕϐ� */
enum GPSRcvStatus{
	WAITING_HEADER = 0,
	WAITING_CRLF
};


/*
 *===========================================================================================
 *					�����ϐ���`
 *===========================================================================================
 */

int gps_send_stage_no = MASTER_RESET_SEND;/* GPS�ւ̑��M��� */
int check_gps_timer;/* GPS��M�^�C���A�E�g����p�^�C�} */
int gps_rcv_count = 0;
int gps_master_reset_rcv_count = 0;
int gps_gp2_rcv_count = 0;
int gps_location_rcv_count = 0;
char gps_rcv_buff[256];
int gps_rcv_buff_p = 0;/* ��M�o�b�t�@�i�[�|�C���^ */
int gps_rcv_stage_no = WAITING_HEADER;/* ��M�҂���ԕϐ� */

char maser_reset_send_data[256];
char gp2_send_data[256];
char location_send_data[256];

/*
 *===========================================================================================
 *					�O���ϐ���`
 *===========================================================================================
 */

extern HANDLE hComm1;       /* �V���A���|�[�g�̃n���h�� */

/*
 *===========================================================================================
 *					����	�֐���`
 *===========================================================================================
 */

void GPSSend(void);/* GPS�ɑ΂��Ă̑��M���� */
void GPSRcv(void);/* GPS����̃f�[�^�̎�M���� */
int CalcBcc(char *p, int size);/* BCC�̌v�Z���� */
int CheckGPSData(char *p);/* GPS�f�[�^��M���� */

/*
 *===========================================================================================
 *					�O��	�֐���`
 *===========================================================================================
 */

extern void Send(HANDLE h, unsigned char *p, int size);

void GPSInit(void)
{
	check_gps_timer = 0;/* GPS��M�^�C���A�E�g����p�^�C�} */
}


/**
 *	@brief GPS�ǂݍ��ݏ���
 *
 *	@retval
 */
void GPSSrv(void)
{
	GPSSend();	
	GPSRcv();
}

/**
 *	@brief GPS�ɑ΂��Ă̑��M����
 *
 *	@retval �Ȃ�
 */
void GPSSend(void)
{
	char str[256];
			char str2[10];
	/* �o�i�q�b�h�C�f�o �| �����ʒu�A�����ݒ�i�Œ蒷�Z���e���X 71 �o�C�g�j */
	//char s1[] = "$PJRCI,GP,llll.ll,a,yyyyy.yy,a,uxxxx,hhmmss,xx,xx,xxxx,A,A,A,A,A,A*hh\r\n";
	/* �}�X�^�[���Z�b�g */
	char maser_reset[] = "PJRCI,GP,0000.00,0,00000.00,0,00000,000000,00,00,0000,V,V,V,A,V,V";
	/* �o�i�q�b�d�C�f�o�C�Q �| �C�ӃZ���e���X�o�͐ݒ�i�Œ蒷�Z���e���X 21 �o�C�g�j*/
	//char s1[] = "$PJRCE,GP,2,xx,x*hh\r\n";
	/* 1�b�Ԋu�̑��M��GPS�ɐݒ肷�� */
	char gp2[] = "PJRCE,GP,2,00,1";
	/* ���f�[�^��M��Ɉʒu������ݒ肷�� */
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
			DebugPrint("","GPSSend �}�X�^�[���Z�b�g���M", 1);
			gps_send_stage_no = WAITING_MASTER_RESET_ACK;
			break;
		case WAITING_MASTER_RESET_ACK:
			if (gps_master_reset_rcv_count != 0) {
				gps_send_stage_no = WAITING_GPS_DATA;
				DebugPrint("","GPSSend �}�X�^�[���Z�b�g��M", 1);
			} else if (check_gps_timer >= 2) {
				gps_send_stage_no = MASTER_RESET_SEND;
			}
			break;
		case WAITING_GPS_DATA:
			if (gps_rcv_count != 0) {
				gps_send_stage_no = GP2_SEND;
			DebugPrint("","GPSSend GPS�f�[�^��M", 1);
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
			DebugPrint("","GPSSend GP2���M", 1);
			gps_send_stage_no = WAITING_GP2_ACK;
			break;
		case WAITING_GP2_ACK:
			if (gps_location_rcv_count != 0) {
				gps_send_stage_no = LOCATION_SEND;
				DebugPrint("","GPSSend GP2��M", 1);
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
			DebugPrint("","GPSSend �ʒu��񑗐M", 1);
			gps_send_stage_no = WAITING_LOCATION_ACK;
			break;
		case WAITING_LOCATION_ACK:
			if (gps_location_rcv_count != 0) {
				gps_send_stage_no = IDLE;
				DebugPrint("","GPSSend �ʒu����M", 1);
			}
			break;
		case IDLE:
			break;
		default:
			break;
	}
}

/**
 *	@brief GPS�f�[�^��M����
 *
 *	@retval �Ȃ�
 */
int CheckGPSData(char *p)
{
//  char s1[] = "$PJRCI,GP,llll.ll,a,yyyyy.yy,a,uxxxx,hhmmss,xx,xx,xxxx,A,A,A,A,A,A*hh\r\n";
//    char s1[] = "$PJRCI,GP,llll.ll,a,yyyyy.yy,a,uxxxx,123456,20,08,2016,A,A,A,A,A,A*hh\r\n";
//12��34��56�b 2016�N8��20��
//	char s2[] = ",";  /* ��+�n�C�t��+�s���I�h */

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
	printf("%s�N%s��%s��%s��%s��%s�b\n"
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
	printf("%04X�N%0X��%02X��%02X��%02X��%02X�b\n"
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
 *	@brief GPS����̃f�[�^�̎�M����
 *
 *	@retval �Ȃ�
 */
void GPSRcv(void)
{
	char str[256];
	
	while(!empty(CONTROLER_QUEUE)) {
		unsigned char d = dequeue(CONTROLER_QUEUE);//��M�f�[�^���o��
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
				/* GPS�f�[�^��M */
				gps_rcv_buff[gps_rcv_buff_p] = '\0';
				strcpy(str, "GPS�f�[�^��M ");
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
					CheckGPSData(gps_rcv_buff);/* GPS�f�[�^��M���� */
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
	
