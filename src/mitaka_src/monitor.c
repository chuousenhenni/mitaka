/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	monitor.c
 *	概要
 *  監視盤制御部
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
 
enum {/* 監視盤送信処理に使用する状態変数 */
	SEND_MONITOR_IDLE_STAGE = 0,
	PRE_DELAY_SET_STAGE, /* kaji20170310 */
	PRE_DELAY_STAGE, /* kaji20170310 */
	CS_ON_STAGE,
	SEND_WAIT_STAGE1,
	SEND_MONITOR_STAGE,
	WAITING_SEND_END_STAGE,
	SEND_WAIT_STAGE2,
	CS_OFF_STAGE,
};

#define MONITOR_TIMEOUT_VALUE (30) /* 監視盤受信タイムアウト判定値(秒) */

/*
 *===========================================================================================
 *					内部変数定義
 *===========================================================================================
 */

static int monitor_ms_timer;/* msタイマ */
static int monitor_sec_timer;/* 秒タイマ */
static int check_delay_timer;/* 待ちタイマ */
static int inhibit_timer;/* 送信禁止用タイマ */
static int inhibit_flag;/* 送信禁止時は１となる */

static int send_request_flag;/* 運用停止、解除送信要求フラグ */
static int send_request_data;/* 運用停止、解除送信要求データ */

static int monitor_command_rcv_count;/* 制御指令コマンド受信回数 */
static int bef_monitor_command_rcv_count;/* 制御指令コマンド受信回数 */
static int snd_count;/* 送信回数 */
static int each_snd_count[3];/* ヘルス,運用停止、運用停止解除の送信回数 */

static int check_monitor_timer;/* 監視盤受信タイムアウト判定用タイマ */
static int monitor_timeout_value;/* 監視盤受信タイムアウト値 */
int monitor_timeout_flag;/* 監視盤受信タイムアウトフラグ */
static int bef_monitor_timeout_flag;/* 前回の監視盤受信タイムアウトフラグ */
static int check_monitor_led_ms_timer;/* 監視盤用の1ms毎に更新するタイマ */

static int bef_btn_status;/* 前回のボタンの状態を保持 */
int btn_status;/* ボタンの状態を保持 */
static int buzzer_stop_flg;/* ブザー停止フラグ */
static int maintenance_flg;/* 保守ボタンが押されたフラグ */

static int monitor_bef_led_status[ MONITOR_LED_MAX_COUNT];/* 前回のLED状態 */
int monitor_led_status[ MONITOR_LED_MAX_COUNT];/* LED状態 */
static int monitor_led_toggle_status;/* LED 点滅モードので現在の状態 */
static int monitor_led_toggle_count;/* LED 点滅モードを切り替えるまでの回数 */
static int check_monitor_led_sec_timer;/* 監視盤LED制御用タイマ更新 */

int monitor_command_flg ;/* 運用指令ボタン押されたフラグ */
static int bef_monitor_command_flg ;/* 前回運用指令ボタン押されたフラグ */
MONITOR_COMMAND monitor_command;/* 制御機からの制御指令の電文  */
static MONITOR_OPERATE_COMMAND monitor_operate_command;/* 制御機への運用指令の電文  */
static int monitor_send_operate_stage_no;

static int monitor_buzzer_status;/*  警報ブザー状態*/

static int monitor_request_status;/*  送信要求の種類(運用停止解除:0,運用停止:1) */
static int monitor_status;/* 受信ステータス(運用停止解除:0,運用停止:1,応答確定待ち状態:2) */
static int hoshu_status;/* 制御器Aの保守ボタン状態を保持 */

/*
 *===========================================================================================
 *					外部変数定義
 *===========================================================================================
 */

//extern int send_com3_p;
extern HANDLE hComm1;       /* シリアルポートのハンドル */
extern void LoadParam(void);
extern PARAM param;

/*
 *===========================================================================================
 *					内部	関数定義
 *===========================================================================================
 */

static void RcvMonitorSrv(void);/* 制御機から監視盤へのデータ受信処理 */
static void CheckStatus(void);/* 状態判定処理 */
static void CheckBtn(void);/* 各種ボタン入力判定処理 */
static void CheckContStatus(void);/* 制御機の状態をチェックする処理 */
static int CheckContStatusSub(int i, int *err_status);/* 各制御機の状態をチェックする処理 */
static void MonitorLEDInit(void);
static void MonitorLEDSrv(void);/* 監視盤のLED制御処理 */
static void SendOperateCommand(int flag);/* 運用指令の送信処理 */
static void SendOperateCommandSrv(void);/* 制御機への運用指令の実際の送信処理 */
static void MonitorCommandSrv(void);/* 監視盤への制御指令受信処理 */

/*
 *===========================================================================================
 *					グローバル関数定義
 *===========================================================================================
 */

void MonitorInit(void);
void MonitorSrv(void);/* 監視盤の制御処理 */
void TimerIntMonitor(int count);/* 監視盤のタイマ割り込み処理 */
void MonitorCommandDisp(void);/* 監視盤への制御指令表示処理 */
void MonitorStatusDisp(void);/* ステータスの表示処理 */

/*
 *===========================================================================================
 *					外部	関数定義
 *===========================================================================================
 */

extern void SendCom3(HANDLE h, unsigned char *p, int size);
extern int ChkSendCom3(void);

/*
 *===========================================================================================
 *					グローバル関数
 *===========================================================================================
 */

/**
 *	@brief 監視盤のタイマ割り込み処理
 *
 *	@retval なし
 */
void TimerIntMonitor(int count)
{
	
	monitor_ms_timer += count;
	check_delay_timer += count;/* 待ちタイマ */
	
	inhibit_timer+= count;/* 送信禁止用タイマ */
	if (inhibit_timer > 5*1000) {
		/* 送信後5秒経過後から次を受信までは送信禁止とする */
		inhibit_flag = 1;/* 送信禁止時は１となる */
	}
	
	check_monitor_led_ms_timer+= count;/* 監視盤用の1ms毎に更新するタイマ */
	if (monitor_ms_timer >= 1000) {
		monitor_ms_timer = 0;
		monitor_sec_timer++;
		check_monitor_timer++;/* 監視盤受信タイムアウト判定用タイマ更新 */
		if (check_monitor_timer > monitor_timeout_value) {
			monitor_timeout_flag = 1;
		}
		check_monitor_led_sec_timer++;/* 監視盤LED制御用タイマ更新 */
	}
}

/**
 *	@brief 監視盤の初期化処理
 *
 *	@retval なし
 */
void MonitorInit(void)
{
	int i;
//	memset(&monitor_command, 0, sizeof(monitor_command));/* 20170206 */
	/* 制御機からの監視盤への制御指令の電文フォーマット */
	monitor_command.h.no = 0;/* 規格番号 00H固定 */
	monitor_command.h.dst = 0x10;/* 宛先アドレス 80H固定 */
	monitor_command.h.src = 1;/* 発信元アドレス  01H固定 */
	monitor_command.h.sub_adr = 0;/* サブアドレス 00H固定 */
	monitor_command.h.priority = 2;/* 優先レベル 02H固定 */
	monitor_command.h.s_no = 0;/* 通番 00H固定 */
	monitor_command.h.contoroler_no = 0x19;/* 端末種別 19H */
	monitor_command.h.info = 0x11;/* 情報種別 11H */
	monitor_command.h.div_no = 0x81;/* 分割番号 81H */
//	monitor_command.h.length = 162;/* データ長 162(A2H) */
	monitor_command.h.length = 8+6*8;/* データ長 162(A2H) */
	monitor_command.t.holiday_week.holiday = 0;/* 休日 */
	monitor_command.t.holiday_week.week = 0;/* 、曜日 */
//	memset(monitor_command.reserved1, 0, sizeof(monitor_command.reserved1));
	memset(monitor_command.command, 0, sizeof(monitor_command.command));
//	memset(monitor_command.reserved2, 0, sizeof(monitor_command.reserved2));
	for ( i = 0; i < CONTROLER_MAX; i++) {
		//yamazaki monitor_command.command[i][1] = 0x20;/* 光無線異常としておく */
	}

	/* 監視盤から制御機への電文 運用指令の電文フォーマット */
	monitor_operate_command.h.no = 0;/* 規格番号 00H固定 */
	monitor_operate_command.h.dst = 1;/* 宛先アドレス 01H固定（未使用） */
	monitor_operate_command.h.src = 1;/* 発信元アドレス  01H固定 */
	monitor_operate_command.h.sub_adr = 0;/* サブアドレス 00H固定 */
	monitor_operate_command.h.priority = 2;/* 優先レベル 02H固定 */
	monitor_operate_command.h.s_no = 0;/* 通番 00H固定 */
	monitor_operate_command.h.contoroler_no = 0x19;/* 端末種別 19H */
	monitor_operate_command.h.info = 0x81;/* 情報種別 81H */
	monitor_operate_command.h.div_no = 0x81;/* 分割番号 81H */
//	monitor_operate_command.h.length = 162;/* データ長 162(A2H) */
	monitor_operate_command.h.length = 9;/* データ長 56(38H) */
	monitor_operate_command.t.holiday_week.holiday = 0;/* 休日 */
	monitor_operate_command.t.holiday_week.week = 0;/* 、曜日 */
	monitor_operate_command.command = 0;
	
	monitor_command_flg  = 0;/* 運用指令ボタン押されたフラグ */
	bef_monitor_command_flg  = 0;

	MonitorCommandDisp();/* 監視盤への制御指令表示処理 */
	monitor_command_rcv_count = 0;
	bef_monitor_command_rcv_count = 0;
	snd_count = 0;/* 送信回数 */
	for ( i = 0 ;i < 3; i++) {
		each_snd_count[i] = 0;/* ヘルス,運用停止、運用停止解除の送信回数 */
	}
	check_monitor_timer = 0;/* 監視盤受信タイムアウト判定用タイマ */
	monitor_timeout_flag = 0;/* 監視盤受信タイムアウトフラグ */
	bef_monitor_timeout_flag = 0;/* 前回の監視盤受信タイムアウトフラグ */
	monitor_timeout_value = MONITOR_TIMEOUT_VALUE;/* 監視盤受信タイムアウト判定値(秒) */

	/* 以下はテスト用 */
	//monitor_led_status[0] = LED_STATUS_TOGGLE;

	btn_status = 0;/* ボタンの状態を保持 */
	bef_btn_status = btn_status;/* 前回のボタンの状態を保持 */
	buzzer_stop_flg = 0;/* ブザー停止フラグ */
	maintenance_flg = 0;/* 保守ボタンが押されたフラグ */
	monitor_buzzer_status = 0;/*  警報ブザー状態*/

	monitor_request_status = 0;/*  送信要求の種類(運用停止解除:0,運用停止:1) */
	monitor_status = 0;/* 受信ステータス(運用停止解除:0,運用停止:1,応答確定待ち状態:2) */
	hoshu_status = 0;/* 制御器Aの保守ボタン状態を保持 */
	monitor_send_operate_stage_no = SEND_MONITOR_IDLE_STAGE;

	monitor_ms_timer = 0;/* msタイマ */
	monitor_sec_timer = 0;/* 秒タイマ */
	inhibit_timer = 0;/* 送信禁止用タイマ */
	inhibit_flag = 0;/* 送信禁止時は１となる */
	send_request_flag = 0;/* 運用停止、解除送信要求フラグ */
	send_request_data = 0;/* 運用停止、解除送信要求データ */

	SetDefault();/* ディフォルトパラメータ設定 kaji20170316 */
//	param.debug_flg = 0xf;/* デバッグ用の表示を行う場合に0以外をセットする */
	//param.linkage_status = DEFAULT_LINKAGE; /* 連動設定 */
//	param.mdmcs_delay_time = 20;/* MDM_CS出力の遅延時間(ms) */
//	param.reset_count = 0;
	
	LoadParam();
	param.linkage_status = 0x11111111; /* 連動設定(制御機からもらうように変更) */
//	param.reset_count++;
//	SaveParam();
	MdmcsWrite(0);
	MonitorLEDInit();
	btn_status = MonitorBtnRead();/* 監視盤ボタン読み込み処理 */
	if ((btn_status & MAINTENANCEL_BIT) != 0) {
		maintenance_flg = 1;/* 保守ボタンが押された状態 */
	} else {
		maintenance_flg = 0;/* 保守ボタンが離された状態 */
	}
}

/**
 *	@brief 監視盤の制御処理
 *
 *	@retval なし
 */
void MonitorSrv(void)
{
	RcvMonitorSrv();/* 制御機から監視盤へのデータ受信処理 */
	CheckStatus();/* 状態判定処理 */
	CheckBtn();/* 各種ボタン入力判定処理 */
	MonitorLEDSrv();/* 監視盤のLED制御処理 */
	
	//if ((inhibit_flag == 0) && (send_request_flag == 1)) {
	//	/* 送信禁止期間でなかったら運用停止、解除送信要求を送信する */
	//	send_request_flag = 0;
	//	SendOperateCommand(monitor_command_flg);/* 運用指令の送信処理 */
	//}
	SendOperateCommandSrv();/* 制御機への運用指令の実際の送信処理 */
	
}

/*
 *===========================================================================================
 *					内部関数
 *===========================================================================================
 */

/**
 *	@brief 監視盤のLED初期化処理
 *
 *	@retval なし
 */
static void MonitorLEDInit(void)
{
	int i;

	check_monitor_led_sec_timer = 0;/* 監視盤LED制御用タイマ更新 */

	monitor_led_toggle_count = TOGGLE_COUNT_MAX;
	monitor_led_toggle_status = LED_OFF;
	for ( i = 0 ; i < MONITOR_LED_MAX_COUNT; i++) {
		monitor_bef_led_status[i] = LED_STATUS_OFF;
		monitor_led_status[i] = LED_STATUS_OFF;
	}
}

/**
 *	@brief 状態判定処理
 *
 *	@retval なし
 */
static void CheckStatus(void)
{
	int i;
	char str[256];

	if (bef_monitor_command_rcv_count != monitor_command_rcv_count){
		CheckContStatus();/* 制御機の状態をチェック */
	}
	bef_monitor_command_rcv_count = monitor_command_rcv_count;
	
	if (monitor_timeout_flag == 1) {
#if 1
/* kaji20170223 */
		for (i = 0; i < CONTROLER_MAX ; i++) {
			monitor_led_status[2 * i + 0] = LED_STATUS_OFF;
			if (i == CONTA_ADDRESS-1) {
				monitor_led_status[2 * i + 1] = LED_STATUS_ORANGE_TOGGLE;
			} else {
				monitor_led_status[2 * i + 1] = LED_STATUS_OFF;
			}
		}
		if ((maintenance_flg == 0) && (buzzer_stop_flg == 0) &&  (hoshu_status == 0)) {
			monitor_buzzer_status = 1;/*  警報ブザー状態*/
			MonitorBuzzerWrite(1);/* 警部ブザー */
		}
		monitor_led_status[ALARM_LAMP_LED] = LED_STATUS_ORANGE;/* 20170205 */
/* kaji20170223 */
#else
		for (i = 0; i < CONTROLER_MAX ; i++) {
			int d = (param.linkage_status >> (4*(CONTROLER_MAX - i - 1)))&0xf;
			if (d != 0) {
				/* 有効な制御機の回線異常 橙点滅を設定する */
				//printf("回線異常 橙点滅 %d\n",2 * i + 1);
				monitor_led_status[2 * i + 1] = LED_STATUS_ORANGE_TOGGLE;
				if ((maintenance_flg == 0) && (buzzer_stop_flg == 0) &&  (hoshu_status == 0)) {
					monitor_buzzer_status = 1;/*  警報ブザー状態*/
					MonitorBuzzerWrite(1);/* 警部ブザー */
				}
				monitor_led_status[ALARM_LAMP_LED] = LED_STATUS_ORANGE;/* 20170205 */
			}
		}
#endif
	}
	
	if ((monitor_buzzer_status == 1 ) && (hoshu_status == 1)) {
		monitor_buzzer_status = 0;/*  警報ブザー状態*/
		MonitorBuzzerWrite(0);/* 警部ブザー */
	}
	
	if (bef_monitor_timeout_flag != monitor_timeout_flag) {
		if (monitor_timeout_flag == 1) {
			/* 運用管理PC間通信異常 */
			sprintf(str, "運用管理PC間通信異常");
			//monitor_timeout_flag = 0;
			//check_monitor_timer =0;
		} else {
			/* 運用管理PC間通信正常 */
			sprintf(str, "運用管理PC間通信正常");
		}
		DebugPrint("", str, 1);
	}
	bef_monitor_timeout_flag = monitor_timeout_flag;
}


/**
 *	@brief 各種ボタン入力判定処理
 *
 *	@retval なし
 */
static void CheckBtn(void)
{
	btn_status = MonitorBtnRead();/* 監視盤ボタン読み込み処理 */
	char str[256];
	if (bef_btn_status != btn_status) {
		sprintf(str, "bef_sw(%.02X)->sw(%.02X)", bef_btn_status, btn_status);
		DebugPrint("", str, 0);
		
		if ((btn_status & MAINTENANCEL_BIT)) {
			/* 保守ボタンが押された状態 */
			maintenance_flg = 1;
			if (monitor_buzzer_status == 1) {
				monitor_buzzer_status = 0;/*  警報ブザー状態*/
				MonitorBuzzerWrite( 0);/* 監視盤ブザーへの停止出力処理 */
				printf("警報ブザーを停止する 1\n");
			}
			/*  警報ブザー状態*/
		} else {
			/* 保守ボタンが離されていない状態 */
			maintenance_flg = 0;
		}
		
		if (monitor_buzzer_status == 1) {/*  警報ブザー状態 */
			//if ((bef_btn_status & MAINTENANCEL_BIT) != (btn_status & MAINTENANCEL_BIT)) {
			//	if ((btn_status & MAINTENANCEL_BIT) != 0) {
			//		/* 保守ボタンが押された */
			//		maintenance_flg = 1;
			//		/* 警報ブザーを停止する */
			//		monitor_buzzer_status = 0;/*  警報ブザー状態*/
			//		MonitorBuzzerWrite( 0);/* 監視盤ブザーへの停止出力処理 */
			//		printf("警報ブザーを停止する 1\n");
			//	} else {
			//		/* 保守ボタンが離された */
			//		maintenance_flg = 0;
			//	}
			//}
			if ((btn_status & BUZZER_STOP_BIT) != 0) {
				/* ブザー停止ボタンが押された */
				buzzer_stop_flg = 1;
				printf("警報ブザーを停止する 2\n");
				/* 警報ブザーを停止する */
				monitor_buzzer_status = 0;/*  警報ブザー状態*/
				MonitorBuzzerWrite( 0);/* 監視盤ブザーへの停止出力処理 */
			}
		}
		
		if ((bef_btn_status & CAMMAND_BIT) != (btn_status & CAMMAND_BIT)) {
			/* 運用停止ボタンが変化した */
			if ((btn_status & CAMMAND_BIT) != 0) {
				/* 運用停止ボタンが押された状態 */
				if (monitor_status == 0) {
					/* 運用解除状態なので運用停止要求をセット */
					send_request_flag = 1;/* 運用停止、解除送信要求フラグ */
					monitor_command_flg = 1;/* 運用停止要求 */
					printf("運用停止ボタンが押された状態に変化した 運用停止要求する\n");
				} else if (monitor_status == 1) {
					/* 運用停止状態なので運用停止要求をセット */
					send_request_flag = 1;/* 運用停止、解除送信要求フラグ */
					monitor_command_flg = 2;/* 運用停止解除要求 */
					printf("運用停止ボタンが押された状態に変化した 運用停止解除要求する\n");
				} else {
					/* 応答待ち状態なのでなにもしない */
					printf("運用停止ボタンが押された状態に変化した 応答待ち状態なのでなにもしない\n");
				}
			} else {
				
				/* 運用停止ボタンが離された状態 */
				printf("運用停止ボタンが離された状態に変化した\n");
				send_request_data = 0;/* 運用停止、解除送信要求データ */
			}
			monitor_led_status[UNYOU_TEISHI_LED] = LED_STATUS_ORANGE_TOGGLE;/* 点滅状態にする */
		}
		/* 前回のボタンの状態を保持 */
		bef_btn_status = btn_status;/* 前回のボタンの状態を保持 */
	}
}


/**
 *	@brief 制御機の状態をチェックする処理
 *
 *	@retval なし
 */
static void CheckContStatus(void)
{
	int i;
	int flg;
	int total_count = 0;
	int count = 0;
	int err_status = 0;
	int err_count = 0;
	char str[256];
	
	/* linkage_statusをdiv_noから取り出す */
	param.linkage_status = LinkageUnPack(monitor_command.h.div_no);
	for (i = 0; i < CONTROLER_MAX ; i++) {
		int d = (param.linkage_status >> (4*(CONTROLER_MAX - i - 1)))&0xf;
		if (d != 0) {
			total_count++;
			
			flg = CheckContStatusSub(i , &err_status);/* 各制御機の状態をチェックする処理 */
			if (flg) {
				count++;
			}
			if (err_status) {
				err_count++;
			}
		}
	}
//	printf("count =%d, total_count=%d\n",count, total_count);
	if (count == total_count) {
		/* 一致したということは全ての制御機が運用停止、または停止解除になったということ */
		sprintf(str, "count == total_count %d,%d\n",total_count, monitor_command_flg);
		DebugPrint("", str, 1);
		if (monitor_command_flg == 1) {
			/* 運用停止 全制御機からの応答で運用停止になった */
			if (monitor_led_status[UNYOU_TEISHI_LED] != LED_STATUS_ORANGE) {
				monitor_led_status[UNYOU_TEISHI_LED] = LED_STATUS_ORANGE;
				monitor_status = 1;/* 運用停止状態にする */
			}
		} else {
			/* 運用停止解除 全制御機からの応答で運用停止解除になった */
			if (monitor_led_status[UNYOU_TEISHI_LED] != LED_STATUS_OFF) {
				monitor_led_status[UNYOU_TEISHI_LED] = LED_STATUS_OFF;
				monitor_status = 0;/* 運用停止解除状態にする */
			}
		}
	} else {
		/* 運用停止状態が同じではない状態 */
		if(monitor_status != 2) {
			/*
				運用、運用解除のいずれかの状態のはずなのに
				応答がおかしいのは制御機にリセットでもかかったのか？
			*/
			printf("応答がおかしいのは制御機にリセットでもかかったのか？monitor_status=%d\n", monitor_status);
			send_request_flag = 1;
			if (monitor_status == 0) {
				monitor_command_flg = 2;/* 運用停止解除要求 */
			} else {
				monitor_command_flg = 1;/* 運用停止要求 */
			}
		}
		if (monitor_led_status[UNYOU_TEISHI_LED] != LED_STATUS_ORANGE_TOGGLE) {
			monitor_led_status[UNYOU_TEISHI_LED] = LED_STATUS_ORANGE_TOGGLE;/* 点滅状態にする */
		}
	}
	
	if ((monitor_command.command[CONTA_ADDRESS - 1][1]&2) != 0) {/* 1:保守/0:通常 */
		hoshu_status = 1;
	} else {
		hoshu_status = 0;
	}
	if (err_count != 0) {
		if ((maintenance_flg == 0) && (buzzer_stop_flg == 0) && (hoshu_status == 0)) {
			monitor_buzzer_status = 1;	/* 警報ブザー状態*/
			MonitorBuzzerWrite(1);		/* 警報ブザー */
		}
		monitor_led_status[ALARM_LAMP_LED] = LED_STATUS_ORANGE;/* 20170205 */
	} else {
		buzzer_stop_flg = 0;		/* 20170214 */
		monitor_buzzer_status = 0;	/* 20170214 警報ブザー状態*/
		MonitorBuzzerWrite(0);		/* 20170214 警報ブザー */
		monitor_led_status[ALARM_LAMP_LED] = LED_STATUS_OFF;/* 20170205 */
	}
}

/**
 *	@brief 各制御機の状態をチェックする処理
 *
 *	@retval 運用停止、解除指令との対応フラグ
 */
static int CheckContStatusSub(int i, int * err_status)
{
	int flg = 0;
	if ((monitor_command_flg == 1) && ((monitor_command.command[i][0]&8) != 0)){
		/* 運用停止 端末状態 */
		flg = 1;
	}
	if ((monitor_command_flg == 0) && ((monitor_command.command[i][0]&8) == 0)){
		/* 運用停止解除 端末状態 */
		flg = 1;
	}
	if ((monitor_command_flg == 2) && ((monitor_command.command[i][0]&8) == 0)){
		/* 運用停止解除 端末状態 */
		flg = 1;
	}
	
#ifdef kaji20170208	//	加地　コメントアウト
	if ((
		(monitor_command.command[i][1]&0x20) != 0) /* 回線異常 */
		 || ((monitor_command.command[i][1]&4) != 0) /* 端末機異常 */
		 || (monitor_command.command[i][2] != 0) /* 可変標識盤異常 */
		 || (monitor_command.command[i][3] != 0) /* 発光鋲異常 */
	)
	{
		/* 何らかの異常がある場合は運用モード 消灯*/
		*err_status = 1;
		monitor_led_status[2 * i] = LED_STATUS_OFF;
	} else {
#else
//	if ((
//		(monitor_command.command[i][1]&0x20) != 0) /* 回線異常 */
// //		 || ((monitor_command.command[i][1]&4) != 0) /* 端末機異常 */
// //		 || (monitor_command.command[i][2] != 0) /* 可変標識盤異常 */
// //		 || (monitor_command.command[i][3] != 0) /* 発光鋲異常 */
//		 || ( ((monitor_command.command[i][0]&0x20) != 0)
//		 	&&((monitor_command.command[i][0]&0x07) == 0) )	/* フェイル	*/
//	)
//	{
//		/* 端末が見えない・フェイルの場合は　一掃　 */
//		*err_status = 1;
//		monitor_led_status[2 * i] = LED_STATUS_ORANGE;
//	} else {
/* kaji20170225 */
	if ( (monitor_command.command[i][1]&0x20) != 0) /* 回線異常 */
	{
		*err_status = 1;
		switch(monitor_command.command[i][0]) {
			case 0x01:/* 通常(推定) */
				monitor_led_status[2 * i] = LED_STATUS_GREEN;
				break;
			case 0x02:/* 一掃(推定) */
			case 0x04:/* 変移(推定) */
			case 0x20:/* フェール */
			monitor_led_status[2 * i] = LED_STATUS_ORANGE;
			break;
		default:
			monitor_led_status[2 * i] = LED_STATUS_OFF;/* 何かわからない状態 */
			break;
		}
	} else {
#endif
		*err_status = 0;
		if ((monitor_command.command[i][0]&1) != 0) {
			/* 通常 運用モード 緑点灯*/
			//printf("通常 運用モード 緑点灯 %d\n",i);
			monitor_led_status[2 * i] = LED_STATUS_GREEN;
		} else if ((monitor_command.command[i][0]&2) != 0) {
			/* 一掃 運用モード 橙点灯*/
			monitor_led_status[2 * i] = LED_STATUS_ORANGE;
		} else if ((monitor_command.command[i][0]&4) != 0) {
			/* 変移 運用モード 赤点灯*/
			monitor_led_status[2 * i] = LED_STATUS_RED;
		} else {
			/* 多分フェイル */
			/* 一掃 運用モード 橙点灯*/
			monitor_led_status[2 * i] = LED_STATUS_ORANGE;
		}
	}

	if (((monitor_command.command[i][1]&0x20) != 0) || (monitor_timeout_flag == 1)) {
		/* 回線異常 橙点滅*/
		//printf("回線異常 橙点滅 %d\n",2 * i + 1);
		monitor_led_status[2 * i + 1] = LED_STATUS_ORANGE_TOGGLE;
	}
	else if ((
		(monitor_command.command[i][1]&4) != 0) /* 端末機異常 */
		 || (monitor_command.command[i][2] != 0) /* 可変標識盤異常 */
		 || (monitor_command.command[i][3] != 0) /* 発光鋲異常 */
	)
	{
		*err_status = 1;
		/* 異常 赤点滅*/
		monitor_led_status[2 * i + 1] = LED_STATUS_RED_TOGGLE;
		if (maintenance_flg == 0) {
			/* 警報ブザーをONする */
			//int d = *(char *) SWITCH_REG;
			//d |= ALARM_BUZZER_BIT;
			//*(char *) SWITCH_REG = d;
		}
	} else if ((monitor_command.command[i][0]&0x40) != 0) {
		/* 手動 端末状態 緑点滅*/
		monitor_led_status[2 * i + 1] = LED_STATUS_GREEN_TOGGLE;
	} else {
		/* 正常 */
#ifdef	kaji20170209
		monitor_led_status[2 * i + 1] = LED_STATUS_OFF;
#else
		monitor_led_status[2 * i + 1] = LED_STATUS_GREEN;
#endif
	}
	return flg;
}

/**
 *	@brief 制御機から監視盤へのデータ受信処理
 *
 *	@retval なし
 */
static void RcvMonitorSrv(void) {
	char str[256];
	unsigned char monitor_rcv_buf[RCV_BUFF_SIZE];

	
	while(!empty(MONITOR_QUEUE)) {
		unsigned char d = peek(MONITOR_QUEUE, 0);/* 先頭データは０か？ */
		if ( d != 0) {
			dequeue(MONITOR_QUEUE);
			continue;
		}
		if (lenqueue(MONITOR_QUEUE) >= 10) {
			int monitor_wait_count = peek(MONITOR_QUEUE, 9) + 10;
			if (monitor_wait_count != (10+8+6*8)){
				dequeue(MONITOR_QUEUE);
			} else {
				//printf("%d\n",lenqueue(MONITOR_QUEUE));
				if (lenqueue(MONITOR_QUEUE) >= monitor_wait_count) {
				//printf("X");
					/* データパケット受信 */
					int i;
					for(i = 0 ; i < monitor_wait_count; i++) {
						monitor_rcv_buf[i] = dequeue(MONITOR_QUEUE);/* 受信データ取り出し */
					}
					CONTROL_HEADER *h =(CONTROL_HEADER *)monitor_rcv_buf;
					if (h->dst == 1) {/* 制御機Aから監視盤への制御指令 */
						DebugPrint("","RcvMonitorSrv 監視盤への制御指令受信", 1);
						memmove(&monitor_command, monitor_rcv_buf, sizeof(MONITOR_COMMAND));
						MonitorCommandSrv();/* 監視盤への制御指令受信処理 */
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
 *	@brief 監視盤への制御指令受信処理
 *
 *	@retval なし
 */
static void MonitorCommandSrv(void)
{
	
	MonitorCommandDisp();/* 監視盤への制御指令表示処理 */
//	inhibit_timer = 0;/* 送信禁止用タイマ */
//	inhibit_flag = 0;/* 送信禁止時は１となる */
	check_monitor_timer = 0;
	monitor_timeout_flag = 0;
	if (monitor_command_rcv_count == 0) {
		/* 時刻をセットする */
		SetTime(&monitor_command.t);/* 時刻設定処理 */
		SaveRTC(&monitor_command.t);/* 不揮発用RTCに書き込む */
	}
	int setdata = 0;
	monitor_command_rcv_count++;
	if (send_request_flag == 1) {
		send_request_flag = 0;
		setdata = monitor_command_flg;
		monitor_status = 2;/* 受信ステータス(運用停止解除:0,運用停止:1,応答確定待ち状態:2) */
	}
	
	SendOperateCommand(setdata);/* ヘルスを送信 制御機への運用指令の送信処理 */
}

/**
 *	@brief 監視盤への制御指令表示処理
 *
 *	@retval なし
 */
void MonitorCommandDisp(void)
{
	int i,j;
	char str[256];
	char str2[256];
	
	for(i = 0; i < 8; i++){
		sprintf(str2,"MontorCommandSrv %2d:",i+1);
		for(j = 0; j < 6; j++){
			sprintf(str,"%02X ",monitor_command.command[i][j]);
			strcat(str2,str);
		}
		if ((monitor_command.command[i][1]&0x20) != 0) {
			strcpy(str,"回線異常 ");
			strcat(str2,str);
		}
		if ((monitor_command.command[i][1]&4) != 0) {
			strcpy(str,"端末機異常 ");
			strcat(str2,str);
		}
		if (monitor_command.command[i][2] != 0) {
			strcpy(str,"標識盤異常 ");
			strcat(str2,str);
		}
		if (monitor_command.command[i][3] != 0) {
			strcpy(str,"発光鋲異常 ");
			strcat(str2,str);
		}
		if (monitor_command.command[i][4] != 0) {
			strcpy(str,"時計異常 ");
			strcat(str2,str);
		}
		if ((monitor_command.command[i][0]&8) != 0) {
			strcpy(str,"運用停止 ");
			strcat(str2,str);
		}
		if ((monitor_command.command[i][0]&0x40) != 0) {
			strcpy(str,"手動 ");
			strcat(str2,str);
		}
		if ((monitor_command.command[i][0]&1) != 0) {
			strcpy(str,"通常 ");
			strcat(str2,str);
		} else if ((monitor_command.command[i][0]&2) != 0) {
			strcpy(str,"一掃 ");
			strcat(str2,str);
		} else if ((monitor_command.command[i][0]&4) != 0) {
			strcpy(str,"変移 ");
			strcat(str2,str);
		}
		DebugPrint("", str2, 8);
	}
}

/**
 *	@brief ステータスの表示処理
 *
 *	@retval なし
 */
void MonitorStatusDisp(void)/* ステータスの表示処理 */
{
	//printf("monitor_request_status = %d 送信要求の種類(運用停止解除:0,運用停止:1)\n",monitor_request_status);
	printf("monitor_status         = %d 受信ステータス(運用停止解除:0,運用停止:1,応答確定待ち状態:2)\n",monitor_status);
	printf("monitor_buzzer_status  = %d 警報ブザー状態(押されていない:0,押されている:1)\n",monitor_buzzer_status);
	printf("maintenance_flg        = %d 保守ボタン(押されていない:0,押されている:1)\n",maintenance_flg);
	printf("hoshu_status           = %d 制御器Aの保守ボタン(押されていない:0,押されている:1)\n",hoshu_status);
	printf("rcv_count              = %d 制御指令コマンド受信回数\n",monitor_command_rcv_count);
	printf("snd_count              = %d(%d,%d,%d) 制御指令コマンド送信回数\n",snd_count,each_snd_count[0],each_snd_count[1],each_snd_count[2]);
}

/**
 *	@brief 制御機への運用指令の送信処理
 *
 * 運用停止スイッチを押された状態では、ランプを点灯し、
 * 制御機Ａに運用停止コマンドを送り、”通常”を維持するとのことです。
 * 再び、スイッチがが押され、解除されるまで、停止を維持することになります。
 *
 *	@retval なし
 */
static void SendOperateCommand(int flag) {
	char str[256];
	
	snd_count++;
	each_snd_count[flag]++;
	SetNowTime(&monitor_operate_command.t);/* 時刻セット */
	monitor_operate_command.command = flag;
//	monitor_send_operate_stage_no = CS_ON_STAGE;
	monitor_send_operate_stage_no = PRE_DELAY_SET_STAGE;/* kaji20170310 */
	sprintf(str, "制御機への運用指令の送信 command = %02X",monitor_operate_command.command);
	DebugPrint("", str, 2);
}

/**
 *	@brief 制御機への運用指令の実際の送信処理
 *
 *	@retval なし
 */
static void SendOperateCommandSrv(void)
{
	switch (monitor_send_operate_stage_no) {
	case SEND_MONITOR_IDLE_STAGE:/* アイドル */
		break;
	case PRE_DELAY_SET_STAGE:/* MDM_CS出力待ち kaji20170310 */
		check_delay_timer = 0;/* 待ちタイマ */
		monitor_send_operate_stage_no = PRE_DELAY_STAGE;
		break;
	case PRE_DELAY_STAGE:/* MDM_CS出力待ち kaji20170310 */
		if (check_delay_timer > param.mdmcs_delay_time) {/* 待ちタイマ */
			monitor_send_operate_stage_no = CS_ON_STAGE;
		}
		break;
	case CS_ON_STAGE:/* MDM_CSの出力処理 */
		MdmcsWrite(1);
		check_delay_timer = 0;/* 待ちタイマ */
		monitor_send_operate_stage_no = SEND_WAIT_STAGE1;
		DebugPrint("SendOperateCommandSrv", "CS_ON MdmcsWrite(1)", 4);
		break;
	case SEND_WAIT_STAGE1:/* MDM_CSの出力処理 */
		if (check_delay_timer > param.mdmcs_delay_time) {/* 待ちタイマ */
			monitor_send_operate_stage_no = SEND_MONITOR_STAGE;
		}
		break;
	case SEND_MONITOR_STAGE:/* の出力処理 */
		DebugPrint("SendOperateCommandSrv", "送信開始", 4);
//		send_com3_p = 0;
		SendCom3(hComm1, (unsigned char *)&monitor_operate_command, sizeof(MONITOR_OPERATE_COMMAND));
		monitor_send_operate_stage_no = WAITING_SEND_END_STAGE;
		break;
	case WAITING_SEND_END_STAGE://送信終了待ち
//		if (send_com3_p != sizeof(MONITOR_OPERATE_COMMAND)) {
//			SendCom3(hComm1, (unsigned char *)&monitor_operate_command, sizeof(MONITOR_OPERATE_COMMAND));
//		} else {
		if (ChkSendCom3() == 0){/* kaji20170310 */
			/*  送信終了*/
			check_delay_timer = 0;/* 待ちタイマ */
			monitor_send_operate_stage_no = SEND_WAIT_STAGE2;
			DebugPrint("SendOperateCommandSrv", "送信終了", 4);
		}
		break;
	case SEND_WAIT_STAGE2:/* MDM_CSの出力処理 */
		if (check_delay_timer > param.mdmcs_delay_time) {/* 待ちタイマ */
			MdmcsWrite(0);
			inhibit_timer = 0;/* 送信禁止用タイマ */
			inhibit_flag = 0;/* 送信禁止時は１となる */
			monitor_send_operate_stage_no = SEND_MONITOR_IDLE_STAGE;
		DebugPrint("SendOperateCommandSrv", "CS_OFF MdmcsWrite(0)", 4);
		}
		break;
	default:
		printf("SendOperateCommandSrv default error\n");
		break;
	}

}
/**
 *	@brief 監視盤のLED制御処理
 *
 *	@retval なし
 */
static void MonitorLEDSrv(void)
{
	int i;
	char str[100];
	
	for( i = 0 ; i < MONITOR_LED_MAX_COUNT; i++) {
		if (monitor_bef_led_status[i] != monitor_led_status[i]) {
			//printf("MonitorLEDSrv\n");
			switch (monitor_led_status[i]) {
			case LED_STATUS_OFF:
				sprintf(str, "monitor(%d) LED_STATUS_OFF",i);
				/* 消灯を出力 */
				MonitorLedOut(i,LED_STATUS_OFF );
				break;
			case LED_STATUS_GREEN:
				sprintf(str, "monitor(%d) LED_STATUS_GREEN",i);
				/* 緑点灯を出力 */
				MonitorLedOut(i,LED_STATUS_GREEN );
				break;
			case LED_STATUS_ORANGE:
				sprintf(str, "monitor(%d) LED_STATUS_ORANGE",i);
				/* 橙点灯を出力 */
				MonitorLedOut(i,LED_STATUS_ORANGE );
				break;
			case LED_STATUS_RED:
				sprintf(str, "monitor(%d) LED_STATUS_RED",i);
				/* 赤点灯を出力 */
				MonitorLedOut(i,LED_STATUS_RED );
				break;
			case LED_STATUS_GREEN_TOGGLE:
				sprintf(str, "monitor(%d) LED_STATUS_GREEN_TOGGLE",i);
				monitor_led_toggle_status = LED_OFF;
				check_monitor_led_ms_timer = 0;
				break;
			case LED_STATUS_ORANGE_TOGGLE:
				sprintf(str, "monitor(%d) LED_STATUS_ORANGE_TOGGLE",i);
				monitor_led_toggle_status = LED_OFF;
				check_monitor_led_ms_timer = 0;
				break;
			case LED_STATUS_RED_TOGGLE:
				sprintf(str, "monitor(%d) LED_STATUS_RED_TOGGLE",i);
				monitor_led_toggle_status = LED_OFF;
				check_monitor_led_ms_timer = 0;
				break;
			default :
				break;
			}
			DebugPrint("", str, 2);
			monitor_bef_led_status[i] = monitor_led_status[i];
		}
	}
	/* 点滅動作の切り替え処理 */
	if (check_monitor_led_ms_timer >= monitor_led_toggle_count ) {
		for( i = 0 ; i < MONITOR_LED_MAX_COUNT; i++) {
			switch(monitor_led_status[i]) {
			case LED_STATUS_GREEN_TOGGLE:
				if (monitor_led_toggle_status == LED_OFF) {
					sprintf(str, "Monitor GREEN_LED_ON  %d",check_monitor_led_ms_timer);
					/* 緑点灯を出力 */
					MonitorLedOut(i,LED_STATUS_GREEN );
				} else {
					sprintf(str, "Monitor GREEN_LED_OFF %d",check_monitor_led_ms_timer);
					/* 消灯を出力 */
					MonitorLedOut(i,LED_STATUS_OFF );
				}
				//DebugPrint("", str, 2);
				break;
			case LED_STATUS_ORANGE_TOGGLE:
				if (monitor_led_toggle_status == LED_OFF) {
					sprintf(str, "Monitor ORANGE_LED_ON  %d",check_monitor_led_ms_timer);
					/* 橙点灯を出力 */
					MonitorLedOut(i,LED_STATUS_ORANGE );
				} else {
					sprintf(str, "Monitor ORANGE_LED_OFF %d",check_monitor_led_ms_timer);
					/* 消灯を出力 */
					MonitorLedOut(i,LED_STATUS_OFF );
				}
				//DebugPrint("", str, 2);
				break;
			case LED_STATUS_RED_TOGGLE:
				if (monitor_led_toggle_status == LED_OFF) {
					sprintf(str, "Monitor RED_LED_ON  %d",check_monitor_led_ms_timer);
					/* 赤点灯を出力 */
					MonitorLedOut(i,LED_STATUS_RED );
				} else {
					sprintf(str, "Monitor RED_LED_OFF %d",check_monitor_led_ms_timer);
					/* 消灯を出力 */
					MonitorLedOut(i,LED_STATUS_OFF );
				}
				//DebugPrint("", str, 2);
				break;
			default :
				break;
			}
		}
		if (monitor_led_toggle_status == LED_OFF) {
			monitor_led_toggle_status = LED_ON;
		} else {
			monitor_led_toggle_status = LED_OFF;
		}
		check_monitor_led_ms_timer = 0;
	}
	MonitorLedWrite();/* LEDに出力する */
}



