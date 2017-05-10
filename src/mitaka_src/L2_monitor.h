/*
 ********************************************************************************************
 *
 *	Copyright (c) 2017  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	L2.h
 *  1200bps有線モデム通信 低レイヤ通信制御部
 *
 *  通信データに「ガワ」をつけたり外したりする
 *  (PPP like)
 *
 ********************************************************************************************
 */

#ifndef	___L2_monitor_H___
#define	___L2_monitor_H___

extern void SendCom3(HANDLE h, unsigned char *p, int size);/* 制御機間通信(USART_ID_3 1200bps ODD)への送信処理 */
extern void RcvCom3Init(void);/* 変数等の初期化 */
extern void RcvCom3Srv(void);/* UART3の受信データを処理してCONTROLER_QUEUEに積みなおす処理　メインループ用 */
extern void TimerIntL2_monitor(int count);/* 受信データ途切れタイマー処理 */


#endif	/* ___L2_monitor_H___ */
