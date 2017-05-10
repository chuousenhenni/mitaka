/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	common.h
 *	�t�@�C���̊T�v���L�q����
 *
 *	�t�@�C���̏ڍׂ��L�q����
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

extern int GetTodaysMinute(void);/* ���ݎ����𕪂ŕԂ����� */
extern int GetTodaysSecond(void);/* ���ݎ�����b�ŕԂ����� */
extern void SetTime(TIME_INFO *t);/* t��RTC�ɐݒ肷�鏈�� */
extern void SetNowTime(TIME_INFO *t);/* ������t�ɐݒ肷�鏈�� */
extern int BIN(int a);/* BIN �ϊ� */
extern int BCD(int a);/* BCD �ϊ� */
extern void wait(int ms);/* �w�莞�ԑ҂��� */
extern int CalcBcc(char *p, int size);/* BCC�̌v�Z���� */
extern int CalcRealBcc(char *p, int size);/* BCC�̌v�Z���� */
extern int subZeller( int y, int m, int d );/* Zeller �̌����ŏT�̉����ڂ����ׂ� */
extern int LinkagePack(int linkage_status);/* linkage_status���p�b�N�Z�b�g���鏈�� */
extern int LinkageUnPack(int linkage_status);/* v���A���p�b�N�Z�b�g���鏈�� */
extern int CheckDiffTime(TIME_INFO *t);/* �����̌��ݎ����Ƃ̔�r���� */

#endif	/* ___COMMON_H___ */
