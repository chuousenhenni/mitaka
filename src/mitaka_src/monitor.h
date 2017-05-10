/*
 ********************************************************************************************
 *
 *	TPMS ECU
 *
 ********************************************************************************************
 */
/** @file	monitor.h
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

#ifndef	___MONITOR_H___
#define	___MONITOR_H___

extern void MonitorInit(void);
extern void MonitorSrv(void);/* 監視盤の制御処理 */
extern void TimerIntMonitor(int count);/* 監視盤のタイマ割り込み処理 */
extern void MonitorCommandDisp(void);/* 監視盤への制御指令表示処理 */
extern void MonitorStatusDisp(void);/* ステータスの表示処理 */

#endif	/* ___MONITOR_H___ */
