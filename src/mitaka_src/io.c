/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	io.c
 *	�T�v
 *  IO���䕔
 *
 *
 ********************************************************************************************
 */
/*
 *===========================================================================================
 *					Includes
 *===========================================================================================
 */

#include "mitaka_common.h"

#include "define.h"
#include "app.h"
#include "maintenance.h"
#include "queue.h"
#include "monitor.h"
#include "common.h"
#include "cont.h"
#include "io.h"

/*
 *===========================================================================================
 *					�����萔��`�E�^��`�E�}�N����`
 *===========================================================================================
 */

/* �`���^�΍􎞂͗L���ɂ��� */
/*
�`���^�΍􏀔��p�ɍ쐬�������A�{�����ɂ�肻�̕K�v�͖����Ȃ�܂���20170206
*/
//#define BORD_CHATT

/* ����@���䃌�W�X�^ */
//#define BASE_REG (0x23FF0000)
#define BASE_REG (0xE3FF0000)
#define KAHEN_BOARD_REG		(0x00) /* �ϕW���i�P�`�W�j���� */
#define TOUKA_BOARD_REG		(0x10) /* ���ΕW���i�P�`�W�j���� */
#define SHADANMAKU_KYOKA_REG (0x12)/* �Ւf������ */
#define HAKKOUBYOU_OUT_REG	(0x16) /* �����eLED���� */
#define HAKKOUBYOU_IN_REG	(0x18) /* �����e���� */
#define PANEL_IN_REG		(0x18) /* ����p�l������ */
#define PANEL_OUT_REG		(0x1A) /* ����p�l��LED���� */

/* �Ď��Ր��䃌�W�X�^ */
#define MAP_LED_REG1		(0x40) /* �n�}��LED����1 */
#define MAP_LED_REG2		(0x42) /* �n�}��LED����2 */
#define MPANEL_OUT_REG		(0x44) /* ����p�l��LED���� */
#define MPANEL_IN_REG		(0x46) /* ����p�l������ */

/* ���ʃ��W�X�^ */
#define FPGA_VER_REG		(0x80) /* FPGA Version */
#define MODEM_REG			(0x82) /* ���f������ */
#define SPI_REG				(0x84) /* SPI���� */
#define FPGA_LED_REG		(0x86) /* FPGA LED���� */
#define FPGA_SW_REG			(0x86) /* FPGA SW���� */
#define SDISP_FPGA_VER_REG	(0x100) /* CPU��FPGA Version */
#define SDISP_FPGA_EXT_CFG	(0x104) /* CPU��FPGA �g���ݒ� */
#define SDISP_LED_REG		(0x200) /* CPU��FPGA LED���� */
#define SDISP_SW_REG		(0x200) /* CPU��FPGA SW���� */

/* SPI���W�X�^����p */
#define	SPI_CS_RTC			(1)
#define	SPI_CS_EEPROM		(2)
#define	SPI_CS_ENABLE		(1)
#define	SPI_CS_DISABLE		(0)

/* RTC(EPSON RX6110SAB) ���W�X�^ */
#define	RTC_REG_SEC				(0x10 + 0x00)	
#define	RTC_REG_MIN				(0x10 + 0x01)	
#define	RTC_REG_HOUR			(0x10 + 0x02)	
#define	RTC_REG_WEEK			(0x10 + 0x03)	
#define	RTC_REG_DAY				(0x10 + 0x04)	
#define	RTC_REG_MONTH			(0x10 + 0x05)	
#define	RTC_REG_YEAR			(0x10 + 0x06)	
#define	RTC_REG_CALIB			(0x10 + 0x07)	
#define	RTC_REG_MIN_ALM			(0x10 + 0x08)	
#define	RTC_REG_HOUR_ALM		(0x10 + 0x09)	
#define	RTC_REG_WEEK_DAY_ALM	(0x10 + 0x0A)	
#define	RTC_REG_CNT0			(0x10 + 0x0B)	
#define	RTC_REG_CNT1			(0x10 + 0x0C)	
#define	RTC_REG_EXT				(0x10 + 0x0D)	
#define	RTC_REG_FLAG			(0x10 + 0x0E)	
#define	RTC_REG_CNTL			(0x10 + 0x0F)	
#define	RTC_REG_RAM_BASE		(0x20 + 0x00)	/* ���[�U�[���W�X�^(16byte) */
#define	RTC_REG_TUNE			(0x30 + 0x00)	
#define	RTC_REG_RSV				(0x30 + 0x01)	
#define	RTC_REG_IRQ				(0x30 + 0x02)	
#define	RTC_REG_POR6_1			(0x60 + 0x00)	/* �p���[�I�����Z�b�g�p bank6-1 */
#define	RTC_REG_POR6_2			(0x60 + 0x06)	/* �p���[�I�����Z�b�g�p bank6-2 */
#define	RTC_REG_POR6_3			(0x60 + 0x0B)	/* �p���[�I�����Z�b�g�p bank6-3 */
#define	RTC_REG_POR6_4			(0x60 + 0x0B)	/* �p���[�I�����Z�b�g�p bank6-4 */

#define REG_MAX				(0x1000) /* ���W�X�^�̏���l */

#if 0
#define KAHEN_BOARD_REG		(BASE_REG + 0x00) /* �ϕW���i�P�`�W�j���� */
#define TOUKA_BOARD_REG		(BASE_REG + 0x10) /* ���ΕW���i�P�`�W�j���� */
#define HAKKOUBYOU_OUT_REG	(BASE_REG + 0x16) /* �����eLED���� */
#define HAKKOUBYOU_IN_REG	(BASE_REG + 0x18) /* �����e���� */
#define PANEL_IN_REG		(BASE_REG + 0x18) /* ����p�l������ */
#define PANEL_OUT_REG		(BASE_REG + 0x1A) /* ����p�l��LED���� */

/* �Ď��Ր��䃌�W�X�^ */
#define MAP_LED_REG1		(BASE_REG + 0x40) /* �n�}��LED����1 */
#define MAP_LED_REG2		(BASE_REG + 0x42) /* �n�}��LED����2 */
#define MPANEL_OUT_REG		(BASE_REG + 0x44) /* ����p�l��LED���� */
#define MPANEL_IN_REG		(BASE_REG + 0x46) /* ����p�l������ */

/* ���ʃ��W�X�^ */
#define FPGA_VER_REG		(BASE_REG + 0x80) /* FPGA Version */
#define MODEM_REG			(BASE_REG + 0x82) /* ���f������ */
#define SPI_REG				(BASE_REG + 0x84) /* SPI���� */
#define FPGA_LED_REG		(BASE_REG + 0x86) /* FPGA LED���� */
#define FPGA_SW_REG			(BASE_REG + 0x86) /* FPGA SW���� */
#define SDISP_FPGA_VER_REG	(BASE_REG + 0x100) /* CPU��FPGA Version */
#define SDISP_LED_REG		(BASE_REG + 0x200) /* CPU��FPGA LED���� */
#define SDISP_SW_REG		(BASE_REG + 0x200) /* CPU��FPGA SW���� */
#endif

/*
 *===========================================================================================
 *					�����ϐ���`
 *===========================================================================================
 */

int board_chatter_timer[DISPLAY_BOARD_MAX];/* �`���^�h�~�W���łh�n���[�h�^�C�}�[ */

int cont_output_data;
int bef_cont_output_data;
int board_reg_buf[8];

int bef_board_status_value[8];/* �O��̕W���ł̃X�e�[�^�X�l */
int board_status_value[8];/* ����̕W���ł̃X�e�[�^�X�l */

//int cds_status_val = 0;//�Ƃ���Ɩ���LED�_�Ō��ۂ��N����H�H�H
int cds_status_val;

int monitor_output_data;
static int bef_monitor_output_data;
static int panel_out_reg_buf;
static int bef_panel_out_reg_buf;
//static uint16_t hakkobyou_out_reg_buf;//�Ȃ���int���Ƃ��������Ȃ��Ă���
//static uint16_t bef_hakkobyou_out_reg_buf;
int hakkobyou_out_reg_buf;//�Ȃ���static int���Ƃ��������Ȃ��Ă��� 20170218
int bef_hakkobyou_out_reg_buf;//�Ȃ���static int���Ƃ��������Ȃ��Ă��� 20170218
int touka_out_reg_buf;
int bef_touka_out_reg_buf;
static int Mdmcs_reg_buf;




/*
 *===========================================================================================
 *					�O���ϐ���`
 *===========================================================================================
 */

extern int swtest;
extern uint32_t input_data[DISPLAY_BOARD_MAX];/* �e�ϕ\������̓��̓f�[�^ */
extern int btn_status;/* �Ď��Ճ{�^���̏�Ԃ�ێ� */
extern int io_power_outage_flag;/* ��d�X�e�[�^�X */
extern STATUS now_status;/* ���݂̐ݒ�l */
extern IO_INFO my_io_info;/* ������@��IO��ԊǗ��p ���Ɣ���ɕK�v */
extern uint32_t board_status[DISPLAY_BOARD_MAX];/* �e�ϕ\���̐���ُ��� */

/*
 *===========================================================================================
 *					����	�֐���`
 *===========================================================================================
 */
#ifndef windows
static void				SPI_CsWrite( unsigned char dst, unsigned char mode );						/* SPI�o�X�f�o�C�X�I�� */
static void				SPI_SendData( unsigned char data);											/* SPI�o�X�ւ̑��M */
static unsigned char	SPI_RcvData( void );														/* SPI�o�X����̎�M */

static void				SPI_RTC_Write( unsigned int address, unsigned char data);					/* RTC�ւ̏������� */
static unsigned char	SPI_RTC_Read( unsigned int address);										/* RTC����̓ǂݏo�� */
static void				SPI_RTC_ReadSTR( unsigned int address, unsigned char *data, int length);	/* RTC����̘A���ǂݏo�� */
#endif
static int BoardRealRead(int no);/* ��Board�X�e�[�^�X�ǂݍ��ݏ��� */
/*
 *===========================================================================================
 *					�O��	�֐���`
 *===========================================================================================
 */

/*
 *===========================================================================================
 *					�O���[�o���֐�
 *===========================================================================================
 */

void TimerIntIO(int count);/* IO�֘A�̃^�C�}���荞�ݏ��� */
void ContIOInit(void);/* ����@��IO���������� */
int SWRead(void);/* SW�ǂݍ��ݏ��� */
int CDSRead(void);/* CDS�ǂݍ��ݏ��� */
void ChoukouWrite(int d);/* ���������o�͏��� */
int BoardRead(int no);/* ��Board�X�e�[�^�X�ǂݍ��ݏ��� */
void BoardWrite(int no, int d);/* Board�ւ̏o�͏��� */
void ByouWrite( int d);/* �����e�ւ̏o�͏��� */
void NaishouWrite( int d);/* ���Ɣւ̏o�͏��� */
void ToukaWrite( int d);/* ���Δւ̏o�͏��� */
void PcPower( int d);/* PCPOWER�̏o�͏��� */
//void TeidenDispWrite( int d);/* ��d�\�����ւ̏o�͏��� */
int PowerStatusRead(void);/* ��d�X�e�[�^�X�ǂݍ��ݏ��� */
void ContLedWrite(void);/* ����@��LED�o�͏��� */
void ContLedOut(int i,int type );/* ����@��LED�\������ */

void MonitorIOInit(void);/* �Ď��Ղ�IO���������� */
int MonitorBtnRead(void);/* �Ď��Ճ{�^���ǂݍ��ݏ��� */
void MonitorBuzzerWrite( int d);/* �Ď��Ճu�U�[�ւ̏o�͏��� */
void MonitorLedWrite(void);/* �Ď��Ղ�LED�o�͏��� */
void MonitorLedOut(int i,int type );/* �Ď��Ղ�LED�\������ */

int FPGAVersionRead(void);/* FPGA�o�[�W�����ǂݍ��ݏ��� */
int CPUFPGAVersionRead(void);/* CPUFPGA�o�[�W�����ǂݍ��ݏ��� */
void MdmcsWrite( int data);/* MDM_CS�̏o�͏��� */

unsigned int RegRead( unsigned int address);/* IO�f�[�^�̓��͏��� */
void RegWrite( unsigned int address, int data);/* IO�f�[�^�̏o�͏��� */
void InitRTC(void);/* RTC�̃p���[�I�����Z�b�g���� */

void LoadRTC(TIME_INFO *t);/* �s�����pRTC����ǂݍ��ޏ��� */
void SaveRTC(TIME_INFO *t);/* �s�����pRTC�ɏ������ޏ��� */
void			SPI_E2ROM_Write( unsigned int address, unsigned char data);	/* E2PROM�ւ̏������� */
unsigned char	SPI_E2ROM_Read( unsigned int address);						/* E2PROM����̓ǂݏo�� */

/*
 *===========================================================================================
 *					�O��	�֐���`
 *===========================================================================================
 */

/*
 *===========================================================================================
 *					�O���[�o���֐�
 *===========================================================================================
 */

/**
 *	@brief IO�֘A�̃^�C�}���荞�ݏ���
 *
 *	@retval �Ȃ�
 */
void TimerIntIO(int count)
{
#ifdef BORD_CHATT
#define BOARD_READ_INTERVAL (10)	/* �W���Փǂݍ��݊Ԋu(ms) */
	int i;
	for ( i=0; i <DISPLAY_BOARD_MAX; i++) {
		board_chatter_timer[i] += count;/* �`���^�h�~�W���łh�n���[�h�^�C�}�[ */
		if(board_chatter_timer[i] > BOARD_READ_INTERVAL) {
			/* ms�Ԋu�Ń��[�h���� */
			board_status_value[i] = BoardRealRead(i);
			board_chatter_timer[i] = 0;
		}
	}
#endif
}

/**
 *	@brief ����@��IO����������
 *
 *	@retval ����
 */
void ContIOInit(void)
{
	int i,j,k;

	RegWrite(SDISP_FPGA_EXT_CFG, 0x405a);	/* extend bus mode select & reset asert  */
	RegWrite(SDISP_FPGA_EXT_CFG, 0x005a);	/*                          reset negate */
	
	//printf("waitkey end \n");
	k=0;
	for (i = 0; i<10000; i++){
	//����͂���for (i = 0; i<1000; i++){
		for (j = 0; j<100; j++){
			k++;
		}
	}
	//printf("waitkey end \n");
	InitRTC();

	cont_output_data = 0;
	bef_cont_output_data = 0;
	RegWrite(PANEL_OUT_REG, cont_output_data);

	Mdmcs_reg_buf = 0;
	RegWrite(MODEM_REG, 0);

	hakkobyou_out_reg_buf = 0;
	bef_hakkobyou_out_reg_buf = 0;
	RegWrite(HAKKOUBYOU_OUT_REG, hakkobyou_out_reg_buf);

	touka_out_reg_buf = 0;	
	bef_touka_out_reg_buf = 0;	
	RegWrite(TOUKA_BOARD_REG, touka_out_reg_buf);

	cds_status_val = 0;
	/* ������Ԃ�����̂ł��傢���� */
	for ( i = 0; i < 8; i++) {
		board_reg_buf[i] = 0;
		BoardWrite( i, 0);
		bef_board_status_value[i] = BoardRealRead(i);/* �O��̕W���ł̃X�e�[�^�X�l */
		board_status_value[i] = bef_board_status_value[i];
		board_chatter_timer[i] = 0;/* �`���^�h�~�W���łh�n���[�h�^�C�}�[ */
	}
	
	RegWrite(SHADANMAKU_KYOKA_REG, 1);/* �Ւf�����ւ̏o�͏��� */

}

/**
 *	@brief SW�ǂݍ��ݏ���
 *
 *	@retval SW���
 */
int SWRead(void)
{
	int reg;
#ifdef windows
	reg = swtest;
#else
	reg = RegRead(PANEL_IN_REG);
#endif
	return reg;
}


/**
 *	@brief CDS�ǂݍ��ݏ���
 *
 *	@retval SW���
 */
int CDSRead(void)
{
	int reg = 0;
	reg = RegRead(PANEL_IN_REG);
	reg &= SW_CDS_BIT;
	if (reg != 0) {
		reg = 1;
	}
	return reg;
}

/**
 *	@brief ���������o�͏���
 *
 *	@retval �Ȃ�
 */
void ChoukouWrite(int data)
{
	char str[100];
	int i;
	int d;
	int mj;
	
	if (now_status.power_outage_flag == 1) {
		/* ��d�������͋����I�Ɍu���������� */
		data = 0;
		cds_status_val = data;
	} else {
		if (now_status.keikoutou_status == 1) {
			/* 1:�u������/0:���� */
			/* �u���������[�h�̏ꍇ�͋����I�Ɍu�������ɂ��� */
			data = 1;
		}
		cds_status_val = data;
		if (now_status.status == STATUS_P3) {/* kaji20170308 �ψڒ��͌u�������ɂ��� */
			/* kaji20170308 	�����e�Ƃ̒ʐM��������Ԃ̎q�P�ƂŎ蓮SW�Ő؂�ւ����ꍇ�́A
									���̊֐����Ă΂��ꏊ��EventRequest��˂����񂾒���Ȃ̂�
									IOsrv��request�����������܂ł�status�͕ς���Ă��炸
									����Ď蓮SW���ψڂł����Ă��u�������_�����Ȃ�
									�c�������A���̌㉽��event������ƁA�_����Ԃ��؂�ւ�遦�� */
			data = 1;/* �u����OFF����STATUS_P3�̏ꍇ�A�����e���Â��Ȃ�̂�h������ */
		}
	}
	for ( i = 0; i < my_io_info.display_board_count; i++) {
		d = board_reg_buf[i];
		d &= ~CHOUKOU_BIT;
		//if (data != 0) {
		mj = CheckMuji(i);/* �W�������n���ǂ����̔��菈�� */
		//20170216 if ((data != 0) && (board_status[i] == 0)){/* �ُ�\���͏��� */
		if ((data != 0) && (board_status[i] == 0) && (mj == 0)){/* �ُ�\���͏��� ���n�̏ꍇ�͓_�����Ȃ� */
			d |= CHOUKOU_BIT;
		}
		if (board_reg_buf[i] != d) {
			board_reg_buf[i] = d;
			RegWrite(KAHEN_BOARD_REG + 2 * i, d);
			sprintf(str, "KAHEN_BOARD_REG(%02XH) = %04X", KAHEN_BOARD_REG + 2*i, d);
			DebugPrint("ChoukouWrite", str, 0x10);
		}
	}
}

/**
 *	@brief ��Board�X�e�[�^�X�ǂݍ��ݏ���
 *
 *	@retval ��Board�X�e�[�^�X
 */
int BoardRead(int no)
{
#ifdef BORD_CHATT
	return board_status_value[no];
#else
	/* ������͒��ڂ̃f�[�^��Ԃ� */
	return BoardRealRead(no);
#endif
}

/**
 *	@brief �`���^���������̉�Board�X�e�[�^�X�ǂݍ��ݏ���
 *
 *	@retval ��Board�X�e�[�^�X
 */
static int BoardRealRead(int no)
{
#ifdef windows
	return input_data[no];
#else
	int reg;
	reg = RegRead(KAHEN_BOARD_REG + 2 * no);
	reg >>= 8;
	reg &= 0xf;
	return reg;
#endif
}

/**
 *	@brief �����e�ւ̏o�͏���
 *
 *	@retval �Ȃ�
 */
void ByouWrite( int d)
{
	extern int keep_byou_status_flg;/* �����e�ُ��ێ�����t���O */

	hakkobyou_out_reg_buf &= ~0xfff0;
	if (keep_byou_status_flg == 1){
	//�������͖߂邩��_�� if (now_status.byou_status == 1){
	//�L���ɂ��� if (0){//�����͖����ɂ��Ă���
		/* �����e�ُ펞�͏����H */
	} else {
		switch (d) {
		case STATUS_P1:
			//hakkobyou_out_reg_buf |= 0x410;
			hakkobyou_out_reg_buf |= 0x010;
			break;
		case STATUS_P2:
			hakkobyou_out_reg_buf |= 0x220;
			break;
		case STATUS_P3:
			//hakkobyou_out_reg_buf |= 0x140;
			hakkobyou_out_reg_buf |= 0x100;
			break;
		case STATUS_FAIL:
			hakkobyou_out_reg_buf |= 0x220;
			break;
		}
		if (cds_status_val != 0) {
			hakkobyou_out_reg_buf |= 0x880;
		}
	}
	//�����ł͏����Ȃ� RegWrite(HAKKOUBYOU_OUT_REG, hakkobyou_out_reg_buf);
	//printf("******hakkobyou_out_reg_buf = %X\n",hakkobyou_out_reg_buf);
}

/**
 *	@brief ���Ɣւ̏o�͏���
 *         ����@�S�݂̂̓��ʂȐ���
 *
 *	@retval �Ȃ�
 */
void NaishouWrite( int data)
{
	char str[100];
	int i;
	int d;
	
	i = 1;
	d = board_reg_buf[i];
	board_reg_buf[i] &= ~5;

	switch (data) {
	case STATUS_P1:
		board_reg_buf[i] |= 1;
		
		break;
	case STATUS_P2:
	case STATUS_P3:
	case STATUS_FAIL:
		board_reg_buf[i] |= 4;/* 20170206 */
		break;
	}
	if (board_reg_buf[i] != d) {
		RegWrite(KAHEN_BOARD_REG + 2 * i, board_reg_buf[i]);
		sprintf(str, "KAHEN_BOARD_REG(%02XH) = %04X", KAHEN_BOARD_REG + 2*i, board_reg_buf[i]);
		DebugPrint("NaishouWrite", str, 0x10);
	}
}

/**
 *	@brief ���Δւ̏o�͏���
 *         �N�����Ɉ�x�����Ă΂��
 *
 *	@retval �Ȃ�
 */
void ToukaWrite( int tanmatsu_type)
{
	if ((tanmatsu_type == 1)|| (tanmatsu_type == 4)){
		if (tanmatsu_type == 1){
			touka_out_reg_buf &= ~0xf000;;
			if (cds_status_val != 0) {
				touka_out_reg_buf |= 0x7000;
			}
		} else {
			touka_out_reg_buf &= ~0xf;;
			if (cds_status_val != 0) {
				touka_out_reg_buf |= 7;
			}
		}
		//RegWrite(TOUKA_BOARD_REG, touka_out_reg_buf);
		//printf("ToukaWrite touka_out_reg_buf = %X\n",touka_out_reg_buf);
	}
}

/**
 *	@brief PCPOWER�̏o�͏���
 *
 *	@retval �Ȃ�
 */
void PcPower( int d)
{
	hakkobyou_out_reg_buf &= 0xfffe;
	if ( d == 1) {
		hakkobyou_out_reg_buf |= 1;
	}
	//�����ł͏����Ȃ� RegWrite(HAKKOUBYOU_OUT_REG, hakkobyou_out_reg_buf);
	//printf("PcPower hakkobyou_out_reg_buf=%X\n",hakkobyou_out_reg_buf);
}

/**
 *	@brief ��d�\�����ւ̏o�͏���
 *
 *	@retval �Ȃ�
 */
//void TeidenDispWrite( int d)
//{
//	//int reg;
//	if ( d != 0) {
//		cont_output_data |= (1 << LED_TEIDEN);/* �Y���r�b�g�Z�b�g */
//	} else {
//		cont_output_data &= (~(1 << LED_TEIDEN));/* �Y���r�b�g�N���A */
//	}
//}


/**
 *	@brief ��d�X�e�[�^�X�ǂݍ��ݏ���
 *
 *	@retval ��d�X�X�e�[�^�X
 */
int PowerStatusRead(void)
{
#ifdef windows
	return io_power_outage_flag;
#else
	int reg = 0;
	
	reg = RegRead(PANEL_IN_REG);
	reg &= SW_TEIDEN_BIT;
	if (reg != 0) {
		reg = 1;
	}
	return reg;
#endif
}

/**
 *	@brief Board�ւ̏o�͏���
 *
 *	@retval �Ȃ�
 */
void BoardWrite(int no, int data)
{
	char str[100];
	int d = board_reg_buf[no];
	d &= ~3;
	d |= data;
	if (board_reg_buf[no] != d) {
		board_reg_buf[no] = d;
		RegWrite(KAHEN_BOARD_REG + 2 * no, d);
		sprintf(str, "KAHEN_BOARD_REG(%02XH) = %04X", KAHEN_BOARD_REG + 2*no, d);
		DebugPrint("BoardWrite", str, 0x10);
	}
}

/**
 *	@brief ����@��LED,IO�o�͏���
 *
 *	@retval �Ȃ�
 */
void ContLedWrite(void)
{
	char str[100];
	
	if(bef_cont_output_data != cont_output_data) {
		RegWrite(PANEL_OUT_REG, cont_output_data);
		sprintf(str, "PANEL_OUT_REG(%02XH) = %04X", PANEL_OUT_REG, cont_output_data);
		DebugPrint("", str, 0x10);
	}
	bef_cont_output_data = cont_output_data;

	if(bef_hakkobyou_out_reg_buf != hakkobyou_out_reg_buf) {
		RegWrite(HAKKOUBYOU_OUT_REG, hakkobyou_out_reg_buf);
		sprintf(str, "HAKKOUBYOU_OUT_REG(%02X) = %04X", HAKKOUBYOU_OUT_REG, hakkobyou_out_reg_buf);
		DebugPrint("", str, 0x10);
	}
	bef_hakkobyou_out_reg_buf = hakkobyou_out_reg_buf;
	
	if(bef_touka_out_reg_buf != touka_out_reg_buf) {
		RegWrite(TOUKA_BOARD_REG, touka_out_reg_buf);
		sprintf(str, "TOUKA_BOARD_REG(%02X) = %04X", TOUKA_BOARD_REG, touka_out_reg_buf);
		DebugPrint("", str, 0x10);
	}
	bef_touka_out_reg_buf = touka_out_reg_buf;

}

/**
 *	@brief ����@��LED�\������
 *
 *	@retval �Ȃ�
 */
/*

����@LED�̔z��ԍ��ɉ�����LED�̐�����s��
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

*/
void ContLedOut(int i,int type )
{
	char str[100];
	switch (type) {
	case LED_STATUS_OFF:
		cont_output_data &= (~(1 << i));/* �Y���r�b�g�N���A */
		sprintf(str, "%d �������o�� %.4X", i, cont_output_data);
		break;
	case LED_STATUS_ON:
		cont_output_data |= (1 << i);/* �Y���r�b�g�Z�b�g */
		sprintf(str, "%d �_�����o�� %.4X", i, cont_output_data);
		break;
	default:
		sprintf(str, "LED�o�͂��� �����ɂ��Ă͂����Ȃ�");
		break;
	}
	//DebugPrint("", str, 4);
}


/**
 *	@brief �Ď��Ղ�IO����������
 *
 *	@retval ����
 */
void MonitorIOInit(void)
{
	int i,j,k;
	
	RegWrite(SDISP_FPGA_EXT_CFG, 0x405a);	/* extend bus mode select & reset asert  */
	RegWrite(SDISP_FPGA_EXT_CFG, 0x005a);	/*                          reset negate */

	//printf("waitkey end \n");
	k=0;
	for (i = 0; i<10000; i++){
	//����͂���for (i = 0; i<1000; i++){
		for (j = 0; j<100; j++){
			k++;
		}
	}
	//printf("waitkey end \n");
	InitRTC();

	monitor_output_data = 0;
	bef_monitor_output_data = 0;
	RegWrite(MAP_LED_REG1, monitor_output_data & 0xffff);
	RegWrite(MAP_LED_REG2, (monitor_output_data >> 16) & 0xffff);
	
	panel_out_reg_buf = 0;
	bef_panel_out_reg_buf = 0;
	RegWrite(MPANEL_OUT_REG, panel_out_reg_buf);

}

/*
 *	@brief �Ď��Ճ{�^���ǂݍ��ݏ���
 *
 *	@retval �Ď��Ճ{�^���X�e�[�^�X
 */
int MonitorBtnRead(void)
{
#ifdef windows
	return btn_status;
#else
	int reg = 0;
	
	reg = RegRead(MPANEL_IN_REG);
	return reg;
#endif
}

/**
 *	@brief �Ď��Ճu�U�[�ւ̏o�͏���
 *
 *	@retval �Ȃ�
 */
void MonitorBuzzerWrite( int d)
{
	if (d == 0) {
		panel_out_reg_buf &= (~ALARM_BUZZER_BIT);
	} else {
		panel_out_reg_buf |= ALARM_BUZZER_BIT;
	}
}

/**
 *	@brief �Ď��Ղ�LED,IO�o�͏���
 *
 *	@retval �Ȃ�
 */
void MonitorLedWrite(void)
{
	char str[100];
	
	if(bef_monitor_output_data != monitor_output_data) {
		RegWrite(MAP_LED_REG1, monitor_output_data & 0xffff);
		RegWrite(MAP_LED_REG2, (monitor_output_data >> 16) & 0xffff);
		sprintf(str, "MAP_LED_REG1,2 = %04X,%04X", monitor_output_data & 0xffff, (monitor_output_data >> 16) & 0xffff);
		DebugPrint("", str, 0x10);
	}
	bef_monitor_output_data = monitor_output_data;
	
	if(bef_panel_out_reg_buf != panel_out_reg_buf) {
		RegWrite(MPANEL_OUT_REG, panel_out_reg_buf);
		sprintf(str, "MPANEL_OUT_REG(%02X) = %04X", MPANEL_OUT_REG, panel_out_reg_buf);
		DebugPrint("", str, 0x10);
	}
	bef_panel_out_reg_buf = panel_out_reg_buf;
	
}

/**
 *	@brief �Ď��Ղ�LED�\������
 *
 *	@retval �Ȃ�
 */

/*
�S����18��
0,2,4,6,8,10,12,14:�^�p���[�hLED
1,3,5,7,9,11,13,15:�[�����LED
�Ō��2�͉^�p��~,�u�U�[��~LED�p�Ȃ̂Ő�����@���قȂ�
�^�p��~LED�̔z��|�C���^
#define UNYOU_TEISHI_LED (2 * CONTROLER_MAX)
�u�U�[��~LED�̔z��|�C���^
#define BUZZER_TEISHI_LED (2 * CONTROLER_MAX + 1)

*/
void MonitorLedOut(int i,int type )
{
	int bit1;
	int bit2;
	char str[100];
//	printf("%d,%d\n",UNYOU_TEISHI_LED,
	bit1 = (1 << ((2*i)+0));
	bit2 = (1 << ((2*i)+1));
	if (i == UNYOU_TEISHI_LED) {
		/* �^�p��~LED */
		if (type == LED_STATUS_OFF) {
			panel_out_reg_buf &= (~UNYO_TEISHI_LED_BIT);
		} else {
			panel_out_reg_buf |= UNYO_TEISHI_LED_BIT;
		}
		
	} else if (i == BUZZER_TEISHI_LED) {
		/* �u�U�[��~LED */
		if (type == LED_STATUS_OFF) {
			panel_out_reg_buf &= (~BUZZER_TEISHI_LED_BIT);
		} else {
			panel_out_reg_buf |= BUZZER_TEISHI_LED_BIT;
		}
	} else if (i == ALARM_LAMP_LED) {
		/* �u�U�[��~LED */
		if (type == LED_STATUS_OFF) {
			panel_out_reg_buf &= (~ALARM_LAMP_LED_BIT);
		} else {
			panel_out_reg_buf |= ALARM_LAMP_LED_BIT;
		}
	} else {
		switch (type) {
		case LED_STATUS_OFF:
			monitor_output_data &= ~bit1;/* �Y���r�b�g�N���A */
			monitor_output_data &= ~bit2;/* �Y���r�b�g�N���A */
			sprintf(str, "%d �������o�� %.8X", i + 1, monitor_output_data);
			break;
		case LED_STATUS_GREEN:
			monitor_output_data &= ~bit1;/* �Y���r�b�g�Z�b�g */
			monitor_output_data |= bit2;/* �Y���r�b�g�N���A */
			sprintf(str, "%d �Γ_�����o�� %.8X", i + 1, monitor_output_data);
			break;
		case LED_STATUS_ORANGE:
			monitor_output_data |= bit1;/* �Y���r�b�g�Z�b�g */
			monitor_output_data |= bit2;/* �Y���r�b�g�Z�b�g */
			sprintf(str, "%d ��_�����o�� %.8X", i + 1, monitor_output_data);
			break;
		case LED_STATUS_RED:
			monitor_output_data |= bit1;/* �Y���r�b�g�Z�b�g */
			monitor_output_data &= ~bit2;/* �Y���r�b�g�N���A */
			sprintf(str, "%d �ԓ_�����o�� %.8X", i + 1, monitor_output_data);
			break;
		default:
			sprintf(str, "LED�o�͂��� �����ɂ��Ă͂����Ȃ�");
			break;
		}
	}
	//DebugPrint("", str, 4);
}

/**
 *	@brief FPGA�o�[�W�����ǂݍ��ݏ���
 *
 *	@retval FPGA�o�[�W����
 */
int FPGAVersionRead(void)
{
	int reg;
	reg = RegRead(FPGA_VER_REG);
	return reg;
}

/**
 *	@brief CPUFPGA�o�[�W�����ǂݍ��ݏ���
 *
 *	@retval CPUFPGA�o�[�W����
 */
int CPUFPGAVersionRead(void)
{
	int reg;
	reg = RegRead(SDISP_FPGA_VER_REG);
	return reg;
}

/**
 *	@brief MDM_CS�̏o�͏���
 *
 *	@retval �Ȃ�
 */
void MdmcsWrite( int data)
{
	int d = Mdmcs_reg_buf;
	d &= ~1;
	d |= (data&1);
	Mdmcs_reg_buf = d;
	RegWrite(MODEM_REG, data);
}
	
/**
 *	@brief IO�f�[�^�̓��͏���
 *
 *	@param [int address]  �A�h���X
 *
 *	@retval �Ȃ�
 */
unsigned int RegRead( unsigned int address){
#ifdef windows
	return 0;
#else
	char str[100];
	if (address > REG_MAX) {
		sprintf("RegRead","�A�h���X�G���[ %X",address);
		DebugPrint("", str, 0);
		return;
	}
	int d = 0;
	d = *(uint16_t *)(BASE_REG + address);
volatile	int	i;
	for(i=0; i<4; i++);
///    if(( address < 0 )||( address > 0xfff )||( address & 1 )){
//    	char str[32];
//        sprintf(str,"rd %04x", address );
//        DebugPrint("", str, 1);
//    }
	return d;
#endif
}

/**
 *	@brief IO�f�[�^�̏o�͏���
 *
 *	@param [int address]  �A�h���X
 *	@param [int data]     �f�[�^
 *
 *	@retval �Ȃ�
 */
void RegWrite( unsigned int address, int data){
#ifndef windows
	char str[100];
	if (address > REG_MAX) {
		sprintf("RegWrite","�A�h���X�G���[ %X",address);
		DebugPrint("", str, 0);
		return;
	}
		
	*(uint16_t *)(BASE_REG + address) = data;
volatile	int	i;
	for(i=0; i<4; i++);
//	char str[32];
//  sprintf(str, "wr %04x %04x", address, data );
//  DebugPrint("", str, 1);
#endif
}

/**
 *	@brief �s�����pRTC�̃p���[�I�����Z�b�g�����iIOEXT�d���N����40ms�ȏ�o�ߌ�ɌĂԂ��Ɓj
 *
 *	@param �Ȃ�
 *
 *	@retval �Ȃ�
 */
void InitRTC(void)
{
#ifndef windows
    int i;
/*
	char	str[16];
	printf("\n10: ");
	for(i=0; i<16; i++){
		sprintf(str,"%02X ", SPI_RTC_Read(i+0x10));
		printf("%s", str);
	}
	printf("\n30: ");
	for(i=0; i<3; i++){
		sprintf(str,"%02X ", SPI_RTC_Read(i+0x30));
		printf("%s", str);
	}
	printf("\n");
*/
	if( SPI_RTC_Read( RTC_REG_FLAG ) & 0x02 ){	/* VLF == 1 ?	*/
		/* yes... */
		SPI_RTC_Write( RTC_REG_RSV, 0x00);		/* 31h <- 00h	*/
		SPI_RTC_Write( RTC_REG_CNTL, 0x00);		/* 1Fh <- 00h	*/
		SPI_RTC_Write( RTC_REG_CNTL, 0x80);		/* 1Fh <- 80h	*/
		SPI_RTC_Write( RTC_REG_POR6_1, 0xD3);	/* 60h <- D3h	*/
		SPI_RTC_Write( RTC_REG_POR6_2, 0x03);	/* 66h <- 03h	*/
		SPI_RTC_Write( RTC_REG_POR6_3, 0x02);	/* 6Bh <- 02h	*/
		SPI_RTC_Write( RTC_REG_POR6_4, 0x01);	/* 6Bh <- 01h	*/

		/* ������2ms�ȏ�҂����� */
		for(i=0; i<0xffff; i++)	/* PLIB�킩��Ȃ��̂ŉ� */
			;
	}
	
	TIME_INFO t;
	LoadRTC(&t);/* �s�����pRTC����ǂݏo�� */
	SetTime(&t);/* PIC����RTC�Ɏ����ݒ菈�� */

#endif
}

	
/**
 *	@brief �s�����pRTC����ǂݍ��ޏ���
 *
 *	@param [TIME_INFO *t] �������\���̂̃|�C���^
 *
 *	@retval �Ȃ�
 */
void LoadRTC(TIME_INFO *t)
{
#ifndef windows
	unsigned char rxdata[10];

	SPI_RTC_ReadSTR( RTC_REG_SEC, rxdata, 7);
	t->sec				= rxdata[0];
	t->min				= rxdata[1];
	t->hour				= rxdata[2];
	t->holiday_week.week= rxdata[3];
	t->day				= rxdata[4];
	t->month			= rxdata[5];
	t->year[1]			= rxdata[6];
	t->year[0]			= 0x20;
#endif
	printf("LoadRTC�@%02X%02X/%02X/%02X(%d) %02X:%02X:%02X\n"
		, t->year[0], t->year[1]
		, t->month, t->day, t->holiday_week.week
		, t->hour, t->min	, t->sec);
}

/**
 *	@brief �s�����pRTC�ɏ������ޏ���
 *
 *	@param [TIME_INFO *t] �������\���̂̃|�C���^
 *
 *	@retval �Ȃ�
 */
void SaveRTC(TIME_INFO *t)
{
#ifndef windows
	SPI_RTC_Write( RTC_REG_CALIB, 0xA8);
	SPI_RTC_Write( RTC_REG_TUNE,  0x00);
	SPI_RTC_Write( RTC_REG_RSV,   0x00);
	SPI_RTC_Write( RTC_REG_IRQ,   0x00);

	SPI_RTC_Write( RTC_REG_EXT,   0x04);
	SPI_RTC_Write( RTC_REG_FLAG,  0x00);
	SPI_RTC_Write( RTC_REG_CNTL,  0x40);

	SPI_RTC_Write( RTC_REG_SEC,   t->sec);
	SPI_RTC_Write( RTC_REG_MIN,   t->min);
	SPI_RTC_Write( RTC_REG_HOUR,  t->hour);
	SPI_RTC_Write( RTC_REG_WEEK,  t->holiday_week.week);
	SPI_RTC_Write( RTC_REG_DAY,   t->day);
	SPI_RTC_Write( RTC_REG_MONTH, t->month);
	SPI_RTC_Write( RTC_REG_YEAR,  t->year[1]);
	
	SPI_RTC_Write( RTC_REG_CNTL,  0x00);
#endif
	printf("SaveRTC�@%02X%02X/%02X/%02X(%d) %02X:%02X:%02X\n"
		, t->year[0], t->year[1]
		, t->month, t->day, t->holiday_week.week
		, t->hour, t->min	, t->sec);
}

#ifndef windows //yamazaki

/**
 *	@brief SPI - EEPROM�ւ̏������ݏ���
 *
 *	@param [unsigned int address] �Ώ۔Ԓn
 *	@param [unsigned char data]   �������݃f�[�^(8bit��)
 *
 *	@retval �Ȃ�
 */
void				SPI_E2ROM_Write( unsigned int address, unsigned char data)
{
#ifndef windows
/*
RDSR	1+1byte	05h, (RX)status
 while(WIP(bit0)==1){}	�������݃C���v���Z�X�҂�
WREN	1byte	06h
WRITE	3byte	02h | A8<<3, A7�`A0, data
WRDI	1byte	04h
*/
	int rdat;

	do {
		SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_ENABLE );
		SPI_SendData( 0x05 );								/* RDSR */
		rdat = SPI_RcvData();
		SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_DISABLE );
	} while( rdat & 1 );
	
	SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_ENABLE );
	SPI_SendData( 0x06 );									/* WREN */
	SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_DISABLE );

	SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_ENABLE );
	SPI_SendData( 0x02 | ((address & 0x100) >> (8-3)) );	/* WRITE */
	SPI_SendData( address & 0xff );							/* address A7�`A0 */
	SPI_SendData( data );									/* data */
	SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_DISABLE );
	
	SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_ENABLE );
	SPI_SendData( 0x04 );									/* WRDI */
	SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_DISABLE );
#endif
}

/**
 *	@brief SPI - EEPROM����̓ǂݏo������
 *
 *	@param [unsigned int address] �Ώ۔Ԓn
 *
 *	@retval �ǂݏo���f�[�^
 */
unsigned char	SPI_E2ROM_Read( unsigned int address)
{
	int rdat = 0;

#ifndef windows
	SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_ENABLE );
	SPI_SendData( 0x03 | ((address & 0x100) >> (8-3)) );	/* READ */
	SPI_SendData( address & 0xff );							/* address A7�`A0 */
	rdat = SPI_RcvData();									/* RX(data) */
	SPI_CsWrite( SPI_CS_EEPROM, SPI_CS_DISABLE );
#endif
	return	rdat;
}


/*
 *===========================================================================================
 *					����	�֐�
 *===========================================================================================
 */


/**
 *	@brief SPI�o�X�f�o�C�X��CS�I������
 *
 *	@param [unsigned char dst]  �Ώۃf�o�C�X  1:RTC(SPI_CS_RTC,        2:EEPROM(SPI_CS_EEPROM)
 *	@param [unsigned char mode] CS���        0:��I��(SPI_CS_DISABLE) 1:�I��(SPI_CS_ENABLE)
 *
 *	@retval �Ȃ�
 */
static void				SPI_CsWrite( unsigned char dst, unsigned char mode )
{
#ifndef windows
	int	dat;
	
	switch(dst){
		case 1:	/* RTC    (SPI_RTC_CS: bit2) */
			dat = (RegRead( SPI_REG ) & 0xfffb) | ((mode & 1) << 2);
			RegWrite( SPI_REG, dat);
			break;
		case 2:	/* EEPROM (SPI_ROM_CS: bit3) */
			dat = (RegRead( SPI_REG ) & 0xfff7) | ((mode & 1) << 3);
			RegWrite( SPI_REG, dat);
			break;
		default:
			break;
	}
#endif
}

/**
 *	@brief SPI�o�X�f�o�C�X�ւ̑��M����
 *
 *	@param [unsigned char data] ���M�f�[�^
 *
 *	@retval �Ȃ�
 */
static void				SPI_SendData( unsigned char data)
{
#ifndef windows
	int		i, inidat, bit;
	
	inidat = RegRead( SPI_REG ) & 0xfffc;
	RegWrite( SPI_REG, inidat );
	for( i=8; i>0; i--){
		bit = (data & 0x80) >> 7;
		data = data << 1;
		RegWrite( SPI_REG, inidat        | bit);	/* SCK 0 */
		RegWrite( SPI_REG, inidat | 0x02 | bit);	/* SCK 1 */
	}
	RegWrite( SPI_REG, inidat );
#endif
}

/**
 *	@brief SPI�o�X�f�o�C�X����̎�M����
 *
 *	@param �Ȃ�
 *
 *	@retval ��M�f�[�^
 */
static unsigned char	SPI_RcvData( void )
{
	unsigned char	rdat = 0;
#ifndef windows
	int		i, inidat, bit;
	
	rdat = 0;
	inidat = RegRead( SPI_REG ) & 0xfffc;
	RegWrite( SPI_REG, inidat );
	for( i=8; i>0; i--){
		rdat = rdat << 1;
		RegWrite( SPI_REG, inidat | 0x02 );		/* SCK 1 */
		bit = (RegRead( SPI_REG ) & 0x100) >> 8;
		RegWrite( SPI_REG, inidat        );		/* SCK 0 */
		rdat = rdat | bit;
	}
#endif
	return	rdat;
}

/**
 *	@brief SPI - RTC�ւ̏������ݏ���
 *
 *	@param [unsigned int address] �Ώ۔Ԓn
 *	@param [unsigned char data]   �������݃f�[�^(8bit��)
 *
 *	@retval �Ȃ�
 */
static void				SPI_RTC_Write( unsigned int address, unsigned char data)
{
#ifndef windows
	SPI_CsWrite( SPI_CS_RTC, SPI_CS_ENABLE );
	SPI_SendData( address );        /* WRITE */
	SPI_SendData( data );			/* data */
	SPI_CsWrite( SPI_CS_RTC, SPI_CS_DISABLE );
#endif
}

/**
 *	@brief SPI - RTC����̓ǂݏo������
 *
 *	@param [unsigned int address] �Ώ۔Ԓn
 *
 *	@retval �ǂݏo���f�[�^
 */
static unsigned char	SPI_RTC_Read( unsigned int address)
{
	int rdat = 0;

#ifndef windows
	SPI_CsWrite( SPI_CS_RTC, SPI_CS_ENABLE );
	SPI_SendData( 0x80 | address );     /* READ */
	rdat = SPI_RcvData();				/* RX(data) */
	SPI_CsWrite( SPI_CS_RTC, SPI_CS_DISABLE );
#endif
	return	rdat;
}

/**
 *	@brief SPI - RTC����̘A���ǂݏo������
 *
 *	@param [unsigned int address] �Ώ۔Ԓn
 *	@param [unsigned char *data]  �ǂݏo���f�[�^�o�b�t�@�ւ̃|�C���^
 *	@param [int length]           �ǂݏo����
 *
 *	@retval �Ȃ�
 */
static void		SPI_RTC_ReadSTR( unsigned int address, unsigned char *data, int length)
{
#ifndef windows
	SPI_CsWrite( SPI_CS_RTC, SPI_CS_ENABLE );
	SPI_SendData( 0x80 | address );     /* READ */
	while( length>0 ){
		*data = SPI_RcvData();			/* RX(data) */
		data++;
		length--;
	}
	SPI_CsWrite( SPI_CS_RTC, SPI_CS_DISABLE );
#endif
}
#endif
