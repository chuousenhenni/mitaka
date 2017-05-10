/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	monitor.c
 *	�T�v
 *  �Ď��Ր��䕔
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
 
enum {/* �Ď��Ց��M�����Ɏg�p�����ԕϐ� */
	SEND_MONITOR_IDLE_STAGE = 0,
	PRE_DELAY_SET_STAGE, /* kaji20170310 */
	PRE_DELAY_STAGE, /* kaji20170310 */
	CS_ON_STAGE,
	SEND_WAIT_STAGE1,
	SEND_MONITOR_STAGE,
	WAITING_SEND_END_STAGE,
	SEND_WAIT_STAGE2,
	CS_OFF_STAGE,
};

#define MONITOR_TIMEOUT_VALUE (30) /* �Ď��Վ�M�^�C���A�E�g����l(�b) */

/*
 *===========================================================================================
 *					�����ϐ���`
 *===========================================================================================
 */

static int monitor_ms_timer;/* ms�^�C�} */
static int monitor_sec_timer;/* �b�^�C�} */
static int check_delay_timer;/* �҂��^�C�} */
static int inhibit_timer;/* ���M�֎~�p�^�C�} */
static int inhibit_flag;/* ���M�֎~���͂P�ƂȂ� */

static int send_request_flag;/* �^�p��~�A�������M�v���t���O */
static int send_request_data;/* �^�p��~�A�������M�v���f�[�^ */

static int monitor_command_rcv_count;/* ����w�߃R�}���h��M�� */
static int bef_monitor_command_rcv_count;/* ����w�߃R�}���h��M�� */
static int snd_count;/* ���M�� */
static int each_snd_count[3];/* �w���X,�^�p��~�A�^�p��~�����̑��M�� */

static int check_monitor_timer;/* �Ď��Վ�M�^�C���A�E�g����p�^�C�} */
static int monitor_timeout_value;/* �Ď��Վ�M�^�C���A�E�g�l */
int monitor_timeout_flag;/* �Ď��Վ�M�^�C���A�E�g�t���O */
static int bef_monitor_timeout_flag;/* �O��̊Ď��Վ�M�^�C���A�E�g�t���O */
static int check_monitor_led_ms_timer;/* �Ď��՗p��1ms���ɍX�V����^�C�} */

static int bef_btn_status;/* �O��̃{�^���̏�Ԃ�ێ� */
int btn_status;/* �{�^���̏�Ԃ�ێ� */
static int buzzer_stop_flg;/* �u�U�[��~�t���O */
static int maintenance_flg;/* �ێ�{�^���������ꂽ�t���O */

static int monitor_bef_led_status[ MONITOR_LED_MAX_COUNT];/* �O���LED��� */
int monitor_led_status[ MONITOR_LED_MAX_COUNT];/* LED��� */
static int monitor_led_toggle_status;/* LED �_�Ń��[�h�̂Ō��݂̏�� */
static int monitor_led_toggle_count;/* LED �_�Ń��[�h��؂�ւ���܂ł̉� */
static int check_monitor_led_sec_timer;/* �Ď���LED����p�^�C�}�X�V */

int monitor_command_flg ;/* �^�p�w�߃{�^�������ꂽ�t���O */
static int bef_monitor_command_flg ;/* �O��^�p�w�߃{�^�������ꂽ�t���O */
MONITOR_COMMAND monitor_command;/* ����@����̐���w�߂̓d��  */
static MONITOR_OPERATE_COMMAND monitor_operate_command;/* ����@�ւ̉^�p�w�߂̓d��  */
static int monitor_send_operate_stage_no;

static int monitor_buzzer_status;/*  �x��u�U�[���*/

static int monitor_request_status;/*  ���M�v���̎��(�^�p��~����:0,�^�p��~:1) */
static int monitor_status;/* ��M�X�e�[�^�X(�^�p��~����:0,�^�p��~:1,�����m��҂����:2) */
static int hoshu_status;/* �����A�̕ێ�{�^����Ԃ�ێ� */

/*
 *===========================================================================================
 *					�O���ϐ���`
 *===========================================================================================
 */

//extern int send_com3_p;
extern HANDLE hComm1;       /* �V���A���|�[�g�̃n���h�� */
extern void LoadParam(void);
extern PARAM param;

/*
 *===========================================================================================
 *					����	�֐���`
 *===========================================================================================
 */

static void RcvMonitorSrv(void);/* ����@����Ď��Ղւ̃f�[�^��M���� */
static void CheckStatus(void);/* ��Ԕ��菈�� */
static void CheckBtn(void);/* �e��{�^�����͔��菈�� */
static void CheckContStatus(void);/* ����@�̏�Ԃ��`�F�b�N���鏈�� */
static int CheckContStatusSub(int i, int *err_status);/* �e����@�̏�Ԃ��`�F�b�N���鏈�� */
static void MonitorLEDInit(void);
static void MonitorLEDSrv(void);/* �Ď��Ղ�LED���䏈�� */
static void SendOperateCommand(int flag);/* �^�p�w�߂̑��M���� */
static void SendOperateCommandSrv(void);/* ����@�ւ̉^�p�w�߂̎��ۂ̑��M���� */
static void MonitorCommandSrv(void);/* �Ď��Ղւ̐���w�ߎ�M���� */

/*
 *===========================================================================================
 *					�O���[�o���֐���`
 *===========================================================================================
 */

void MonitorInit(void);
void MonitorSrv(void);/* �Ď��Ղ̐��䏈�� */
void TimerIntMonitor(int count);/* �Ď��Ղ̃^�C�}���荞�ݏ��� */
void MonitorCommandDisp(void);/* �Ď��Ղւ̐���w�ߕ\������ */
void MonitorStatusDisp(void);/* �X�e�[�^�X�̕\������ */

/*
 *===========================================================================================
 *					�O��	�֐���`
 *===========================================================================================
 */

extern void SendCom3(HANDLE h, unsigned char *p, int size);
extern int ChkSendCom3(void);

/*
 *===========================================================================================
 *					�O���[�o���֐�
 *===========================================================================================
 */

/**
 *	@brief �Ď��Ղ̃^�C�}���荞�ݏ���
 *
 *	@retval �Ȃ�
 */
void TimerIntMonitor(int count)
{
	
	monitor_ms_timer += count;
	check_delay_timer += count;/* �҂��^�C�} */
	
	inhibit_timer+= count;/* ���M�֎~�p�^�C�} */
	if (inhibit_timer > 5*1000) {
		/* ���M��5�b�o�ߌォ�玟����M�܂ł͑��M�֎~�Ƃ��� */
		inhibit_flag = 1;/* ���M�֎~���͂P�ƂȂ� */
	}
	
	check_monitor_led_ms_timer+= count;/* �Ď��՗p��1ms���ɍX�V����^�C�} */
	if (monitor_ms_timer >= 1000) {
		monitor_ms_timer = 0;
		monitor_sec_timer++;
		check_monitor_timer++;/* �Ď��Վ�M�^�C���A�E�g����p�^�C�}�X�V */
		if (check_monitor_timer > monitor_timeout_value) {
			monitor_timeout_flag = 1;
		}
		check_monitor_led_sec_timer++;/* �Ď���LED����p�^�C�}�X�V */
	}
}

/**
 *	@brief �Ď��Ղ̏���������
 *
 *	@retval �Ȃ�
 */
void MonitorInit(void)
{
	int i;
//	memset(&monitor_command, 0, sizeof(monitor_command));/* 20170206 */
	/* ����@����̊Ď��Ղւ̐���w�߂̓d���t�H�[�}�b�g */
	monitor_command.h.no = 0;/* �K�i�ԍ� 00H�Œ� */
	monitor_command.h.dst = 0x10;/* ����A�h���X 80H�Œ� */
	monitor_command.h.src = 1;/* ���M���A�h���X  01H�Œ� */
	monitor_command.h.sub_adr = 0;/* �T�u�A�h���X 00H�Œ� */
	monitor_command.h.priority = 2;/* �D�惌�x�� 02H�Œ� */
	monitor_command.h.s_no = 0;/* �ʔ� 00H�Œ� */
	monitor_command.h.contoroler_no = 0x19;/* �[����� 19H */
	monitor_command.h.info = 0x11;/* ����� 11H */
	monitor_command.h.div_no = 0x81;/* �����ԍ� 81H */
//	monitor_command.h.length = 162;/* �f�[�^�� 162(A2H) */
	monitor_command.h.length = 8+6*8;/* �f�[�^�� 162(A2H) */
	monitor_command.t.holiday_week.holiday = 0;/* �x�� */
	monitor_command.t.holiday_week.week = 0;/* �A�j�� */
//	memset(monitor_command.reserved1, 0, sizeof(monitor_command.reserved1));
	memset(monitor_command.command, 0, sizeof(monitor_command.command));
//	memset(monitor_command.reserved2, 0, sizeof(monitor_command.reserved2));
	for ( i = 0; i < CONTROLER_MAX; i++) {
		//yamazaki monitor_command.command[i][1] = 0x20;/* �������ُ�Ƃ��Ă��� */
	}

	/* �Ď��Ղ��琧��@�ւ̓d�� �^�p�w�߂̓d���t�H�[�}�b�g */
	monitor_operate_command.h.no = 0;/* �K�i�ԍ� 00H�Œ� */
	monitor_operate_command.h.dst = 1;/* ����A�h���X 01H�Œ�i���g�p�j */
	monitor_operate_command.h.src = 1;/* ���M���A�h���X  01H�Œ� */
	monitor_operate_command.h.sub_adr = 0;/* �T�u�A�h���X 00H�Œ� */
	monitor_operate_command.h.priority = 2;/* �D�惌�x�� 02H�Œ� */
	monitor_operate_command.h.s_no = 0;/* �ʔ� 00H�Œ� */
	monitor_operate_command.h.contoroler_no = 0x19;/* �[����� 19H */
	monitor_operate_command.h.info = 0x81;/* ����� 81H */
	monitor_operate_command.h.div_no = 0x81;/* �����ԍ� 81H */
//	monitor_operate_command.h.length = 162;/* �f�[�^�� 162(A2H) */
	monitor_operate_command.h.length = 9;/* �f�[�^�� 56(38H) */
	monitor_operate_command.t.holiday_week.holiday = 0;/* �x�� */
	monitor_operate_command.t.holiday_week.week = 0;/* �A�j�� */
	monitor_operate_command.command = 0;
	
	monitor_command_flg  = 0;/* �^�p�w�߃{�^�������ꂽ�t���O */
	bef_monitor_command_flg  = 0;

	MonitorCommandDisp();/* �Ď��Ղւ̐���w�ߕ\������ */
	monitor_command_rcv_count = 0;
	bef_monitor_command_rcv_count = 0;
	snd_count = 0;/* ���M�� */
	for ( i = 0 ;i < 3; i++) {
		each_snd_count[i] = 0;/* �w���X,�^�p��~�A�^�p��~�����̑��M�� */
	}
	check_monitor_timer = 0;/* �Ď��Վ�M�^�C���A�E�g����p�^�C�} */
	monitor_timeout_flag = 0;/* �Ď��Վ�M�^�C���A�E�g�t���O */
	bef_monitor_timeout_flag = 0;/* �O��̊Ď��Վ�M�^�C���A�E�g�t���O */
	monitor_timeout_value = MONITOR_TIMEOUT_VALUE;/* �Ď��Վ�M�^�C���A�E�g����l(�b) */

	/* �ȉ��̓e�X�g�p */
	//monitor_led_status[0] = LED_STATUS_TOGGLE;

	btn_status = 0;/* �{�^���̏�Ԃ�ێ� */
	bef_btn_status = btn_status;/* �O��̃{�^���̏�Ԃ�ێ� */
	buzzer_stop_flg = 0;/* �u�U�[��~�t���O */
	maintenance_flg = 0;/* �ێ�{�^���������ꂽ�t���O */
	monitor_buzzer_status = 0;/*  �x��u�U�[���*/

	monitor_request_status = 0;/*  ���M�v���̎��(�^�p��~����:0,�^�p��~:1) */
	monitor_status = 0;/* ��M�X�e�[�^�X(�^�p��~����:0,�^�p��~:1,�����m��҂����:2) */
	hoshu_status = 0;/* �����A�̕ێ�{�^����Ԃ�ێ� */
	monitor_send_operate_stage_no = SEND_MONITOR_IDLE_STAGE;

	monitor_ms_timer = 0;/* ms�^�C�} */
	monitor_sec_timer = 0;/* �b�^�C�} */
	inhibit_timer = 0;/* ���M�֎~�p�^�C�} */
	inhibit_flag = 0;/* ���M�֎~���͂P�ƂȂ� */
	send_request_flag = 0;/* �^�p��~�A�������M�v���t���O */
	send_request_data = 0;/* �^�p��~�A�������M�v���f�[�^ */

	SetDefault();/* �f�B�t�H���g�p�����[�^�ݒ� kaji20170316 */
//	param.debug_flg = 0xf;/* �f�o�b�O�p�̕\�����s���ꍇ��0�ȊO���Z�b�g���� */
	//param.linkage_status = DEFAULT_LINKAGE; /* �A���ݒ� */
//	param.mdmcs_delay_time = 20;/* MDM_CS�o�͂̒x������(ms) */
//	param.reset_count = 0;
	
	LoadParam();
	param.linkage_status = 0x11111111; /* �A���ݒ�(����@������炤�悤�ɕύX) */
//	param.reset_count++;
//	SaveParam();
	MdmcsWrite(0);
	MonitorLEDInit();
	btn_status = MonitorBtnRead();/* �Ď��Ճ{�^���ǂݍ��ݏ��� */
	if ((btn_status & MAINTENANCEL_BIT) != 0) {
		maintenance_flg = 1;/* �ێ�{�^���������ꂽ��� */
	} else {
		maintenance_flg = 0;/* �ێ�{�^���������ꂽ��� */
	}
}

/**
 *	@brief �Ď��Ղ̐��䏈��
 *
 *	@retval �Ȃ�
 */
void MonitorSrv(void)
{
	RcvMonitorSrv();/* ����@����Ď��Ղւ̃f�[�^��M���� */
	CheckStatus();/* ��Ԕ��菈�� */
	CheckBtn();/* �e��{�^�����͔��菈�� */
	MonitorLEDSrv();/* �Ď��Ղ�LED���䏈�� */
	
	//if ((inhibit_flag == 0) && (send_request_flag == 1)) {
	//	/* ���M�֎~���ԂłȂ�������^�p��~�A�������M�v���𑗐M���� */
	//	send_request_flag = 0;
	//	SendOperateCommand(monitor_command_flg);/* �^�p�w�߂̑��M���� */
	//}
	SendOperateCommandSrv();/* ����@�ւ̉^�p�w�߂̎��ۂ̑��M���� */
	
}

/*
 *===========================================================================================
 *					�����֐�
 *===========================================================================================
 */

/**
 *	@brief �Ď��Ղ�LED����������
 *
 *	@retval �Ȃ�
 */
static void MonitorLEDInit(void)
{
	int i;

	check_monitor_led_sec_timer = 0;/* �Ď���LED����p�^�C�}�X�V */

	monitor_led_toggle_count = TOGGLE_COUNT_MAX;
	monitor_led_toggle_status = LED_OFF;
	for ( i = 0 ; i < MONITOR_LED_MAX_COUNT; i++) {
		monitor_bef_led_status[i] = LED_STATUS_OFF;
		monitor_led_status[i] = LED_STATUS_OFF;
	}
}

/**
 *	@brief ��Ԕ��菈��
 *
 *	@retval �Ȃ�
 */
static void CheckStatus(void)
{
	int i;
	char str[256];

	if (bef_monitor_command_rcv_count != monitor_command_rcv_count){
		CheckContStatus();/* ����@�̏�Ԃ��`�F�b�N */
	}
	bef_monitor_command_rcv_count = monitor_command_rcv_count;
	
	if (monitor_timeout_flag == 1) {
#if 1
/* kaji20170223 */
		for (i = 0; i < CONTROLER_MAX ; i++) {
			monitor_led_status[2 * i + 0] = LED_STATUS_OFF;
			if (i == CONTA_ADDRESS-1) {
				monitor_led_status[2 * i + 1] = LED_STATUS_ORANGE_TOGGLE;
			} else {
				monitor_led_status[2 * i + 1] = LED_STATUS_OFF;
			}
		}
		if ((maintenance_flg == 0) && (buzzer_stop_flg == 0) &&  (hoshu_status == 0)) {
			monitor_buzzer_status = 1;/*  �x��u�U�[���*/
			MonitorBuzzerWrite(1);/* �x���u�U�[ */
		}
		monitor_led_status[ALARM_LAMP_LED] = LED_STATUS_ORANGE;/* 20170205 */
/* kaji20170223 */
#else
		for (i = 0; i < CONTROLER_MAX ; i++) {
			int d = (param.linkage_status >> (4*(CONTROLER_MAX - i - 1)))&0xf;
			if (d != 0) {
				/* �L���Ȑ���@�̉���ُ� ��_�ł�ݒ肷�� */
				//printf("����ُ� ��_�� %d\n",2 * i + 1);
				monitor_led_status[2 * i + 1] = LED_STATUS_ORANGE_TOGGLE;
				if ((maintenance_flg == 0) && (buzzer_stop_flg == 0) &&  (hoshu_status == 0)) {
					monitor_buzzer_status = 1;/*  �x��u�U�[���*/
					MonitorBuzzerWrite(1);/* �x���u�U�[ */
				}
				monitor_led_status[ALARM_LAMP_LED] = LED_STATUS_ORANGE;/* 20170205 */
			}
		}
#endif
	}
	
	if ((monitor_buzzer_status == 1 ) && (hoshu_status == 1)) {
		monitor_buzzer_status = 0;/*  �x��u�U�[���*/
		MonitorBuzzerWrite(0);/* �x���u�U�[ */
	}
	
	if (bef_monitor_timeout_flag != monitor_timeout_flag) {
		if (monitor_timeout_flag == 1) {
			/* �^�p�Ǘ�PC�ԒʐM�ُ� */
			sprintf(str, "�^�p�Ǘ�PC�ԒʐM�ُ�");
			//monitor_timeout_flag = 0;
			//check_monitor_timer =0;
		} else {
			/* �^�p�Ǘ�PC�ԒʐM���� */
			sprintf(str, "�^�p�Ǘ�PC�ԒʐM����");
		}
		DebugPrint("", str, 1);
	}
	bef_monitor_timeout_flag = monitor_timeout_flag;
}


/**
 *	@brief �e��{�^�����͔��菈��
 *
 *	@retval �Ȃ�
 */
static void CheckBtn(void)
{
	btn_status = MonitorBtnRead();/* �Ď��Ճ{�^���ǂݍ��ݏ��� */
	char str[256];
	if (bef_btn_status != btn_status) {
		sprintf(str, "bef_sw(%.02X)->sw(%.02X)", bef_btn_status, btn_status);
		DebugPrint("", str, 0);
		
		if ((btn_status & MAINTENANCEL_BIT)) {
			/* �ێ�{�^���������ꂽ��� */
			maintenance_flg = 1;
			if (monitor_buzzer_status == 1) {
				monitor_buzzer_status = 0;/*  �x��u�U�[���*/
				MonitorBuzzerWrite( 0);/* �Ď��Ճu�U�[�ւ̒�~�o�͏��� */
				printf("�x��u�U�[���~���� 1\n");
			}
			/*  �x��u�U�[���*/
		} else {
			/* �ێ�{�^����������Ă��Ȃ���� */
			maintenance_flg = 0;
		}
		
		if (monitor_buzzer_status == 1) {/*  �x��u�U�[��� */
			//if ((bef_btn_status & MAINTENANCEL_BIT) != (btn_status & MAINTENANCEL_BIT)) {
			//	if ((btn_status & MAINTENANCEL_BIT) != 0) {
			//		/* �ێ�{�^���������ꂽ */
			//		maintenance_flg = 1;
			//		/* �x��u�U�[���~���� */
			//		monitor_buzzer_status = 0;/*  �x��u�U�[���*/
			//		MonitorBuzzerWrite( 0);/* �Ď��Ճu�U�[�ւ̒�~�o�͏��� */
			//		printf("�x��u�U�[���~���� 1\n");
			//	} else {
			//		/* �ێ�{�^���������ꂽ */
			//		maintenance_flg = 0;
			//	}
			//}
			if ((btn_status & BUZZER_STOP_BIT) != 0) {
				/* �u�U�[��~�{�^���������ꂽ */
				buzzer_stop_flg = 1;
				printf("�x��u�U�[���~���� 2\n");
				/* �x��u�U�[���~���� */
				monitor_buzzer_status = 0;/*  �x��u�U�[���*/
				MonitorBuzzerWrite( 0);/* �Ď��Ճu�U�[�ւ̒�~�o�͏��� */
			}
		}
		
		if ((bef_btn_status & CAMMAND_BIT) != (btn_status & CAMMAND_BIT)) {
			/* �^�p��~�{�^�����ω����� */
			if ((btn_status & CAMMAND_BIT) != 0) {
				/* �^�p��~�{�^���������ꂽ��� */
				if (monitor_status == 0) {
					/* �^�p������ԂȂ̂ŉ^�p��~�v�����Z�b�g */
					send_request_flag = 1;/* �^�p��~�A�������M�v���t���O */
					monitor_command_flg = 1;/* �^�p��~�v�� */
					printf("�^�p��~�{�^���������ꂽ��Ԃɕω����� �^�p��~�v������\n");
				} else if (monitor_status == 1) {
					/* �^�p��~��ԂȂ̂ŉ^�p��~�v�����Z�b�g */
					send_request_flag = 1;/* �^�p��~�A�������M�v���t���O */
					monitor_command_flg = 2;/* �^�p��~�����v�� */
					printf("�^�p��~�{�^���������ꂽ��Ԃɕω����� �^�p��~�����v������\n");
				} else {
					/* �����҂���ԂȂ̂łȂɂ����Ȃ� */
					printf("�^�p��~�{�^���������ꂽ��Ԃɕω����� �����҂���ԂȂ̂łȂɂ����Ȃ�\n");
				}
			} else {
				
				/* �^�p��~�{�^���������ꂽ��� */
				printf("�^�p��~�{�^���������ꂽ��Ԃɕω�����\n");
				send_request_data = 0;/* �^�p��~�A�������M�v���f�[�^ */
			}
			monitor_led_status[UNYOU_TEISHI_LED] = LED_STATUS_ORANGE_TOGGLE;/* �_�ŏ�Ԃɂ��� */
		}
		/* �O��̃{�^���̏�Ԃ�ێ� */
		bef_btn_status = btn_status;/* �O��̃{�^���̏�Ԃ�ێ� */
	}
}


/**
 *	@brief ����@�̏�Ԃ��`�F�b�N���鏈��
 *
 *	@retval �Ȃ�
 */
static void CheckContStatus(void)
{
	int i;
	int flg;
	int total_count = 0;
	int count = 0;
	int err_status = 0;
	int err_count = 0;
	char str[256];
	
	/* linkage_status��div_no������o�� */
	param.linkage_status = LinkageUnPack(monitor_command.h.div_no);
	for (i = 0; i < CONTROLER_MAX ; i++) {
		int d = (param.linkage_status >> (4*(CONTROLER_MAX - i - 1)))&0xf;
		if (d != 0) {
			total_count++;
			
			flg = CheckContStatusSub(i , &err_status);/* �e����@�̏�Ԃ��`�F�b�N���鏈�� */
			if (flg) {
				count++;
			}
			if (err_status) {
				err_count++;
			}
		}
	}
//	printf("count =%d, total_count=%d\n",count, total_count);
	if (count == total_count) {
		/* ��v�����Ƃ������Ƃ͑S�Ă̐���@���^�p��~�A�܂��͒�~�����ɂȂ����Ƃ������� */
		sprintf(str, "count == total_count %d,%d\n",total_count, monitor_command_flg);
		DebugPrint("", str, 1);
		if (monitor_command_flg == 1) {
			/* �^�p��~ �S����@����̉����ŉ^�p��~�ɂȂ��� */
			if (monitor_led_status[UNYOU_TEISHI_LED] != LED_STATUS_ORANGE) {
				monitor_led_status[UNYOU_TEISHI_LED] = LED_STATUS_ORANGE;
				monitor_status = 1;/* �^�p��~��Ԃɂ��� */
			}
		} else {
			/* �^�p��~���� �S����@����̉����ŉ^�p��~�����ɂȂ��� */
			if (monitor_led_status[UNYOU_TEISHI_LED] != LED_STATUS_OFF) {
				monitor_led_status[UNYOU_TEISHI_LED] = LED_STATUS_OFF;
				monitor_status = 0;/* �^�p��~������Ԃɂ��� */
			}
		}
	} else {
		/* �^�p��~��Ԃ������ł͂Ȃ���� */
		if(monitor_status != 2) {
			/*
				�^�p�A�^�p�����̂����ꂩ�̏�Ԃ̂͂��Ȃ̂�
				���������������̂͐���@�Ƀ��Z�b�g�ł����������̂��H
			*/
			printf("���������������̂͐���@�Ƀ��Z�b�g�ł����������̂��Hmonitor_status=%d\n", monitor_status);
			send_request_flag = 1;
			if (monitor_status == 0) {
				monitor_command_flg = 2;/* �^�p��~�����v�� */
			} else {
				monitor_command_flg = 1;/* �^�p��~�v�� */
			}
		}
		if (monitor_led_status[UNYOU_TEISHI_LED] != LED_STATUS_ORANGE_TOGGLE) {
			monitor_led_status[UNYOU_TEISHI_LED] = LED_STATUS_ORANGE_TOGGLE;/* �_�ŏ�Ԃɂ��� */
		}
	}
	
	if ((monitor_command.command[CONTA_ADDRESS - 1][1]&2) != 0) {/* 1:�ێ�/0:�ʏ� */
		hoshu_status = 1;
	} else {
		hoshu_status = 0;
	}
	if (err_count != 0) {
		if ((maintenance_flg == 0) && (buzzer_stop_flg == 0) && (hoshu_status == 0)) {
			monitor_buzzer_status = 1;	/* �x��u�U�[���*/
			MonitorBuzzerWrite(1);		/* �x��u�U�[ */
		}
		monitor_led_status[ALARM_LAMP_LED] = LED_STATUS_ORANGE;/* 20170205 */
	} else {
		buzzer_stop_flg = 0;		/* 20170214 */
		monitor_buzzer_status = 0;	/* 20170214 �x��u�U�[���*/
		MonitorBuzzerWrite(0);		/* 20170214 �x��u�U�[ */
		monitor_led_status[ALARM_LAMP_LED] = LED_STATUS_OFF;/* 20170205 */
	}
}

/**
 *	@brief �e����@�̏�Ԃ��`�F�b�N���鏈��
 *
 *	@retval �^�p��~�A�����w�߂Ƃ̑Ή��t���O
 */
static int CheckContStatusSub(int i, int * err_status)
{
	int flg = 0;
	if ((monitor_command_flg == 1) && ((monitor_command.command[i][0]&8) != 0)){
		/* �^�p��~ �[����� */
		flg = 1;
	}
	if ((monitor_command_flg == 0) && ((monitor_command.command[i][0]&8) == 0)){
		/* �^�p��~���� �[����� */
		flg = 1;
	}
	if ((monitor_command_flg == 2) && ((monitor_command.command[i][0]&8) == 0)){
		/* �^�p��~���� �[����� */
		flg = 1;
	}
	
#ifdef kaji20170208	//	���n�@�R�����g�A�E�g
	if ((
		(monitor_command.command[i][1]&0x20) != 0) /* ����ُ� */
		 || ((monitor_command.command[i][1]&4) != 0) /* �[���@�ُ� */
		 || (monitor_command.command[i][2] != 0) /* �ϕW���Ոُ� */
		 || (monitor_command.command[i][3] != 0) /* �����e�ُ� */
	)
	{
		/* ���炩�ُ̈킪����ꍇ�͉^�p���[�h ����*/
		*err_status = 1;
		monitor_led_status[2 * i] = LED_STATUS_OFF;
	} else {
#else
//	if ((
//		(monitor_command.command[i][1]&0x20) != 0) /* ����ُ� */
// //		 || ((monitor_command.command[i][1]&4) != 0) /* �[���@�ُ� */
// //		 || (monitor_command.command[i][2] != 0) /* �ϕW���Ոُ� */
// //		 || (monitor_command.command[i][3] != 0) /* �����e�ُ� */
//		 || ( ((monitor_command.command[i][0]&0x20) != 0)
//		 	&&((monitor_command.command[i][0]&0x07) == 0) )	/* �t�F�C��	*/
//	)
//	{
//		/* �[���������Ȃ��E�t�F�C���̏ꍇ�́@��|�@ */
//		*err_status = 1;
//		monitor_led_status[2 * i] = LED_STATUS_ORANGE;
//	} else {
/* kaji20170225 */
	if ( (monitor_command.command[i][1]&0x20) != 0) /* ����ُ� */
	{
		*err_status = 1;
		switch(monitor_command.command[i][0]) {
			case 0x01:/* �ʏ�(����) */
				monitor_led_status[2 * i] = LED_STATUS_GREEN;
				break;
			case 0x02:/* ��|(����) */
			case 0x04:/* �ψ�(����) */
			case 0x20:/* �t�F�[�� */
			monitor_led_status[2 * i] = LED_STATUS_ORANGE;
			break;
		default:
			monitor_led_status[2 * i] = LED_STATUS_OFF;/* �����킩��Ȃ���� */
			break;
		}
	} else {
#endif
		*err_status = 0;
		if ((monitor_command.command[i][0]&1) != 0) {
			/* �ʏ� �^�p���[�h �Γ_��*/
			//printf("�ʏ� �^�p���[�h �Γ_�� %d\n",i);
			monitor_led_status[2 * i] = LED_STATUS_GREEN;
		} else if ((monitor_command.command[i][0]&2) != 0) {
			/* ��| �^�p���[�h ��_��*/
			monitor_led_status[2 * i] = LED_STATUS_ORANGE;
		} else if ((monitor_command.command[i][0]&4) != 0) {
			/* �ψ� �^�p���[�h �ԓ_��*/
			monitor_led_status[2 * i] = LED_STATUS_RED;
		} else {
			/* �����t�F�C�� */
			/* ��| �^�p���[�h ��_��*/
			monitor_led_status[2 * i] = LED_STATUS_ORANGE;
		}
	}

	if (((monitor_command.command[i][1]&0x20) != 0) || (monitor_timeout_flag == 1)) {
		/* ����ُ� ��_��*/
		//printf("����ُ� ��_�� %d\n",2 * i + 1);
		monitor_led_status[2 * i + 1] = LED_STATUS_ORANGE_TOGGLE;
	}
	else if ((
		(monitor_command.command[i][1]&4) != 0) /* �[���@�ُ� */
		 || (monitor_command.command[i][2] != 0) /* �ϕW���Ոُ� */
		 || (monitor_command.command[i][3] != 0) /* �����e�ُ� */
	)
	{
		*err_status = 1;
		/* �ُ� �ԓ_��*/
		monitor_led_status[2 * i + 1] = LED_STATUS_RED_TOGGLE;
		if (maintenance_flg == 0) {
			/* �x��u�U�[��ON���� */
			//int d = *(char *) SWITCH_REG;
			//d |= ALARM_BUZZER_BIT;
			//*(char *) SWITCH_REG = d;
		}
	} else if ((monitor_command.command[i][0]&0x40) != 0) {
		/* �蓮 �[����� �Γ_��*/
		monitor_led_status[2 * i + 1] = LED_STATUS_GREEN_TOGGLE;
	} else {
		/* ���� */
#ifdef	kaji20170209
		monitor_led_status[2 * i + 1] = LED_STATUS_OFF;
#else
		monitor_led_status[2 * i + 1] = LED_STATUS_GREEN;
#endif
	}
	return flg;
}

/**
 *	@brief ����@����Ď��Ղւ̃f�[�^��M����
 *
 *	@retval �Ȃ�
 */
static void RcvMonitorSrv(void) {
	char str[256];
	unsigned char monitor_rcv_buf[RCV_BUFF_SIZE];

	
	while(!empty(MONITOR_QUEUE)) {
		unsigned char d = peek(MONITOR_QUEUE, 0);/* �擪�f�[�^�͂O���H */
		if ( d != 0) {
			dequeue(MONITOR_QUEUE);
			continue;
		}
		if (lenqueue(MONITOR_QUEUE) >= 10) {
			int monitor_wait_count = peek(MONITOR_QUEUE, 9) + 10;
			if (monitor_wait_count != (10+8+6*8)){
				dequeue(MONITOR_QUEUE);
			} else {
				//printf("%d\n",lenqueue(MONITOR_QUEUE));
				if (lenqueue(MONITOR_QUEUE) >= monitor_wait_count) {
				//printf("X");
					/* �f�[�^�p�P�b�g��M */
					int i;
					for(i = 0 ; i < monitor_wait_count; i++) {
						monitor_rcv_buf[i] = dequeue(MONITOR_QUEUE);/* ��M�f�[�^���o�� */
					}
					CONTROL_HEADER *h =(CONTROL_HEADER *)monitor_rcv_buf;
					if (h->dst == 1) {/* ����@A����Ď��Ղւ̐���w�� */
						DebugPrint("","RcvMonitorSrv �Ď��Ղւ̐���w�ߎ�M", 1);
						memmove(&monitor_command, monitor_rcv_buf, sizeof(MONITOR_COMMAND));
						MonitorCommandSrv();/* �Ď��Ղւ̐���w�ߎ�M���� */
					} else {
						//��M�G���[
						sprintf(str, "RcvMonitorSrv ����ʃG���[ %02X",h->info);
						DebugPrint("", str, 2);
					}
				} else {
					break;
				}
			}
		} else {
			break;
		}
	}
}

/**
 *	@brief �Ď��Ղւ̐���w�ߎ�M����
 *
 *	@retval �Ȃ�
 */
static void MonitorCommandSrv(void)
{
	
	MonitorCommandDisp();/* �Ď��Ղւ̐���w�ߕ\������ */
//	inhibit_timer = 0;/* ���M�֎~�p�^�C�} */
//	inhibit_flag = 0;/* ���M�֎~���͂P�ƂȂ� */
	check_monitor_timer = 0;
	monitor_timeout_flag = 0;
	if (monitor_command_rcv_count == 0) {
		/* �������Z�b�g���� */
		SetTime(&monitor_command.t);/* �����ݒ菈�� */
		SaveRTC(&monitor_command.t);/* �s�����pRTC�ɏ������� */
	}
	int setdata = 0;
	monitor_command_rcv_count++;
	if (send_request_flag == 1) {
		send_request_flag = 0;
		setdata = monitor_command_flg;
		monitor_status = 2;/* ��M�X�e�[�^�X(�^�p��~����:0,�^�p��~:1,�����m��҂����:2) */
	}
	
	SendOperateCommand(setdata);/* �w���X�𑗐M ����@�ւ̉^�p�w�߂̑��M���� */
}

/**
 *	@brief �Ď��Ղւ̐���w�ߕ\������
 *
 *	@retval �Ȃ�
 */
void MonitorCommandDisp(void)
{
	int i,j;
	char str[256];
	char str2[256];
	
	for(i = 0; i < 8; i++){
		sprintf(str2,"MontorCommandSrv %2d:",i+1);
		for(j = 0; j < 6; j++){
			sprintf(str,"%02X ",monitor_command.command[i][j]);
			strcat(str2,str);
		}
		if ((monitor_command.command[i][1]&0x20) != 0) {
			strcpy(str,"����ُ� ");
			strcat(str2,str);
		}
		if ((monitor_command.command[i][1]&4) != 0) {
			strcpy(str,"�[���@�ُ� ");
			strcat(str2,str);
		}
		if (monitor_command.command[i][2] != 0) {
			strcpy(str,"�W���Ոُ� ");
			strcat(str2,str);
		}
		if (monitor_command.command[i][3] != 0) {
			strcpy(str,"�����e�ُ� ");
			strcat(str2,str);
		}
		if (monitor_command.command[i][4] != 0) {
			strcpy(str,"���v�ُ� ");
			strcat(str2,str);
		}
		if ((monitor_command.command[i][0]&8) != 0) {
			strcpy(str,"�^�p��~ ");
			strcat(str2,str);
		}
		if ((monitor_command.command[i][0]&0x40) != 0) {
			strcpy(str,"�蓮 ");
			strcat(str2,str);
		}
		if ((monitor_command.command[i][0]&1) != 0) {
			strcpy(str,"�ʏ� ");
			strcat(str2,str);
		} else if ((monitor_command.command[i][0]&2) != 0) {
			strcpy(str,"��| ");
			strcat(str2,str);
		} else if ((monitor_command.command[i][0]&4) != 0) {
			strcpy(str,"�ψ� ");
			strcat(str2,str);
		}
		DebugPrint("", str2, 8);
	}
}

/**
 *	@brief �X�e�[�^�X�̕\������
 *
 *	@retval �Ȃ�
 */
void MonitorStatusDisp(void)/* �X�e�[�^�X�̕\������ */
{
	//printf("monitor_request_status = %d ���M�v���̎��(�^�p��~����:0,�^�p��~:1)\n",monitor_request_status);
	printf("monitor_status         = %d ��M�X�e�[�^�X(�^�p��~����:0,�^�p��~:1,�����m��҂����:2)\n",monitor_status);
	printf("monitor_buzzer_status  = %d �x��u�U�[���(������Ă��Ȃ�:0,������Ă���:1)\n",monitor_buzzer_status);
	printf("maintenance_flg        = %d �ێ�{�^��(������Ă��Ȃ�:0,������Ă���:1)\n",maintenance_flg);
	printf("hoshu_status           = %d �����A�̕ێ�{�^��(������Ă��Ȃ�:0,������Ă���:1)\n",hoshu_status);
	printf("rcv_count              = %d ����w�߃R�}���h��M��\n",monitor_command_rcv_count);
	printf("snd_count              = %d(%d,%d,%d) ����w�߃R�}���h���M��\n",snd_count,each_snd_count[0],each_snd_count[1],each_snd_count[2]);
}

/**
 *	@brief ����@�ւ̉^�p�w�߂̑��M����
 *
 * �^�p��~�X�C�b�`�������ꂽ��Ԃł́A�����v��_�����A
 * ����@�`�ɉ^�p��~�R�}���h�𑗂�A�h�ʏ�h���ێ�����Ƃ̂��Ƃł��B
 * �ĂсA�X�C�b�`����������A���������܂ŁA��~���ێ����邱�ƂɂȂ�܂��B
 *
 *	@retval �Ȃ�
 */
static void SendOperateCommand(int flag) {
	char str[256];
	
	snd_count++;
	each_snd_count[flag]++;
	SetNowTime(&monitor_operate_command.t);/* �����Z�b�g */
	monitor_operate_command.command = flag;
//	monitor_send_operate_stage_no = CS_ON_STAGE;
	monitor_send_operate_stage_no = PRE_DELAY_SET_STAGE;/* kaji20170310 */
	sprintf(str, "����@�ւ̉^�p�w�߂̑��M command = %02X",monitor_operate_command.command);
	DebugPrint("", str, 2);
}

/**
 *	@brief ����@�ւ̉^�p�w�߂̎��ۂ̑��M����
 *
 *	@retval �Ȃ�
 */
static void SendOperateCommandSrv(void)
{
	switch (monitor_send_operate_stage_no) {
	case SEND_MONITOR_IDLE_STAGE:/* �A�C�h�� */
		break;
	case PRE_DELAY_SET_STAGE:/* MDM_CS�o�͑҂� kaji20170310 */
		check_delay_timer = 0;/* �҂��^�C�} */
		monitor_send_operate_stage_no = PRE_DELAY_STAGE;
		break;
	case PRE_DELAY_STAGE:/* MDM_CS�o�͑҂� kaji20170310 */
		if (check_delay_timer > param.mdmcs_delay_time) {/* �҂��^�C�} */
			monitor_send_operate_stage_no = CS_ON_STAGE;
		}
		break;
	case CS_ON_STAGE:/* MDM_CS�̏o�͏��� */
		MdmcsWrite(1);
		check_delay_timer = 0;/* �҂��^�C�} */
		monitor_send_operate_stage_no = SEND_WAIT_STAGE1;
		DebugPrint("SendOperateCommandSrv", "CS_ON MdmcsWrite(1)", 4);
		break;
	case SEND_WAIT_STAGE1:/* MDM_CS�̏o�͏��� */
		if (check_delay_timer > param.mdmcs_delay_time) {/* �҂��^�C�} */
			monitor_send_operate_stage_no = SEND_MONITOR_STAGE;
		}
		break;
	case SEND_MONITOR_STAGE:/* �̏o�͏��� */
		DebugPrint("SendOperateCommandSrv", "���M�J�n", 4);
//		send_com3_p = 0;
		SendCom3(hComm1, (unsigned char *)&monitor_operate_command, sizeof(MONITOR_OPERATE_COMMAND));
		monitor_send_operate_stage_no = WAITING_SEND_END_STAGE;
		break;
	case WAITING_SEND_END_STAGE://���M�I���҂�
//		if (send_com3_p != sizeof(MONITOR_OPERATE_COMMAND)) {
//			SendCom3(hComm1, (unsigned char *)&monitor_operate_command, sizeof(MONITOR_OPERATE_COMMAND));
//		} else {
		if (ChkSendCom3() == 0){/* kaji20170310 */
			/*  ���M�I��*/
			check_delay_timer = 0;/* �҂��^�C�} */
			monitor_send_operate_stage_no = SEND_WAIT_STAGE2;
			DebugPrint("SendOperateCommandSrv", "���M�I��", 4);
		}
		break;
	case SEND_WAIT_STAGE2:/* MDM_CS�̏o�͏��� */
		if (check_delay_timer > param.mdmcs_delay_time) {/* �҂��^�C�} */
			MdmcsWrite(0);
			inhibit_timer = 0;/* ���M�֎~�p�^�C�} */
			inhibit_flag = 0;/* ���M�֎~���͂P�ƂȂ� */
			monitor_send_operate_stage_no = SEND_MONITOR_IDLE_STAGE;
		DebugPrint("SendOperateCommandSrv", "CS_OFF MdmcsWrite(0)", 4);
		}
		break;
	default:
		printf("SendOperateCommandSrv default error\n");
		break;
	}

}
/**
 *	@brief �Ď��Ղ�LED���䏈��
 *
 *	@retval �Ȃ�
 */
static void MonitorLEDSrv(void)
{
	int i;
	char str[100];
	
	for( i = 0 ; i < MONITOR_LED_MAX_COUNT; i++) {
		if (monitor_bef_led_status[i] != monitor_led_status[i]) {
			//printf("MonitorLEDSrv\n");
			switch (monitor_led_status[i]) {
			case LED_STATUS_OFF:
				sprintf(str, "monitor(%d) LED_STATUS_OFF",i);
				/* �������o�� */
				MonitorLedOut(i,LED_STATUS_OFF );
				break;
			case LED_STATUS_GREEN:
				sprintf(str, "monitor(%d) LED_STATUS_GREEN",i);
				/* �Γ_�����o�� */
				MonitorLedOut(i,LED_STATUS_GREEN );
				break;
			case LED_STATUS_ORANGE:
				sprintf(str, "monitor(%d) LED_STATUS_ORANGE",i);
				/* ��_�����o�� */
				MonitorLedOut(i,LED_STATUS_ORANGE );
				break;
			case LED_STATUS_RED:
				sprintf(str, "monitor(%d) LED_STATUS_RED",i);
				/* �ԓ_�����o�� */
				MonitorLedOut(i,LED_STATUS_RED );
				break;
			case LED_STATUS_GREEN_TOGGLE:
				sprintf(str, "monitor(%d) LED_STATUS_GREEN_TOGGLE",i);
				monitor_led_toggle_status = LED_OFF;
				check_monitor_led_ms_timer = 0;
				break;
			case LED_STATUS_ORANGE_TOGGLE:
				sprintf(str, "monitor(%d) LED_STATUS_ORANGE_TOGGLE",i);
				monitor_led_toggle_status = LED_OFF;
				check_monitor_led_ms_timer = 0;
				break;
			case LED_STATUS_RED_TOGGLE:
				sprintf(str, "monitor(%d) LED_STATUS_RED_TOGGLE",i);
				monitor_led_toggle_status = LED_OFF;
				check_monitor_led_ms_timer = 0;
				break;
			default :
				break;
			}
			DebugPrint("", str, 2);
			monitor_bef_led_status[i] = monitor_led_status[i];
		}
	}
	/* �_�œ���̐؂�ւ����� */
	if (check_monitor_led_ms_timer >= monitor_led_toggle_count ) {
		for( i = 0 ; i < MONITOR_LED_MAX_COUNT; i++) {
			switch(monitor_led_status[i]) {
			case LED_STATUS_GREEN_TOGGLE:
				if (monitor_led_toggle_status == LED_OFF) {
					sprintf(str, "Monitor GREEN_LED_ON  %d",check_monitor_led_ms_timer);
					/* �Γ_�����o�� */
					MonitorLedOut(i,LED_STATUS_GREEN );
				} else {
					sprintf(str, "Monitor GREEN_LED_OFF %d",check_monitor_led_ms_timer);
					/* �������o�� */
					MonitorLedOut(i,LED_STATUS_OFF );
				}
				//DebugPrint("", str, 2);
				break;
			case LED_STATUS_ORANGE_TOGGLE:
				if (monitor_led_toggle_status == LED_OFF) {
					sprintf(str, "Monitor ORANGE_LED_ON  %d",check_monitor_led_ms_timer);
					/* ��_�����o�� */
					MonitorLedOut(i,LED_STATUS_ORANGE );
				} else {
					sprintf(str, "Monitor ORANGE_LED_OFF %d",check_monitor_led_ms_timer);
					/* �������o�� */
					MonitorLedOut(i,LED_STATUS_OFF );
				}
				//DebugPrint("", str, 2);
				break;
			case LED_STATUS_RED_TOGGLE:
				if (monitor_led_toggle_status == LED_OFF) {
					sprintf(str, "Monitor RED_LED_ON  %d",check_monitor_led_ms_timer);
					/* �ԓ_�����o�� */
					MonitorLedOut(i,LED_STATUS_RED );
				} else {
					sprintf(str, "Monitor RED_LED_OFF %d",check_monitor_led_ms_timer);
					/* �������o�� */
					MonitorLedOut(i,LED_STATUS_OFF );
				}
				//DebugPrint("", str, 2);
				break;
			default :
				break;
			}
		}
		if (monitor_led_toggle_status == LED_OFF) {
			monitor_led_toggle_status = LED_ON;
		} else {
			monitor_led_toggle_status = LED_OFF;
		}
		check_monitor_led_ms_timer = 0;
	}
	MonitorLedWrite();/* LED�ɏo�͂��� */
}



