/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	io.c
 *	概要
 *  IO制御部
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
#include "cont.h"
#include "io.h"

/*
 *===========================================================================================
 *					内部定数定義・型定義・マクロ定義
 *===========================================================================================
 */

/* チャタ対策時は有効にする */
/*
チャタ対策準備用に作成したが、本調整によりその必要は無くなりました20170206
*/
//#define BORD_CHATT

/* 制御機制御レジスタ */
//#define BASE_REG (0x23FF0000)
#define BASE_REG (0xE3FF0000)
#define KAHEN_BOARD_REG		(0x00) /* 可変標識板（１～８）制御 */
#define TOUKA_BOARD_REG		(0x10) /* 灯火標識板（１～８）制御 */
#define SHADANMAKU_KYOKA_REG (0x12)/* 遮断幕許可 */
#define HAKKOUBYOU_OUT_REG	(0x16) /* 発光鋲LED制御 */
#define HAKKOUBYOU_IN_REG	(0x18) /* 発光鋲制御 */
#define PANEL_IN_REG		(0x18) /* 操作パネル制御 */
#define PANEL_OUT_REG		(0x1A) /* 操作パネルLED制御 */

/* 監視盤制御レジスタ */
#define MAP_LED_REG1		(0x40) /* 地図盤LED制御1 */
#define MAP_LED_REG2		(0x42) /* 地図盤LED制御2 */
#define MPANEL_OUT_REG		(0x44) /* 操作パネルLED制御 */
#define MPANEL_IN_REG		(0x46) /* 操作パネル制御 */

/* 共通レジスタ */
#define FPGA_VER_REG		(0x80) /* FPGA Version */
#define MODEM_REG			(0x82) /* モデム制御 */
#define SPI_REG				(0x84) /* SPI制御 */
#define FPGA_LED_REG		(0x86) /* FPGA LED制御 */
#define FPGA_SW_REG			(0x86) /* FPGA SW制御 */
#define SDISP_FPGA_VER_REG	(0x100) /* CPU側FPGA Version */
#define SDISP_FPGA_EXT_CFG	(0x104) /* CPU側FPGA 拡張設定 */
#define SDISP_LED_REG		(0x200) /* CPU側FPGA LED制御 */
#define SDISP_SW_REG		(0x200) /* CPU側FPGA SW制御 */

/* SPIレジスタ制御用 */
#define	SPI_CS_RTC			(1)
#define	SPI_CS_EEPROM		(2)
#define	SPI_CS_ENABLE		(1)
#define	SPI_CS_DISABLE		(0)

/* RTC(EPSON RX6110SAB) レジスタ */
#define	RTC_REG_SEC				(0x10 + 0x00)	
#define	RTC_REG_MIN				(0x10 + 0x01)	
#define	RTC_REG_HOUR			(0x10 + 0x02)	
#define	RTC_REG_WEEK			(0x10 + 0x03)	
#define	RTC_REG_DAY				(0x10 + 0x04)	
#define	RTC_REG_MONTH			(0x10 + 0x05)	
#define	RTC_REG_YEAR			(0x10 + 0x06)	
#define	RTC_REG_CALIB			(0x10 + 0x07)	
#define	RTC_REG_MIN_ALM			(0x10 + 0x08)	
#define	RTC_REG_HOUR_ALM		(0x10 + 0x09)	
#define	RTC_REG_WEEK_DAY_ALM	(0x10 + 0x0A)	
#define	RTC_REG_CNT0			(0x10 + 0x0B)	
#define	RTC_REG_CNT1			(0x10 + 0x0C)	
#define	RTC_REG_EXT				(0x10 + 0x0D)	
#define	RTC_REG_FLAG			(0x10 + 0x0E)	
#define	RTC_REG_CNTL			(0x10 + 0x0F)	
#define	RTC_REG_RAM_BASE		(0x20 + 0x00)	/* ユーザーレジスタ(16byte) */
#define	RTC_REG_TUNE			(0x30 + 0x00)	
#define	RTC_REG_RSV				(0x30 + 0x01)	
#define	RTC_REG_IRQ				(0x30 + 0x02)	
#define	RTC_REG_POR6_1			(0x60 + 0x00)	/* パワーオンリセット用 bank6-1 */
#define	RTC_REG_POR6_2			(0x60 + 0x06)	/* パワーオンリセット用 bank6-2 */
#define	RTC_REG_POR6_3			(0x60 + 0x0B)	/* パワーオンリセット用 bank6-3 */
#define	RTC_REG_POR6_4			(0x60 + 0x0B)	/* パワーオンリセット用 bank6-4 */

#define REG_MAX				(0x1000) /* レジスタの上限値 */

#if 0
#define KAHEN_BOARD_REG		(BASE_REG + 0x00) /* 可変標識板（１～８）制御 */
#define TOUKA_BOARD_REG		(BASE_REG + 0x10) /* 灯火標識板（１～８）制御 */
#define HAKKOUBYOU_OUT_REG	(BASE_REG + 0x16) /* 発光鋲LED制御 */
#define HAKKOUBYOU_IN_REG	(BASE_REG + 0x18) /* 発光鋲制御 */
#define PANEL_IN_REG		(BASE_REG + 0x18) /* 操作パネル制御 */
#define PANEL_OUT_REG		(BASE_REG + 0x1A) /* 操作パネルLED制御 */

/* 監視盤制御レジスタ */
#define MAP_LED_REG1		(BASE_REG + 0x40) /* 地図盤LED制御1 */
#define MAP_LED_REG2		(BASE_REG + 0x42) /* 地図盤LED制御2 */
#define MPANEL_OUT_REG		(BASE_REG + 0x44) /* 操作パネルLED制御 */
#define MPANEL_IN_REG		(BASE_REG + 0x46) /* 操作パネル制御 */

/* 共通レジスタ */
#define FPGA_VER_REG		(BASE_REG + 0x80) /* FPGA Version */
#define MODEM_REG			(BASE_REG + 0x82) /* モデム制御 */
#define SPI_REG				(BASE_REG + 0x84) /* SPI制御 */
#define FPGA_LED_REG		(BASE_REG + 0x86) /* FPGA LED制御 */
#define FPGA_SW_REG			(BASE_REG + 0x86) /* FPGA SW制御 */
#define SDISP_FPGA_VER_REG	(BASE_REG + 0x100) /* CPU側FPGA Version */
#define SDISP_LED_REG		(BASE_REG + 0x200) /* CPU側FPGA LED制御 */
#define SDISP_SW_REG		(BASE_REG + 0x200) /* CPU側FPGA SW制御 */
#endif

/*
 *===========================================================================================
 *					内部変数定義
 *===========================================================================================
 */

int board_chatter_timer[DISPLAY_BOARD_MAX];/* チャタ防止標識版ＩＯリードタイマー */

int cont_output_data;
int bef_cont_output_data;
int board_reg_buf[8];

int bef_board_status_value[8];/* 前回の標識版のステータス値 */
int board_status_value[8];/* 今回の標識版のステータス値 */

//int cds_status_val = 0;//とすると妙なLED点滅現象が起こる？？？
int cds_status_val;

int monitor_output_data;
static int bef_monitor_output_data;
static int panel_out_reg_buf;
static int bef_panel_out_reg_buf;
//static uint16_t hakkobyou_out_reg_buf;//なぜかintだとおかしくなっていた
//static uint16_t bef_hakkobyou_out_reg_buf;
int hakkobyou_out_reg_buf;//なぜかstatic intだとおかしくなっていた 20170218
int bef_hakkobyou_out_reg_buf;//なぜかstatic intだとおかしくなっていた 20170218
int touka_out_reg_buf;
int bef_touka_out_reg_buf;
static int Mdmcs_reg_buf;




/*
 *===========================================================================================
 *					外部変数定義
 *===========================================================================================
 */

extern int swtest;
extern uint32_t input_data[DISPLAY_BOARD_MAX];/* 各可変表示板からの入力データ */
extern int btn_status;/* 監視盤ボタンの状態を保持 */
extern int io_power_outage_flag;/* 停電ステータス */
extern STATUS now_status;/* 現在の設定値 */
extern IO_INFO my_io_info;/* 自制御機のIO状態管理用 内照板制御に必要 */
extern uint32_t board_status[DISPLAY_BOARD_MAX];/* 各可変表示板の正常異常状態 */

/*
 *===========================================================================================
 *					内部	関数定義
 *===========================================================================================
 */
#ifndef windows
static void				SPI_CsWrite( unsigned char dst, unsigned char mode );						/* SPIバスデバイス選択 */
static void				SPI_SendData( unsigned char data);											/* SPIバスへの送信 */
static unsigned char	SPI_RcvData( void );														/* SPIバスからの受信 */

static void				SPI_RTC_Write( unsigned int address, unsigned char data);					/* RTCへの書き込み */
static unsigned char	SPI_RTC_Read( unsigned int address);										/* RTCからの読み出し */
static void				SPI_RTC_ReadSTR( unsigned int address, unsigned char *data, int length);	/* RTCからの連続読み出し */
#endif
static int BoardRealRead(int no);/* 可変Boardステータス読み込み処理 */
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

void TimerIntIO(int count);/* IO関連のタイマ割り込み処理 */
void ContIOInit(void);/* 制御機のIO初期化処理 */
int SWRead(void);/* SW読み込み処理 */
int CDSRead(void);/* CDS読み込み処理 */
void ChoukouWrite(int d);/* 調光強制出力処理 */
int BoardRead(int no);/* 可変Boardステータス読み込み処理 */
void BoardWrite(int no, int d);/* Boardへの出力処理 */
void ByouWrite( int d);/* 発光鋲への出力処理 */
void NaishouWrite( int d);/* 内照板への出力処理 */
void ToukaWrite( int d);/* 灯火板への出力処理 */
void PcPower( int d);/* PCPOWERの出力処理 */
//void TeidenDispWrite( int d);/* 停電表示灯への出力処理 */
int PowerStatusRead(void);/* 停電ステータス読み込み処理 */
void ContLedWrite(void);/* 制御機のLED出力処理 */
void ContLedOut(int i,int type );/* 制御機のLED表示処理 */

void MonitorIOInit(void);/* 監視盤のIO初期化処理 */
int MonitorBtnRead(void);/* 監視盤ボタン読み込み処理 */
void MonitorBuzzerWrite( int d);/* 監視盤ブザーへの出力処理 */
void MonitorLedWrite(void);/* 監視盤のLED出力処理 */
void MonitorLedOut(int i,int type );/* 監視盤のLED表示処理 */

int FPGAVersionRead(void);/* FPGAバージョン読み込み処理 */
int CPUFPGAVersionRead(void);/* CPUFPGAバージョン読み込み処理 */
void MdmcsWrite( int data);/* MDM_CSの出力処理 */

unsigned int RegRead( unsigned int address);/* IOデータの入力処理 */
void RegWrite( unsigned int address, int data);/* IOデータの出力処理 */
void InitRTC(void);/* RTCのパワーオンリセット処理 */

void LoadRTC(TIME_INFO *t);/* 不揮発用RTCから読み込む処理 */
void SaveRTC(TIME_INFO *t);/* 不揮発用RTCに書き込む処理 */
void			SPI_E2ROM_Write( unsigned int address, unsigned char data);	/* E2PROMへの書き込み */
unsigned char	SPI_E2ROM_Read( unsigned int address);						/* E2PROMからの読み出し */

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
 *	@brief IO関連のタイマ割り込み処理
 *
 *	@retval なし
 */
void TimerIntIO(int count)
{
#ifdef BORD_CHATT
#define BOARD_READ_INTERVAL (10)	/* 標識盤読み込み間隔(ms) */
	int i;
	for ( i=0; i <DISPLAY_BOARD_MAX; i++) {
		board_chatter_timer[i] += count;/* チャタ防止標識版ＩＯリードタイマー */
		if(board_chatter_timer[i] > BOARD_READ_INTERVAL) {
			/* ms間隔でリードする */
			board_status_value[i] = BoardRealRead(i);
			board_chatter_timer[i] = 0;
		}
	}
#endif
}

/**
 *	@brief 制御機のIO初期化処理
 *
 *	@retval 無し
 */
void ContIOInit(void)
{
	int i,j,k;

	RegWrite(SDISP_FPGA_EXT_CFG, 0x405a);	/* extend bus mode select & reset asert  */
	RegWrite(SDISP_FPGA_EXT_CFG, 0x005a);	/*                          reset negate */
	
	//printf("waitkey end \n");
	k=0;
	for (i = 0; i<10000; i++){
	//これはだめfor (i = 0; i<1000; i++){
		for (j = 0; j<100; j++){
			k++;
		}
	}
	//printf("waitkey end \n");
	InitRTC();

	cont_output_data = 0;
	bef_cont_output_data = 0;
	RegWrite(PANEL_OUT_REG, cont_output_data);

	Mdmcs_reg_buf = 0;
	RegWrite(MODEM_REG, 0);

	hakkobyou_out_reg_buf = 0;
	bef_hakkobyou_out_reg_buf = 0;
	RegWrite(HAKKOUBYOU_OUT_REG, hakkobyou_out_reg_buf);

	touka_out_reg_buf = 0;	
	bef_touka_out_reg_buf = 0;	
	RegWrite(TOUKA_BOARD_REG, touka_out_reg_buf);

	cds_status_val = 0;
	/* 初期状態があるのでちょい検討 */
	for ( i = 0; i < 8; i++) {
		board_reg_buf[i] = 0;
		BoardWrite( i, 0);
		bef_board_status_value[i] = BoardRealRead(i);/* 前回の標識版のステータス値 */
		board_status_value[i] = bef_board_status_value[i];
		board_chatter_timer[i] = 0;/* チャタ防止標識版ＩＯリードタイマー */
	}
	
	RegWrite(SHADANMAKU_KYOKA_REG, 1);/* 遮断幕許可への出力処理 */

}

/**
 *	@brief SW読み込み処理
 *
 *	@retval SW状態
 */
int SWRead(void)
{
	int reg;
#ifdef windows
	reg = swtest;
#else
	reg = RegRead(PANEL_IN_REG);
#endif
	return reg;
}


/**
 *	@brief CDS読み込み処理
 *
 *	@retval SW状態
 */
int CDSRead(void)
{
	int reg = 0;
	reg = RegRead(PANEL_IN_REG);
	reg &= SW_CDS_BIT;
	if (reg != 0) {
		reg = 1;
	}
	return reg;
}

/**
 *	@brief 調光強制出力処理
 *
 *	@retval なし
 */
void ChoukouWrite(int data)
{
	char str[100];
	int i;
	int d;
	int mj;
	
	if (now_status.power_outage_flag == 1) {
		/* 停電発生時は強制的に蛍光灯を消す */
		data = 0;
		cds_status_val = data;
	} else {
		if (now_status.keikoutou_status == 1) {
			/* 1:蛍光灯入/0:自動 */
			/* 蛍光灯入モードの場合は強制的に蛍光灯入にする */
			data = 1;
		}
		cds_status_val = data;
		if (now_status.status == STATUS_P3) {/* kaji20170308 変移中は蛍光灯入にする */
			/* kaji20170308 	※※親との通信が無い状態の子単独で手動SWで切り替えた場合は、
									この関数が呼ばれる場所はEventRequestを突っ込んだ直後なので
									IOsrvでrequestが処理されるまではstatusは変わっておらず
									よって手動SWが変移であっても蛍光灯が点灯しない
									…ただし、その後何かeventがあると、点灯状態が切り替わる※※ */
			data = 1;/* 蛍光灯OFF時にSTATUS_P3の場合、発光鋲が暗くなるのを防ぐため */
		}
	}
	for ( i = 0; i < my_io_info.display_board_count; i++) {
		d = board_reg_buf[i];
		d &= ~CHOUKOU_BIT;
		//if (data != 0) {
		mj = CheckMuji(i);/* 標識板が無地かどうかの判定処理 */
		//20170216 if ((data != 0) && (board_status[i] == 0)){/* 異常表示板は消灯 */
		if ((data != 0) && (board_status[i] == 0) && (mj == 0)){/* 異常表示板は消灯 無地の場合は点灯しない */
			d |= CHOUKOU_BIT;
		}
		if (board_reg_buf[i] != d) {
			board_reg_buf[i] = d;
			RegWrite(KAHEN_BOARD_REG + 2 * i, d);
			sprintf(str, "KAHEN_BOARD_REG(%02XH) = %04X", KAHEN_BOARD_REG + 2*i, d);
			DebugPrint("ChoukouWrite", str, 0x10);
		}
	}
}

/**
 *	@brief 可変Boardステータス読み込み処理
 *
 *	@retval 可変Boardステータス
 */
int BoardRead(int no)
{
#ifdef BORD_CHATT
	return board_status_value[no];
#else
	/* こちらは直接のデータを返す */
	return BoardRealRead(no);
#endif
}

/**
 *	@brief チャタ処理無しの可変Boardステータス読み込み処理
 *
 *	@retval 可変Boardステータス
 */
static int BoardRealRead(int no)
{
#ifdef windows
	return input_data[no];
#else
	int reg;
	reg = RegRead(KAHEN_BOARD_REG + 2 * no);
	reg >>= 8;
	reg &= 0xf;
	return reg;
#endif
}

/**
 *	@brief 発光鋲への出力処理
 *
 *	@retval なし
 */
void ByouWrite( int d)
{
	extern int keep_byou_status_flg;/* 発光鋲異常を保持するフラグ */

	hakkobyou_out_reg_buf &= ~0xfff0;
	if (keep_byou_status_flg == 1){
	//こっちは戻るからダメ if (now_status.byou_status == 1){
	//有効にする if (0){//ここは無効にしておく
		/* 発光鋲異常時は消灯？ */
	} else {
		switch (d) {
		case STATUS_P1:
			//hakkobyou_out_reg_buf |= 0x410;
			hakkobyou_out_reg_buf |= 0x010;
			break;
		case STATUS_P2:
			hakkobyou_out_reg_buf |= 0x220;
			break;
		case STATUS_P3:
			//hakkobyou_out_reg_buf |= 0x140;
			hakkobyou_out_reg_buf |= 0x100;
			break;
		case STATUS_FAIL:
			hakkobyou_out_reg_buf |= 0x220;
			break;
		}
		if (cds_status_val != 0) {
			hakkobyou_out_reg_buf |= 0x880;
		}
	}
	//ここでは書かない RegWrite(HAKKOUBYOU_OUT_REG, hakkobyou_out_reg_buf);
	//printf("******hakkobyou_out_reg_buf = %X\n",hakkobyou_out_reg_buf);
}

/**
 *	@brief 内照板への出力処理
 *         制御機４のみの特別な制御
 *
 *	@retval なし
 */
void NaishouWrite( int data)
{
	char str[100];
	int i;
	int d;
	
	i = 1;
	d = board_reg_buf[i];
	board_reg_buf[i] &= ~5;

	switch (data) {
	case STATUS_P1:
		board_reg_buf[i] |= 1;
		
		break;
	case STATUS_P2:
	case STATUS_P3:
	case STATUS_FAIL:
		board_reg_buf[i] |= 4;/* 20170206 */
		break;
	}
	if (board_reg_buf[i] != d) {
		RegWrite(KAHEN_BOARD_REG + 2 * i, board_reg_buf[i]);
		sprintf(str, "KAHEN_BOARD_REG(%02XH) = %04X", KAHEN_BOARD_REG + 2*i, board_reg_buf[i]);
		DebugPrint("NaishouWrite", str, 0x10);
	}
}

/**
 *	@brief 灯火板への出力処理
 *         起動時に一度だけ呼ばれる
 *
 *	@retval なし
 */
void ToukaWrite( int tanmatsu_type)
{
	if ((tanmatsu_type == 1)|| (tanmatsu_type == 4)){
		if (tanmatsu_type == 1){
			touka_out_reg_buf &= ~0xf000;;
			if (cds_status_val != 0) {
				touka_out_reg_buf |= 0x7000;
			}
		} else {
			touka_out_reg_buf &= ~0xf;;
			if (cds_status_val != 0) {
				touka_out_reg_buf |= 7;
			}
		}
		//RegWrite(TOUKA_BOARD_REG, touka_out_reg_buf);
		//printf("ToukaWrite touka_out_reg_buf = %X\n",touka_out_reg_buf);
	}
}

/**
 *	@brief PCPOWERの出力処理
 *
 *	@retval なし
 */
void PcPower( int d)
{
	hakkobyou_out_reg_buf &= 0xfffe;
	if ( d == 1) {
		hakkobyou_out_reg_buf |= 1;
	}
	//ここでは書かない RegWrite(HAKKOUBYOU_OUT_REG, hakkobyou_out_reg_buf);
	//printf("PcPower hakkobyou_out_reg_buf=%X\n",hakkobyou_out_reg_buf);
}

/**
 *	@brief 停電表示灯への出力処理
 *
 *	@retval なし
 */
//void TeidenDispWrite( int d)
//{
//	//int reg;
//	if ( d != 0) {
//		cont_output_data |= (1 << LED_TEIDEN);/* 該当ビットセット */
//	} else {
//		cont_output_data &= (~(1 << LED_TEIDEN));/* 該当ビットクリア */
//	}
//}


/**
 *	@brief 停電ステータス読み込み処理
 *
 *	@retval 停電スステータス
 */
int PowerStatusRead(void)
{
#ifdef windows
	return io_power_outage_flag;
#else
	int reg = 0;
	
	reg = RegRead(PANEL_IN_REG);
	reg &= SW_TEIDEN_BIT;
	if (reg != 0) {
		reg = 1;
	}
	return reg;
#endif
}

/**
 *	@brief Boardへの出力処理
 *
 *	@retval なし
 */
void BoardWrite(int no, int data)
{
	char str[100];
	int d = board_reg_buf[no];
	d &= ~3;
	d |= data;
	if (board_reg_buf[no] != d) {
		board_reg_buf[no] = d;
		RegWrite(KAHEN_BOARD_REG + 2 * no, d);
		sprintf(str, "KAHEN_BOARD_REG(%02XH) = %04X", KAHEN_BOARD_REG + 2*no, d);
		DebugPrint("BoardWrite", str, 0x10);
	}
}

/**
 *	@brief 制御機のLED,IO出力処理
 *
 *	@retval なし
 */
void ContLedWrite(void)
{
	char str[100];
	
	if(bef_cont_output_data != cont_output_data) {
		RegWrite(PANEL_OUT_REG, cont_output_data);
		sprintf(str, "PANEL_OUT_REG(%02XH) = %04X", PANEL_OUT_REG, cont_output_data);
		DebugPrint("", str, 0x10);
	}
	bef_cont_output_data = cont_output_data;

	if(bef_hakkobyou_out_reg_buf != hakkobyou_out_reg_buf) {
		RegWrite(HAKKOUBYOU_OUT_REG, hakkobyou_out_reg_buf);
		sprintf(str, "HAKKOUBYOU_OUT_REG(%02X) = %04X", HAKKOUBYOU_OUT_REG, hakkobyou_out_reg_buf);
		DebugPrint("", str, 0x10);
	}
	bef_hakkobyou_out_reg_buf = hakkobyou_out_reg_buf;
	
	if(bef_touka_out_reg_buf != touka_out_reg_buf) {
		RegWrite(TOUKA_BOARD_REG, touka_out_reg_buf);
		sprintf(str, "TOUKA_BOARD_REG(%02X) = %04X", TOUKA_BOARD_REG, touka_out_reg_buf);
		DebugPrint("", str, 0x10);
	}
	bef_touka_out_reg_buf = touka_out_reg_buf;

}

/**
 *	@brief 制御機のLED表示処理
 *
 *	@retval なし
 */
/*

制御機LEDの配列番号に応じたLEDの制御を行う
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

*/
void ContLedOut(int i,int type )
{
	char str[100];
	switch (type) {
	case LED_STATUS_OFF:
		cont_output_data &= (~(1 << i));/* 該当ビットクリア */
		sprintf(str, "%d 消灯を出力 %.4X", i, cont_output_data);
		break;
	case LED_STATUS_ON:
		cont_output_data |= (1 << i);/* 該当ビットセット */
		sprintf(str, "%d 点灯を出力 %.4X", i, cont_output_data);
		break;
	default:
		sprintf(str, "LED出力が変 ここにきてはいけない");
		break;
	}
	//DebugPrint("", str, 4);
}


/**
 *	@brief 監視盤のIO初期化処理
 *
 *	@retval 無し
 */
void MonitorIOInit(void)
{
	int i,j,k;
	
	RegWrite(SDISP_FPGA_EXT_CFG, 0x405a);	/* extend bus mode select & reset asert  */
	RegWrite(SDISP_FPGA_EXT_CFG, 0x005a);	/*                          reset negate */

	//printf("waitkey end \n");
	k=0;
	for (i = 0; i<10000; i++){
	//これはだめfor (i = 0; i<1000; i++){
		for (j = 0; j<100; j++){
			k++;
		}
	}
	//printf("waitkey end \n");
	InitRTC();

	monitor_output_data = 0;
	bef_monitor_output_data = 0;
	RegWrite(MAP_LED_REG1, monitor_output_data & 0xffff);
	RegWrite(MAP_LED_REG2, (monitor_output_data >> 16) & 0xffff);
	
	panel_out_reg_buf = 0;
	bef_panel_out_reg_buf = 0;
	RegWrite(MPANEL_OUT_REG, panel_out_reg_buf);

}

/*
 *	@brief 監視盤ボタン読み込み処理
 *
 *	@retval 監視盤ボタンステータス
 */
int MonitorBtnRead(void)
{
#ifdef windows
	return btn_status;
#else
	int reg = 0;
	
	reg = RegRead(MPANEL_IN_REG);
	return reg;
#endif
}

/**
 *	@brief 監視盤ブザーへの出力処理
 *
 *	@retval なし
 */
void MonitorBuzzerWrite( int d)
{
	if (d == 0) {
		panel_out_reg_buf &= (~ALARM_BUZZER_BIT);
	} else {
		panel_out_reg_buf |= ALARM_BUZZER_BIT;
	}
}

/**
 *	@brief 監視盤のLED,IO出力処理
 *
 *	@retval なし
 */
void MonitorLedWrite(void)
{
	char str[100];
	
	if(bef_monitor_output_data != monitor_output_data) {
		RegWrite(MAP_LED_REG1, monitor_output_data & 0xffff);
		RegWrite(MAP_LED_REG2, (monitor_output_data >> 16) & 0xffff);
		sprintf(str, "MAP_LED_REG1,2 = %04X,%04X", monitor_output_data & 0xffff, (monitor_output_data >> 16) & 0xffff);
		DebugPrint("", str, 0x10);
	}
	bef_monitor_output_data = monitor_output_data;
	
	if(bef_panel_out_reg_buf != panel_out_reg_buf) {
		RegWrite(MPANEL_OUT_REG, panel_out_reg_buf);
		sprintf(str, "MPANEL_OUT_REG(%02X) = %04X", MPANEL_OUT_REG, panel_out_reg_buf);
		DebugPrint("", str, 0x10);
	}
	bef_panel_out_reg_buf = panel_out_reg_buf;
	
}

/**
 *	@brief 監視盤のLED表示処理
 *
 *	@retval なし
 */

/*
全部で18個
0,2,4,6,8,10,12,14:運用モードLED
1,3,5,7,9,11,13,15:端末状態LED
最後の2個は運用停止,ブザー停止LED用なので制御方法が異なる
運用停止LEDの配列ポインタ
#define UNYOU_TEISHI_LED (2 * CONTROLER_MAX)
ブザー停止LEDの配列ポインタ
#define BUZZER_TEISHI_LED (2 * CONTROLER_MAX + 1)

*/
void MonitorLedOut(int i,int type )
{
	int bit1;
	int bit2;
	char str[100];
//	printf("%d,%d\n",UNYOU_TEISHI_LED,
	bit1 = (1 << ((2*i)+0));
	bit2 = (1 << ((2*i)+1));
	if (i == UNYOU_TEISHI_LED) {
		/* 運用停止LED */
		if (type == LED_STATUS_OFF) {
			panel_out_reg_buf &= (~UNYO_TEISHI_LED_BIT);
		} else {
			panel_out_reg_buf |= UNYO_TEISHI_LED_BIT;
		}
		
	} else if (i == BUZZER_TEISHI_LED) {
		/* ブザー停止LED */
		if (type == LED_STATUS_OFF) {
			panel_out_reg_buf &= (~BUZZER_TEISHI_LED_BIT);
		} else {
			panel_out_reg_buf |= BUZZER_TEISHI_LED_BIT;
		}
	} else if (i == ALARM_LAMP_LED) {
		/* ブザー停止LED */
		if (type == LED_STATUS_OFF) {
			panel_out_reg_buf &= (~ALARM_LAMP_LED_BIT);
		} else {
			panel_out_reg_buf |= ALARM_LAMP_LED_BIT;
		}
	} else {
		switch (type) {
		case LED_STATUS_OFF:
			monitor_output_data &= ~bit1;/* 該当ビットクリア */
			monitor_output_data &= ~bit2;/* 該当ビットクリア */
			sprintf(str, "%d 消灯を出力 %.8X", i + 1, monitor_output_data);
			break;
		case LED_STATUS_GREEN:
			monitor_output_data &= ~bit1;/* 該当ビットセット */
			monitor_output_data |= bit2;/* 該当ビットクリア */
			sprintf(str, "%d 緑点灯を出力 %.8X", i + 1, monitor_output_data);
			break;
		case LED_STATUS_ORANGE:
			monitor_output_data |= bit1;/* 該当ビットセット */
			monitor_output_data |= bit2;/* 該当ビットセット */
			sprintf(str, "%d 橙点灯を出力 %.8X", i + 1, monitor_output_data);
			break;
		case LED_STATUS_RED:
			monitor_output_data |= bit1;/* 該当ビットセット */
			monitor_output_data &= ~bit2;/* 該当ビットクリア */
			sprintf(str, "%d 赤点灯を出力 %.8X", i + 1, monitor_output_data);
			break;
		default:
			sprintf(str, "LED出力が変 ここにきてはいけない");
			break;
		}
	}
	//DebugPrint("", str, 4);
}

/**
 *	@brief FPGAバージョン読み込み処理
 *
 *	@retval FPGAバージョン
 */
int FPGAVersionRead(void)
{
	int reg;
	reg = RegRead(FPGA_VER_REG);
	return reg;
}

/**
 *	@brief CPUFPGAバージョン読み込み処理
 *
 *	@retval CPUFPGAバージョン
 */
int CPUFPGAVersionRead(void)
{
	int reg;
	reg = RegRead(SDISP_FPGA_VER_REG);
	return reg;
}

/**
 *	@brief MDM_CSの出力処理
 *
 *	@retval なし
 */
void MdmcsWrite( int data)
{
	int d = Mdmcs_reg_buf;
	d &= ~1;
	d |= (data&1);
	Mdmcs_reg_buf = d;
	RegWrite(MODEM_REG, data);
}
	
/**
 *	@brief IOデータの入力処理
 *
 *	@param [int address]  アドレス
 *
 *	@retval なし
 */
unsigned int RegRead( unsigned int address){
#ifdef windows
	return 0;
#else
	char str[100];
	if (address > REG_MAX) {
		sprintf("RegRead","アドレスエラー %X",address);
		DebugPrint("", str, 0);
		return;
	}
	int d = 0;
	d = *(uint16_t *)(BASE_REG + address);
volatile	int	i;
	for(i=0; i<4; i++);
///    if(( address < 0 )||( address > 0xfff )||( address & 1 )){
//    	char str[32];
//        sprintf(str,"rd %04x", address );
//        DebugPrint("", str, 1);
//    }
	return d;
#endif
}

/**
 *	@brief IOデータの出力処理
 *
 *	@param [int address]  アドレス
 *	@param [int data]     データ
 *
 *	@retval なし
 */
void RegWrite( unsigned int address, int data){
#ifndef windows
	char str[100];
	if (address > REG_MAX) {
		sprintf("RegWrite","アドレスエラー %X",address);
		DebugPrint("", str, 0);
		return;
	}
		
	*(uint16_t *)(BASE_REG + address) = data;
volatile	int	i;
	for(i=0; i<4; i++);
//	char str[32];
//  sprintf(str, "wr %04x %04x", address, data );
//  DebugPrint("", str, 1);
#endif
}

/**
 *	@brief 不揮発用RTCのパワーオンリセット処理（IOEXT電源起動後40ms以上経過後に呼ぶこと）
 *
 *	@param なし
 *
 *	@retval なし
 */
void InitRTC(void)
{
#ifndef windows
    int i;
/*
	char	str[16];
	printf("\n10: ");
	for(i=0; i<16; i++){
		sprintf(str,"%02X ", SPI_RTC_Read(i+0x10));
		printf("%s", str);
	}
	printf("\n30: ");
	for(i=0; i<3; i++){
		sprintf(str,"%02X ", SPI_RTC_Read(i+0x30));
		printf("%s", str);
	}
	printf("\n");
*/
	if( SPI_RTC_Read( RTC_REG_FLAG ) & 0x02 ){	/* VLF == 1 ?	*/
		/* yes... */
		SPI_RTC_Write( RTC_REG_RSV, 0x00);		/* 31h <- 00h	*/
		SPI_RTC_Write( RTC_REG_CNTL, 0x00);		/* 1Fh <- 00h	*/
		SPI_RTC_Write( RTC_REG_CNTL, 0x80);		/* 1Fh <- 80h	*/
		SPI_RTC_Write( RTC_REG_POR6_1, 0xD3);	/* 60h <- D3h	*/
		SPI_RTC_Write( RTC_REG_POR6_2, 0x03);	/* 66h <- 03h	*/
		SPI_RTC_Write( RTC_REG_POR6_3, 0x02);	/* 6Bh <- 02h	*/
		SPI_RTC_Write( RTC_REG_POR6_4, 0x01);	/* 6Bh <- 01h	*/

		/* ここで2ms以上待たせる */
		for(i=0; i<0xffff; i++)	/* PLIBわかんないので仮 */
			;
	}
	
	TIME_INFO t;
	LoadRTC(&t);/* 不揮発用RTCから読み出す */
	SetTime(&t);/* PIC内蔵RTCに時刻設定処理 */

#endif
}

	
/**
 *	@brief 不揮発用RTCから読み込む処理
 *
 *	@param [TIME_INFO *t] 時刻情報構造体のポインタ
 *
 *	@retval なし
 */
void LoadRTC(TIME_INFO *t)
{
#ifndef windows
	unsigned char rxdata[10];

	SPI_RTC_ReadSTR( RTC_REG_SEC, rxdata, 7);
	t->sec				= rxdata[0];
	t->min				= rxdata[1];
	t->hour				= rxdata[2];
	t->holiday_week.week= rxdata[3];
	t->day				= rxdata[4];
	t->month			= rxdata[5];
	t->year[1]			= rxdata[6];
	t->year[0]			= 0x20;
#endif
	printf("LoadRTC　%02X%02X/%02X/%02X(%d) %02X:%02X:%02X\n"
		, t->year[0], t->year[1]
		, t->month, t->day, t->holiday_week.week
		, t->hour, t->min	, t->sec);
}

/**
 *	@brief 不揮発用RTCに書き込む処理
 *
 *	@param [TIME_INFO *t] 時刻情報構造体のポインタ
 *
 *	@retval なし
 */
void SaveRTC(TIME_INFO *t)
{
#ifndef windows
	SPI_RTC_Write( RTC_REG_CALIB, 0xA8);
	SPI_RTC_Write( RTC_REG_TUNE,  0x00);
	SPI_RTC_Write( RTC_REG_RSV,   0x00);
	SPI_RTC_Write( RTC_REG_IRQ,   0x00);

	SPI_RTC_Write( RTC_REG_EXT,   0x04);
	SPI_RTC_Write( RTC_REG_FLAG,  0x00);
	SPI_RTC_Write( RTC_REG_CNTL,  0x40);

	SPI_RTC_Write( RTC_REG_SEC,   t->sec);
	SPI_RTC_Write( RTC_REG_MIN,   t->min);
	SPI_RTC_Write( RTC_REG_HOUR,  t->hour);
	SPI_RTC_Write( RTC_REG_WEEK,  t->holiday_week.week);
	SPI_RTC_Write( RTC_REG_DAY,   t->day);
	SPI_RTC_Write( RTC_REG_MONTH, t->month);
	SPI_RTC_Write( RTC_REG_YEAR,  t->year[1]);
	
	SPI_RTC_Write( RTC_REG_CNTL,  0x00);
#endif
	printf("SaveRTC　%02X%02X/%02X/%02X(%d) %02X:%02X:%02X\n"
		, t->year[0], t->year[1]
		, t->month, t->day, t->holiday_week.week
		, t->hour, t->min	, t->sec);
}

#ifndef windows //yamazaki

/**
 *	@brief SPI - EEPROMへの書き込み処理
 *
 *	@param [unsigned int address] 対象番地
 *	@param [unsigned char data]   書き込みデータ(8bit幅)
 *
 *	@retval なし
 */
void				SPI_E2ROM_Write( unsigned int address, unsigned char data)
{
#ifndef windows
/*
RDSR	1+1byte	05h, (RX)status
 while(WIP(bit0)==1){}	書き込みインプロセス待ち
WREN	1byte	06h
WRITE	3byte	02h | A8<<3, A7～A0, data
WRDI	1byte	04h
*/
	int rdat;

	do {
		SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_ENABLE );
		SPI_SendData( 0x05 );								/* RDSR */
		rdat = SPI_RcvData();
		SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_DISABLE );
	} while( rdat & 1 );
	
	SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_ENABLE );
	SPI_SendData( 0x06 );									/* WREN */
	SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_DISABLE );

	SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_ENABLE );
	SPI_SendData( 0x02 | ((address & 0x100) >> (8-3)) );	/* WRITE */
	SPI_SendData( address & 0xff );							/* address A7～A0 */
	SPI_SendData( data );									/* data */
	SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_DISABLE );
	
	SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_ENABLE );
	SPI_SendData( 0x04 );									/* WRDI */
	SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_DISABLE );
#endif
}

/**
 *	@brief SPI - EEPROMからの読み出し処理
 *
 *	@param [unsigned int address] 対象番地
 *
 *	@retval 読み出しデータ
 */
unsigned char	SPI_E2ROM_Read( unsigned int address)
{
	int rdat = 0;

#ifndef windows
	SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_ENABLE );
	SPI_SendData( 0x03 | ((address & 0x100) >> (8-3)) );	/* READ */
	SPI_SendData( address & 0xff );							/* address A7～A0 */
	rdat = SPI_RcvData();									/* RX(data) */
	SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_DISABLE );
#endif
	return	rdat;
}


/*
 *===========================================================================================
 *					内部	関数
 *===========================================================================================
 */


/**
 *	@brief SPIバスデバイスのCS選択処理
 *
 *	@param [unsigned char dst]  対象デバイス  1:RTC(SPI_CS_RTC,        2:EEPROM(SPI_CS_EEPROM)
 *	@param [unsigned char mode] CS状態        0:非選択(SPI_CS_DISABLE) 1:選択(SPI_CS_ENABLE)
 *
 *	@retval なし
 */
static void				SPI_CsWrite( unsigned char dst, unsigned char mode )
{
#ifndef windows
	int	dat;
	
	switch(dst){
		case 1:	/* RTC    (SPI_RTC_CS: bit2) */
			dat = (RegRead( SPI_REG ) & 0xfffb) | ((mode & 1) << 2);
			RegWrite( SPI_REG, dat);
			break;
		case 2:	/* EEPROM (SPI_ROM_CS: bit3) */
			dat = (RegRead( SPI_REG ) & 0xfff7) | ((mode & 1) << 3);
			RegWrite( SPI_REG, dat);
			break;
		default:
			break;
	}
#endif
}

/**
 *	@brief SPIバスデバイスへの送信処理
 *
 *	@param [unsigned char data] 送信データ
 *
 *	@retval なし
 */
static void				SPI_SendData( unsigned char data)
{
#ifndef windows
	int		i, inidat, bit;
	
	inidat = RegRead( SPI_REG ) & 0xfffc;
	RegWrite( SPI_REG, inidat );
	for( i=8; i>0; i--){
		bit = (data & 0x80) >> 7;
		data = data << 1;
		RegWrite( SPI_REG, inidat        | bit);	/* SCK 0 */
		RegWrite( SPI_REG, inidat | 0x02 | bit);	/* SCK 1 */
	}
	RegWrite( SPI_REG, inidat );
#endif
}

/**
 *	@brief SPIバスデバイスからの受信処理
 *
 *	@param なし
 *
 *	@retval 受信データ
 */
static unsigned char	SPI_RcvData( void )
{
	unsigned char	rdat = 0;
#ifndef windows
	int		i, inidat, bit;
	
	rdat = 0;
	inidat = RegRead( SPI_REG ) & 0xfffc;
	RegWrite( SPI_REG, inidat );
	for( i=8; i>0; i--){
		rdat = rdat << 1;
		RegWrite( SPI_REG, inidat | 0x02 );		/* SCK 1 */
		bit = (RegRead( SPI_REG ) & 0x100) >> 8;
		RegWrite( SPI_REG, inidat        );		/* SCK 0 */
		rdat = rdat | bit;
	}
#endif
	return	rdat;
}

/**
 *	@brief SPI - RTCへの書き込み処理
 *
 *	@param [unsigned int address] 対象番地
 *	@param [unsigned char data]   書き込みデータ(8bit幅)
 *
 *	@retval なし
 */
static void				SPI_RTC_Write( unsigned int address, unsigned char data)
{
#ifndef windows
	SPI_CsWrite( SPI_CS_RTC, SPI_CS_ENABLE );
	SPI_SendData( address );        /* WRITE */
	SPI_SendData( data );			/* data */
	SPI_CsWrite( SPI_CS_RTC, SPI_CS_DISABLE );
#endif
}

/**
 *	@brief SPI - RTCからの読み出し処理
 *
 *	@param [unsigned int address] 対象番地
 *
 *	@retval 読み出しデータ
 */
static unsigned char	SPI_RTC_Read( unsigned int address)
{
	int rdat = 0;

#ifndef windows
	SPI_CsWrite( SPI_CS_RTC, SPI_CS_ENABLE );
	SPI_SendData( 0x80 | address );     /* READ */
	rdat = SPI_RcvData();				/* RX(data) */
	SPI_CsWrite( SPI_CS_RTC, SPI_CS_DISABLE );
#endif
	return	rdat;
}

/**
 *	@brief SPI - RTCからの連続読み出し処理
 *
 *	@param [unsigned int address] 対象番地
 *	@param [unsigned char *data]  読み出しデータバッファへのポインタ
 *	@param [int length]           読み出し個数
 *
 *	@retval なし
 */
static void		SPI_RTC_ReadSTR( unsigned int address, unsigned char *data, int length)
{
#ifndef windows
	SPI_CsWrite( SPI_CS_RTC, SPI_CS_ENABLE );
	SPI_SendData( 0x80 | address );     /* READ */
	while( length>0 ){
		*data = SPI_RcvData();			/* RX(data) */
		data++;
		length--;
	}
	SPI_CsWrite( SPI_CS_RTC, SPI_CS_DISABLE );
#endif
}
#endif
