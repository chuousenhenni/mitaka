/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	cont.c
 *	概要
 *  制御機共通部
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
#include "conta.h"
#include "contb.h"
#include "cont.h"
#include "io.h"

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

static int cont_ms_timer;/* msタイマ */
static int cont_sec_timer;/* 秒タイマ */

int sw_test_flg;/* テスト用に使用  */
int sw_test_data;/* テスト用に使用  */
int sw;/* SW状態を保持 */
static int bef_sw;/* 前回のSW状態を保持 */
static int teiden_hukki_flg;/* 停電復帰フラグ */

int board_rotate_stage_no;/* 可変表示板制御の状態関数 */

static int cont_bef_led_status[ CONT_LED_MAX_COUNT ];/* 前回のLED状態 */
int cont_led_status[ CONT_LED_MAX_COUNT ];/* LED状態 */
static int cont_led_toggle_status;/* LED 点滅モードので現在の状態 */
static int cont_led_toggle_count;/* LED 点滅モードを切り替えるまでの回数 */

static int check_cont_led_ms_timer;/* LED制御用の1ms毎に更新するタイマ */

static int date_change_flag;/* 日付変更フラグ */
static int offset_time_up_value;/* タイムアップチェック用 */
static int now_time;/* 現在時刻を保持 */

static uint32_t output_data[DISPLAY_BOARD_MAX];/* 各可変表示板への出力データ */
uint32_t input_data[DISPLAY_BOARD_MAX];/* 各可変表示板からの入力データ */
uint32_t board_status[DISPLAY_BOARD_MAX];/* 各可変表示板の正常異常状態 */

static int bef_io_power_outage_flag;/* 停電発生フラグ */
int io_power_outage_flag;/* 停電発生フラグ */
int power_off_bef_status;/* 停電時のステータス */

static int io_control_timer;/* IO制御用のタイマー */
static int board_rotate_timer;/* 表示板制御用のタイマー */
int schedule_timer;/* スケジュール要求用のタイマー */
static int lap_count_timer;/* 経過時間測定用のタイマー */
static int lap_time_max;/* 経過時間最大値 */
static int board_rotate_total_count;/* 表示板回転処理全回数 */
static int board_status_error_check_flg;/* 可変表示板エラー判定要求フラグ */
int err_recover_flg;		/* 異常復帰ボタンON状態 */
int keep_byou_status_flg;	/* 発光鋲異常を保持するフラグ　　　1:異常 0:通常 */
int keep_board_status_flg;	/* 可変表示板異常を保持するフラグ　1:異常 0:通常 kaji20170307 */
int board_choukou_status;	/* 一斉調光指令の値を保持する　1:点灯 0:滅灯 kaji20170330 */
int rtc_sync_timer;/* RTCに同期するmsタイマ */

PARAM param;/* 設定値 */

STATUS now_status;/* 現在の設定値 */

int first_p22;/* 最初のオフセットタイマは特殊 */

/* 一掃時の設定値を保持 */
/* P2パターンがP1,P3のどちらと同じかを設定 */
static int p2_status[CONTROLER_MAX][DISPLAY_BOARD_MAX] =
{
	{P1,P1,P1,P1,P3,P3,P3,-1},/* 端末制御機１ 三鷹通り北行、南行 灯火標識有り２上連雀2-5-12 下連雀3-41-12 */
	{P3,P3,-1,P1,P1,P1,P1,P1},/* 端末制御機２ 三鷹通り南行 下連雀3-41-11 */
	{P1,-1,P1,P1,P1,P1,P1,P1},/* 端末制御機３ 市道 下連雀3-40-16 */
	{P1,-1,-1,P1,P1,P1,P1,P1},/* 端末制御機４ 三鷹通り北行、南行 灯火標識有り２ 上連雀4-1-2,下連雀3-40-10 */
	{P1,P1,P1,P3,P3,-1,P1,P1},/* 端末制御機５ 三鷹通り北行、南行 上連雀4-6,下連雀4-20-24 */
	{P1,P1,-1,P1,P1,P1,P1,P1},/* 端末制御機６ 三鷹通り北行 上連雀6-6-1 */
	{P1,P1,P1,P2,P2,-1,P1,P1},/* 端末制御機７ 三鷹通り北行、南行 上連雀4-8-5,下連雀4-18-23 */
	{P1,-1,P1,P1,P1,P1,P1,P1} /* 端末制御機８ 三鷹通り北行 上連雀6-11-4 */
};
/* 無地の表示板かどうかを設定 */
/* 無地の表示板は蛍光灯をつけない */
/* 無地 1 */
static int muji[CONTROLER_MAX][DISPLAY_BOARD_MAX][3] =
{
	{{0,0,0},{0,0,1},{0,0,0},{0,0,0},{1,0,0},{0,0,0},{0,1,1}},	/* 端末制御機１*/
//	{{0,0,0},{0,0,1},{0,0,0},{0,0,0},{1,0,0},{0,1,1},{0,0,0}},	/* 端末制御機１(kaji20170221折角直してもらいましたのに…)*/
	{{0,0,0},{0,0,0}},											/* 端末制御機２ */
	{{0,0,0}},													/* 端末制御機３ */
	{{0,0,0}},													/* 端末制御機４ */
	{{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,1,1}},					/* 端末制御機５ */
	{{0,0,0},{0,0,0}},											/* 端末制御機６ */
	{{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,1,1}},					/* 端末制御機７ */
	{{0,0,0}} 													/* 端末制御機８ */
};
/* 起点案内の表示板かどうかを設定 */
static int kiten_annai[CONTROLER_MAX][DISPLAY_BOARD_MAX] =
{
	{0,0,0,0,0,0,0},											/* 端末制御機１ */
	{0,0},														/* 端末制御機２ */
	{1},														/* 端末制御機３ */
	{0},														/* 端末制御機４ */
	{0,0,0,0,0},												/* 端末制御機５ */
	{0,0},														/* 端末制御機６ */
	{0,0,0,0,0},												/* 端末制御機７ */
	{1} 														/* 端末制御機８ */
};
/* 発光鋲が有るか無いかを設定 */
static int hakkobyou_exist[CONTROLER_MAX][2] =
{
	{1,1},														/* 端末制御機１ */
	{0,0},														/* 端末制御機２ */
	{0,0},														/* 端末制御機３ */
	{1,1},														/* 端末制御機４ */
	{1,1},														/* 端末制御機５ */
	{0,0},														/* 端末制御機６ */
	{1,1},														/* 端末制御機７ */
	{0,0} 														/* 端末制御機８ */
};
static IO_INFO io_info[CONTROLER_MAX];/* 各制御機のIO状態管理用 */
IO_INFO my_io_info;/* 自制御機のIO状態管理用 */
int display_board_count;/* 自制御機の表示板枚数 */
static RESPONSE_COMMAND *response = NULL;/* 監視応答コマンド */
int startup_error_status;/* 起動時のエラーの有無を保持(0:エラー無し、1:エラー有り) */

static char request_pattern [][100] = {/* デバッグ用の表示パターン */
	"P2_P2 オフセットタイマ待ち",
	"PX_P1 通常へboardを移動",/* 通常へ */
	"PX_P2 一掃へboardを移動",/* 一掃へ */
	"PX_P3 変移へboardを移動",/* 変移へ */
	"PX_FAIL フェイルへboardを移動",
	"PX_POWEROFF パワーオフの位置へboardを移動",
	"PX_POWER_RECOVER 復帰位置へboardを移動"
};

/*
 *===========================================================================================
 *					外部変数定義
 *===========================================================================================
 */

extern int my_tanmatsu_type;/* 端末タイプ 1:A,2:B... */
extern int remote_command_request;/* 遠隔時の表示板変更要求ありフラグ */
extern int remote_command_command;/* 遠隔時の表示板変更コマンド */
extern int swtest;
extern int response_received_count[CONTROLER_MAX];/* 制御機Ｂからの受信回数を保持する */
extern int response_time[CONTROLER_MAX];/* 制御器Aからの要求から制御器Bからの応答受信までの時間(ms) */
extern int response_total_err_count[CONTROLER_MAX];/* kaji20170305 制御機Ｂからの総エラー回数を保持する */
//extern int fail_flag;/* 20170224 フェイル送信時1となる */
extern int fail_hnd_state;/* kaji20170225 フェイル管理の状態変数 */
extern int CheckTanmatuError(void) ;

/*
 *===========================================================================================
 *					内部	関数定義
 *===========================================================================================
 */

static void ContLEDInit(void);
static void ContLEDSrv(void);/* 制御機のLED制御処理 */

static void ActionRemoteSrv(void);/* 遠隔時のIO動作処理 */
static void ScheduleRequestChkSrv(void);/* スケジュール更新要求動作判定処理 */
static void BoardRotateSrv(void);/* 可変表示板制御処理 */
static int GetWaitingPattern(void);/* 移動すべきパターンを求める処理 */
static void BoardStatusErrorCheck(void);/* 可変表示板エラー判定処理 */
static void IO_ChangeSrv(void );/* 表示板,鋲等を変化させる処理 */

static void SWSrv(STATUS *status);/* SW状態読み込み処理 */
static void ActionSrv(void);/* 各種動作処理 */
static void PowerOutMainSrv(void);/* 停電発生時の処理 */
static void BSPLedSrv(void);/* BSPLEDの表示処理 */

static int CheckBoardStatus(int dst);/* 正常な可変表示板の状態を獲得する処理 */
static int CheckInput(int dst, int display_check_count, int out_put_req);/* IO入力データ判定処理 */
static void  GetAlowedPattern(int dst, int no, int *allowed_pattern);/* 目標位置に対しての許されるパターンを求める処理 */
static void Output(int d, int board_count);/* IO出力処理 */

static void SetRequest(BROADCAST_COMMAND *com, char *str);/* モード設定要求時の処理 */
static void SetNowLEDStatus(int status);/* ステータスをLEDに反映する処理 */
static void SetNowLEDStatusToggle(int status);/* ステータスをLED（点滅状態）に反映する処理 */

static void SetByouStatus(int sw);/* 発光鋲ステータスをセットする処理 */

static int GetStartupBoardPattern(void);/* (停電起動時に)可変表示板の多数決を取る処理 */

/*
 *===========================================================================================
 *					グローバル関数定義
 *===========================================================================================
 */

int GetNowRemoteStatus(void);/* 現在の遠隔時のステータスを返す処理 */
void SetNowStatus(int status);/* ステータスのセーブ処理 */
void IO_ChangeReq(char chane_type);/* 表示板,鋲等を変化させる処理 */

void ControlerInit(void );
void SetMode(STATUS *status, RESPONSE_COMMAND *response);/* 現在のモードをセット */
void SetResponseStatus(RESPONSE_COMMAND *response);/* IOステータスをセット */

void SetDefault(void);/* ディフォルトパラメータ設定 */
int CheckErrorStatus(void);/* 現在のエラー状態をnow_statusから判定する処理 */

void TimerIntCont(int count);/* 制御機共通のタイマ割り込み処理 */

void EventRequest(int event);/* 状態変更要求処理 */

void ContSrv(RESPONSE_COMMAND *response);/* 制御機共通の処理 */
void StatusDisp(void);/* ステータスの表示処理 */

void BroadcastCommand(BROADCAST_COMMAND *com);/* 同報指令受信処理 */

void SaveParam(void);/* パラメータのセーブ処理 */
int LoadStatus(void);/* ステータスのロード処理 */
void SaveStatus(void);/* ステータスのセーブ処理 */
void LoadParam(void);/* パラメータのロード処理 */

int CheckMuji(int no);/* 標識板が無地かどうかの判定処理 */

int deb(int code);

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
 *	@brief 制御機共通のタイマ割り込み処理 (count [ms]毎)
 *
 *	@retval なし
 */
void TimerIntCont(int count)
{
	cont_ms_timer += count;
	check_cont_led_ms_timer += count;
	io_control_timer += count;/* IO制御用のタイマー */
	board_rotate_timer += count;/* 表示板制御用のタイマー */
	schedule_timer += count;/* スケジュール要求用のタイマー */
	lap_count_timer += count;/* 経過時間測定用のタイマー */
	rtc_sync_timer += count;/* RTCに同期するmsタイマ */
	if (rtc_sync_timer >= 999) {
		rtc_sync_timer = 999;
	}
	if (cont_ms_timer >= 1000) {
		cont_ms_timer = 0;
		cont_sec_timer++;
		if ((cont_sec_timer%5) == 0) {
			board_status_error_check_flg = 1;/* 可変表示板エラー判定要求フラグセット */
		}
		if (offset_time_up_value != 0) {
			offset_time_up_value--;
		}
	}
}
//int aaa=0;
void ClearSyncTimer(void)
{
	//aaa++;
	//if(aaa>10) {
	//	aaa=0;
	//	printf("ClearSyncTimer %X %X\n",rtc_sync_timer,&rtc_sync_timer);
	//}
	rtc_sync_timer = 0;
}

/**
 *	@brief 制御機共通の初期化処理
 *
 *	@retval なし
 */
void ControlerInit(void )
{
	int i;
	int j;
	int st;
	int io_power_outage_flag;
	
	for ( i = 0 ; i< CONTROLER_MAX; i++) {
		io_info[i].display_board_count = 0;
		for ( j = 0 ; j< DISPLAY_BOARD_MAX; j++) {
			if (p2_status[i][j] == -1) {
				break;
			} else {
				if (p2_status[i][j] == P1) {
					/* P1 */
					io_info[i].allowed_pattern_p1[j][0] = P1;/* P1 または P2 */
					io_info[i].allowed_pattern_p1[j][1] = P2;
					io_info[i].allowed_pattern_p2[j][0] = P2;/* P2 または P1 */
					io_info[i].allowed_pattern_p2[j][1] = P1;
					io_info[i].allowed_pattern_p3[j][0] = P3;/* P3だけ */
					io_info[i].allowed_pattern_p3[j][1] = P3;
				} else {
					/* P3 */
					io_info[i].allowed_pattern_p1[j][0] = P1;/* P1だけ */
					io_info[i].allowed_pattern_p1[j][1] = P1;
					io_info[i].allowed_pattern_p2[j][0] = P2;/* P2 または P3 */
					io_info[i].allowed_pattern_p2[j][1] = P3;
					io_info[i].allowed_pattern_p3[j][0] = P3;/* P3 または P2 */
					io_info[i].allowed_pattern_p3[j][1] = P2;
				}
				io_info[i].kiten_annai[j] = kiten_annai[i][j];/* 起点案内標識かどうか情報 */
				io_info[i].muji_pattern_p1[j] = muji[i][j][0];/* P1が無地かどうか */
				io_info[i].muji_pattern_p2[j] = muji[i][j][1];/* P2が無地かどうか */
				io_info[i].muji_pattern_p3[j] = muji[i][j][2];/* P3が無地かどうか */
				io_info[i].display_board_count++;
			}
		}
	}
	if ((my_tanmatsu_type>=1) && (my_tanmatsu_type<=8)){
		memmove(&my_io_info, &io_info[my_tanmatsu_type-1], sizeof(IO_INFO));
		display_board_count = my_io_info.display_board_count;
		printf("可変標識板枚数=%d\n",display_board_count);
		for ( j = 0 ; j < display_board_count; j++) {
			printf("No.%d p1(%d,%d)mj=%d,",j+1
				,my_io_info.allowed_pattern_p1[j][0]
				,my_io_info.allowed_pattern_p1[j][1]
				,my_io_info.muji_pattern_p1[j]
				);
			printf("p2(%d,%d)mj=%d,"
				,my_io_info.allowed_pattern_p2[j][0]
				,my_io_info.allowed_pattern_p2[j][1]
				,my_io_info.muji_pattern_p2[j]
				);
			printf("p3(%d,%d)mj=%d"
				,my_io_info.allowed_pattern_p3[j][0]
				,my_io_info.allowed_pattern_p3[j][1]
				,my_io_info.muji_pattern_p3[j]
				);
			if (my_io_info.kiten_annai[j] == 1)  printf(" 起点案内標識");
			printf("\n");
		}
		printf("発光鋲１%s,発光鋲２%s\n"
			,hakkobyou_exist[my_tanmatsu_type-1][0] ? "有り" : "無し"
			,hakkobyou_exist[my_tanmatsu_type-1][1] ? "有り" : "無し"
		);

	} else {
		display_board_count = 10;
	}

	for ( i = 0 ; i< DISPLAY_BOARD_MAX; i++) {
		output_data[ i ] = 0;/* 各可変表示板への出力データ */
		input_data[ i ] = 0;/* 各可変表示板からの入力データ */
		board_status[ i ] = 0;/* 各可変表示板の正常異常状態 */
	}
	sw = 0;
	bef_sw = 0;
	teiden_hukki_flg = 0;/* 停電復帰フラグ */
	date_change_flag = 0;/* 日付変更フラグ */
	io_control_timer = 0;/* IO制御用のタイマー */
	board_rotate_timer = 0;/* 表示板制御用のタイマー */
	schedule_timer = 0;/* スケジュール要求用のタイマー */
	lap_count_timer = 0;/* 経過時間測定用のタイマー */
	lap_time_max = 0;/* 経過時間最大値 */
	board_rotate_total_count = 0;/* 表示板回転処理全回数 */
	board_status_error_check_flg = 0;/* 可変表示板エラー判定要求フラグ */
	err_recover_flg = 0;/* 異常復帰ボタンON状態 */
	keep_byou_status_flg = 0;/* 発光鋲異常を保持するフラグ */
	keep_board_status_flg = 0;/* 可変表示板異常を保持するフラグ kaji20170307 */
	board_choukou_status = 0;/* 一斉調光指令の値を保持する kaji20170330 */
	rtc_sync_timer = 0;/* RTCに同期するmsタイマ */
	first_p22 = 0;/* 最初のオフセットタイマは特殊 */

	SetDefault();/* ディフォルトパラメータ設定 */
	ContLEDInit();/* LED初期化 */
	LoadParam();/* パラメータのロード処理 */
	param.reset_count++;
//	SaveParam();プログラム書き込み後の起動では読見込みに失敗するのでやめる
	LoadStatus();/* ステータスのロード処理 */
	if (now_status.power_outage_flag != 0) {
		/* 停電からの復帰 */
		DebugPrint("","停電からの復帰", 1);
		now_status.power_outage_flag = 0;
		SaveStatus();/* ステータスのセーブ処理 */
		teiden_hukki_flg = 1;/* 停電復帰フラグ */
	}
	bef_io_power_outage_flag = 0;
	io_power_outage_flag = 0;
	SaveStatus();/* ステータスのセーブ処理 */
	board_rotate_stage_no = BOARD_ROTATE_IDLE;
	
	//now_status.start_time=5*60*60;
	//now_status.end_time=18*60*60;
	if (sw_test_flg != 0) {
		printf("sw_test_data=%X\n",sw_test_data);
		swtest = sw_test_data;
	}
	sw = SWRead();/* SW読み込み処理 */
	
	/* 今の状態のパターンをセットしておく */
	int pattern = GetWaitingPattern();
	for( i = 0; i < display_board_count; i++) {
		input_data[i] = pattern;
	}
#if 0 //20170209
	if (now_status.mode == MODE_REMOTE) {
		if ((sw & SW_STATUS_BIT) == 1) {
		/* （３）	停電前が遠隔で、手動にて立ち上がった場合は項目の変更を行わずに現状維持とする */
			printf("（３）	停電前が遠隔で、手動にて立ち上がった場合は項目の変更を行わずに現状維持とする\n");
			now_status.mode = MODE_MANUAL;
		} else if (now_status.status == STATUS_P1){
			printf("***p1\n");
			IO_ChangeReq(PX_P1);/* この位置に移動する */
		} else if (now_status.status == STATUS_P2){
			printf("***p2\n");
			IO_ChangeReq(PX_P2);/* この位置に移動する */
		} else if (now_status.status == STATUS_P3){
			printf("***p3\n");
			IO_ChangeReq(PX_P3);/* この位置に移動する */
		} else {
			printf("***p4\n");
			IO_ChangeReq(PX_P2);/* この位置に移動する */
		}
	} else {
#if 1 //yamazaki この機能は反射標識板　将来用に記載されていたが、やめておく
		if (param.same_nomove_flag == 0) { /* 同じ場合は可変表示版を変化させないモード */
//			IO_ChangeReq(PX_P3);/* 別の位置に移動してから */
			IO_ChangeReq(PX_P1);/* この位置に移動する */
		}
#endif
	}
#endif
	if ((sw & SW_STATUS_BIT) == 1) {
		/* 手動の動作 */
		now_status.mode = MODE_MANUAL;
		IO_ChangeReq(PX_P2);/* この位置（一掃）に移動する */
		remote_command_request = 1;/* 20170226 遠隔時の表示板変更要求ありフラグ */
		remote_command_command = 2;/* 20170226 遠隔時の表示板一掃への変更コマンド */
	} else {
		/* 遠隔の動作 */
		now_status.mode = MODE_REMOTE;
		remote_command_request = 0;/* 遠隔時の表示板変更要求フラグ 0:要求無し*/
#if 1 //20170225
		if (my_tanmatsu_type == CONTA_ADDRESS){
			//if(teiden_hukki_flg == 1) {
			//if ((teiden_hukki_flg == 1) && (now_status.power_outage_flag2 == 0)){
			io_power_outage_flag = PowerStatusRead();/* 停電ステータス(UPS-ALM)読み込み処理 */
//printf("******teiden_hukki_flg=%d,io_power_outage_flag=%d\n",teiden_hukki_flg,io_power_outage_flag);
			if (io_power_outage_flag == 1){
				//st = GetNowRemoteStatus();
//				st = now_status.status;/* 20170226 停電前の状態がロードされている */
				st = GetStartupBoardPattern();/* 20170301 表示板の現在位置(多数)を読み取る */
//				if (st == STATUS_P1) {
				if (st == P1) {/* P1の場合だけP1（通常）に移動する */
					IO_ChangeReq(PX_P1);/* この位置（通常）に移動する */
				} else {
					IO_ChangeReq(PX_P2);/* この位置（一掃）に移動する */
				}
			} else {
				IO_ChangeReq(PX_P2);/* この位置（一掃）に移動する */
				first_p22 = 1;/* 最初のオフセットタイマは特殊 */
				IO_ChangeReq(P2_P2);/* オフセットタイマ（120秒待つ） */
			}
		} else {
			IO_ChangeReq(PX_P2);/* この位置（一掃）に移動する */
		}
#else
		if(teiden_hukki_flg == 1) {
			/* 停電復帰フラグ */
			IO_ChangeReq(PX_FAIL);/* フェイルの位置（一掃）に移動する */
		} else {
			// 20170217
			if (my_tanmatsu_type == CONTA_ADDRESS){
				IO_ChangeReq(PX_P2);/* この位置（一掃）に移動する */
				first_p22 = 1;/* 最初のオフセットタイマは特殊 */
				IO_ChangeReq(P2_P2);/* オフセットタイマ（120秒待つ） */
				//ここではやらない IO_ChangeReq(PX_P1);/* 通常に遷移 */
			} else {
				IO_ChangeReq(PX_P2);/* この位置（一掃）に移動する */
			}
		}
#endif
		/*
		その後関数ActionRemoteSrv内において変移の時間なので変移に移動するかな？
		*/
	}
	
	ByouWrite(now_status.status);/* 発光鋲を初期化する */
	SetNowLEDStatus(now_status.status);/* 初期状態をLEDに反映 */
	if ((my_tanmatsu_type == 1) || (my_tanmatsu_type ==4)) {
		ToukaWrite(my_tanmatsu_type);/* 灯火板への出力処理 */
	}
	if (my_tanmatsu_type == 4) {/* 制御期のみ */
		NaishouWrite(now_status.status);/* 内照板を初期化する */
	}
	
	
//	printf("*****now_status.power_outage_flag=%d\n",now_status.power_outage_flag);
	
	
}

int deb(int code)
{
//	extern int cont_output_data;
//extern uint16_t hakkobyou_out_reg_buf;//なぜかintだとおかしくなっていた

//	if ((hakkobyou_out_reg_buf & 0x110) == 0x110) {
//		printf("**********************code=%d,hakkobyou_out_reg_buf=%X\n",code,hakkobyou_out_reg_buf);
//		while(1) {
//			;
//		}
//	}
//	if ((cont_output_data & 0xc000011) != 0) {
//		printf("**********************code=%d,cont_output_data=%X\n",code,cont_output_data);
//		while(1) {
//			;
//		}
//	}
	return 0;
}

/**
 *	@brief 制御機共通の処理
 *
 *	@retval なし
 */
void ContSrv(RESPONSE_COMMAND *resp)
{
	lap_count_timer = 0;/* 経過時間測定用のタイマー */
	response = resp;/* 制御機A,Bの自身の応答電文アドレスはここセットする */
	deb(1);
	SWSrv(&now_status);/* SW状態読み込み処理 */
	deb(2);
	ContLEDSrv();/* LED制御処理 */
	deb(3);
	ActionSrv();/* 各種動作処理 */
	deb(4);
	PowerOutMainSrv();/* 停電発生時の処理 */
	deb(5);
	BSPLedSrv();/* BSPLEDの表示処理 */
	deb(6);
	if (board_status_error_check_flg == 1) {
		board_status_error_check_flg = 0;/* 可変表示板エラー判定要求フラグクリア */
	deb(7);
		BoardStatusErrorCheck();/* 可変表示板エラー判定処理 */
	deb(8);
	}
	if (lap_time_max < lap_count_timer) {
		lap_time_max = lap_count_timer;/* 経過時間最大値更新 */
	}
}

/**
 *	@brief 現在のモードをセットする処理
 *
 *	@retval なし
 */
void SetMode(STATUS *status, RESPONSE_COMMAND *response)
{
	int mode = 0;
	
	if (status->test_flag == 1) {
		/* テスト中フラグ */
		mode |= 0x10;
	}

	if (status->mode == MODE_MANUAL) {
		/* 端末手動 */
		mode |= 0x40;
	} else if (status->mode == MODE_MONITOR) {
		/* 監視盤からの指令 */
		mode |= 8;
	}
	/* 常に今の状態を返せばよいのではないだろうか */
	if (status->status == STATUS_P1) {
		/* 通常 */
		mode |= 1;
	} else if (status->status == STATUS_P2) {
		/* 一掃 */
		mode |= 2;
	} else if (status->status == STATUS_P3) {
		/* 変移 */
		mode |= 4;
	} else if (status->status == STATUS_FAIL) {
		/* フェイル指定 */
		mode |= 0x20;
	}
	response->response.status[0] = mode;
}

/**
*	@brief 制御応答用バッファ(response_command)にセットする処理
 *
 *	@retval なし
 */
void SetResponseStatus(RESPONSE_COMMAND *response)
{
	char str[256];
//	response_command.response.byte3.board_error1 = 1;
//	response_command.response.byte3.board_error3 = 1;
	int i;
	int d = 0;
	int board_error_cancel_flag = 0;/* 表示板異常をキャンセルするやいなや 0:しない,1:する */
	int bef_board_error = now_status.board_error;
	int bef_kiten_error = response->response.byte2.kiten_error;
	int now_board_error;
	int bef_d = response->response.byte3.byte;/* 標識板異常を保持  */

	now_status.board_error = 0;/* 表示板異常クリア */
	response->response.byte2.tanmatu_error = 0;/* 端末制御機異常クリア */
	response->response.byte2.kiten_error = 0;/* 起点案内標識異常クリア */
	response->response.byte2.jimaku = 0;/* 字幕移動中フラグクリア */
	response->response.byte2.hoshu_status = 0;/* 1:保守/0:通常 */
	if (now_status.byou1_status == 1) {
		response->response.byte2.tanmatu_error = 1;/* 端末制御機異常もセット */
		response->response.byte4.byou1_error = 1;/* 道路発光鋲1異常フラグセット */
	} else {
		response->response.byte4.byou1_error = 0;/* 道路発光鋲1異常フラグクリア */
	}
	if (now_status.byou2_status == 1) {
		response->response.byte2.tanmatu_error = 1;/* 端末制御機異常もセット */
		response->response.byte4.byou2_error = 1;/* 道路発光鋲2異常フラグセット */
	} else {
		response->response.byte4.byou2_error = 0;/* 道路発光鋲2異常フラグクリア */
	}
	if (now_status.musen_status != 0) {
		response->response.byte2.tanmatu_error = 1;/* 端末制御機異常もセット yamazaki20170207 */
		response->response.byte2.musen_error = 1;/* 無線異常セット */
	} else {
		response->response.byte2.musen_error = 0;/* 無線異常クリア */
	}
	if (now_status.pc_tuushin_status != 0) {
		response->response.byte2.pc_tuushin_error = 1;/* 運用管理PC間通信異常セット */
	} else {
		response->response.byte2.pc_tuushin_error = 0;/* 運用管理間PC通信異常クリア */
	}
	if (now_status.moniter_tuushin_status != 0) {
		response->response.byte2.moniter_tuushin_error = 1;/* 監視盤間通信異常セット */
	} else {
		response->response.byte2.moniter_tuushin_error = 0;/* 監視盤間通信異常クリア */
	}
	if ((my_tanmatsu_type == CONTA_ADDRESS) && (now_status.hoshu_status != 0)) {
		response->response.byte2.hoshu_status = 1;/* 1:保守/0:通常 */
	} else {
		response->response.byte2.hoshu_status = 0;/* 1:保守/0:通常 */
	}
	response->response.byte5.byte = 0;/* 時計異常フラグクリア */
	if (now_status.time_status == 1) {
		sprintf(str,"時計異常フラグセット\n");
		DebugPrint("", str, 1);
		response->response.byte5.byte = 1;/* 時計異常フラグセット */
	}

	now_board_error = 0;
	if (bef_board_error == 1) {
		for( i =  0; i < 8; i++){
			d >>= 1;
			if (board_status[i] == 1) {
				d |= 0x80;
				now_board_error = 1;/* 表示板異常もセット */
			}
		}
	}
	
#if 1
	/* 20170225 標識板異常発生時、異常復帰が押されるまで復旧させない */
	if ((bef_board_error == 1) && (now_board_error == 0)) {
		/* 前回標識板異常で今回標識板正常 */
		if (err_recover_flg == 0) {
			/* 異常復帰ボタンが押されていない状態 */
			board_error_cancel_flag = 1;/* 表示板異常復帰をキャンセルするやいなや 0:しない,1:する */
			sprintf(str, "***board異常復帰をキャンセルする\n");
			DebugPrint("", str, 2);
		} else {
			sprintf(str, "***board異常復帰をキャンセルしない\n");
			DebugPrint("", str, 2);
		}
	}
	
	if (board_error_cancel_flag == 1) {
		/* 表示板異常復帰をキャンセルする */
		now_status.board_error = 1;/* 表示板異常もセット */
		response->response.byte2.tanmatu_error = 1;/* 端末制御機異常もセット */
		response->response.byte2.kiten_error = bef_kiten_error;
	} else {
		/* 表示板異常復帰をキャンセルしない */
		for( i =  0; i < 8; i++){
			d >>= 1;
			if (board_status[i] == 1) {
				d |= 0x80;
				now_status.board_error = 1;/* 表示板異常もセット */
				response->response.byte2.tanmatu_error = 1;/* 端末制御機異常もセット */
				if (my_io_info.kiten_annai[i] == 1) {
					/* 起点案内標識なので */
					response->response.byte2.kiten_error = 1;
				}
			}
		}
	}
	
	if (startup_error_status != 0) {
		/* 起動時エラー状態のままの場合はいやおう無しに端末制御機異常もセット */
		response->response.byte2.tanmatu_error = 1;/* 端末制御機異常もセット */
	}
#else
	for( i =  0; i < 8; i++){
		d >>= 1;
		if (board_status[i] == 1) {
			d |= 0x80;
			now_status.board_error = 1;/* 表示板異常もセット */
			response->response.byte2.tanmatu_error = 1;/* 端末制御機異常もセット */
			if (my_io_info.kiten_annai[i] == 1) {
				/* 起点案内標識なので */
				response->response.byte2.kiten_error = 1;
			}
		}
	}
	
#endif
	
	if (response->response.byte2.tanmatu_error == 0) {//20170225
		err_recover_flg = 0;/* kaji20170223 */
	}
	
	
	if (now_status.tanmatu_error != response->response.byte2.tanmatu_error) {
		now_status.tanmatu_error = response->response.byte2.tanmatu_error;
		SaveStatus();/* ステータスを保存 */
	}
	if (now_status.jimaku_ido_flag != 0) {
		response->response.byte2.jimaku = 1;/* 図柄字幕移動中フラグ */
	}
	if (board_error_cancel_flag == 1) {
		/* 表示板異常復帰をキャンセルする */
		response->response.byte3.byte = bef_d;
	} else {
		response->response.byte3.byte = d;
	}
	//char str[128];
	//sprintf(str, "*** SetResponseStatus*** byte3=%X,byte4=%X", response->response.byte3.byte, response->response.byte4.byte);
	//DebugPrint("", str, 1);

}

/*
 *===========================================================================================
 *					内部関数
 *===========================================================================================
 */

/**
 *	@brief BSPLEDの表示処理
 *         LED3 点滅
 *         LED4 0:遠隔,1:手動 or 監視盤
 *         LED7,7 00:通常,11:変移,01,10:一掃(変化時はオフセットタイマ動作中)
 *
 *
 *	@retval なし
 */
static void BSPLedSrv(void)
{
#ifndef windows
	if (cont_sec_timer < 2) {/* 2秒間はDGSWの状態を表示しておく */
		return;
	}
	if (now_status.mode == MODE_REMOTE) {
		BSP_LEDOff(BSP_LED_4);
	} else {
		BSP_LEDOn(BSP_LED_4);
	}
	if (now_status.status == STATUS_P1) {
		BSP_LEDOff(BSP_LED_7);
		BSP_LEDOff(BSP_LED_8);
	} else if (now_status.status == STATUS_P3) {
		BSP_LEDOn(BSP_LED_7);
		BSP_LEDOn(BSP_LED_8);
	} else {
		if (offset_time_up_value % 2) {
			BSP_LEDOff(BSP_LED_7);
			BSP_LEDOn(BSP_LED_8);
		} else {
			BSP_LEDOn(BSP_LED_7);
			BSP_LEDOff(BSP_LED_8);
		}
	}
#endif
}

/**
 *	@brief LED初期化処理
 *
 *	@retval なし
 */
static void ContLEDInit(void)
{
	int i;
	
	cont_led_toggle_count = TOGGLE_COUNT_MAX;
	cont_led_toggle_status = LED_OFF;
	for ( i = 0 ; i < CONT_LED_MAX_COUNT; i++) {
		cont_bef_led_status[i] = LED_STATUS_OFF;
		cont_led_status[i] = LED_STATUS_OFF;
	}
	cont_led_status[LED_TEIDEN] = LED_STATUS_ON;/* 起動時は停電表示灯 */

}

/**
 *	@brief 制御機のLED制御処理
 *
 *	@retval なし
 */
static void ContLEDSrv(void)
{
	int i;
	char str[100];
	
	for( i = 0 ; i < CONT_LED_MAX_COUNT; i++) {
		if (cont_bef_led_status[i] != cont_led_status[i]) {
			switch (cont_led_status[i]) {
			case LED_STATUS_OFF:
				sprintf(str, "Cont(%d) LED_STATUS_OFF",i);
				/* 消灯を出力 */
				ContLedOut(i,LED_STATUS_OFF );
				break;
			case LED_STATUS_ON:
				sprintf(str, "Cont(%d) LED_STATUS_ON",i);
				/* 緑点灯を出力 */
				ContLedOut(i,LED_STATUS_ON );
				break;
			case LED_STATUS_TOGGLE:
				sprintf(str, "Cont(%d) LED_STATUS_TOGGLE",i);
				cont_led_toggle_status = LED_OFF;
				check_cont_led_ms_timer = 0;
				break;
			default :
				break;
			}
			//DebugPrint("", str, 2);
			cont_bef_led_status[i] = cont_led_status[i];
		}
	}
	/* 点滅動作の切り替え処理 */
	if (check_cont_led_ms_timer >= cont_led_toggle_count ) {
		for( i = 0 ; i < CONT_LED_MAX_COUNT; i++) {
			switch(cont_led_status[i]) {
			case LED_STATUS_TOGGLE:
				if (cont_led_toggle_status == LED_OFF) {
					sprintf(str, "Cont LED_ON  %d",check_cont_led_ms_timer);
					/* 点灯を出力 */
					ContLedOut(i,LED_STATUS_ON );
				} else {
					sprintf(str, "Cont LED_OFF %d",check_cont_led_ms_timer);
					/* 消灯を出力 */
					ContLedOut(i,LED_STATUS_OFF );
				}
				//DebugPrint("", str, 2);
				break;
			default :
				break;
			}
		}
		if (cont_led_toggle_status == LED_OFF) {
			cont_led_toggle_status = LED_ON;
		} else {
			cont_led_toggle_status = LED_OFF;
		}
		check_cont_led_ms_timer = 0;
		
	}
	ContLedWrite();/* LEDに出力する */
}

/* SW状態読み込み処理 */
static void SWSrv(STATUS *status)
{
/*
	コンソールから
	d リターンとして表示をオフする
	その後 sw XX　リターンとする
	XX=10は起動ボタン
	   3->13 端末手動の通常
	   5->15 端末手動の一掃
	   9->19 端末手動の変移
	   0  遠隔に戻る
*/
	int sw = SWRead();/* SW読み込み処理 */
	char str[256];
	if (bef_sw != sw) {
		sprintf(str, "bef_sw(%.02X)->sw(%.02X)", bef_sw, sw);
		DebugPrint("", str, 0);
		if ((bef_sw & SW_STATUS_BIT) != (sw & SW_STATUS_BIT)) {
			if ((sw & SW_STATUS_BIT) == 0) {
				/* 手動から遠隔に変わった */
				sprintf(str,"SWによる端末手動から遠隔モードに移行");
				DebugPrint("", str, 0);
				EventRequest(REMOTE_REQUEST);/* 遠隔リクエスト */
				SaveStatus();/* ステータスを保存 */
			}
		}
		if ((sw & SW_STATUS_BIT) != 0) {
			/* 手動状態 */
			now_status.mode = MODE_MANUAL;/* 20170303 */
			now_status.manual_status = 0;/* kaji20170303 手動状態 0:SWによる手動,1:運用PCからの手動による */
			if ((bef_sw & SW_START_BIT) != (sw & SW_START_BIT)) {
				if ((sw & SW_START_BIT) != 0) {
					/* 起動ボタンが押された */
					if (board_rotate_stage_no != BOARD_ROTATE_IDLE) { //20170216
							sprintf(str,"アイドル状態ではないのでキャンセル");
							DebugPrint("", str, 0);
					} else if ((sw & SW_TUUJOU_BIT) != 0) {
						/* 通常 sw 0->0x13 */
						now_status.manual_status = 0;/* 手動状態 0:SWによる手動,1:運用PCからの手動による */
						if (now_status.status == STATUS_P2) {
							sprintf(str,"SWによる手動 通常に設定");
							DebugPrint("", str, 0);
							EventRequest(SW_MANUAL_TUUJOU_REQUEST);/* SW手動通常リクエスト */
						} else if (now_status.status == STATUS_FAIL) {
							sprintf(str,"フェイルだがSWによる手動 通常に設定");
							DebugPrint("", str, 0);
							EventRequest(SW_MANUAL_TUUJOU_REQUEST);/* SW手動通常リクエスト */
						} else if (now_status.status == STATUS_P1) {
							sprintf(str,"通常だがSWによる手動 通常に設定");//20170205
							DebugPrint("", str, 0);
							EventRequest(SW_MANUAL_TUUJOU_REQUEST);/* SW手動通常リクエスト */
						} else {
							sprintf(str,"SWによる手動 通常に設定は一掃でないためキャンセル");
							EventRequest(SW_MANUAL_TUUJOU_REQUEST);/* SW手動通常リクエスト */
							DebugPrint("", str, 0);
						}
					} else if ((sw & SW_ISSOU_BIT) != 0) {
						/* 一掃 0->0x15 */
						now_status.manual_status = 0;/* 手動状態 0:SWによる手動,1:運用PCからの手動による */
						sprintf(str,"SWによる手動 一掃に設定");
						DebugPrint("", str, 0);
						EventRequest(SW_MANUAL_ISSOU_REQUEST);/* SW手動一掃リクエスト */
					} else if ((sw & SW_HENNI_BIT) != 0) {
						now_status.manual_status = 0;/* 手動状態 0:SWによる手動,1:運用PCからの手動による */
						if (now_status.status == STATUS_P2) {
							/* 変移 0->0x19 */
							sprintf(str,"SWによる手動 変移に設定");
							DebugPrint("", str, 0);
							EventRequest(SW_MANUAL_HENNI_REQUEST);/* SW手動変移リクエスト */
						} else if (now_status.status == STATUS_FAIL) {
							/* 変移 0->0x19 */
							sprintf(str,"ファイルだがSWによる手動 変移に設定");
							DebugPrint("", str, 0);
							EventRequest(SW_MANUAL_HENNI_REQUEST);/* SW手動変移リクエスト */
						} else if (now_status.status == STATUS_P3) {
							/* 変移 0->0x19 */
							sprintf(str,"変移だがSWによる手動 変移に設定");//20170205
							DebugPrint("", str, 0);
							EventRequest(SW_MANUAL_HENNI_REQUEST);/* SW手動変移リクエスト */
						} else {
							sprintf(str,"SWによる手動 変移に設定は一掃でないためキャンセル");
							DebugPrint("", str, 0);
						}
					}
					/* 20170221 */
					/* CDS状態 */
					if (now_status.cds_status == 1) {
						ChoukouWrite(1);/* 調光強制出力処理 */
					} else {
						ChoukouWrite(0);/* 調光強制出力処理 */
					}
					ByouWrite(now_status.status);/* すぐに対応するため */
					ToukaWrite(my_tanmatsu_type);//20170129 yamazaki
				}
			}
		}
		if ((bef_sw & SW_ERROR_RECOVERT_BIT) != (sw & SW_ERROR_RECOVERT_BIT)) {
			if ((sw & SW_ERROR_RECOVERT_BIT) != 0) {
				/* 異常復帰ボタンが押された */
				sprintf(str,"異常復帰ボタンが押された");
				DebugPrint("", str, 0);
				startup_error_status = 0;/* 起動時のエラーの有無を保持(0:エラー無し、1:エラー有り) */
				err_recover_flg = 1;/* 異常復帰ボタンON状態 */
				/* kaji20170223↓ */
				now_status.byou_status = 0;
				now_status.byou1_status = 0;
				now_status.byou2_status = 0;
				keep_byou_status_flg = 0;
				ByouWrite(now_status.status);/* すぐに対応するため */
				/* kaji20170223↑ */
				keep_board_status_flg = 0;/* kaji20170307 */
				cont_led_status[LED_BOARD] = LED_STATUS_OFF;
				cont_led_status[LED_BYOU] = LED_STATUS_OFF;
				cont_led_status[LED_DENSOU] = LED_STATUS_OFF;
//				cont_led_status[LED_MUSEN] = LED_STATUS_OFF;/* kaji20170301 無線は自動復帰なので消さないことにする */
			}
		}
		if ((bef_sw & SW_TEIDEN_HUKKI_BIT) != (sw & SW_TEIDEN_HUKKI_BIT)) {
			if ((sw & SW_TEIDEN_HUKKI_BIT) != 0) {
				/* 停電復帰ボタンが押された */
				sprintf(str,"停電復帰ボタンが押された");
				DebugPrint("", str, 0);
				cont_led_status[LED_TEIDEN] = LED_STATUS_OFF;/* 停電表示灯消灯 */
//				TeidenDispWrite( 0);/* 停電表示灯クリア */
			}
		}
		if ((bef_sw & SW_MAINTENANCE_BIT) != (sw & SW_MAINTENANCE_BIT)) {
			if ((sw & SW_MAINTENANCE_BIT) == 0) {
				/* 保守状態から通常状態に変化した */
				now_status.hoshu_status = 0;/* 1:保守/0:通常 */
				sprintf(str,"保守状態から通常状態に変化した");
				DebugPrint("", str, 0);
			} else {
				/* 通常状態から保守状態に変化した */
				now_status.hoshu_status = 1;/* 1:保守/0:通常 */
				sprintf(str,"通常状態から保守状態に変化した");
				DebugPrint("", str, 0);
			}
		}
		if ((bef_sw & SW_KEIKOUTOU_BIT) != (sw & SW_KEIKOUTOU_BIT)) {
			if ((sw & SW_KEIKOUTOU_BIT) == 0) {
				/* 蛍光灯入状態から蛍光灯自動状態に変化した */
				now_status.keikoutou_status = 0;/* 1:蛍光灯入/0:自動 */
				sprintf(str,"蛍光灯入状態から蛍光灯自動状態に変化した");
				DebugPrint("", str, 0);
			} else {
				/* 蛍光灯自動状態から蛍光灯入状態に変化した */
				now_status.keikoutou_status = 1;/* 1:蛍光灯入/0:自動 */
				sprintf(str,"蛍光灯自動状態から蛍光灯入状態に変化した");
				DebugPrint("", str, 0);
			}
			/* CDS状態 */
			if (now_status.cds_status == 1) {
				ChoukouWrite(1);/* 調光強制出力処理 */
			} else {
				ChoukouWrite(0);/* 調光強制出力処理 */
			}
			ByouWrite(now_status.status);/* すぐに対応するため */
			ToukaWrite(my_tanmatsu_type);//20170129 yamazaki
		}
		if ((bef_sw & SW_BYOU_BIT) != (sw & SW_BYOU_BIT)) {
			if (((bef_sw & SW_BYOU_BIT) == 0) && ((sw & SW_BYOU_BIT) != 0)){
				/* 発光鋲正常から発光鋲異常に変化した */
				now_status.byou_status = 1;/* 発光鋲状態 */
				keep_byou_status_flg = 1;/* 発光鋲異常を保持するフラグ */
				sprintf(str,"発光鋲正常から発光鋲異常に変化した 2");
//				DebugPrint("", str, 0x10);
				DebugPrint("", str, 0);
				cont_led_status[LED_BYOU] = LED_STATUS_TOGGLE;
				SetByouStatus(sw);/* 発光鋲ステータスをセットする処理 */
			//} else if (((bef_sw & SW_BYOU_BIT) != 0) && ((sw & SW_BYOU_BIT) == 0)){
				/* 一度エラーとなったら二度と復帰しない 20170130 → 自動復帰しない kaji20170223 */
			} else if (((bef_sw & SW_BYOU_BIT) != 0) && ((sw & SW_BYOU_BIT) == 0) && (keep_byou_status_flg == 0)){
				/* 発光鋲異常から発光鋲正常に変化した */
				now_status.byou_status = 0;/* 発光鋲状態 */
//printf("pass 1\n");
				sprintf(str,"発光鋲異常から発光鋲正常に変化した 4");
				DebugPrint("", str, 0x10);
				cont_led_status[LED_BYOU] = LED_STATUS_OFF;
				ByouWrite(now_status.status);/* すぐに対応するため */
			} else {
				sprintf(str,"発光鋲異常(%X)から発光鋲異常(%X)に変化した",bef_sw, sw );
				DebugPrint("", str, 0x10);
			}
		}
		
		bef_sw = sw;
	}
}

/* 発光鋲ステータスをセットする処理 */
void SetByouStatus(int sw)
{
	if ((sw & SW_BYOU1_BIT) != 0) {
		now_status.byou1_status =1;
	} else {
		now_status.byou1_status =0;
	}
	if ((sw & SW_BYOU2_BIT) != 0) {
		now_status.byou2_status =1;
	} else {
		now_status.byou2_status =0;
	}
	if (param.no_fail_flag == 1) {/* この場合はデバッグ用にフェイルにはしない */
		now_status.byou_status = 0;
		now_status.byou1_status = 0;
		now_status.byou2_status = 0;
	}

//printf("pass 1 %d,%d,%d\n",now_status.byou_status,now_status.byou1_status,now_status.byou2_status);
/* ここで発光鋲の有り無し判定を行い異常ステータスをクリアすることにする */
	/* 理由：それが一番簡単と思えるから */
	if (hakkobyou_exist[my_tanmatsu_type-1][0] == 0) {
		if (hakkobyou_exist[my_tanmatsu_type-1][1] == 0) {
			/* 発光鋲１無し、２無し */
			now_status.byou_status = 0;
			now_status.byou1_status = 0;
			now_status.byou2_status = 0;
			cont_led_status[LED_BYOU] = LED_STATUS_OFF;
		} else {
			/* 発光鋲１無し、２有り */
			printf("発光鋲１無し、２有りの場合の処理 %d,%d\n",now_status.byou1_status,now_status.byou2_status);
			now_status.byou1_status = 0;
			if (now_status.byou2_status == 0) {
				/* 発光鋲２が異常でないなら */
				now_status.byou_status = 0;
				now_status.byou1_status = 0;
				now_status.byou2_status = 0;
				cont_led_status[LED_BYOU] = LED_STATUS_OFF;
			}
		}
	} else {
		if (hakkobyou_exist[my_tanmatsu_type-1][1] == 0) {
			/* 発光鋲１有り、２無し */
			if (now_status.byou1_status == 0) {
				/* 発光鋲１が異常でないなら */
				now_status.byou_status = 0;
				now_status.byou1_status = 0;
				now_status.byou2_status = 0;
				cont_led_status[LED_BYOU] = LED_STATUS_OFF;
			}
		} else {
			/* 発光鋲１有り、２有り */
		}
	}
//printf("pass 3 %d,%d,%d\n",now_status.byou_status,now_status.byou1_status,now_status.byou2_status);
}

/* 各種動作処理 */
/*
動作状態判定
異常判定
*/
static void ActionSrv(void)
{
//20170217	if (now_status.mode == MODE_REMOTE) {
	if ((now_status.mode == MODE_REMOTE) && (my_tanmatsu_type == CONTA_ADDRESS)){
		ActionRemoteSrv();/* 遠隔時の動作判定処理 */
	}
	ScheduleRequestChkSrv();/* スケジュール更新要求動作判定処理 */
	BoardRotateSrv();/* IO動作処理 */
	IO_ChangeSrv();/* 表示板,鋲等を変化させる処理 */
}

/**
 *	@brief 遠隔時の動作判定処理
 *
 *	@retval なし
 */
static void ScheduleRequestChkSrv(void)
{
	char str[256];
//#define NEW_DAY_TIME (60) /* 新たな一日が始まる時刻(00:01とする) */
#define NEW_DAY_TIME (1) /* 新たな一日が始まる時刻(00:01とする) */
//#define NEW_DAY_TIME (10*60+43) /* 新たな一日が始まる時刻 */
	if ((date_change_flag == 0) && (GetTodaysMinute() == NEW_DAY_TIME)) {/* kaji20170302 1秒の間に来ないことを危惧していた？ */
//	if ((date_change_flag == 0) && (GetTodaysSecond() == NEW_DAY_TIME)) {/* kaji20170223 */
	//if ((date_change_flag == 0) && (GetTodaysMinute() == 0)) {
		/* 新たな一日が始まった */
		/* ここでスケジュールを要求したいところ */
		sprintf(str,"新たな一日が始まった");
		DebugPrint("", str, 2);
		date_change_flag = 1;
		now_status.schedule = 0;/* これでスケジュール登録要求を送信する */
		now_status.time_req = 0;/* これで時刻設定要求を送信する */
		now_status.start_time = 0;
		now_status.end_time = 0;
		schedule_timer = 0;/* スケジュール要求用のタイマー */
		
	} else if ((date_change_flag == 1) && (GetTodaysMinute() != NEW_DAY_TIME)) {
		sprintf(str,"新たな一日が始まった+1");
		DebugPrint("", str, 2);
		date_change_flag = 0;
		/*
		ここで運用管理PCからのスケジュール登録がされていなかったら
		制御機Aは自分の情報から登録を行い、
		制御機Bに送信する
		*/
//		if (now_status.schedule == 0) {
//			if (my_tanmatsu_type == CONTA_ADDRESS) {
//				/* 制御機A */
//				SetSchedule( );/* 本日のスケジュールをセットする処理 */
//			}
//		}
	}

}

/**
 *	@brief 遠隔時の動作判定処理
 *
 *	@retval なし
 */
static void ActionRemoteSrv(void)/* 遠隔時の動作 */
{
	char str[256];
/*
	状態変化の条件

	休日モードでは無い
	開始時間になった
	終了時間になった
	手動で変化要求があった
*/
	
	int tmp_now_time = GetTodaysSecond();/* 現在の時刻取得（秒換算）*/
	if ( tmp_now_time == now_time ) {
		/* 時刻に変化が無ければ何もしない */
		return;
	}

	if ((now_status.start_time == 0) && (now_status.end_time == 0)) {
		if ((now_status.status != STATUS_P1) && (now_status.status != STATUS_FAIL)) {
			/* 時間が設定されていない場合且つFAILでない場合は遠隔時は通常とする */
//これはまずい、手動から遠隔に変わったとき破たんする
//		printf("A******now_status.status=%d\n",now_status.status);
//			SetNowStatus(STATUS_P1);/* 遠隔通常 */;
//		printf("A******now_status.status=%d\n",now_status.status);
		}
//遠隔変移で起動するとだめになるので		return;
	}
	/* 制御機Bに送信する場合はこれを有効にするが、今は未使用 */
	//if (my_tanmatsu_type == CONTA_ADDRESS) {
		if ((now_status.status == STATUS_P1) && ((GetNowRemoteStatus() == STATUS_P1P2) || (GetNowRemoteStatus() == STATUS_P3))) {
			/* ステータスが通常で今の時間が変位 */
//			if (fail_flag == 0) {/* 20170224 フェイル中は移動しない */
			if (fail_hnd_state == FAIL_HND_NONE) {/* kaji20170225 フェイル中は移動しない */
				if (empty(IO_QUEUE) && (board_rotate_stage_no == BOARD_ROTATE_IDLE) ) {
					sprintf(str, "***ステータスが通常で今の時間が変位");
					DebugPrint("", str, 4);
					EventRequest(REMOTE_TUUJOU2HENNI_REQUEST);/* 遠隔時変移時間開始リクエスト */
					//remote_command_request = 1;/* 遠隔時の表示板変更要求ありフラグ */
					//remote_command_command = 2;/* 遠隔時の表示板一掃への変更コマンド */
				}
			}
		}
		if ((now_status.status == STATUS_P3) && ((GetNowRemoteStatus() == STATUS_P3P2) || (GetNowRemoteStatus() == STATUS_P1))) {
			/* ステータスが変位で今の時間が通常 */
//			if (fail_flag == 0) {/* 20170224 フェイル中は移動しない */
			if (fail_hnd_state == FAIL_HND_NONE) {/* kaji20170225 フェイル中は移動しない */
				if (empty(IO_QUEUE) && (board_rotate_stage_no == BOARD_ROTATE_IDLE) ) {
					sprintf(str, "***ステータスが変位で今の時間が通常");
					DebugPrint("", str, 4);
					EventRequest(REMOTE_HENNI2TUUJOU_REQUEST);/* 遠隔時通常時間開始リクエスト */
					//remote_command_request = 1;/* 遠隔時の表示板変更要求ありフラグ */
					//remote_command_command = 2;/* 遠隔時の表示板一掃への変更コマンド */
				}
			}
		} else if ((now_status.status == STATUS_FAIL) && ((GetNowRemoteStatus() == STATUS_P3P2) || (GetNowRemoteStatus() == STATUS_P1))) {
			/* ステータスが変位で今の時間が通常 */
//20170212			if (empty(IO_QUEUE) && (board_rotate_stage_no == BOARD_ROTATE_IDLE) ) {
//20170212				sprintf(str, "***ステータスがフェイルで今の時間が通常");
//20170212				DebugPrint("", str, 4);
//20170212				EventRequest(REMOTE_HENNI2TUUJOU_REQUEST);/* 遠隔時通常時間開始リクエスト */
				//remote_command_request = 1;/* 遠隔時の表示板変更要求ありフラグ */
				//remote_command_command = 2;/* 遠隔時の表示板一掃への変更コマンド */
//20170212			}
		}
		now_time = tmp_now_time;/* 時刻を更新しておく */
	//}
}

/**
 *	@brief IO動作処理
 *
 *	@retval なし
 */
int waiting_patterm;
int bef_match_count;/* 前回の項目一致数 */
int allowed_pattern[2];/* 許されるパターンを保持 */

/**
 *	@brief 可変表示板制御処理
 *
 *	@retval なし
 */
int board_rotate_start_time[DISPLAY_BOARD_MAX];
int board_rotate_delay_time = 0;/* 時間をずらして制御を開始するために変数*/
int bef_board_rotate_count = 0;
static void BoardRotateSrv(void)
{
	int i;
	int d;
	int match_count;
	int board_rotate_count;
	int out_put_req;/* out_putを強制する(power off用) */
	int st;
	
	switch (board_rotate_stage_no) {
	case BOARD_ROTATE_READY:
		io_control_timer = 0;
		bef_match_count = -1;/* かならず出力するためのー１ */
		board_rotate_timer = 0;
		for ( i = 0 ; i < display_board_count; i++) {
			board_rotate_start_time[i] = i * board_rotate_delay_time;/* 制御開始時間をセット */
		}
		waiting_patterm = GetWaitingPattern();/* 移動すべきパターンを求める */
		SetResponseStatus(response);/* IOステータスをセット */
		//now_status.jimaku_ido_flag = 1;/* 図柄字幕移動中にする */
		//cont_led_status[LED_ZUGARA] = LED_STATUS_ON;
		board_rotate_stage_no = BOARD_ROTATE_START;
		bef_board_rotate_count = 0;
		break;
	case BOARD_ROTATE_START:
		board_rotate_total_count++;/* 表示板回転処理全回数 */
		board_rotate_count = 0;
		for ( i = 0 ; i < display_board_count; i++) {
			if (board_rotate_timer >= board_rotate_start_time[i]) {
				board_rotate_count++;
			}
		}
		out_put_req = 0;
		if ( bef_board_rotate_count != board_rotate_count) {
			bef_board_rotate_count = board_rotate_count;
			out_put_req = 1;
		}
		CheckInput(waiting_patterm, board_rotate_count, out_put_req);/* IO入力データ判定処理 */
		if (board_rotate_count == display_board_count) {
			/* 全表示板の制御を開始している */
			board_rotate_stage_no = BOARD_ROTATE_WAIT;
		}
		
		break;
	case BOARD_ROTATE_WAIT:
			if (io_control_timer > UNMATCH_TIMEOUT_VALUE) {
				/* タイムアウト発生 */
				DebugPrint("BoardRotateSrv","BOARD_ROTATE_WAIT　タイムアウト発生", 0);
				board_rotate_stage_no = BOARD_ROTATE_END;
			}
		
			d = CheckInput(waiting_patterm, display_board_count, 0);/* IO入力データ判定処理 */
			if ( d == waiting_patterm) {
				board_rotate_stage_no = BOARD_ROTATE_END;
			} else {
				/* 初めから指定位置にあった場合は図柄字幕移動中にしないのでここでセットする */
				now_status.jimaku_ido_flag = 1;/* 図柄字幕移動中にする */
				cont_led_status[LED_ZUGARA] = LED_STATUS_ON;
				SetNowLEDStatusToggle(now_status.status);/* ステータスLEDを点滅状態にする処理 */
			}

		break;
	case BOARD_ROTATE_END:
			match_count = CheckBoardStatus(waiting_patterm);/* 正常な可変表示板の状態を獲得する処理 */
			if (match_count == display_board_count) {
//				if (startup_error_status != 0) {/* kaji20170223 */
				if (keep_board_status_flg != 0) {/* kaji20170307 */
					/* エラー無しだけどもエラーフラグ保持されているので状態継続 */
					DebugPrint( "BoardRotateSrv", "標識板移動完了（標識板異常継続）", 0);
				} else {
					/* エラー無し点滅しない */
					DebugPrint( "BoardRotateSrv", "標識板移動完了", 0);
					cont_led_status[LED_BOARD] = LED_STATUS_OFF;
				}
			} else {
				/* エラー有り点滅 */
				//led_status[0] = LED_STATUS_TOGGLE;
				for( i = 0; i < display_board_count; i++) {
					output_data[i] = 0;
					BoardWrite(i, 0);/* モータを停止する処理 */
				}
				DebugPrint( "BoardRotateSrv", "標識板異常リクエスト エラー有り点滅", 0);
				if (param.no_fail_flag == 1) {/* この場合はデバッグ用にフェイルにはしない */
					/* フェイルだけどフェイルにしない */
					DebugPrint("BoardRotateSrv","標識板異常フェイルだけどフェイルにしない", 0);
				} else {
					EventRequest(FAIL_REQUEST);/* 表示板異常リクエスト */
				}
				if (err_recover_flg == 0 ) {
					cont_led_status[LED_BOARD] = LED_STATUS_TOGGLE;
					keep_board_status_flg = 1;/* kaji20170307 */
				}
			}
			now_status.jimaku_ido_flag = 0;/* 図柄字幕移動中を解除する */
			SetResponseStatus(response);/* IOステータスをセット */
			cont_led_status[LED_ZUGARA] = LED_STATUS_OFF;
			SetNowLEDStatus(now_status.status);/* ステータスLEDを点灯状態にする処理 */
			board_rotate_stage_no = BOARD_ROTATE_IDLE;
		break;
	case BOARD_ROTATE_OFFSET_TIMER:
			if (offset_time_up_value == 0) {
				/* これは念のため、普通は通らない */
				offset_time_up_value = now_status.offset_timer;
			}
			if (first_p22 == 1) {
				/* 最初のオフセットタイマは特殊,10秒にする */
				//20170224 offset_time_up_value = 10;
				/* 結局オフセットタイマ値に戻した */
				offset_time_up_value = param.offset_timer;
			}
			DebugPrint( "BoardRotateSrv", "オフセットタイマ起動", 4);
			board_rotate_stage_no = BOARD_ROTATE_OFFSET_TIMER_END;
		break;
	case BOARD_ROTATE_OFFSET_TIMER_END:
			if (offset_time_up_value == 0) {
				DebugPrint( "BoardRotateSrv", "オフセットタイマ時間経過した", 4);
				/* 制御機Bに送信する場合はこれを有効にするが、今は未使用 */
				//if ((now_status.mode == MODE_REMOTE) && (my_tanmatsu_type == CONTA_ADDRESS)) {
				//	if (peek(IO_QUEUE, 0) == PX_P1) {
				//		remote_command_request = 1;/* 遠隔時の表示板変更要求ありフラグ */
				//		remote_command_command = 2;/* 遠隔時の表示板通常への変更コマンド */
				//		printf("制御機Bへの通常への変更要求\n");
				//	} else if (peek(IO_QUEUE, 0) == PX_P3) {
				//		remote_command_request = 1;/* 遠隔時の表示板変更要求ありフラグ */
				//		remote_command_command = 4;/* 遠隔時の表示板変移への変更コマンド */
				//		printf("制御機Bへの変移への変更要求\n");
				//	}
				//}
				board_rotate_stage_no = BOARD_ROTATE_IDLE;
				if (first_p22 == 1) {
					/* ここで最初の移動場所を決める */
					first_p22 = 0;
					if (now_status.power_outage_flag == 1) {
						/* 停電発生時 */
						printf("****** 停電発生時\n");
						EventRequest(FAIL_REQUEST);
					} else if ((now_status.mode == MODE_REMOTE) && (CheckTanmatuError() == 0)){
						st = GetNowRemoteStatus();
						if (st == STATUS_P1) {
							IO_ChangeReq(PX_P1);/* 通常に遷移 */
						} else if (st == STATUS_P1P2) {
							IO_ChangeReq(PX_P1);/* 通常に遷移 */
						} else if (st == STATUS_P3P2) {
							IO_ChangeReq(PX_P1);/* 通常に遷移 */
						} else {
							IO_ChangeReq(PX_P3);/* 変移に遷移 */
						}
					}
				}
			}
		break;
	case BOARD_ROTATE_IDLE:
		break;
	default:
		break;
	}
}

/**
 *	@brief 可変表示板エラー判定処理
 *
 *	@retval なし
 */
static void BoardStatusErrorCheck(void)
{
	//if ((board_rotate_total_count > 0) && (board_rotate_stage_no == BOARD_ROTATE_IDLE)) {
		if ((board_rotate_stage_no == BOARD_ROTATE_IDLE) || (board_rotate_stage_no == BOARD_ROTATE_OFFSET_TIMER_END)){
		int match_count = CheckBoardStatus(GetWaitingPattern());/* 正常な可変表示板の状態を獲得する処理 */
		if (match_count != display_board_count) {
			/* エラー有り点滅 */
			DebugPrint( "BoardStatusErrorCheck", "board板異常リクエスト エラー有り点滅", 8);
			if (param.no_fail_flag == 1) {/* この場合はデバッグ用にフェイルにはしない */
				/* フェイルだけどフェイルにしない */
				DebugPrint("BoardStatusErrorCheck","フェイルだけどフェイルにしない", 8);
			} else {
				if (now_status.status != STATUS_FAIL) {
					EventRequest(FAIL_REQUEST);/* 表示板異常リクエスト */
				} else {
					DebugPrint("BoardStatusErrorCheck","フェイルだけどすでにフェイルになっている", 8);
				}
			}
			if (err_recover_flg == 0 ) {
				cont_led_status[LED_BOARD] = LED_STATUS_TOGGLE;
				keep_board_status_flg = 1;/* kaji20170307 */
			}
		} else {
//20170307	if (startup_error_status == 0) {/* kaji20170223 起動時エラー保持されてない場合のみLED消す */
			if (keep_board_status_flg == 0) {/* kaji20170307 */
				cont_led_status[LED_BOARD] = LED_STATUS_OFF;/* 標識板は正常 */
			}
			if (now_status.byou_status != now_status.before_byou_status) {
				if (now_status.byou_status == 1) {
					EventRequest(FAIL_REQUEST);/* 発光鋲異常リクエスト */
				}
				now_status.before_byou_status = now_status.byou_status;
			} else {
//20170225				err_recover_flg = 0;/* kaji20170223 */
//				if (startup_error_status == 0) {/* kaji20170223 起動時エラー保持されてない場合のみLED消す */
				if (keep_byou_status_flg == 0) {/* kaji20170223 */
					cont_led_status[LED_BYOU] = LED_STATUS_OFF;/* 発光鋲は正常 */
//					printf("発光鋲異常LED消し(BoardStatusErrorCheck2)\n");
				}
			}
		}
		SetResponseStatus(response);/* IOステータスをセット */
	}
	
	
}

	
/**
 *	@brief 移動すべきパターンを求める処理
 *
 *	@retval 移動すべきパターン
 */
int GetWaitingPattern(void)
{
	int pattern = P1;
	switch (now_status.status) {
	case STATUS_P1:
		pattern = P1;
		break;
	case STATUS_P2:
		pattern = P2;
		break;
	case STATUS_P3:
		pattern = P3;
		break;
	case STATUS_FAIL:
		pattern = param.fail_pattern;
		break;
	}
	return pattern;
}

/**
 *	@brief 正常な可変表示板の状態を獲得する処理
 *
 *	@retval 正常な表示板数
 */
static int CheckBoardStatus(int dst)
{
	int i;
	int match_count = 0;
	for( i = 0; i < display_board_count; i++) {
		/* まず状態としてゆるされるパターンを求める */
		int allowed_pattern[2];
		GetAlowedPattern(dst, i, allowed_pattern);/* 目標位置に対しての許されるパターンを求める処理 */
		input_data[i] = BoardRead(i);/* 可変Boardステータス読み込み処理 */
		if ((param.no_board_error_flag & (1<<i)) != 0 ) {
			/* デバッグモード 強制的に一致とする */
			//printf("デバッグモード 強制的に一致とする %d\n",i);
			input_data[i] = allowed_pattern[0];
		}
		if (input_data[i] == allowed_pattern[0]) {
			match_count++;
			/* 一度でも異常になったら戻さないので正常にはしない */
			/* 仕様かもしれないが、とりあえず復帰するとする */
			board_status[i]=0;
		} else if ((param.same_nomove_flag == 1) && (input_data[i] == allowed_pattern[1])){ /* 同じ場合は可変表示版を変化させないモード */
			match_count++;/* パターン一致 */
			/* 一度でも異常になったら戻さないので正常にはしない */
			/* 仕様かもしれないが、とりあえず復帰するとする */
			board_status[i]=0;
		} else {
			board_status[i]=1;/* 異常 */
			if (now_status.cds_status == 1) {
				ChoukouWrite(1);/* 調光強制出力処理（蛍光灯消灯のための処理） */
			} else {
				ChoukouWrite(0);/* 調光強制出力処理 (蛍光灯消灯のための処理) */
			}
		}
		//printf("board_status[%d]=%d\n",i,board_status[i]);
	}
	return match_count;
}

/**
 *	@brief IO入力データ判定処理
 *
 *	@param [int dst]  設定すべきパターン(P1,P2,P3)
 *
 *	@retval なし
 */
static int CheckInput(int dst, int display_check_count, int out_put_req)
{
	int d = 0;
	int match_count = 0;
	int i;
	char str[128];
	char str2[128];
	for( i = 0; i < display_check_count; i++) {
		/* まず状態としてゆるされるパターンを求める */
		int allowed_pattern[2];
		GetAlowedPattern(dst, i, allowed_pattern);/* 目標位置に対しての許されるパターンを求める処理 */
		input_data[i] = BoardRead(i);/* 可変Boardステータス読み込み処理 */
		if ((param.no_board_error_flag & (1<<i)) != 0 ) {
			/* デバッグモード 強制的に一致とする */
			//printf("デバッグモード 強制的に一致とする %d\n",i);
			/* デバッグモード 強制的に一致とする */
			input_data[i] = allowed_pattern[0];
		}
		if (input_data[i] == allowed_pattern[0]) {
			match_count++;/* パターン一致 */
		} else if ((param.same_nomove_flag == 1) && (input_data[i] == allowed_pattern[1])){ /* 同じ場合は可変表示版を変化させないモード */
			match_count++;/* パターン一致 */
		}
	}
	if (match_count == display_check_count) {
		/* 全ての表示板がパターン一致 */
		d = dst;
	}
	if (bef_match_count != match_count) {
		sprintf(str, "CheckInput input_data %d,%d= ",bef_match_count,match_count);
		for(i=0;i < display_check_count;i++) {
			sprintf(str2, "%02X ",input_data[i]);
			strcat(str, str2);
		}
		DebugPrint("", str, 1);
		Output(dst, display_check_count);/* 入力が変化したので出力する */
	} else if (out_put_req == 1){
		Output(dst, display_check_count);/* 入力が変化したので出力する */
	}
	bef_match_count = match_count;
	return d;
}
	
	
/**
 *	@brief 目標位置に対しての許されるパターンを求める処理
 *
 *	@param [int dst]  設定すべきパターン(P1,P2,P3)
 *	@param [int no]   表示板番号
 *	@param [int allowed_pattern]  許されるパターン(P1,P2,P3)格納ポインタ
 *
 *	@retval なし
 */
static void  GetAlowedPattern(int dst, int no, int *allowed_pattern)
{
	if ( dst == P1) {
		allowed_pattern[0] = my_io_info.allowed_pattern_p1[no][0];
		allowed_pattern[1] = my_io_info.allowed_pattern_p1[no][1];
	} else if ( dst == P2) {
		allowed_pattern[0] = my_io_info.allowed_pattern_p2[no][0];
		allowed_pattern[1] = my_io_info.allowed_pattern_p2[no][1];
	} else {
		/* dst == P3 */
		allowed_pattern[0] = my_io_info.allowed_pattern_p3[no][0];
		allowed_pattern[1] = my_io_info.allowed_pattern_p3[no][1];
	}
}

/**
 *	@brief IO出力処理
 *
 *	@param [int dst]  設定すべきパターン(P1,P2,P3)
 *
 *	@retval なし
 */
static void Output(int dst, int board_count)
{
	int i;
	char str[128];
	char str2[128];
	for( i = 0; i < board_count; i++) {
		/* まず状態としてゆるされるパターンを求める */
		int allowed_pattern[2];
		GetAlowedPattern(dst, i, allowed_pattern);/* 目標位置に対しての許されるパターンを求める処理 */
		if (input_data[i] == allowed_pattern[0]) {
			output_data[i] = 0;/* パターン一致ならば停止する */
		} else if ((param.same_nomove_flag == 1) && (input_data[i] == allowed_pattern[1])){ /* 同じ場合は可変表示版を変化させないモード */
			output_data[i] = 0;/* パターン一致ならば停止する */
		} else {
			//yamazaki output_data[i] = 1;/* パターンが一致していないのでモータを動かす */
			output_data[i] = 2;/* パターンが一致していないのでモータを動かす */
		}
		BoardWrite(i, output_data[i]);/* Boardへの出力処理 */
	}
	sprintf(str, "Output output_data = ");
	for(i=0;i < display_board_count;i++) {
		sprintf(str2, "%02X ",output_data[i]);
		strcat(str, str2);
	}
	DebugPrint("", str, 4);
}
	
	
/**
 *	@brief 表示板,鋲等を変化させる処理要求をキューにいれる
 *
 *	@retval なし
 */
void IO_ChangeReq(char change_type)
{
	char str[128];
	int q_trash;
	int loop_flag;
	
	sprintf(str, "IO_ChangeReq %sを要求", request_pattern[(int)change_type]);
	DebugPrint("", str, 0);
// kaji20170308↓
	if (change_type == PX_FAIL) {
		
		int loop_flag = 1;
		while ((!empty(IO_QUEUE)) && (loop_flag == 1)) {
			switch (peek(IO_QUEUE, 0)) {
				case PX_P1:
				case PX_P2:
				case PX_P3:
				case P2_P2:
					q_trash = dequeue(IO_QUEUE);//先積みされたIO変更要求データを破棄
					sprintf(str, " -> PX_FAIL要求有りのため、IO_ChangeReq %sを破棄", request_pattern[(int)q_trash]);
					DebugPrint("", str, 0);
					break;
				default:
					loop_flag = 0;
					break;
			}
		}
	}
// kaji20170308↑
	enqueue(IO_QUEUE, change_type);
	return;
}

/**
 *	@brief 表示板,鋲等を変化させる処理
 *         ここで監視応答のステータスをセットする
 *
 *	@retval なし
 */
static void IO_ChangeSrv(void )
{
	int change_type;
	char str[128];
	if (board_rotate_stage_no != BOARD_ROTATE_IDLE){
		//printf("IO_ChangeSrv board_rotate_stage_no が BOARD_ROTATE_IDLEではない\n");
		return;
	}
	if (!empty(IO_QUEUE)) {
		change_type = dequeue(IO_QUEUE);//IO変更要求データ取り出し
		/* 20170226 オフセットタイマ中にエラーになった場合の処理 */
		if (change_type == PX_P3) {
			if ((my_tanmatsu_type == CONTA_ADDRESS) && (now_status.mode == MODE_REMOTE) && (CheckTanmatuError() != 0)){
				change_type = PX_FAIL;
				sprintf(str, "エラー状態のため変移からフェイルに変更\n");
				DebugPrint("", str, 0);
			}
		}
		else if (change_type == PX_P1) {
			if ((my_tanmatsu_type == CONTA_ADDRESS) && (now_status.mode == MODE_REMOTE) && (CheckTanmatuError() != 0)){
				sprintf(str, "エラー状態のため通常への移動をやめる\n");
				DebugPrint("", str, 0);
				return;
			}
		}
		sprintf(str, "IO_ChangeSrv %s", request_pattern[change_type]);
		DebugPrint("", str, 0);

		switch (change_type) {
		case PX_P2:
			board_rotate_stage_no = BOARD_ROTATE_READY;
			board_rotate_delay_time = 0;/* 即開始 */
			ByouWrite(STATUS_P2);/* 発光鋲を一掃状態にセット */
			if (my_tanmatsu_type == 4) {/* 制御期のみ */
				NaishouWrite(STATUS_P2);/* 発光鋲を一掃状態にセット */
			}
			SetNowStatus(STATUS_P2);/* 一掃 */;
			SetResponseStatus(response);/* IOステータスをセット */
			
			if (lenqueue(IO_QUEUE) >= 1) {
				if (peek(IO_QUEUE, 0) == P2_P2) {
					/* 先頭データはP2_P2か？ */
					/* 一掃移動の開始時にタイマーをセットしておく */
					offset_time_up_value = now_status.offset_timer;
				}
			}
			
			break;
		case PX_P3:
			board_rotate_stage_no = BOARD_ROTATE_READY;
			board_rotate_delay_time = 0;/* 即開始 */
			ByouWrite(STATUS_P3);/* 発光鋲を変移状態にセット */
			if (my_tanmatsu_type == 4) {/* 制御期のみ */
				NaishouWrite(STATUS_P3);/* 内照板を変移状態にセット */
			}
			SetNowStatus(STATUS_P3);/* 変移に設定 */;
			SetResponseStatus(response);/* IOステータスをセット */
			break;
		case PX_P1:
			board_rotate_stage_no = BOARD_ROTATE_READY;
			board_rotate_delay_time = 0;/* 即開始 */
			ByouWrite(STATUS_P1);/* 発光鋲を通常状態にセット */
			if (my_tanmatsu_type == 4) {/* 制御期のみ */
				NaishouWrite(STATUS_P1);/* 内照板を通常状態にセット */
			}
			SetNowStatus(STATUS_P1);/* 通常設定 */;
			SetResponseStatus(response);/* IOステータスをセット */
			break;
		case PX_FAIL:
			board_rotate_stage_no = BOARD_ROTATE_READY;
			board_rotate_delay_time = 0;/* 即開始 */
			ByouWrite(STATUS_FAIL);/* 発光鋲をフェイル状態にセット */
			if (my_tanmatsu_type == 4) {/* 制御期のみ */
				NaishouWrite(STATUS_FAIL);/* 内照板をフェイル状態にセット */
			}
			SetNowStatus(STATUS_FAIL);/* フェイルに設定 */;
			SetResponseStatus(response);/* IOステータスをセット */
			break;
		case P2_P2:
			board_rotate_stage_no = BOARD_ROTATE_OFFSET_TIMER;/* オフセットタイマ時間待ち */
			break;
		case PX_POWEROFF:
			board_rotate_stage_no = BOARD_ROTATE_READY;
			board_rotate_delay_time = 1000;/* 一秒ずつ開始を遅らせる */
			power_off_bef_status = now_status.status;/* 現在のステータスをセーブ */
			SetNowStatus(STATUS_FAIL);/* フェイルに設定 */;
			break;
		case PX_POWER_RECOVER:
			board_rotate_stage_no = BOARD_ROTATE_READY;
			board_rotate_delay_time = 0;/* 即開始 */
			SetNowStatus(power_off_bef_status);/* 現在のステータスに設定をも戻す */;
			break;
		default:
			/* ここに来てはいけない */
			break;
		}
	}
}

/**
 *	@brief 停電検出時の処理
 *
 *	@retval なし
 */
static void PowerOutMainSrv(void)
{
	char str[128];
	
	io_power_outage_flag = PowerStatusRead();/* 停電ステータス読み込み処理 */
	if ((bef_io_power_outage_flag == 0) && (io_power_outage_flag == 1)) {
		sprintf(str, "停電発生 in PowerOutMainSrv");
		DebugPrint("", str, 0);
		/* 停電発生 */
		now_status.power_outage_flag = 1;
		if (teiden_hukki_flg == 1) {
			now_status.power_outage_flag2 = 1;/* 停電復帰起動時の起動の後 */
		} else {
			now_status.power_outage_flag2 = 0;/* 停電復帰起動時の起動の後ではない */
		}
		cont_led_status[LED_TEIDEN] = LED_STATUS_ON;/* 起動時は停電表示灯 */
		ChoukouWrite(0);/* 調光強制切出力処理 */
		ToukaWrite(my_tanmatsu_type);//20170131 yamazaki
//20170226		EventRequest(POWER_OFF_REQUEST);/* 停電発生リクエスト */
//20170226		SaveStatus();/* ステータスを保存 */
	} if ((bef_io_power_outage_flag == 1) && (io_power_outage_flag == 0)) {
		sprintf(str, "停電復帰 in PowerOutMainSrv");
		DebugPrint("", str, 0);
		/* 停電発生復帰 */
		now_status.power_outage_flag = 0;
		EventRequest(POWER_RECOVER_REQUEST);/* 停電発生復帰リクエスト */
//20170226		SaveStatus();/* ステータスを保存 */
	}
	bef_io_power_outage_flag = io_power_outage_flag;
	return;
}

/**
 *	@brief 現在の遠隔時のステータスを返す処理
 *
 *	@retval 現在の遠隔時のステータス
 */
int GetNowRemoteStatus(void)
{
	if ((now_status.start_time == 0) && (now_status.end_time == 0)) {
		/* 時間が設定されていない場合は遠隔時は通常とする */
		return STATUS_P1;/* 通常 */
	}
	if (offset_time_up_value != 0) {
		return STATUS_P2;/* 一掃 */
	}
	//yamazaki int now_time = GetTodaysMinute();/* 現在の時刻取得*/
	int now_time = GetTodaysSecond();/* 現在の時刻取得*/
	/* start_timeと同時は変位 */
	/* end_timeと同時は通常 */
	//if (now_time < now_status.start_time) {
		if (now_time < (now_status.start_time - now_status.offset_timer)) {//yamazaki
		return STATUS_P1;/* 通常 */
	}else if (now_time < now_status.start_time) {
		return STATUS_P1P2;/* 通常からの変化時の一掃 */
	} else if (now_time < now_status.end_time) {
		return STATUS_P3;/* 変移 */
	} else if (now_time < (now_status.end_time + now_status.offset_timer)) {//yamazaki
		return STATUS_P3P2;/* 変移からの変化時の一掃 */
	} else {
		return STATUS_P1;/* 通常 */
	}
		
		
		
}

/**
 *	@brief 同報指令受信処理
 *
 *	@retval なし
 */
void BroadcastCommand(BROADCAST_COMMAND *com) {

	char str[256];
	char str2[256];

	DebugPrint("","BroadcastCommand 同報制御指令受信", 1);
	strcpy(str,"");
#if 1
	
	int tanmatu_no = (com->status.byte >> 5) + 1;
//	printf("*** %d,%d\n",tanmatu_no , my_tanmatsu_type);
	if ((com->status.byte & 8) != 0){/* テスト指令 */
		if (tanmatu_no == my_tanmatsu_type) {
			strcat(str,"テスト ");
			/* テストでステータスは変えない */
			now_status.test_flag = 1;/* テスト中フラグ */
			cont_led_status[LED_TEST] = LED_STATUS_ON;
		}
	} else if ((com->status.byte & 0x10) != 0) {/* テスト指令解除 */
		if (tanmatu_no == my_tanmatsu_type) {
			strcat(str,"テスト解除 ");
			now_status.test_flag = 0;/* テスト中フラグ */
			cont_led_status[LED_TEST] = LED_STATUS_OFF;
		}
	}
#else
	if (com->command.test != 0) {
		strcat(str,"テスト ");
		/* テストでステータスは変えない */
		now_status.test_flag = 1;/* テスト中フラグ */
		cont_led_status[LED_TEST] = LED_STATUS_ON;
	} else {
		now_status.test_flag = 0;/* テスト中フラグ */
		cont_led_status[LED_TEST] = LED_STATUS_OFF;
	}
#endif
	if ((com->light_command.sreq == 0) &&
		(com->light_command.time_req == 0) &&
		(com->light_command.rendou_req == 0) && //20170320
		(com->light_command.issei == 0) &&
		//2-170219 ((com->status.byte & 0x18) == 0)){/* テスト */
		(com->status.byte == 0)){/* テスト,GPS状態変化 */
			/* 他の要求が無い場合 */
			SetRequest(com, str);/* モード設定要求時の処理 */
	}

	if (com->light_command.sreq != 0) {
		/* スケジュール登録要求受信 */
		DebugPrint("", "BroadcastCommand スケジュール登録要求受信\n", 1);
		now_status.start_time = 60 * ( 10 * ((com->schedule.start_time[0]>>4) & 0xf) + 
				(com->schedule.start_time[0] & 0xf)) +
				10 * ((com->schedule.start_time[1]>>4) & 0xf) + 
				(com->schedule.start_time[1] & 0xf);
		now_status.start_time *= 60;/* 秒単位に変換 *///yamazaki
		now_status.end_time = 60 * ( 10 * ((com->schedule.end_time[0]>>4) & 0xf) + 
				(com->schedule.end_time[0] & 0xf)) +
				10 * ((com->schedule.end_time[1]>>4) & 0xf) + 
				(com->schedule.end_time[1] & 0xf);
		now_status.end_time *= 60;/* 秒単位に変換 *///yamazaki
		now_status.offset_timer = 10 * BIN(com->schedule.offset_timer);
		now_status.schedule = 1;/* 運用管理ＰＣからのスケジュール登録済み */
		if ( now_status.start_time != 0) {
			if ( (now_status.start_time != param.start_time) ||
				(now_status.end_time != param.end_time) ||
				(now_status.offset_timer != param.offset_timer) ) {
				param.start_time = now_status.start_time;
				param.end_time = now_status.end_time;
				param.offset_timer = now_status.offset_timer;
				DebugPrint("", "パラメータが変わったためパラメータセーブ\n", 0);
				SaveParam();
			}
		}
	}
	if (com->light_command.time_req != 0) {
		/* 時刻修正要求受信 */
		SetTime(&(com->t));/* 時刻設定処理 */
		SaveRTC(&(com->t));/* 不揮発用RTCに書き込む */
		now_status.time_req = 1;/* 運用管理ＰＣからの時刻修正済み */
	}
	if (com->light_command.issei != 0) {
		/* 調光一斉指令有受信 */
		if (com->light_command.choukou_iri != 0) {
			/* 調光強制入（低） */
			ChoukouWrite(1);/* 調光強制入出力処理 */
			board_choukou_status = 1;/* kaji20170330 */
			sprintf(str, "調光一斉指令有  調光強制入（低）受信 ");
		} else if (com->light_command.choukou_kiri != 0) {
			/* 調光強制切（高） */
			ChoukouWrite(0);/* 調光強制切出力処理 */
			board_choukou_status = 0;/* kaji20170330 */
			sprintf(str, "調光一斉指令有  調光強制切（高）受信 ");
		} else {
			sprintf(str, "調光一斉指令有  ?????");
		}
		DebugPrint("", str, 1);
	}
	
	if ((com->status.byte & 2) != 0) {
		/* GPS異常 */
		now_status.gps_status = 1;
	} else if ((com->status.byte & 4) != 0) {
		now_status.gps_status = 0;
	}
	if (now_status.gps_status != now_status.before_gps_status) {
		/* GPS異常の状態が変化した */
		//if (now_status.gps_status == 1) {
		//	/* 時計異常 */
		///	EventRequest(FAIL_REQUEST);/* 時計異常は表示板異常とするリクエスト */
		//} else {
		//	/* この場合は時計異常から復帰 */
		//	/* もしかして時計異常から復帰したのかもしれないのでチェックする */
		//	if ((now_status.status == STATUS_FAIL) && (CheckErrorStatus() == 0)) {
		//		EventRequest(FAIL_RECOVER_REQUEST);/* 時計異常復帰リクエスト */
		//	}
		//}
	}
	now_status.before_gps_status = now_status.gps_status;
	
	sprintf(str2, "運用管理ＰＣから受信 %s %02X %02X %02X st=%d:%d,ed=%d:%d,off=%d"
														,str
														,com->command.byte
														,com->light_command.byte
														,com->status.byte
														,(now_status.start_time/60)/60
														,(now_status.start_time/60)%60
														,(now_status.end_time/60)/60
														,(now_status.end_time/60)%60
														,now_status.offset_timer );

	DebugPrint("", str2, 1);
}

/**
 *	@brief モード設定要求時の処理
 *
 *	@retval なし
 */
static void SetRequest(BROADCAST_COMMAND *com, char *str)
{
// ------------------------------------- 除外↓
#ifdef kaji20170302
	if (com->command.shudou != 0) {
		now_status.manual_status = 1;/* 手動状態 0:SWによる手動,1:運用PCからの手動による */
		strcat(str,"手動 ");
		if (com->command.tuujou != 0) {
			EventRequest(MANUAL_TUUJOU_REQUEST);/* 手動通常リクエスト */
		} else if (com->command.issou != 0) {
			EventRequest(MANUAL_ISSOU_REQUEST);/* 手動一掃リクエスト */
		} else if (com->command.henni != 0) {
			EventRequest(MANUAL_HENNI_REQUEST);/* 手動変移リクエスト */
		} else if (com->command.fail != 0) {
			EventRequest(MANUAL_FAIL_REQUEST);/* 手動フェイルリクエスト */
		}
	} else if (com->command.teishi != 0) {
		strcat(str,"運用停止 ");
		EventRequest(MONITOR_REQUEST);/* 運用停止リクエスト */
	} else if (com->command.yobi2 != 0) {
		strcat(str,"運用停止解除 ");
		EventRequest(MONITOR_RELEASE_REQUEST);/* 運用停止解除リクエスト */
	} else {
#ifdef	yamazaki20170209
		strcat(str,"遠隔 ");
		if (now_status.mode == MODE_MANUAL) {
			EventRequest(REMOTE_REQUEST);/* 遠隔リクエスト */
		}
#else
		if (now_status.mode == MODE_MANUAL) {
			if (now_status.manual_status == 0 ) {
				/* 手動状態 0:SWによる手動,1:運用PCからの手動による */
				strcat(str,"遠隔(仕様により拒否) ");	/* 遠隔リクエスト (拒否) yamazaki 20170209 */
				return;
			} else {
				strcat(str,"運用PCからの手動設定後なので遠隔要求を許可");	/* 遠隔リクエスト yamazaki 20170217 */
				EventRequest(REMOTE_REQUEST);/* 遠隔リクエスト */
			}
		}
#endif
		if (com->command.tuujou != 0) {
			strcat(str,"通常 ");
			if (my_tanmatsu_type == CONTA_ADDRESS) {
				printf("***通常リクエスト キャンセル\n");
				//20170210 EventRequest(FAIL2TUUJOU_REQUEST);/* フェイル時の通常への復帰リクエスト */
			} else {
				printf("***制御機Bの通常リクエスト キャンセル\n");
				//20170210 EventRequest(TUUJOU_REQUEST);/* 制御機Bの通常リクエスト */
			}
		} else if (com->command.issou != 0) {
			strcat(str,"一掃 ");
			if (my_tanmatsu_type != CONTA_ADDRESS) {
				printf("***制御機Bの一掃リクエスト\n");
				//20170226 20170210 EventRequest(ISSOU_REQUEST);/* 制御機Bの一掃リクエスト */
				EventRequest(ISSOU_REQUEST);/* 制御機Bの一掃リクエスト */
			}
		} else if (com->command.henni != 0) {
			strcat(str,"変移 ");
			if (my_tanmatsu_type != CONTA_ADDRESS) {
				printf("***制御機Bの変移リクエストキャンセル\n");
				//20170210 EventRequest(HENNI_REQUEST);/* 制御機Bの変移リクエスト */
			}
		} else if (com->command.fail != 0) {
			strcat(str,"フェイル ");
			EventRequest(FAIL_REQUEST);/* 遠隔フェイルリクエスト */
		} else {
			if (now_status.status != STATUS_FAIL) {//20170218 フェイルではない場合のみ
				/* 遠隔でもこのリクエストをする 20170212 */
				EventRequest(REMOTE_REQUEST);/* 遠隔リクエスト */
			}
		}
	}
#else
// ----------------------------------------- out ↑


	if (com->command.shudou != 0) {
		if ((now_status.mode == MODE_MANUAL) && (now_status.manual_status == 0)) {
				/* 手動状態 0:SWによる手動,1:運用PCからの手動による */
			strcat(str,"手動(仕様により拒否) ");	/* 手動リクエスト (拒否) kaji20170209 */
			return;
		} else {
			now_status.manual_status = 1;/* 手動状態 0:SWによる手動,1:運用PCからの手動による */
			strcat(str,"手動 ");
			if (com->command.tuujou != 0) {
				EventRequest(MANUAL_TUUJOU_REQUEST);/* 手動通常リクエスト */
			} else if (com->command.issou != 0) {
				EventRequest(MANUAL_ISSOU_REQUEST);/* 手動一掃リクエスト */
			} else if (com->command.henni != 0) {
				EventRequest(MANUAL_HENNI_REQUEST);/* 手動変移リクエスト */
			} else if (com->command.fail != 0) {
				EventRequest(MANUAL_FAIL_REQUEST);/* 手動フェイルリクエスト */
			}
		}
	} else if (com->command.teishi != 0) {
		strcat(str,"運用停止 ");
		EventRequest(MONITOR_REQUEST);/* 運用停止リクエスト */
	} else if (com->command.yobi2 != 0) {
		strcat(str,"運用停止解除 ");
		EventRequest(MONITOR_RELEASE_REQUEST);/* 運用停止解除リクエスト */
	} else {
		if (now_status.mode == MODE_MANUAL) {
			if (now_status.manual_status == 0 ) {
				/* 手動状態 0:SWによる手動,1:運用PCからの手動による */
				strcat(str,"遠隔(仕様により拒否) ");	/* 遠隔リクエスト (拒否) yamazaki 20170209 */
				return;
			} else {
				strcat(str,"運用PCからの手動設定後なので遠隔要求を許可");	/* 遠隔リクエスト yamazaki 20170217 */
				EventRequest(REMOTE_REQUEST);/* 遠隔リクエスト */
			}
		}
		if (com->command.tuujou != 0) {
			strcat(str,"通常 ");
			if (my_tanmatsu_type == CONTA_ADDRESS) {
				printf("***通常リクエスト キャンセル\n");
				//20170210 EventRequest(FAIL2TUUJOU_REQUEST);/* フェイル時の通常への復帰リクエスト */
			} else {
				printf("***制御機Bの通常リクエスト キャンセル\n");
				//20170210 EventRequest(TUUJOU_REQUEST);/* 制御機Bの通常リクエスト */
			}
		} else if (com->command.issou != 0) {
			strcat(str,"一掃 ");
			if (my_tanmatsu_type != CONTA_ADDRESS) {
				printf("***制御機Bの一掃リクエスト\n");
				//20170226 20170210 EventRequest(ISSOU_REQUEST);/* 制御機Bの一掃リクエスト */
				EventRequest(ISSOU_REQUEST);/* 制御機Bの一掃リクエスト */
			}
		} else if (com->command.henni != 0) {
			strcat(str,"変移 ");
			if (my_tanmatsu_type != CONTA_ADDRESS) {
				printf("***制御機Bの変移リクエストキャンセル\n");
				//20170210 EventRequest(HENNI_REQUEST);/* 制御機Bの変移リクエスト */
			}
		} else if (com->command.fail != 0) {
			strcat(str,"フェイル ");
			EventRequest(FAIL_REQUEST);/* 遠隔フェイルリクエスト */
		} else {
			if (now_status.status != STATUS_FAIL) {//20170218 フェイルではない場合のみ
				/* 遠隔でもこのリクエストをする 20170212 */
				EventRequest(REMOTE_REQUEST);/* 遠隔リクエスト */
			}
		}
	}

	
#endif
	SaveStatus();/* ステータスを保存 */
}

/**
 *	@brief ステータスのセーブ処理
 *         now_status.status変更時は不揮発メモリにセーブする
 *         LEDの状態を切り替える
 *
 *	@retval なし
 */
void SetNowStatus(int status)
{
	if (now_status.status != status) {
		if (status == STATUS_FAIL) {
			/* 変更要求がFAILの場合はその前の状態を保持しておく */
			now_status.before_fail_status = now_status.status;
		}
		now_status.status = status;
		SetNowLEDStatus(status);/* ステータスをLEDに反映する処理 */
		SaveStatus();
	}
}
/**
 *	@brief ステータスをLEDに反映する処理
 *
 *	@retval なし
 */
static void SetNowLEDStatus(int status)
{
		switch (status) {
		case STATUS_P1:
			cont_led_status[LED_TUUJOU] = LED_STATUS_ON;
			cont_led_status[LED_ISSOU] = LED_STATUS_OFF;
			cont_led_status[LED_HENNI] = LED_STATUS_OFF;
			break;
		case STATUS_P2:
			cont_led_status[LED_TUUJOU] = LED_STATUS_OFF;
			cont_led_status[LED_ISSOU] = LED_STATUS_ON;
			cont_led_status[LED_HENNI] = LED_STATUS_OFF;
			break;
		case STATUS_P3:
			cont_led_status[LED_TUUJOU] = LED_STATUS_OFF;
			cont_led_status[LED_ISSOU] = LED_STATUS_OFF;
			cont_led_status[LED_HENNI] = LED_STATUS_ON;
			break;
		case STATUS_FAIL:
			cont_led_status[LED_TUUJOU] = LED_STATUS_OFF;
			cont_led_status[LED_ISSOU] = LED_STATUS_ON;	// yamazaki 20170209
			cont_led_status[LED_HENNI] = LED_STATUS_OFF;
			break;
		};
}

/**
 *	@brief ステータスをLEDに反映する処理
 *         LEDを点滅状態にする
 *
 *	@retval なし
 */
static void SetNowLEDStatusToggle(int status)
{
		switch (status) {
		case STATUS_P1:
			cont_led_status[LED_TUUJOU] = LED_STATUS_TOGGLE;
			cont_led_status[LED_ISSOU] = LED_STATUS_OFF;
			cont_led_status[LED_HENNI] = LED_STATUS_OFF;
			break;
		case STATUS_P2:
			cont_led_status[LED_TUUJOU] = LED_STATUS_OFF;
			cont_led_status[LED_ISSOU] = LED_STATUS_TOGGLE;
			cont_led_status[LED_HENNI] = LED_STATUS_OFF;
			break;
		case STATUS_P3:
			cont_led_status[LED_TUUJOU] = LED_STATUS_OFF;
			cont_led_status[LED_ISSOU] = LED_STATUS_OFF;
			cont_led_status[LED_HENNI] = LED_STATUS_TOGGLE;
			break;
		case STATUS_FAIL:
			cont_led_status[LED_TUUJOU] = LED_STATUS_OFF;
#ifdef kajikaji20170208
			cont_led_status[LED_ISSOU] = LED_STATUS_OFF;
#else
			cont_led_status[LED_ISSOU] = LED_STATUS_TOGGLE;
#endif
			cont_led_status[LED_HENNI] = LED_STATUS_OFF;
			break;
		};
}
/**
 *	@brief ステータスのロード処理
 *
 *	@retval なし
 */
#define STATUS_FILE_NAME ("status.ini")
#define PARAM_FILE_NAME ("param.ini")
int LoadStatus(void)
{
#ifdef windows	
	FILE *fp;
	char fname[256];

	sprintf(fname,"%s",STATUS_FILE_NAME);
	fp = fopen(fname, "rb");
    if (fp == NULL) {
		printf("%s:%s is not opened!\n",__func__,fname);
		return -1;
    }
	STATUS tmp_status;
	STATUS *p = &tmp_status;
	fread(p, sizeof (STATUS), 1, fp);
	now_status.mode = p->mode;
	now_status.status = p->status;
	now_status.before_fail_status = p->before_fail_status;
	now_status.power_outage_flag = p->power_outage_flag;
	now_status.power_outage_flag2 = p->power_outage_flag2;
	fclose(fp);
#else
	/* ここでFlashROMからステータスをロードする */
	STATUS *p = (STATUS *)STATUS_ADDRESS;
	int bcc = CalcBcc((char *)p,sizeof(STATUS) - sizeof(int));
	if (bcc == p->bcc) {
		printf("now_status load success bcc=%X\n",bcc);
		now_status.mode = p->mode;
		now_status.status = p->status;
		now_status.power_outage_flag = p->power_outage_flag;
		now_status.start_time = p->start_time;/* 20170224 通常→一掃開始時間 */
		now_status.end_time = p->end_time;/* 20170224 変移→一掃開始時間 */
		now_status.offset_timer = p->offset_timer;/* 20170224 オフセットタイマ値 */
		
// kaji20170223 端末エラーresumeされると訳がわからなくなってきたので一時取りやめ
//		startup_error_status = p->tanmatu_error;/* 2017022 起動時のエラーの有無を保持(0:エラー無し、1:エラー有り) */
/* kaji20170301 端末エラーresumeの考え方止め？↓
		if (p->musen_status != 0) {
			cont_led_status[LED_MUSEN] = LED_STATUS_TOGGLE;
			printf("起動時LED_MUSEN TOGGLE\n" ) ;// kaji20170223
		}
		if (p->board_error != 0) {
			cont_led_status[LED_BOARD] = LED_STATUS_TOGGLE;
			printf("起動時LED_BOARD TOGGLE\n" ) ;// kaji20170223
		}
		if (p->byou_status != 0) {
			cont_led_status[LED_BYOU] = LED_STATUS_TOGGLE;
			printf("起動時LED_BYOU TOGGLE\n" ) ;// kaji20170223
		}
  kaji20170301 端末エラーresumeの考え方止め？↑*/
	} else {
		printf("************now_status load fail p->bcc=%.08X %.08X\n", p->bcc,bcc ) ;
	}
#endif
	if (now_status.mode == MODE_MONITOR) {
		now_status.mode = MODE_REMOTE;
		printf("LoadStatus　now_status 運用から遠隔に変更\n" ) ;
	}
	
	if (now_status.mode == MODE_REMOTE) {
		if (now_status.status == STATUS_P2) {
			SetNowStatus(STATUS_P1);/* 通常 */;
		} else if (now_status.status == STATUS_FAIL) {
			SetNowStatus(STATUS_P1);/* 通常 */;
		}
	} else if (now_status.mode == MODE_MANUAL) {
		if (now_status.status == STATUS_FAIL) {
			now_status.status = now_status.before_fail_status;
			printf("LoadStatus　now_status STATUS_FAILなのでbefore_fail_statusに変更 %d\n", now_status.status) ;
		}
		sw = 0;
		sw |= SW_STATUS_BIT;
		if (now_status.status == STATUS_P1) {
			sw |= SW_TUUJOU_BIT;
		} else if (now_status.status == STATUS_P2) {
			sw |= SW_ISSOU_BIT;
		} else {
			sw |= SW_HENNI_BIT;
		}
		bef_sw = sw;
		swtest = sw;
	}
	StatusDisp();/* ステータスの表示処理 */
	return 0;
}

/**
 *	@brief ステータスのセーブ処理
 *
 *	@retval なし
 */
void SaveStatus(void)
{
#ifdef windows	

	FILE *fp;
	char fname[256];

	sprintf(fname,"%s",STATUS_FILE_NAME);
	fp = fopen(fname, "wb");
    if (fp == NULL) {
		printf("%s:%s is not opened!\n",__func__,fname);
    }
	fwrite(&now_status, sizeof (STATUS), 1, fp);
	fclose(fp);

#else
	/* ここでFlashROMのステータスをセーブする */
	int bcc = CalcBcc((char *)&now_status,sizeof(STATUS) - sizeof(int));
	now_status.bcc = bcc;
#if 1
	flash_sector_erase(STATUS_ADDRESS);
	flash_write_buf(STATUS_ADDRESS, (int)&now_status, sizeof(STATUS)/2);
#else
	extern int flash_modify_APP_Tasks_init ( int *p ,int size);
	extern int flash_modify_APP_Tasks(void);
    int ret;
	ret = flash_modify_APP_Tasks_init((int *)&now_status, sizeof(STATUS));
	if (ret <0) {
		printf("%s:init error!\n","SaveStatus");
		return;
	}
    while(1) {
    	ret = flash_modify_APP_Tasks();
        if (ret == 1) {
            //printf("now_status OK\r\n");
            break;
        } else if (ret ==0) {
            //printf("now_status NG\r\n");
            break;
        }
	}
#endif
#endif
}
/**
 *	@brief 不揮発メモリ上パラメータのロード処理
 *
 *	@retval なし
 */
void LoadParam(void)
{
#ifdef windows	
	FILE *fp;
	char fname[256];

	sprintf(fname,"%s",PARAM_FILE_NAME);
	fp = fopen(fname, "rb");
    if (fp == NULL) {
		printf("%s:%s is not opened!\n",__func__,fname);
		return;
    }
	PARAM *p = &param;
	fread(p, sizeof (PARAM), 1, fp);
	fclose(fp);
#else
	PARAM *p = (PARAM *)PARAM_ADDRESS;
	int bcc = CalcBcc((char *)p,sizeof(PARAM) - sizeof(int));
	if (bcc == p->bcc) {
		printf("param load success bcc=%X\n",bcc);
		memmove(&param, p , sizeof(PARAM) );
	} else {
		printf("*********************param load fail p->bcc=%.08X %.08X\n", p->bcc,bcc ) ;
	}
#endif
}

/**
 *	@brief 不揮発メモリ上へのパラメータのセーブ処理
 *
 *	@retval なし
 */
void SaveParam(void)
{
#ifdef windows	

	FILE *fp;
	char fname[256];

	sprintf(fname,"%s",PARAM_FILE_NAME);
	fp = fopen(fname, "wb");
    if (fp == NULL) {
		printf("%s:%s is not opened!\n",__func__,fname);
    }
	fwrite(&param, sizeof (PARAM), 1, fp);
	fclose(fp);

#else
	int bcc = CalcBcc((char *)&param,sizeof(PARAM) - sizeof(int));
	param.bcc = bcc;
#if 1
	flash_sector_erase(PARAM_ADDRESS);
	flash_write_buf(PARAM_ADDRESS, (int)&param, sizeof(PARAM)/2);
#else
	extern int flash_modify_APP_Tasks_init2 ( int *p ,int size);
	extern int flash_modify_APP_Tasks2(void);
    int ret;
//	flash_write_status_init();
	ret = flash_modify_APP_Tasks_init((int *)&param, sizeof(PARAM) );
	if (ret <0) {
		printf("%s: init error!\n","SaveParam");
		return;
	}
    while(1) {
//    	ret = flash_write_status((int *)&now_status);
    	ret = flash_modify_APP_Tasks2();
        if (ret == 1) {
            //printf("param OK\n");
            break;
        } else if (ret ==0) {
            //printf("param NG\n");
            break;
        }
	}
#endif
#endif
}

/**
 *	@brief ディフォルトパラメータ設定処理
 *
 *	@retval なし
 */
void SetDefault(void)
{
	
	param.start_time = DEFAULT_START_TIME; /* 変移開始時間 */
	param.end_time = DEFAULT_END_TIME; /* 変移終了時間 */
	param.time_correction_time = DEFAULT_TIME_CORRECTION_TIME; /* 時刻修正時刻 */
	param.time_correction_request_time = DEFAULT_TIME_CORRECTION_REQUEST_TIME; /* 時刻修正要求時刻 */
	param.holiday_pattern = DEFAULT_HOLIDAY_DISPLAY_PATTERN; /* 日・休表示パターン P3 */
	param.holiday_mode = DEFAULT_HOLIDAY_MODE; /* なし */
	param.holiday_start_time =DEFAULT_HOLIDAY_START_TIME ; /* なし　日・休開始時 */
	param.holiday_end_time = DEFAULT_HOLIDAY_END_TIME; /* なし　日・休終了時 */
	param.saturday_mode = DEFAULT_SATURDAY_MODE; /* 変移あり（月～金と同じ） */
	param.holiday_mode = DEFAULT_HOLIDAY_MODE; /* 変移なし（通常） */
	param.fail_pattern = DEFAULT_FAIL_PATTERN; /* 可変表示板 P2 */
	param.offset_timer = DEFAULT_OFFSET_TIMER;/* オフセットタイマ */
	param.no_musen_error_flag = 0; /* 無線異常をチェックするや否や 0:する,1:しない */
	param.same_nomove_flag = 0; /* 同じ場合は可変表示版を変化させないモード */
	param.no_fail_flag = 0; /* フェイルにするや否や 0:する,1:しない */
	param.linkage_status = DEFAULT_LINKAGE; /* 連動設定 */
	param.no_board_error_flag = 0; /* 標識版異常をチェックするや否や 標識版番号のビット位置 0:する,1:しない */
	param.no_pc_check_flag =0; /* 運用管理ＰＣ間通信異常を判定をチェックするや否や 0:する,1:しない */
	param.debug_flg = 0xf;/* デバッグ用の表示を行う場合に0以外をセットする */
	param.mdmcs_delay_time = 20;/* MDM_CS出力の遅延時間(ms) */
	param.reset_count = 0;
	param.response_interval_time_val = RESPONSE_INTERVAL_TIME_VAL;/* 20170305 制御機Aからの要求から制御機Bからの応答受信確認までの最小待ち時間(ms) */
	param.response_timeout_val = RESPONSE_TIMEOUT_VAL;/* 20170305 制御機Aからの要求から制御機Bからの応答受信までのタイムアウト値(ms) */
	param.musen_timeout_val = MUSEN_TIMEOUT_VAL;/* 20170305 無線通信の受信エラータイムアウト値(秒) */
	param.preamble_ptn = PREAMBLE_PTN;/* 20170308 L2.cで使用するプリアンブルパターン */

	now_status.mode = MODE_REMOTE;/* 遠隔 */
	now_status.status = STATUS_P1;/* 通常 */
	now_status.bef_status = STATUS_P1;/* 通常 */
	now_status.before_fail_status = STATUS_P1;/* 通常 */
	now_status.schedule = 0;
	now_status.start_time = 0;
	now_status.end_time = 0;
	now_status.offset_timer = DEFAULT_OFFSET_TIMER;
	now_status.power_outage_flag = 0;/* 停電発生フラグ */
	now_status.power_outage_flag2 = 0;/* 停電発生フラグ2 */
	now_status.power_outage_move_flag = 0;/* 停電発生でフェイルへ移動したフラグ */
	now_status.test_flag = 0;/* テスト中フラグ */
	now_status.board_error = 0;/* 表示板異常フラグ */
	now_status.time_status = 0;/* 時刻状態 0:正常,1:異常 */
	now_status.gps_status = 0;/* GPS状態 */
	now_status.hoshu_status = 0;/* 1:保守/0:通常 */
	now_status.keikoutou_status = 0;/* 1:蛍光灯入/0:自動 */
	now_status.before_gps_status = 0;/* 前回のGPS状態 */
	now_status.musen_status = 0;/* 無線通信状態 */
	now_status.before_musen_status = 0;/* 前回の無線通信状態 */
	now_status.byou_status = 0;/* 発光鋲状態 */
	now_status.before_byou_status = 0;/* 前回の発光鋲状態 */
	now_status.byou1_status = 0;/* 発光鋲1状態 */
	now_status.byou2_status = 0;/* 発光鋲2状態 */
	now_status.pc_tuushin_status = 0;/* 運用管理PC間通信状態 */
	now_status.moniter_tuushin_status = 0;/* 監視盤間通信状態 */
	now_status.tanmatu_error = 0;/* 端末エラー状態を保持 0:エラー無し,1:エラー有り */
	
}

/**
 *	@brief 現在のエラー状態をnow_statusから判定する処理
 *
 *	@retval エラー状態:1,エラー無し状態:0
 */
int CheckErrorStatus(void)
{
	int ret = 0;
	if (now_status.board_error != 0) {
		ret = 1;
	}
	if (now_status.musen_status != 0) {
		ret = 1;
	}
	if (now_status.byou_status != 0) {
		ret = 1;
	}
	if (param.no_fail_flag == 1) {/* この場合はデバッグ用にフェイルにはしない */
		/* フェイルだけどフェイルにしない */
		ret = 0;
	}
	return ret;
}

/**
 *	@brief ステータスの表示処理
 *
 *	@retval なし
 */
void StatusDisp(void)
{
	int i;
	
	static char mode_str[][16] = { "遠隔", "手動", "監視盤指令"};
	static char status_str[][16] = { "無し", "通常", "一掃", "変移", "日・休", "フェイル"};
	
	if (my_tanmatsu_type == CONTA_ADDRESS) {
		/* 制御機A */
		printf("制御機A\n");
	} else if (my_tanmatsu_type == 0x10) {
		/* 監視盤 */
		printf("監視盤\n");
	} else {
		/* 制御機B */
		printf("制御機B %d\n", my_tanmatsu_type);
	}
	printf("モードステータス       = %s,%s\n",mode_str[now_status.mode],status_str[now_status.status]);
	printf("変移開始終了時間,タイマ= %.02d:%.02d %.02d:%.02d %d\n"
		, (now_status.start_time/60) / 60, (now_status.start_time/60) % 60//yamazaki
		, (now_status.end_time/60) / 60, (now_status.end_time/60) % 60//yamazaki
		, now_status.offset_timer);
	printf("スケジュール,時刻設定  = %d,%d\n", now_status.schedule, now_status.time_req);
	printf("停電,test,brd_err,time,gps,mus,pc,mon,cds,b1,b2,ms= %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n"
		,now_status.power_outage_flag
		,now_status.test_flag
		,now_status.board_error
		,now_status.time_status
		,now_status.gps_status
		,now_status.musen_status
		,now_status.pc_tuushin_status
		,now_status.moniter_tuushin_status
		,now_status.cds_status
		,now_status.byou1_status
		,now_status.byou2_status
		,now_status.manual_status
	);
	printf("offset_value,rstage,lap,qsize=%d,%d,%d,%d,%d\n"
		,offset_time_up_value
		,board_rotate_stage_no
		,lap_time_max
		,io_control_timer/1000
		,lenqueue(IO_QUEUE) );
	printf("input_data     :");
	for ( i = 0 ; i < display_board_count; i++) {
		printf("%.02X ",input_data[i]);
	}
	printf("\n");
	printf("output_data    :");
	for ( i = 0 ; i < display_board_count; i++) {
		printf("%.02X ",output_data[i]);
	}
	printf("\n");
	printf("board_reg_buf  :");
	for ( i = 0 ; i < display_board_count; i++) {
		printf("%.02X ",board_reg_buf[i]);
	}
	printf("\n");
	printf("board_status   :");
	for ( i = 0 ; i < display_board_count; i++) {
		printf("%.02X ",board_status[i]);
	}
	printf("\n");
	if (response) {
		printf("response_status:");
		for ( i = 0 ; i < 7; i++) {
			printf("%.02X ",response->response.status[i]);
		}
		printf("\n");
	}
	extern int pcpower_stage_no;
	printf("pcpower_stage_no=%d\n",pcpower_stage_no);

	if (my_tanmatsu_type == CONTA_ADDRESS) {
		printf("response_received_count ");
		for ( i = 0 ; i < CONTROLER_MAX; i++) {
			printf("%d:%d ", i + 1, response_received_count[i]);
		}
		printf("\n");
		printf("response_total_err_count ");/* kaji20170305 */
		for ( i = 0 ; i < CONTROLER_MAX; i++) {
			printf("%d:%d ", i + 1, response_total_err_count[i]);
		}
		printf("\n");
		printf("response_time ");
		for ( i = 0 ; i < CONTROLER_MAX; i++) {
			printf("%d:%d ", i + 1, response_time[i]);
		}
		printf("\n");
	}

}

/**
 *	@brief 標識板が無地かどうかの判定処理
 *
	*	@param [int no]  標識板番号(0-7)
 *
 *	@retval 0:無地ではない,1:無地である
 */
int CheckMuji(int no)
{
	int mj;
	if (now_status.status == STATUS_P1) {
		mj = my_io_info.muji_pattern_p1[no];
	} else if (now_status.status == STATUS_P2) {
		mj = my_io_info.muji_pattern_p2[no];
	}else if (now_status.status == STATUS_P3) {
		mj = my_io_info.muji_pattern_p3[no];
	}else {
		mj = my_io_info.muji_pattern_p2[no];
	}
//printf("***mj=%d\n",mj);
	return mj;
}


/**
 *	@brief (停電起動時に)可変表示板の多数決を取る処理
 *
 *	@param なし
 *
 *	@retval -1: なし  P1,P2,P3: パターン
 */
static int GetStartupBoardPattern(void)
{
	int	i, v, c, p;
	int ptn[3];
	
	ptn[0] = 0;
	ptn[1] = 0;
	ptn[2] = 0;
	for ( i = 0 ; i < display_board_count; i++) {
		v = BoardRead(i);
		switch (v) {/* 停止位置に居ない場合は無効とする */
			case P1:
				ptn[0]++;
				break;
			case P2:
				ptn[1]++;
				break;
			case P3:
				ptn[2]++;
		}
	}
	
	c = ptn[0];			p = P1;
	if ( ptn[1] > c ) {
		c = ptn[1];		p = P2;
	}
	if ( ptn[2] > v ) {
		c = ptn[2];		p = P3;
	}
	
	if ( (c*2) <= display_board_count ) {// 過半数を超えていること(半々は×)
		p = -1;
	}
	
	printf("***Read Board Pattern=%d\n", p);
	return p;
}


