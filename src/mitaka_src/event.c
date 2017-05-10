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

/*
 *===========================================================================================
 *					外部変数定義
 *===========================================================================================
 */

extern STATUS now_status;/* 現在の設定値 */
extern int board_rotate_stage_no;

/*
 *===========================================================================================
 *					内部	関数定義
 *===========================================================================================
 */


static void EventMonitorRequest(void);/* 運用停止リクエストによる状態変更要求処理 */
static void EventMonitorReleaseRequest(void);/* 運用停止解除リクエストによる状態変更要求処理 */
static void EventManualTuujouRequest(void);/* 手動通常リクエストによる状態変更要求処理 */
static void EventManualIssouRequest(void);/* 手動一掃リクエストによる状態変更要求処理 */
static void EventManualHenniRequest(void);/* 手動変移リクエストによる状態変更要求処理 */
static int EventSWManualTuujouRequest(void);/* SW手動通常リクエストによる状態変更要求処理 */
static int EventSWManualIssouRequest(void);/* SW手動一掃リクエストによる状態変更要求処理 */
static int EventSWManualHenniRequest(void);/* SW手動変移リクエストによる状態変更要求処理 */
static int EventManualFailRequest(void);/* 手動フェイルリクエストによる状態変更要求処理 */
static void EventRemoteTuujouRequest(void);/* 通常時間内での手動→遠隔リクエストによる状態変更要求処理 */
static void EventRemoteHenniRequest(void);/* 変移時間内での手動→遠隔リクエストによる状態変更要求処理 */
static void EventRemoteTuujou2HenniRequest(void);/* 遠隔時変移時間開始リクエストによる状態変更要求処理 */
static void EventRemoteHenni2TuujouRequest(void);/* 遠隔時通常時間開始リクエストによる状態変更要求処理 */

/*
 *===========================================================================================
 *					グローバル関数定義
 *===========================================================================================
 */



void EventRequest(int event);/* 状態変更要求処理 */

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
 *	@brief 状態変更要求処理
 *
 *	@param [int event]  リスエストイベント
 *
 *	@retval なし
 */
static char event_pattern [][100] = {/* デバッグ用の表示パターン */
	"遠隔リクエスト",
	"通常時間内での遠隔リクエスト",
	"変移時間内での遠隔リクエスト",
	"遠隔通常から変移リクエスト",
	"遠隔変移から通常リクエスト",
	"手動通常リクエスト",
	"手動一掃リクエスト",
	"手動変移リクエスト",
	"SW手動通常リクエスト",
	"SW手動一掃リクエスト",
	"SW手動変移リクエスト",
	"手動フェイルリクエスト",
	"運用停止リクエスト",
	"運用停止解除リクエスト",
	"フェイル異常（時計、GPS、board）リクエスト",
	"停電発生リクエスト",
	"停電発生復帰リクエスト",
	"フェイル異常復帰（時計、GPS）リクエスト",
	"フェイル時の通常への復帰リクエスト",
	"制御機Bの通常リクエスト",
	"制御機Bの一掃リクエスト",
	"制御機Bの変移リクエスト",
};

/*
 *===========================================================================================
 *					グローバル関数
 *===========================================================================================
 */

void EventRequest(int event)
{
	int st;
	char str[256];
	
	//if (event != FAIL_REQUEST) {
	if ((event != FAIL_REQUEST) && (event != REMOTE_REQUEST)){
		/* フェイルリクエストは常に処理する */
		if (!empty(IO_QUEUE)) {
			/* 実行中または要求有り */
			sprintf(str, "他の要求有りのため EventRequest event = %s キャンセル",event_pattern[event]);
			DebugPrint("", str, 1);
			return;
		}
		if ((board_rotate_stage_no != BOARD_ROTATE_END) && (board_rotate_stage_no != BOARD_ROTATE_IDLE) ) {
			/* 実行中または要求有り */
			sprintf(str, "動作中 EventRequest event = %s キャンセル",event_pattern[event]);
			DebugPrint("", str, 1);
 			return;
		}
		sprintf(str, "EventRequest event = %s",event_pattern[event]);
		DebugPrint("", str, 1);
	}
	if (event == REMOTE_REQUEST) {
		/* 同報指令またはSWで手動から遠隔に変わった場合 */
		st = GetNowRemoteStatus();
//printf("*****st=%d\n",st);
		if (st == STATUS_P1) {
			event = REMOTE_TUUJOU_REQUEST;/* 通常時間内での手動→遠隔リクエスト */
		} else if (st == STATUS_P1P2) {
			event = REMOTE_TUUJOU_REQUEST;/* 通常時間内での手動→遠隔リクエスト */
		} else if (st == STATUS_P3P2) {
			event = REMOTE_TUUJOU_REQUEST;/* 通常時間内での手動→遠隔リクエスト */
		} else {
			event = REMOTE_HENNI_REQUEST;/* 変移時間内での手動→遠隔リクエスト */
		}
	}

	switch (event) {
	case MONITOR_REQUEST:/* 運用停止リクエスト */
		sprintf(str, "運用停止リクエスト");
		EventMonitorRequest();
		break;
	case MONITOR_RELEASE_REQUEST:/* 運用停止解除リクエスト */
		sprintf(str, "運用停止解除リクエスト");
		EventMonitorReleaseRequest();/* kaji20170308 */
		break;
		
	case MANUAL_TUUJOU_REQUEST:/* 手動通常リクエスト */
		sprintf(str, "手動通常リクエスト");
		EventManualTuujouRequest();
		break;
		
	case MANUAL_ISSOU_REQUEST:/* 手動一掃リクエスト */
		sprintf(str, "手動一掃リクエスト");
		EventManualIssouRequest();
		break;

	case MANUAL_HENNI_REQUEST:/* 手動変移リクエスト */
		sprintf(str, "手動変移リクエスト");
		EventManualHenniRequest();
		break;

	case SW_MANUAL_TUUJOU_REQUEST:/* SW手動通常リクエスト */
		sprintf(str, "SW手動通常リクエスト");
		if (!EventSWManualTuujouRequest()) {
			strcat(str, " キャンセル");
		}
		break;
		
	case SW_MANUAL_ISSOU_REQUEST:/* SW手動一掃リクエスト */
		sprintf(str, "SW手動一掃リクエスト");
		if (!EventSWManualIssouRequest()) {
			strcat(str, " キャンセル");
		}
		break;
		
	case SW_MANUAL_HENNI_REQUEST:/* SW手動変移リクエスト */
		sprintf(str, "SW手動変移リクエスト");
		if (!EventSWManualHenniRequest()) {
			strcat(str, " キャンセル");
		}
		break;
	case MANUAL_FAIL_REQUEST:/* SW手動変移リクエスト */
		sprintf(str, "手動フェイルリクエスト");
		if (!EventManualFailRequest()) {
			strcat(str, " キャンセル");
		}
		break;

	case REMOTE_TUUJOU_REQUEST:/* 通常時間内での手動→遠隔リクエスト */
		sprintf(str, "通常時間内での手動→遠隔リクエスト");
		EventRemoteTuujouRequest();
		break;
	case REMOTE_HENNI_REQUEST:/* 変移時間内での手動→遠隔リクエスト */
		sprintf(str, "変移時間内での手動→遠隔リクエスト");
		EventRemoteHenniRequest();
		break;
		
		
	case REMOTE_TUUJOU2HENNI_REQUEST:/* 遠隔時変移時間開始リクエスト */
		sprintf(str, "遠隔時変移時間開始リクエスト");
		EventRemoteTuujou2HenniRequest();
		break;
	case REMOTE_HENNI2TUUJOU_REQUEST:/* 遠隔時通常時間開始リクエスト */
		sprintf(str, "遠隔時通常時間開始リクエスト");
		EventRemoteHenni2TuujouRequest();
		break;
		
	case FAIL_REQUEST:/* フェイルリクエスト */
		if (now_status.mode == MODE_REMOTE) {
			st = GetNowRemoteStatus();
//			if (st == STATUS_P3) {
//			if ((st == STATUS_P2)||(st == STATUS_P3)||(st == STATUS_P3P2)) {/* kaji20170225 */
			if ((st == STATUS_P1P2)||(st == STATUS_P2)||(st == STATUS_P3)||(st == STATUS_P3P2)) {/* kaji20170227 */
				strcpy(str, "");
				//if (now_status.gps_status == 1) {
				//	strcat(str, "時計異常");
				//}
				if (now_status.musen_status == 1) {
					strcat(str, "無線異常");
				}
				if (now_status.byou_status == 1) {
					strcat(str, "発光鋲異常");
				}
				if (now_status.board_error == 1) {
					strcat(str, "BOARD異常");
				}
				strcat(str, "リクエスト");
				if (now_status.status != STATUS_FAIL) {
					strcat(str, " フェイルへ移動");
					IO_ChangeReq(PX_FAIL);
				} else {
					strcat(str, " でもすでにフェイル");
				}
			} else {
				strcpy(str, "遠隔一掃/遠隔変移じゃないので");
				//if (now_status.gps_status == 1) {
				//	strcat(str, "時計異常");
				//}
				if (now_status.musen_status == 1) {
					strcat(str, "無線異常");
				}
				if (now_status.byou_status == 1) {
					strcat(str, "発光鋲異常");
				}
				if (now_status.board_error == 1) {
					strcat(str, "BOARD異常");
				}
				strcat(str, "リクエスト キャンセル");
			}
		} else {
			strcpy(str, "遠隔じゃないので");
			//if (now_status.gps_status == 1) {
			//	strcat(str, "時計異常");
			//}
			if (now_status.musen_status == 1) {
				strcat(str, "無線異常");
			}
			if (now_status.byou_status == 1) {
				strcat(str, "発光鋲異常");
			}
			if (now_status.board_error == 1) {
				strcat(str, "BOARD異常");
			}
			strcat(str, "リクエスト キャンセル");
		}
		break;
	case FAIL2TUUJOU_REQUEST:/* フェイル時の通常への復帰リクエスト */
		sprintf(str, "フェイル時の通常への復帰リクエスト");
		now_status.mode = MODE_REMOTE;
		if (now_status.status == STATUS_FAIL) {
			IO_ChangeReq(PX_P1);
		}
		break;
	case FAIL_RECOVER_REQUEST:/* 時計異常復帰リクエスト */
		sprintf(str, "フェイル異常復帰（時計、無線）リクエスト");
		//now_status.mode = MODE_MANUAL;
		if (now_status.status == STATUS_FAIL) {
			switch (now_status.mode) {
				case MODE_REMOTE: /* 遠隔 */
					IO_ChangeReq(GetNowRemoteStatus());/* 現在の遠隔の状態にする */
					break;
				case MODE_MANUAL: /* 手動 */
					IO_ChangeReq(now_status.before_fail_status);
					break;
				case MODE_MONITOR: /* 監視盤 */
					IO_ChangeReq(STATUS_P1);/* 通常に戻る */
					break;
			}
		}
		break;

	case POWER_OFF_REQUEST:/* 停電発生リクエスト */
		sprintf(str, "停電発生リクエスト");
		switch (now_status.mode) {
			case MODE_REMOTE: /* 遠隔 */
				switch (now_status.status) {
					case STATUS_P3:/* 変移 */
						init_queue(IO_QUEUE);/* 緊急事態なのでキューを空にする */
						IO_ChangeReq(PX_POWEROFF);
						now_status.power_outage_move_flag = 1;/* 停電発生でフェイルへ移動したフラグ */
						break;
					default:
						strcat(str, " 変移ではないので何もしない");
						break;
				}
				break;
			case MODE_MANUAL: /*  */
				switch (now_status.status) {
					case STATUS_P3:/* 変移 */
						init_queue(IO_QUEUE);/* 緊急事態なのでキューを空にする */
						IO_ChangeReq(PX_POWEROFF);
						now_status.power_outage_move_flag = 1;/* 停電発生でフェイルへ移動したフラグ */
						break;
					default:
						strcat(str, " 変移ではないので何もしない");
						break;
				}
				break;
			default:
				strcat(str, " 何もしない");
				break;
		}
		//20170207 PcPower(1);/* PCPOWERの出力処理 */
		break;
	case POWER_RECOVER_REQUEST:/* 停電発生復帰リクエスト */
		printf("**POWER_RECOVER_REQUEST\n");
		sprintf(str, "停電発生復帰リクエスト");
		/* ここで調光を戻す */
		//if (now_status.power_outage_move_flag == 1) {
		if (1) {
		printf("2**POWER_RECOVER_REQUEST %d\n",power_off_bef_status);
			power_off_bef_status = GetNowRemoteStatus();
		printf("2**POWER_RECOVER_REQUEST %d\n",power_off_bef_status);
			switch (power_off_bef_status) {
				case STATUS_P1:/* 通常 */
				case STATUS_P3P2:/* 20170225 変移からの変化時の一掃 */
					if (param.fail_pattern != STATUS_P1) {
						IO_ChangeReq(PX_P1);
					}
					break;
				case STATUS_P2:/* 一掃 */
					if (param.fail_pattern != STATUS_P2) {
						IO_ChangeReq(PX_P2);
					}
					break;
				case STATUS_P3:/* 変移 */
				case STATUS_P1P2:/* 20170225 通常からの変化時の一掃 */
					if (param.fail_pattern != STATUS_P3) {
						IO_ChangeReq(P2_P2);/* 一掃を経由する 20170224 */
						IO_ChangeReq(PX_P3);
					}
					break;
				default:
					break;
			}
			now_status.power_outage_move_flag = 0;/* 停電発生でフェイルへ移動したフラグクリア */
		} else {
			strcat(str, " キャンセル");
		}
		//20170207 PcPower(0);/* PCPOWERの出力処理 */
		break;
	case TUUJOU_REQUEST:/* 制御機Bの通常リクエスト */
		if (now_status.mode == MODE_REMOTE) {
			sprintf(str, "制御機Bの通常リクエスト");
			IO_ChangeReq(PX_P1);
		} else {
			sprintf(str, "制御機Bの通常リクエスト　キャンセル");
		}
		break;
	case ISSOU_REQUEST:/* 制御機Bの一掃リクエスト */
		if (now_status.mode == MODE_REMOTE) {
			sprintf(str, "制御機Bの一掃リクエスト");
			IO_ChangeReq(PX_P2);
		} else {
			sprintf(str, "制御機Bの一掃リクエスト　キャンセル");
		}
		break;
	case HENNI_REQUEST:/* 制御機Bの変移リクエスト */
		if (now_status.mode == MODE_REMOTE) {
			sprintf(str, "制御機Bの変移リクエスト");
			IO_ChangeReq(PX_P3);
		} else {
			sprintf(str, "制御機Bの変移リクエスト　キャンセル");
		}
		break;
	case CONTB_FAIL_REQUEST:/* 制御機Bのフェイルリクエスト */
		if (now_status.mode == MODE_REMOTE) {
			sprintf(str, "制御機Bのフェイルリクエスト");
			IO_ChangeReq(PX_FAIL);
		} else {
			sprintf(str, "制御機Bのフェイルリクエスト　キャンセル");
		}
		break;
	}
	DebugPrint("", str, 1);
	
}

/*
 *===========================================================================================
 *					内部関数
 *===========================================================================================
 */

/**
 *	@brief 運用停止リクエストによる状態変更要求処理
 *
 *	@retval なし
 */
static void EventMonitorRequest(void)
{
	switch (now_status.mode) {
		case MODE_REMOTE: /* 遠隔 */
		case MODE_MONITOR: /* 監視盤からの指令モード */
			switch (now_status.status) {
				case STATUS_P2:/* 一掃 */
				case STATUS_FAIL:/* フェイルならば */
					IO_ChangeReq(PX_P1);/* 通常に遷移 */
					break;
				case STATUS_P3:/* 変移 */
					IO_ChangeReq(PX_P2);/* 一掃に移動 */
					IO_ChangeReq(P2_P2);/* オフセットタイマ */
					IO_ChangeReq(PX_P1);/* 通常に遷移 */
					break;
				default:
					break;
			}
			break;
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P2:/* 一掃 */
				case STATUS_FAIL:/* フェイルならば */
					IO_ChangeReq(PX_P1);
					break;
				case STATUS_P3:/* 変移 */
					IO_ChangeReq(PX_P2);/* 一掃に移動 */
					IO_ChangeReq(P2_P2);/* オフセットタイマ */
					IO_ChangeReq(PX_P1);
					break;
				default:
					break;
			}
			break;
	}
	now_status.mode = MODE_MONITOR;
}

/**
 *	@brief 運用停止解除リクエストによる状態変更要求処理
 *
 *	@retval なし
 */
static void EventMonitorReleaseRequest(void)
{
	switch (now_status.status) {
		case STATUS_P1:/* 通常ならば */
			IO_ChangeReq(PX_P1);/* 通常に遷移 */
			break;
		case STATUS_P2:/* 一掃 */
		case STATUS_FAIL:/* フェイルならば */
			IO_ChangeReq(P2_P2);/* オフセットタイマ */
			break;
		case STATUS_P3:/* 変移 */
			IO_ChangeReq(PX_P2);/* 一掃に移動 */
			IO_ChangeReq(P2_P2);/* オフセットタイマ */
			break;
		default:
			break;
	}
	now_status.mode = MODE_REMOTE;
}

/**
 *	@brief 手動通常リクエストによる状態変更要求処理
 *
 *	@retval なし
 */
static void EventManualTuujouRequest(void)
{
	switch (now_status.mode) {
		case MODE_REMOTE: /* 遠隔 */
			switch (now_status.status) {
				case STATUS_P1:/* 通常 */
					now_status.mode = MODE_MANUAL;
					break;
				case STATUS_P3:/* 変移 */
					IO_ChangeReq(PX_P2);/* 一掃に移動 */
					IO_ChangeReq(P2_P2);/* オフセットタイマ */
					IO_ChangeReq(PX_P1);/* 通常に遷移 */
					now_status.mode = MODE_MANUAL;
					break;
				default:
					break;
			}
			break;
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P2:/* 一掃 */
				case STATUS_FAIL:/* フェイルならば */
					IO_ChangeReq(PX_P1);
					break;
				case STATUS_P3:/* 変移 */
					IO_ChangeReq(PX_P2);/* 一掃に移動 */
					IO_ChangeReq(P2_P2);/* オフセットタイマ */
					IO_ChangeReq(PX_P1);/* 通常に遷移 */
					break;
				default:
					break;
			}
			break;
	}
}

/**
 *	@brief 手動一掃リクエストによる状態変更要求処理
 *
 *	@retval なし
 */
static void EventManualIssouRequest(void)
{
	switch (now_status.mode) {
		case MODE_REMOTE: /* 遠隔 */
			switch (now_status.status) {
				case STATUS_P1:/* 通常 */
					IO_ChangeReq(PX_P2);
					now_status.mode = MODE_MANUAL;
					break;
				case STATUS_P3:/* 変移 */
					IO_ChangeReq(PX_P2);
					now_status.mode = MODE_MANUAL;
					break;
				default:
					break;
			}
			break;
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P1:/* 通常ならば */
				case STATUS_P3:/* 変移ならば */
				case STATUS_FAIL:/* フェイルならば */
					IO_ChangeReq(PX_P2);/* 一掃へ遷移 */
					break;
				default:
					break;
			}
			break;
	}
}

/**
 *	@brief 手動変移リクエストによる状態変更要求処理
 *
 *	@retval なし
 */
static void EventManualHenniRequest(void)
{
	switch (now_status.mode) {
		case MODE_REMOTE: /* 遠隔 */
			switch (now_status.status) {
				case STATUS_P1:/* 通常 */
					IO_ChangeReq(PX_P2);/* 一掃に移動 */
					IO_ChangeReq(P2_P2);/* オフセットタイマ */
					IO_ChangeReq(PX_P3);/* 変移に移動 */
					now_status.mode = MODE_MANUAL;
					break;
				case STATUS_P3:/* 変移 */
					now_status.mode = MODE_MANUAL;
					break;
				default:
					break;
			}
			break;
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P1:/* 通常ならば */
					IO_ChangeReq(PX_P2);/* 一掃に移動 */
					IO_ChangeReq(P2_P2);/* オフセットタイマ */
					IO_ChangeReq(PX_P3);/* 変移へ遷移 */
					break;
				case STATUS_P2:/* 一掃ならば */
				case STATUS_FAIL:/* フェイルならば */
					IO_ChangeReq(PX_P3);/* 変移へ遷移 */
					break;
				default:
					break;
			}
			break;
	}
}

/**
 *	@brief SW手動通常リクエストによる状態変更要求処理
 *
 *	@retval なし
 */
static int EventSWManualTuujouRequest(void)
{
	int flg = 0;
	switch (now_status.mode) {
		case MODE_REMOTE: /* 遠隔 */
			switch (now_status.status) {
				case STATUS_P1:/* 通常ならば */
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				case STATUS_FAIL:/* 20170210 遠隔フェイルならば */
					IO_ChangeReq(PX_P1);
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				case STATUS_P2:/* 一掃ならば */
					IO_ChangeReq(PX_P1);/* 通常に遷移 */
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				default:
					break;
			}
			break;
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P1:/* 手動通常でも 20170205 */
				case STATUS_P2:/* 手動一掃ならば */
				case STATUS_FAIL:/* 手動フェイルならば */
					IO_ChangeReq(PX_P1);/* 通常に遷移 */
					flg = 1;
					break;
				default:
					break;
			}
			break;
	}
	return flg;
}

/**
 *	@brief SW手動一掃リクエストによる状態変更要求処理
 *
 *	@retval なし
 */
static int EventSWManualIssouRequest(void)
{
	int flg = 0;
	switch (now_status.mode) {
		case MODE_REMOTE: /* 遠隔 */
			switch (now_status.status) {
				case STATUS_P1:/* 遠隔通常ならば */
					IO_ChangeReq(PX_P2);
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				case STATUS_FAIL:/* 20170210 遠隔フェイルならば */
					IO_ChangeReq(PX_P2);
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				case STATUS_P3:/* 遠隔変移ならば */
					IO_ChangeReq(PX_P2);
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				default:
					break;
			}
			break;
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P1:/* 手動通常ならば */
				case STATUS_P2:/* 手動一掃でも 20170205 */
				case STATUS_P3:/* 手動変移ならば */
				case STATUS_FAIL:/* 手動フェイルならば */
					IO_ChangeReq(PX_P2);
					flg = 1;
					break;
				default:
					break;
			}
			break;
	}
	return flg;
}

/**
 *	@brief SW手動変移リクエストによる状態変更要求処理
 *
 *	@retval なし
 */
static int EventSWManualHenniRequest(void)
{
	int flg = 0;
	switch (now_status.mode) {
		case MODE_REMOTE: /* 遠隔 */
			switch (now_status.status) {
				case STATUS_P2:/* 一掃ならば */
					IO_ChangeReq(PX_P3);
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				case STATUS_FAIL:/* 20170210 遠隔フェイルならば */
					IO_ChangeReq(PX_P3);
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				case STATUS_P3:/* 遠隔変移ならば */
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				default:
					break;
			}
			break;
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P2:/* 一掃ならば */
				case STATUS_P3:/* 変異でも 20170205 */
				case STATUS_FAIL:/* フェイルならば */
					IO_ChangeReq(PX_P3);
					flg = 1;
					break;
				default:
					break;
			}
			break;
	}
	return flg;
}

/**
 *	@brief 手動フェイルリクエストによる状態変更要求処理
 *
 *	@retval なし
 */
static int EventManualFailRequest(void)
{
	int flg = 0;
	switch (now_status.mode) {
		case MODE_REMOTE: /* 遠隔 */
			switch (now_status.status) {
				case STATUS_P1:/* 遠隔通常ならば */
				case STATUS_P2:/* 遠隔一掃ならば */
				case STATUS_P3:/* 遠隔変移ならば */
					now_status.mode = MODE_MANUAL;
					IO_ChangeReq(PX_FAIL);
					flg = 1;
					break;
				default:
					break;
			}
			break;
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P1:/* 通常ならば */
				case STATUS_P2:/* 一掃ならば */
				case STATUS_P3:/* 変移ならば */
					IO_ChangeReq(PX_FAIL);
					flg = 1;
					break;
				default:
					break;
			}
			break;
	}
	return flg;
}

/**
 *	@brief 通常時間内での手動→遠隔リクエストによる状態変更要求処理
 *         通常時間内でのフェイルからの復帰時も呼ばれる
 *
 *	@retval なし
 */
static void EventRemoteTuujouRequest(void)
{
	switch (now_status.mode) {
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P1:/* 通常 */
					now_status.mode = MODE_REMOTE;
					break;
				case STATUS_P2:/* 一掃 */
					now_status.mode = MODE_REMOTE;
					IO_ChangeReq(P2_P2);/* 20170226 オフセットタイマ */
					IO_ChangeReq(PX_P1);/* 通常に移動 */
					break;
				case STATUS_P3:/* 変移 */
					now_status.mode = MODE_REMOTE;
					IO_ChangeReq(PX_P2);/* 一掃に移動 */
					IO_ChangeReq(P2_P2);/* オフセットタイマ */
					IO_ChangeReq(PX_P1);/* 通常に移動 */
					break;
				case STATUS_FAIL:/* フェイル */
					/* フェイルからの復帰がある場合　ここはよくわからない */
					IO_ChangeReq(PX_P1);/* 通常に移動 */
					break;
				default:
					break;
			}
			break;
		case MODE_REMOTE: /*  */
			switch (now_status.status) {
				case STATUS_FAIL:/* フェイル */
					/* フェイルからの復帰がある場合　ここはよくわからない */
					IO_ChangeReq(PX_P2);/* 20170225 一掃に移動 */
					IO_ChangeReq(P2_P2);/* 20170225  オフセットタイマ */
					IO_ChangeReq(PX_P1);/* 通常に移動 */
					break;
				default:
					break;
			}
			break;
	}
}

/**
 *	@brief 変移時間内での手動→遠隔リクエストによる状態変更要求処理
 *         変移時間内でのフェイルからの復帰時も呼ばれる
 *
 *	@retval なし
 */
static void EventRemoteHenniRequest(void)
{
	switch (now_status.mode) {
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P1:/* 通常 */
					now_status.mode = MODE_REMOTE;
					IO_ChangeReq(PX_P2);/* 一掃に移動 */
					IO_ChangeReq(P2_P2);/* オフセットタイマ */
					IO_ChangeReq(PX_P3);/* 変移に移動 */
					break;
				case STATUS_P2:/* 一掃 */
					IO_ChangeReq(P2_P2);/* 20170226 オフセットタイマ */
					IO_ChangeReq(PX_P3);/* 変移に移動 */
					now_status.mode = MODE_REMOTE;
					break;
				case STATUS_P3:/* 変移 */
					now_status.mode = MODE_REMOTE;
					break;
				default:
					break;
			}
			break;
		case MODE_REMOTE: /* 20170212 */
			switch (now_status.status) {
				case STATUS_FAIL:/* フェイル */
					/* フェイルからの復帰がある場合　ここはよくわからない */
					IO_ChangeReq(PX_P2);/* 20170225 一掃に移動 */
					IO_ChangeReq(P2_P2);/* 20170225  オフセットタイマ */
					IO_ChangeReq(PX_P3);/* 変移に移動 */
					break;
				default:
					break;
			}
			break;
	}
}

/**
 *	@brief 遠隔時変移時間開始リクエストによる状態変更要求処理
 *
 *	@retval なし
 */
static void EventRemoteTuujou2HenniRequest(void)
{
	switch (now_status.mode) {
		case MODE_REMOTE: /* 遠隔 */
			if (CheckErrorStatus() != 0) {
				/* 異常状態の場合は */
//#ifdef	kaji20170208
//異常時は時間でイベントしない
				IO_ChangeReq(PX_FAIL);/* フェイルに移動 */
//#endif
			} else {
				switch (now_status.status) {
					case STATUS_P1:/* 通常 */
						IO_ChangeReq(PX_P2);/* 一掃に移動 */
						IO_ChangeReq(P2_P2);/* オフセットタイマ */
						IO_ChangeReq(PX_P3);/* 変移に移動 */
						break;
					default:
						break;
				}
			}
			break;
	}
}

/**
 *	@brief 遠隔時通常時間開始リクエストによる状態変更要求処理
 *
 *	@retval なし
 */
static void EventRemoteHenni2TuujouRequest(void)
{
	switch (now_status.mode) {
		case MODE_REMOTE: /* 遠隔 */
			switch (now_status.status) {
				case STATUS_P3:/* 変移 */
					IO_ChangeReq(PX_P2);/* 一掃に移動 */
					IO_ChangeReq(P2_P2);/* オフセットタイマ */
					IO_ChangeReq(PX_P1);/* 通常に移動 */
					break;
				case STATUS_FAIL:/* フェイル */
					/* フェイルからの復帰がある場合　ここはよくわからない */
					/*
					エラー要因をチェックし、エラー無しならば
					フェイルになった要因は変移中の保他の制御機のフェイルによるものなので通常に移動する
					*/
//#ifdef	kaji20170208
//フェイルは時間で戻さない
				//201070209 if (CheckErrorStatus() == 0) {
						/* 異常では無い場合は */
						IO_ChangeReq(PX_P1);/* 通常に移動 */
				//20170209	}
//#endif
					break;
				default:
					break;
			}
			break;
	}
}
