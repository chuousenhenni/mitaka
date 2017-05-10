/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	common.h
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

#ifndef	___COMMON_H___
#define	___COMMON_H___

extern int GetTodaysMinute(void);/* 現在時刻を分で返す処理 */
extern int GetTodaysSecond(void);/* 現在時刻を秒で返す処理 */
extern void SetTime(TIME_INFO *t);/* tをRTCに設定する処理 */
extern void SetNowTime(TIME_INFO *t);/* 時刻をtに設定する処理 */
extern int BIN(int a);/* BIN 変換 */
extern int BCD(int a);/* BCD 変換 */
extern void wait(int ms);/* 指定時間待つ処理 */
extern int CalcBcc(char *p, int size);/* BCCの計算処理 */
extern int CalcRealBcc(char *p, int size);/* BCCの計算処理 */
extern int subZeller( int y, int m, int d );/* Zeller の公式で週の何日目か調べる */
extern int LinkagePack(int linkage_status);/* linkage_statusをパックセットする処理 */
extern int LinkageUnPack(int linkage_status);/* vをアンパックセットする処理 */
extern int CheckDiffTime(TIME_INFO *t);/* 時刻の現在時刻との比較処理 */

#endif	/* ___COMMON_H___ */
