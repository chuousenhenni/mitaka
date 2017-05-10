/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	conta.h
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

#ifndef	___CONTA_H___
#define	___CONTA_H___

extern void ControlerAInit(void );
extern void ControlerASrv(void);
extern void TimerIntContA(int count);/* 制御機Aのタイマ割り込み処理 */
extern void SetSchedule( void );/* 本日のスケジュールをセットする処理 */

#endif	/* ___CONTA_H___ */
