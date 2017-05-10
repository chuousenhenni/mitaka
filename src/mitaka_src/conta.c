/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	conta.c
 *	概要
 *  制御機A制御部
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
#include "GPS.h"
#include "queue.h"
#include "monitor.h"
#include "common.h"
#include "contb.h"
#include "cont.h"
#include "event.h"
#include "io.h"

/*
 *===========================================================================================
 *					内部定数定義・型定義・マクロ定義
 *===========================================================================================
 */

/* 制御盤との通信テスト時は以下の有効にする */
//#define TEST_MONITOR

/* 運用管理PCとの通信テスト時は以下の有効にする */
/* この時はPCではCOM2を使用する */
//#define TEST_PC

/* 制御機Bとのテスト用 */
//#define TEST_B

enum {/* 端末制御機送信処理に使用する状態変数 */
	TEST_NONE = 0,	/* デバッグなし、本番ハード */
	TEST_MONITOR,	/* 制御盤との通信テスト時 */
	TEST_PC,		/* 運用管理PCとの通信テスト時 この時はPCではCOM2を使用する */
	TEST_B			/* 制御機Bとのテスト用 */
};

enum {/* 端末制御機送信処理に使用する状態変数 */
	SEND_BROADCAST_STAGE = 0,
	SEND_BROADCAST_REP_STAGE,	/* kaji20170308 */
	WAITING_200MS_TIME_STAGE,
	SEND_NORMAL_STAGE,
	WAITING_1S_TIME_STAGE
};


enum {/* 監視盤送信処理に使用する状態変数 */
	CS_ON_STAGE = 0,
	SEND_WAIT_STAGE1,
	SEND_MONITOR_STAGE,
	WAITING_SEND_END_STAGE,
	SEND_WAIT_STAGE2,
	CS_OFF_STAGE,
	WAITING_6S_TIME_STAGE,
};

enum {/* 運用管理ＰＣの電源制御に使用する状態変数 */
	PCPOWER_IDLE = 0,
	PCPOWER_SHUTDOWN_ON,
	PCPOWER_SHUTDOWN_OFF,
	PCPOWER_WAIT,
};


//#define SCHEDULE_TIMEOUT_VAL (120)	/* 運用管理ＰＣからのスケジュール取得タイムアウト値(秒) */
#define SCHEDULE_TIMEOUT_VAL (5)		/* 運用管理ＰＣからのスケジュール取得タイムアウト値(秒) */

#define SHUTDOWN_WIDTH (6) 			/* シャットダウン信号の幅（秒） */
#define CHOTTO_SHUTDOWN_WIDTH (1) 	/* ちょっと押しシャットダウン信号の幅（秒） */
#define SHUTDOWN_WAIT (2 * 60)		/* シャットダウンからの復帰待ち時間(秒) */

//#define RESPONSE_ERROR_MAX (10)		/* 無線エラーとする連続エラー回数 */

/*
 *===========================================================================================
 *					内部変数定義
 *===========================================================================================
 */

#ifdef windows
//static int debug_type = TEST_MONITOR;	/* 制御盤との通信テスト時 */
/* TEST_PC設定ではapp.cのRcvCom1,3Srvをコメントアウトすること 20170320 */
static int debug_type = TEST_PC;		/* 運用管理PCとの通信テスト時 この時はPCではCOM2を使用する */
//static int debug_type = TEST_B;			/* 制御機Bとのテスト用 */
//static int debug_type = TEST_NONE;	 /* デバッグなし、本番ハード */
#else
static int debug_type = TEST_NONE;	 /* デバッグなし、本番ハード */
#endif


static int controler_send_stage_no = SEND_BROADCAST_STAGE;
static int monitor_send_stage_no = CS_ON_STAGE;
int pcpower_stage_no = PCPOWER_IDLE;
int shutdown_width;/* シャットダウンの押し幅を保持 */
static int tanmatsu_type;/* 端末タイプ（１～CONTROLER_MAX） */

/* 運用管理ＰＣからの同報指令の電文 */
/* 制御機Bへの同報指令の電文 */
static BROADCAST_COMMAND broadcast_command;/* 制御機Bへの同報指令の電文 */
static NORMAL_COMMAND normal_command;/* 制御機Bへの通常指令の電文 */
static RESPONSE_COMMAND my_response_command;/* 自分（制御器A)の監視応答の電文 */
static RESPONSE_COMMAND response_command[CONTROLER_MAX];/* 制御器Bからの監視応答の電文 */

static MONITOR_COMMAND monitor_command;/* 監視盤間への電文 制御指令 */
static MONITOR_OPERATE_COMMAND monitor_operate_command;/* 監視盤間からの運用指令の電文  */

static BROADCAST_COMMAND operation_management_rcv_command;/* 運用管理ＰＣからの同報指令の電文 */
static int operation_management_request;/* 運用管理PCからの同報通知有り */
static int monitor_command_request;/* 監視盤からの要求ありフラグ */
static int monitor_command_type;/* 監視盤から運用要求タイプ(=1:運用停止,=0:運用停止解除) */
int remote_command_request;/* 遠隔時の表示板変更要求ありフラグ */
int remote_command_command;/* 遠隔時の表示板変更コマンド */
static unsigned int broadcast_retry_count;/* kaji20170308 同報指令の複数回送信カウンタ */

//static int ms_timer;/* msタイマ */
static unsigned int check_200ms_timer;/* 200ms待ちタイマ */
static unsigned int check_6s_timer;/* 6秒待ちタイマ */
static unsigned int check_1s_timer;/* 1秒待ちタイマ */
static unsigned int check_delay_timer;/* 待ちタイマ */
static unsigned int pcpower_timer;/* PC電源制御用タイマ */
static unsigned int response_received_timer;/* 制御機Bから受信チェック用タイマー */
static unsigned int response_timer[CONTROLER_MAX];/* 制御器間監視応答受信待ちタイマ */
static int response_received_flag[CONTROLER_MAX];/* 制御器Bから受信時セットされる */
static int response_timeout_flag[CONTROLER_MAX];/* 制御器Bから受信タイムアウト時セットされる */
static unsigned int response_error_count[CONTROLER_MAX];/* 無応答時の連続エラー回数を保持する */
unsigned int response_received_count[CONTROLER_MAX];/* 制御機Ｂからの受信回数を保持する */
unsigned int response_time[CONTROLER_MAX];/* 制御器Aからの要求から制御器Bからの応答受信までの時間(ms) */
unsigned int response_error_time[CONTROLER_MAX];/* 制御器Bからの応答なし継続時間(ms) */
unsigned int response_total_err_count[CONTROLER_MAX];/* kaji20170305 制御機Ｂからの総エラー回数を保持する */

static unsigned int normal_command_timer;/* 通常指令受信待ちタイマ */
static unsigned int moniter_command_timer;/* 監視盤間通信受信待ちタイマ */
static unsigned int watchdog_nofail_timer;/* kaji20170308 なんでもないフェイルのまま固まってないか確認タイマ */

//static int send_stop_req;/* 送信を抑止 0:しない,1:する */


HOLIDAY_DATA holiday_data;/* 変移休止日管理用 */
#if 0
#define HOLIDAY_COUNT (16)
static int holiday_list[HOLIDAY_COUNT][3] =
{
	{1,1,0},/* 元日 */
	{1,2,1},/* 成人の日(1月第2月曜日) */
	{2,11,0},/* 建国記念日 */
	{3,20,2},/* 春分の日 */
	{4,29,0},/* 昭和の日 */
	{5,3,0},/* 憲法記念日 */
	{5,4,0},/* みどりの日 */
	{5,5,0},/* こどもの日 */
	{7,3,1},/* 海の日(7月第3月曜日) */
	{8,11,0},/* 山の日 */
	{9,3,1},/* 敬老の日(9月第3月曜日) */
	{9,22,3},/* 秋分の日 */
	{10,2,1},/* 体育の日(10月第2月曜日 */
	{11,3,0},/* 文化の日 */
	{11,23,0},/* 勤労感謝の日 */
	{12,23,0},/* 天皇誕生日 */
};
#define HAPPY_MONDAY_COUNT (4)
static int happy_monday_list[HAPPY_MONDAY_COUNT][2] =
{
	{1,2},/* 成人の日(1月第2月曜日) */
	{7,3},/* 海の日(7月第3月曜日) */
	{9,3},/* 敬老の日(9月第3月曜日) */
	{10,2},/* 体育の日(10月第2月曜日 */
};
	
#define KYUUSHI_COUNT (2)
static int kyuushi_list[KYUUSHI_COUNT][3] =
{
	{3,3,0},/* 元日 */
	{4,4,},/* 成人の日(1月第2月曜日) */
};
#endif
/*
春分の日(３月XX日)
　= int(20.8431+0.242194*(年-1980)-int((年-1980)/4))
秋分の日(9月XX日)
　= int(23.2488+0.242194*(年-1980)-int((年-1980)/4))
*/


/*
 *===========================================================================================
 *					外部変数定義
 *===========================================================================================
 */

extern int my_tanmatsu_type;/* 端末タイプ 1:A,2:B... */
extern HANDLE hComm1;       /* シリアルポートのハンドル */
extern STATUS now_status;/* 現在の設定値 */
//extern int send_com3_p;
extern int schedule_timer;
extern int err_recover_flg;/* 異常復帰ボタンON状態 */
extern int board_choukou_status;	/* 一斉調光指令の値を保持する　1:点灯 0:滅灯 kaji20170330 */


/*
 *===========================================================================================
 *					内部	関数定義
 *===========================================================================================
 */

static void SendControlerSrv(void);/* 制御機Aの制御機Bへのデータの送信処理 */
static void SetResponse(void);/* 自分の監視応答をバッファにセットする処理 */
static void SetNextTanmatu(void);/* 次に送信する端末を設定する処理 */
static void SendBroadcast(void);/* 同報指令の送信処理 */
static void SendNormal(int no);/* 制御指令の送信処理 */
static void RcvControlerASrv(void);/* 制御機Aの制御機Bからのデータの受信処理 */

static void RcvMonitorSrv(void);/* 制御機Aの監視盤からのデータの受信処理 */
static void SendMonitorSrv(void);/* 監視盤間通信処理 */
static void SendMonitor(void);/* 監視盤への送信処理 */

static void RcvOperationManagementSrv(void);/* 運用管理ＰＣ間通信受信処理 */

static void SendResponse(RESPONSE_COMMAND *response_command );/* 監視応答の送信処理 */

static void PCPowerSrv(void);/* 運用管理ＰＣの電源制御処理 */

static void SaveHoliday(HOLIDAY_COMMAND *p);/* 変移休止日のセーブ処理 */
static void LoadHoliday();/* 変移休止日のロード処理 */

static void SetSchedule( void );/* 本日のスケジュールをセットする処理 */
static void ScheduleSendReq ( void );/* スケジュールの制御機Ｂへの送信要求処理 */

//int fail_flag;/* フェイル送信時1となる */
int fail_hnd_state;/* kaji20170225 フェイル管理の状態変数 */
int CheckTanmatuError(void) ;/* すべての制御機に端末エラーが発生しているかどうかを判定する処理 */

/*
 *===========================================================================================
 *					グローバル関数定義
 *===========================================================================================
 */

void ControlerAInit(void );
void ControlerASrv(void);
void TimerIntContA(int count);/* 制御機Aのタイマ割り込み処理 */
int CheckTodaysStartTime(void);/* 本日の開始時刻の判定処理 */
int CheckStartTime(TIME_INFO_BIN *t);/* 開始時刻の判定処理 */
int CheckHoliday(TIME_INFO_BIN *t);/* 休日かどうかの判定処理 */
int CheckKyuushiday(TIME_INFO_BIN *t);/* 変移休止日かどうかの判定処理 */

/*
 *===========================================================================================
 *					外部	関数定義
 *===========================================================================================
 */

extern void SendCom1(HANDLE h, unsigned char *p, int size);
extern void SendCom2(HANDLE h, unsigned char *p, int size);
extern void SendCom3(HANDLE h, unsigned char *p, int size);
extern int ChkSendCom3(void);

/*
 *===========================================================================================
 *					グローバル関数
 *===========================================================================================
 */

/**
 *	@brief 制御機Aのタイマ割り込み処理
 *
 *	@retval なし
 */
void TimerIntContA(int count)
{
	int i;
//	ms_timer              += count;
	check_200ms_timer     += count;
	check_1s_timer        += count;
	check_6s_timer        += count;
	check_delay_timer     += count;/* 待ちタイマ */
	for ( i = 0 ; i < CONTROLER_MAX; i++) {
		response_timer[i] += count;/* 制御器間監視応答受信待ちタイマ */
		response_error_time[i] += count;/* 制御器Bからの応答なし継続時間(ms) */
	}
	normal_command_timer  += count;/* 通常指令受信待ちタイマ */
	moniter_command_timer += count;/* 監視盤間通信受信待ちタイマ */
	pcpower_timer         += count;/* PC電源制御用タイマ */
	watchdog_nofail_timer += count;/* なんでもないフェイルのまま固まってないか確認タイマ */
	//20170207 if (normal_command_timer > (30*1000)) {
	if (normal_command_timer > (120*1000)) {
		now_status.pc_tuushin_status = 1;/* 運用管理PC間通信状態 */
	} else {
		now_status.pc_tuushin_status = 0;/* 運用管理PC間通信状態 */
	}
	if (moniter_command_timer > (30*1000)) {
		now_status.moniter_tuushin_status = 1;/* 監視盤間通信状態 */
		cont_led_status[LED_DENSOU] = LED_STATUS_TOGGLE;/* kaji20170218 */
	} else {
		now_status.moniter_tuushin_status = 0;/* 監視盤間通信状態 */
		cont_led_status[LED_DENSOU] = LED_STATUS_OFF;/* kaji20170218 */
	}
}


/**
 *	@brief 制御機Aの初期化処理
 *
 *	@retval なし
 */
void ControlerAInit(void )
{
	int i;
	/* 制御機Aから制御機Bへの同報指令の電文フォーマット */
	broadcast_command.h.no = 0;/* 規格番号 00H固定 */
	broadcast_command.h.dst = 0;/* 宛先アドレス 00H固定（未使用?いや使用） */
	broadcast_command.h.src = 1;/* 発信元アドレス  01H固定 */
	broadcast_command.h.sub_adr = 0;/* サブアドレス 00H固定 */
	broadcast_command.h.priority = 2;/* 優先レベル 02H固定 */
	broadcast_command.h.s_no = 0;/* 通番 00H固定 */
	broadcast_command.h.contoroler_no = 0x19;/* 端末種別 19H */
	broadcast_command.h.info = 1;/* 情報種別 01H */
	broadcast_command.h.div_no = 0x81;/* 分割番号 81H */
	broadcast_command.h.length = 18;/* データ長 18(12H) */
	broadcast_command.t.holiday_week.holiday = 0;/* 休日 */
	broadcast_command.t.holiday_week.week = 0;/* 、曜日 */
	broadcast_command.command.byte = 0;/* 制御指令 */
	broadcast_command.light_command.byte = 0;/* 調光指令 */
	broadcast_command.status.byte = 0;/* 端末制御機（Ⅰ）状態 */
	broadcast_command.schedule.start_time[0] = BCD(DEFAULT_START_TIME/60);
	broadcast_command.schedule.start_time[1] = BCD(DEFAULT_START_TIME%60);
	broadcast_command.schedule.start_command = 0;
	broadcast_command.schedule.end_time[0] = BCD(DEFAULT_END_TIME/60);
	broadcast_command.schedule.end_time[1] = BCD(DEFAULT_END_TIME%60);
	broadcast_command.schedule.end_command = 0;
	broadcast_command.schedule.offset_timer = BCD(DEFAULT_OFFSET_TIMER/10);
	operation_management_rcv_command = broadcast_command;
	/* 制御機Aから制御機Bへの通常指令の電文フォーマット */
	normal_command.h.no = 0;/* 規格番号 00H固定 */
	normal_command.h.dst = my_tanmatsu_type;/* 宛先アドレス 01H～08H */
	normal_command.h.src = 1;/* 発信元アドレス  01H固定 */
	normal_command.h.sub_adr = 0;/* サブアドレス 00H固定 */
	normal_command.h.priority = 2;/* 優先レベル 02H固定 */
	normal_command.h.s_no = 0;/* 通番 00H固定 */
	normal_command.h.contoroler_no = 0x19;/* 端末種別 19H */
	normal_command.h.info = 1;/* 情報種別 01H */
	normal_command.h.div_no = 0x81;/* 分割番号 81H */
	normal_command.h.length = 11;/* データ長 11(0BH) */
	normal_command.t.holiday_week.holiday = 0;/* 休日 */
	normal_command.t.holiday_week.week = 0;/* 、曜日 */
	normal_command.command.byte = 0;/* 制御指令 */
	normal_command.light_command.byte = 0;/* 調光指令 */
	normal_command.status.byte = 0;/* 端末制御機（Ⅰ）状態 */

	/* 制御機Bから制御機Aへの監視応答の電文フォーマット */
	for ( i = 0; i < CONTROLER_MAX; i++) {
		memset((char *)&response_command[i], 0, sizeof (RESPONSE_COMMAND));
		response_command[i].h.no = 0;/* 規格番号 00H固定 */
		response_command[i].h.dst = 1;/* 宛先アドレス  01H固定 */
		response_command[i].h.src = i + 1;/* 発信元アドレス 01H～08H */
		response_command[i].h.sub_adr = 0;/* サブアドレス 00H固定 */
		response_command[i].h.priority = 2;/* 優先レベル 02H固定 */
		response_command[i].h.s_no = 0;/* 通番 00H固定 */
		response_command[i].h.contoroler_no = 0x19;/* 端末種別 19H */
		response_command[i].h.info = 0x11;/* 情報種別 11H */
		response_command[i].h.div_no = 0x81;/* 分割番号 81H */
		response_command[i].h.length = 15;/* データ長 15(0FH) */
//		response_command[i].response.byte2.tanmatu_error = 1;/* 端末機異常としておく */
	}
	memset((char *)&my_response_command, 0, sizeof (RESPONSE_COMMAND));
	my_response_command.h.no = 0;/* 規格番号 00H固定 */
	my_response_command.h.dst = 1;/* 宛先アドレス  01H固定 */
	my_response_command.h.src = CONTA_ADDRESS;/* 発信元アドレス 01H～08H */
	my_response_command.h.sub_adr = 0;/* サブアドレス 00H固定 */
	my_response_command.h.priority = 2;/* 優先レベル 02H固定 */
	my_response_command.h.s_no = 0;/* 通番 00H固定 */
	my_response_command.h.contoroler_no = 0x19;/* 端末種別 19H */
	my_response_command.h.info = 0x11;/* 情報種別 11H */
	my_response_command.h.div_no = 0x81;/* 分割番号 81H */
	my_response_command.h.length = 15;/* データ長 15(0FH) */
//	my_response_command.response.byte2.tanmatu_error = 1;/* 端末機異常としておく */

	/* 制御機からの監視盤への制御指令の電文フォーマット */
	monitor_command.h.no = 0;/* 規格番号 00H固定 */
	monitor_command.h.dst = 1;/* 宛先アドレス 01H固定（未使用） */
	monitor_command.h.src = 1;/* 発信元アドレス  01H固定 */
	monitor_command.h.sub_adr = 0;/* サブアドレス 00H固定 */
	monitor_command.h.priority = 2;/* 優先レベル 02H固定 */
	monitor_command.h.s_no = 0;/* 通番 00H固定 */
	monitor_command.h.contoroler_no = 0x19;/* 端末種別 19H */
	monitor_command.h.info = 0x11;/* 情報種別 01H */
	monitor_command.h.div_no = 0x81;/* 分割番号 81H */
//	monitor_command.h.length = 162;/* データ長 162(A2H) */
	monitor_command.h.length = 56;/* データ長 56(38H) */
	monitor_command.t.holiday_week.holiday = 0;/* 休日 */
	monitor_command.t.holiday_week.week = 0;/* 、曜日 */
	memset(monitor_command.command, 0, sizeof(monitor_command.command));
	
	/* 監視盤から制御機への電文 運用指令の電文フォーマット */
	monitor_operate_command.h.no = 0;/* 規格番号 00H固定 */
	monitor_operate_command.h.dst = 1;/* 宛先アドレス 01H固定（未使用） */
	monitor_operate_command.h.src = 1;/* 発信元アドレス  01H固定 */
	monitor_operate_command.h.sub_adr = 0;/* サブアドレス 00H固定 */
	monitor_operate_command.h.priority = 2;/* 優先レベル 02H固定 */
	monitor_operate_command.h.s_no = 0;/* 通番 00H固定 */
	monitor_operate_command.h.contoroler_no = 0x19;/* 端末種別 19H */
	monitor_operate_command.h.info = 0x19;/* 情報種別 09H */
	monitor_operate_command.h.div_no = 0x81;/* 分割番号 81H */
//	monitor_operate_command.h.length = 162;/* データ長 162(A2H) */
	monitor_operate_command.h.length = 56;/* データ長 56(38H) */
	monitor_operate_command.t.holiday_week.holiday = 0;/* 休日 */
	monitor_operate_command.t.holiday_week.week = 0;/* 、曜日 */
	monitor_operate_command.command = 0;

	operation_management_request = 0;/* 運用管理PCからの同報通知要求フラグ */
	monitor_command_request = 0;/* 監視盤からの要求フラグ */
	//remote_command_request = 0;/* 遠隔時の表示板変更要求ありフラグ */
	normal_command_timer = 0;/* 通常指令受信待ちタイマ */
	moniter_command_timer = 0;/* 監視盤間通信受信待ちタイマ */
	shutdown_width = SHUTDOWN_WIDTH;/* シャットダウンの押し幅を保持 */
//	fail_flag = 0;/* フェイル送信時1となる */
	fail_hnd_state = FAIL_HND_NONE;/* フェイル状態　無し */
	watchdog_nofail_timer = 0;/* なんでもないフェイルのまま固まってないか確認タイマ クリア */

	for ( i = 0; i < CONTROLER_MAX; i++) {
		response_error_count[i] = 0;/* 無応答時の連続エラー回数をクリア */
		response_received_count[i]= 0;/* 制御機Ｂからの受信回数をクリア */
		response_total_err_count[i] = 0;/* kaji20170305 制御機Ｂからの総エラー回数をクリア */
	}
	//send_stop_req = 0;/* 送信を抑止 0:しない,1:する */

	holiday_data.holiday_count = 0;/* 祝日登録数クリア */
	holiday_data.kyuushi_count = 0;/* 変移休止日登録数クリア */
	LoadHoliday();/* 祝日,変移休止日ロード */
	
#if 0
	holiday_data.holiday_count=HOLIDAY_COUNT;
	holiday_data.kyuushi_count=KYUUSHI_COUNT;
	for( i=0;i<holiday_data.holiday_count;i++){
		holiday_data.holiday[i].month = holiday_list[i][0];
		holiday_data.holiday[i].day = holiday_list[i][1];
		holiday_data.holiday[i].type = holiday_list[i][2];
	}
	for( i=0;i<holiday_data.kyuushi_count;i++){
		holiday_data.kyuushi[i].month = kyuushi_list[i][0];
		holiday_data.kyuushi[i].day = kyuushi_list[i][1];
		holiday_data.kyuushi[i].type = kyuushi_list[i][2];
	}
#endif
	tanmatsu_type = 0;
	SetNextTanmatu();/* 次に送信する端末を設定する処理 */
	if (tanmatsu_type == CONTA_ADDRESS) {
		SetNextTanmatu();/* 次に送信する端末を設定する処理 */
	}
	PcPower(0);
	MdmcsWrite(0);
	//GPSInit();
}

/**
 *	@brief 制御機Aの処理
 *
 *	@retval なし
 */

void ControlerASrv(void)
{
	if (debug_type == TEST_MONITOR) {
		/* 監視盤とのテスト用 */
		SendMonitorSrv();/* 制御機Aの監視盤へのデータの送信処理 */
		RcvMonitorSrv();/* 制御機Aの監視盤からのデータの受信処理 */
		SendControlerSrv();/* 制御機Aの制御機Bへのデータの送信処理 */
	} else if (debug_type == TEST_PC) {
		/* 運用管理PCとのテスト用 */
		//常に不要	SendOperationManagementSrv();/* 運用管理ＰＣ間通信送信処理 */
		SendControlerSrv();/* 制御機Aの制御機Bへのデータの送信処理 */
		RcvOperationManagementSrv();/* 運用管理ＰＣ間通信受信処理 */
	} else if (debug_type == TEST_B) {
		/* 制御機Bとのテスト用 */
		SendControlerSrv();/* 制御機Aの制御機Bへのデータの送信処理 */
		RcvControlerASrv();
	} else {
		/* これが本来の動き */
		SendMonitorSrv();/* 制御機Aの監視盤へのデータの送信処理 */
		RcvMonitorSrv();/* 制御機Aの監視盤からのデータの受信処理 */
		//常に不要　たぶん	SendOperationManagementSrv();/* 運用管理ＰＣ間通信送信処理 */
		RcvOperationManagementSrv();/* 運用管理ＰＣ間通信受信処理 */
		SendControlerSrv();/* 制御機Aの制御機Bへのデータの送信処理 */
		RcvControlerASrv();/* 制御機Aの制御機Bからのデータの受信処理 */
	}
	PCPowerSrv();/* 運用管理ＰＣの電源制御処理 */
	/*
	ここで運用管理PCからのスケジュール登録がされていなかったら
	制御機Aは自分の情報から登録を行い、
	制御機Bに送信する
	*/
	if (schedule_timer>(SCHEDULE_TIMEOUT_VAL * 1000)) {
		if (now_status.schedule == 0) {
			now_status.schedule = 1;/* スケジュール登録済み */
			now_status.time_req = 1;/* 時刻修正も登録済みとする */
			SetSchedule( );/* 本日のスケジュールをセットする処理 */
		}
	}

	ContSrv(&my_response_command);/* 制御機共通の処理 */
}

/*
 *===========================================================================================
 *					内部関数
 *===========================================================================================
 */

/**
 *	@brief 制御機Aの制御機Bへのデータの送信処理
 *
 *	@retval なし
 */
static void SendControlerSrv(void)
{
	int i;
	char str[256];
	char str2[256];
	switch (controler_send_stage_no) {
	case SEND_BROADCAST_STAGE:/* パワーON / 同報指令送信 */
		DebugPrint("SendControlerSrv", "SEND_BROADCAST_STAGE", 1);
		SendBroadcast();/* そのまま制御機Bへ同報指令を送信する */
		check_200ms_timer = 0;/* 200ms待ちタイマ */
		controler_send_stage_no = WAITING_200MS_TIME_STAGE;
//		broadcast_retry_count = BROADCAST_RETRY_VAL;/* kaji20170308 同報指令の複数回送信数 */
		broadcast_retry_count = 1;/* 問題ありのため休止 */
		break;
	case SEND_BROADCAST_REP_STAGE:/* 同報指令送信(繰り返し) */
		DebugPrint("SendControlerSrv", "SEND_BROADCAST_REP_STAGE", 1);
		SendBroadcast();/* そのまま制御機Bへ同報指令を送信する */
		check_200ms_timer = 0;/* 200ms待ちタイマ */
		controler_send_stage_no = WAITING_200MS_TIME_STAGE;
		break;
	case WAITING_200MS_TIME_STAGE://200ms待ち
		if (check_200ms_timer >= 200) {
			broadcast_retry_count--;
			if (broadcast_retry_count == 0) {/* kaji20170308 同報指令を複数回送信させる */
				controler_send_stage_no = SEND_NORMAL_STAGE;
			} else {
				controler_send_stage_no = SEND_BROADCAST_REP_STAGE;
			}
		}
		break;
	case SEND_NORMAL_STAGE:/* 制御指令送信 */
		SendNormal( tanmatsu_type );
		sprintf(str, "SEND_NORMAL_STAGE %d %02X,%02X,%02X"
			, tanmatsu_type
			, (normal_command.command.byte)&0xff
			, (normal_command.light_command.byte)&0xff
			, (normal_command.status.byte)&0xff);
		DebugPrint("SendControlerSrv",str, 8);
		check_200ms_timer = 0;/* 1s待ちタイマ */
		controler_send_stage_no = WAITING_1S_TIME_STAGE;
		break;
	case WAITING_1S_TIME_STAGE://1s待ち
//20170305	if (check_200ms_timer < RESPONSE_INTERVAL_TIME_VAL){/* kaji20170305 */
		if (check_200ms_timer < param.response_interval_time_val){/* kaji20170305 */
			/* インターバルタイム(当初500ms待ち) */
//20170305	} else if ((check_200ms_timer >= RESPONSE_TIMEOUT_VAL) || (response_received_flag[tanmatsu_type -1] == 1)){
		} else if ((check_200ms_timer >= param.response_timeout_val) || (response_received_flag[tanmatsu_type -1] == 1)){
			response_time[tanmatsu_type -1] = check_200ms_timer;/* 制御器Aからの要求から制御器Bからの応答受信までの時間(ms) */
			if(response_received_flag[tanmatsu_type -1] == 1) {
				/* 制御機から受信している */
				response_timeout_flag[tanmatsu_type -1] = 0;
				response_error_count[tanmatsu_type -1] = 0;/* 無応答時の連続エラー回数をクリアする */
				response_received_count[tanmatsu_type -1]++;/* 制御機Ｂからの受信回更新 */
				response_error_time[tanmatsu_type -1] = 0;/* 制御器Bからの応答なし継続時間(ms) */
			} else {
				response_error_count[tanmatsu_type -1]++;/* 無応答時の連続エラー回数更新 */
				response_total_err_count[tanmatsu_type -1]++;/* kaji20170316 制御機Ｂからの総エラー回数更新 */
//20170303		if (response_error_count[tanmatsu_type -1] > RESPONSE_ERROR_MAX) {
//20170305		if (response_error_time[tanmatsu_type -1] > (MUSEN_TIMEOUT_VAL*1000)) {/* 指定時間連続で応答を受信出来ない */
				if (response_error_time[tanmatsu_type -1] > (param.musen_timeout_val*1000)) {/* 指定時間連続で応答を受信出来ない */
					response_timeout_flag[tanmatsu_type -1] = 1;/* 受信タイムアウト発生 */
//				}
//#if 1
// kaji20170227 ifの内側に以下のブロックを移動↓ (LED_MUSENと運用管理PCの無線異常表示が発生する時期を同じにする)
					/* 20170221 無線異常時も運用管理PCに監視応答を送信する */
					response_command[tanmatsu_type - 1].response.byte2.musen_error = 1;/* 無線異常をセット */
					SendResponse(&response_command[tanmatsu_type - 1]);/* そのまま運用管理PCに監視応答を送信する */
					sprintf(str, "SendControlerSrv 監視応答 %d src=%d %02X %02X %02X", 1, tanmatsu_type
						, response_command[tanmatsu_type - 1].response.status[0]
						, response_command[tanmatsu_type - 1].response.status[1]
						, response_command[tanmatsu_type - 1].response.status[2]
					);
					DebugPrint("", str, 1);
//					response_total_err_count[tanmatsu_type -1]++;/* kaji20170305 制御機Ｂからの総エラー回数更新 */
				}
// kaji20170227 ↑
			}
			//sprintf(str, "%d,%d,%d,%d",check_200ms_timer, RESPONSE_TIMEOUT_VAL,response_received_flag[tanmatsu_type -1],response_timeout_flag[tanmatsu_type -1]);
			//DebugPrint("", str, 0x20);
			
			if (param.no_musen_error_flag == 0) {
				/* ここで無線異常をチェックする */
				now_status.musen_status = 0;
				for ( i = 0; i < CONTROLER_MAX; i++) {
					if (response_timeout_flag[i] != 0) {
						/* いずれかの制御機Bが受信タイムアウトの場合 */
						sprintf(str,"いずれかの制御機Bが受信タイムアウトの場合 制御機番号=%d",i+1);
						DebugPrint("", str, 0x20);
						now_status.musen_status = 1;
						break;
					}
				}
				if (now_status.before_musen_status != now_status.musen_status) {
					if (param.no_fail_flag == 0) {/* この場合はデバッグ用にフェイルにはしない */
						if (now_status.before_musen_status == 0) {
							EventRequest(FAIL_REQUEST);/* 異常リクエスト */
							cont_led_status[LED_MUSEN] = LED_STATUS_TOGGLE;
						} else {
#ifdef	kaji20170208
自分ではフェイルから抜ける判断を行わない
							if ((now_status.status == STATUS_FAIL) && (CheckErrorStatus() == 0)) {
								EventRequest(FAIL_RECOVER_REQUEST);/* 異常復帰リクエスト */
							}
#endif
							cont_led_status[LED_MUSEN] = LED_STATUS_OFF;
						}
//						SetResponseStatus(response);/* yamazaki??20170227 IOステータスをセット */
					}
				}
				now_status.before_musen_status = now_status.musen_status;
			}
			/* タイムアウトまたは送信応答が該当制御機から有った */
			if (operation_management_request ==  1) {
				/* 運用管理PCからの同報通知有り */
				operation_management_request = 0;
				memmove(&broadcast_command, &operation_management_rcv_command, sizeof(BROADCAST_COMMAND));
				controler_send_stage_no = SEND_BROADCAST_STAGE;
				strcpy(str,"");
				if ((broadcast_command.status.byte&8) != 0) {
					sprintf(str2, "伝送テスト[制御機%d]の送信要求有り", (broadcast_command.status.byte >> 5) + 1);
					strcat(str, str2);
					strcat(str, " ");
				}
				if ((broadcast_command.status.byte&0x10) != 0) {
					sprintf(str2, "伝送テスト解除[制御機%d]の送信要求有り", (broadcast_command.status.byte >> 5) + 1);
					strcat(str, str2);
					strcat(str, " ");
				}
				if (broadcast_command.command.shudou != 0) {
					/* 手動の送信要求有り */
					sprintf(str2, "手動の送信要求有り");
					strcat(str, str2);
					strcat(str, " ");
				}
				if (broadcast_command.command.teishi != 0) {
					/* 運用停止の送信要求有り */
					sprintf(str2, "運用停止の送信要求有り");
					strcat(str, str2);
					strcat(str, " ");
				}
				if (broadcast_command.command.yobi2 != 0) {
					/* 運用停止の送信要求有り */
					sprintf(str2, "運用停止解除の送信要求有り");
					strcat(str, str2);
					strcat(str, " ");
				}
				if (broadcast_command.light_command.sreq != 0) {
					/* スケジュール登録要求の送信要求有り */
					sprintf(str2, "スケジュール登録要求の送信要求有り");
					strcat(str, str2);
					strcat(str, " ");
				}
				if (broadcast_command.light_command.time_req != 0) {
					/* 時刻修正要求の送信要求有り */
					sprintf(str2, "時刻修正要求の送信要求有り");
					strcat(str, str2);
					strcat(str, " ");
				}
				if (str[0] == 0) {
					/* 送信要求無し */
					sprintf(str, "遠隔の送信要求有り");
				}
				DebugPrint("SendControlerSrv",str, 1);
				
/* kaji20170225↓ */
//			} else if ((fail_flag == 0) && (CheckTanmatuError() != 0) && ((GetNowRemoteStatus() == STATUS_P1P2) || (GetNowRemoteStatus() == STATUS_P3) )){
//				fail_flag = 1;/* フェイルを送信する */
//20170305 			} else if ( (fail_hnd_state == FAIL_HND_NONE) && (CheckTanmatuError() != 0) 
			} else if ( (fail_hnd_state == FAIL_HND_NONE) && (now_status.mode == MODE_REMOTE) && (CheckTanmatuError() != 0) 
					&&   (GetNowRemoteStatus() == STATUS_P1) ){/* kaji20170226 */
//					&& ( (GetNowRemoteStatus() == STATUS_P1)||(GetNowRemoteStatus() == STATUS_P1P2) ) ){
				fail_hnd_state = FAIL_HND_P1_PARK;/* 通常時に発生したのでフェイルにしない状態 */
				strcpy(str, "通常時に発生したフェイル要件なので変化させない");
				DebugPrint("",str, 0);

//20170305			} else if ((fail_hnd_state == FAIL_HND_NONE) && (CheckTanmatuError() != 0) 
			} else if ((fail_hnd_state == FAIL_HND_NONE) && (now_status.mode == MODE_REMOTE) && (CheckTanmatuError() != 0) 
					&& ( (GetNowRemoteStatus() == STATUS_P1P2)||(GetNowRemoteStatus() == STATUS_P2)
					   ||(GetNowRemoteStatus() == STATUS_P3)  ||(GetNowRemoteStatus() == STATUS_P3P2) ) ){/* kaji20170226 */
//					&& ( (GetNowRemoteStatus() == STATUS_P2)||(GetNowRemoteStatus() == STATUS_P3)||(GetNowRemoteStatus() == STATUS_P3P2) ) ){
				fail_hnd_state = FAIL_HND_FAIL;/* フェイル状態 */
/* kaji20170225↑ */
				broadcast_command.command.byte = 0x20;/* フェイル */
				broadcast_command.light_command.byte = 0;/* light_commandをクリア */
				broadcast_command.status.byte = 0;/* statusをクリア */
				sprintf(str, "遠隔時のboard変更送信要求有り %02X", broadcast_command.command.byte);
				DebugPrint("",str, 0);
				controler_send_stage_no = SEND_BROADCAST_STAGE;
				/* 自分をフェイルとする */
				EventRequest(FAIL_REQUEST);/* 異常リクエスト */
			
/* kaji20170225↓ */
//			//20170208 } else if ((fail_flag == 1) && (CheckTanmatuError() == 0) && (err_recover_flg == 1)){
//				fail_flag = 0;/* フェイルからの復帰 */
//20170305			} else if ((fail_hnd_state != FAIL_HND_NONE) && (CheckTanmatuError() == 0)){/* フェイル条件から外れる */
			} else if ((fail_hnd_state != FAIL_HND_NONE) && (now_status.mode == MODE_REMOTE) && (CheckTanmatuError() == 0)){/* フェイル条件から外れる */
				fail_hnd_state = FAIL_HND_NONE;/* フェイル状態　無し */
/* kaji20170225↑ */
				err_recover_flg = 0;/* 異常復帰ボタンON状態クリア */
				if (GetNowRemoteStatus() == STATUS_P3) {
					/* 変移を送信する */
					//broadcast_command.command.byte = 4;
					broadcast_command.command.byte = 0;/* 遠隔 */
				} else {
					/* 通常を送信する */
					//broadcast_command.command.byte = 1;
					broadcast_command.command.byte = 0;/* 遠隔 */
				}
				broadcast_command.light_command.byte = 0;/* light_commandをクリア */
				broadcast_command.status.byte = 0;/* statusをクリア */
				sprintf(str, "遠隔時のboard変更送信要求有り  %02X", broadcast_command.command.byte);
				DebugPrint("",str, 0);
				controler_send_stage_no = SEND_BROADCAST_STAGE;
				EventRequest(REMOTE_REQUEST);/* 遠隔リクエスト */
/* kaji20170308↓ */
//			} else if ((fail_hnd_state != FAIL_HND_NONE) && (now_status.mode == MODE_MONITOR) && (CheckTanmatuError() == 0)){/* フェイル条件から外れる */
//				fail_hnd_state = FAIL_HND_NONE;/* フェイル状態　無し */
//				err_recover_flg = 0;/* 異常復帰ボタンON状態クリア */
//				broadcast_command.command.byte = 0;/* 遠隔 */
//				broadcast_command.light_command.byte = 0;/* light_commandをクリア */
//				broadcast_command.status.byte = 0;/* statusをクリア */
//				broadcast_command.command.teishi = 1;/* commandにteishiをセット */
//				sprintf(str, "運用停止中のフェイル外れによる送信要求有り %02X", broadcast_command.command.byte);
//				DebugPrint("",str, 0);
//				controler_send_stage_no = SEND_BROADCAST_STAGE;
//				EventRequest(MONITOR_REQUEST);/* 遠隔リクエスト */
/* kaji20170308↑ */
			} else if (remote_command_request == 1) {
				/* 遠隔時の表示板変更要求有り */
				remote_command_request = 0;
				broadcast_command.command.byte = remote_command_command;
				broadcast_command.light_command.byte = 0;/* light_commandをクリア */
				broadcast_command.status.byte = 0;/* statusをクリア */
				sprintf(str, "遠隔時のboard変更送信要求有り   %02X", broadcast_command.command.byte);
				DebugPrint("",str, 0);
				controler_send_stage_no = SEND_BROADCAST_STAGE;
			} else if (monitor_command_request == 1) {
				/* 監視盤からの同報通知要求有り */
				monitor_command_request = 0;
				broadcast_command.command.byte = 0;/* commandをクリアしてから */
				broadcast_command.light_command.byte = 0;/* light_commandをクリア */
				broadcast_command.status.byte = 0;/* statusをクリア */
				if (monitor_command_type == 1) {
					/* 監視盤から運用停止要求 */
					sprintf(str, "監視盤からの運用停止要求の送信要求有り");
					broadcast_command.command.teishi = 1;/* commandにteishiをセット */
				} else {
					/* 監視盤から運用停止解除要求 */
					sprintf(str, "監視盤からの運用停止解除要求の送信要求有り");
					broadcast_command.command.yobi2 = 1;/* commandにyobi2をセット */
				}
//				DebugPrint("SendControlerSrv",str, 1);
				DebugPrint("SendControlerSrv",str, 0);
				controler_send_stage_no = SEND_BROADCAST_STAGE;
			} else {
// kaji20170308	fail_hnd_state = FAIL_HND_NONE;/* フェイル状態　無し */
/* kaji20170308↓*/
				if ((now_status.mode == MODE_REMOTE) && (now_status.status == STATUS_FAIL) && (CheckTanmatuError() == 0)){
					if (watchdog_nofail_timer > (5*1000)) {/* 5秒待ち */
						DebugPrint("SendControlerSrv", " *** nofail TRAP 強制離脱", 0);
						EventRequest(REMOTE_REQUEST);/* 遠隔リクエスト */
						watchdog_nofail_timer = 0;/* なんでもないフェイルのまま固まってないか確認タイマ クリア */
					}
				} else {
					watchdog_nofail_timer = 0;/* なんでもないフェイルのまま固まってないか確認タイマ クリア */
				}
/* kaji20170308↑*/
				SetNextTanmatu();/* 次に送信する端末を設定する処理 */
				if (tanmatsu_type == CONTA_ADDRESS) {
					/* 自分(制御機Ａ)のアドレスの場合はスキップしてバッファにセットする */
					//action 直前にセットする SetStaus(&response_command[0]);/* IOステータスをセット */
//					response_timer[tanmatsu_type - 1] = 0;/* 制御器間監視応答受信待ちタイマクリア */
//					response_received_flag[tanmatsu_type - 1] = 0;
					SetResponse();/* 自分の監視応答をバッファにセットする */
					sprintf(str, "運用管理PCに監視応答送信 %02X,%02X,%02X,%02X,%02X,%02X,%02X"
						,response_command[tanmatsu_type - 1].response.status[0]
						,response_command[tanmatsu_type - 1].response.status[1]
						,response_command[tanmatsu_type - 1].response.status[2]
						,response_command[tanmatsu_type - 1].response.status[3]
						,response_command[tanmatsu_type - 1].response.status[4]
						,response_command[tanmatsu_type - 1].response.status[5]
						,response_command[tanmatsu_type - 1].response.status[6]
					);
					DebugPrint("SendControlerSrv", str, 8);
					SendResponse(&response_command[tanmatsu_type - 1]);/* そのまま運用管理PCに監視応答を送信する */
					
					SetNextTanmatu();/* 次に送信する端末を設定する処理 */
				}
				response_timer[tanmatsu_type - 1] = 0;/* 制御器間監視応答受信待ちタイマクリア */
				response_received_flag[tanmatsu_type - 1] = 0;
				controler_send_stage_no = SEND_NORMAL_STAGE;
			}
		}
		break;
	default:
		printf("SendControlerSrv default error\n");
		break;
	}

}

/**
 *	@brief 自分の監視応答をバッファにセットする処理
 *
 *	@retval なし
 */
static void SetResponse(void)
{
	SetMode(&now_status, &my_response_command);/* 現在のモードをセット */
	if (now_status.schedule == 0) {
		/* スケジュール登録依頼送信 */
		DebugPrint("SendControlerSrv","スケジュール登録依頼送信", 1);
		my_response_command.response.byte7.schedule_req = 1;
	} else {
		my_response_command.response.byte7.schedule_req = 0;
	}
	if (now_status.time_req == 0) {
		/* 時刻修正依頼送信 */
		DebugPrint("SendControlerSrv","時刻修正依頼送信", 1);
		my_response_command.response.byte7.time_req = 1;
	} else {
		my_response_command.response.byte7.time_req = 0;
	}
	
	int cds = CDSRead();
	/* kaji20170330 ↓ */
	if (board_choukou_status == 1) {
		cds = 1;/* 点灯させる */
	}
	/* kaji20170330 ↑ */
	now_status.cds_status = cds; /* CDS状態 */
	if ( now_status.cds_status == 1) {
		/* 明るいので調光強制切 */
		//printf("ChoukouWriteA 0\n");
		ChoukouWrite(1);/* 調光強制出力処理 */
		my_response_command.response.byte7.choukou_iri =0;/* 調光入（昼） */
		my_response_command.response.byte7.choukou_kiri=1;/* 調光切（夜） */
	} else {
		//printf("ChoukouWriteB 0\n");
		/* 暗いので調光強制入 */
		ChoukouWrite(0);/* 調光強制出力処理 */
		my_response_command.response.byte7.choukou_iri =1;/* 調光入（昼） */
		my_response_command.response.byte7.choukou_kiri=0;/* 調光切（夜） */
	}
	ByouWrite(now_status.status);//20170128 yamazaki
	ToukaWrite(my_tanmatsu_type);//20170129 yamazaki
	
	memmove(&response_command[CONTA_ADDRESS - 1], &my_response_command, sizeof(RESPONSE_COMMAND));
	//if (now_status.gps_status != 0) {
	//	/* 時計異常 */
	//	response_command[CONTA_ADDRESS - 1].response.byte2.tanmatu_error = 1;/* 端末制御機異常 */
	//}
}

/**
 *	@brief 次に送信する端末を設定する処理
 *
 *	@retval なし
 */
static void SetNextTanmatu(void) {
	int i;
	
	tanmatsu_type++;
	if (tanmatsu_type > CONTROLER_MAX) {
		tanmatsu_type = 1;
	}
	i = tanmatsu_type -1;
	int d = (param.linkage_status >> (4*(CONTROLER_MAX - i - 1)))&0xf;
	if (d == 0) {
		/* 連動していないので次を選択 */
		SetNextTanmatu();
	}
}

/**
 *	@brief すべての制御機に端末エラーが発生しているかどうかを判定する処理
 *
 *	@retval 0:エラー無し,1:エラー有り
 */
int CheckTanmatuError(void) {
	int i;
	int ret = 0;
	int err_count = 0;
	for ( i = 0; i < CONTROLER_MAX; i++) {
		int d = (param.linkage_status >> (4*(CONTROLER_MAX - i - 1)))&0xf;
		if (d == 0) {
			/* 連動していないので次を選択 */
			continue;
		}
		if (response_command[i].response.byte2.tanmatu_error != 0) {
			err_count++;
		}
	}
	if (err_count != 0) {
		ret = 1;
	}
	if (param.no_fail_flag == 1) {/* この場合はデバッグ用にフェイルにはしない */
		/* フェイルだけどフェイルにしない */
		ret = 0;
	}
	
	return ret;
}


/* 同報指令の送信処理 */
static void SendBroadcast(void) {
	int bcc;
	if ((debug_type == TEST_NONE) || (debug_type == TEST_B)) {
		SetNowTime(&broadcast_command.t);/* 時刻セット */
		broadcast_command.h.s_no = 0;/* 通番 00H固定 */
		broadcast_command.h.sub_adr = LinkagePack(param.linkage_status);/* kaji20170407 連動設定値をsub_adrにセットする */
		bcc = CalcRealBcc((char *)&broadcast_command,sizeof(BROADCAST_COMMAND));
		broadcast_command.h.s_no = bcc;/* 20170305 Bccを通番にセットする */
		SendCom1(hComm1, (unsigned char *)&broadcast_command, sizeof(BROADCAST_COMMAND));
	}
}

/* 制御指令の送信処理 */
static void SendNormal(int no) {
	normal_command.h.dst = no;/* 宛先アドレス 02H～08H */
	
	normal_command.command.byte = 0;
	if (now_status.status == STATUS_P1) {
		normal_command.command.tuujou = 1;
	} else if (now_status.status == STATUS_P2) {
		normal_command.command.issou = 1;
	} else if (now_status.status == STATUS_P3) {
		normal_command.command.henni = 1;
	} else if (now_status.status == STATUS_FAIL) {
		normal_command.command.fail = 1;
	}
	if (now_status.mode == MODE_MANUAL) {
		normal_command.command.shudou = 1;
	} else if (now_status.mode == MODE_MONITOR) {/* kaji20170301 */
		normal_command.command.teishi = 1;/* kaji20170301 */
	}
	
	/* ここでCDSをチェックして調光を制御する */
	/*
	（2）変移中は強制調光低（蛍光灯ON）とする。
	とは
	調光一斉指令有り＋調光強制入
	でいい
	*/
	int cds = CDSRead();
	/* kaji20170330 ↓ */
	if (board_choukou_status == 1) {
		cds = 1;/* 点灯させる */
	}
	/* kaji20170330 ↑ */

	normal_command.light_command.issei = 1;/* 調光一斉指令有 */
	if ( cds == 1) {
		/* 明るいので調光強制切 */
//		ChoukouWrite(1);/* 調光強制出力処理 */
		//printf("明るいので調光強制切 %X\n", normal_command.light_command.byte);
		normal_command.light_command.choukou_kiri = 1;/* 調光強制切（高） */
		normal_command.light_command.choukou_iri = 0;/* 調光強制入（低） */
	} else {
		/* 暗いので暗いので調光強制入 */
//		ChoukouWrite(0);/* 調光強制出力処理 */
		//printf("暗いので調光強制入 %X\n", normal_command.light_command.byte);
		normal_command.light_command.choukou_kiri = 0;/* 調光強制切（高） */
		normal_command.light_command.choukou_iri = 1;/* 調光強制入（低） */
	}
	SetNowTime(&normal_command.t);/* 時刻セット */
	if ((debug_type == TEST_NONE) || (debug_type == TEST_B)) {
		int bcc;
		normal_command.h.s_no = 0;/* 通番 00H固定 */
		bcc = CalcRealBcc((char *)&normal_command,sizeof(NORMAL_COMMAND));
		normal_command.h.s_no = bcc;/* 20170305 Bccを通番にセットする */
		SendCom1(hComm1, (unsigned char *)&normal_command, sizeof(NORMAL_COMMAND));
	}
}

/**
 *	@brief 制御機Aの制御機Bからのデータの受信処理
 *
 *	@retval なし
 */
static void RcvControlerASrv(void) {
	char str[256];
	char str2[20];
	unsigned char conta_rcv_buf[RCV_BUFF_SIZE];
	
	while(!empty(CONTROLER_QUEUE)) {
		unsigned char d = peek(CONTROLER_QUEUE, 0);/* 先頭データは０か？ */
		if ( d != 0) {
			dequeue(CONTROLER_QUEUE);
			continue;
		}
		if (lenqueue(CONTROLER_QUEUE) >= 10) {
			int conta_wait_count = peek(CONTROLER_QUEUE, 9) + 10;
			if (conta_wait_count != 25){
				dequeue(CONTROLER_QUEUE);
			} else {
				//printf("%d\n",lenqueue(CONTROLER_QUEUE));
				if (lenqueue(CONTROLER_QUEUE) >= conta_wait_count) {
				//printf("X");
					/* データパケット受信 */
					int i;
					for(i = 0 ; i < conta_wait_count; i++) {
						conta_rcv_buf[i] = dequeue(CONTROLER_QUEUE);/* 受信データ取り出し */
					}
					CONTROL_HEADER *h =(CONTROL_HEADER *)conta_rcv_buf;
					if (h->info == 0x11) {/* 監視応答 */
						//sprintf(str, "RcvControlerASrv 監視応答 %d", h->dst);
						//DebugPrint("", str, 1);
						if ((h->dst < 1) || (h->dst >= CONTROLER_MAX)) {
							sprintf(str, "RcvControlerASrv 端末番号エラー %d",h->dst);
							DebugPrint("", str, 2);
						} else {
							RESPONSE_COMMAND * rp = (RESPONSE_COMMAND *) conta_rcv_buf;
							int bcc = rp->h.s_no;/* Bcc取り出し */
							rp->h.s_no = 0;
							int cbcc = CalcRealBcc(conta_rcv_buf,sizeof(RESPONSE_COMMAND));
							cbcc &= 0xff;
							if ( bcc == cbcc) {
//							if ( 1) {
								memmove(&response_command[h->src - 1], conta_rcv_buf, sizeof(RESPONSE_COMMAND));
								response_timer[h->src - 1] = 0;/* 制御器間監視応答受信待ちタイマクリア */
								response_received_flag[h->src - 1] = 1;
								response_received_timer = 0;/* 制御機Bから受信したのでクリア */
								SendResponse(&response_command[h->src - 1]);/* そのまま運用管理PCに監視応答を送信する */
								sprintf(str, "RcvControlerASrv 監視応答 %d src=%d %02X %02X %02X", h->dst, h->src
									, response_command[h->src - 1].response.status[0]
									, response_command[h->src - 1].response.status[1]
									, response_command[h->src - 1].response.status[2]
								);
								DebugPrint("", str, 8);
								if (response_command[h->src - 1].response.byte7.schedule_req != 0) {
									/* スケジュール登録依頼 */
									ScheduleSendReq ( );/* スケジュールの制御機Ｂへの送信要求処理 */
									sprintf(str,"スケジュールを制御機Bに送った");
									DebugPrint("", str, 0);
								}
							} else {
								sprintf(str, "RcvControlerASrv Bccエラー %d,%X<->%X",h->dst,bcc, cbcc);
								DebugPrint("", str, 2);
								str[0] = '\0';
								for ( i = 0; i < sizeof(RESPONSE_COMMAND);i++) {
									sprintf(str2,"%02X ",conta_rcv_buf[i]&0xff);
									strcat(str, str2);
								}
								DebugPrint("", str, 2);
							}
						}
					} else {
						//受信エラー
						sprintf(str, "RcvControlerASrv 情報種別エラー %02X",h->info);
						DebugPrint("", str, 2);
					}
				} else {
					break;
				}
			}
		} else {
			break;
		}
	}
}
/**
 *	@brief 制御機Aの監視盤からのデータの受信処理
 *
 *
 * 
 *
 *
 *	@retval なし
 */
static void RcvMonitorSrv(void) {
	char str[256];
	unsigned char contm_rcv_buf[RCV_BUFF_SIZE];

	
	while(!empty(MONITOR_QUEUE)) {
		unsigned char d = peek(MONITOR_QUEUE, 0);/* 先頭データは０か？ */
		if ( d != 0) {
			dequeue(MONITOR_QUEUE);
			continue;
		}
		if (lenqueue(MONITOR_QUEUE) >= 10) {
			int contm_wait_count = peek(MONITOR_QUEUE, 9) + 10;
			if (contm_wait_count != 19){
				dequeue(MONITOR_QUEUE);
			} else {
				//printf("%d\n",lenqueue(MONITOR_QUEUE));
				if (lenqueue(MONITOR_QUEUE) >= contm_wait_count) {
				//printf("X");
					/* データパケット受信 */
					int i;
					for(i = 0 ; i < contm_wait_count; i++) {
						contm_rcv_buf[i] = dequeue(MONITOR_QUEUE);/* 受信データ取り出し */
					}
					CONTROL_HEADER *h =(CONTROL_HEADER *)contm_rcv_buf;
					MONITOR_OPERATE_COMMAND *com = (MONITOR_OPERATE_COMMAND *)contm_rcv_buf;
					if (h->info == 0x81) {/* 運用停止解除コマンド */
						if ((com->command & 1) != 0) {
							monitor_command_request = 1;/* 監視盤からの要求あり */
							now_status.mode = MODE_MONITOR;/* 監視盤からの指令モードに切り替える */
							sprintf(str, "RcvMonitorSrv 運用停止コマンド");
							monitor_command_type = 1;/* 監視盤から運用停止要求 */
							EventRequest(MONITOR_REQUEST);/* 運用停止リクエスト */
						} else if ((com->command & 2) != 0){
							monitor_command_request = 1;/* 監視盤からの要求あり */
							now_status.mode = MODE_REMOTE;/* 遠隔モードに切り替える */
							sprintf(str, "RcvMonitorSrv 運用停止解除コマンド");
							monitor_command_type = 0;/* 監視盤から運用停止解除要求 */
							EventRequest(MONITOR_RELEASE_REQUEST);/* 運用停止解除リクエスト */
						} else {
							moniter_command_timer = 0;/* 監視盤間通信受信待ちタイマ */
							sprintf(str, "RcvMonitorSrv 運用ヘルスコマンド");
						}
						DebugPrint("", str, 1|4);
//						DebugPrint("", str, 0);
						
					} else {
						//受信エラー
						sprintf(str, "RcvMonitorSrv 情報種別エラー %02X",h->info);
						DebugPrint("", str, 2);
					}
				} else {
					break;
				}
			}
		} else {
			break;
		}
	}
}
/**
 *	@brief 制御機Aの監視盤へのデータの送信処理
 *
 *	@retval なし
 */
static void SendMonitorSrv(void)
{
//	char str[256];
	switch (monitor_send_stage_no) {
	case CS_ON_STAGE:/* MDM_CSの出力処理 */
		check_6s_timer = 0;/* 6秒s待ちタイマ */
		MdmcsWrite(1);
		check_delay_timer = 0;/* 待ちタイマ */
		monitor_send_stage_no = SEND_WAIT_STAGE1;
		DebugPrint("SendMonitorSrv", "CS_ON MdmcsWrite(1)", 4);
		break;
	case SEND_WAIT_STAGE1:/* MDM_CSの出力処理 */
		if (check_delay_timer > param.mdmcs_delay_time) {/* 待ちタイマ */
			monitor_send_stage_no = SEND_MONITOR_STAGE;
		}
		break;
	case SEND_MONITOR_STAGE:/* パワーON　同報指令送信 */
		DebugPrint("SendMonitorSrv", "送信開始", 4);
		SendMonitor();
		monitor_send_stage_no = WAITING_SEND_END_STAGE;
		break;
	case WAITING_SEND_END_STAGE://送信終了待ち
//		if (send_com3_p != sizeof(MONITOR_COMMAND)) {
//			SendCom3(hComm1, (unsigned char *)&monitor_command, sizeof(MONITOR_COMMAND));
//		} else {
		if (ChkSendCom3() == 0){/* kaji20170310 */
			/*  送信終了*/
			DebugPrint("SendMonitorSrv", "送信終了", 4);
			check_delay_timer = 0;/* 待ちタイマ */
			monitor_send_stage_no = SEND_WAIT_STAGE2;
		}
		break;
	case SEND_WAIT_STAGE2:/* MDM_CSの出力処理 */
		if (check_delay_timer > param.mdmcs_delay_time) {/* 待ちタイマ */
			MdmcsWrite(0);
			monitor_send_stage_no = WAITING_6S_TIME_STAGE;
			DebugPrint("SendMonitorSrv", "CS_OFF MdmcsWrite(0)", 4);
		}
		break;
	case WAITING_6S_TIME_STAGE://6s待ち
		if (check_6s_timer >= 6000) {
			monitor_send_stage_no = CS_ON_STAGE;
		}
		break;
	default:
		printf("SendMonitorSrv default error\n");
		break;
	}

}

/* 監視盤への送信処理 
	*/
static void SendMonitor(void) {
	SetNowTime(&monitor_command.t);/* 時刻セット */
	SetResponse();/* 自分の監視応答をバッファにセットする */
	int i;
	/* linkage_statusをdiv_noにセットする */
	monitor_command.h.div_no = LinkagePack(param.linkage_status);
	for ( i = 0; i < 8; i++) {
		memmove(monitor_command.command[i], response_command[i].response.status, 6);
		if (i != (CONTA_ADDRESS-1)) {
			if (response_timeout_flag[i] == 0) {
				//monitor_command.command[i][2] &= ~0x20;/* 無線異常をクリア */
				monitor_command.responce[i].response.byte2.musen_error = 0;/* 無線異常をクリア */
			} else {
				//monitor_command.command[i][2] |= 0x20;/* 無線異常をセット */
				monitor_command.responce[i].response.byte2.musen_error = 1;/* 無線異常をセット */
			}
		}
		/* 制御機Bが運用停止モードかどうかはここでセットする 20170218 */
		if (now_status.mode == MODE_MONITOR) {
			monitor_command.responce[i].response.status[0] |= 8;
		} else {//20170226
			monitor_command.responce[i].response.status[0] &= (~8);
		}
		
	}
	
//	send_com3_p = 0;
	SendCom3(hComm1, (unsigned char *)&monitor_command, sizeof(MONITOR_COMMAND));
}


/* 運用管理PCへの監視応答の送信処理 */
static void SendResponse(RESPONSE_COMMAND *response_command ) {
	char str[256];
	//if (send_stop_req != 0) {
	//	/* 送信を抑止する */
	//	return;
	//}
	sprintf(str, "運用管理PCに監視応答送信 %02X,%02X"
		,response_command->h.src
		,response_command->h.dst
	);
	DebugPrint("SendResponse", str, 8);
	if ((debug_type == TEST_NONE) || (debug_type == TEST_PC)) {
		response_command->h.sub_adr = LinkagePack(param.linkage_status);/* 連動設定値をsub_adrにセットする */
		SetNowTime(&response_command->t);/* 時刻セット */
		SendCom2(hComm1, (unsigned char *)response_command, sizeof(RESPONSE_COMMAND));
	}
}

/* 運用管理ＰＣからの受信処理 */
/*
	同報制御指令と同じフォーマットにする
		時刻情報受信
		本日のスケジュール受信
		設定値受信（オフセットタイマ,）
		テスト（アドレスで端末を決定する）
		端末切り離し（アドレスで端末を決定する、制御指令のDB7で設定,DB6:0:で切り離し,1:連動）
*/
static void RcvOperationManagementSrv(void) {
	char operation_management_rcv_buf[RCV_BUFF_SIZE];

//	while(!empty(PC_QUEUE)) {
	while(lenqueue(PC_QUEUE) >= 3) {/* kaji20170310 */
//		unsigned char d = peek(PC_QUEUE, 0);/* 先頭データは０か？ */
//		if ( d != 0) {
		unsigned char d0 = peek(PC_QUEUE, 0);/* 0byte目データは 5A か？ */
		unsigned char d1 = peek(PC_QUEUE, 1);/* 1byte目データは FF か？ */
		unsigned char d2 = peek(PC_QUEUE, 2);/* 2byte目データは 00 か？ */
		if ( (d0 != 0x5a)||(d1 != 0xff)||(d2 != 0x00) ) {/* kaji20170321修正 */
			dequeue(PC_QUEUE);
			continue;
		}
//		if (lenqueue(PC_QUEUE) >= 10) {
		if (lenqueue(PC_QUEUE) >= 10+2) {/* kaji20170310 */
//			unsigned int operation_management_wait_count = peek(PC_QUEUE, 9) + 10;
			unsigned int operation_management_wait_count = peek(PC_QUEUE, 9+2) + 10;/* kaji20170310 */
//			if ((operation_management_wait_count != 21) && (operation_management_wait_count != 28)){
			if ((operation_management_wait_count != 21) && 
				(operation_management_wait_count != 28) && 
				(operation_management_wait_count != 192)){
				printf("*** illigal_operation_management_wait_count_value=%d\n",operation_management_wait_count);
				dequeue(PC_QUEUE);
			} else {
				//printf("%d\n",lenqueue(PC_QUEUE));
//				if (lenqueue(PC_QUEUE) >= operation_management_wait_count) {
				if (lenqueue(PC_QUEUE) >= operation_management_wait_count+2) {/* kaji20170310 */
					/* データパケット受信 */
					int i;
					dequeue(PC_QUEUE);/* 0x5A 読み捨て kaji20170310 */
					dequeue(PC_QUEUE);/* 0xFF 読み捨て kaji20170310 */
					for(i = 0 ; i < operation_management_wait_count; i++) {
						operation_management_rcv_buf[i] = dequeue(PC_QUEUE);/* 受信データ取り出し */
					}
					CONTROL_HEADER *h =(CONTROL_HEADER *)operation_management_rcv_buf;
					if (operation_management_wait_count == 192 ) {
						printf("祝日変移休止日通知受信\n");
						SaveHoliday((HOLIDAY_COMMAND *) operation_management_rcv_buf);/* 変移休止日 */
					}
					else if (h->dst == 0) {/* 同報制御指令 */
					//if (h->info == 0x11) {/* 同報指令 */
						BROADCAST_COMMAND *com = (BROADCAST_COMMAND *)operation_management_rcv_buf;
						if (com->light_command.time_req != 0) {
							/* 時刻修正要求受信 */
							SetTime(&(com->t));/* 時刻設定処理 */
							SaveRTC(&(com->t));/* 不揮発用RTCに書き込む */
							DebugPrint("","RcvOperationManagementSrv 運用管理ＰＣから 時刻修正要求受信", 0);
						}
						if (com->light_command.sreq != 0) {
							/* スケジュール登録要求受信 */
							DebugPrint("","RcvOperationManagementSrv 運用管理ＰＣから スケジュール登録要求受信", 0);
						}
						if ((com->light_command.byte&0x80) != 0) {
							/* スケジュール登録要求受信 */
							DebugPrint("","RcvOperationManagementSrv 運用管理ＰＣから 送信停止要求を受信した", 0);
							//send_stop_req = 1;/* 送信を抑止する */
						} else {
							//send_stop_req = 0;/* 送信を抑止しない */
						}
						DebugPrint("","RcvOperationManagementSrv 運用管理ＰＣから受信", 0);
						BroadcastCommand(com);/* 同報指令受信処理 */
						int diff_time = CheckDiffTime(&(com->t));
						if (diff_time > TIME_ERROR_VAL) {
							/* 時刻エラーとなる時間の差(秒) */
							now_status.time_status = 1;
							now_status.schedule = 0;/* 20170224 これでスケジュール登録要求を送信する */
							now_status.time_req = 0;/* 20170224 これで時刻設定要求を送信する */
						} else {
							now_status.time_status = 0;
						}
						//if ((com->status.byte&6) == 0){/* 20170219 GPS状態変化は送信しない */
						if (((com->status.byte&6) == 0) && (com->light_command.rendou_req == 0)){
							/* 20170320 GPS状態変化 or 連動設定要求は送信しない */
							operation_management_request = 1;/* 運用管理PCからの同報通知有り */
							/* 同報通知をセット */
							memmove(&operation_management_rcv_command, operation_management_rcv_buf, sizeof(BROADCAST_COMMAND));
						} else {
							if (com->light_command.rendou_req != 0) {
								unsigned char link_status = LinkagePack(param.linkage_status);
								if (link_status != h->sub_adr) {
									printf("連動設定値が変わった %X->%X\n",link_status,h->sub_adr);
									param.linkage_status = LinkageUnPack(h->sub_adr);
									DebugPrint("", "パラメータが変わったためパラメータセーブ\n", 0);
									SaveParam();
								}
							}
						}
					} else {/* 通常指令 */
						/* 運用管理ＰＣからのヘルスを受信 */
						normal_command_timer = 0;/* 通常指令受信待ちタイマ */
						DebugPrint("","RcvOperationManagementSrv 運用管理ＰＣから通常指令（ヘルス）受信", 1);
						BROADCAST_COMMAND *com = (BROADCAST_COMMAND *)operation_management_rcv_buf;
						int diff_time = CheckDiffTime(&(com->t));
						if (diff_time > TIME_ERROR_VAL) {
							/* 時刻エラーとなる時間の差(秒) */
							now_status.time_status = 1;
							now_status.time_req = 0;/* 20170224 これで時刻設定要求を送信する */
						} else {
							now_status.time_status = 0;
						}
					}
				} else {
					break;
				}
			}
		} else {
			break;
		}
	}
}

/**
 *	@brief 本日のスケジュールをセットする処理
 *
 *	@retval なし
 */
static void SetSchedule( void )
{
	int ret ;
	char str[256];
	ret = CheckTodaysStartTime();
	if (ret == TODAY_IS_NORMAL_DAY) {
		now_status.start_time = param.start_time;
		now_status.end_time = param.end_time;
		now_status.offset_timer = param.offset_timer;
	} else {
		now_status.start_time = 0;
		now_status.end_time = 0;
		now_status.offset_timer = param.offset_timer;
	}
	sprintf(str,"自分でスケジュール登録を行った %d", ret);
	DebugPrint("", str, 0);
	ScheduleSendReq ( );/* スケジュールの制御機Ｂへの送信要求処理 */
}

/**
 *	@brief スケジュールの制御機Ｂへの送信要求処理
 *
 *	@retval なし
 */
void ScheduleSendReq ( void )
{
	operation_management_request =  1;/* これをセットしないと送ってくれない */
	SetNowTime(&(operation_management_rcv_command.t));/* 時刻設定処理 */
	operation_management_rcv_command.light_command.sreq = 1;/* スケジュール設定要求 */
	operation_management_rcv_command.light_command.time_req = 1;/* 時刻修正要求 */
	operation_management_rcv_command.schedule.start_time[0] = BCD((now_status.start_time/60) / 60);
	operation_management_rcv_command.schedule.start_time[1] = BCD((now_status.start_time/60) % 60);
	operation_management_rcv_command.schedule.end_time[0] = BCD((now_status.end_time/60) / 60);
	operation_management_rcv_command.schedule.end_time[1] = BCD((now_status.end_time/60) % 60);
	operation_management_rcv_command.schedule.offset_timer = BCD(now_status.offset_timer/10);
	
}

/**
 *	@brief 開始時刻の判定処理
 *
 *	@retval その日の状態
 */
int CheckTodaysStartTime()
{
	TIME_INFO t;
	TIME_INFO_BIN tb;
	SetNowTime(&t);
	tb.year = 100 * BIN(t.year[0]) + BIN(t.year[1]);
	tb.month = BIN(t.month);
	tb.day = BIN(t.day);
	tb.week = subZeller(tb.year, tb.month, tb.day)+1;//日曜日を１とする
	//tb.week = t.holiday_week.week;
	return CheckStartTime(&tb);
}

/**
 *	@brief 開始時刻の判定処理
 *
 *	@param [TIME_INFO_BIN *t] 日時格納ポインタ
 *
 *	@retval その日の状態
 */
/*
春分の日(３月XX日)
　= int(20.8431+0.242194*(年-1980)-int((年-1980)/4))
秋分の日(9月XX日)
　= int(23.2488+0.242194*(年-1980)-int((年-1980)/4))
*/
int CheckStartTime(TIME_INFO_BIN *t)
{
	int i;
	char str[256];
	
	sprintf(str, "%d年%d月%d日",t->year,t->month,t->day);
	DebugPrint("CheckStartTime", str, 2);
	if (CheckKyuushiday(t) != 0)
	{
		/* 変移休止日 */
		//20170224 return TODAY_IS_NORMAL_DAY;
		return TODAY_IS_HOLIDAY;
	}
	
	
	for ( i = 0; i < holiday_data.holiday_count; i++) {
		if (holiday_data.holiday[i].type == 0) {
			/* 普通の祝日 */
			if ((holiday_data.holiday[i].month == t->month) && /* 月が一致 */
				(holiday_data.holiday[i].day == t->day) ) { /* 日が一致 */
				/* 祝日 */
				return TODAY_IS_HOLIDAY;
			}
		}
		if (holiday_data.holiday[i].type == 1) {
			/* ハッピーマンデイ */
			if (holiday_data.holiday[i].month == t->month) {
				/* 月が一致 */
				if (t->week == 2) {/* 月曜日? */
					/* 月曜日 */
					int m = (t->day-1)/7+1;
					if (holiday_data.holiday[i].day == m) {
						/* 第Ｘ番目の月曜日が一致 */
						return TODAY_IS_HOLIDAY;
					}
				}
			}
		}
	}
	int year = t->year;
	if (t->month == 3) {
		/* 春分の日の月 */
		int d = (int)(20.8431+0.242194*(year-1980)-(int)((year-1980)/4));
		//printf("year = %d,d=%d\n",year,d);
		if (t->day == d) {
			/* 春分の日 */
			return TODAY_IS_HOLIDAY;
		}
	}
	if (t->month == 9) {
		/* 秋分の日の月 */
		int d = (int)(23.2488+0.242194*(year-1980)-(int)((year-1980)/4));
		//printf("year = %d,d=%d\n",year,d);
		if (t->day == d) {
			/* 秋分の日 */
			return TODAY_IS_HOLIDAY;
		}
	}

	if (t->week == 1) {/* 日曜日? */
		return TODAY_IS_HOLIDAY;
	}
	//ここからは振り替え祝日かどうかをチェックする
	TIME_INFO_BIN tmp;
	int count = t->week - 1; /* 月曜日を１とする */
	tmp = *t;
	
static int month_day[12]={0x31,0x28,0x31,0x30,0x31,0x30,0x31,0x31,0x30,0x31,0x30,0x31};
	for (i = 0 ; i < count; i++)
	{
		tmp.day = tmp.day - 1;
		if (tmp.day == 0) {
			tmp.month = tmp.month - 1;
			if (tmp.month == 0) {
				tmp.month = 0x12;
			}
			if (tmp.month == 2) {
				int n;
				int y = tmp.year;
				n = 28 + (1 / (y - y / 4 * 4 + 1)) * (1 - 1 / (y - y / 100 * 100 + 1))
					+ (1 / (y - y / 400 * 400 + 1));	
				tmp.day = n;
			} else {
				tmp.day = month_day[tmp.month - 1];
			}
		}
		
		//printf("m=%X,d=%x\n",tmp.month,tmp.day);
		if (CheckHoliday(&tmp) != TODAY_IS_HOLIDAY)
		{
			 return TODAY_IS_NORMAL_DAY;
		}
	}
	return TODAY_IS_HOLIDAY;
}

/**
 *	@brief 休日かどうかの判定処理
 *
 *	@param [TIME_INFO_BIN *t] 日時格納ポインタ
 *
 *	@retval 判定結果
 */
int CheckHoliday(TIME_INFO_BIN *t)
{
	int i;
	for ( i = 0; i < holiday_data.holiday_count; i++) {
		if (holiday_data.holiday[i].type == 0) {
			/* 普通の祝日 */
			if ((holiday_data.holiday[i].month == t->month) && /* 月が一致 */
				(holiday_data.holiday[i].day == t->day) ) { /* 日が一致 */
				/* 祝日 */
				return TODAY_IS_HOLIDAY;
			}
		}
		int year = t->year;
		if (t->month == 3) {
			/* 春分の日の月 */
			int d = (int)(20.8431+0.242194*(year-1980)-(int)((year-1980)/4));
			//printf("year = %d,d=%d\n",year,d);
			if (t->day == d) {
				/* 春分の日 */
				return TODAY_IS_HOLIDAY;
			}
		}
		if (t->month == 9) {
			/* 秋分の日の月 */
			int d = (int)(23.2488+0.242194*(year-1980)-(int)((year-1980)/4));
			//printf("year = %d,d=%d\n",year,d);
			if (t->day == d) {
				/* 秋分の日 */
				return TODAY_IS_HOLIDAY;
			}
		}
	}
	return TODAY_IS_NORMAL_DAY;
}
/**
 *	@brief 休日かどうかの判定処理
 *
 *	@param [TIME_INFO_BIN *t] 日時格納ポインタ
 *
 *	@retval 判定結果 0:休止日ではない,1:休止日
 */
int CheckKyuushiday(TIME_INFO_BIN *t)
{
	int i;
	for ( i = 0; i < holiday_data.kyuushi_count; i++) {
		if ((holiday_data.kyuushi[i].month == t->month) && /* 月が一致 */
			(holiday_data.kyuushi[i].day == t->day) ) { /* 日が一致 */
			/* 休止日 */
			return 1;
		}
	}
	return 0;
}

/**
 *	@brief 変移休止日のセーブ処理
 *
 *	@retval なし
 */
static void SaveHoliday(HOLIDAY_COMMAND *p)
{
	memmove(&holiday_data,(char *)(p->d), 182);
	int bcc = CalcBcc((char *)&holiday_data, sizeof(HOLIDAY_DATA) - sizeof(int));
	holiday_data.bcc = bcc;
#ifdef windows	

	FILE *fp;
	char fname[256];
#define HOLIDAY_FILE_NAME ("holiday.ini")

	sprintf(fname,"%s",HOLIDAY_FILE_NAME);
	fp = fopen(fname, "wb");
    if (fp == NULL) {
		printf("%s:%s is not opened!\n",__func__,fname);
    }
	fwrite(&holiday_data, sizeof(HOLIDAY_DATA), 1, fp);
	fclose(fp);

#else
	flash_sector_erase(HOLIDAY_ADDRESS);
	flash_write_buf(HOLIDAY_ADDRESS, (int)&holiday_data, sizeof(HOLIDAY_DATA) / 2);
#endif
}

/**
 *	@brief 変移休止日のロード処理
 *
 *	@retval なし
 */
static void LoadHoliday(void)
{
#ifdef windows	

	FILE *fp;
	char fname[256];
#define HOLIDAY_FILE_NAME ("holiday.ini")

	sprintf(fname,"%s",HOLIDAY_FILE_NAME);
	fp = fopen(fname, "rb");
    if (fp == NULL) {
		printf("%s:%s is not opened!\n",__func__,fname);
    }
	fread(&holiday_data, sizeof(HOLIDAY_DATA), 1, fp);
	fclose(fp);
#else
	HOLIDAY_DATA *p = (HOLIDAY_DATA *)HOLIDAY_ADDRESS;
	int bcc = CalcBcc((char *)p,sizeof(HOLIDAY_DATA) - sizeof(int));
	if (bcc == p->bcc) {
		//printf("holiday_data load success bcc=%X\n",bcc);
		memmove(&holiday_data, p , sizeof(HOLIDAY_DATA) );
	} else {
		printf("holiday_data load fail p->bcc=%.08X %.08X\n", p->bcc,bcc ) ;
	}
#endif
}
/**
 *	@brief 監視応答の表示処理
 *
 *	@retval なし
 */
void ContAResponseDisp ( void )
{
	int i;
	RESPONSE_COMMAND *p;
	printf("■■■監視応答 ■■■\n");
	for (i = 0; i < CONTROLER_MAX ; i++) {
		int d = (param.linkage_status >> (4*(CONTROLER_MAX - i - 1)))&0xf;
		if (d != 0) {
			p = &response_command[i] ;
			printf("時刻 %02X%02X/%02X/%02X %02X:%02X:%02X\n",p->t.year[0]
													,p->t.year[1]
													,p->t.month
													,p->t.day
													,p->t.hour
													,p->t.min
													,p->t.sec);
			printf("休日(DB7 1)、曜日 %02X\n",p->t.holiday_week.byte);
			printf("応答 %02X,%02X,%02X,%02X,%02X,%02X,%02X\n"
				,p->response.byte1.byte
				,p->response.byte2.byte
				,p->response.byte3.byte
				,p->response.byte4.byte
				,p->response.byte5.byte
				,p->response.byte6.byte
				,p->response.byte7.byte
			);
		}
	}
	
}

/**
 *	@brief 運用管理ＰＣの電源制御処理
 *
 *	@retval なし
 */
static void PCPowerSrv(void)
{
//	return;
	char str[256];
	switch (pcpower_stage_no) {
	case PCPOWER_IDLE:/* アイドル状態 */
		if (param.no_pc_check_flag == 0) {
			if (now_status.pc_tuushin_status != 0) {
				PcPower(1);
				pcpower_timer = 0;/* 待ちタイマ */
				shutdown_width = SHUTDOWN_WIDTH;/* シャットダウンの押し幅を保持 */
				pcpower_stage_no = PCPOWER_SHUTDOWN_ON;
				DebugPrint("PCPowerSrv", "通信異常検出 シャットダウン信号をＯＮする", 4);
			} else
			if (now_status.power_outage_flag != 0) {
				/* 停電検出 */
				PcPower(1);
				pcpower_timer = 0;/* 待ちタイマ */
				shutdown_width = CHOTTO_SHUTDOWN_WIDTH;/* ちょっと押しシャットダウンの押し幅を保持 */
				pcpower_stage_no = PCPOWER_SHUTDOWN_ON;
				DebugPrint("PCPowerSrv", "停電検出 シャットダウン信号をＯＮする", 0);
			}
		}
		break;
	case PCPOWER_SHUTDOWN_ON:/* MDM_CSの出力処理 */
		if (pcpower_timer > (1000 * shutdown_width)) {/* shutdown_width秒待ちタイマ */
			PcPower(0);
			pcpower_timer = 0;/* 待ちタイマ */
			pcpower_stage_no = PCPOWER_SHUTDOWN_OFF;
			DebugPrint("PCPowerSrv", "シャットダウン信号をＯＦＦする", 0);
		}
		break;
	case PCPOWER_SHUTDOWN_OFF:/* パワーON　同報指令送信 */
		if (pcpower_timer > (1000 * SHUTDOWN_WAIT)) {/* SHUTDOWN_WAIT秒待ちタイマ */
			pcpower_timer = 0;/* 待ちタイマ */
			pcpower_stage_no = PCPOWER_WAIT;
			sprintf(str, "シャットダウン信号をＯＦＦから%d秒経過 ",SHUTDOWN_WAIT );
			DebugPrint("PCPowerSrv", str, 0);
		}
		break;
	case PCPOWER_WAIT://シャットダウン信号制御後SHUTDOWN_WAIT秒経過時にここに来る
		if (now_status.pc_tuushin_status == 0) {
			pcpower_stage_no = PCPOWER_IDLE;
			DebugPrint("PCPowerSrv", "通信確立を検出 ", 0);
		} else if (now_status.power_outage_flag == 0) {
			//停電からの復帰を検出
			PcPower(1);
			pcpower_timer = 0;/* 待ちタイマ */
			shutdown_width = SHUTDOWN_WIDTH;/* シャットダウンの押し幅を保持 */
			pcpower_stage_no = PCPOWER_SHUTDOWN_ON;
			DebugPrint("PCPowerSrv", "停電からの復帰を検出 シャットダウン信号をＯＮする", 0);
		} else {
		}
		break;
	default:
		printf("PCPowerSrv default error\n");
		break;
	}

}

