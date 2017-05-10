/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	cont.h
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

#ifndef	___CONT_H___
#define	___CONT_H___

extern int GetNowRemoteStatus(void);/* ���݂̉��u���̃X�e�[�^�X��Ԃ����� */
extern void SetNowStatus(int status);/* �X�e�[�^�X�̃Z�[�u���� */
extern void IO_ChangeReq(char chane_type);/* �\����,�e����ω������鏈�� */

extern void ControlerInit(void );
extern void SetMode(STATUS *status, RESPONSE_COMMAND *response);/* ���݂̃��[�h���Z�b�g */

extern void SetDefault(void);/* �f�B�t�H���g�p�����[�^�ݒ� */
extern int CheckErrorStatus(void);/* ���݂̃G���[��Ԃ�now_status���画�肷�鏈�� */

extern void TimerIntCont(int count);/* ����@���ʂ̃^�C�}���荞�ݏ��� */

extern void ContSrv(RESPONSE_COMMAND *response);/* ����@���ʂ̏��� */
extern  void StatusDisp(void);/* �X�e�[�^�X�̕\������ */

extern void BroadcastCommand(BROADCAST_COMMAND *com);/* ����w�ߎ�M���� */

extern void SetDefault(void);/* �f�B�t�H���g�p�����[�^�ݒ� */
extern void SaveParam(void);/* �p�����[�^�̃Z�[�u���� */
extern int LoadStatus(void);/* �X�e�[�^�X�̃��[�h���� */
extern void SaveStatus(void);/* �X�e�[�^�X�̃Z�[�u���� */
extern void LoadParam(void);/* �p�����[�^�̃��[�h���� */

extern int CheckMuji(int no);/* �W�������n���ǂ����̔��菈�� */

extern PARAM param;/* �p�����[�^ */
extern int cont_led_status[ ];/* LED��� */
extern int power_off_bef_status;/* ��d���̃X�e�[�^�X */

#endif	/* ___CONT_H___ */
