/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	io.h
 *  IO���䕔
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

#ifndef	___IO_H___
#define	___IO_H___

/*
 *===========================================================================================
 *					�萔��`�E�^��`�E�}�N����`
 *===========================================================================================
 */

/* SW�|�[�g�̃r�b�g�A�T�C�� */
#define SW_STATUS_BIT (1)/* �蓮:1/���u:0  */
#define SW_TUUJOU_BIT (2)/* �ʏ� */
#define SW_ISSOU_BIT (4)/* ��| */
#define SW_HENNI_BIT (8)/* �ψ� */
#define SW_START_BIT (0x10)/* �N�� */
#define SW_ERROR_RECOVERT_BIT (0x20)/* �ُ한�A */
#define SW_KEIKOUTOU_BIT (0x40)/* �u������:1/����:0 */
#define SW_TEIDEN_HUKKI_BIT (0x80)/* ��d���A */
#define SW_BYOU_BIT (0x300)/* �����e1,2�ُ�:1,2,3/����:0 */
#define SW_BYOU1_BIT (0x100)/* �����e1�ُ�:1/����:0 */
#define SW_BYOU2_BIT (0x200)/* �����e2�ُ�:1/����:0 */
#define SW_CDS_BIT (0x400)/* CDS�X�e�[�^�X */
#define SW_TEIDEN_BIT (0x800)/* ��d���o */
#define SW_MAINTENANCE_BIT (0x1000)/* �ێ�:1/�ʏ�:0 */

#define CHOUKOU_BIT (8)/* ��������@�u�����@FL�̎��H */

/* �Ď��Ճ|�[�g�̃r�b�g�A�T�C�� */
#define MAINTENANCEL_BIT (4)/* �ێ�{�^�� */
#define CAMMAND_BIT (1)/* �^�p��~�{�^�� */
#define BUZZER_STOP_BIT (2)/* �u�U�[��~�{�^�� */

#define ALARM_LAMP_LED_BIT (1)/* �x�񃉃��v */
#define UNYO_TEISHI_LED_BIT (2)/* �^�p��~LED */
#define BUZZER_TEISHI_LED_BIT (4)/* �u�U�[��~LED */
#define ALARM_BUZZER_BIT (8)/* �x��u�U�[  */

/* ����@LED�̔z��ԍ��A�r�b�g�ʒu */
enum _Led_ArrayNo{
	LED_YOBI1 = 0,
	LED_TUUJOU,
	LED_ISSOU,
	LED_HENNI,
	LED_YOBI5,
	LED_TEST,
	LED_ZUGARA,
	LED_YOBI8,
	LED_BOARD,
	LED_BYOU,
	LED_DENSOU,
	LED_MUSEN,
	LED_TEIDEN
};


/*
 *===========================================================================================
 *					�֐��v���g�^�C�v
 *===========================================================================================
 */

extern void TimerIntIO(int count);/* IO�֘A�̃^�C�}���荞�ݏ��� */
extern void ContIOInit(void);/* ����@��IO���������� */
extern int SWRead(void);/* SW�ǂݍ��ݏ��� */
extern int CDSRead(void);/* CDS�ǂݍ��ݏ��� */
extern void ChoukouWrite(int d);/* ���������o�͏��� */
extern int BoardRead(int no);/* ��Board�X�e�[�^�X�ǂݍ��ݏ��� */
extern void BoardWrite(int no, int d);/* Board�ւ̏o�͏��� */
extern int BoardRead(int no);/* �����e�X�e�[�^�X�ǂݍ��ݏ��� */
extern void ByouWrite( int d);/* �����e�ւ̏o�͏��� */
extern void NaishouWrite( int d);/* ���Ɣւ̏o�͏��� */
extern void ToukaWrite( int d);/* ���Δւ̏o�͏��� */
extern void PcPower( int d);/* PCPOWER�̏o�͏��� */
//extern void TeidenDispWrite( int d);/* ��d�\�����ւ̏o�͏��� */
extern int PowerStatusRead(void);/* ��d�X�e�[�^�X�ǂݍ��ݏ��� */
extern void ContLedWrite(void);/* ����@��LED�o�͏��� */
extern void ContLedOut(int i,int type );/* ����@��LED�\������ */

extern void MonitorIOInit(void);/* �Ď��Ղ�IO���������� */
extern int MonitorBtnRead(void);/* �Ď��Ճ{�^���ǂݍ��ݏ��� */
extern void MonitorBuzzerWrite( int d);/* �Ď��Ճu�U�[�ւ̏o�͏��� */
extern void MonitorLedWrite(void);/* �Ď��Ղ�LED�o�͏��� */
extern void MonitorLedOut(int i,int type );/* �Ď��Ղ�LED�\������ */

extern int FPGAVersionRead(void);/* FPGA�o�[�W�����ǂݍ��ݏ��� */
extern int CPUFPGAVersionRead(void);/* CPUFPGA�o�[�W�����ǂݍ��ݏ��� */
extern void MdmcsWrite( int data);/* MDM_CS�̏o�͏��� */

extern unsigned int RegRead( unsigned int address);/* IO�f�[�^�̓��͏��� */
extern void RegWrite( unsigned int address, int data);/* IO�f�[�^�̏o�͏��� */
extern void InitRTC(void);/* RTC�̃p���[�I�����Z�b�g���� */
extern void LoadRTC(TIME_INFO *t);/* �s�����pRTC����ǂݍ��ޏ��� */
extern void SaveRTC(TIME_INFO *t);/* �s�����pRTC�ɏ������ޏ��� */
extern void				SPI_E2ROM_Write( unsigned int address, unsigned char data);	/* E2PROM�ւ̏������� */
extern unsigned char	SPI_E2ROM_Read( unsigned int address);						/* E2PROM����̓ǂݏo�� */

extern int board_reg_buf[8];

#endif	/* ___IO_H___ */
