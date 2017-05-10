/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	io.h
 *  IO制御部
 *
 *	ファイルの詳細を記述する
 *
 *
 *
 *
 *
 *
 *
 ********************************************************************************************
 */

#ifndef	___IO_H___
#define	___IO_H___

/*
 *===========================================================================================
 *					定数定義・型定義・マクロ定義
 *===========================================================================================
 */

/* SWポートのビットアサイン */
#define SW_STATUS_BIT (1)/* 手動:1/遠隔:0  */
#define SW_TUUJOU_BIT (2)/* 通常 */
#define SW_ISSOU_BIT (4)/* 一掃 */
#define SW_HENNI_BIT (8)/* 変移 */
#define SW_START_BIT (0x10)/* 起動 */
#define SW_ERROR_RECOVERT_BIT (0x20)/* 異常復帰 */
#define SW_KEIKOUTOU_BIT (0x40)/* 蛍光灯入:1/自動:0 */
#define SW_TEIDEN_HUKKI_BIT (0x80)/* 停電復帰 */
#define SW_BYOU_BIT (0x300)/* 発光鋲1,2異常:1,2,3/正常:0 */
#define SW_BYOU1_BIT (0x100)/* 発光鋲1異常:1/正常:0 */
#define SW_BYOU2_BIT (0x200)/* 発光鋲2異常:1/正常:0 */
#define SW_CDS_BIT (0x400)/* CDSステータス */
#define SW_TEIDEN_BIT (0x800)/* 停電検出 */
#define SW_MAINTENANCE_BIT (0x1000)/* 保守:1/通常:0 */

#define CHOUKOU_BIT (8)/* 調光制御　蛍光灯　FLの事？ */

/* 監視盤ポートのビットアサイン */
#define MAINTENANCEL_BIT (4)/* 保守ボタン */
#define CAMMAND_BIT (1)/* 運用停止ボタン */
#define BUZZER_STOP_BIT (2)/* ブザー停止ボタン */

#define ALARM_LAMP_LED_BIT (1)/* 警報ランプ */
#define UNYO_TEISHI_LED_BIT (2)/* 運用停止LED */
#define BUZZER_TEISHI_LED_BIT (4)/* ブザー停止LED */
#define ALARM_BUZZER_BIT (8)/* 警報ブザー  */

/* 制御機LEDの配列番号、ビット位置 */
enum _Led_ArrayNo{
	LED_YOBI1 = 0,
	LED_TUUJOU,
	LED_ISSOU,
	LED_HENNI,
	LED_YOBI5,
	LED_TEST,
	LED_ZUGARA,
	LED_YOBI8,
	LED_BOARD,
	LED_BYOU,
	LED_DENSOU,
	LED_MUSEN,
	LED_TEIDEN
};


/*
 *===========================================================================================
 *					関数プロトタイプ
 *===========================================================================================
 */

extern void TimerIntIO(int count);/* IO関連のタイマ割り込み処理 */
extern void ContIOInit(void);/* 制御機のIO初期化処理 */
extern int SWRead(void);/* SW読み込み処理 */
extern int CDSRead(void);/* CDS読み込み処理 */
extern void ChoukouWrite(int d);/* 調光強制出力処理 */
extern int BoardRead(int no);/* 可変Boardステータス読み込み処理 */
extern void BoardWrite(int no, int d);/* Boardへの出力処理 */
extern int BoardRead(int no);/* 発光鋲ステータス読み込み処理 */
extern void ByouWrite( int d);/* 発光鋲への出力処理 */
extern void NaishouWrite( int d);/* 内照板への出力処理 */
extern void ToukaWrite( int d);/* 灯火板への出力処理 */
extern void PcPower( int d);/* PCPOWERの出力処理 */
//extern void TeidenDispWrite( int d);/* 停電表示灯への出力処理 */
extern int PowerStatusRead(void);/* 停電ステータス読み込み処理 */
extern void ContLedWrite(void);/* 制御機のLED出力処理 */
extern void ContLedOut(int i,int type );/* 制御機のLED表示処理 */

extern void MonitorIOInit(void);/* 監視盤のIO初期化処理 */
extern int MonitorBtnRead(void);/* 監視盤ボタン読み込み処理 */
extern void MonitorBuzzerWrite( int d);/* 監視盤ブザーへの出力処理 */
extern void MonitorLedWrite(void);/* 監視盤のLED出力処理 */
extern void MonitorLedOut(int i,int type );/* 監視盤のLED表示処理 */

extern int FPGAVersionRead(void);/* FPGAバージョン読み込み処理 */
extern int CPUFPGAVersionRead(void);/* CPUFPGAバージョン読み込み処理 */
extern void MdmcsWrite( int data);/* MDM_CSの出力処理 */

extern unsigned int RegRead( unsigned int address);/* IOデータの入力処理 */
extern void RegWrite( unsigned int address, int data);/* IOデータの出力処理 */
extern void InitRTC(void);/* RTCのパワーオンリセット処理 */
extern void LoadRTC(TIME_INFO *t);/* 不揮発用RTCから読み込む処理 */
extern void SaveRTC(TIME_INFO *t);/* 不揮発用RTCに書き込む処理 */
extern void				SPI_E2ROM_Write( unsigned int address, unsigned char data);	/* E2PROMへの書き込み */
extern unsigned char	SPI_E2ROM_Read( unsigned int address);						/* E2PROMからの読み出し */

extern int board_reg_buf[8];

#endif	/* ___IO_H___ */
