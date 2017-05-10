/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	common.c
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

#include "mitaka_common.h"

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
 
/*
 *===========================================================================================
 *					内部変数定義
 *===========================================================================================
 */
 
/*
 *===========================================================================================
 *					外部変数定義
 *===========================================================================================
 */


/*
 *===========================================================================================
 *					内部	関数定義
 *===========================================================================================
 */

#ifndef windows
static void PLIB_RTCC_RTCDateTimeGet(uint32_t *date, uint32_t *time);/* RTCDate,RTCTimeの二度読み処理 */
static uint32_t PLIB_RTCC_RTCTimeGet2(void);/* RTCTimeの二度読み処理 */
#endif
/*
 *===========================================================================================
 *					グローバル関数定義
 *===========================================================================================
 */

int GetTodaysMinute(void);/* 現在時刻を分で返す処理 */
int GetTodaysSecond(void);/* 現在時刻を秒で返す処理 */
void SetTime(TIME_INFO *t);/* tをRTCに設定する処理 */
void SetNowTime(TIME_INFO *t);/* 時刻をtに設定する処理 */
int BIN(int a);/* BIN 変換 */
int BCD(int a);/* BCD 変換 */
void wait(int ms);/* 指定時間待つ処理 */
int CalcBcc(char *p, int size);/* BCCの計算処理 */
int CalcRealBcc(char *p, int size);/* BCCの計算処理 */
int subZeller( int y, int m, int d );/* Zeller の公式で週の何日目か調べる */
int LinkagePack(int linkage_status);/* linkage_statusをパックセットする処理 */
int LinkageUnPack(int linkage_status);/* linkage_statusをアンパックセットする処理 */
int CheckDiffTime(TIME_INFO *t);/* 時刻の現在時刻との比較処理 */

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
 *	@brief 現在時刻を分で返す処理
 *
 *	@retval 分
 */
int GetTodaysMinute(void)
{
	int ret;
#ifdef windows	
	time_t now = time(NULL);
	struct tm *pnow = localtime(&now);
	ret = 60 * pnow->tm_hour + pnow->tm_min;
#else
	/* ここでRTCから現在時刻を取得する */
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
 *	@brief 現在時刻を秒で返す処理
 *
 *	@retval 分
 */
int GetTodaysSecond(void)
{
	int ret;
#ifdef windows	
	time_t now = time(NULL);
	struct tm *pnow = localtime(&now);
	ret = 60 * 60 * pnow->tm_hour + 60 * pnow->tm_min + pnow->tm_sec;
#else
	/* ここでRTCから現在時刻を取得する */
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

/* BIN 変換 */
int BIN(int a) {
	return (10 * (a >> 4) + (a & 0xf));
}
	
/* BCD 変換 */
int BCD(int a) {
	return (((a/10) << 4) + a % 10);
}

/**
 *	@brief 指定時間待つ処理
 *
 *	@param [int ms] 待ち時間	(ms)
 *
 *	@retval なし
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
 *	@brief 時刻情報構造体の時刻をRTCにセットする処理
 *
 *	@param [TIME_INFO *t] 時刻情報構造体のポインタ
 *
 *	@retval なし
 */
void SetTime(TIME_INFO *t)
{
	char str[256];
	
	sprintf(str,"時刻修正　%02X%02X/%02X/%02X(%d) %02X:%02X:%02X"
		, t->year[0], t->year[1]
		, t->month, t->day, t->holiday_week.week
		, t->hour, t->min	, t->sec);
	DebugPrint("", str, 1);
	
#ifndef windows
	/* ここでRTCに時刻を設定する */
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
 *	@brief 現在の時刻を時刻情報構造体にセットする処理
 *
 *	@param [TIME_INFO *t] 時刻情報構造体のポインタ
 *
 *	@retval なし
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
	/* ここでRTCから時刻をロードする */
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
 *					内部関数
 *===========================================================================================
 */

#ifndef windows
/**
 *	@brief RTCDate,RTCTimeの二度読み処理
 *
 *	@retval BCC値
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
 *	@brief RTCTimeの二度読み処理
 *
 *	@retval BCC値
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
 *	@brief BCCの計算処理
 *
 *	@param [unit8 *p]  データ格納ポインタ
 *	@param [int size]  データサイズ
 *
 *	@retval BCC値
 */
int CalcBcc(char *p, int size){
	int bcc;
	int i;

	bcc = 0;
	for (i = 0; i < size; i++) {
		bcc ^= p[ i ];
		//printf("%X ",p[ i ]);
	}
	bcc ^= 0x55AA;/* オール０対策 */
	return bcc;
}

/**
 *	@brief BCCの計算処理
 *
 *	@param [unit8 *p]  データ格納ポインタ
 *	@param [int size]  データサイズ
 *
 *	@retval BCC値
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
// Zeller の公式で週の何日目か調べる
int subZeller( int y, int m, int d )
{
    if( m < 3 ) {
        y--; m += 12;
    }
    return ( y + y/4 - y/100 + y/400 + ( 13*m + 8 )/5 + d )%7;
}	

/**
 *	@brief linkage_statusをパックセットする処理
 *
 *	@param [int linkage_status] linkage_status
 *
 *	@retval パックされたlinkage_status
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
 *	@brief linkage_statusをアンパックセットする処理
 *
 *	@param [int linkage_status] パックされたlinkage_status
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
 *	@brief 時刻の現在時刻との比較処理
 *
 *	@param [TIME_INFO *t] 時刻構造体
 *
*	@retval 時刻の差(秒)
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
			/* 日にちをまたがっている場合はチェックしないで差を1とする */
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


