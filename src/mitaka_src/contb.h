/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	contb.h
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

#ifndef	___CONTB_H___
#define	___CONTB_H___

extern void ControlerBInit(void );
extern void ControlerBSrv(void);
extern void TimerIntContB(int count);/* 制御機Bのタイマ割り込み処理 */

extern void BroadcastDisp ( void );
extern void NormalDisp ( void);
extern void ResponseDisp ( void);


#endif	/* ___CONTB_H___ */
