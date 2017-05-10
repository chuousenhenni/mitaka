/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	cont.c
 *	�T�v
 *  ����@���ʕ�
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
#include "conta.h"
#include "contb.h"
#include "cont.h"
#include "io.h"

/*
 *===========================================================================================
 *					�����萔��`�E�^��`�E�}�N����`
 *===========================================================================================
 */


/*
 *===========================================================================================
 *					�����ϐ���`
 *===========================================================================================
 */

static int cont_ms_timer;/* ms�^�C�} */
static int cont_sec_timer;/* �b�^�C�} */

int sw_test_flg;/* �e�X�g�p�Ɏg�p  */
int sw_test_data;/* �e�X�g�p�Ɏg�p  */
int sw;/* SW��Ԃ�ێ� */
static int bef_sw;/* �O���SW��Ԃ�ێ� */
static int teiden_hukki_flg;/* ��d���A�t���O */

int board_rotate_stage_no;/* �ϕ\������̏�Ԋ֐� */

static int cont_bef_led_status[ CONT_LED_MAX_COUNT ];/* �O���LED��� */
int cont_led_status[ CONT_LED_MAX_COUNT ];/* LED��� */
static int cont_led_toggle_status;/* LED �_�Ń��[�h�̂Ō��݂̏�� */
static int cont_led_toggle_count;/* LED �_�Ń��[�h��؂�ւ���܂ł̉� */

static int check_cont_led_ms_timer;/* LED����p��1ms���ɍX�V����^�C�} */

static int date_change_flag;/* ���t�ύX�t���O */
static int offset_time_up_value;/* �^�C���A�b�v�`�F�b�N�p */
static int now_time;/* ���ݎ�����ێ� */

static uint32_t output_data[DISPLAY_BOARD_MAX];/* �e�ϕ\���ւ̏o�̓f�[�^ */
uint32_t input_data[DISPLAY_BOARD_MAX];/* �e�ϕ\������̓��̓f�[�^ */
uint32_t board_status[DISPLAY_BOARD_MAX];/* �e�ϕ\���̐���ُ��� */

static int bef_io_power_outage_flag;/* ��d�����t���O */
int io_power_outage_flag;/* ��d�����t���O */
int power_off_bef_status;/* ��d���̃X�e�[�^�X */

static int io_control_timer;/* IO����p�̃^�C�}�[ */
static int board_rotate_timer;/* �\������p�̃^�C�}�[ */
int schedule_timer;/* �X�P�W���[���v���p�̃^�C�}�[ */
static int lap_count_timer;/* �o�ߎ��ԑ���p�̃^�C�}�[ */
static int lap_time_max;/* �o�ߎ��ԍő�l */
static int board_rotate_total_count;/* �\����]�����S�� */
static int board_status_error_check_flg;/* �ϕ\���G���[����v���t���O */
int err_recover_flg;		/* �ُ한�A�{�^��ON��� */
int keep_byou_status_flg;	/* �����e�ُ��ێ�����t���O�@�@�@1:�ُ� 0:�ʏ� */
int keep_board_status_flg;	/* �ϕ\���ُ��ێ�����t���O�@1:�ُ� 0:�ʏ� kaji20170307 */
int board_choukou_status;	/* ��Ē����w�߂̒l��ێ�����@1:�_�� 0:�œ� kaji20170330 */
int rtc_sync_timer;/* RTC�ɓ�������ms�^�C�} */

PARAM param;/* �ݒ�l */

STATUS now_status;/* ���݂̐ݒ�l */

int first_p22;/* �ŏ��̃I�t�Z�b�g�^�C�}�͓��� */

/* ��|���̐ݒ�l��ێ� */
/* P2�p�^�[����P1,P3�̂ǂ���Ɠ�������ݒ� */
static int p2_status[CONTROLER_MAX][DISPLAY_BOARD_MAX] =
{
	{P1,P1,P1,P1,P3,P3,P3,-1},/* �[������@�P �O��ʂ�k�s�A��s ���ΕW���L��Q��A��2-5-12 ���A��3-41-12 */
	{P3,P3,-1,P1,P1,P1,P1,P1},/* �[������@�Q �O��ʂ��s ���A��3-41-11 */
	{P1,-1,P1,P1,P1,P1,P1,P1},/* �[������@�R �s�� ���A��3-40-16 */
	{P1,-1,-1,P1,P1,P1,P1,P1},/* �[������@�S �O��ʂ�k�s�A��s ���ΕW���L��Q ��A��4-1-2,���A��3-40-10 */
	{P1,P1,P1,P3,P3,-1,P1,P1},/* �[������@�T �O��ʂ�k�s�A��s ��A��4-6,���A��4-20-24 */
	{P1,P1,-1,P1,P1,P1,P1,P1},/* �[������@�U �O��ʂ�k�s ��A��6-6-1 */
	{P1,P1,P1,P2,P2,-1,P1,P1},/* �[������@�V �O��ʂ�k�s�A��s ��A��4-8-5,���A��4-18-23 */
	{P1,-1,P1,P1,P1,P1,P1,P1} /* �[������@�W �O��ʂ�k�s ��A��6-11-4 */
};
/* ���n�̕\�����ǂ�����ݒ� */
/* ���n�̕\���͌u���������Ȃ� */
/* ���n 1 */
static int muji[CONTROLER_MAX][DISPLAY_BOARD_MAX][3] =
{
	{{0,0,0},{0,0,1},{0,0,0},{0,0,0},{1,0,0},{0,0,0},{0,1,1}},	/* �[������@�P*/
//	{{0,0,0},{0,0,1},{0,0,0},{0,0,0},{1,0,0},{0,1,1},{0,0,0}},	/* �[������@�P(kaji20170221�܊p�����Ă��炢�܂����̂Ɂc)*/
	{{0,0,0},{0,0,0}},											/* �[������@�Q */
	{{0,0,0}},													/* �[������@�R */
	{{0,0,0}},													/* �[������@�S */
	{{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,1,1}},					/* �[������@�T */
	{{0,0,0},{0,0,0}},											/* �[������@�U */
	{{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,1,1}},					/* �[������@�V */
	{{0,0,0}} 													/* �[������@�W */
};
/* �N�_�ē��̕\�����ǂ�����ݒ� */
static int kiten_annai[CONTROLER_MAX][DISPLAY_BOARD_MAX] =
{
	{0,0,0,0,0,0,0},											/* �[������@�P */
	{0,0},														/* �[������@�Q */
	{1},														/* �[������@�R */
	{0},														/* �[������@�S */
	{0,0,0,0,0},												/* �[������@�T */
	{0,0},														/* �[������@�U */
	{0,0,0,0,0},												/* �[������@�V */
	{1} 														/* �[������@�W */
};
/* �����e���L�邩��������ݒ� */
static int hakkobyou_exist[CONTROLER_MAX][2] =
{
	{1,1},														/* �[������@�P */
	{0,0},														/* �[������@�Q */
	{0,0},														/* �[������@�R */
	{1,1},														/* �[������@�S */
	{1,1},														/* �[������@�T */
	{0,0},														/* �[������@�U */
	{1,1},														/* �[������@�V */
	{0,0} 														/* �[������@�W */
};
static IO_INFO io_info[CONTROLER_MAX];/* �e����@��IO��ԊǗ��p */
IO_INFO my_io_info;/* ������@��IO��ԊǗ��p */
int display_board_count;/* ������@�̕\������ */
static RESPONSE_COMMAND *response = NULL;/* �Ď������R�}���h */
int startup_error_status;/* �N�����̃G���[�̗L����ێ�(0:�G���[�����A1:�G���[�L��) */

static char request_pattern [][100] = {/* �f�o�b�O�p�̕\���p�^�[�� */
	"P2_P2 �I�t�Z�b�g�^�C�}�҂�",
	"PX_P1 �ʏ��board���ړ�",/* �ʏ�� */
	"PX_P2 ��|��board���ړ�",/* ��|�� */
	"PX_P3 �ψڂ�board���ړ�",/* �ψڂ� */
	"PX_FAIL �t�F�C����board���ړ�",
	"PX_POWEROFF �p���[�I�t�̈ʒu��board���ړ�",
	"PX_POWER_RECOVER ���A�ʒu��board���ړ�"
};

/*
 *===========================================================================================
 *					�O���ϐ���`
 *===========================================================================================
 */

extern int my_tanmatsu_type;/* �[���^�C�v 1:A,2:B... */
extern int remote_command_request;/* ���u���̕\���ύX�v������t���O */
extern int remote_command_command;/* ���u���̕\���ύX�R�}���h */
extern int swtest;
extern int response_received_count[CONTROLER_MAX];/* ����@�a����̎�M�񐔂�ێ����� */
extern int response_time[CONTROLER_MAX];/* �����A����̗v�����琧���B����̉�����M�܂ł̎���(ms) */
extern int response_total_err_count[CONTROLER_MAX];/* kaji20170305 ����@�a����̑��G���[�񐔂�ێ����� */
//extern int fail_flag;/* 20170224 �t�F�C�����M��1�ƂȂ� */
extern int fail_hnd_state;/* kaji20170225 �t�F�C���Ǘ��̏�ԕϐ� */
extern int CheckTanmatuError(void) ;

/*
 *===========================================================================================
 *					����	�֐���`
 *===========================================================================================
 */

static void ContLEDInit(void);
static void ContLEDSrv(void);/* ����@��LED���䏈�� */

static void ActionRemoteSrv(void);/* ���u����IO���쏈�� */
static void ScheduleRequestChkSrv(void);/* �X�P�W���[���X�V�v�����씻�菈�� */
static void BoardRotateSrv(void);/* �ϕ\�����䏈�� */
static int GetWaitingPattern(void);/* �ړ����ׂ��p�^�[�������߂鏈�� */
static void BoardStatusErrorCheck(void);/* �ϕ\���G���[���菈�� */
static void IO_ChangeSrv(void );/* �\����,�e����ω������鏈�� */

static void SWSrv(STATUS *status);/* SW��ԓǂݍ��ݏ��� */
static void ActionSrv(void);/* �e�퓮�쏈�� */
static void PowerOutMainSrv(void);/* ��d�������̏��� */
static void BSPLedSrv(void);/* BSPLED�̕\������ */

static int CheckBoardStatus(int dst);/* ����ȉϕ\���̏�Ԃ��l�����鏈�� */
static int CheckInput(int dst, int display_check_count, int out_put_req);/* IO���̓f�[�^���菈�� */
static void  GetAlowedPattern(int dst, int no, int *allowed_pattern);/* �ڕW�ʒu�ɑ΂��Ă̋������p�^�[�������߂鏈�� */
static void Output(int d, int board_count);/* IO�o�͏��� */

static void SetRequest(BROADCAST_COMMAND *com, char *str);/* ���[�h�ݒ�v�����̏��� */
static void SetNowLEDStatus(int status);/* �X�e�[�^�X��LED�ɔ��f���鏈�� */
static void SetNowLEDStatusToggle(int status);/* �X�e�[�^�X��LED�i�_�ŏ�ԁj�ɔ��f���鏈�� */

static void SetByouStatus(int sw);/* �����e�X�e�[�^�X���Z�b�g���鏈�� */

static int GetStartupBoardPattern(void);/* (��d�N������)�ϕ\���̑���������鏈�� */

/*
 *===========================================================================================
 *					�O���[�o���֐���`
 *===========================================================================================
 */

int GetNowRemoteStatus(void);/* ���݂̉��u���̃X�e�[�^�X��Ԃ����� */
void SetNowStatus(int status);/* �X�e�[�^�X�̃Z�[�u���� */
void IO_ChangeReq(char chane_type);/* �\����,�e����ω������鏈�� */

void ControlerInit(void );
void SetMode(STATUS *status, RESPONSE_COMMAND *response);/* ���݂̃��[�h���Z�b�g */
void SetResponseStatus(RESPONSE_COMMAND *response);/* IO�X�e�[�^�X���Z�b�g */

void SetDefault(void);/* �f�B�t�H���g�p�����[�^�ݒ� */
int CheckErrorStatus(void);/* ���݂̃G���[��Ԃ�now_status���画�肷�鏈�� */

void TimerIntCont(int count);/* ����@���ʂ̃^�C�}���荞�ݏ��� */

void EventRequest(int event);/* ��ԕύX�v������ */

void ContSrv(RESPONSE_COMMAND *response);/* ����@���ʂ̏��� */
void StatusDisp(void);/* �X�e�[�^�X�̕\������ */

void BroadcastCommand(BROADCAST_COMMAND *com);/* ����w�ߎ�M���� */

void SaveParam(void);/* �p�����[�^�̃Z�[�u���� */
int LoadStatus(void);/* �X�e�[�^�X�̃��[�h���� */
void SaveStatus(void);/* �X�e�[�^�X�̃Z�[�u���� */
void LoadParam(void);/* �p�����[�^�̃��[�h���� */

int CheckMuji(int no);/* �W�������n���ǂ����̔��菈�� */

int deb(int code);

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
 *	@brief ����@���ʂ̃^�C�}���荞�ݏ��� (count [ms]��)
 *
 *	@retval �Ȃ�
 */
void TimerIntCont(int count)
{
	cont_ms_timer += count;
	check_cont_led_ms_timer += count;
	io_control_timer += count;/* IO����p�̃^�C�}�[ */
	board_rotate_timer += count;/* �\������p�̃^�C�}�[ */
	schedule_timer += count;/* �X�P�W���[���v���p�̃^�C�}�[ */
	lap_count_timer += count;/* �o�ߎ��ԑ���p�̃^�C�}�[ */
	rtc_sync_timer += count;/* RTC�ɓ�������ms�^�C�} */
	if (rtc_sync_timer >= 999) {
		rtc_sync_timer = 999;
	}
	if (cont_ms_timer >= 1000) {
		cont_ms_timer = 0;
		cont_sec_timer++;
		if ((cont_sec_timer%5) == 0) {
			board_status_error_check_flg = 1;/* �ϕ\���G���[����v���t���O�Z�b�g */
		}
		if (offset_time_up_value != 0) {
			offset_time_up_value--;
		}
	}
}
//int aaa=0;
void ClearSyncTimer(void)
{
	//aaa++;
	//if(aaa>10) {
	//	aaa=0;
	//	printf("ClearSyncTimer %X %X\n",rtc_sync_timer,&rtc_sync_timer);
	//}
	rtc_sync_timer = 0;
}

/**
 *	@brief ����@���ʂ̏���������
 *
 *	@retval �Ȃ�
 */
void ControlerInit(void )
{
	int i;
	int j;
	int st;
	int io_power_outage_flag;
	
	for ( i = 0 ; i< CONTROLER_MAX; i++) {
		io_info[i].display_board_count = 0;
		for ( j = 0 ; j< DISPLAY_BOARD_MAX; j++) {
			if (p2_status[i][j] == -1) {
				break;
			} else {
				if (p2_status[i][j] == P1) {
					/* P1 */
					io_info[i].allowed_pattern_p1[j][0] = P1;/* P1 �܂��� P2 */
					io_info[i].allowed_pattern_p1[j][1] = P2;
					io_info[i].allowed_pattern_p2[j][0] = P2;/* P2 �܂��� P1 */
					io_info[i].allowed_pattern_p2[j][1] = P1;
					io_info[i].allowed_pattern_p3[j][0] = P3;/* P3���� */
					io_info[i].allowed_pattern_p3[j][1] = P3;
				} else {
					/* P3 */
					io_info[i].allowed_pattern_p1[j][0] = P1;/* P1���� */
					io_info[i].allowed_pattern_p1[j][1] = P1;
					io_info[i].allowed_pattern_p2[j][0] = P2;/* P2 �܂��� P3 */
					io_info[i].allowed_pattern_p2[j][1] = P3;
					io_info[i].allowed_pattern_p3[j][0] = P3;/* P3 �܂��� P2 */
					io_info[i].allowed_pattern_p3[j][1] = P2;
				}
				io_info[i].kiten_annai[j] = kiten_annai[i][j];/* �N�_�ē��W�����ǂ������ */
				io_info[i].muji_pattern_p1[j] = muji[i][j][0];/* P1�����n���ǂ��� */
				io_info[i].muji_pattern_p2[j] = muji[i][j][1];/* P2�����n���ǂ��� */
				io_info[i].muji_pattern_p3[j] = muji[i][j][2];/* P3�����n���ǂ��� */
				io_info[i].display_board_count++;
			}
		}
	}
	if ((my_tanmatsu_type>=1) && (my_tanmatsu_type<=8)){
		memmove(&my_io_info, &io_info[my_tanmatsu_type-1], sizeof(IO_INFO));
		display_board_count = my_io_info.display_board_count;
		printf("�ϕW������=%d\n",display_board_count);
		for ( j = 0 ; j < display_board_count; j++) {
			printf("No.%d p1(%d,%d)mj=%d,",j+1
				,my_io_info.allowed_pattern_p1[j][0]
				,my_io_info.allowed_pattern_p1[j][1]
				,my_io_info.muji_pattern_p1[j]
				);
			printf("p2(%d,%d)mj=%d,"
				,my_io_info.allowed_pattern_p2[j][0]
				,my_io_info.allowed_pattern_p2[j][1]
				,my_io_info.muji_pattern_p2[j]
				);
			printf("p3(%d,%d)mj=%d"
				,my_io_info.allowed_pattern_p3[j][0]
				,my_io_info.allowed_pattern_p3[j][1]
				,my_io_info.muji_pattern_p3[j]
				);
			if (my_io_info.kiten_annai[j] == 1)  printf(" �N�_�ē��W��");
			printf("\n");
		}
		printf("�����e�P%s,�����e�Q%s\n"
			,hakkobyou_exist[my_tanmatsu_type-1][0] ? "�L��" : "����"
			,hakkobyou_exist[my_tanmatsu_type-1][1] ? "�L��" : "����"
		);

	} else {
		display_board_count = 10;
	}

	for ( i = 0 ; i< DISPLAY_BOARD_MAX; i++) {
		output_data[ i ] = 0;/* �e�ϕ\���ւ̏o�̓f�[�^ */
		input_data[ i ] = 0;/* �e�ϕ\������̓��̓f�[�^ */
		board_status[ i ] = 0;/* �e�ϕ\���̐���ُ��� */
	}
	sw = 0;
	bef_sw = 0;
	teiden_hukki_flg = 0;/* ��d���A�t���O */
	date_change_flag = 0;/* ���t�ύX�t���O */
	io_control_timer = 0;/* IO����p�̃^�C�}�[ */
	board_rotate_timer = 0;/* �\������p�̃^�C�}�[ */
	schedule_timer = 0;/* �X�P�W���[���v���p�̃^�C�}�[ */
	lap_count_timer = 0;/* �o�ߎ��ԑ���p�̃^�C�}�[ */
	lap_time_max = 0;/* �o�ߎ��ԍő�l */
	board_rotate_total_count = 0;/* �\����]�����S�� */
	board_status_error_check_flg = 0;/* �ϕ\���G���[����v���t���O */
	err_recover_flg = 0;/* �ُ한�A�{�^��ON��� */
	keep_byou_status_flg = 0;/* �����e�ُ��ێ�����t���O */
	keep_board_status_flg = 0;/* �ϕ\���ُ��ێ�����t���O kaji20170307 */
	board_choukou_status = 0;/* ��Ē����w�߂̒l��ێ����� kaji20170330 */
	rtc_sync_timer = 0;/* RTC�ɓ�������ms�^�C�} */
	first_p22 = 0;/* �ŏ��̃I�t�Z�b�g�^�C�}�͓��� */

	SetDefault();/* �f�B�t�H���g�p�����[�^�ݒ� */
	ContLEDInit();/* LED������ */
	LoadParam();/* �p�����[�^�̃��[�h���� */
	param.reset_count++;
//	SaveParam();�v���O�����������݌�̋N���ł͓ǌ����݂Ɏ��s����̂ł�߂�
	LoadStatus();/* �X�e�[�^�X�̃��[�h���� */
	if (now_status.power_outage_flag != 0) {
		/* ��d����̕��A */
		DebugPrint("","��d����̕��A", 1);
		now_status.power_outage_flag = 0;
		SaveStatus();/* �X�e�[�^�X�̃Z�[�u���� */
		teiden_hukki_flg = 1;/* ��d���A�t���O */
	}
	bef_io_power_outage_flag = 0;
	io_power_outage_flag = 0;
	SaveStatus();/* �X�e�[�^�X�̃Z�[�u���� */
	board_rotate_stage_no = BOARD_ROTATE_IDLE;
	
	//now_status.start_time=5*60*60;
	//now_status.end_time=18*60*60;
	if (sw_test_flg != 0) {
		printf("sw_test_data=%X\n",sw_test_data);
		swtest = sw_test_data;
	}
	sw = SWRead();/* SW�ǂݍ��ݏ��� */
	
	/* ���̏�Ԃ̃p�^�[�����Z�b�g���Ă��� */
	int pattern = GetWaitingPattern();
	for( i = 0; i < display_board_count; i++) {
		input_data[i] = pattern;
	}
#if 0 //20170209
	if (now_status.mode == MODE_REMOTE) {
		if ((sw & SW_STATUS_BIT) == 1) {
		/* �i�R�j	��d�O�����u�ŁA�蓮�ɂė����オ�����ꍇ�͍��ڂ̕ύX���s�킸�Ɍ���ێ��Ƃ��� */
			printf("�i�R�j	��d�O�����u�ŁA�蓮�ɂė����オ�����ꍇ�͍��ڂ̕ύX���s�킸�Ɍ���ێ��Ƃ���\n");
			now_status.mode = MODE_MANUAL;
		} else if (now_status.status == STATUS_P1){
			printf("***p1\n");
			IO_ChangeReq(PX_P1);/* ���̈ʒu�Ɉړ����� */
		} else if (now_status.status == STATUS_P2){
			printf("***p2\n");
			IO_ChangeReq(PX_P2);/* ���̈ʒu�Ɉړ����� */
		} else if (now_status.status == STATUS_P3){
			printf("***p3\n");
			IO_ChangeReq(PX_P3);/* ���̈ʒu�Ɉړ����� */
		} else {
			printf("***p4\n");
			IO_ChangeReq(PX_P2);/* ���̈ʒu�Ɉړ����� */
		}
	} else {
#if 1 //yamazaki ���̋@�\�͔��˕W���@�����p�ɋL�ڂ���Ă������A��߂Ă���
		if (param.same_nomove_flag == 0) { /* �����ꍇ�͉ϕ\���ł�ω������Ȃ����[�h */
//			IO_ChangeReq(PX_P3);/* �ʂ̈ʒu�Ɉړ����Ă��� */
			IO_ChangeReq(PX_P1);/* ���̈ʒu�Ɉړ����� */
		}
#endif
	}
#endif
	if ((sw & SW_STATUS_BIT) == 1) {
		/* �蓮�̓��� */
		now_status.mode = MODE_MANUAL;
		IO_ChangeReq(PX_P2);/* ���̈ʒu�i��|�j�Ɉړ����� */
		remote_command_request = 1;/* 20170226 ���u���̕\���ύX�v������t���O */
		remote_command_command = 2;/* 20170226 ���u���̕\����|�ւ̕ύX�R�}���h */
	} else {
		/* ���u�̓��� */
		now_status.mode = MODE_REMOTE;
		remote_command_request = 0;/* ���u���̕\���ύX�v���t���O 0:�v������*/
#if 1 //20170225
		if (my_tanmatsu_type == CONTA_ADDRESS){
			//if(teiden_hukki_flg == 1) {
			//if ((teiden_hukki_flg == 1) && (now_status.power_outage_flag2 == 0)){
			io_power_outage_flag = PowerStatusRead();/* ��d�X�e�[�^�X(UPS-ALM)�ǂݍ��ݏ��� */
//printf("******teiden_hukki_flg=%d,io_power_outage_flag=%d\n",teiden_hukki_flg,io_power_outage_flag);
			if (io_power_outage_flag == 1){
				//st = GetNowRemoteStatus();
//				st = now_status.status;/* 20170226 ��d�O�̏�Ԃ����[�h����Ă��� */
				st = GetStartupBoardPattern();/* 20170301 �\���̌��݈ʒu(����)��ǂݎ�� */
//				if (st == STATUS_P1) {
				if (st == P1) {/* P1�̏ꍇ����P1�i�ʏ�j�Ɉړ����� */
					IO_ChangeReq(PX_P1);/* ���̈ʒu�i�ʏ�j�Ɉړ����� */
				} else {
					IO_ChangeReq(PX_P2);/* ���̈ʒu�i��|�j�Ɉړ����� */
				}
			} else {
				IO_ChangeReq(PX_P2);/* ���̈ʒu�i��|�j�Ɉړ����� */
				first_p22 = 1;/* �ŏ��̃I�t�Z�b�g�^�C�}�͓��� */
				IO_ChangeReq(P2_P2);/* �I�t�Z�b�g�^�C�}�i120�b�҂j */
			}
		} else {
			IO_ChangeReq(PX_P2);/* ���̈ʒu�i��|�j�Ɉړ����� */
		}
#else
		if(teiden_hukki_flg == 1) {
			/* ��d���A�t���O */
			IO_ChangeReq(PX_FAIL);/* �t�F�C���̈ʒu�i��|�j�Ɉړ����� */
		} else {
			// 20170217
			if (my_tanmatsu_type == CONTA_ADDRESS){
				IO_ChangeReq(PX_P2);/* ���̈ʒu�i��|�j�Ɉړ����� */
				first_p22 = 1;/* �ŏ��̃I�t�Z�b�g�^�C�}�͓��� */
				IO_ChangeReq(P2_P2);/* �I�t�Z�b�g�^�C�}�i120�b�҂j */
				//�����ł͂��Ȃ� IO_ChangeReq(PX_P1);/* �ʏ�ɑJ�� */
			} else {
				IO_ChangeReq(PX_P2);/* ���̈ʒu�i��|�j�Ɉړ����� */
			}
		}
#endif
		/*
		���̌�֐�ActionRemoteSrv���ɂ����ĕψڂ̎��ԂȂ̂ŕψڂɈړ����邩�ȁH
		*/
	}
	
	ByouWrite(now_status.status);/* �����e������������ */
	SetNowLEDStatus(now_status.status);/* ������Ԃ�LED�ɔ��f */
	if ((my_tanmatsu_type == 1) || (my_tanmatsu_type ==4)) {
		ToukaWrite(my_tanmatsu_type);/* ���Δւ̏o�͏��� */
	}
	if (my_tanmatsu_type == 4) {/* ������̂� */
		NaishouWrite(now_status.status);/* ���Ɣ����������� */
	}
	
	
//	printf("*****now_status.power_outage_flag=%d\n",now_status.power_outage_flag);
	
	
}

int deb(int code)
{
//	extern int cont_output_data;
//extern uint16_t hakkobyou_out_reg_buf;//�Ȃ���int���Ƃ��������Ȃ��Ă���

//	if ((hakkobyou_out_reg_buf & 0x110) == 0x110) {
//		printf("**********************code=%d,hakkobyou_out_reg_buf=%X\n",code,hakkobyou_out_reg_buf);
//		while(1) {
//			;
//		}
//	}
//	if ((cont_output_data & 0xc000011) != 0) {
//		printf("**********************code=%d,cont_output_data=%X\n",code,cont_output_data);
//		while(1) {
//			;
//		}
//	}
	return 0;
}

/**
 *	@brief ����@���ʂ̏���
 *
 *	@retval �Ȃ�
 */
void ContSrv(RESPONSE_COMMAND *resp)
{
	lap_count_timer = 0;/* �o�ߎ��ԑ���p�̃^�C�}�[ */
	response = resp;/* ����@A,B�̎��g�̉����d���A�h���X�͂����Z�b�g���� */
	deb(1);
	SWSrv(&now_status);/* SW��ԓǂݍ��ݏ��� */
	deb(2);
	ContLEDSrv();/* LED���䏈�� */
	deb(3);
	ActionSrv();/* �e�퓮�쏈�� */
	deb(4);
	PowerOutMainSrv();/* ��d�������̏��� */
	deb(5);
	BSPLedSrv();/* BSPLED�̕\������ */
	deb(6);
	if (board_status_error_check_flg == 1) {
		board_status_error_check_flg = 0;/* �ϕ\���G���[����v���t���O�N���A */
	deb(7);
		BoardStatusErrorCheck();/* �ϕ\���G���[���菈�� */
	deb(8);
	}
	if (lap_time_max < lap_count_timer) {
		lap_time_max = lap_count_timer;/* �o�ߎ��ԍő�l�X�V */
	}
}

/**
 *	@brief ���݂̃��[�h���Z�b�g���鏈��
 *
 *	@retval �Ȃ�
 */
void SetMode(STATUS *status, RESPONSE_COMMAND *response)
{
	int mode = 0;
	
	if (status->test_flag == 1) {
		/* �e�X�g���t���O */
		mode |= 0x10;
	}

	if (status->mode == MODE_MANUAL) {
		/* �[���蓮 */
		mode |= 0x40;
	} else if (status->mode == MODE_MONITOR) {
		/* �Ď��Ղ���̎w�� */
		mode |= 8;
	}
	/* ��ɍ��̏�Ԃ�Ԃ��΂悢�̂ł͂Ȃ����낤�� */
	if (status->status == STATUS_P1) {
		/* �ʏ� */
		mode |= 1;
	} else if (status->status == STATUS_P2) {
		/* ��| */
		mode |= 2;
	} else if (status->status == STATUS_P3) {
		/* �ψ� */
		mode |= 4;
	} else if (status->status == STATUS_FAIL) {
		/* �t�F�C���w�� */
		mode |= 0x20;
	}
	response->response.status[0] = mode;
}

/**
*	@brief ���䉞���p�o�b�t�@(response_command)�ɃZ�b�g���鏈��
 *
 *	@retval �Ȃ�
 */
void SetResponseStatus(RESPONSE_COMMAND *response)
{
	char str[256];
//	response_command.response.byte3.board_error1 = 1;
//	response_command.response.byte3.board_error3 = 1;
	int i;
	int d = 0;
	int board_error_cancel_flag = 0;/* �\���ُ���L�����Z������₢�Ȃ� 0:���Ȃ�,1:���� */
	int bef_board_error = now_status.board_error;
	int bef_kiten_error = response->response.byte2.kiten_error;
	int now_board_error;
	int bef_d = response->response.byte3.byte;/* �W���ُ��ێ�  */

	now_status.board_error = 0;/* �\���ُ�N���A */
	response->response.byte2.tanmatu_error = 0;/* �[������@�ُ�N���A */
	response->response.byte2.kiten_error = 0;/* �N�_�ē��W���ُ�N���A */
	response->response.byte2.jimaku = 0;/* �����ړ����t���O�N���A */
	response->response.byte2.hoshu_status = 0;/* 1:�ێ�/0:�ʏ� */
	if (now_status.byou1_status == 1) {
		response->response.byte2.tanmatu_error = 1;/* �[������@�ُ���Z�b�g */
		response->response.byte4.byou1_error = 1;/* ���H�����e1�ُ�t���O�Z�b�g */
	} else {
		response->response.byte4.byou1_error = 0;/* ���H�����e1�ُ�t���O�N���A */
	}
	if (now_status.byou2_status == 1) {
		response->response.byte2.tanmatu_error = 1;/* �[������@�ُ���Z�b�g */
		response->response.byte4.byou2_error = 1;/* ���H�����e2�ُ�t���O�Z�b�g */
	} else {
		response->response.byte4.byou2_error = 0;/* ���H�����e2�ُ�t���O�N���A */
	}
	if (now_status.musen_status != 0) {
		response->response.byte2.tanmatu_error = 1;/* �[������@�ُ���Z�b�g yamazaki20170207 */
		response->response.byte2.musen_error = 1;/* �����ُ�Z�b�g */
	} else {
		response->response.byte2.musen_error = 0;/* �����ُ�N���A */
	}
	if (now_status.pc_tuushin_status != 0) {
		response->response.byte2.pc_tuushin_error = 1;/* �^�p�Ǘ�PC�ԒʐM�ُ�Z�b�g */
	} else {
		response->response.byte2.pc_tuushin_error = 0;/* �^�p�Ǘ���PC�ʐM�ُ�N���A */
	}
	if (now_status.moniter_tuushin_status != 0) {
		response->response.byte2.moniter_tuushin_error = 1;/* �Ď��ՊԒʐM�ُ�Z�b�g */
	} else {
		response->response.byte2.moniter_tuushin_error = 0;/* �Ď��ՊԒʐM�ُ�N���A */
	}
	if ((my_tanmatsu_type == CONTA_ADDRESS) && (now_status.hoshu_status != 0)) {
		response->response.byte2.hoshu_status = 1;/* 1:�ێ�/0:�ʏ� */
	} else {
		response->response.byte2.hoshu_status = 0;/* 1:�ێ�/0:�ʏ� */
	}
	response->response.byte5.byte = 0;/* ���v�ُ�t���O�N���A */
	if (now_status.time_status == 1) {
		sprintf(str,"���v�ُ�t���O�Z�b�g\n");
		DebugPrint("", str, 1);
		response->response.byte5.byte = 1;/* ���v�ُ�t���O�Z�b�g */
	}

	now_board_error = 0;
	if (bef_board_error == 1) {
		for( i =  0; i < 8; i++){
			d >>= 1;
			if (board_status[i] == 1) {
				d |= 0x80;
				now_board_error = 1;/* �\���ُ���Z�b�g */
			}
		}
	}
	
#if 1
	/* 20170225 �W���ُ픭�����A�ُ한�A���������܂ŕ��������Ȃ� */
	if ((bef_board_error == 1) && (now_board_error == 0)) {
		/* �O��W���ُ�ō���W������ */
		if (err_recover_flg == 0) {
			/* �ُ한�A�{�^����������Ă��Ȃ���� */
			board_error_cancel_flag = 1;/* �\���ُ한�A���L�����Z������₢�Ȃ� 0:���Ȃ�,1:���� */
			sprintf(str, "***board�ُ한�A���L�����Z������\n");
			DebugPrint("", str, 2);
		} else {
			sprintf(str, "***board�ُ한�A���L�����Z�����Ȃ�\n");
			DebugPrint("", str, 2);
		}
	}
	
	if (board_error_cancel_flag == 1) {
		/* �\���ُ한�A���L�����Z������ */
		now_status.board_error = 1;/* �\���ُ���Z�b�g */
		response->response.byte2.tanmatu_error = 1;/* �[������@�ُ���Z�b�g */
		response->response.byte2.kiten_error = bef_kiten_error;
	} else {
		/* �\���ُ한�A���L�����Z�����Ȃ� */
		for( i =  0; i < 8; i++){
			d >>= 1;
			if (board_status[i] == 1) {
				d |= 0x80;
				now_status.board_error = 1;/* �\���ُ���Z�b�g */
				response->response.byte2.tanmatu_error = 1;/* �[������@�ُ���Z�b�g */
				if (my_io_info.kiten_annai[i] == 1) {
					/* �N�_�ē��W���Ȃ̂� */
					response->response.byte2.kiten_error = 1;
				}
			}
		}
	}
	
	if (startup_error_status != 0) {
		/* �N�����G���[��Ԃ̂܂܂̏ꍇ�͂��₨�������ɒ[������@�ُ���Z�b�g */
		response->response.byte2.tanmatu_error = 1;/* �[������@�ُ���Z�b�g */
	}
#else
	for( i =  0; i < 8; i++){
		d >>= 1;
		if (board_status[i] == 1) {
			d |= 0x80;
			now_status.board_error = 1;/* �\���ُ���Z�b�g */
			response->response.byte2.tanmatu_error = 1;/* �[������@�ُ���Z�b�g */
			if (my_io_info.kiten_annai[i] == 1) {
				/* �N�_�ē��W���Ȃ̂� */
				response->response.byte2.kiten_error = 1;
			}
		}
	}
	
#endif
	
	if (response->response.byte2.tanmatu_error == 0) {//20170225
		err_recover_flg = 0;/* kaji20170223 */
	}
	
	
	if (now_status.tanmatu_error != response->response.byte2.tanmatu_error) {
		now_status.tanmatu_error = response->response.byte2.tanmatu_error;
		SaveStatus();/* �X�e�[�^�X��ۑ� */
	}
	if (now_status.jimaku_ido_flag != 0) {
		response->response.byte2.jimaku = 1;/* �}�������ړ����t���O */
	}
	if (board_error_cancel_flag == 1) {
		/* �\���ُ한�A���L�����Z������ */
		response->response.byte3.byte = bef_d;
	} else {
		response->response.byte3.byte = d;
	}
	//char str[128];
	//sprintf(str, "*** SetResponseStatus*** byte3=%X,byte4=%X", response->response.byte3.byte, response->response.byte4.byte);
	//DebugPrint("", str, 1);

}

/*
 *===========================================================================================
 *					�����֐�
 *===========================================================================================
 */

/**
 *	@brief BSPLED�̕\������
 *         LED3 �_��
 *         LED4 0:���u,1:�蓮 or �Ď���
 *         LED7,7 00:�ʏ�,11:�ψ�,01,10:��|(�ω����̓I�t�Z�b�g�^�C�}���쒆)
 *
 *
 *	@retval �Ȃ�
 */
static void BSPLedSrv(void)
{
#ifndef windows
	if (cont_sec_timer < 2) {/* 2�b�Ԃ�DGSW�̏�Ԃ�\�����Ă��� */
		return;
	}
	if (now_status.mode == MODE_REMOTE) {
		BSP_LEDOff(BSP_LED_4);
	} else {
		BSP_LEDOn(BSP_LED_4);
	}
	if (now_status.status == STATUS_P1) {
		BSP_LEDOff(BSP_LED_7);
		BSP_LEDOff(BSP_LED_8);
	} else if (now_status.status == STATUS_P3) {
		BSP_LEDOn(BSP_LED_7);
		BSP_LEDOn(BSP_LED_8);
	} else {
		if (offset_time_up_value % 2) {
			BSP_LEDOff(BSP_LED_7);
			BSP_LEDOn(BSP_LED_8);
		} else {
			BSP_LEDOn(BSP_LED_7);
			BSP_LEDOff(BSP_LED_8);
		}
	}
#endif
}

/**
 *	@brief LED����������
 *
 *	@retval �Ȃ�
 */
static void ContLEDInit(void)
{
	int i;
	
	cont_led_toggle_count = TOGGLE_COUNT_MAX;
	cont_led_toggle_status = LED_OFF;
	for ( i = 0 ; i < CONT_LED_MAX_COUNT; i++) {
		cont_bef_led_status[i] = LED_STATUS_OFF;
		cont_led_status[i] = LED_STATUS_OFF;
	}
	cont_led_status[LED_TEIDEN] = LED_STATUS_ON;/* �N�����͒�d�\���� */

}

/**
 *	@brief ����@��LED���䏈��
 *
 *	@retval �Ȃ�
 */
static void ContLEDSrv(void)
{
	int i;
	char str[100];
	
	for( i = 0 ; i < CONT_LED_MAX_COUNT; i++) {
		if (cont_bef_led_status[i] != cont_led_status[i]) {
			switch (cont_led_status[i]) {
			case LED_STATUS_OFF:
				sprintf(str, "Cont(%d) LED_STATUS_OFF",i);
				/* �������o�� */
				ContLedOut(i,LED_STATUS_OFF );
				break;
			case LED_STATUS_ON:
				sprintf(str, "Cont(%d) LED_STATUS_ON",i);
				/* �Γ_�����o�� */
				ContLedOut(i,LED_STATUS_ON );
				break;
			case LED_STATUS_TOGGLE:
				sprintf(str, "Cont(%d) LED_STATUS_TOGGLE",i);
				cont_led_toggle_status = LED_OFF;
				check_cont_led_ms_timer = 0;
				break;
			default :
				break;
			}
			//DebugPrint("", str, 2);
			cont_bef_led_status[i] = cont_led_status[i];
		}
	}
	/* �_�œ���̐؂�ւ����� */
	if (check_cont_led_ms_timer >= cont_led_toggle_count ) {
		for( i = 0 ; i < CONT_LED_MAX_COUNT; i++) {
			switch(cont_led_status[i]) {
			case LED_STATUS_TOGGLE:
				if (cont_led_toggle_status == LED_OFF) {
					sprintf(str, "Cont LED_ON  %d",check_cont_led_ms_timer);
					/* �_�����o�� */
					ContLedOut(i,LED_STATUS_ON );
				} else {
					sprintf(str, "Cont LED_OFF %d",check_cont_led_ms_timer);
					/* �������o�� */
					ContLedOut(i,LED_STATUS_OFF );
				}
				//DebugPrint("", str, 2);
				break;
			default :
				break;
			}
		}
		if (cont_led_toggle_status == LED_OFF) {
			cont_led_toggle_status = LED_ON;
		} else {
			cont_led_toggle_status = LED_OFF;
		}
		check_cont_led_ms_timer = 0;
		
	}
	ContLedWrite();/* LED�ɏo�͂��� */
}

/* SW��ԓǂݍ��ݏ��� */
static void SWSrv(STATUS *status)
{
/*
	�R���\�[������
	d ���^�[���Ƃ��ĕ\�����I�t����
	���̌� sw XX�@���^�[���Ƃ���
	XX=10�͋N���{�^��
	   3->13 �[���蓮�̒ʏ�
	   5->15 �[���蓮�̈�|
	   9->19 �[���蓮�̕ψ�
	   0  ���u�ɖ߂�
*/
	int sw = SWRead();/* SW�ǂݍ��ݏ��� */
	char str[256];
	if (bef_sw != sw) {
		sprintf(str, "bef_sw(%.02X)->sw(%.02X)", bef_sw, sw);
		DebugPrint("", str, 0);
		if ((bef_sw & SW_STATUS_BIT) != (sw & SW_STATUS_BIT)) {
			if ((sw & SW_STATUS_BIT) == 0) {
				/* �蓮���牓�u�ɕς���� */
				sprintf(str,"SW�ɂ��[���蓮���牓�u���[�h�Ɉڍs");
				DebugPrint("", str, 0);
				EventRequest(REMOTE_REQUEST);/* ���u���N�G�X�g */
				SaveStatus();/* �X�e�[�^�X��ۑ� */
			}
		}
		if ((sw & SW_STATUS_BIT) != 0) {
			/* �蓮��� */
			now_status.mode = MODE_MANUAL;/* 20170303 */
			now_status.manual_status = 0;/* kaji20170303 �蓮��� 0:SW�ɂ��蓮,1:�^�pPC����̎蓮�ɂ�� */
			if ((bef_sw & SW_START_BIT) != (sw & SW_START_BIT)) {
				if ((sw & SW_START_BIT) != 0) {
					/* �N���{�^���������ꂽ */
					if (board_rotate_stage_no != BOARD_ROTATE_IDLE) { //20170216
							sprintf(str,"�A�C�h����Ԃł͂Ȃ��̂ŃL�����Z��");
							DebugPrint("", str, 0);
					} else if ((sw & SW_TUUJOU_BIT) != 0) {
						/* �ʏ� sw 0->0x13 */
						now_status.manual_status = 0;/* �蓮��� 0:SW�ɂ��蓮,1:�^�pPC����̎蓮�ɂ�� */
						if (now_status.status == STATUS_P2) {
							sprintf(str,"SW�ɂ��蓮 �ʏ�ɐݒ�");
							DebugPrint("", str, 0);
							EventRequest(SW_MANUAL_TUUJOU_REQUEST);/* SW�蓮�ʏ탊�N�G�X�g */
						} else if (now_status.status == STATUS_FAIL) {
							sprintf(str,"�t�F�C������SW�ɂ��蓮 �ʏ�ɐݒ�");
							DebugPrint("", str, 0);
							EventRequest(SW_MANUAL_TUUJOU_REQUEST);/* SW�蓮�ʏ탊�N�G�X�g */
						} else if (now_status.status == STATUS_P1) {
							sprintf(str,"�ʏ킾��SW�ɂ��蓮 �ʏ�ɐݒ�");//20170205
							DebugPrint("", str, 0);
							EventRequest(SW_MANUAL_TUUJOU_REQUEST);/* SW�蓮�ʏ탊�N�G�X�g */
						} else {
							sprintf(str,"SW�ɂ��蓮 �ʏ�ɐݒ�͈�|�łȂ����߃L�����Z��");
							EventRequest(SW_MANUAL_TUUJOU_REQUEST);/* SW�蓮�ʏ탊�N�G�X�g */
							DebugPrint("", str, 0);
						}
					} else if ((sw & SW_ISSOU_BIT) != 0) {
						/* ��| 0->0x15 */
						now_status.manual_status = 0;/* �蓮��� 0:SW�ɂ��蓮,1:�^�pPC����̎蓮�ɂ�� */
						sprintf(str,"SW�ɂ��蓮 ��|�ɐݒ�");
						DebugPrint("", str, 0);
						EventRequest(SW_MANUAL_ISSOU_REQUEST);/* SW�蓮��|���N�G�X�g */
					} else if ((sw & SW_HENNI_BIT) != 0) {
						now_status.manual_status = 0;/* �蓮��� 0:SW�ɂ��蓮,1:�^�pPC����̎蓮�ɂ�� */
						if (now_status.status == STATUS_P2) {
							/* �ψ� 0->0x19 */
							sprintf(str,"SW�ɂ��蓮 �ψڂɐݒ�");
							DebugPrint("", str, 0);
							EventRequest(SW_MANUAL_HENNI_REQUEST);/* SW�蓮�ψڃ��N�G�X�g */
						} else if (now_status.status == STATUS_FAIL) {
							/* �ψ� 0->0x19 */
							sprintf(str,"�t�@�C������SW�ɂ��蓮 �ψڂɐݒ�");
							DebugPrint("", str, 0);
							EventRequest(SW_MANUAL_HENNI_REQUEST);/* SW�蓮�ψڃ��N�G�X�g */
						} else if (now_status.status == STATUS_P3) {
							/* �ψ� 0->0x19 */
							sprintf(str,"�ψڂ���SW�ɂ��蓮 �ψڂɐݒ�");//20170205
							DebugPrint("", str, 0);
							EventRequest(SW_MANUAL_HENNI_REQUEST);/* SW�蓮�ψڃ��N�G�X�g */
						} else {
							sprintf(str,"SW�ɂ��蓮 �ψڂɐݒ�͈�|�łȂ����߃L�����Z��");
							DebugPrint("", str, 0);
						}
					}
					/* 20170221 */
					/* CDS��� */
					if (now_status.cds_status == 1) {
						ChoukouWrite(1);/* ���������o�͏��� */
					} else {
						ChoukouWrite(0);/* ���������o�͏��� */
					}
					ByouWrite(now_status.status);/* �����ɑΉ����邽�� */
					ToukaWrite(my_tanmatsu_type);//20170129 yamazaki
				}
			}
		}
		if ((bef_sw & SW_ERROR_RECOVERT_BIT) != (sw & SW_ERROR_RECOVERT_BIT)) {
			if ((sw & SW_ERROR_RECOVERT_BIT) != 0) {
				/* �ُ한�A�{�^���������ꂽ */
				sprintf(str,"�ُ한�A�{�^���������ꂽ");
				DebugPrint("", str, 0);
				startup_error_status = 0;/* �N�����̃G���[�̗L����ێ�(0:�G���[�����A1:�G���[�L��) */
				err_recover_flg = 1;/* �ُ한�A�{�^��ON��� */
				/* kaji20170223�� */
				now_status.byou_status = 0;
				now_status.byou1_status = 0;
				now_status.byou2_status = 0;
				keep_byou_status_flg = 0;
				ByouWrite(now_status.status);/* �����ɑΉ����邽�� */
				/* kaji20170223�� */
				keep_board_status_flg = 0;/* kaji20170307 */
				cont_led_status[LED_BOARD] = LED_STATUS_OFF;
				cont_led_status[LED_BYOU] = LED_STATUS_OFF;
				cont_led_status[LED_DENSOU] = LED_STATUS_OFF;
//				cont_led_status[LED_MUSEN] = LED_STATUS_OFF;/* kaji20170301 �����͎������A�Ȃ̂ŏ����Ȃ����Ƃɂ��� */
			}
		}
		if ((bef_sw & SW_TEIDEN_HUKKI_BIT) != (sw & SW_TEIDEN_HUKKI_BIT)) {
			if ((sw & SW_TEIDEN_HUKKI_BIT) != 0) {
				/* ��d���A�{�^���������ꂽ */
				sprintf(str,"��d���A�{�^���������ꂽ");
				DebugPrint("", str, 0);
				cont_led_status[LED_TEIDEN] = LED_STATUS_OFF;/* ��d�\�������� */
//				TeidenDispWrite( 0);/* ��d�\�����N���A */
			}
		}
		if ((bef_sw & SW_MAINTENANCE_BIT) != (sw & SW_MAINTENANCE_BIT)) {
			if ((sw & SW_MAINTENANCE_BIT) == 0) {
				/* �ێ��Ԃ���ʏ��Ԃɕω����� */
				now_status.hoshu_status = 0;/* 1:�ێ�/0:�ʏ� */
				sprintf(str,"�ێ��Ԃ���ʏ��Ԃɕω�����");
				DebugPrint("", str, 0);
			} else {
				/* �ʏ��Ԃ���ێ��Ԃɕω����� */
				now_status.hoshu_status = 1;/* 1:�ێ�/0:�ʏ� */
				sprintf(str,"�ʏ��Ԃ���ێ��Ԃɕω�����");
				DebugPrint("", str, 0);
			}
		}
		if ((bef_sw & SW_KEIKOUTOU_BIT) != (sw & SW_KEIKOUTOU_BIT)) {
			if ((sw & SW_KEIKOUTOU_BIT) == 0) {
				/* �u��������Ԃ���u����������Ԃɕω����� */
				now_status.keikoutou_status = 0;/* 1:�u������/0:���� */
				sprintf(str,"�u��������Ԃ���u����������Ԃɕω�����");
				DebugPrint("", str, 0);
			} else {
				/* �u����������Ԃ���u��������Ԃɕω����� */
				now_status.keikoutou_status = 1;/* 1:�u������/0:���� */
				sprintf(str,"�u����������Ԃ���u��������Ԃɕω�����");
				DebugPrint("", str, 0);
			}
			/* CDS��� */
			if (now_status.cds_status == 1) {
				ChoukouWrite(1);/* ���������o�͏��� */
			} else {
				ChoukouWrite(0);/* ���������o�͏��� */
			}
			ByouWrite(now_status.status);/* �����ɑΉ����邽�� */
			ToukaWrite(my_tanmatsu_type);//20170129 yamazaki
		}
		if ((bef_sw & SW_BYOU_BIT) != (sw & SW_BYOU_BIT)) {
			if (((bef_sw & SW_BYOU_BIT) == 0) && ((sw & SW_BYOU_BIT) != 0)){
				/* �����e���킩�甭���e�ُ�ɕω����� */
				now_status.byou_status = 1;/* �����e��� */
				keep_byou_status_flg = 1;/* �����e�ُ��ێ�����t���O */
				sprintf(str,"�����e���킩�甭���e�ُ�ɕω����� 2");
//				DebugPrint("", str, 0x10);
				DebugPrint("", str, 0);
				cont_led_status[LED_BYOU] = LED_STATUS_TOGGLE;
				SetByouStatus(sw);/* �����e�X�e�[�^�X���Z�b�g���鏈�� */
			//} else if (((bef_sw & SW_BYOU_BIT) != 0) && ((sw & SW_BYOU_BIT) == 0)){
				/* ��x�G���[�ƂȂ������x�ƕ��A���Ȃ� 20170130 �� �������A���Ȃ� kaji20170223 */
			} else if (((bef_sw & SW_BYOU_BIT) != 0) && ((sw & SW_BYOU_BIT) == 0) && (keep_byou_status_flg == 0)){
				/* �����e�ُ킩�甭���e����ɕω����� */
				now_status.byou_status = 0;/* �����e��� */
//printf("pass 1\n");
				sprintf(str,"�����e�ُ킩�甭���e����ɕω����� 4");
				DebugPrint("", str, 0x10);
				cont_led_status[LED_BYOU] = LED_STATUS_OFF;
				ByouWrite(now_status.status);/* �����ɑΉ����邽�� */
			} else {
				sprintf(str,"�����e�ُ�(%X)���甭���e�ُ�(%X)�ɕω�����",bef_sw, sw );
				DebugPrint("", str, 0x10);
			}
		}
		
		bef_sw = sw;
	}
}

/* �����e�X�e�[�^�X���Z�b�g���鏈�� */
void SetByouStatus(int sw)
{
	if ((sw & SW_BYOU1_BIT) != 0) {
		now_status.byou1_status =1;
	} else {
		now_status.byou1_status =0;
	}
	if ((sw & SW_BYOU2_BIT) != 0) {
		now_status.byou2_status =1;
	} else {
		now_status.byou2_status =0;
	}
	if (param.no_fail_flag == 1) {/* ���̏ꍇ�̓f�o�b�O�p�Ƀt�F�C���ɂ͂��Ȃ� */
		now_status.byou_status = 0;
		now_status.byou1_status = 0;
		now_status.byou2_status = 0;
	}

//printf("pass 1 %d,%d,%d\n",now_status.byou_status,now_status.byou1_status,now_status.byou2_status);
/* �����Ŕ����e�̗L�薳��������s���ُ�X�e�[�^�X���N���A���邱�Ƃɂ��� */
	/* ���R�F���ꂪ��ԊȒP�Ǝv���邩�� */
	if (hakkobyou_exist[my_tanmatsu_type-1][0] == 0) {
		if (hakkobyou_exist[my_tanmatsu_type-1][1] == 0) {
			/* �����e�P�����A�Q���� */
			now_status.byou_status = 0;
			now_status.byou1_status = 0;
			now_status.byou2_status = 0;
			cont_led_status[LED_BYOU] = LED_STATUS_OFF;
		} else {
			/* �����e�P�����A�Q�L�� */
			printf("�����e�P�����A�Q�L��̏ꍇ�̏��� %d,%d\n",now_status.byou1_status,now_status.byou2_status);
			now_status.byou1_status = 0;
			if (now_status.byou2_status == 0) {
				/* �����e�Q���ُ�łȂ��Ȃ� */
				now_status.byou_status = 0;
				now_status.byou1_status = 0;
				now_status.byou2_status = 0;
				cont_led_status[LED_BYOU] = LED_STATUS_OFF;
			}
		}
	} else {
		if (hakkobyou_exist[my_tanmatsu_type-1][1] == 0) {
			/* �����e�P�L��A�Q���� */
			if (now_status.byou1_status == 0) {
				/* �����e�P���ُ�łȂ��Ȃ� */
				now_status.byou_status = 0;
				now_status.byou1_status = 0;
				now_status.byou2_status = 0;
				cont_led_status[LED_BYOU] = LED_STATUS_OFF;
			}
		} else {
			/* �����e�P�L��A�Q�L�� */
		}
	}
//printf("pass 3 %d,%d,%d\n",now_status.byou_status,now_status.byou1_status,now_status.byou2_status);
}

/* �e�퓮�쏈�� */
/*
�����Ԕ���
�ُ픻��
*/
static void ActionSrv(void)
{
//20170217	if (now_status.mode == MODE_REMOTE) {
	if ((now_status.mode == MODE_REMOTE) && (my_tanmatsu_type == CONTA_ADDRESS)){
		ActionRemoteSrv();/* ���u���̓��씻�菈�� */
	}
	ScheduleRequestChkSrv();/* �X�P�W���[���X�V�v�����씻�菈�� */
	BoardRotateSrv();/* IO���쏈�� */
	IO_ChangeSrv();/* �\����,�e����ω������鏈�� */
}

/**
 *	@brief ���u���̓��씻�菈��
 *
 *	@retval �Ȃ�
 */
static void ScheduleRequestChkSrv(void)
{
	char str[256];
//#define NEW_DAY_TIME (60) /* �V���Ȉ�����n�܂鎞��(00:01�Ƃ���) */
#define NEW_DAY_TIME (1) /* �V���Ȉ�����n�܂鎞��(00:01�Ƃ���) */
//#define NEW_DAY_TIME (10*60+43) /* �V���Ȉ�����n�܂鎞�� */
	if ((date_change_flag == 0) && (GetTodaysMinute() == NEW_DAY_TIME)) {/* kaji20170302 1�b�̊Ԃɗ��Ȃ����Ƃ��뜜���Ă����H */
//	if ((date_change_flag == 0) && (GetTodaysSecond() == NEW_DAY_TIME)) {/* kaji20170223 */
	//if ((date_change_flag == 0) && (GetTodaysMinute() == 0)) {
		/* �V���Ȉ�����n�܂��� */
		/* �����ŃX�P�W���[����v���������Ƃ��� */
		sprintf(str,"�V���Ȉ�����n�܂���");
		DebugPrint("", str, 2);
		date_change_flag = 1;
		now_status.schedule = 0;/* ����ŃX�P�W���[���o�^�v���𑗐M���� */
		now_status.time_req = 0;/* ����Ŏ����ݒ�v���𑗐M���� */
		now_status.start_time = 0;
		now_status.end_time = 0;
		schedule_timer = 0;/* �X�P�W���[���v���p�̃^�C�}�[ */
		
	} else if ((date_change_flag == 1) && (GetTodaysMinute() != NEW_DAY_TIME)) {
		sprintf(str,"�V���Ȉ�����n�܂���+1");
		DebugPrint("", str, 2);
		date_change_flag = 0;
		/*
		�����ŉ^�p�Ǘ�PC����̃X�P�W���[���o�^������Ă��Ȃ�������
		����@A�͎����̏�񂩂�o�^���s���A
		����@B�ɑ��M����
		*/
//		if (now_status.schedule == 0) {
//			if (my_tanmatsu_type == CONTA_ADDRESS) {
//				/* ����@A */
//				SetSchedule( );/* �{���̃X�P�W���[�����Z�b�g���鏈�� */
//			}
//		}
	}

}

/**
 *	@brief ���u���̓��씻�菈��
 *
 *	@retval �Ȃ�
 */
static void ActionRemoteSrv(void)/* ���u���̓��� */
{
	char str[256];
/*
	��ԕω��̏���

	�x�����[�h�ł͖���
	�J�n���ԂɂȂ���
	�I�����ԂɂȂ���
	�蓮�ŕω��v����������
*/
	
	int tmp_now_time = GetTodaysSecond();/* ���݂̎����擾�i�b���Z�j*/
	if ( tmp_now_time == now_time ) {
		/* �����ɕω���������Ή������Ȃ� */
		return;
	}

	if ((now_status.start_time == 0) && (now_status.end_time == 0)) {
		if ((now_status.status != STATUS_P1) && (now_status.status != STATUS_FAIL)) {
			/* ���Ԃ��ݒ肳��Ă��Ȃ��ꍇ����FAIL�łȂ��ꍇ�͉��u���͒ʏ�Ƃ��� */
//����͂܂����A�蓮���牓�u�ɕς�����Ƃ��j���񂷂�
//		printf("A******now_status.status=%d\n",now_status.status);
//			SetNowStatus(STATUS_P1);/* ���u�ʏ� */;
//		printf("A******now_status.status=%d\n",now_status.status);
		}
//���u�ψڂŋN������Ƃ��߂ɂȂ�̂�		return;
	}
	/* ����@B�ɑ��M����ꍇ�͂����L���ɂ��邪�A���͖��g�p */
	//if (my_tanmatsu_type == CONTA_ADDRESS) {
		if ((now_status.status == STATUS_P1) && ((GetNowRemoteStatus() == STATUS_P1P2) || (GetNowRemoteStatus() == STATUS_P3))) {
			/* �X�e�[�^�X���ʏ�ō��̎��Ԃ��ψ� */
//			if (fail_flag == 0) {/* 20170224 �t�F�C�����͈ړ����Ȃ� */
			if (fail_hnd_state == FAIL_HND_NONE) {/* kaji20170225 �t�F�C�����͈ړ����Ȃ� */
				if (empty(IO_QUEUE) && (board_rotate_stage_no == BOARD_ROTATE_IDLE) ) {
					sprintf(str, "***�X�e�[�^�X���ʏ�ō��̎��Ԃ��ψ�");
					DebugPrint("", str, 4);
					EventRequest(REMOTE_TUUJOU2HENNI_REQUEST);/* ���u���ψڎ��ԊJ�n���N�G�X�g */
					//remote_command_request = 1;/* ���u���̕\���ύX�v������t���O */
					//remote_command_command = 2;/* ���u���̕\����|�ւ̕ύX�R�}���h */
				}
			}
		}
		if ((now_status.status == STATUS_P3) && ((GetNowRemoteStatus() == STATUS_P3P2) || (GetNowRemoteStatus() == STATUS_P1))) {
			/* �X�e�[�^�X���ψʂō��̎��Ԃ��ʏ� */
//			if (fail_flag == 0) {/* 20170224 �t�F�C�����͈ړ����Ȃ� */
			if (fail_hnd_state == FAIL_HND_NONE) {/* kaji20170225 �t�F�C�����͈ړ����Ȃ� */
				if (empty(IO_QUEUE) && (board_rotate_stage_no == BOARD_ROTATE_IDLE) ) {
					sprintf(str, "***�X�e�[�^�X���ψʂō��̎��Ԃ��ʏ�");
					DebugPrint("", str, 4);
					EventRequest(REMOTE_HENNI2TUUJOU_REQUEST);/* ���u���ʏ펞�ԊJ�n���N�G�X�g */
					//remote_command_request = 1;/* ���u���̕\���ύX�v������t���O */
					//remote_command_command = 2;/* ���u���̕\����|�ւ̕ύX�R�}���h */
				}
			}
		} else if ((now_status.status == STATUS_FAIL) && ((GetNowRemoteStatus() == STATUS_P3P2) || (GetNowRemoteStatus() == STATUS_P1))) {
			/* �X�e�[�^�X���ψʂō��̎��Ԃ��ʏ� */
//20170212			if (empty(IO_QUEUE) && (board_rotate_stage_no == BOARD_ROTATE_IDLE) ) {
//20170212				sprintf(str, "***�X�e�[�^�X���t�F�C���ō��̎��Ԃ��ʏ�");
//20170212				DebugPrint("", str, 4);
//20170212				EventRequest(REMOTE_HENNI2TUUJOU_REQUEST);/* ���u���ʏ펞�ԊJ�n���N�G�X�g */
				//remote_command_request = 1;/* ���u���̕\���ύX�v������t���O */
				//remote_command_command = 2;/* ���u���̕\����|�ւ̕ύX�R�}���h */
//20170212			}
		}
		now_time = tmp_now_time;/* �������X�V���Ă��� */
	//}
}

/**
 *	@brief IO���쏈��
 *
 *	@retval �Ȃ�
 */
int waiting_patterm;
int bef_match_count;/* �O��̍��ڈ�v�� */
int allowed_pattern[2];/* �������p�^�[����ێ� */

/**
 *	@brief �ϕ\�����䏈��
 *
 *	@retval �Ȃ�
 */
int board_rotate_start_time[DISPLAY_BOARD_MAX];
int board_rotate_delay_time = 0;/* ���Ԃ����炵�Đ�����J�n���邽�߂ɕϐ�*/
int bef_board_rotate_count = 0;
static void BoardRotateSrv(void)
{
	int i;
	int d;
	int match_count;
	int board_rotate_count;
	int out_put_req;/* out_put����������(power off�p) */
	int st;
	
	switch (board_rotate_stage_no) {
	case BOARD_ROTATE_READY:
		io_control_timer = 0;
		bef_match_count = -1;/* ���Ȃ炸�o�͂��邽�߂́[�P */
		board_rotate_timer = 0;
		for ( i = 0 ; i < display_board_count; i++) {
			board_rotate_start_time[i] = i * board_rotate_delay_time;/* ����J�n���Ԃ��Z�b�g */
		}
		waiting_patterm = GetWaitingPattern();/* �ړ����ׂ��p�^�[�������߂� */
		SetResponseStatus(response);/* IO�X�e�[�^�X���Z�b�g */
		//now_status.jimaku_ido_flag = 1;/* �}�������ړ����ɂ��� */
		//cont_led_status[LED_ZUGARA] = LED_STATUS_ON;
		board_rotate_stage_no = BOARD_ROTATE_START;
		bef_board_rotate_count = 0;
		break;
	case BOARD_ROTATE_START:
		board_rotate_total_count++;/* �\����]�����S�� */
		board_rotate_count = 0;
		for ( i = 0 ; i < display_board_count; i++) {
			if (board_rotate_timer >= board_rotate_start_time[i]) {
				board_rotate_count++;
			}
		}
		out_put_req = 0;
		if ( bef_board_rotate_count != board_rotate_count) {
			bef_board_rotate_count = board_rotate_count;
			out_put_req = 1;
		}
		CheckInput(waiting_patterm, board_rotate_count, out_put_req);/* IO���̓f�[�^���菈�� */
		if (board_rotate_count == display_board_count) {
			/* �S�\���̐�����J�n���Ă��� */
			board_rotate_stage_no = BOARD_ROTATE_WAIT;
		}
		
		break;
	case BOARD_ROTATE_WAIT:
			if (io_control_timer > UNMATCH_TIMEOUT_VALUE) {
				/* �^�C���A�E�g���� */
				DebugPrint("BoardRotateSrv","BOARD_ROTATE_WAIT�@�^�C���A�E�g����", 0);
				board_rotate_stage_no = BOARD_ROTATE_END;
			}
		
			d = CheckInput(waiting_patterm, display_board_count, 0);/* IO���̓f�[�^���菈�� */
			if ( d == waiting_patterm) {
				board_rotate_stage_no = BOARD_ROTATE_END;
			} else {
				/* ���߂���w��ʒu�ɂ������ꍇ�͐}�������ړ����ɂ��Ȃ��̂ł����ŃZ�b�g���� */
				now_status.jimaku_ido_flag = 1;/* �}�������ړ����ɂ��� */
				cont_led_status[LED_ZUGARA] = LED_STATUS_ON;
				SetNowLEDStatusToggle(now_status.status);/* �X�e�[�^�XLED��_�ŏ�Ԃɂ��鏈�� */
			}

		break;
	case BOARD_ROTATE_END:
			match_count = CheckBoardStatus(waiting_patterm);/* ����ȉϕ\���̏�Ԃ��l�����鏈�� */
			if (match_count == display_board_count) {
//				if (startup_error_status != 0) {/* kaji20170223 */
				if (keep_board_status_flg != 0) {/* kaji20170307 */
					/* �G���[���������ǂ��G���[�t���O�ێ�����Ă���̂ŏ�Ԍp�� */
					DebugPrint( "BoardRotateSrv", "�W���ړ������i�W���ُ�p���j", 0);
				} else {
					/* �G���[�����_�ł��Ȃ� */
					DebugPrint( "BoardRotateSrv", "�W���ړ�����", 0);
					cont_led_status[LED_BOARD] = LED_STATUS_OFF;
				}
			} else {
				/* �G���[�L��_�� */
				//led_status[0] = LED_STATUS_TOGGLE;
				for( i = 0; i < display_board_count; i++) {
					output_data[i] = 0;
					BoardWrite(i, 0);/* ���[�^���~���鏈�� */
				}
				DebugPrint( "BoardRotateSrv", "�W���ُ탊�N�G�X�g �G���[�L��_��", 0);
				if (param.no_fail_flag == 1) {/* ���̏ꍇ�̓f�o�b�O�p�Ƀt�F�C���ɂ͂��Ȃ� */
					/* �t�F�C�������ǃt�F�C���ɂ��Ȃ� */
					DebugPrint("BoardRotateSrv","�W���ُ�t�F�C�������ǃt�F�C���ɂ��Ȃ�", 0);
				} else {
					EventRequest(FAIL_REQUEST);/* �\���ُ탊�N�G�X�g */
				}
				if (err_recover_flg == 0 ) {
					cont_led_status[LED_BOARD] = LED_STATUS_TOGGLE;
					keep_board_status_flg = 1;/* kaji20170307 */
				}
			}
			now_status.jimaku_ido_flag = 0;/* �}�������ړ������������� */
			SetResponseStatus(response);/* IO�X�e�[�^�X���Z�b�g */
			cont_led_status[LED_ZUGARA] = LED_STATUS_OFF;
			SetNowLEDStatus(now_status.status);/* �X�e�[�^�XLED��_����Ԃɂ��鏈�� */
			board_rotate_stage_no = BOARD_ROTATE_IDLE;
		break;
	case BOARD_ROTATE_OFFSET_TIMER:
			if (offset_time_up_value == 0) {
				/* ����͔O�̂��߁A���ʂ͒ʂ�Ȃ� */
				offset_time_up_value = now_status.offset_timer;
			}
			if (first_p22 == 1) {
				/* �ŏ��̃I�t�Z�b�g�^�C�}�͓���,10�b�ɂ��� */
				//20170224 offset_time_up_value = 10;
				/* ���ǃI�t�Z�b�g�^�C�}�l�ɖ߂��� */
				offset_time_up_value = param.offset_timer;
			}
			DebugPrint( "BoardRotateSrv", "�I�t�Z�b�g�^�C�}�N��", 4);
			board_rotate_stage_no = BOARD_ROTATE_OFFSET_TIMER_END;
		break;
	case BOARD_ROTATE_OFFSET_TIMER_END:
			if (offset_time_up_value == 0) {
				DebugPrint( "BoardRotateSrv", "�I�t�Z�b�g�^�C�}���Ԍo�߂���", 4);
				/* ����@B�ɑ��M����ꍇ�͂����L���ɂ��邪�A���͖��g�p */
				//if ((now_status.mode == MODE_REMOTE) && (my_tanmatsu_type == CONTA_ADDRESS)) {
				//	if (peek(IO_QUEUE, 0) == PX_P1) {
				//		remote_command_request = 1;/* ���u���̕\���ύX�v������t���O */
				//		remote_command_command = 2;/* ���u���̕\���ʏ�ւ̕ύX�R�}���h */
				//		printf("����@B�ւ̒ʏ�ւ̕ύX�v��\n");
				//	} else if (peek(IO_QUEUE, 0) == PX_P3) {
				//		remote_command_request = 1;/* ���u���̕\���ύX�v������t���O */
				//		remote_command_command = 4;/* ���u���̕\���ψڂւ̕ύX�R�}���h */
				//		printf("����@B�ւ̕ψڂւ̕ύX�v��\n");
				//	}
				//}
				board_rotate_stage_no = BOARD_ROTATE_IDLE;
				if (first_p22 == 1) {
					/* �����ōŏ��̈ړ��ꏊ�����߂� */
					first_p22 = 0;
					if (now_status.power_outage_flag == 1) {
						/* ��d������ */
						printf("****** ��d������\n");
						EventRequest(FAIL_REQUEST);
					} else if ((now_status.mode == MODE_REMOTE) && (CheckTanmatuError() == 0)){
						st = GetNowRemoteStatus();
						if (st == STATUS_P1) {
							IO_ChangeReq(PX_P1);/* �ʏ�ɑJ�� */
						} else if (st == STATUS_P1P2) {
							IO_ChangeReq(PX_P1);/* �ʏ�ɑJ�� */
						} else if (st == STATUS_P3P2) {
							IO_ChangeReq(PX_P1);/* �ʏ�ɑJ�� */
						} else {
							IO_ChangeReq(PX_P3);/* �ψڂɑJ�� */
						}
					}
				}
			}
		break;
	case BOARD_ROTATE_IDLE:
		break;
	default:
		break;
	}
}

/**
 *	@brief �ϕ\���G���[���菈��
 *
 *	@retval �Ȃ�
 */
static void BoardStatusErrorCheck(void)
{
	//if ((board_rotate_total_count > 0) && (board_rotate_stage_no == BOARD_ROTATE_IDLE)) {
		if ((board_rotate_stage_no == BOARD_ROTATE_IDLE) || (board_rotate_stage_no == BOARD_ROTATE_OFFSET_TIMER_END)){
		int match_count = CheckBoardStatus(GetWaitingPattern());/* ����ȉϕ\���̏�Ԃ��l�����鏈�� */
		if (match_count != display_board_count) {
			/* �G���[�L��_�� */
			DebugPrint( "BoardStatusErrorCheck", "board�ُ탊�N�G�X�g �G���[�L��_��", 8);
			if (param.no_fail_flag == 1) {/* ���̏ꍇ�̓f�o�b�O�p�Ƀt�F�C���ɂ͂��Ȃ� */
				/* �t�F�C�������ǃt�F�C���ɂ��Ȃ� */
				DebugPrint("BoardStatusErrorCheck","�t�F�C�������ǃt�F�C���ɂ��Ȃ�", 8);
			} else {
				if (now_status.status != STATUS_FAIL) {
					EventRequest(FAIL_REQUEST);/* �\���ُ탊�N�G�X�g */
				} else {
					DebugPrint("BoardStatusErrorCheck","�t�F�C�������ǂ��łɃt�F�C���ɂȂ��Ă���", 8);
				}
			}
			if (err_recover_flg == 0 ) {
				cont_led_status[LED_BOARD] = LED_STATUS_TOGGLE;
				keep_board_status_flg = 1;/* kaji20170307 */
			}
		} else {
//20170307	if (startup_error_status == 0) {/* kaji20170223 �N�����G���[�ێ�����ĂȂ��ꍇ�̂�LED���� */
			if (keep_board_status_flg == 0) {/* kaji20170307 */
				cont_led_status[LED_BOARD] = LED_STATUS_OFF;/* �W���͐��� */
			}
			if (now_status.byou_status != now_status.before_byou_status) {
				if (now_status.byou_status == 1) {
					EventRequest(FAIL_REQUEST);/* �����e�ُ탊�N�G�X�g */
				}
				now_status.before_byou_status = now_status.byou_status;
			} else {
//20170225				err_recover_flg = 0;/* kaji20170223 */
//				if (startup_error_status == 0) {/* kaji20170223 �N�����G���[�ێ�����ĂȂ��ꍇ�̂�LED���� */
				if (keep_byou_status_flg == 0) {/* kaji20170223 */
					cont_led_status[LED_BYOU] = LED_STATUS_OFF;/* �����e�͐��� */
//					printf("�����e�ُ�LED����(BoardStatusErrorCheck2)\n");
				}
			}
		}
		SetResponseStatus(response);/* IO�X�e�[�^�X���Z�b�g */
	}
	
	
}

	
/**
 *	@brief �ړ����ׂ��p�^�[�������߂鏈��
 *
 *	@retval �ړ����ׂ��p�^�[��
 */
int GetWaitingPattern(void)
{
	int pattern = P1;
	switch (now_status.status) {
	case STATUS_P1:
		pattern = P1;
		break;
	case STATUS_P2:
		pattern = P2;
		break;
	case STATUS_P3:
		pattern = P3;
		break;
	case STATUS_FAIL:
		pattern = param.fail_pattern;
		break;
	}
	return pattern;
}

/**
 *	@brief ����ȉϕ\���̏�Ԃ��l�����鏈��
 *
 *	@retval ����ȕ\����
 */
static int CheckBoardStatus(int dst)
{
	int i;
	int match_count = 0;
	for( i = 0; i < display_board_count; i++) {
		/* �܂���ԂƂ��Ă�邳���p�^�[�������߂� */
		int allowed_pattern[2];
		GetAlowedPattern(dst, i, allowed_pattern);/* �ڕW�ʒu�ɑ΂��Ă̋������p�^�[�������߂鏈�� */
		input_data[i] = BoardRead(i);/* ��Board�X�e�[�^�X�ǂݍ��ݏ��� */
		if ((param.no_board_error_flag & (1<<i)) != 0 ) {
			/* �f�o�b�O���[�h �����I�Ɉ�v�Ƃ��� */
			//printf("�f�o�b�O���[�h �����I�Ɉ�v�Ƃ��� %d\n",i);
			input_data[i] = allowed_pattern[0];
		}
		if (input_data[i] == allowed_pattern[0]) {
			match_count++;
			/* ��x�ł��ُ�ɂȂ�����߂��Ȃ��̂Ő���ɂ͂��Ȃ� */
			/* �d�l��������Ȃ����A�Ƃ肠�������A����Ƃ��� */
			board_status[i]=0;
		} else if ((param.same_nomove_flag == 1) && (input_data[i] == allowed_pattern[1])){ /* �����ꍇ�͉ϕ\���ł�ω������Ȃ����[�h */
			match_count++;/* �p�^�[����v */
			/* ��x�ł��ُ�ɂȂ�����߂��Ȃ��̂Ő���ɂ͂��Ȃ� */
			/* �d�l��������Ȃ����A�Ƃ肠�������A����Ƃ��� */
			board_status[i]=0;
		} else {
			board_status[i]=1;/* �ُ� */
			if (now_status.cds_status == 1) {
				ChoukouWrite(1);/* ���������o�͏����i�u���������̂��߂̏����j */
			} else {
				ChoukouWrite(0);/* ���������o�͏��� (�u���������̂��߂̏���) */
			}
		}
		//printf("board_status[%d]=%d\n",i,board_status[i]);
	}
	return match_count;
}

/**
 *	@brief IO���̓f�[�^���菈��
 *
 *	@param [int dst]  �ݒ肷�ׂ��p�^�[��(P1,P2,P3)
 *
 *	@retval �Ȃ�
 */
static int CheckInput(int dst, int display_check_count, int out_put_req)
{
	int d = 0;
	int match_count = 0;
	int i;
	char str[128];
	char str2[128];
	for( i = 0; i < display_check_count; i++) {
		/* �܂���ԂƂ��Ă�邳���p�^�[�������߂� */
		int allowed_pattern[2];
		GetAlowedPattern(dst, i, allowed_pattern);/* �ڕW�ʒu�ɑ΂��Ă̋������p�^�[�������߂鏈�� */
		input_data[i] = BoardRead(i);/* ��Board�X�e�[�^�X�ǂݍ��ݏ��� */
		if ((param.no_board_error_flag & (1<<i)) != 0 ) {
			/* �f�o�b�O���[�h �����I�Ɉ�v�Ƃ��� */
			//printf("�f�o�b�O���[�h �����I�Ɉ�v�Ƃ��� %d\n",i);
			/* �f�o�b�O���[�h �����I�Ɉ�v�Ƃ��� */
			input_data[i] = allowed_pattern[0];
		}
		if (input_data[i] == allowed_pattern[0]) {
			match_count++;/* �p�^�[����v */
		} else if ((param.same_nomove_flag == 1) && (input_data[i] == allowed_pattern[1])){ /* �����ꍇ�͉ϕ\���ł�ω������Ȃ����[�h */
			match_count++;/* �p�^�[����v */
		}
	}
	if (match_count == display_check_count) {
		/* �S�Ă̕\�����p�^�[����v */
		d = dst;
	}
	if (bef_match_count != match_count) {
		sprintf(str, "CheckInput input_data %d,%d= ",bef_match_count,match_count);
		for(i=0;i < display_check_count;i++) {
			sprintf(str2, "%02X ",input_data[i]);
			strcat(str, str2);
		}
		DebugPrint("", str, 1);
		Output(dst, display_check_count);/* ���͂��ω������̂ŏo�͂��� */
	} else if (out_put_req == 1){
		Output(dst, display_check_count);/* ���͂��ω������̂ŏo�͂��� */
	}
	bef_match_count = match_count;
	return d;
}
	
	
/**
 *	@brief �ڕW�ʒu�ɑ΂��Ă̋������p�^�[�������߂鏈��
 *
 *	@param [int dst]  �ݒ肷�ׂ��p�^�[��(P1,P2,P3)
 *	@param [int no]   �\���ԍ�
 *	@param [int allowed_pattern]  �������p�^�[��(P1,P2,P3)�i�[�|�C���^
 *
 *	@retval �Ȃ�
 */
static void  GetAlowedPattern(int dst, int no, int *allowed_pattern)
{
	if ( dst == P1) {
		allowed_pattern[0] = my_io_info.allowed_pattern_p1[no][0];
		allowed_pattern[1] = my_io_info.allowed_pattern_p1[no][1];
	} else if ( dst == P2) {
		allowed_pattern[0] = my_io_info.allowed_pattern_p2[no][0];
		allowed_pattern[1] = my_io_info.allowed_pattern_p2[no][1];
	} else {
		/* dst == P3 */
		allowed_pattern[0] = my_io_info.allowed_pattern_p3[no][0];
		allowed_pattern[1] = my_io_info.allowed_pattern_p3[no][1];
	}
}

/**
 *	@brief IO�o�͏���
 *
 *	@param [int dst]  �ݒ肷�ׂ��p�^�[��(P1,P2,P3)
 *
 *	@retval �Ȃ�
 */
static void Output(int dst, int board_count)
{
	int i;
	char str[128];
	char str2[128];
	for( i = 0; i < board_count; i++) {
		/* �܂���ԂƂ��Ă�邳���p�^�[�������߂� */
		int allowed_pattern[2];
		GetAlowedPattern(dst, i, allowed_pattern);/* �ڕW�ʒu�ɑ΂��Ă̋������p�^�[�������߂鏈�� */
		if (input_data[i] == allowed_pattern[0]) {
			output_data[i] = 0;/* �p�^�[����v�Ȃ�Β�~���� */
		} else if ((param.same_nomove_flag == 1) && (input_data[i] == allowed_pattern[1])){ /* �����ꍇ�͉ϕ\���ł�ω������Ȃ����[�h */
			output_data[i] = 0;/* �p�^�[����v�Ȃ�Β�~���� */
		} else {
			//yamazaki output_data[i] = 1;/* �p�^�[������v���Ă��Ȃ��̂Ń��[�^�𓮂��� */
			output_data[i] = 2;/* �p�^�[������v���Ă��Ȃ��̂Ń��[�^�𓮂��� */
		}
		BoardWrite(i, output_data[i]);/* Board�ւ̏o�͏��� */
	}
	sprintf(str, "Output output_data = ");
	for(i=0;i < display_board_count;i++) {
		sprintf(str2, "%02X ",output_data[i]);
		strcat(str, str2);
	}
	DebugPrint("", str, 4);
}
	
	
/**
 *	@brief �\����,�e����ω������鏈���v�����L���[�ɂ����
 *
 *	@retval �Ȃ�
 */
void IO_ChangeReq(char change_type)
{
	char str[128];
	int q_trash;
	int loop_flag;
	
	sprintf(str, "IO_ChangeReq %s��v��", request_pattern[(int)change_type]);
	DebugPrint("", str, 0);
// kaji20170308��
	if (change_type == PX_FAIL) {
		
		int loop_flag = 1;
		while ((!empty(IO_QUEUE)) && (loop_flag == 1)) {
			switch (peek(IO_QUEUE, 0)) {
				case PX_P1:
				case PX_P2:
				case PX_P3:
				case P2_P2:
					q_trash = dequeue(IO_QUEUE);//��ς݂��ꂽIO�ύX�v���f�[�^��j��
					sprintf(str, " -> PX_FAIL�v���L��̂��߁AIO_ChangeReq %s��j��", request_pattern[(int)q_trash]);
					DebugPrint("", str, 0);
					break;
				default:
					loop_flag = 0;
					break;
			}
		}
	}
// kaji20170308��
	enqueue(IO_QUEUE, change_type);
	return;
}

/**
 *	@brief �\����,�e����ω������鏈��
 *         �����ŊĎ������̃X�e�[�^�X���Z�b�g����
 *
 *	@retval �Ȃ�
 */
static void IO_ChangeSrv(void )
{
	int change_type;
	char str[128];
	if (board_rotate_stage_no != BOARD_ROTATE_IDLE){
		//printf("IO_ChangeSrv board_rotate_stage_no �� BOARD_ROTATE_IDLE�ł͂Ȃ�\n");
		return;
	}
	if (!empty(IO_QUEUE)) {
		change_type = dequeue(IO_QUEUE);//IO�ύX�v���f�[�^���o��
		/* 20170226 �I�t�Z�b�g�^�C�}���ɃG���[�ɂȂ����ꍇ�̏��� */
		if (change_type == PX_P3) {
			if ((my_tanmatsu_type == CONTA_ADDRESS) && (now_status.mode == MODE_REMOTE) && (CheckTanmatuError() != 0)){
				change_type = PX_FAIL;
				sprintf(str, "�G���[��Ԃ̂��ߕψڂ���t�F�C���ɕύX\n");
				DebugPrint("", str, 0);
			}
		}
		else if (change_type == PX_P1) {
			if ((my_tanmatsu_type == CONTA_ADDRESS) && (now_status.mode == MODE_REMOTE) && (CheckTanmatuError() != 0)){
				sprintf(str, "�G���[��Ԃ̂��ߒʏ�ւ̈ړ�����߂�\n");
				DebugPrint("", str, 0);
				return;
			}
		}
		sprintf(str, "IO_ChangeSrv %s", request_pattern[change_type]);
		DebugPrint("", str, 0);

		switch (change_type) {
		case PX_P2:
			board_rotate_stage_no = BOARD_ROTATE_READY;
			board_rotate_delay_time = 0;/* ���J�n */
			ByouWrite(STATUS_P2);/* �����e����|��ԂɃZ�b�g */
			if (my_tanmatsu_type == 4) {/* ������̂� */
				NaishouWrite(STATUS_P2);/* �����e����|��ԂɃZ�b�g */
			}
			SetNowStatus(STATUS_P2);/* ��| */;
			SetResponseStatus(response);/* IO�X�e�[�^�X���Z�b�g */
			
			if (lenqueue(IO_QUEUE) >= 1) {
				if (peek(IO_QUEUE, 0) == P2_P2) {
					/* �擪�f�[�^��P2_P2���H */
					/* ��|�ړ��̊J�n���Ƀ^�C�}�[���Z�b�g���Ă��� */
					offset_time_up_value = now_status.offset_timer;
				}
			}
			
			break;
		case PX_P3:
			board_rotate_stage_no = BOARD_ROTATE_READY;
			board_rotate_delay_time = 0;/* ���J�n */
			ByouWrite(STATUS_P3);/* �����e��ψڏ�ԂɃZ�b�g */
			if (my_tanmatsu_type == 4) {/* ������̂� */
				NaishouWrite(STATUS_P3);/* ���Ɣ�ψڏ�ԂɃZ�b�g */
			}
			SetNowStatus(STATUS_P3);/* �ψڂɐݒ� */;
			SetResponseStatus(response);/* IO�X�e�[�^�X���Z�b�g */
			break;
		case PX_P1:
			board_rotate_stage_no = BOARD_ROTATE_READY;
			board_rotate_delay_time = 0;/* ���J�n */
			ByouWrite(STATUS_P1);/* �����e��ʏ��ԂɃZ�b�g */
			if (my_tanmatsu_type == 4) {/* ������̂� */
				NaishouWrite(STATUS_P1);/* ���Ɣ�ʏ��ԂɃZ�b�g */
			}
			SetNowStatus(STATUS_P1);/* �ʏ�ݒ� */;
			SetResponseStatus(response);/* IO�X�e�[�^�X���Z�b�g */
			break;
		case PX_FAIL:
			board_rotate_stage_no = BOARD_ROTATE_READY;
			board_rotate_delay_time = 0;/* ���J�n */
			ByouWrite(STATUS_FAIL);/* �����e���t�F�C����ԂɃZ�b�g */
			if (my_tanmatsu_type == 4) {/* ������̂� */
				NaishouWrite(STATUS_FAIL);/* ���Ɣ��t�F�C����ԂɃZ�b�g */
			}
			SetNowStatus(STATUS_FAIL);/* �t�F�C���ɐݒ� */;
			SetResponseStatus(response);/* IO�X�e�[�^�X���Z�b�g */
			break;
		case P2_P2:
			board_rotate_stage_no = BOARD_ROTATE_OFFSET_TIMER;/* �I�t�Z�b�g�^�C�}���ԑ҂� */
			break;
		case PX_POWEROFF:
			board_rotate_stage_no = BOARD_ROTATE_READY;
			board_rotate_delay_time = 1000;/* ��b���J�n��x�点�� */
			power_off_bef_status = now_status.status;/* ���݂̃X�e�[�^�X���Z�[�u */
			SetNowStatus(STATUS_FAIL);/* �t�F�C���ɐݒ� */;
			break;
		case PX_POWER_RECOVER:
			board_rotate_stage_no = BOARD_ROTATE_READY;
			board_rotate_delay_time = 0;/* ���J�n */
			SetNowStatus(power_off_bef_status);/* ���݂̃X�e�[�^�X�ɐݒ�����߂� */;
			break;
		default:
			/* �����ɗ��Ă͂����Ȃ� */
			break;
		}
	}
}

/**
 *	@brief ��d���o���̏���
 *
 *	@retval �Ȃ�
 */
static void PowerOutMainSrv(void)
{
	char str[128];
	
	io_power_outage_flag = PowerStatusRead();/* ��d�X�e�[�^�X�ǂݍ��ݏ��� */
	if ((bef_io_power_outage_flag == 0) && (io_power_outage_flag == 1)) {
		sprintf(str, "��d���� in PowerOutMainSrv");
		DebugPrint("", str, 0);
		/* ��d���� */
		now_status.power_outage_flag = 1;
		if (teiden_hukki_flg == 1) {
			now_status.power_outage_flag2 = 1;/* ��d���A�N�����̋N���̌� */
		} else {
			now_status.power_outage_flag2 = 0;/* ��d���A�N�����̋N���̌�ł͂Ȃ� */
		}
		cont_led_status[LED_TEIDEN] = LED_STATUS_ON;/* �N�����͒�d�\���� */
		ChoukouWrite(0);/* ���������؏o�͏��� */
		ToukaWrite(my_tanmatsu_type);//20170131 yamazaki
//20170226		EventRequest(POWER_OFF_REQUEST);/* ��d�������N�G�X�g */
//20170226		SaveStatus();/* �X�e�[�^�X��ۑ� */
	} if ((bef_io_power_outage_flag == 1) && (io_power_outage_flag == 0)) {
		sprintf(str, "��d���A in PowerOutMainSrv");
		DebugPrint("", str, 0);
		/* ��d�������A */
		now_status.power_outage_flag = 0;
		EventRequest(POWER_RECOVER_REQUEST);/* ��d�������A���N�G�X�g */
//20170226		SaveStatus();/* �X�e�[�^�X��ۑ� */
	}
	bef_io_power_outage_flag = io_power_outage_flag;
	return;
}

/**
 *	@brief ���݂̉��u���̃X�e�[�^�X��Ԃ�����
 *
 *	@retval ���݂̉��u���̃X�e�[�^�X
 */
int GetNowRemoteStatus(void)
{
	if ((now_status.start_time == 0) && (now_status.end_time == 0)) {
		/* ���Ԃ��ݒ肳��Ă��Ȃ��ꍇ�͉��u���͒ʏ�Ƃ��� */
		return STATUS_P1;/* �ʏ� */
	}
	if (offset_time_up_value != 0) {
		return STATUS_P2;/* ��| */
	}
	//yamazaki int now_time = GetTodaysMinute();/* ���݂̎����擾*/
	int now_time = GetTodaysSecond();/* ���݂̎����擾*/
	/* start_time�Ɠ����͕ψ� */
	/* end_time�Ɠ����͒ʏ� */
	//if (now_time < now_status.start_time) {
		if (now_time < (now_status.start_time - now_status.offset_timer)) {//yamazaki
		return STATUS_P1;/* �ʏ� */
	}else if (now_time < now_status.start_time) {
		return STATUS_P1P2;/* �ʏ킩��̕ω����̈�| */
	} else if (now_time < now_status.end_time) {
		return STATUS_P3;/* �ψ� */
	} else if (now_time < (now_status.end_time + now_status.offset_timer)) {//yamazaki
		return STATUS_P3P2;/* �ψڂ���̕ω����̈�| */
	} else {
		return STATUS_P1;/* �ʏ� */
	}
		
		
		
}

/**
 *	@brief ����w�ߎ�M����
 *
 *	@retval �Ȃ�
 */
void BroadcastCommand(BROADCAST_COMMAND *com) {

	char str[256];
	char str2[256];

	DebugPrint("","BroadcastCommand ���񐧌�w�ߎ�M", 1);
	strcpy(str,"");
#if 1
	
	int tanmatu_no = (com->status.byte >> 5) + 1;
//	printf("*** %d,%d\n",tanmatu_no , my_tanmatsu_type);
	if ((com->status.byte & 8) != 0){/* �e�X�g�w�� */
		if (tanmatu_no == my_tanmatsu_type) {
			strcat(str,"�e�X�g ");
			/* �e�X�g�ŃX�e�[�^�X�͕ς��Ȃ� */
			now_status.test_flag = 1;/* �e�X�g���t���O */
			cont_led_status[LED_TEST] = LED_STATUS_ON;
		}
	} else if ((com->status.byte & 0x10) != 0) {/* �e�X�g�w�߉��� */
		if (tanmatu_no == my_tanmatsu_type) {
			strcat(str,"�e�X�g���� ");
			now_status.test_flag = 0;/* �e�X�g���t���O */
			cont_led_status[LED_TEST] = LED_STATUS_OFF;
		}
	}
#else
	if (com->command.test != 0) {
		strcat(str,"�e�X�g ");
		/* �e�X�g�ŃX�e�[�^�X�͕ς��Ȃ� */
		now_status.test_flag = 1;/* �e�X�g���t���O */
		cont_led_status[LED_TEST] = LED_STATUS_ON;
	} else {
		now_status.test_flag = 0;/* �e�X�g���t���O */
		cont_led_status[LED_TEST] = LED_STATUS_OFF;
	}
#endif
	if ((com->light_command.sreq == 0) &&
		(com->light_command.time_req == 0) &&
		(com->light_command.rendou_req == 0) && //20170320
		(com->light_command.issei == 0) &&
		//2-170219 ((com->status.byte & 0x18) == 0)){/* �e�X�g */
		(com->status.byte == 0)){/* �e�X�g,GPS��ԕω� */
			/* ���̗v���������ꍇ */
			SetRequest(com, str);/* ���[�h�ݒ�v�����̏��� */
	}

	if (com->light_command.sreq != 0) {
		/* �X�P�W���[���o�^�v����M */
		DebugPrint("", "BroadcastCommand �X�P�W���[���o�^�v����M\n", 1);
		now_status.start_time = 60 * ( 10 * ((com->schedule.start_time[0]>>4) & 0xf) + 
				(com->schedule.start_time[0] & 0xf)) +
				10 * ((com->schedule.start_time[1]>>4) & 0xf) + 
				(com->schedule.start_time[1] & 0xf);
		now_status.start_time *= 60;/* �b�P�ʂɕϊ� *///yamazaki
		now_status.end_time = 60 * ( 10 * ((com->schedule.end_time[0]>>4) & 0xf) + 
				(com->schedule.end_time[0] & 0xf)) +
				10 * ((com->schedule.end_time[1]>>4) & 0xf) + 
				(com->schedule.end_time[1] & 0xf);
		now_status.end_time *= 60;/* �b�P�ʂɕϊ� *///yamazaki
		now_status.offset_timer = 10 * BIN(com->schedule.offset_timer);
		now_status.schedule = 1;/* �^�p�Ǘ��o�b����̃X�P�W���[���o�^�ς� */
		if ( now_status.start_time != 0) {
			if ( (now_status.start_time != param.start_time) ||
				(now_status.end_time != param.end_time) ||
				(now_status.offset_timer != param.offset_timer) ) {
				param.start_time = now_status.start_time;
				param.end_time = now_status.end_time;
				param.offset_timer = now_status.offset_timer;
				DebugPrint("", "�p�����[�^���ς�������߃p�����[�^�Z�[�u\n", 0);
				SaveParam();
			}
		}
	}
	if (com->light_command.time_req != 0) {
		/* �����C���v����M */
		SetTime(&(com->t));/* �����ݒ菈�� */
		SaveRTC(&(com->t));/* �s�����pRTC�ɏ������� */
		now_status.time_req = 1;/* �^�p�Ǘ��o�b����̎����C���ς� */
	}
	if (com->light_command.issei != 0) {
		/* ������Ďw�ߗL��M */
		if (com->light_command.choukou_iri != 0) {
			/* �����������i��j */
			ChoukouWrite(1);/* �����������o�͏��� */
			board_choukou_status = 1;/* kaji20170330 */
			sprintf(str, "������Ďw�ߗL  �����������i��j��M ");
		} else if (com->light_command.choukou_kiri != 0) {
			/* ���������؁i���j */
			ChoukouWrite(0);/* ���������؏o�͏��� */
			board_choukou_status = 0;/* kaji20170330 */
			sprintf(str, "������Ďw�ߗL  ���������؁i���j��M ");
		} else {
			sprintf(str, "������Ďw�ߗL  ?????");
		}
		DebugPrint("", str, 1);
	}
	
	if ((com->status.byte & 2) != 0) {
		/* GPS�ُ� */
		now_status.gps_status = 1;
	} else if ((com->status.byte & 4) != 0) {
		now_status.gps_status = 0;
	}
	if (now_status.gps_status != now_status.before_gps_status) {
		/* GPS�ُ�̏�Ԃ��ω����� */
		//if (now_status.gps_status == 1) {
		//	/* ���v�ُ� */
		///	EventRequest(FAIL_REQUEST);/* ���v�ُ�͕\���ُ�Ƃ��郊�N�G�X�g */
		//} else {
		//	/* ���̏ꍇ�͎��v�ُ킩�畜�A */
		//	/* ���������Ď��v�ُ킩�畜�A�����̂�������Ȃ��̂Ń`�F�b�N���� */
		//	if ((now_status.status == STATUS_FAIL) && (CheckErrorStatus() == 0)) {
		//		EventRequest(FAIL_RECOVER_REQUEST);/* ���v�ُ한�A���N�G�X�g */
		//	}
		//}
	}
	now_status.before_gps_status = now_status.gps_status;
	
	sprintf(str2, "�^�p�Ǘ��o�b�����M %s %02X %02X %02X st=%d:%d,ed=%d:%d,off=%d"
														,str
														,com->command.byte
														,com->light_command.byte
														,com->status.byte
														,(now_status.start_time/60)/60
														,(now_status.start_time/60)%60
														,(now_status.end_time/60)/60
														,(now_status.end_time/60)%60
														,now_status.offset_timer );

	DebugPrint("", str2, 1);
}

/**
 *	@brief ���[�h�ݒ�v�����̏���
 *
 *	@retval �Ȃ�
 */
static void SetRequest(BROADCAST_COMMAND *com, char *str)
{
// ------------------------------------- ���O��
#ifdef kaji20170302
	if (com->command.shudou != 0) {
		now_status.manual_status = 1;/* �蓮��� 0:SW�ɂ��蓮,1:�^�pPC����̎蓮�ɂ�� */
		strcat(str,"�蓮 ");
		if (com->command.tuujou != 0) {
			EventRequest(MANUAL_TUUJOU_REQUEST);/* �蓮�ʏ탊�N�G�X�g */
		} else if (com->command.issou != 0) {
			EventRequest(MANUAL_ISSOU_REQUEST);/* �蓮��|���N�G�X�g */
		} else if (com->command.henni != 0) {
			EventRequest(MANUAL_HENNI_REQUEST);/* �蓮�ψڃ��N�G�X�g */
		} else if (com->command.fail != 0) {
			EventRequest(MANUAL_FAIL_REQUEST);/* �蓮�t�F�C�����N�G�X�g */
		}
	} else if (com->command.teishi != 0) {
		strcat(str,"�^�p��~ ");
		EventRequest(MONITOR_REQUEST);/* �^�p��~���N�G�X�g */
	} else if (com->command.yobi2 != 0) {
		strcat(str,"�^�p��~���� ");
		EventRequest(MONITOR_RELEASE_REQUEST);/* �^�p��~�������N�G�X�g */
	} else {
#ifdef	yamazaki20170209
		strcat(str,"���u ");
		if (now_status.mode == MODE_MANUAL) {
			EventRequest(REMOTE_REQUEST);/* ���u���N�G�X�g */
		}
#else
		if (now_status.mode == MODE_MANUAL) {
			if (now_status.manual_status == 0 ) {
				/* �蓮��� 0:SW�ɂ��蓮,1:�^�pPC����̎蓮�ɂ�� */
				strcat(str,"���u(�d�l�ɂ�苑��) ");	/* ���u���N�G�X�g (����) yamazaki 20170209 */
				return;
			} else {
				strcat(str,"�^�pPC����̎蓮�ݒ��Ȃ̂ŉ��u�v��������");	/* ���u���N�G�X�g yamazaki 20170217 */
				EventRequest(REMOTE_REQUEST);/* ���u���N�G�X�g */
			}
		}
#endif
		if (com->command.tuujou != 0) {
			strcat(str,"�ʏ� ");
			if (my_tanmatsu_type == CONTA_ADDRESS) {
				printf("***�ʏ탊�N�G�X�g �L�����Z��\n");
				//20170210 EventRequest(FAIL2TUUJOU_REQUEST);/* �t�F�C�����̒ʏ�ւ̕��A���N�G�X�g */
			} else {
				printf("***����@B�̒ʏ탊�N�G�X�g �L�����Z��\n");
				//20170210 EventRequest(TUUJOU_REQUEST);/* ����@B�̒ʏ탊�N�G�X�g */
			}
		} else if (com->command.issou != 0) {
			strcat(str,"��| ");
			if (my_tanmatsu_type != CONTA_ADDRESS) {
				printf("***����@B�̈�|���N�G�X�g\n");
				//20170226 20170210 EventRequest(ISSOU_REQUEST);/* ����@B�̈�|���N�G�X�g */
				EventRequest(ISSOU_REQUEST);/* ����@B�̈�|���N�G�X�g */
			}
		} else if (com->command.henni != 0) {
			strcat(str,"�ψ� ");
			if (my_tanmatsu_type != CONTA_ADDRESS) {
				printf("***����@B�̕ψڃ��N�G�X�g�L�����Z��\n");
				//20170210 EventRequest(HENNI_REQUEST);/* ����@B�̕ψڃ��N�G�X�g */
			}
		} else if (com->command.fail != 0) {
			strcat(str,"�t�F�C�� ");
			EventRequest(FAIL_REQUEST);/* ���u�t�F�C�����N�G�X�g */
		} else {
			if (now_status.status != STATUS_FAIL) {//20170218 �t�F�C���ł͂Ȃ��ꍇ�̂�
				/* ���u�ł����̃��N�G�X�g������ 20170212 */
				EventRequest(REMOTE_REQUEST);/* ���u���N�G�X�g */
			}
		}
	}
#else
// ----------------------------------------- out ��


	if (com->command.shudou != 0) {
		if ((now_status.mode == MODE_MANUAL) && (now_status.manual_status == 0)) {
				/* �蓮��� 0:SW�ɂ��蓮,1:�^�pPC����̎蓮�ɂ�� */
			strcat(str,"�蓮(�d�l�ɂ�苑��) ");	/* �蓮���N�G�X�g (����) kaji20170209 */
			return;
		} else {
			now_status.manual_status = 1;/* �蓮��� 0:SW�ɂ��蓮,1:�^�pPC����̎蓮�ɂ�� */
			strcat(str,"�蓮 ");
			if (com->command.tuujou != 0) {
				EventRequest(MANUAL_TUUJOU_REQUEST);/* �蓮�ʏ탊�N�G�X�g */
			} else if (com->command.issou != 0) {
				EventRequest(MANUAL_ISSOU_REQUEST);/* �蓮��|���N�G�X�g */
			} else if (com->command.henni != 0) {
				EventRequest(MANUAL_HENNI_REQUEST);/* �蓮�ψڃ��N�G�X�g */
			} else if (com->command.fail != 0) {
				EventRequest(MANUAL_FAIL_REQUEST);/* �蓮�t�F�C�����N�G�X�g */
			}
		}
	} else if (com->command.teishi != 0) {
		strcat(str,"�^�p��~ ");
		EventRequest(MONITOR_REQUEST);/* �^�p��~���N�G�X�g */
	} else if (com->command.yobi2 != 0) {
		strcat(str,"�^�p��~���� ");
		EventRequest(MONITOR_RELEASE_REQUEST);/* �^�p��~�������N�G�X�g */
	} else {
		if (now_status.mode == MODE_MANUAL) {
			if (now_status.manual_status == 0 ) {
				/* �蓮��� 0:SW�ɂ��蓮,1:�^�pPC����̎蓮�ɂ�� */
				strcat(str,"���u(�d�l�ɂ�苑��) ");	/* ���u���N�G�X�g (����) yamazaki 20170209 */
				return;
			} else {
				strcat(str,"�^�pPC����̎蓮�ݒ��Ȃ̂ŉ��u�v��������");	/* ���u���N�G�X�g yamazaki 20170217 */
				EventRequest(REMOTE_REQUEST);/* ���u���N�G�X�g */
			}
		}
		if (com->command.tuujou != 0) {
			strcat(str,"�ʏ� ");
			if (my_tanmatsu_type == CONTA_ADDRESS) {
				printf("***�ʏ탊�N�G�X�g �L�����Z��\n");
				//20170210 EventRequest(FAIL2TUUJOU_REQUEST);/* �t�F�C�����̒ʏ�ւ̕��A���N�G�X�g */
			} else {
				printf("***����@B�̒ʏ탊�N�G�X�g �L�����Z��\n");
				//20170210 EventRequest(TUUJOU_REQUEST);/* ����@B�̒ʏ탊�N�G�X�g */
			}
		} else if (com->command.issou != 0) {
			strcat(str,"��| ");
			if (my_tanmatsu_type != CONTA_ADDRESS) {
				printf("***����@B�̈�|���N�G�X�g\n");
				//20170226 20170210 EventRequest(ISSOU_REQUEST);/* ����@B�̈�|���N�G�X�g */
				EventRequest(ISSOU_REQUEST);/* ����@B�̈�|���N�G�X�g */
			}
		} else if (com->command.henni != 0) {
			strcat(str,"�ψ� ");
			if (my_tanmatsu_type != CONTA_ADDRESS) {
				printf("***����@B�̕ψڃ��N�G�X�g�L�����Z��\n");
				//20170210 EventRequest(HENNI_REQUEST);/* ����@B�̕ψڃ��N�G�X�g */
			}
		} else if (com->command.fail != 0) {
			strcat(str,"�t�F�C�� ");
			EventRequest(FAIL_REQUEST);/* ���u�t�F�C�����N�G�X�g */
		} else {
			if (now_status.status != STATUS_FAIL) {//20170218 �t�F�C���ł͂Ȃ��ꍇ�̂�
				/* ���u�ł����̃��N�G�X�g������ 20170212 */
				EventRequest(REMOTE_REQUEST);/* ���u���N�G�X�g */
			}
		}
	}

	
#endif
	SaveStatus();/* �X�e�[�^�X��ۑ� */
}

/**
 *	@brief �X�e�[�^�X�̃Z�[�u����
 *         now_status.status�ύX���͕s�����������ɃZ�[�u����
 *         LED�̏�Ԃ�؂�ւ���
 *
 *	@retval �Ȃ�
 */
void SetNowStatus(int status)
{
	if (now_status.status != status) {
		if (status == STATUS_FAIL) {
			/* �ύX�v����FAIL�̏ꍇ�͂��̑O�̏�Ԃ�ێ����Ă��� */
			now_status.before_fail_status = now_status.status;
		}
		now_status.status = status;
		SetNowLEDStatus(status);/* �X�e�[�^�X��LED�ɔ��f���鏈�� */
		SaveStatus();
	}
}
/**
 *	@brief �X�e�[�^�X��LED�ɔ��f���鏈��
 *
 *	@retval �Ȃ�
 */
static void SetNowLEDStatus(int status)
{
		switch (status) {
		case STATUS_P1:
			cont_led_status[LED_TUUJOU] = LED_STATUS_ON;
			cont_led_status[LED_ISSOU] = LED_STATUS_OFF;
			cont_led_status[LED_HENNI] = LED_STATUS_OFF;
			break;
		case STATUS_P2:
			cont_led_status[LED_TUUJOU] = LED_STATUS_OFF;
			cont_led_status[LED_ISSOU] = LED_STATUS_ON;
			cont_led_status[LED_HENNI] = LED_STATUS_OFF;
			break;
		case STATUS_P3:
			cont_led_status[LED_TUUJOU] = LED_STATUS_OFF;
			cont_led_status[LED_ISSOU] = LED_STATUS_OFF;
			cont_led_status[LED_HENNI] = LED_STATUS_ON;
			break;
		case STATUS_FAIL:
			cont_led_status[LED_TUUJOU] = LED_STATUS_OFF;
			cont_led_status[LED_ISSOU] = LED_STATUS_ON;	// yamazaki 20170209
			cont_led_status[LED_HENNI] = LED_STATUS_OFF;
			break;
		};
}

/**
 *	@brief �X�e�[�^�X��LED�ɔ��f���鏈��
 *         LED��_�ŏ�Ԃɂ���
 *
 *	@retval �Ȃ�
 */
static void SetNowLEDStatusToggle(int status)
{
		switch (status) {
		case STATUS_P1:
			cont_led_status[LED_TUUJOU] = LED_STATUS_TOGGLE;
			cont_led_status[LED_ISSOU] = LED_STATUS_OFF;
			cont_led_status[LED_HENNI] = LED_STATUS_OFF;
			break;
		case STATUS_P2:
			cont_led_status[LED_TUUJOU] = LED_STATUS_OFF;
			cont_led_status[LED_ISSOU] = LED_STATUS_TOGGLE;
			cont_led_status[LED_HENNI] = LED_STATUS_OFF;
			break;
		case STATUS_P3:
			cont_led_status[LED_TUUJOU] = LED_STATUS_OFF;
			cont_led_status[LED_ISSOU] = LED_STATUS_OFF;
			cont_led_status[LED_HENNI] = LED_STATUS_TOGGLE;
			break;
		case STATUS_FAIL:
			cont_led_status[LED_TUUJOU] = LED_STATUS_OFF;
#ifdef kajikaji20170208
			cont_led_status[LED_ISSOU] = LED_STATUS_OFF;
#else
			cont_led_status[LED_ISSOU] = LED_STATUS_TOGGLE;
#endif
			cont_led_status[LED_HENNI] = LED_STATUS_OFF;
			break;
		};
}
/**
 *	@brief �X�e�[�^�X�̃��[�h����
 *
 *	@retval �Ȃ�
 */
#define STATUS_FILE_NAME ("status.ini")
#define PARAM_FILE_NAME ("param.ini")
int LoadStatus(void)
{
#ifdef windows	
	FILE *fp;
	char fname[256];

	sprintf(fname,"%s",STATUS_FILE_NAME);
	fp = fopen(fname, "rb");
    if (fp == NULL) {
		printf("%s:%s is not opened!\n",__func__,fname);
		return -1;
    }
	STATUS tmp_status;
	STATUS *p = &tmp_status;
	fread(p, sizeof (STATUS), 1, fp);
	now_status.mode = p->mode;
	now_status.status = p->status;
	now_status.before_fail_status = p->before_fail_status;
	now_status.power_outage_flag = p->power_outage_flag;
	now_status.power_outage_flag2 = p->power_outage_flag2;
	fclose(fp);
#else
	/* ������FlashROM����X�e�[�^�X�����[�h���� */
	STATUS *p = (STATUS *)STATUS_ADDRESS;
	int bcc = CalcBcc((char *)p,sizeof(STATUS) - sizeof(int));
	if (bcc == p->bcc) {
		printf("now_status load success bcc=%X\n",bcc);
		now_status.mode = p->mode;
		now_status.status = p->status;
		now_status.power_outage_flag = p->power_outage_flag;
		now_status.start_time = p->start_time;/* 20170224 �ʏ큨��|�J�n���� */
		now_status.end_time = p->end_time;/* 20170224 �ψځ���|�J�n���� */
		now_status.offset_timer = p->offset_timer;/* 20170224 �I�t�Z�b�g�^�C�}�l */
		
// kaji20170223 �[���G���[resume�����Ɩ󂪂킩��Ȃ��Ȃ��Ă����̂ňꎞ�����
//		startup_error_status = p->tanmatu_error;/* 2017022 �N�����̃G���[�̗L����ێ�(0:�G���[�����A1:�G���[�L��) */
/* kaji20170301 �[���G���[resume�̍l�����~�߁H��
		if (p->musen_status != 0) {
			cont_led_status[LED_MUSEN] = LED_STATUS_TOGGLE;
			printf("�N����LED_MUSEN TOGGLE\n" ) ;// kaji20170223
		}
		if (p->board_error != 0) {
			cont_led_status[LED_BOARD] = LED_STATUS_TOGGLE;
			printf("�N����LED_BOARD TOGGLE\n" ) ;// kaji20170223
		}
		if (p->byou_status != 0) {
			cont_led_status[LED_BYOU] = LED_STATUS_TOGGLE;
			printf("�N����LED_BYOU TOGGLE\n" ) ;// kaji20170223
		}
  kaji20170301 �[���G���[resume�̍l�����~�߁H��*/
	} else {
		printf("************now_status load fail p->bcc=%.08X %.08X\n", p->bcc,bcc ) ;
	}
#endif
	if (now_status.mode == MODE_MONITOR) {
		now_status.mode = MODE_REMOTE;
		printf("LoadStatus�@now_status �^�p���牓�u�ɕύX\n" ) ;
	}
	
	if (now_status.mode == MODE_REMOTE) {
		if (now_status.status == STATUS_P2) {
			SetNowStatus(STATUS_P1);/* �ʏ� */;
		} else if (now_status.status == STATUS_FAIL) {
			SetNowStatus(STATUS_P1);/* �ʏ� */;
		}
	} else if (now_status.mode == MODE_MANUAL) {
		if (now_status.status == STATUS_FAIL) {
			now_status.status = now_status.before_fail_status;
			printf("LoadStatus�@now_status STATUS_FAIL�Ȃ̂�before_fail_status�ɕύX %d\n", now_status.status) ;
		}
		sw = 0;
		sw |= SW_STATUS_BIT;
		if (now_status.status == STATUS_P1) {
			sw |= SW_TUUJOU_BIT;
		} else if (now_status.status == STATUS_P2) {
			sw |= SW_ISSOU_BIT;
		} else {
			sw |= SW_HENNI_BIT;
		}
		bef_sw = sw;
		swtest = sw;
	}
	StatusDisp();/* �X�e�[�^�X�̕\������ */
	return 0;
}

/**
 *	@brief �X�e�[�^�X�̃Z�[�u����
 *
 *	@retval �Ȃ�
 */
void SaveStatus(void)
{
#ifdef windows	

	FILE *fp;
	char fname[256];

	sprintf(fname,"%s",STATUS_FILE_NAME);
	fp = fopen(fname, "wb");
    if (fp == NULL) {
		printf("%s:%s is not opened!\n",__func__,fname);
    }
	fwrite(&now_status, sizeof (STATUS), 1, fp);
	fclose(fp);

#else
	/* ������FlashROM�̃X�e�[�^�X���Z�[�u���� */
	int bcc = CalcBcc((char *)&now_status,sizeof(STATUS) - sizeof(int));
	now_status.bcc = bcc;
#if 1
	flash_sector_erase(STATUS_ADDRESS);
	flash_write_buf(STATUS_ADDRESS, (int)&now_status, sizeof(STATUS)/2);
#else
	extern int flash_modify_APP_Tasks_init ( int *p ,int size);
	extern int flash_modify_APP_Tasks(void);
    int ret;
	ret = flash_modify_APP_Tasks_init((int *)&now_status, sizeof(STATUS));
	if (ret <0) {
		printf("%s:init error!\n","SaveStatus");
		return;
	}
    while(1) {
    	ret = flash_modify_APP_Tasks();
        if (ret == 1) {
            //printf("now_status OK\r\n");
            break;
        } else if (ret ==0) {
            //printf("now_status NG\r\n");
            break;
        }
	}
#endif
#endif
}
/**
 *	@brief �s������������p�����[�^�̃��[�h����
 *
 *	@retval �Ȃ�
 */
void LoadParam(void)
{
#ifdef windows	
	FILE *fp;
	char fname[256];

	sprintf(fname,"%s",PARAM_FILE_NAME);
	fp = fopen(fname, "rb");
    if (fp == NULL) {
		printf("%s:%s is not opened!\n",__func__,fname);
		return;
    }
	PARAM *p = &param;
	fread(p, sizeof (PARAM), 1, fp);
	fclose(fp);
#else
	PARAM *p = (PARAM *)PARAM_ADDRESS;
	int bcc = CalcBcc((char *)p,sizeof(PARAM) - sizeof(int));
	if (bcc == p->bcc) {
		printf("param load success bcc=%X\n",bcc);
		memmove(&param, p , sizeof(PARAM) );
	} else {
		printf("*********************param load fail p->bcc=%.08X %.08X\n", p->bcc,bcc ) ;
	}
#endif
}

/**
 *	@brief �s������������ւ̃p�����[�^�̃Z�[�u����
 *
 *	@retval �Ȃ�
 */
void SaveParam(void)
{
#ifdef windows	

	FILE *fp;
	char fname[256];

	sprintf(fname,"%s",PARAM_FILE_NAME);
	fp = fopen(fname, "wb");
    if (fp == NULL) {
		printf("%s:%s is not opened!\n",__func__,fname);
    }
	fwrite(&param, sizeof (PARAM), 1, fp);
	fclose(fp);

#else
	int bcc = CalcBcc((char *)&param,sizeof(PARAM) - sizeof(int));
	param.bcc = bcc;
#if 1
	flash_sector_erase(PARAM_ADDRESS);
	flash_write_buf(PARAM_ADDRESS, (int)&param, sizeof(PARAM)/2);
#else
	extern int flash_modify_APP_Tasks_init2 ( int *p ,int size);
	extern int flash_modify_APP_Tasks2(void);
    int ret;
//	flash_write_status_init();
	ret = flash_modify_APP_Tasks_init((int *)&param, sizeof(PARAM) );
	if (ret <0) {
		printf("%s: init error!\n","SaveParam");
		return;
	}
    while(1) {
//    	ret = flash_write_status((int *)&now_status);
    	ret = flash_modify_APP_Tasks2();
        if (ret == 1) {
            //printf("param OK\n");
            break;
        } else if (ret ==0) {
            //printf("param NG\n");
            break;
        }
	}
#endif
#endif
}

/**
 *	@brief �f�B�t�H���g�p�����[�^�ݒ菈��
 *
 *	@retval �Ȃ�
 */
void SetDefault(void)
{
	
	param.start_time = DEFAULT_START_TIME; /* �ψڊJ�n���� */
	param.end_time = DEFAULT_END_TIME; /* �ψڏI������ */
	param.time_correction_time = DEFAULT_TIME_CORRECTION_TIME; /* �����C������ */
	param.time_correction_request_time = DEFAULT_TIME_CORRECTION_REQUEST_TIME; /* �����C���v������ */
	param.holiday_pattern = DEFAULT_HOLIDAY_DISPLAY_PATTERN; /* ���E�x�\���p�^�[�� P3 */
	param.holiday_mode = DEFAULT_HOLIDAY_MODE; /* �Ȃ� */
	param.holiday_start_time =DEFAULT_HOLIDAY_START_TIME ; /* �Ȃ��@���E�x�J�n�� */
	param.holiday_end_time = DEFAULT_HOLIDAY_END_TIME; /* �Ȃ��@���E�x�I���� */
	param.saturday_mode = DEFAULT_SATURDAY_MODE; /* �ψڂ���i���`���Ɠ����j */
	param.holiday_mode = DEFAULT_HOLIDAY_MODE; /* �ψڂȂ��i�ʏ�j */
	param.fail_pattern = DEFAULT_FAIL_PATTERN; /* �ϕ\���� P2 */
	param.offset_timer = DEFAULT_OFFSET_TIMER;/* �I�t�Z�b�g�^�C�} */
	param.no_musen_error_flag = 0; /* �����ُ���`�F�b�N�����ۂ� 0:����,1:���Ȃ� */
	param.same_nomove_flag = 0; /* �����ꍇ�͉ϕ\���ł�ω������Ȃ����[�h */
	param.no_fail_flag = 0; /* �t�F�C���ɂ����ۂ� 0:����,1:���Ȃ� */
	param.linkage_status = DEFAULT_LINKAGE; /* �A���ݒ� */
	param.no_board_error_flag = 0; /* �W���ňُ���`�F�b�N�����ۂ� �W���Ŕԍ��̃r�b�g�ʒu 0:����,1:���Ȃ� */
	param.no_pc_check_flag =0; /* �^�p�Ǘ��o�b�ԒʐM�ُ�𔻒���`�F�b�N�����ۂ� 0:����,1:���Ȃ� */
	param.debug_flg = 0xf;/* �f�o�b�O�p�̕\�����s���ꍇ��0�ȊO���Z�b�g���� */
	param.mdmcs_delay_time = 20;/* MDM_CS�o�͂̒x������(ms) */
	param.reset_count = 0;
	param.response_interval_time_val = RESPONSE_INTERVAL_TIME_VAL;/* 20170305 ����@A����̗v�����琧��@B����̉�����M�m�F�܂ł̍ŏ��҂�����(ms) */
	param.response_timeout_val = RESPONSE_TIMEOUT_VAL;/* 20170305 ����@A����̗v�����琧��@B����̉�����M�܂ł̃^�C���A�E�g�l(ms) */
	param.musen_timeout_val = MUSEN_TIMEOUT_VAL;/* 20170305 �����ʐM�̎�M�G���[�^�C���A�E�g�l(�b) */
	param.preamble_ptn = PREAMBLE_PTN;/* 20170308 L2.c�Ŏg�p����v���A���u���p�^�[�� */

	now_status.mode = MODE_REMOTE;/* ���u */
	now_status.status = STATUS_P1;/* �ʏ� */
	now_status.bef_status = STATUS_P1;/* �ʏ� */
	now_status.before_fail_status = STATUS_P1;/* �ʏ� */
	now_status.schedule = 0;
	now_status.start_time = 0;
	now_status.end_time = 0;
	now_status.offset_timer = DEFAULT_OFFSET_TIMER;
	now_status.power_outage_flag = 0;/* ��d�����t���O */
	now_status.power_outage_flag2 = 0;/* ��d�����t���O2 */
	now_status.power_outage_move_flag = 0;/* ��d�����Ńt�F�C���ֈړ������t���O */
	now_status.test_flag = 0;/* �e�X�g���t���O */
	now_status.board_error = 0;/* �\���ُ�t���O */
	now_status.time_status = 0;/* ������� 0:����,1:�ُ� */
	now_status.gps_status = 0;/* GPS��� */
	now_status.hoshu_status = 0;/* 1:�ێ�/0:�ʏ� */
	now_status.keikoutou_status = 0;/* 1:�u������/0:���� */
	now_status.before_gps_status = 0;/* �O���GPS��� */
	now_status.musen_status = 0;/* �����ʐM��� */
	now_status.before_musen_status = 0;/* �O��̖����ʐM��� */
	now_status.byou_status = 0;/* �����e��� */
	now_status.before_byou_status = 0;/* �O��̔����e��� */
	now_status.byou1_status = 0;/* �����e1��� */
	now_status.byou2_status = 0;/* �����e2��� */
	now_status.pc_tuushin_status = 0;/* �^�p�Ǘ�PC�ԒʐM��� */
	now_status.moniter_tuushin_status = 0;/* �Ď��ՊԒʐM��� */
	now_status.tanmatu_error = 0;/* �[���G���[��Ԃ�ێ� 0:�G���[����,1:�G���[�L�� */
	
}

/**
 *	@brief ���݂̃G���[��Ԃ�now_status���画�肷�鏈��
 *
 *	@retval �G���[���:1,�G���[�������:0
 */
int CheckErrorStatus(void)
{
	int ret = 0;
	if (now_status.board_error != 0) {
		ret = 1;
	}
	if (now_status.musen_status != 0) {
		ret = 1;
	}
	if (now_status.byou_status != 0) {
		ret = 1;
	}
	if (param.no_fail_flag == 1) {/* ���̏ꍇ�̓f�o�b�O�p�Ƀt�F�C���ɂ͂��Ȃ� */
		/* �t�F�C�������ǃt�F�C���ɂ��Ȃ� */
		ret = 0;
	}
	return ret;
}

/**
 *	@brief �X�e�[�^�X�̕\������
 *
 *	@retval �Ȃ�
 */
void StatusDisp(void)
{
	int i;
	
	static char mode_str[][16] = { "���u", "�蓮", "�Ď��Վw��"};
	static char status_str[][16] = { "����", "�ʏ�", "��|", "�ψ�", "���E�x", "�t�F�C��"};
	
	if (my_tanmatsu_type == CONTA_ADDRESS) {
		/* ����@A */
		printf("����@A\n");
	} else if (my_tanmatsu_type == 0x10) {
		/* �Ď��� */
		printf("�Ď���\n");
	} else {
		/* ����@B */
		printf("����@B %d\n", my_tanmatsu_type);
	}
	printf("���[�h�X�e�[�^�X       = %s,%s\n",mode_str[now_status.mode],status_str[now_status.status]);
	printf("�ψڊJ�n�I������,�^�C�}= %.02d:%.02d %.02d:%.02d %d\n"
		, (now_status.start_time/60) / 60, (now_status.start_time/60) % 60//yamazaki
		, (now_status.end_time/60) / 60, (now_status.end_time/60) % 60//yamazaki
		, now_status.offset_timer);
	printf("�X�P�W���[��,�����ݒ�  = %d,%d\n", now_status.schedule, now_status.time_req);
	printf("��d,test,brd_err,time,gps,mus,pc,mon,cds,b1,b2,ms= %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n"
		,now_status.power_outage_flag
		,now_status.test_flag
		,now_status.board_error
		,now_status.time_status
		,now_status.gps_status
		,now_status.musen_status
		,now_status.pc_tuushin_status
		,now_status.moniter_tuushin_status
		,now_status.cds_status
		,now_status.byou1_status
		,now_status.byou2_status
		,now_status.manual_status
	);
	printf("offset_value,rstage,lap,qsize=%d,%d,%d,%d,%d\n"
		,offset_time_up_value
		,board_rotate_stage_no
		,lap_time_max
		,io_control_timer/1000
		,lenqueue(IO_QUEUE) );
	printf("input_data     :");
	for ( i = 0 ; i < display_board_count; i++) {
		printf("%.02X ",input_data[i]);
	}
	printf("\n");
	printf("output_data    :");
	for ( i = 0 ; i < display_board_count; i++) {
		printf("%.02X ",output_data[i]);
	}
	printf("\n");
	printf("board_reg_buf  :");
	for ( i = 0 ; i < display_board_count; i++) {
		printf("%.02X ",board_reg_buf[i]);
	}
	printf("\n");
	printf("board_status   :");
	for ( i = 0 ; i < display_board_count; i++) {
		printf("%.02X ",board_status[i]);
	}
	printf("\n");
	if (response) {
		printf("response_status:");
		for ( i = 0 ; i < 7; i++) {
			printf("%.02X ",response->response.status[i]);
		}
		printf("\n");
	}
	extern int pcpower_stage_no;
	printf("pcpower_stage_no=%d\n",pcpower_stage_no);

	if (my_tanmatsu_type == CONTA_ADDRESS) {
		printf("response_received_count ");
		for ( i = 0 ; i < CONTROLER_MAX; i++) {
			printf("%d:%d ", i + 1, response_received_count[i]);
		}
		printf("\n");
		printf("response_total_err_count ");/* kaji20170305 */
		for ( i = 0 ; i < CONTROLER_MAX; i++) {
			printf("%d:%d ", i + 1, response_total_err_count[i]);
		}
		printf("\n");
		printf("response_time ");
		for ( i = 0 ; i < CONTROLER_MAX; i++) {
			printf("%d:%d ", i + 1, response_time[i]);
		}
		printf("\n");
	}

}

/**
 *	@brief �W�������n���ǂ����̔��菈��
 *
	*	@param [int no]  �W���ԍ�(0-7)
 *
 *	@retval 0:���n�ł͂Ȃ�,1:���n�ł���
 */
int CheckMuji(int no)
{
	int mj;
	if (now_status.status == STATUS_P1) {
		mj = my_io_info.muji_pattern_p1[no];
	} else if (now_status.status == STATUS_P2) {
		mj = my_io_info.muji_pattern_p2[no];
	}else if (now_status.status == STATUS_P3) {
		mj = my_io_info.muji_pattern_p3[no];
	}else {
		mj = my_io_info.muji_pattern_p2[no];
	}
//printf("***mj=%d\n",mj);
	return mj;
}


/**
 *	@brief (��d�N������)�ϕ\���̑���������鏈��
 *
 *	@param �Ȃ�
 *
 *	@retval -1: �Ȃ�  P1,P2,P3: �p�^�[��
 */
static int GetStartupBoardPattern(void)
{
	int	i, v, c, p;
	int ptn[3];
	
	ptn[0] = 0;
	ptn[1] = 0;
	ptn[2] = 0;
	for ( i = 0 ; i < display_board_count; i++) {
		v = BoardRead(i);
		switch (v) {/* ��~�ʒu�ɋ��Ȃ��ꍇ�͖����Ƃ��� */
			case P1:
				ptn[0]++;
				break;
			case P2:
				ptn[1]++;
				break;
			case P3:
				ptn[2]++;
		}
	}
	
	c = ptn[0];			p = P1;
	if ( ptn[1] > c ) {
		c = ptn[1];		p = P2;
	}
	if ( ptn[2] > v ) {
		c = ptn[2];		p = P3;
	}
	
	if ( (c*2) <= display_board_count ) {// �ߔ����𒴂��Ă��邱��(���X�́~)
		p = -1;
	}
	
	printf("***Read Board Pattern=%d\n", p);
	return p;
}


