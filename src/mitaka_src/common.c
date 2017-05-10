/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	common.c
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

#include "mitaka_common.h"

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
 
/*
 *===========================================================================================
 *					�����ϐ���`
 *===========================================================================================
 */
 
/*
 *===========================================================================================
 *					�O���ϐ���`
 *===========================================================================================
 */


/*
 *===========================================================================================
 *					����	�֐���`
 *===========================================================================================
 */

#ifndef windows
static void PLIB_RTCC_RTCDateTimeGet(uint32_t *date, uint32_t *time);/* RTCDate,RTCTime�̓�x�ǂݏ��� */
static uint32_t PLIB_RTCC_RTCTimeGet2(void);/* RTCTime�̓�x�ǂݏ��� */
#endif
/*
 *===========================================================================================
 *					�O���[�o���֐���`
 *===========================================================================================
 */

int GetTodaysMinute(void);/* ���ݎ����𕪂ŕԂ����� */
int GetTodaysSecond(void);/* ���ݎ�����b�ŕԂ����� */
void SetTime(TIME_INFO *t);/* t��RTC�ɐݒ肷�鏈�� */
void SetNowTime(TIME_INFO *t);/* ������t�ɐݒ肷�鏈�� */
int BIN(int a);/* BIN �ϊ� */
int BCD(int a);/* BCD �ϊ� */
void wait(int ms);/* �w�莞�ԑ҂��� */
int CalcBcc(char *p, int size);/* BCC�̌v�Z���� */
int CalcRealBcc(char *p, int size);/* BCC�̌v�Z���� */
int subZeller( int y, int m, int d );/* Zeller �̌����ŏT�̉����ڂ����ׂ� */
int LinkagePack(int linkage_status);/* linkage_status���p�b�N�Z�b�g���鏈�� */
int LinkageUnPack(int linkage_status);/* linkage_status���A���p�b�N�Z�b�g���鏈�� */
int CheckDiffTime(TIME_INFO *t);/* �����̌��ݎ����Ƃ̔�r���� */

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
 *	@brief ���ݎ����𕪂ŕԂ�����
 *
 *	@retval ��
 */
int GetTodaysMinute(void)
{
	int ret;
#ifdef windows	
	time_t now = time(NULL);
	struct tm *pnow = localtime(&now);
	ret = 60 * pnow->tm_hour + pnow->tm_min;
#else
	/* ������RTC���猻�ݎ������擾���� */
	uint32_t data;
	data = PLIB_RTCC_RTCTimeGet2();
	ret = 60 * BIN((data >> 24) &0xff) + BIN((data >> 16) &0xff);
//	t->min = (data >> 16) &0xff;
//	t->day = (data >> 8) &0xff;
//	t->sec = (data >> 8) &0xff;
//	t->holiday_week.week = (data >> 0) &0xff;
#endif	
	return ret;
}

 /**
 *	@brief ���ݎ�����b�ŕԂ�����
 *
 *	@retval ��
 */
int GetTodaysSecond(void)
{
	int ret;
#ifdef windows	
	time_t now = time(NULL);
	struct tm *pnow = localtime(&now);
	ret = 60 * 60 * pnow->tm_hour + 60 * pnow->tm_min + pnow->tm_sec;
#else
	/* ������RTC���猻�ݎ������擾���� */
	uint32_t data;
	data = PLIB_RTCC_RTCTimeGet2();
	ret = 60 * 60 * BIN((data >> 24) &0xff) + 60 * BIN((data >> 16) &0xff) + BIN((data >> 8) &0xff);
//	t->min = (data >> 16) &0xff;
//	t->day = (data >> 8) &0xff;
//	t->sec = (data >> 8) &0xff;
//	t->holiday_week.week = (data >> 0) &0xff;
#endif	
	return ret;
}

/* BIN �ϊ� */
int BIN(int a) {
	return (10 * (a >> 4) + (a & 0xf));
}
	
/* BCD �ϊ� */
int BCD(int a) {
	return (((a/10) << 4) + a % 10);
}

/**
 *	@brief �w�莞�ԑ҂���
 *
 *	@param [int ms] �҂�����	(ms)
 *
 *	@retval �Ȃ�
 */
void wait(int ms)
{
#ifdef windows	
	
	int	start = clock();
	int	end;
	
	while(1) {
		end = clock();
		if ((end - start) > ms) {
			break;
		}
	}
#endif	
}

/**
 *	@brief �������\���̂̎�����RTC�ɃZ�b�g���鏈��
 *
 *	@param [TIME_INFO *t] �������\���̂̃|�C���^
 *
 *	@retval �Ȃ�
 */
void SetTime(TIME_INFO *t)
{
	char str[256];
	
	sprintf(str,"�����C���@%02X%02X/%02X/%02X(%d) %02X:%02X:%02X"
		, t->year[0], t->year[1]
		, t->month, t->day, t->holiday_week.week
		, t->hour, t->min	, t->sec);
	DebugPrint("", str, 1);
	
#ifndef windows
	/* ������RTC�Ɏ�����ݒ肷�� */
	PLIB_RTCC_Disable(RTCC_ID_0);
	// where, MY_RTCC_INSTANCE - is a specific instance of the hardware peripheral.
	while(PLIB_RTCC_RTCSyncStatusGet(RTCC_ID_0)); // Wait for clock to turn off
	uint32_t data;
	data = (t->year[1] << 24) + (t->month << 16) + (t->day << 8)+ (t->holiday_week.week - 1);/* week is 1-7 rtc is 0-6 */
	PLIB_RTCC_RTCDateSet (RTCC_ID_0, data );
	data = (t->hour << 24) + (t->min << 16) + (t->sec << 8) ;
	PLIB_RTCC_RTCTimeSet ( RTCC_ID_0, data );
    PLIB_RTCC_Enable(RTCC_ID_0);
	
#endif	
}

/**
 *	@brief ���݂̎������������\���̂ɃZ�b�g���鏈��
 *
 *	@param [TIME_INFO *t] �������\���̂̃|�C���^
 *
 *	@retval �Ȃ�
 */
void SetNowTime(TIME_INFO *t)
{
#ifdef windows	
	time_t now = time(NULL);
	struct tm *pnow = localtime(&now);
	t->year[0] = BCD((pnow->tm_year + 1900) / 100);
	t->year[1] = BCD((pnow->tm_year + 1900) % 100);
	t->month = BCD(pnow->tm_mon + 1);
	t->day = BCD(pnow->tm_mday);
	t->hour = BCD(pnow->tm_hour);
	t->min = BCD(pnow->tm_min);
	t->sec = BCD(pnow->tm_sec);
	t->holiday_week.week = pnow->tm_wday + 1;
#else
	/* ������RTC���玞�������[�h���� */
	uint32_t date;
	uint32_t time;
	PLIB_RTCC_RTCDateTimeGet(&date, &time);
	t->year[0] = 0x20;
	t->year[1] = (date >> 24) &0xff;
	t->month = (date >> 16) &0xff;
	t->day = (date >> 8) &0xff;
	t->holiday_week.week = ((date >> 0) & 0xff) + 1;
    //printf("PLIB_RTCC_RTCDateGet=%08X\n",date);
	t->hour = (time >> 24) &0xff;
	t->min = (time >> 16) &0xff;
	t->sec = (time >> 8) &0xff;
    //printf("PLIB_RTCC_RTCTimeGet=%08X\n",time);
#endif
}

/*
 *===========================================================================================
 *					�����֐�
 *===========================================================================================
 */

#ifndef windows
/**
 *	@brief RTCDate,RTCTime�̓�x�ǂݏ���
 *
 *	@retval BCC�l
 */
static void PLIB_RTCC_RTCDateTimeGet(uint32_t *date, uint32_t *time)
{
	uint32_t date1 = PLIB_RTCC_RTCDateGet( RTCC_ID_0 );
	uint32_t time1 = PLIB_RTCC_RTCTimeGet( RTCC_ID_0 );
	while(1) {
		uint32_t date2 = PLIB_RTCC_RTCDateGet( RTCC_ID_0 );
		uint32_t time2 = PLIB_RTCC_RTCTimeGet( RTCC_ID_0 );
		if ((date1 == date2) && (time1 == time2)){
			*date = date1;
			*time = time1;
			return;
		}
		date1 = date2;
		time1 = time2;
	}

}

/**
 *	@brief RTCTime�̓�x�ǂݏ���
 *
 *	@retval BCC�l
 */
static uint32_t PLIB_RTCC_RTCTimeGet2(void)
{
	uint32_t data = PLIB_RTCC_RTCTimeGet( RTCC_ID_0 );
	while(1) {
		uint32_t data2 = PLIB_RTCC_RTCTimeGet( RTCC_ID_0 );
		if (data == data2) {
			return data;
		}
		data = data2;
	}

}
#endif

/**
 *	@brief BCC�̌v�Z����
 *
 *	@param [unit8 *p]  �f�[�^�i�[�|�C���^
 *	@param [int size]  �f�[�^�T�C�Y
 *
 *	@retval BCC�l
 */
int CalcBcc(char *p, int size){
	int bcc;
	int i;

	bcc = 0;
	for (i = 0; i < size; i++) {
		bcc ^= p[ i ];
		//printf("%X ",p[ i ]);
	}
	bcc ^= 0x55AA;/* �I�[���O�΍� */
	return bcc;
}

/**
 *	@brief BCC�̌v�Z����
 *
 *	@param [unit8 *p]  �f�[�^�i�[�|�C���^
 *	@param [int size]  �f�[�^�T�C�Y
 *
 *	@retval BCC�l
 */
int CalcRealBcc(char *p, int size){
	int bcc;
	int i;

	bcc = 0;
	for (i = 0; i < size; i++) {
		bcc ^= p[ i ];
		//printf("%X ",p[ i ]);
	}
	return bcc;
}
// Zeller �̌����ŏT�̉����ڂ����ׂ�
int subZeller( int y, int m, int d )
{
    if( m < 3 ) {
        y--; m += 12;
    }
    return ( y + y/4 - y/100 + y/400 + ( 13*m + 8 )/5 + d )%7;
}	

/**
 *	@brief linkage_status���p�b�N�Z�b�g���鏈��
 *
 *	@param [int linkage_status] linkage_status
 *
 *	@retval �p�b�N���ꂽlinkage_status
 */
int LinkagePack(int linkage_status)
{
	int i;
	int d = 0;
	for (i = 0; i < CONTROLER_MAX ; i++) {
		d <<=1;
		int dtmp = (linkage_status >> (4*(CONTROLER_MAX - i - 1)))&0xf;
		if (dtmp != 0) {
			d |= 1;
		}
	}
	return d;
}

/**
 *	@brief linkage_status���A���p�b�N�Z�b�g���鏈��
 *
 *	@param [int linkage_status] �p�b�N���ꂽlinkage_status
 *
 *	@retval linkage_status
 */
int LinkageUnPack(int linkage_status)
{
	int i;
	int d = 0;
	for (i = 0; i < CONTROLER_MAX ; i++) {
		d <<=4;
		int dtmp = (linkage_status >> ((CONTROLER_MAX - i - 1)))&1;
		if (dtmp != 0) {
			d |= 1;
		}
	}
	return d;
}


/**
 *	@brief �����̌��ݎ����Ƃ̔�r����
 *
 *	@param [TIME_INFO *t] �����\����
 *
*	@retval �����̍�(�b)
 */
int CheckDiffTime(TIME_INFO *t)
{
	int d;
	int sec1;
	int sec2;
	TIME_INFO t2;
	SetNowTime(&t2);
	if (t->hour != t2.hour) {
		if (((t->hour == 0) || (t->hour == 0x23)) && ((t2.hour == 0) || (t2.hour == 0x23))) {
			/* ���ɂ����܂������Ă���ꍇ�̓`�F�b�N���Ȃ��ō���1�Ƃ��� */
			return 1;
		}
	}
	sec1 = 60 * 60 * BIN(t->hour) + 60 * BIN(t->min) + BIN(t->sec);
	sec2 = 60 * 60 * BIN(t2.hour) + 60 * BIN(t2.min) + BIN(t2.sec);
	if (sec1 < sec2) {
		d = sec2-sec1;
	} else {
		d = sec1-sec2;
	}
	return d;
}


