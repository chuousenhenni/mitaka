/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	contb.h
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

#ifndef	___CONTB_H___
#define	___CONTB_H___

extern void ControlerBInit(void );
extern void ControlerBSrv(void);
extern void TimerIntContB(int count);/* ����@B�̃^�C�}���荞�ݏ��� */

extern void BroadcastDisp ( void );
extern void NormalDisp ( void);
extern void ResponseDisp ( void);


#endif	/* ___CONTB_H___ */
