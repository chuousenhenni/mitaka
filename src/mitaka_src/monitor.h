/*
 ********************************************************************************************
 *
 *	TPMS ECU
 *
 ********************************************************************************************
 */
/** @file	monitor.h
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

#ifndef	___MONITOR_H___
#define	___MONITOR_H___

extern void MonitorInit(void);
extern void MonitorSrv(void);/* �Ď��Ղ̐��䏈�� */
extern void TimerIntMonitor(int count);/* �Ď��Ղ̃^�C�}���荞�ݏ��� */
extern void MonitorCommandDisp(void);/* �Ď��Ղւ̐���w�ߕ\������ */
extern void MonitorStatusDisp(void);/* �X�e�[�^�X�̕\������ */

#endif	/* ___MONITOR_H___ */
