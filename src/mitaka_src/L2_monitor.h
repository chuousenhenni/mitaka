/*
 ********************************************************************************************
 *
 *	Copyright (c) 2017  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	L2.h
 *  1200bps�L�����f���ʐM �჌�C���ʐM���䕔
 *
 *  �ʐM�f�[�^�Ɂu�K���v��������O�����肷��
 *  (PPP like)
 *
 ********************************************************************************************
 */

#ifndef	___L2_monitor_H___
#define	___L2_monitor_H___

extern void SendCom3(HANDLE h, unsigned char *p, int size);/* ����@�ԒʐM(USART_ID_3 1200bps ODD)�ւ̑��M���� */
extern void RcvCom3Init(void);/* �ϐ����̏����� */
extern void RcvCom3Srv(void);/* UART3�̎�M�f�[�^����������CONTROLER_QUEUE�ɐς݂Ȃ��������@���C�����[�v�p */
extern void TimerIntL2_monitor(int count);/* ��M�f�[�^�r�؂�^�C�}�[���� */


#endif	/* ___L2_monitor_H___ */
