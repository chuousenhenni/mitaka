/*
 ********************************************************************************************
 *
 *	TPMS ECU
 *
 ********************************************************************************************
 */
/** @file	maintenance.h
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

#ifndef	___MAINTENANCE_H___
#define	___MAINTENANCE_H___

extern void MaintenanceInit(void );/* �f�o�b�O�p�̃L�[���͂ɂ�鏈���̏��������� */
extern void MaintenanceSrv(void);/* �f�o�b�O�p�̃L�[���͏��� */
extern void DebugPrint(char *str1, char *str2, int mask);/* �f�o�b�O�p�̕\������ */

#endif	/* ___MAINTENANCE_H___ */
