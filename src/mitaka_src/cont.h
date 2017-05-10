/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	cont.h
 *	ファイルの概要を記述する
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

#ifndef	___CONT_H___
#define	___CONT_H___

extern int GetNowRemoteStatus(void);/* 現在の遠隔時のステータスを返す処理 */
extern void SetNowStatus(int status);/* ステータスのセーブ処理 */
extern void IO_ChangeReq(char chane_type);/* 表示板,鋲等を変化させる処理 */

extern void ControlerInit(void );
extern void SetMode(STATUS *status, RESPONSE_COMMAND *response);/* 現在のモードをセット */

extern void SetDefault(void);/* ディフォルトパラメータ設定 */
extern int CheckErrorStatus(void);/* 現在のエラー状態をnow_statusから判定する処理 */

extern void TimerIntCont(int count);/* 制御機共通のタイマ割り込み処理 */

extern void ContSrv(RESPONSE_COMMAND *response);/* 制御機共通の処理 */
extern  void StatusDisp(void);/* ステータスの表示処理 */

extern void BroadcastCommand(BROADCAST_COMMAND *com);/* 同報指令受信処理 */

extern void SetDefault(void);/* ディフォルトパラメータ設定 */
extern void SaveParam(void);/* パラメータのセーブ処理 */
extern int LoadStatus(void);/* ステータスのロード処理 */
extern void SaveStatus(void);/* ステータスのセーブ処理 */
extern void LoadParam(void);/* パラメータのロード処理 */

extern int CheckMuji(int no);/* 標識板が無地かどうかの判定処理 */

extern PARAM param;/* パラメータ */
extern int cont_led_status[ ];/* LED状態 */
extern int power_off_bef_status;/* 停電時のステータス */

#endif	/* ___CONT_H___ */
