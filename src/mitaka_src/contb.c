/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	contb.c
 *	概要
 *  制御機B制御部
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
#include "event.h"
#include "io.h"

/*
 *===========================================================================================
 *					内部定数定義・型定義・マクロ定義
 *===========================================================================================
 */
 
//define.hへ移動 #define MUSEN_TIMEOUT_VAL (45) /* 無線通信の受信エラータイムアウト値(秒)  kaji20170301 60→45 */

/*
 *===========================================================================================
 *					内部変数定義
 *===========================================================================================
 */

static BROADCAST_COMMAND broadcast_command;/* 制御機Aからの同報指令の電文 */
static NORMAL_COMMAND normal_command;/* 制御機Aからの通常指令の電文 */
static RESPONSE_COMMAND response_command;/* 制御機Aへの監視応答の電文 */
static RESPONSE_COMMAND response_command_send_buffer;/* 制御機Aへの監視応答の送信する電文 */
static int normal_command_timer;/* 通常指令受信待ちタイマ */
static unsigned char cmp_my_linkage;/* kaji20170407 BROADCAST_COMMANDのlinkage(sub_adr)flagと自局の比較用 */

/*
 *===========================================================================================
 *					外部変数定義
 *===========================================================================================
 */

extern int my_tanmatsu_type;/* 端末タイプ 1:A,2:B... */
extern HANDLE hComm1;       /* シリアルポートのハンドル */
extern STATUS now_status;/* 現在の設定値 */

/*
 *===========================================================================================
 *					内部	関数定義
 *===========================================================================================
 */

static void RcvControlerBSrv(void);
//static void BroadcastCommand(BROADCAST_COMMAND *com);/* 同報指令受信処理 */
static void NormalCommand(NORMAL_COMMAND *com) ;/* 通常指令受信処理 */
static void SendResponse(void);/* 監視応答の送信処理 */

/*
 *===========================================================================================
 *					グローバル関数定義
 *===========================================================================================
 */

void ControlerBInit(void );
void ControlerBSrv(void);
void TimerIntContB(int count);/* 制御機Bのタイマ割り込み処理 */

void BroadcastDisp ( void );
void NormalDisp ( void);
void ResponseDisp ( void);

/*
 *===========================================================================================
 *					外部	関数定義
 *===========================================================================================
 */

extern void SendCom1(HANDLE h, unsigned char *p, int size);

/*
 *===========================================================================================
 *					グローバル関数
 *===========================================================================================
 */

/**
 *	@brief 制御機Bのタイマ割り込み処理
 *
 *	@retval なし
 */
void TimerIntContB(int count)
{
	char str[256];
	if (param.no_musen_error_flag == 0) {
		normal_command_timer += count;/* 通常指令受信待ちタイマ */
//20170305	if (normal_command_timer > (MUSEN_TIMEOUT_VAL*1000)) {
		if (normal_command_timer > (param.musen_timeout_val*1000)) {
			now_status.musen_status = 1;
		} else {
			now_status.musen_status = 0;
		}
		if (now_status.before_musen_status != now_status.musen_status) {
			if (param.no_fail_flag == 0) {/* この場合はデバッグ用にフェイルにはしない */
				if (now_status.before_musen_status == 0) {
					sprintf(str,"無線異常に変化");
					DebugPrint("", str, 1);
					EventRequest(FAIL_REQUEST);/* 異常リクエスト */
					cont_led_status[LED_MUSEN] = LED_STATUS_TOGGLE;
				} else {
					sprintf(str, "無線正常に変化\n");
					DebugPrint("", str, 1);
#ifdef	kaji20170208
自分ではフェイルから抜ける判断を行わない
					if ((now_status.status == STATUS_FAIL) && (CheckErrorStatus() == 0)) {
						EventRequest(FAIL_RECOVER_REQUEST);/* 異常復帰リクエスト */
					}
#endif
					cont_led_status[LED_MUSEN] = LED_STATUS_OFF;
				}
			}
		}
		now_status.before_musen_status = now_status.musen_status;
	}
}


/**
 *	@brief 制御機Bの初期化処理
 *
 *	@retval なし
 */
void ControlerBInit(void )
{
	int	i;
	
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
	response_command.h.no = 0;/* 規格番号 00H固定 */
	response_command.h.dst = 1;/* 宛先アドレス  01H固定 */
	response_command.h.src = my_tanmatsu_type;/* 発信元アドレス 01H～08H */
	response_command.h.sub_adr = 0;/* サブアドレス 00H固定 */
	response_command.h.priority = 2;/* 優先レベル 02H固定 */
	response_command.h.s_no = 0;/* 通番 00H固定 */
	response_command.h.contoroler_no = 0x19;/* 端末種別 19H */
	response_command.h.info = 0x11;/* 情報種別 11H */
	response_command.h.div_no = 0x81;/* 分割番号 81H */
	response_command.h.length = 15;/* データ長 15(0FH) */

	normal_command_timer = 0;/* 通常指令受信待ちタイマ */

	cmp_my_linkage = 0x80;	/* kaji20170407 */
	for ( i=1; i<my_tanmatsu_type; i++ ) {
		cmp_my_linkage >>= 1;
	}
}

/**
 *	@brief 制御機Bの処理
 *
 *	@retval なし
 */
void ControlerBSrv(void)
{
	RcvControlerBSrv();
	ContSrv(&response_command);/* 制御機共通の処理 */
}

/*
 *===========================================================================================
 *					内部関数
 *===========================================================================================
 */

/**
 *	@brief 制御機Bの制御機Aからのデータの受信処理
 *
 *	@retval なし
 */
static void RcvControlerBSrv(void) {
	char str[256];
	char str2[20];
	unsigned char contb_rcv_buf[RCV_BUFF_SIZE];

	while(!empty(CONTROLER_QUEUE)) {
		unsigned char d = peek(CONTROLER_QUEUE, 0);/* 先頭データは０か？ */
		if ( d != 0) {
			dequeue(CONTROLER_QUEUE);
			continue;
		}
		if (lenqueue(CONTROLER_QUEUE) >= 10) {
			int contb_wait_count = peek(CONTROLER_QUEUE, 9) + 10;
			if ((contb_wait_count != 21) && (contb_wait_count != 28)){
				dequeue(CONTROLER_QUEUE);
			} else {
				//printf("%d\n",lenqueue(CONTROLER_QUEUE));
				if (lenqueue(CONTROLER_QUEUE) >= contb_wait_count) {
				//printf("X");
					/* データパケット受信 */
					int i;
					for(i = 0 ; i < contb_wait_count; i++) {
						contb_rcv_buf[i] = dequeue(CONTROLER_QUEUE);/* 受信データ取り出し */
					}
					CONTROL_HEADER *h =(CONTROL_HEADER *)contb_rcv_buf;
					if (h->dst == 0) {/* 同報制御指令 */
						BROADCAST_COMMAND * rp = (BROADCAST_COMMAND *) contb_rcv_buf;
						int bcc = rp->h.s_no;/* Bcc取り出し */
						rp->h.s_no = 0;
						int cbcc = CalcRealBcc(contb_rcv_buf,sizeof(BROADCAST_COMMAND));
						cbcc &= 0xff;
						if ( bcc == cbcc) {
//						if ( 1 ) {
							if ( (rp->h.sub_adr & cmp_my_linkage) != 0 ) {/* kaji20170407 linkage(sub_adr)に自局が含まれているか判定 */
								memmove(&broadcast_command, contb_rcv_buf, sizeof(BROADCAST_COMMAND));
								BROADCAST_COMMAND *com = (BROADCAST_COMMAND *)contb_rcv_buf;
								BroadcastCommand(com);/* 同報指令受信処理 */
							}
							else {
								sprintf(str, "RcvControlerBSrv同報制御指令(linkage不一致のため処理せず %X, %X)", rp->h.sub_adr, cmp_my_linkage);/* kaji20170407 */
								DebugPrint("", str, 2);
							}
						} else {
							sprintf(str, "RcvControlerBSrv同報制御指令 Bccエラー %d,%X<->%X",h->info,bcc, cbcc);
							DebugPrint("", str, 2);
							str[0] = '\0';
							for ( i = 0; i < sizeof(BROADCAST_COMMAND);i++) {
								sprintf(str2,"%02X ",contb_rcv_buf[i]);
								strcat(str, str2);
							}
							DebugPrint("", str, 2);
						}
					} else {
						if (h->info == 1) {/* 通常制御指令 */
							if (h->dst == my_tanmatsu_type) {
							//if (1) {
								NORMAL_COMMAND * rp = (NORMAL_COMMAND *) contb_rcv_buf;
								int bcc = rp->h.s_no;/* Bcc取り出し */
								rp->h.s_no = 0;
								int cbcc = CalcRealBcc(contb_rcv_buf,sizeof(NORMAL_COMMAND));
								cbcc &= 0xff;
								if ( bcc == cbcc) {
//								if ( 1) {
									memmove(&normal_command, contb_rcv_buf, sizeof(NORMAL_COMMAND));
									NORMAL_COMMAND *com = (NORMAL_COMMAND *)contb_rcv_buf;
									NormalCommand(com);/* 通常指令受信処理 */
									normal_command_timer = 0;/* 通常指令受信待ちタイマ */
								} else {
									sprintf(str, "RcvControlerBSrv通常制御指令 Bccエラー %d,%X<->%X",h->info,bcc, cbcc);
									DebugPrint("", str, 2);
									str[0] = '\0';
									for ( i = 0; i < sizeof(NORMAL_COMMAND);i++) {
										sprintf(str2,"%02X ",contb_rcv_buf[i]);
										strcat(str, str2);
									}
									DebugPrint("", str, 2);
								}
							} 
						} else {
							//受信エラー
							sprintf(str, "RcvControlerBSrv 情報種別エラー %02X",h->info);
							DebugPrint("", str, 2);
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
 *	@brief 通常指令受信処理
 *
 *	@retval なし
 */
static void NormalCommand(NORMAL_COMMAND *com) {

	char str[256];
	char str2[256];
	
	str2[0] = '\0';
	if (com->light_command.issei != 0) {
		/* 調光一斉指令有受信 */
		if (com->light_command.choukou_iri != 0) {
			/* 調光強制入（低） */
			sprintf(str2, " 調光強制入低）");
			//now_status.cds_status = 1; /* CDS状態 */
			now_status.cds_status = 0; /* CDS状態 */
			ChoukouWrite(0);/* 調光強制入出力処理 */
			response_command.response.byte7.choukou_iri =1;/* 調光入（昼） */
			response_command.response.byte7.choukou_kiri=0;/* 調光切（夜） */
		} else if (com->light_command.choukou_kiri != 0) {
			/* 調光強制切（高） */
			sprintf(str2, " 調光強制切高）");
			//now_status.cds_status = 0; /* CDS状態 */
			now_status.cds_status = 1; /* CDS状態 */
			ChoukouWrite(1);/* 調光強制切出力処理 */
			response_command.response.byte7.choukou_iri =0;/* 調光入（昼） */
			response_command.response.byte7.choukou_kiri=1;/* 調光切（夜） */
		} else {
			sprintf(str2, " 調光 ?????");
		}
		ByouWrite(now_status.status);//20170128 yamazaki
		ToukaWrite(my_tanmatsu_type);//20170129 yamazaki
	}
	sprintf(str, "通常制御指令受信 %d %d %02X,%02X,%02X", com->h.dst, my_tanmatsu_type
		, com->command.byte&0xf
		, com->light_command.byte&0xF
		, com->status.byte&0xf);
	strcat(str, str2);
	DebugPrint("NormalCommand", str, 4);
//yamazaki20170217 kaji20170216 ↓
	if ((now_status.mode == MODE_REMOTE) && (com->command.shudou == 0)){
		/* 制御機ABともに遠隔の時 */
		if (com->command.teishi != 0) {/* kaji20170301 */
			EventRequest(MONITOR_REQUEST);/* 運用停止解除リクエスト kaji20170301 */
			DebugPrint("NormalCommand","運用停止リクエスト受信", 1);/* kaji20170301 */
		} else if (com->command.tuujou != 0) {
			if (now_status.status != STATUS_P1) {
				EventRequest(TUUJOU_REQUEST);/* 制御機Bの通常リクエスト */
				DebugPrint("NormalCommand","通常リクエスト受信", 1);
			}
		} else if (com->command.issou != 0) {
			if (now_status.status != STATUS_P2) {
				EventRequest(ISSOU_REQUEST);/* 制御機Bの一掃リクエスト */
				DebugPrint("NormalCommand","一掃リクエスト受信", 1);
			}
		} else if (com->command.henni != 0) {
			if (now_status.status != STATUS_P3) {
				EventRequest(HENNI_REQUEST);/* 制御機Bの変移リクエスト */
				DebugPrint("NormalCommand","変移リクエスト受信", 1);
			}
		} else if (com->command.fail != 0) {
			/* フェイル受信(同報取りこぼしフォロー) */
			if (now_status.status != STATUS_FAIL) {
				//20170225 EventRequest(FAIL_REQUEST);/* 異常リクエスト */
				EventRequest(CONTB_FAIL_REQUEST);/* 異常リクエスト */
				DebugPrint("NormalCommand","フェイルリクエスト受信", 1);
			}
		} else { /* 20170217 */
			/* 遠隔受信(同報取りこぼしフォロー) */
			if (now_status.status == STATUS_FAIL) {
				EventRequest(REMOTE_REQUEST);/* 遠隔リクエスト */
				DebugPrint("NormalCommand","遠隔リクエスト受信", 1);
			}
		}
	}
//yamazaki20170217 kaji20170216 ↑
// kaji20170301 ↓
	else if (now_status.mode == MODE_MONITOR) {
		if (com->command.teishi == 0) {
			EventRequest(MONITOR_RELEASE_REQUEST);/* 運用停止解除リクエスト */
			DebugPrint("NormalCommand","運用停止解除状態受信", 1);
		}
	}
// 全線手動～全線自動　の取りこぼしフォローが、まだ出来ていない
	else if (now_status.mode == MODE_MANUAL) {
//now_status.manual_status = 0;/* 手動状態 0:SWによる手動,1:運用PCからの手動による */
	}
// kaji20170301 ↑
	
	if (com->light_command.time_req != 0) {
		/* 時刻修正要求受信 */
		SetTime(&(com->t));/* 時刻設定処理 */
		SaveRTC(&(com->t));/* 不揮発用RTCに書き込む */
	}
	SetMode(&now_status, &response_command);/* 現在のモードをセット */
	response_command.h.src = com->h.dst;
	if (now_status.schedule == 0) {
		/* スケジュール登録依頼送信 */
		DebugPrint("NormalCommand","スケジュール登録依頼送信", 1);
		response_command.response.byte7.schedule_req = 1;
	} else {
		response_command.response.byte7.schedule_req = 0;
	}
	if (now_status.time_req == 0) {
		/* 時刻修正依頼送信 */
		DebugPrint("NormalCommand","時刻修正依頼送信", 1);
		response_command.response.byte7.time_req = 1;
	} else {
		response_command.response.byte7.time_req = 0;
	}
	int diff_time = CheckDiffTime(&(com->t));
	//printf("***diff_time=%d\n",diff_time);
	if (diff_time > TIME_ERROR_VAL) {
		/* 時刻エラーとなる時間の差(秒) */
		now_status.time_status = 1;
		now_status.schedule = 0;/* 20170224 これでスケジュール登録要求を送信する */
		now_status.time_req = 0;/* 20170224 これで時刻設定要求を送信する */
	} else {
		now_status.time_status = 0;
	}
	memmove(&response_command_send_buffer, &response_command, sizeof(RESPONSE_COMMAND));
	//if (now_status.gps_status != 0) {
	//	/* 時計異常 */
	//	response_command_send_buffer.response.byte2.tanmatu_error = 1;/* 端末制御機異常 */
	//}
	
	sprintf(str, "RcvContolerSrv 制御応答送信 mode=%02X,%02X,%02X,%02X"
		,response_command_send_buffer.response.status[0]
		,response_command_send_buffer.response.status[1]
		,response_command_send_buffer.response.status[2]
		,response_command_send_buffer.response.status[3]
	);
	DebugPrint("NormalCommand ", str, 4);
	SendResponse( );
}
		
/* 監視応答の送信処理 */
static void SendResponse(void) {
	int bcc;
	SetNowTime(&response_command_send_buffer.t);/* 時刻セット */
	response_command_send_buffer.h.s_no = 0;/* 通番 00H固定 */
	bcc = CalcRealBcc((char *)&response_command_send_buffer,sizeof(RESPONSE_COMMAND));
	response_command_send_buffer.h.s_no = bcc;/* 20170305 Bccを通番にセットする */
	SendCom1(hComm1, (unsigned char *)&response_command_send_buffer, sizeof(RESPONSE_COMMAND));
}
/**
 *	@brief 同報制御指令の表示処理
 *
 *	@retval なし
 */
void BroadcastDisp ( void )
{
	BROADCAST_COMMAND *p;
	p = &broadcast_command;
	printf("■■■同報制御指令■■■\n");
	printf("時刻 %02X%02X/%02X/%02X %02X:%02X:%02X\n",p->t.year[0]
											,p->t.year[1]
											,p->t.month
											,p->t.day
											,p->t.hour
											,p->t.min
											,p->t.sec);
	printf("休日(DB7 1)、曜日 %02X\n",p->t.holiday_week.byte);
	printf("制御指令 %02X\n",p->command.byte);
	printf("調光指令 %02X\n",p->light_command.byte);
	printf("端末制御機（Ⅰ）状態 %02X\n",p->status.byte);
	printf("開始時刻 %02X:%02X 指令 %02X\n",p->schedule.start_time[0]
										,p->schedule.start_time[1]
										,p->schedule.start_command);
	printf("終了時刻 %02X:%02X 指令 %02X\n",p->schedule.end_time[0]
										,p->schedule.end_time[1]
										,p->schedule.end_command);
	printf("オフセットタイマ %02X0Sec\n",p->schedule.offset_timer);
}
/**
 *	@brief 通常制御指令の表示処理
 *
 *	@retval なし
 */
void NormalDisp ( void )
{
	NORMAL_COMMAND *p;
	p = &normal_command ;
	printf("■■■通常制御指令■■■\n");
	printf("時刻 %02X%02X/%02X/%02X %02X:%02X:%02X\n",p->t.year[0]
											,p->t.year[1]
											,p->t.month
											,p->t.day
											,p->t.hour
											,p->t.min
											,p->t.sec);
	printf("休日(DB7 1)、曜日 %02X\n",p->t.holiday_week.byte);
	printf("制御指令 %02X\n",p->command.byte);
	printf("調光指令 %02X\n",p->light_command.byte);
	printf("端末制御機（Ⅰ）状態 %02X\n",p->status.byte);
}
/**
 *	@brief 監視応答の表示処理
 *
 *	@retval なし
 */
void ResponseDisp ( void )
{
	RESPONSE_COMMAND *p;
	p = &response_command_send_buffer ;
	printf("■■■監視応答 ■■■\n");
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

