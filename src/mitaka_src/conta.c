/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	conta.c
 *	�T�v
 *  ����@A���䕔
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
#include "GPS.h"
#include "queue.h"
#include "monitor.h"
#include "common.h"
#include "contb.h"
#include "cont.h"
#include "event.h"
#include "io.h"

/*
 *===========================================================================================
 *					�����萔��`�E�^��`�E�}�N����`
 *===========================================================================================
 */

/* ����ՂƂ̒ʐM�e�X�g���͈ȉ��̗L���ɂ��� */
//#define TEST_MONITOR

/* �^�p�Ǘ�PC�Ƃ̒ʐM�e�X�g���͈ȉ��̗L���ɂ��� */
/* ���̎���PC�ł�COM2���g�p���� */
//#define TEST_PC

/* ����@B�Ƃ̃e�X�g�p */
//#define TEST_B

enum {/* �[������@���M�����Ɏg�p�����ԕϐ� */
	TEST_NONE = 0,	/* �f�o�b�O�Ȃ��A�{�ԃn�[�h */
	TEST_MONITOR,	/* ����ՂƂ̒ʐM�e�X�g�� */
	TEST_PC,		/* �^�p�Ǘ�PC�Ƃ̒ʐM�e�X�g�� ���̎���PC�ł�COM2���g�p���� */
	TEST_B			/* ����@B�Ƃ̃e�X�g�p */
};

enum {/* �[������@���M�����Ɏg�p�����ԕϐ� */
	SEND_BROADCAST_STAGE = 0,
	SEND_BROADCAST_REP_STAGE,	/* kaji20170308 */
	WAITING_200MS_TIME_STAGE,
	SEND_NORMAL_STAGE,
	WAITING_1S_TIME_STAGE
};


enum {/* �Ď��Ց��M�����Ɏg�p�����ԕϐ� */
	CS_ON_STAGE = 0,
	SEND_WAIT_STAGE1,
	SEND_MONITOR_STAGE,
	WAITING_SEND_END_STAGE,
	SEND_WAIT_STAGE2,
	CS_OFF_STAGE,
	WAITING_6S_TIME_STAGE,
};

enum {/* �^�p�Ǘ��o�b�̓d������Ɏg�p�����ԕϐ� */
	PCPOWER_IDLE = 0,
	PCPOWER_SHUTDOWN_ON,
	PCPOWER_SHUTDOWN_OFF,
	PCPOWER_WAIT,
};


//#define SCHEDULE_TIMEOUT_VAL (120)	/* �^�p�Ǘ��o�b����̃X�P�W���[���擾�^�C���A�E�g�l(�b) */
#define SCHEDULE_TIMEOUT_VAL (5)		/* �^�p�Ǘ��o�b����̃X�P�W���[���擾�^�C���A�E�g�l(�b) */

#define SHUTDOWN_WIDTH (6) 			/* �V���b�g�_�E���M���̕��i�b�j */
#define CHOTTO_SHUTDOWN_WIDTH (1) 	/* ������Ɖ����V���b�g�_�E���M���̕��i�b�j */
#define SHUTDOWN_WAIT (2 * 60)		/* �V���b�g�_�E������̕��A�҂�����(�b) */

//#define RESPONSE_ERROR_MAX (10)		/* �����G���[�Ƃ���A���G���[�� */

/*
 *===========================================================================================
 *					�����ϐ���`
 *===========================================================================================
 */

#ifdef windows
//static int debug_type = TEST_MONITOR;	/* ����ՂƂ̒ʐM�e�X�g�� */
/* TEST_PC�ݒ�ł�app.c��RcvCom1,3Srv���R�����g�A�E�g���邱�� 20170320 */
static int debug_type = TEST_PC;		/* �^�p�Ǘ�PC�Ƃ̒ʐM�e�X�g�� ���̎���PC�ł�COM2���g�p���� */
//static int debug_type = TEST_B;			/* ����@B�Ƃ̃e�X�g�p */
//static int debug_type = TEST_NONE;	 /* �f�o�b�O�Ȃ��A�{�ԃn�[�h */
#else
static int debug_type = TEST_NONE;	 /* �f�o�b�O�Ȃ��A�{�ԃn�[�h */
#endif


static int controler_send_stage_no = SEND_BROADCAST_STAGE;
static int monitor_send_stage_no = CS_ON_STAGE;
int pcpower_stage_no = PCPOWER_IDLE;
int shutdown_width;/* �V���b�g�_�E���̉�������ێ� */
static int tanmatsu_type;/* �[���^�C�v�i�P�`CONTROLER_MAX�j */

/* �^�p�Ǘ��o�b����̓���w�߂̓d�� */
/* ����@B�ւ̓���w�߂̓d�� */
static BROADCAST_COMMAND broadcast_command;/* ����@B�ւ̓���w�߂̓d�� */
static NORMAL_COMMAND normal_command;/* ����@B�ւ̒ʏ�w�߂̓d�� */
static RESPONSE_COMMAND my_response_command;/* �����i�����A)�̊Ď������̓d�� */
static RESPONSE_COMMAND response_command[CONTROLER_MAX];/* �����B����̊Ď������̓d�� */

static MONITOR_COMMAND monitor_command;/* �Ď��ՊԂւ̓d�� ����w�� */
static MONITOR_OPERATE_COMMAND monitor_operate_command;/* �Ď��ՊԂ���̉^�p�w�߂̓d��  */

static BROADCAST_COMMAND operation_management_rcv_command;/* �^�p�Ǘ��o�b����̓���w�߂̓d�� */
static int operation_management_request;/* �^�p�Ǘ�PC����̓���ʒm�L�� */
static int monitor_command_request;/* �Ď��Ղ���̗v������t���O */
static int monitor_command_type;/* �Ď��Ղ���^�p�v���^�C�v(=1:�^�p��~,=0:�^�p��~����) */
int remote_command_request;/* ���u���̕\���ύX�v������t���O */
int remote_command_command;/* ���u���̕\���ύX�R�}���h */
static unsigned int broadcast_retry_count;/* kaji20170308 ����w�߂̕����񑗐M�J�E���^ */

//static int ms_timer;/* ms�^�C�} */
static unsigned int check_200ms_timer;/* 200ms�҂��^�C�} */
static unsigned int check_6s_timer;/* 6�b�҂��^�C�} */
static unsigned int check_1s_timer;/* 1�b�҂��^�C�} */
static unsigned int check_delay_timer;/* �҂��^�C�} */
static unsigned int pcpower_timer;/* PC�d������p�^�C�} */
static unsigned int response_received_timer;/* ����@B�����M�`�F�b�N�p�^�C�}�[ */
static unsigned int response_timer[CONTROLER_MAX];/* �����ԊĎ�������M�҂��^�C�} */
static int response_received_flag[CONTROLER_MAX];/* �����B�����M���Z�b�g����� */
static int response_timeout_flag[CONTROLER_MAX];/* �����B�����M�^�C���A�E�g���Z�b�g����� */
static unsigned int response_error_count[CONTROLER_MAX];/* ���������̘A���G���[�񐔂�ێ����� */
unsigned int response_received_count[CONTROLER_MAX];/* ����@�a����̎�M�񐔂�ێ����� */
unsigned int response_time[CONTROLER_MAX];/* �����A����̗v�����琧���B����̉�����M�܂ł̎���(ms) */
unsigned int response_error_time[CONTROLER_MAX];/* �����B����̉����Ȃ��p������(ms) */
unsigned int response_total_err_count[CONTROLER_MAX];/* kaji20170305 ����@�a����̑��G���[�񐔂�ێ����� */

static unsigned int normal_command_timer;/* �ʏ�w�ߎ�M�҂��^�C�} */
static unsigned int moniter_command_timer;/* �Ď��ՊԒʐM��M�҂��^�C�} */
static unsigned int watchdog_nofail_timer;/* kaji20170308 �Ȃ�ł��Ȃ��t�F�C���̂܂܌ł܂��ĂȂ����m�F�^�C�} */

//static int send_stop_req;/* ���M��}�~ 0:���Ȃ�,1:���� */


HOLIDAY_DATA holiday_data;/* �ψڋx�~���Ǘ��p */
#if 0
#define HOLIDAY_COUNT (16)
static int holiday_list[HOLIDAY_COUNT][3] =
{
	{1,1,0},/* ���� */
	{1,2,1},/* ���l�̓�(1����2���j��) */
	{2,11,0},/* �����L�O�� */
	{3,20,2},/* �t���̓� */
	{4,29,0},/* ���a�̓� */
	{5,3,0},/* ���@�L�O�� */
	{5,4,0},/* �݂ǂ�̓� */
	{5,5,0},/* ���ǂ��̓� */
	{7,3,1},/* �C�̓�(7����3���j��) */
	{8,11,0},/* �R�̓� */
	{9,3,1},/* �h�V�̓�(9����3���j��) */
	{9,22,3},/* �H���̓� */
	{10,2,1},/* �̈�̓�(10����2���j�� */
	{11,3,0},/* �����̓� */
	{11,23,0},/* �ΘJ���ӂ̓� */
	{12,23,0},/* �V�c�a���� */
};
#define HAPPY_MONDAY_COUNT (4)
static int happy_monday_list[HAPPY_MONDAY_COUNT][2] =
{
	{1,2},/* ���l�̓�(1����2���j��) */
	{7,3},/* �C�̓�(7����3���j��) */
	{9,3},/* �h�V�̓�(9����3���j��) */
	{10,2},/* �̈�̓�(10����2���j�� */
};
	
#define KYUUSHI_COUNT (2)
static int kyuushi_list[KYUUSHI_COUNT][3] =
{
	{3,3,0},/* ���� */
	{4,4,},/* ���l�̓�(1����2���j��) */
};
#endif
/*
�t���̓�(�R��XX��)
�@= int(20.8431+0.242194*(�N-1980)-int((�N-1980)/4))
�H���̓�(9��XX��)
�@= int(23.2488+0.242194*(�N-1980)-int((�N-1980)/4))
*/


/*
 *===========================================================================================
 *					�O���ϐ���`
 *===========================================================================================
 */

extern int my_tanmatsu_type;/* �[���^�C�v 1:A,2:B... */
extern HANDLE hComm1;       /* �V���A���|�[�g�̃n���h�� */
extern STATUS now_status;/* ���݂̐ݒ�l */
//extern int send_com3_p;
extern int schedule_timer;
extern int err_recover_flg;/* �ُ한�A�{�^��ON��� */
extern int board_choukou_status;	/* ��Ē����w�߂̒l��ێ�����@1:�_�� 0:�œ� kaji20170330 */


/*
 *===========================================================================================
 *					����	�֐���`
 *===========================================================================================
 */

static void SendControlerSrv(void);/* ����@A�̐���@B�ւ̃f�[�^�̑��M���� */
static void SetResponse(void);/* �����̊Ď��������o�b�t�@�ɃZ�b�g���鏈�� */
static void SetNextTanmatu(void);/* ���ɑ��M����[����ݒ肷�鏈�� */
static void SendBroadcast(void);/* ����w�߂̑��M���� */
static void SendNormal(int no);/* ����w�߂̑��M���� */
static void RcvControlerASrv(void);/* ����@A�̐���@B����̃f�[�^�̎�M���� */

static void RcvMonitorSrv(void);/* ����@A�̊Ď��Ղ���̃f�[�^�̎�M���� */
static void SendMonitorSrv(void);/* �Ď��ՊԒʐM���� */
static void SendMonitor(void);/* �Ď��Ղւ̑��M���� */

static void RcvOperationManagementSrv(void);/* �^�p�Ǘ��o�b�ԒʐM��M���� */

static void SendResponse(RESPONSE_COMMAND *response_command );/* �Ď������̑��M���� */

static void PCPowerSrv(void);/* �^�p�Ǘ��o�b�̓d�����䏈�� */

static void SaveHoliday(HOLIDAY_COMMAND *p);/* �ψڋx�~���̃Z�[�u���� */
static void LoadHoliday();/* �ψڋx�~���̃��[�h���� */

static void SetSchedule( void );/* �{���̃X�P�W���[�����Z�b�g���鏈�� */
static void ScheduleSendReq ( void );/* �X�P�W���[���̐���@�a�ւ̑��M�v������ */

//int fail_flag;/* �t�F�C�����M��1�ƂȂ� */
int fail_hnd_state;/* kaji20170225 �t�F�C���Ǘ��̏�ԕϐ� */
int CheckTanmatuError(void) ;/* ���ׂĂ̐���@�ɒ[���G���[���������Ă��邩�ǂ����𔻒肷�鏈�� */

/*
 *===========================================================================================
 *					�O���[�o���֐���`
 *===========================================================================================
 */

void ControlerAInit(void );
void ControlerASrv(void);
void TimerIntContA(int count);/* ����@A�̃^�C�}���荞�ݏ��� */
int CheckTodaysStartTime(void);/* �{���̊J�n�����̔��菈�� */
int CheckStartTime(TIME_INFO_BIN *t);/* �J�n�����̔��菈�� */
int CheckHoliday(TIME_INFO_BIN *t);/* �x�����ǂ����̔��菈�� */
int CheckKyuushiday(TIME_INFO_BIN *t);/* �ψڋx�~�����ǂ����̔��菈�� */

/*
 *===========================================================================================
 *					�O��	�֐���`
 *===========================================================================================
 */

extern void SendCom1(HANDLE h, unsigned char *p, int size);
extern void SendCom2(HANDLE h, unsigned char *p, int size);
extern void SendCom3(HANDLE h, unsigned char *p, int size);
extern int ChkSendCom3(void);

/*
 *===========================================================================================
 *					�O���[�o���֐�
 *===========================================================================================
 */

/**
 *	@brief ����@A�̃^�C�}���荞�ݏ���
 *
 *	@retval �Ȃ�
 */
void TimerIntContA(int count)
{
	int i;
//	ms_timer              += count;
	check_200ms_timer     += count;
	check_1s_timer        += count;
	check_6s_timer        += count;
	check_delay_timer     += count;/* �҂��^�C�} */
	for ( i = 0 ; i < CONTROLER_MAX; i++) {
		response_timer[i] += count;/* �����ԊĎ�������M�҂��^�C�} */
		response_error_time[i] += count;/* �����B����̉����Ȃ��p������(ms) */
	}
	normal_command_timer  += count;/* �ʏ�w�ߎ�M�҂��^�C�} */
	moniter_command_timer += count;/* �Ď��ՊԒʐM��M�҂��^�C�} */
	pcpower_timer         += count;/* PC�d������p�^�C�} */
	watchdog_nofail_timer += count;/* �Ȃ�ł��Ȃ��t�F�C���̂܂܌ł܂��ĂȂ����m�F�^�C�} */
	//20170207 if (normal_command_timer > (30*1000)) {
	if (normal_command_timer > (120*1000)) {
		now_status.pc_tuushin_status = 1;/* �^�p�Ǘ�PC�ԒʐM��� */
	} else {
		now_status.pc_tuushin_status = 0;/* �^�p�Ǘ�PC�ԒʐM��� */
	}
	if (moniter_command_timer > (30*1000)) {
		now_status.moniter_tuushin_status = 1;/* �Ď��ՊԒʐM��� */
		cont_led_status[LED_DENSOU] = LED_STATUS_TOGGLE;/* kaji20170218 */
	} else {
		now_status.moniter_tuushin_status = 0;/* �Ď��ՊԒʐM��� */
		cont_led_status[LED_DENSOU] = LED_STATUS_OFF;/* kaji20170218 */
	}
}


/**
 *	@brief ����@A�̏���������
 *
 *	@retval �Ȃ�
 */
void ControlerAInit(void )
{
	int i;
	/* ����@A���琧��@B�ւ̓���w�߂̓d���t�H�[�}�b�g */
	broadcast_command.h.no = 0;/* �K�i�ԍ� 00H�Œ� */
	broadcast_command.h.dst = 0;/* ����A�h���X 00H�Œ�i���g�p?����g�p�j */
	broadcast_command.h.src = 1;/* ���M���A�h���X  01H�Œ� */
	broadcast_command.h.sub_adr = 0;/* �T�u�A�h���X 00H�Œ� */
	broadcast_command.h.priority = 2;/* �D�惌�x�� 02H�Œ� */
	broadcast_command.h.s_no = 0;/* �ʔ� 00H�Œ� */
	broadcast_command.h.contoroler_no = 0x19;/* �[����� 19H */
	broadcast_command.h.info = 1;/* ����� 01H */
	broadcast_command.h.div_no = 0x81;/* �����ԍ� 81H */
	broadcast_command.h.length = 18;/* �f�[�^�� 18(12H) */
	broadcast_command.t.holiday_week.holiday = 0;/* �x�� */
	broadcast_command.t.holiday_week.week = 0;/* �A�j�� */
	broadcast_command.command.byte = 0;/* ����w�� */
	broadcast_command.light_command.byte = 0;/* �����w�� */
	broadcast_command.status.byte = 0;/* �[������@�i�T�j��� */
	broadcast_command.schedule.start_time[0] = BCD(DEFAULT_START_TIME/60);
	broadcast_command.schedule.start_time[1] = BCD(DEFAULT_START_TIME%60);
	broadcast_command.schedule.start_command = 0;
	broadcast_command.schedule.end_time[0] = BCD(DEFAULT_END_TIME/60);
	broadcast_command.schedule.end_time[1] = BCD(DEFAULT_END_TIME%60);
	broadcast_command.schedule.end_command = 0;
	broadcast_command.schedule.offset_timer = BCD(DEFAULT_OFFSET_TIMER/10);
	operation_management_rcv_command = broadcast_command;
	/* ����@A���琧��@B�ւ̒ʏ�w�߂̓d���t�H�[�}�b�g */
	normal_command.h.no = 0;/* �K�i�ԍ� 00H�Œ� */
	normal_command.h.dst = my_tanmatsu_type;/* ����A�h���X 01H�`08H */
	normal_command.h.src = 1;/* ���M���A�h���X  01H�Œ� */
	normal_command.h.sub_adr = 0;/* �T�u�A�h���X 00H�Œ� */
	normal_command.h.priority = 2;/* �D�惌�x�� 02H�Œ� */
	normal_command.h.s_no = 0;/* �ʔ� 00H�Œ� */
	normal_command.h.contoroler_no = 0x19;/* �[����� 19H */
	normal_command.h.info = 1;/* ����� 01H */
	normal_command.h.div_no = 0x81;/* �����ԍ� 81H */
	normal_command.h.length = 11;/* �f�[�^�� 11(0BH) */
	normal_command.t.holiday_week.holiday = 0;/* �x�� */
	normal_command.t.holiday_week.week = 0;/* �A�j�� */
	normal_command.command.byte = 0;/* ����w�� */
	normal_command.light_command.byte = 0;/* �����w�� */
	normal_command.status.byte = 0;/* �[������@�i�T�j��� */

	/* ����@B���琧��@A�ւ̊Ď������̓d���t�H�[�}�b�g */
	for ( i = 0; i < CONTROLER_MAX; i++) {
		memset((char *)&response_command[i], 0, sizeof (RESPONSE_COMMAND));
		response_command[i].h.no = 0;/* �K�i�ԍ� 00H�Œ� */
		response_command[i].h.dst = 1;/* ����A�h���X  01H�Œ� */
		response_command[i].h.src = i + 1;/* ���M���A�h���X 01H�`08H */
		response_command[i].h.sub_adr = 0;/* �T�u�A�h���X 00H�Œ� */
		response_command[i].h.priority = 2;/* �D�惌�x�� 02H�Œ� */
		response_command[i].h.s_no = 0;/* �ʔ� 00H�Œ� */
		response_command[i].h.contoroler_no = 0x19;/* �[����� 19H */
		response_command[i].h.info = 0x11;/* ����� 11H */
		response_command[i].h.div_no = 0x81;/* �����ԍ� 81H */
		response_command[i].h.length = 15;/* �f�[�^�� 15(0FH) */
//		response_command[i].response.byte2.tanmatu_error = 1;/* �[���@�ُ�Ƃ��Ă��� */
	}
	memset((char *)&my_response_command, 0, sizeof (RESPONSE_COMMAND));
	my_response_command.h.no = 0;/* �K�i�ԍ� 00H�Œ� */
	my_response_command.h.dst = 1;/* ����A�h���X  01H�Œ� */
	my_response_command.h.src = CONTA_ADDRESS;/* ���M���A�h���X 01H�`08H */
	my_response_command.h.sub_adr = 0;/* �T�u�A�h���X 00H�Œ� */
	my_response_command.h.priority = 2;/* �D�惌�x�� 02H�Œ� */
	my_response_command.h.s_no = 0;/* �ʔ� 00H�Œ� */
	my_response_command.h.contoroler_no = 0x19;/* �[����� 19H */
	my_response_command.h.info = 0x11;/* ����� 11H */
	my_response_command.h.div_no = 0x81;/* �����ԍ� 81H */
	my_response_command.h.length = 15;/* �f�[�^�� 15(0FH) */
//	my_response_command.response.byte2.tanmatu_error = 1;/* �[���@�ُ�Ƃ��Ă��� */

	/* ����@����̊Ď��Ղւ̐���w�߂̓d���t�H�[�}�b�g */
	monitor_command.h.no = 0;/* �K�i�ԍ� 00H�Œ� */
	monitor_command.h.dst = 1;/* ����A�h���X 01H�Œ�i���g�p�j */
	monitor_command.h.src = 1;/* ���M���A�h���X  01H�Œ� */
	monitor_command.h.sub_adr = 0;/* �T�u�A�h���X 00H�Œ� */
	monitor_command.h.priority = 2;/* �D�惌�x�� 02H�Œ� */
	monitor_command.h.s_no = 0;/* �ʔ� 00H�Œ� */
	monitor_command.h.contoroler_no = 0x19;/* �[����� 19H */
	monitor_command.h.info = 0x11;/* ����� 01H */
	monitor_command.h.div_no = 0x81;/* �����ԍ� 81H */
//	monitor_command.h.length = 162;/* �f�[�^�� 162(A2H) */
	monitor_command.h.length = 56;/* �f�[�^�� 56(38H) */
	monitor_command.t.holiday_week.holiday = 0;/* �x�� */
	monitor_command.t.holiday_week.week = 0;/* �A�j�� */
	memset(monitor_command.command, 0, sizeof(monitor_command.command));
	
	/* �Ď��Ղ��琧��@�ւ̓d�� �^�p�w�߂̓d���t�H�[�}�b�g */
	monitor_operate_command.h.no = 0;/* �K�i�ԍ� 00H�Œ� */
	monitor_operate_command.h.dst = 1;/* ����A�h���X 01H�Œ�i���g�p�j */
	monitor_operate_command.h.src = 1;/* ���M���A�h���X  01H�Œ� */
	monitor_operate_command.h.sub_adr = 0;/* �T�u�A�h���X 00H�Œ� */
	monitor_operate_command.h.priority = 2;/* �D�惌�x�� 02H�Œ� */
	monitor_operate_command.h.s_no = 0;/* �ʔ� 00H�Œ� */
	monitor_operate_command.h.contoroler_no = 0x19;/* �[����� 19H */
	monitor_operate_command.h.info = 0x19;/* ����� 09H */
	monitor_operate_command.h.div_no = 0x81;/* �����ԍ� 81H */
//	monitor_operate_command.h.length = 162;/* �f�[�^�� 162(A2H) */
	monitor_operate_command.h.length = 56;/* �f�[�^�� 56(38H) */
	monitor_operate_command.t.holiday_week.holiday = 0;/* �x�� */
	monitor_operate_command.t.holiday_week.week = 0;/* �A�j�� */
	monitor_operate_command.command = 0;

	operation_management_request = 0;/* �^�p�Ǘ�PC����̓���ʒm�v���t���O */
	monitor_command_request = 0;/* �Ď��Ղ���̗v���t���O */
	//remote_command_request = 0;/* ���u���̕\���ύX�v������t���O */
	normal_command_timer = 0;/* �ʏ�w�ߎ�M�҂��^�C�} */
	moniter_command_timer = 0;/* �Ď��ՊԒʐM��M�҂��^�C�} */
	shutdown_width = SHUTDOWN_WIDTH;/* �V���b�g�_�E���̉�������ێ� */
//	fail_flag = 0;/* �t�F�C�����M��1�ƂȂ� */
	fail_hnd_state = FAIL_HND_NONE;/* �t�F�C����ԁ@���� */
	watchdog_nofail_timer = 0;/* �Ȃ�ł��Ȃ��t�F�C���̂܂܌ł܂��ĂȂ����m�F�^�C�} �N���A */

	for ( i = 0; i < CONTROLER_MAX; i++) {
		response_error_count[i] = 0;/* ���������̘A���G���[�񐔂��N���A */
		response_received_count[i]= 0;/* ����@�a����̎�M�񐔂��N���A */
		response_total_err_count[i] = 0;/* kaji20170305 ����@�a����̑��G���[�񐔂��N���A */
	}
	//send_stop_req = 0;/* ���M��}�~ 0:���Ȃ�,1:���� */

	holiday_data.holiday_count = 0;/* �j���o�^���N���A */
	holiday_data.kyuushi_count = 0;/* �ψڋx�~���o�^���N���A */
	LoadHoliday();/* �j��,�ψڋx�~�����[�h */
	
#if 0
	holiday_data.holiday_count=HOLIDAY_COUNT;
	holiday_data.kyuushi_count=KYUUSHI_COUNT;
	for( i=0;i<holiday_data.holiday_count;i++){
		holiday_data.holiday[i].month = holiday_list[i][0];
		holiday_data.holiday[i].day = holiday_list[i][1];
		holiday_data.holiday[i].type = holiday_list[i][2];
	}
	for( i=0;i<holiday_data.kyuushi_count;i++){
		holiday_data.kyuushi[i].month = kyuushi_list[i][0];
		holiday_data.kyuushi[i].day = kyuushi_list[i][1];
		holiday_data.kyuushi[i].type = kyuushi_list[i][2];
	}
#endif
	tanmatsu_type = 0;
	SetNextTanmatu();/* ���ɑ��M����[����ݒ肷�鏈�� */
	if (tanmatsu_type == CONTA_ADDRESS) {
		SetNextTanmatu();/* ���ɑ��M����[����ݒ肷�鏈�� */
	}
	PcPower(0);
	MdmcsWrite(0);
	//GPSInit();
}

/**
 *	@brief ����@A�̏���
 *
 *	@retval �Ȃ�
 */

void ControlerASrv(void)
{
	if (debug_type == TEST_MONITOR) {
		/* �Ď��ՂƂ̃e�X�g�p */
		SendMonitorSrv();/* ����@A�̊Ď��Ղւ̃f�[�^�̑��M���� */
		RcvMonitorSrv();/* ����@A�̊Ď��Ղ���̃f�[�^�̎�M���� */
		SendControlerSrv();/* ����@A�̐���@B�ւ̃f�[�^�̑��M���� */
	} else if (debug_type == TEST_PC) {
		/* �^�p�Ǘ�PC�Ƃ̃e�X�g�p */
		//��ɕs�v	SendOperationManagementSrv();/* �^�p�Ǘ��o�b�ԒʐM���M���� */
		SendControlerSrv();/* ����@A�̐���@B�ւ̃f�[�^�̑��M���� */
		RcvOperationManagementSrv();/* �^�p�Ǘ��o�b�ԒʐM��M���� */
	} else if (debug_type == TEST_B) {
		/* ����@B�Ƃ̃e�X�g�p */
		SendControlerSrv();/* ����@A�̐���@B�ւ̃f�[�^�̑��M���� */
		RcvControlerASrv();
	} else {
		/* ���ꂪ�{���̓��� */
		SendMonitorSrv();/* ����@A�̊Ď��Ղւ̃f�[�^�̑��M���� */
		RcvMonitorSrv();/* ����@A�̊Ď��Ղ���̃f�[�^�̎�M���� */
		//��ɕs�v�@���Ԃ�	SendOperationManagementSrv();/* �^�p�Ǘ��o�b�ԒʐM���M���� */
		RcvOperationManagementSrv();/* �^�p�Ǘ��o�b�ԒʐM��M���� */
		SendControlerSrv();/* ����@A�̐���@B�ւ̃f�[�^�̑��M���� */
		RcvControlerASrv();/* ����@A�̐���@B����̃f�[�^�̎�M���� */
	}
	PCPowerSrv();/* �^�p�Ǘ��o�b�̓d�����䏈�� */
	/*
	�����ŉ^�p�Ǘ�PC����̃X�P�W���[���o�^������Ă��Ȃ�������
	����@A�͎����̏�񂩂�o�^���s���A
	����@B�ɑ��M����
	*/
	if (schedule_timer>(SCHEDULE_TIMEOUT_VAL * 1000)) {
		if (now_status.schedule == 0) {
			now_status.schedule = 1;/* �X�P�W���[���o�^�ς� */
			now_status.time_req = 1;/* �����C�����o�^�ς݂Ƃ��� */
			SetSchedule( );/* �{���̃X�P�W���[�����Z�b�g���鏈�� */
		}
	}

	ContSrv(&my_response_command);/* ����@���ʂ̏��� */
}

/*
 *===========================================================================================
 *					�����֐�
 *===========================================================================================
 */

/**
 *	@brief ����@A�̐���@B�ւ̃f�[�^�̑��M����
 *
 *	@retval �Ȃ�
 */
static void SendControlerSrv(void)
{
	int i;
	char str[256];
	char str2[256];
	switch (controler_send_stage_no) {
	case SEND_BROADCAST_STAGE:/* �p���[ON / ����w�ߑ��M */
		DebugPrint("SendControlerSrv", "SEND_BROADCAST_STAGE", 1);
		SendBroadcast();/* ���̂܂ܐ���@B�֓���w�߂𑗐M���� */
		check_200ms_timer = 0;/* 200ms�҂��^�C�} */
		controler_send_stage_no = WAITING_200MS_TIME_STAGE;
//		broadcast_retry_count = BROADCAST_RETRY_VAL;/* kaji20170308 ����w�߂̕����񑗐M�� */
		broadcast_retry_count = 1;/* ��肠��̂��ߋx�~ */
		break;
	case SEND_BROADCAST_REP_STAGE:/* ����w�ߑ��M(�J��Ԃ�) */
		DebugPrint("SendControlerSrv", "SEND_BROADCAST_REP_STAGE", 1);
		SendBroadcast();/* ���̂܂ܐ���@B�֓���w�߂𑗐M���� */
		check_200ms_timer = 0;/* 200ms�҂��^�C�} */
		controler_send_stage_no = WAITING_200MS_TIME_STAGE;
		break;
	case WAITING_200MS_TIME_STAGE://200ms�҂�
		if (check_200ms_timer >= 200) {
			broadcast_retry_count--;
			if (broadcast_retry_count == 0) {/* kaji20170308 ����w�߂𕡐��񑗐M������ */
				controler_send_stage_no = SEND_NORMAL_STAGE;
			} else {
				controler_send_stage_no = SEND_BROADCAST_REP_STAGE;
			}
		}
		break;
	case SEND_NORMAL_STAGE:/* ����w�ߑ��M */
		SendNormal( tanmatsu_type );
		sprintf(str, "SEND_NORMAL_STAGE %d %02X,%02X,%02X"
			, tanmatsu_type
			, (normal_command.command.byte)&0xff
			, (normal_command.light_command.byte)&0xff
			, (normal_command.status.byte)&0xff);
		DebugPrint("SendControlerSrv",str, 8);
		check_200ms_timer = 0;/* 1s�҂��^�C�} */
		controler_send_stage_no = WAITING_1S_TIME_STAGE;
		break;
	case WAITING_1S_TIME_STAGE://1s�҂�
//20170305	if (check_200ms_timer < RESPONSE_INTERVAL_TIME_VAL){/* kaji20170305 */
		if (check_200ms_timer < param.response_interval_time_val){/* kaji20170305 */
			/* �C���^�[�o���^�C��(����500ms�҂�) */
//20170305	} else if ((check_200ms_timer >= RESPONSE_TIMEOUT_VAL) || (response_received_flag[tanmatsu_type -1] == 1)){
		} else if ((check_200ms_timer >= param.response_timeout_val) || (response_received_flag[tanmatsu_type -1] == 1)){
			response_time[tanmatsu_type -1] = check_200ms_timer;/* �����A����̗v�����琧���B����̉�����M�܂ł̎���(ms) */
			if(response_received_flag[tanmatsu_type -1] == 1) {
				/* ����@�����M���Ă��� */
				response_timeout_flag[tanmatsu_type -1] = 0;
				response_error_count[tanmatsu_type -1] = 0;/* ���������̘A���G���[�񐔂��N���A���� */
				response_received_count[tanmatsu_type -1]++;/* ����@�a����̎�M��X�V */
				response_error_time[tanmatsu_type -1] = 0;/* �����B����̉����Ȃ��p������(ms) */
			} else {
				response_error_count[tanmatsu_type -1]++;/* ���������̘A���G���[�񐔍X�V */
				response_total_err_count[tanmatsu_type -1]++;/* kaji20170316 ����@�a����̑��G���[�񐔍X�V */
//20170303		if (response_error_count[tanmatsu_type -1] > RESPONSE_ERROR_MAX) {
//20170305		if (response_error_time[tanmatsu_type -1] > (MUSEN_TIMEOUT_VAL*1000)) {/* �w�莞�ԘA���ŉ�������M�o���Ȃ� */
				if (response_error_time[tanmatsu_type -1] > (param.musen_timeout_val*1000)) {/* �w�莞�ԘA���ŉ�������M�o���Ȃ� */
					response_timeout_flag[tanmatsu_type -1] = 1;/* ��M�^�C���A�E�g���� */
//				}
//#if 1
// kaji20170227 if�̓����Ɉȉ��̃u���b�N���ړ��� (LED_MUSEN�Ɖ^�p�Ǘ�PC�̖����ُ�\�����������鎞���𓯂��ɂ���)
					/* 20170221 �����ُ펞���^�p�Ǘ�PC�ɊĎ������𑗐M���� */
					response_command[tanmatsu_type - 1].response.byte2.musen_error = 1;/* �����ُ���Z�b�g */
					SendResponse(&response_command[tanmatsu_type - 1]);/* ���̂܂܉^�p�Ǘ�PC�ɊĎ������𑗐M���� */
					sprintf(str, "SendControlerSrv �Ď����� %d src=%d %02X %02X %02X", 1, tanmatsu_type
						, response_command[tanmatsu_type - 1].response.status[0]
						, response_command[tanmatsu_type - 1].response.status[1]
						, response_command[tanmatsu_type - 1].response.status[2]
					);
					DebugPrint("", str, 1);
//					response_total_err_count[tanmatsu_type -1]++;/* kaji20170305 ����@�a����̑��G���[�񐔍X�V */
				}
// kaji20170227 ��
			}
			//sprintf(str, "%d,%d,%d,%d",check_200ms_timer, RESPONSE_TIMEOUT_VAL,response_received_flag[tanmatsu_type -1],response_timeout_flag[tanmatsu_type -1]);
			//DebugPrint("", str, 0x20);
			
			if (param.no_musen_error_flag == 0) {
				/* �����Ŗ����ُ���`�F�b�N���� */
				now_status.musen_status = 0;
				for ( i = 0; i < CONTROLER_MAX; i++) {
					if (response_timeout_flag[i] != 0) {
						/* �����ꂩ�̐���@B����M�^�C���A�E�g�̏ꍇ */
						sprintf(str,"�����ꂩ�̐���@B����M�^�C���A�E�g�̏ꍇ ����@�ԍ�=%d",i+1);
						DebugPrint("", str, 0x20);
						now_status.musen_status = 1;
						break;
					}
				}
				if (now_status.before_musen_status != now_status.musen_status) {
					if (param.no_fail_flag == 0) {/* ���̏ꍇ�̓f�o�b�O�p�Ƀt�F�C���ɂ͂��Ȃ� */
						if (now_status.before_musen_status == 0) {
							EventRequest(FAIL_REQUEST);/* �ُ탊�N�G�X�g */
							cont_led_status[LED_MUSEN] = LED_STATUS_TOGGLE;
						} else {
#ifdef	kaji20170208
�����ł̓t�F�C�����甲���锻�f���s��Ȃ�
							if ((now_status.status == STATUS_FAIL) && (CheckErrorStatus() == 0)) {
								EventRequest(FAIL_RECOVER_REQUEST);/* �ُ한�A���N�G�X�g */
							}
#endif
							cont_led_status[LED_MUSEN] = LED_STATUS_OFF;
						}
//						SetResponseStatus(response);/* yamazaki??20170227 IO�X�e�[�^�X���Z�b�g */
					}
				}
				now_status.before_musen_status = now_status.musen_status;
			}
			/* �^�C���A�E�g�܂��͑��M�������Y������@����L���� */
			if (operation_management_request ==  1) {
				/* �^�p�Ǘ�PC����̓���ʒm�L�� */
				operation_management_request = 0;
				memmove(&broadcast_command, &operation_management_rcv_command, sizeof(BROADCAST_COMMAND));
				controler_send_stage_no = SEND_BROADCAST_STAGE;
				strcpy(str,"");
				if ((broadcast_command.status.byte&8) != 0) {
					sprintf(str2, "�`���e�X�g[����@%d]�̑��M�v���L��", (broadcast_command.status.byte >> 5) + 1);
					strcat(str, str2);
					strcat(str, " ");
				}
				if ((broadcast_command.status.byte&0x10) != 0) {
					sprintf(str2, "�`���e�X�g����[����@%d]�̑��M�v���L��", (broadcast_command.status.byte >> 5) + 1);
					strcat(str, str2);
					strcat(str, " ");
				}
				if (broadcast_command.command.shudou != 0) {
					/* �蓮�̑��M�v���L�� */
					sprintf(str2, "�蓮�̑��M�v���L��");
					strcat(str, str2);
					strcat(str, " ");
				}
				if (broadcast_command.command.teishi != 0) {
					/* �^�p��~�̑��M�v���L�� */
					sprintf(str2, "�^�p��~�̑��M�v���L��");
					strcat(str, str2);
					strcat(str, " ");
				}
				if (broadcast_command.command.yobi2 != 0) {
					/* �^�p��~�̑��M�v���L�� */
					sprintf(str2, "�^�p��~�����̑��M�v���L��");
					strcat(str, str2);
					strcat(str, " ");
				}
				if (broadcast_command.light_command.sreq != 0) {
					/* �X�P�W���[���o�^�v���̑��M�v���L�� */
					sprintf(str2, "�X�P�W���[���o�^�v���̑��M�v���L��");
					strcat(str, str2);
					strcat(str, " ");
				}
				if (broadcast_command.light_command.time_req != 0) {
					/* �����C���v���̑��M�v���L�� */
					sprintf(str2, "�����C���v���̑��M�v���L��");
					strcat(str, str2);
					strcat(str, " ");
				}
				if (str[0] == 0) {
					/* ���M�v������ */
					sprintf(str, "���u�̑��M�v���L��");
				}
				DebugPrint("SendControlerSrv",str, 1);
				
/* kaji20170225�� */
//			} else if ((fail_flag == 0) && (CheckTanmatuError() != 0) && ((GetNowRemoteStatus() == STATUS_P1P2) || (GetNowRemoteStatus() == STATUS_P3) )){
//				fail_flag = 1;/* �t�F�C���𑗐M���� */
//20170305 			} else if ( (fail_hnd_state == FAIL_HND_NONE) && (CheckTanmatuError() != 0) 
			} else if ( (fail_hnd_state == FAIL_HND_NONE) && (now_status.mode == MODE_REMOTE) && (CheckTanmatuError() != 0) 
					&&   (GetNowRemoteStatus() == STATUS_P1) ){/* kaji20170226 */
//					&& ( (GetNowRemoteStatus() == STATUS_P1)||(GetNowRemoteStatus() == STATUS_P1P2) ) ){
				fail_hnd_state = FAIL_HND_P1_PARK;/* �ʏ펞�ɔ��������̂Ńt�F�C���ɂ��Ȃ���� */
				strcpy(str, "�ʏ펞�ɔ��������t�F�C���v���Ȃ̂ŕω������Ȃ�");
				DebugPrint("",str, 0);

//20170305			} else if ((fail_hnd_state == FAIL_HND_NONE) && (CheckTanmatuError() != 0) 
			} else if ((fail_hnd_state == FAIL_HND_NONE) && (now_status.mode == MODE_REMOTE) && (CheckTanmatuError() != 0) 
					&& ( (GetNowRemoteStatus() == STATUS_P1P2)||(GetNowRemoteStatus() == STATUS_P2)
					   ||(GetNowRemoteStatus() == STATUS_P3)  ||(GetNowRemoteStatus() == STATUS_P3P2) ) ){/* kaji20170226 */
//					&& ( (GetNowRemoteStatus() == STATUS_P2)||(GetNowRemoteStatus() == STATUS_P3)||(GetNowRemoteStatus() == STATUS_P3P2) ) ){
				fail_hnd_state = FAIL_HND_FAIL;/* �t�F�C����� */
/* kaji20170225�� */
				broadcast_command.command.byte = 0x20;/* �t�F�C�� */
				broadcast_command.light_command.byte = 0;/* light_command���N���A */
				broadcast_command.status.byte = 0;/* status���N���A */
				sprintf(str, "���u����board�ύX���M�v���L�� %02X", broadcast_command.command.byte);
				DebugPrint("",str, 0);
				controler_send_stage_no = SEND_BROADCAST_STAGE;
				/* �������t�F�C���Ƃ��� */
				EventRequest(FAIL_REQUEST);/* �ُ탊�N�G�X�g */
			
/* kaji20170225�� */
//			//20170208 } else if ((fail_flag == 1) && (CheckTanmatuError() == 0) && (err_recover_flg == 1)){
//				fail_flag = 0;/* �t�F�C������̕��A */
//20170305			} else if ((fail_hnd_state != FAIL_HND_NONE) && (CheckTanmatuError() == 0)){/* �t�F�C����������O��� */
			} else if ((fail_hnd_state != FAIL_HND_NONE) && (now_status.mode == MODE_REMOTE) && (CheckTanmatuError() == 0)){/* �t�F�C����������O��� */
				fail_hnd_state = FAIL_HND_NONE;/* �t�F�C����ԁ@���� */
/* kaji20170225�� */
				err_recover_flg = 0;/* �ُ한�A�{�^��ON��ԃN���A */
				if (GetNowRemoteStatus() == STATUS_P3) {
					/* �ψڂ𑗐M���� */
					//broadcast_command.command.byte = 4;
					broadcast_command.command.byte = 0;/* ���u */
				} else {
					/* �ʏ�𑗐M���� */
					//broadcast_command.command.byte = 1;
					broadcast_command.command.byte = 0;/* ���u */
				}
				broadcast_command.light_command.byte = 0;/* light_command���N���A */
				broadcast_command.status.byte = 0;/* status���N���A */
				sprintf(str, "���u����board�ύX���M�v���L��  %02X", broadcast_command.command.byte);
				DebugPrint("",str, 0);
				controler_send_stage_no = SEND_BROADCAST_STAGE;
				EventRequest(REMOTE_REQUEST);/* ���u���N�G�X�g */
/* kaji20170308�� */
//			} else if ((fail_hnd_state != FAIL_HND_NONE) && (now_status.mode == MODE_MONITOR) && (CheckTanmatuError() == 0)){/* �t�F�C����������O��� */
//				fail_hnd_state = FAIL_HND_NONE;/* �t�F�C����ԁ@���� */
//				err_recover_flg = 0;/* �ُ한�A�{�^��ON��ԃN���A */
//				broadcast_command.command.byte = 0;/* ���u */
//				broadcast_command.light_command.byte = 0;/* light_command���N���A */
//				broadcast_command.status.byte = 0;/* status���N���A */
//				broadcast_command.command.teishi = 1;/* command��teishi���Z�b�g */
//				sprintf(str, "�^�p��~���̃t�F�C���O��ɂ�鑗�M�v���L�� %02X", broadcast_command.command.byte);
//				DebugPrint("",str, 0);
//				controler_send_stage_no = SEND_BROADCAST_STAGE;
//				EventRequest(MONITOR_REQUEST);/* ���u���N�G�X�g */
/* kaji20170308�� */
			} else if (remote_command_request == 1) {
				/* ���u���̕\���ύX�v���L�� */
				remote_command_request = 0;
				broadcast_command.command.byte = remote_command_command;
				broadcast_command.light_command.byte = 0;/* light_command���N���A */
				broadcast_command.status.byte = 0;/* status���N���A */
				sprintf(str, "���u����board�ύX���M�v���L��   %02X", broadcast_command.command.byte);
				DebugPrint("",str, 0);
				controler_send_stage_no = SEND_BROADCAST_STAGE;
			} else if (monitor_command_request == 1) {
				/* �Ď��Ղ���̓���ʒm�v���L�� */
				monitor_command_request = 0;
				broadcast_command.command.byte = 0;/* command���N���A���Ă��� */
				broadcast_command.light_command.byte = 0;/* light_command���N���A */
				broadcast_command.status.byte = 0;/* status���N���A */
				if (monitor_command_type == 1) {
					/* �Ď��Ղ���^�p��~�v�� */
					sprintf(str, "�Ď��Ղ���̉^�p��~�v���̑��M�v���L��");
					broadcast_command.command.teishi = 1;/* command��teishi���Z�b�g */
				} else {
					/* �Ď��Ղ���^�p��~�����v�� */
					sprintf(str, "�Ď��Ղ���̉^�p��~�����v���̑��M�v���L��");
					broadcast_command.command.yobi2 = 1;/* command��yobi2���Z�b�g */
				}
//				DebugPrint("SendControlerSrv",str, 1);
				DebugPrint("SendControlerSrv",str, 0);
				controler_send_stage_no = SEND_BROADCAST_STAGE;
			} else {
// kaji20170308	fail_hnd_state = FAIL_HND_NONE;/* �t�F�C����ԁ@���� */
/* kaji20170308��*/
				if ((now_status.mode == MODE_REMOTE) && (now_status.status == STATUS_FAIL) && (CheckTanmatuError() == 0)){
					if (watchdog_nofail_timer > (5*1000)) {/* 5�b�҂� */
						DebugPrint("SendControlerSrv", " *** nofail TRAP �������E", 0);
						EventRequest(REMOTE_REQUEST);/* ���u���N�G�X�g */
						watchdog_nofail_timer = 0;/* �Ȃ�ł��Ȃ��t�F�C���̂܂܌ł܂��ĂȂ����m�F�^�C�} �N���A */
					}
				} else {
					watchdog_nofail_timer = 0;/* �Ȃ�ł��Ȃ��t�F�C���̂܂܌ł܂��ĂȂ����m�F�^�C�} �N���A */
				}
/* kaji20170308��*/
				SetNextTanmatu();/* ���ɑ��M����[����ݒ肷�鏈�� */
				if (tanmatsu_type == CONTA_ADDRESS) {
					/* ����(����@�`)�̃A�h���X�̏ꍇ�̓X�L�b�v���ăo�b�t�@�ɃZ�b�g���� */
					//action ���O�ɃZ�b�g���� SetStaus(&response_command[0]);/* IO�X�e�[�^�X���Z�b�g */
//					response_timer[tanmatsu_type - 1] = 0;/* �����ԊĎ�������M�҂��^�C�}�N���A */
//					response_received_flag[tanmatsu_type - 1] = 0;
					SetResponse();/* �����̊Ď��������o�b�t�@�ɃZ�b�g���� */
					sprintf(str, "�^�p�Ǘ�PC�ɊĎ��������M %02X,%02X,%02X,%02X,%02X,%02X,%02X"
						,response_command[tanmatsu_type - 1].response.status[0]
						,response_command[tanmatsu_type - 1].response.status[1]
						,response_command[tanmatsu_type - 1].response.status[2]
						,response_command[tanmatsu_type - 1].response.status[3]
						,response_command[tanmatsu_type - 1].response.status[4]
						,response_command[tanmatsu_type - 1].response.status[5]
						,response_command[tanmatsu_type - 1].response.status[6]
					);
					DebugPrint("SendControlerSrv", str, 8);
					SendResponse(&response_command[tanmatsu_type - 1]);/* ���̂܂܉^�p�Ǘ�PC�ɊĎ������𑗐M���� */
					
					SetNextTanmatu();/* ���ɑ��M����[����ݒ肷�鏈�� */
				}
				response_timer[tanmatsu_type - 1] = 0;/* �����ԊĎ�������M�҂��^�C�}�N���A */
				response_received_flag[tanmatsu_type - 1] = 0;
				controler_send_stage_no = SEND_NORMAL_STAGE;
			}
		}
		break;
	default:
		printf("SendControlerSrv default error\n");
		break;
	}

}

/**
 *	@brief �����̊Ď��������o�b�t�@�ɃZ�b�g���鏈��
 *
 *	@retval �Ȃ�
 */
static void SetResponse(void)
{
	SetMode(&now_status, &my_response_command);/* ���݂̃��[�h���Z�b�g */
	if (now_status.schedule == 0) {
		/* �X�P�W���[���o�^�˗����M */
		DebugPrint("SendControlerSrv","�X�P�W���[���o�^�˗����M", 1);
		my_response_command.response.byte7.schedule_req = 1;
	} else {
		my_response_command.response.byte7.schedule_req = 0;
	}
	if (now_status.time_req == 0) {
		/* �����C���˗����M */
		DebugPrint("SendControlerSrv","�����C���˗����M", 1);
		my_response_command.response.byte7.time_req = 1;
	} else {
		my_response_command.response.byte7.time_req = 0;
	}
	
	int cds = CDSRead();
	/* kaji20170330 �� */
	if (board_choukou_status == 1) {
		cds = 1;/* �_�������� */
	}
	/* kaji20170330 �� */
	now_status.cds_status = cds; /* CDS��� */
	if ( now_status.cds_status == 1) {
		/* ���邢�̂Œ��������� */
		//printf("ChoukouWriteA 0\n");
		ChoukouWrite(1);/* ���������o�͏��� */
		my_response_command.response.byte7.choukou_iri =0;/* �������i���j */
		my_response_command.response.byte7.choukou_kiri=1;/* �����؁i��j */
	} else {
		//printf("ChoukouWriteB 0\n");
		/* �Â��̂Œ��������� */
		ChoukouWrite(0);/* ���������o�͏��� */
		my_response_command.response.byte7.choukou_iri =1;/* �������i���j */
		my_response_command.response.byte7.choukou_kiri=0;/* �����؁i��j */
	}
	ByouWrite(now_status.status);//20170128 yamazaki
	ToukaWrite(my_tanmatsu_type);//20170129 yamazaki
	
	memmove(&response_command[CONTA_ADDRESS - 1], &my_response_command, sizeof(RESPONSE_COMMAND));
	//if (now_status.gps_status != 0) {
	//	/* ���v�ُ� */
	//	response_command[CONTA_ADDRESS - 1].response.byte2.tanmatu_error = 1;/* �[������@�ُ� */
	//}
}

/**
 *	@brief ���ɑ��M����[����ݒ肷�鏈��
 *
 *	@retval �Ȃ�
 */
static void SetNextTanmatu(void) {
	int i;
	
	tanmatsu_type++;
	if (tanmatsu_type > CONTROLER_MAX) {
		tanmatsu_type = 1;
	}
	i = tanmatsu_type -1;
	int d = (param.linkage_status >> (4*(CONTROLER_MAX - i - 1)))&0xf;
	if (d == 0) {
		/* �A�����Ă��Ȃ��̂Ŏ���I�� */
		SetNextTanmatu();
	}
}

/**
 *	@brief ���ׂĂ̐���@�ɒ[���G���[���������Ă��邩�ǂ����𔻒肷�鏈��
 *
 *	@retval 0:�G���[����,1:�G���[�L��
 */
int CheckTanmatuError(void) {
	int i;
	int ret = 0;
	int err_count = 0;
	for ( i = 0; i < CONTROLER_MAX; i++) {
		int d = (param.linkage_status >> (4*(CONTROLER_MAX - i - 1)))&0xf;
		if (d == 0) {
			/* �A�����Ă��Ȃ��̂Ŏ���I�� */
			continue;
		}
		if (response_command[i].response.byte2.tanmatu_error != 0) {
			err_count++;
		}
	}
	if (err_count != 0) {
		ret = 1;
	}
	if (param.no_fail_flag == 1) {/* ���̏ꍇ�̓f�o�b�O�p�Ƀt�F�C���ɂ͂��Ȃ� */
		/* �t�F�C�������ǃt�F�C���ɂ��Ȃ� */
		ret = 0;
	}
	
	return ret;
}


/* ����w�߂̑��M���� */
static void SendBroadcast(void) {
	int bcc;
	if ((debug_type == TEST_NONE) || (debug_type == TEST_B)) {
		SetNowTime(&broadcast_command.t);/* �����Z�b�g */
		broadcast_command.h.s_no = 0;/* �ʔ� 00H�Œ� */
		broadcast_command.h.sub_adr = LinkagePack(param.linkage_status);/* kaji20170407 �A���ݒ�l��sub_adr�ɃZ�b�g���� */
		bcc = CalcRealBcc((char *)&broadcast_command,sizeof(BROADCAST_COMMAND));
		broadcast_command.h.s_no = bcc;/* 20170305 Bcc��ʔԂɃZ�b�g���� */
		SendCom1(hComm1, (unsigned char *)&broadcast_command, sizeof(BROADCAST_COMMAND));
	}
}

/* ����w�߂̑��M���� */
static void SendNormal(int no) {
	normal_command.h.dst = no;/* ����A�h���X 02H�`08H */
	
	normal_command.command.byte = 0;
	if (now_status.status == STATUS_P1) {
		normal_command.command.tuujou = 1;
	} else if (now_status.status == STATUS_P2) {
		normal_command.command.issou = 1;
	} else if (now_status.status == STATUS_P3) {
		normal_command.command.henni = 1;
	} else if (now_status.status == STATUS_FAIL) {
		normal_command.command.fail = 1;
	}
	if (now_status.mode == MODE_MANUAL) {
		normal_command.command.shudou = 1;
	} else if (now_status.mode == MODE_MONITOR) {/* kaji20170301 */
		normal_command.command.teishi = 1;/* kaji20170301 */
	}
	
	/* ������CDS���`�F�b�N���Ē����𐧌䂷�� */
	/*
	�i2�j�ψڒ��͋���������i�u����ON�j�Ƃ���B
	�Ƃ�
	������Ďw�ߗL��{����������
	�ł���
	*/
	int cds = CDSRead();
	/* kaji20170330 �� */
	if (board_choukou_status == 1) {
		cds = 1;/* �_�������� */
	}
	/* kaji20170330 �� */

	normal_command.light_command.issei = 1;/* ������Ďw�ߗL */
	if ( cds == 1) {
		/* ���邢�̂Œ��������� */
//		ChoukouWrite(1);/* ���������o�͏��� */
		//printf("���邢�̂Œ��������� %X\n", normal_command.light_command.byte);
		normal_command.light_command.choukou_kiri = 1;/* ���������؁i���j */
		normal_command.light_command.choukou_iri = 0;/* �����������i��j */
	} else {
		/* �Â��̂ňÂ��̂Œ��������� */
//		ChoukouWrite(0);/* ���������o�͏��� */
		//printf("�Â��̂Œ��������� %X\n", normal_command.light_command.byte);
		normal_command.light_command.choukou_kiri = 0;/* ���������؁i���j */
		normal_command.light_command.choukou_iri = 1;/* �����������i��j */
	}
	SetNowTime(&normal_command.t);/* �����Z�b�g */
	if ((debug_type == TEST_NONE) || (debug_type == TEST_B)) {
		int bcc;
		normal_command.h.s_no = 0;/* �ʔ� 00H�Œ� */
		bcc = CalcRealBcc((char *)&normal_command,sizeof(NORMAL_COMMAND));
		normal_command.h.s_no = bcc;/* 20170305 Bcc��ʔԂɃZ�b�g���� */
		SendCom1(hComm1, (unsigned char *)&normal_command, sizeof(NORMAL_COMMAND));
	}
}

/**
 *	@brief ����@A�̐���@B����̃f�[�^�̎�M����
 *
 *	@retval �Ȃ�
 */
static void RcvControlerASrv(void) {
	char str[256];
	char str2[20];
	unsigned char conta_rcv_buf[RCV_BUFF_SIZE];
	
	while(!empty(CONTROLER_QUEUE)) {
		unsigned char d = peek(CONTROLER_QUEUE, 0);/* �擪�f�[�^�͂O���H */
		if ( d != 0) {
			dequeue(CONTROLER_QUEUE);
			continue;
		}
		if (lenqueue(CONTROLER_QUEUE) >= 10) {
			int conta_wait_count = peek(CONTROLER_QUEUE, 9) + 10;
			if (conta_wait_count != 25){
				dequeue(CONTROLER_QUEUE);
			} else {
				//printf("%d\n",lenqueue(CONTROLER_QUEUE));
				if (lenqueue(CONTROLER_QUEUE) >= conta_wait_count) {
				//printf("X");
					/* �f�[�^�p�P�b�g��M */
					int i;
					for(i = 0 ; i < conta_wait_count; i++) {
						conta_rcv_buf[i] = dequeue(CONTROLER_QUEUE);/* ��M�f�[�^���o�� */
					}
					CONTROL_HEADER *h =(CONTROL_HEADER *)conta_rcv_buf;
					if (h->info == 0x11) {/* �Ď����� */
						//sprintf(str, "RcvControlerASrv �Ď����� %d", h->dst);
						//DebugPrint("", str, 1);
						if ((h->dst < 1) || (h->dst >= CONTROLER_MAX)) {
							sprintf(str, "RcvControlerASrv �[���ԍ��G���[ %d",h->dst);
							DebugPrint("", str, 2);
						} else {
							RESPONSE_COMMAND * rp = (RESPONSE_COMMAND *) conta_rcv_buf;
							int bcc = rp->h.s_no;/* Bcc���o�� */
							rp->h.s_no = 0;
							int cbcc = CalcRealBcc(conta_rcv_buf,sizeof(RESPONSE_COMMAND));
							cbcc &= 0xff;
							if ( bcc == cbcc) {
//							if ( 1) {
								memmove(&response_command[h->src - 1], conta_rcv_buf, sizeof(RESPONSE_COMMAND));
								response_timer[h->src - 1] = 0;/* �����ԊĎ�������M�҂��^�C�}�N���A */
								response_received_flag[h->src - 1] = 1;
								response_received_timer = 0;/* ����@B�����M�����̂ŃN���A */
								SendResponse(&response_command[h->src - 1]);/* ���̂܂܉^�p�Ǘ�PC�ɊĎ������𑗐M���� */
								sprintf(str, "RcvControlerASrv �Ď����� %d src=%d %02X %02X %02X", h->dst, h->src
									, response_command[h->src - 1].response.status[0]
									, response_command[h->src - 1].response.status[1]
									, response_command[h->src - 1].response.status[2]
								);
								DebugPrint("", str, 8);
								if (response_command[h->src - 1].response.byte7.schedule_req != 0) {
									/* �X�P�W���[���o�^�˗� */
									ScheduleSendReq ( );/* �X�P�W���[���̐���@�a�ւ̑��M�v������ */
									sprintf(str,"�X�P�W���[���𐧌�@B�ɑ�����");
									DebugPrint("", str, 0);
								}
							} else {
								sprintf(str, "RcvControlerASrv Bcc�G���[ %d,%X<->%X",h->dst,bcc, cbcc);
								DebugPrint("", str, 2);
								str[0] = '\0';
								for ( i = 0; i < sizeof(RESPONSE_COMMAND);i++) {
									sprintf(str2,"%02X ",conta_rcv_buf[i]&0xff);
									strcat(str, str2);
								}
								DebugPrint("", str, 2);
							}
						}
					} else {
						//��M�G���[
						sprintf(str, "RcvControlerASrv ����ʃG���[ %02X",h->info);
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
 *	@brief ����@A�̊Ď��Ղ���̃f�[�^�̎�M����
 *
 *
 * 
 *
 *
 *	@retval �Ȃ�
 */
static void RcvMonitorSrv(void) {
	char str[256];
	unsigned char contm_rcv_buf[RCV_BUFF_SIZE];

	
	while(!empty(MONITOR_QUEUE)) {
		unsigned char d = peek(MONITOR_QUEUE, 0);/* �擪�f�[�^�͂O���H */
		if ( d != 0) {
			dequeue(MONITOR_QUEUE);
			continue;
		}
		if (lenqueue(MONITOR_QUEUE) >= 10) {
			int contm_wait_count = peek(MONITOR_QUEUE, 9) + 10;
			if (contm_wait_count != 19){
				dequeue(MONITOR_QUEUE);
			} else {
				//printf("%d\n",lenqueue(MONITOR_QUEUE));
				if (lenqueue(MONITOR_QUEUE) >= contm_wait_count) {
				//printf("X");
					/* �f�[�^�p�P�b�g��M */
					int i;
					for(i = 0 ; i < contm_wait_count; i++) {
						contm_rcv_buf[i] = dequeue(MONITOR_QUEUE);/* ��M�f�[�^���o�� */
					}
					CONTROL_HEADER *h =(CONTROL_HEADER *)contm_rcv_buf;
					MONITOR_OPERATE_COMMAND *com = (MONITOR_OPERATE_COMMAND *)contm_rcv_buf;
					if (h->info == 0x81) {/* �^�p��~�����R�}���h */
						if ((com->command & 1) != 0) {
							monitor_command_request = 1;/* �Ď��Ղ���̗v������ */
							now_status.mode = MODE_MONITOR;/* �Ď��Ղ���̎w�߃��[�h�ɐ؂�ւ��� */
							sprintf(str, "RcvMonitorSrv �^�p��~�R�}���h");
							monitor_command_type = 1;/* �Ď��Ղ���^�p��~�v�� */
							EventRequest(MONITOR_REQUEST);/* �^�p��~���N�G�X�g */
						} else if ((com->command & 2) != 0){
							monitor_command_request = 1;/* �Ď��Ղ���̗v������ */
							now_status.mode = MODE_REMOTE;/* ���u���[�h�ɐ؂�ւ��� */
							sprintf(str, "RcvMonitorSrv �^�p��~�����R�}���h");
							monitor_command_type = 0;/* �Ď��Ղ���^�p��~�����v�� */
							EventRequest(MONITOR_RELEASE_REQUEST);/* �^�p��~�������N�G�X�g */
						} else {
							moniter_command_timer = 0;/* �Ď��ՊԒʐM��M�҂��^�C�} */
							sprintf(str, "RcvMonitorSrv �^�p�w���X�R�}���h");
						}
						DebugPrint("", str, 1|4);
//						DebugPrint("", str, 0);
						
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
 *	@brief ����@A�̊Ď��Ղւ̃f�[�^�̑��M����
 *
 *	@retval �Ȃ�
 */
static void SendMonitorSrv(void)
{
//	char str[256];
	switch (monitor_send_stage_no) {
	case CS_ON_STAGE:/* MDM_CS�̏o�͏��� */
		check_6s_timer = 0;/* 6�bs�҂��^�C�} */
		MdmcsWrite(1);
		check_delay_timer = 0;/* �҂��^�C�} */
		monitor_send_stage_no = SEND_WAIT_STAGE1;
		DebugPrint("SendMonitorSrv", "CS_ON MdmcsWrite(1)", 4);
		break;
	case SEND_WAIT_STAGE1:/* MDM_CS�̏o�͏��� */
		if (check_delay_timer > param.mdmcs_delay_time) {/* �҂��^�C�} */
			monitor_send_stage_no = SEND_MONITOR_STAGE;
		}
		break;
	case SEND_MONITOR_STAGE:/* �p���[ON�@����w�ߑ��M */
		DebugPrint("SendMonitorSrv", "���M�J�n", 4);
		SendMonitor();
		monitor_send_stage_no = WAITING_SEND_END_STAGE;
		break;
	case WAITING_SEND_END_STAGE://���M�I���҂�
//		if (send_com3_p != sizeof(MONITOR_COMMAND)) {
//			SendCom3(hComm1, (unsigned char *)&monitor_command, sizeof(MONITOR_COMMAND));
//		} else {
		if (ChkSendCom3() == 0){/* kaji20170310 */
			/*  ���M�I��*/
			DebugPrint("SendMonitorSrv", "���M�I��", 4);
			check_delay_timer = 0;/* �҂��^�C�} */
			monitor_send_stage_no = SEND_WAIT_STAGE2;
		}
		break;
	case SEND_WAIT_STAGE2:/* MDM_CS�̏o�͏��� */
		if (check_delay_timer > param.mdmcs_delay_time) {/* �҂��^�C�} */
			MdmcsWrite(0);
			monitor_send_stage_no = WAITING_6S_TIME_STAGE;
			DebugPrint("SendMonitorSrv", "CS_OFF MdmcsWrite(0)", 4);
		}
		break;
	case WAITING_6S_TIME_STAGE://6s�҂�
		if (check_6s_timer >= 6000) {
			monitor_send_stage_no = CS_ON_STAGE;
		}
		break;
	default:
		printf("SendMonitorSrv default error\n");
		break;
	}

}

/* �Ď��Ղւ̑��M���� 
	*/
static void SendMonitor(void) {
	SetNowTime(&monitor_command.t);/* �����Z�b�g */
	SetResponse();/* �����̊Ď��������o�b�t�@�ɃZ�b�g���� */
	int i;
	/* linkage_status��div_no�ɃZ�b�g���� */
	monitor_command.h.div_no = LinkagePack(param.linkage_status);
	for ( i = 0; i < 8; i++) {
		memmove(monitor_command.command[i], response_command[i].response.status, 6);
		if (i != (CONTA_ADDRESS-1)) {
			if (response_timeout_flag[i] == 0) {
				//monitor_command.command[i][2] &= ~0x20;/* �����ُ���N���A */
				monitor_command.responce[i].response.byte2.musen_error = 0;/* �����ُ���N���A */
			} else {
				//monitor_command.command[i][2] |= 0x20;/* �����ُ���Z�b�g */
				monitor_command.responce[i].response.byte2.musen_error = 1;/* �����ُ���Z�b�g */
			}
		}
		/* ����@B���^�p��~���[�h���ǂ����͂����ŃZ�b�g���� 20170218 */
		if (now_status.mode == MODE_MONITOR) {
			monitor_command.responce[i].response.status[0] |= 8;
		} else {//20170226
			monitor_command.responce[i].response.status[0] &= (~8);
		}
		
	}
	
//	send_com3_p = 0;
	SendCom3(hComm1, (unsigned char *)&monitor_command, sizeof(MONITOR_COMMAND));
}


/* �^�p�Ǘ�PC�ւ̊Ď������̑��M���� */
static void SendResponse(RESPONSE_COMMAND *response_command ) {
	char str[256];
	//if (send_stop_req != 0) {
	//	/* ���M��}�~���� */
	//	return;
	//}
	sprintf(str, "�^�p�Ǘ�PC�ɊĎ��������M %02X,%02X"
		,response_command->h.src
		,response_command->h.dst
	);
	DebugPrint("SendResponse", str, 8);
	if ((debug_type == TEST_NONE) || (debug_type == TEST_PC)) {
		response_command->h.sub_adr = LinkagePack(param.linkage_status);/* �A���ݒ�l��sub_adr�ɃZ�b�g���� */
		SetNowTime(&response_command->t);/* �����Z�b�g */
		SendCom2(hComm1, (unsigned char *)response_command, sizeof(RESPONSE_COMMAND));
	}
}

/* �^�p�Ǘ��o�b����̎�M���� */
/*
	���񐧌�w�߂Ɠ����t�H�[�}�b�g�ɂ���
		��������M
		�{���̃X�P�W���[����M
		�ݒ�l��M�i�I�t�Z�b�g�^�C�},�j
		�e�X�g�i�A�h���X�Œ[�������肷��j
		�[���؂藣���i�A�h���X�Œ[�������肷��A����w�߂�DB7�Őݒ�,DB6:0:�Ő؂藣��,1:�A���j
*/
static void RcvOperationManagementSrv(void) {
	char operation_management_rcv_buf[RCV_BUFF_SIZE];

//	while(!empty(PC_QUEUE)) {
	while(lenqueue(PC_QUEUE) >= 3) {/* kaji20170310 */
//		unsigned char d = peek(PC_QUEUE, 0);/* �擪�f�[�^�͂O���H */
//		if ( d != 0) {
		unsigned char d0 = peek(PC_QUEUE, 0);/* 0byte�ڃf�[�^�� 5A ���H */
		unsigned char d1 = peek(PC_QUEUE, 1);/* 1byte�ڃf�[�^�� FF ���H */
		unsigned char d2 = peek(PC_QUEUE, 2);/* 2byte�ڃf�[�^�� 00 ���H */
		if ( (d0 != 0x5a)||(d1 != 0xff)||(d2 != 0x00) ) {/* kaji20170321�C�� */
			dequeue(PC_QUEUE);
			continue;
		}
//		if (lenqueue(PC_QUEUE) >= 10) {
		if (lenqueue(PC_QUEUE) >= 10+2) {/* kaji20170310 */
//			unsigned int operation_management_wait_count = peek(PC_QUEUE, 9) + 10;
			unsigned int operation_management_wait_count = peek(PC_QUEUE, 9+2) + 10;/* kaji20170310 */
//			if ((operation_management_wait_count != 21) && (operation_management_wait_count != 28)){
			if ((operation_management_wait_count != 21) && 
				(operation_management_wait_count != 28) && 
				(operation_management_wait_count != 192)){
				printf("*** illigal_operation_management_wait_count_value=%d\n",operation_management_wait_count);
				dequeue(PC_QUEUE);
			} else {
				//printf("%d\n",lenqueue(PC_QUEUE));
//				if (lenqueue(PC_QUEUE) >= operation_management_wait_count) {
				if (lenqueue(PC_QUEUE) >= operation_management_wait_count+2) {/* kaji20170310 */
					/* �f�[�^�p�P�b�g��M */
					int i;
					dequeue(PC_QUEUE);/* 0x5A �ǂݎ̂� kaji20170310 */
					dequeue(PC_QUEUE);/* 0xFF �ǂݎ̂� kaji20170310 */
					for(i = 0 ; i < operation_management_wait_count; i++) {
						operation_management_rcv_buf[i] = dequeue(PC_QUEUE);/* ��M�f�[�^���o�� */
					}
					CONTROL_HEADER *h =(CONTROL_HEADER *)operation_management_rcv_buf;
					if (operation_management_wait_count == 192 ) {
						printf("�j���ψڋx�~���ʒm��M\n");
						SaveHoliday((HOLIDAY_COMMAND *) operation_management_rcv_buf);/* �ψڋx�~�� */
					}
					else if (h->dst == 0) {/* ���񐧌�w�� */
					//if (h->info == 0x11) {/* ����w�� */
						BROADCAST_COMMAND *com = (BROADCAST_COMMAND *)operation_management_rcv_buf;
						if (com->light_command.time_req != 0) {
							/* �����C���v����M */
							SetTime(&(com->t));/* �����ݒ菈�� */
							SaveRTC(&(com->t));/* �s�����pRTC�ɏ������� */
							DebugPrint("","RcvOperationManagementSrv �^�p�Ǘ��o�b���� �����C���v����M", 0);
						}
						if (com->light_command.sreq != 0) {
							/* �X�P�W���[���o�^�v����M */
							DebugPrint("","RcvOperationManagementSrv �^�p�Ǘ��o�b���� �X�P�W���[���o�^�v����M", 0);
						}
						if ((com->light_command.byte&0x80) != 0) {
							/* �X�P�W���[���o�^�v����M */
							DebugPrint("","RcvOperationManagementSrv �^�p�Ǘ��o�b���� ���M��~�v������M����", 0);
							//send_stop_req = 1;/* ���M��}�~���� */
						} else {
							//send_stop_req = 0;/* ���M��}�~���Ȃ� */
						}
						DebugPrint("","RcvOperationManagementSrv �^�p�Ǘ��o�b�����M", 0);
						BroadcastCommand(com);/* ����w�ߎ�M���� */
						int diff_time = CheckDiffTime(&(com->t));
						if (diff_time > TIME_ERROR_VAL) {
							/* �����G���[�ƂȂ鎞�Ԃ̍�(�b) */
							now_status.time_status = 1;
							now_status.schedule = 0;/* 20170224 ����ŃX�P�W���[���o�^�v���𑗐M���� */
							now_status.time_req = 0;/* 20170224 ����Ŏ����ݒ�v���𑗐M���� */
						} else {
							now_status.time_status = 0;
						}
						//if ((com->status.byte&6) == 0){/* 20170219 GPS��ԕω��͑��M���Ȃ� */
						if (((com->status.byte&6) == 0) && (com->light_command.rendou_req == 0)){
							/* 20170320 GPS��ԕω� or �A���ݒ�v���͑��M���Ȃ� */
							operation_management_request = 1;/* �^�p�Ǘ�PC����̓���ʒm�L�� */
							/* ����ʒm���Z�b�g */
							memmove(&operation_management_rcv_command, operation_management_rcv_buf, sizeof(BROADCAST_COMMAND));
						} else {
							if (com->light_command.rendou_req != 0) {
								unsigned char link_status = LinkagePack(param.linkage_status);
								if (link_status != h->sub_adr) {
									printf("�A���ݒ�l���ς���� %X->%X\n",link_status,h->sub_adr);
									param.linkage_status = LinkageUnPack(h->sub_adr);
									DebugPrint("", "�p�����[�^���ς�������߃p�����[�^�Z�[�u\n", 0);
									SaveParam();
								}
							}
						}
					} else {/* �ʏ�w�� */
						/* �^�p�Ǘ��o�b����̃w���X����M */
						normal_command_timer = 0;/* �ʏ�w�ߎ�M�҂��^�C�} */
						DebugPrint("","RcvOperationManagementSrv �^�p�Ǘ��o�b����ʏ�w�߁i�w���X�j��M", 1);
						BROADCAST_COMMAND *com = (BROADCAST_COMMAND *)operation_management_rcv_buf;
						int diff_time = CheckDiffTime(&(com->t));
						if (diff_time > TIME_ERROR_VAL) {
							/* �����G���[�ƂȂ鎞�Ԃ̍�(�b) */
							now_status.time_status = 1;
							now_status.time_req = 0;/* 20170224 ����Ŏ����ݒ�v���𑗐M���� */
						} else {
							now_status.time_status = 0;
						}
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
 *	@brief �{���̃X�P�W���[�����Z�b�g���鏈��
 *
 *	@retval �Ȃ�
 */
static void SetSchedule( void )
{
	int ret ;
	char str[256];
	ret = CheckTodaysStartTime();
	if (ret == TODAY_IS_NORMAL_DAY) {
		now_status.start_time = param.start_time;
		now_status.end_time = param.end_time;
		now_status.offset_timer = param.offset_timer;
	} else {
		now_status.start_time = 0;
		now_status.end_time = 0;
		now_status.offset_timer = param.offset_timer;
	}
	sprintf(str,"�����ŃX�P�W���[���o�^���s���� %d", ret);
	DebugPrint("", str, 0);
	ScheduleSendReq ( );/* �X�P�W���[���̐���@�a�ւ̑��M�v������ */
}

/**
 *	@brief �X�P�W���[���̐���@�a�ւ̑��M�v������
 *
 *	@retval �Ȃ�
 */
void ScheduleSendReq ( void )
{
	operation_management_request =  1;/* ������Z�b�g���Ȃ��Ƒ����Ă���Ȃ� */
	SetNowTime(&(operation_management_rcv_command.t));/* �����ݒ菈�� */
	operation_management_rcv_command.light_command.sreq = 1;/* �X�P�W���[���ݒ�v�� */
	operation_management_rcv_command.light_command.time_req = 1;/* �����C���v�� */
	operation_management_rcv_command.schedule.start_time[0] = BCD((now_status.start_time/60) / 60);
	operation_management_rcv_command.schedule.start_time[1] = BCD((now_status.start_time/60) % 60);
	operation_management_rcv_command.schedule.end_time[0] = BCD((now_status.end_time/60) / 60);
	operation_management_rcv_command.schedule.end_time[1] = BCD((now_status.end_time/60) % 60);
	operation_management_rcv_command.schedule.offset_timer = BCD(now_status.offset_timer/10);
	
}

/**
 *	@brief �J�n�����̔��菈��
 *
 *	@retval ���̓��̏��
 */
int CheckTodaysStartTime()
{
	TIME_INFO t;
	TIME_INFO_BIN tb;
	SetNowTime(&t);
	tb.year = 100 * BIN(t.year[0]) + BIN(t.year[1]);
	tb.month = BIN(t.month);
	tb.day = BIN(t.day);
	tb.week = subZeller(tb.year, tb.month, tb.day)+1;//���j�����P�Ƃ���
	//tb.week = t.holiday_week.week;
	return CheckStartTime(&tb);
}

/**
 *	@brief �J�n�����̔��菈��
 *
 *	@param [TIME_INFO_BIN *t] �����i�[�|�C���^
 *
 *	@retval ���̓��̏��
 */
/*
�t���̓�(�R��XX��)
�@= int(20.8431+0.242194*(�N-1980)-int((�N-1980)/4))
�H���̓�(9��XX��)
�@= int(23.2488+0.242194*(�N-1980)-int((�N-1980)/4))
*/
int CheckStartTime(TIME_INFO_BIN *t)
{
	int i;
	char str[256];
	
	sprintf(str, "%d�N%d��%d��",t->year,t->month,t->day);
	DebugPrint("CheckStartTime", str, 2);
	if (CheckKyuushiday(t) != 0)
	{
		/* �ψڋx�~�� */
		//20170224 return TODAY_IS_NORMAL_DAY;
		return TODAY_IS_HOLIDAY;
	}
	
	
	for ( i = 0; i < holiday_data.holiday_count; i++) {
		if (holiday_data.holiday[i].type == 0) {
			/* ���ʂ̏j�� */
			if ((holiday_data.holiday[i].month == t->month) && /* ������v */
				(holiday_data.holiday[i].day == t->day) ) { /* ������v */
				/* �j�� */
				return TODAY_IS_HOLIDAY;
			}
		}
		if (holiday_data.holiday[i].type == 1) {
			/* �n�b�s�[�}���f�C */
			if (holiday_data.holiday[i].month == t->month) {
				/* ������v */
				if (t->week == 2) {/* ���j��? */
					/* ���j�� */
					int m = (t->day-1)/7+1;
					if (holiday_data.holiday[i].day == m) {
						/* ��w�Ԗڂ̌��j������v */
						return TODAY_IS_HOLIDAY;
					}
				}
			}
		}
	}
	int year = t->year;
	if (t->month == 3) {
		/* �t���̓��̌� */
		int d = (int)(20.8431+0.242194*(year-1980)-(int)((year-1980)/4));
		//printf("year = %d,d=%d\n",year,d);
		if (t->day == d) {
			/* �t���̓� */
			return TODAY_IS_HOLIDAY;
		}
	}
	if (t->month == 9) {
		/* �H���̓��̌� */
		int d = (int)(23.2488+0.242194*(year-1980)-(int)((year-1980)/4));
		//printf("year = %d,d=%d\n",year,d);
		if (t->day == d) {
			/* �H���̓� */
			return TODAY_IS_HOLIDAY;
		}
	}

	if (t->week == 1) {/* ���j��? */
		return TODAY_IS_HOLIDAY;
	}
	//��������͐U��ւ��j�����ǂ������`�F�b�N����
	TIME_INFO_BIN tmp;
	int count = t->week - 1; /* ���j�����P�Ƃ��� */
	tmp = *t;
	
static int month_day[12]={0x31,0x28,0x31,0x30,0x31,0x30,0x31,0x31,0x30,0x31,0x30,0x31};
	for (i = 0 ; i < count; i++)
	{
		tmp.day = tmp.day - 1;
		if (tmp.day == 0) {
			tmp.month = tmp.month - 1;
			if (tmp.month == 0) {
				tmp.month = 0x12;
			}
			if (tmp.month == 2) {
				int n;
				int y = tmp.year;
				n = 28 + (1 / (y - y / 4 * 4 + 1)) * (1 - 1 / (y - y / 100 * 100 + 1))
					+ (1 / (y - y / 400 * 400 + 1));	
				tmp.day = n;
			} else {
				tmp.day = month_day[tmp.month - 1];
			}
		}
		
		//printf("m=%X,d=%x\n",tmp.month,tmp.day);
		if (CheckHoliday(&tmp) != TODAY_IS_HOLIDAY)
		{
			 return TODAY_IS_NORMAL_DAY;
		}
	}
	return TODAY_IS_HOLIDAY;
}

/**
 *	@brief �x�����ǂ����̔��菈��
 *
 *	@param [TIME_INFO_BIN *t] �����i�[�|�C���^
 *
 *	@retval ���茋��
 */
int CheckHoliday(TIME_INFO_BIN *t)
{
	int i;
	for ( i = 0; i < holiday_data.holiday_count; i++) {
		if (holiday_data.holiday[i].type == 0) {
			/* ���ʂ̏j�� */
			if ((holiday_data.holiday[i].month == t->month) && /* ������v */
				(holiday_data.holiday[i].day == t->day) ) { /* ������v */
				/* �j�� */
				return TODAY_IS_HOLIDAY;
			}
		}
		int year = t->year;
		if (t->month == 3) {
			/* �t���̓��̌� */
			int d = (int)(20.8431+0.242194*(year-1980)-(int)((year-1980)/4));
			//printf("year = %d,d=%d\n",year,d);
			if (t->day == d) {
				/* �t���̓� */
				return TODAY_IS_HOLIDAY;
			}
		}
		if (t->month == 9) {
			/* �H���̓��̌� */
			int d = (int)(23.2488+0.242194*(year-1980)-(int)((year-1980)/4));
			//printf("year = %d,d=%d\n",year,d);
			if (t->day == d) {
				/* �H���̓� */
				return TODAY_IS_HOLIDAY;
			}
		}
	}
	return TODAY_IS_NORMAL_DAY;
}
/**
 *	@brief �x�����ǂ����̔��菈��
 *
 *	@param [TIME_INFO_BIN *t] �����i�[�|�C���^
 *
 *	@retval ���茋�� 0:�x�~���ł͂Ȃ�,1:�x�~��
 */
int CheckKyuushiday(TIME_INFO_BIN *t)
{
	int i;
	for ( i = 0; i < holiday_data.kyuushi_count; i++) {
		if ((holiday_data.kyuushi[i].month == t->month) && /* ������v */
			(holiday_data.kyuushi[i].day == t->day) ) { /* ������v */
			/* �x�~�� */
			return 1;
		}
	}
	return 0;
}

/**
 *	@brief �ψڋx�~���̃Z�[�u����
 *
 *	@retval �Ȃ�
 */
static void SaveHoliday(HOLIDAY_COMMAND *p)
{
	memmove(&holiday_data,(char *)(p->d), 182);
	int bcc = CalcBcc((char *)&holiday_data, sizeof(HOLIDAY_DATA) - sizeof(int));
	holiday_data.bcc = bcc;
#ifdef windows	

	FILE *fp;
	char fname[256];
#define HOLIDAY_FILE_NAME ("holiday.ini")

	sprintf(fname,"%s",HOLIDAY_FILE_NAME);
	fp = fopen(fname, "wb");
    if (fp == NULL) {
		printf("%s:%s is not opened!\n",__func__,fname);
    }
	fwrite(&holiday_data, sizeof(HOLIDAY_DATA), 1, fp);
	fclose(fp);

#else
	flash_sector_erase(HOLIDAY_ADDRESS);
	flash_write_buf(HOLIDAY_ADDRESS, (int)&holiday_data, sizeof(HOLIDAY_DATA) / 2);
#endif
}

/**
 *	@brief �ψڋx�~���̃��[�h����
 *
 *	@retval �Ȃ�
 */
static void LoadHoliday(void)
{
#ifdef windows	

	FILE *fp;
	char fname[256];
#define HOLIDAY_FILE_NAME ("holiday.ini")

	sprintf(fname,"%s",HOLIDAY_FILE_NAME);
	fp = fopen(fname, "rb");
    if (fp == NULL) {
		printf("%s:%s is not opened!\n",__func__,fname);
    }
	fread(&holiday_data, sizeof(HOLIDAY_DATA), 1, fp);
	fclose(fp);
#else
	HOLIDAY_DATA *p = (HOLIDAY_DATA *)HOLIDAY_ADDRESS;
	int bcc = CalcBcc((char *)p,sizeof(HOLIDAY_DATA) - sizeof(int));
	if (bcc == p->bcc) {
		//printf("holiday_data load success bcc=%X\n",bcc);
		memmove(&holiday_data, p , sizeof(HOLIDAY_DATA) );
	} else {
		printf("holiday_data load fail p->bcc=%.08X %.08X\n", p->bcc,bcc ) ;
	}
#endif
}
/**
 *	@brief �Ď������̕\������
 *
 *	@retval �Ȃ�
 */
void ContAResponseDisp ( void )
{
	int i;
	RESPONSE_COMMAND *p;
	printf("�������Ď����� ������\n");
	for (i = 0; i < CONTROLER_MAX ; i++) {
		int d = (param.linkage_status >> (4*(CONTROLER_MAX - i - 1)))&0xf;
		if (d != 0) {
			p = &response_command[i] ;
			printf("���� %02X%02X/%02X/%02X %02X:%02X:%02X\n",p->t.year[0]
													,p->t.year[1]
													,p->t.month
													,p->t.day
													,p->t.hour
													,p->t.min
													,p->t.sec);
			printf("�x��(DB7 1)�A�j�� %02X\n",p->t.holiday_week.byte);
			printf("���� %02X,%02X,%02X,%02X,%02X,%02X,%02X\n"
				,p->response.byte1.byte
				,p->response.byte2.byte
				,p->response.byte3.byte
				,p->response.byte4.byte
				,p->response.byte5.byte
				,p->response.byte6.byte
				,p->response.byte7.byte
			);
		}
	}
	
}

/**
 *	@brief �^�p�Ǘ��o�b�̓d�����䏈��
 *
 *	@retval �Ȃ�
 */
static void PCPowerSrv(void)
{
//	return;
	char str[256];
	switch (pcpower_stage_no) {
	case PCPOWER_IDLE:/* �A�C�h����� */
		if (param.no_pc_check_flag == 0) {
			if (now_status.pc_tuushin_status != 0) {
				PcPower(1);
				pcpower_timer = 0;/* �҂��^�C�} */
				shutdown_width = SHUTDOWN_WIDTH;/* �V���b�g�_�E���̉�������ێ� */
				pcpower_stage_no = PCPOWER_SHUTDOWN_ON;
				DebugPrint("PCPowerSrv", "�ʐM�ُ팟�o �V���b�g�_�E���M�����n�m����", 4);
			} else
			if (now_status.power_outage_flag != 0) {
				/* ��d���o */
				PcPower(1);
				pcpower_timer = 0;/* �҂��^�C�} */
				shutdown_width = CHOTTO_SHUTDOWN_WIDTH;/* ������Ɖ����V���b�g�_�E���̉�������ێ� */
				pcpower_stage_no = PCPOWER_SHUTDOWN_ON;
				DebugPrint("PCPowerSrv", "��d���o �V���b�g�_�E���M�����n�m����", 0);
			}
		}
		break;
	case PCPOWER_SHUTDOWN_ON:/* MDM_CS�̏o�͏��� */
		if (pcpower_timer > (1000 * shutdown_width)) {/* shutdown_width�b�҂��^�C�} */
			PcPower(0);
			pcpower_timer = 0;/* �҂��^�C�} */
			pcpower_stage_no = PCPOWER_SHUTDOWN_OFF;
			DebugPrint("PCPowerSrv", "�V���b�g�_�E���M�����n�e�e����", 0);
		}
		break;
	case PCPOWER_SHUTDOWN_OFF:/* �p���[ON�@����w�ߑ��M */
		if (pcpower_timer > (1000 * SHUTDOWN_WAIT)) {/* SHUTDOWN_WAIT�b�҂��^�C�} */
			pcpower_timer = 0;/* �҂��^�C�} */
			pcpower_stage_no = PCPOWER_WAIT;
			sprintf(str, "�V���b�g�_�E���M�����n�e�e����%d�b�o�� ",SHUTDOWN_WAIT );
			DebugPrint("PCPowerSrv", str, 0);
		}
		break;
	case PCPOWER_WAIT://�V���b�g�_�E���M�������SHUTDOWN_WAIT�b�o�ߎ��ɂ����ɗ���
		if (now_status.pc_tuushin_status == 0) {
			pcpower_stage_no = PCPOWER_IDLE;
			DebugPrint("PCPowerSrv", "�ʐM�m�������o ", 0);
		} else if (now_status.power_outage_flag == 0) {
			//��d����̕��A�����o
			PcPower(1);
			pcpower_timer = 0;/* �҂��^�C�} */
			shutdown_width = SHUTDOWN_WIDTH;/* �V���b�g�_�E���̉�������ێ� */
			pcpower_stage_no = PCPOWER_SHUTDOWN_ON;
			DebugPrint("PCPowerSrv", "��d����̕��A�����o �V���b�g�_�E���M�����n�m����", 0);
		} else {
		}
		break;
	default:
		printf("PCPowerSrv default error\n");
		break;
	}

}

