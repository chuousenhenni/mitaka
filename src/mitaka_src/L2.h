/*
 ********************************************************************************************
 *
 *	Copyright (c) 2017  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	L2.h
 *	�������ʐM �჌�C���ʐM���䕔
 *
 *  �ʐM�f�[�^�Ɂu�K���v��������O�����肷��
 *  (PPP like)
 *
 ********************************************************************************************
 */

#ifndef	___L2_H___
#define	___L2_H___

extern void SendCom1(HANDLE h, unsigned char *p, int size);/* ����@�ԒʐM(USART_ID_1 64000bps ODD)�ւ̑��M���� */
extern void RcvCom1Init(void);/* �ϐ����̏����� */
extern void RcvCom1Srv(void);/* UART1�̎�M�f�[�^����������CONTROLER_QUEUE�ɐς݂Ȃ��������@���C�����[�v�p */
extern void TimerIntL2(int count);/* ��M�f�[�^�r�؂�^�C�}�[���� */


#endif	/* ___L2_H___ */
