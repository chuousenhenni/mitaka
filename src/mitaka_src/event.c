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

/*
 *===========================================================================================
 *					�O���ϐ���`
 *===========================================================================================
 */

extern STATUS now_status;/* ���݂̐ݒ�l */
extern int board_rotate_stage_no;

/*
 *===========================================================================================
 *					����	�֐���`
 *===========================================================================================
 */


static void EventMonitorRequest(void);/* �^�p��~���N�G�X�g�ɂ���ԕύX�v������ */
static void EventMonitorReleaseRequest(void);/* �^�p��~�������N�G�X�g�ɂ���ԕύX�v������ */
static void EventManualTuujouRequest(void);/* �蓮�ʏ탊�N�G�X�g�ɂ���ԕύX�v������ */
static void EventManualIssouRequest(void);/* �蓮��|���N�G�X�g�ɂ���ԕύX�v������ */
static void EventManualHenniRequest(void);/* �蓮�ψڃ��N�G�X�g�ɂ���ԕύX�v������ */
static int EventSWManualTuujouRequest(void);/* SW�蓮�ʏ탊�N�G�X�g�ɂ���ԕύX�v������ */
static int EventSWManualIssouRequest(void);/* SW�蓮��|���N�G�X�g�ɂ���ԕύX�v������ */
static int EventSWManualHenniRequest(void);/* SW�蓮�ψڃ��N�G�X�g�ɂ���ԕύX�v������ */
static int EventManualFailRequest(void);/* �蓮�t�F�C�����N�G�X�g�ɂ���ԕύX�v������ */
static void EventRemoteTuujouRequest(void);/* �ʏ펞�ԓ��ł̎蓮�����u���N�G�X�g�ɂ���ԕύX�v������ */
static void EventRemoteHenniRequest(void);/* �ψڎ��ԓ��ł̎蓮�����u���N�G�X�g�ɂ���ԕύX�v������ */
static void EventRemoteTuujou2HenniRequest(void);/* ���u���ψڎ��ԊJ�n���N�G�X�g�ɂ���ԕύX�v������ */
static void EventRemoteHenni2TuujouRequest(void);/* ���u���ʏ펞�ԊJ�n���N�G�X�g�ɂ���ԕύX�v������ */

/*
 *===========================================================================================
 *					�O���[�o���֐���`
 *===========================================================================================
 */



void EventRequest(int event);/* ��ԕύX�v������ */

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
 *	@brief ��ԕύX�v������
 *
 *	@param [int event]  ���X�G�X�g�C�x���g
 *
 *	@retval �Ȃ�
 */
static char event_pattern [][100] = {/* �f�o�b�O�p�̕\���p�^�[�� */
	"���u���N�G�X�g",
	"�ʏ펞�ԓ��ł̉��u���N�G�X�g",
	"�ψڎ��ԓ��ł̉��u���N�G�X�g",
	"���u�ʏ킩��ψڃ��N�G�X�g",
	"���u�ψڂ���ʏ탊�N�G�X�g",
	"�蓮�ʏ탊�N�G�X�g",
	"�蓮��|���N�G�X�g",
	"�蓮�ψڃ��N�G�X�g",
	"SW�蓮�ʏ탊�N�G�X�g",
	"SW�蓮��|���N�G�X�g",
	"SW�蓮�ψڃ��N�G�X�g",
	"�蓮�t�F�C�����N�G�X�g",
	"�^�p��~���N�G�X�g",
	"�^�p��~�������N�G�X�g",
	"�t�F�C���ُ�i���v�AGPS�Aboard�j���N�G�X�g",
	"��d�������N�G�X�g",
	"��d�������A���N�G�X�g",
	"�t�F�C���ُ한�A�i���v�AGPS�j���N�G�X�g",
	"�t�F�C�����̒ʏ�ւ̕��A���N�G�X�g",
	"����@B�̒ʏ탊�N�G�X�g",
	"����@B�̈�|���N�G�X�g",
	"����@B�̕ψڃ��N�G�X�g",
};

/*
 *===========================================================================================
 *					�O���[�o���֐�
 *===========================================================================================
 */

void EventRequest(int event)
{
	int st;
	char str[256];
	
	//if (event != FAIL_REQUEST) {
	if ((event != FAIL_REQUEST) && (event != REMOTE_REQUEST)){
		/* �t�F�C�����N�G�X�g�͏�ɏ������� */
		if (!empty(IO_QUEUE)) {
			/* ���s���܂��͗v���L�� */
			sprintf(str, "���̗v���L��̂��� EventRequest event = %s �L�����Z��",event_pattern[event]);
			DebugPrint("", str, 1);
			return;
		}
		if ((board_rotate_stage_no != BOARD_ROTATE_END) && (board_rotate_stage_no != BOARD_ROTATE_IDLE) ) {
			/* ���s���܂��͗v���L�� */
			sprintf(str, "���쒆 EventRequest event = %s �L�����Z��",event_pattern[event]);
			DebugPrint("", str, 1);
 			return;
		}
		sprintf(str, "EventRequest event = %s",event_pattern[event]);
		DebugPrint("", str, 1);
	}
	if (event == REMOTE_REQUEST) {
		/* ����w�߂܂���SW�Ŏ蓮���牓�u�ɕς�����ꍇ */
		st = GetNowRemoteStatus();
//printf("*****st=%d\n",st);
		if (st == STATUS_P1) {
			event = REMOTE_TUUJOU_REQUEST;/* �ʏ펞�ԓ��ł̎蓮�����u���N�G�X�g */
		} else if (st == STATUS_P1P2) {
			event = REMOTE_TUUJOU_REQUEST;/* �ʏ펞�ԓ��ł̎蓮�����u���N�G�X�g */
		} else if (st == STATUS_P3P2) {
			event = REMOTE_TUUJOU_REQUEST;/* �ʏ펞�ԓ��ł̎蓮�����u���N�G�X�g */
		} else {
			event = REMOTE_HENNI_REQUEST;/* �ψڎ��ԓ��ł̎蓮�����u���N�G�X�g */
		}
	}

	switch (event) {
	case MONITOR_REQUEST:/* �^�p��~���N�G�X�g */
		sprintf(str, "�^�p��~���N�G�X�g");
		EventMonitorRequest();
		break;
	case MONITOR_RELEASE_REQUEST:/* �^�p��~�������N�G�X�g */
		sprintf(str, "�^�p��~�������N�G�X�g");
		EventMonitorReleaseRequest();/* kaji20170308 */
		break;
		
	case MANUAL_TUUJOU_REQUEST:/* �蓮�ʏ탊�N�G�X�g */
		sprintf(str, "�蓮�ʏ탊�N�G�X�g");
		EventManualTuujouRequest();
		break;
		
	case MANUAL_ISSOU_REQUEST:/* �蓮��|���N�G�X�g */
		sprintf(str, "�蓮��|���N�G�X�g");
		EventManualIssouRequest();
		break;

	case MANUAL_HENNI_REQUEST:/* �蓮�ψڃ��N�G�X�g */
		sprintf(str, "�蓮�ψڃ��N�G�X�g");
		EventManualHenniRequest();
		break;

	case SW_MANUAL_TUUJOU_REQUEST:/* SW�蓮�ʏ탊�N�G�X�g */
		sprintf(str, "SW�蓮�ʏ탊�N�G�X�g");
		if (!EventSWManualTuujouRequest()) {
			strcat(str, " �L�����Z��");
		}
		break;
		
	case SW_MANUAL_ISSOU_REQUEST:/* SW�蓮��|���N�G�X�g */
		sprintf(str, "SW�蓮��|���N�G�X�g");
		if (!EventSWManualIssouRequest()) {
			strcat(str, " �L�����Z��");
		}
		break;
		
	case SW_MANUAL_HENNI_REQUEST:/* SW�蓮�ψڃ��N�G�X�g */
		sprintf(str, "SW�蓮�ψڃ��N�G�X�g");
		if (!EventSWManualHenniRequest()) {
			strcat(str, " �L�����Z��");
		}
		break;
	case MANUAL_FAIL_REQUEST:/* SW�蓮�ψڃ��N�G�X�g */
		sprintf(str, "�蓮�t�F�C�����N�G�X�g");
		if (!EventManualFailRequest()) {
			strcat(str, " �L�����Z��");
		}
		break;

	case REMOTE_TUUJOU_REQUEST:/* �ʏ펞�ԓ��ł̎蓮�����u���N�G�X�g */
		sprintf(str, "�ʏ펞�ԓ��ł̎蓮�����u���N�G�X�g");
		EventRemoteTuujouRequest();
		break;
	case REMOTE_HENNI_REQUEST:/* �ψڎ��ԓ��ł̎蓮�����u���N�G�X�g */
		sprintf(str, "�ψڎ��ԓ��ł̎蓮�����u���N�G�X�g");
		EventRemoteHenniRequest();
		break;
		
		
	case REMOTE_TUUJOU2HENNI_REQUEST:/* ���u���ψڎ��ԊJ�n���N�G�X�g */
		sprintf(str, "���u���ψڎ��ԊJ�n���N�G�X�g");
		EventRemoteTuujou2HenniRequest();
		break;
	case REMOTE_HENNI2TUUJOU_REQUEST:/* ���u���ʏ펞�ԊJ�n���N�G�X�g */
		sprintf(str, "���u���ʏ펞�ԊJ�n���N�G�X�g");
		EventRemoteHenni2TuujouRequest();
		break;
		
	case FAIL_REQUEST:/* �t�F�C�����N�G�X�g */
		if (now_status.mode == MODE_REMOTE) {
			st = GetNowRemoteStatus();
//			if (st == STATUS_P3) {
//			if ((st == STATUS_P2)||(st == STATUS_P3)||(st == STATUS_P3P2)) {/* kaji20170225 */
			if ((st == STATUS_P1P2)||(st == STATUS_P2)||(st == STATUS_P3)||(st == STATUS_P3P2)) {/* kaji20170227 */
				strcpy(str, "");
				//if (now_status.gps_status == 1) {
				//	strcat(str, "���v�ُ�");
				//}
				if (now_status.musen_status == 1) {
					strcat(str, "�����ُ�");
				}
				if (now_status.byou_status == 1) {
					strcat(str, "�����e�ُ�");
				}
				if (now_status.board_error == 1) {
					strcat(str, "BOARD�ُ�");
				}
				strcat(str, "���N�G�X�g");
				if (now_status.status != STATUS_FAIL) {
					strcat(str, " �t�F�C���ֈړ�");
					IO_ChangeReq(PX_FAIL);
				} else {
					strcat(str, " �ł����łɃt�F�C��");
				}
			} else {
				strcpy(str, "���u��|/���u�ψڂ���Ȃ��̂�");
				//if (now_status.gps_status == 1) {
				//	strcat(str, "���v�ُ�");
				//}
				if (now_status.musen_status == 1) {
					strcat(str, "�����ُ�");
				}
				if (now_status.byou_status == 1) {
					strcat(str, "�����e�ُ�");
				}
				if (now_status.board_error == 1) {
					strcat(str, "BOARD�ُ�");
				}
				strcat(str, "���N�G�X�g �L�����Z��");
			}
		} else {
			strcpy(str, "���u����Ȃ��̂�");
			//if (now_status.gps_status == 1) {
			//	strcat(str, "���v�ُ�");
			//}
			if (now_status.musen_status == 1) {
				strcat(str, "�����ُ�");
			}
			if (now_status.byou_status == 1) {
				strcat(str, "�����e�ُ�");
			}
			if (now_status.board_error == 1) {
				strcat(str, "BOARD�ُ�");
			}
			strcat(str, "���N�G�X�g �L�����Z��");
		}
		break;
	case FAIL2TUUJOU_REQUEST:/* �t�F�C�����̒ʏ�ւ̕��A���N�G�X�g */
		sprintf(str, "�t�F�C�����̒ʏ�ւ̕��A���N�G�X�g");
		now_status.mode = MODE_REMOTE;
		if (now_status.status == STATUS_FAIL) {
			IO_ChangeReq(PX_P1);
		}
		break;
	case FAIL_RECOVER_REQUEST:/* ���v�ُ한�A���N�G�X�g */
		sprintf(str, "�t�F�C���ُ한�A�i���v�A�����j���N�G�X�g");
		//now_status.mode = MODE_MANUAL;
		if (now_status.status == STATUS_FAIL) {
			switch (now_status.mode) {
				case MODE_REMOTE: /* ���u */
					IO_ChangeReq(GetNowRemoteStatus());/* ���݂̉��u�̏�Ԃɂ��� */
					break;
				case MODE_MANUAL: /* �蓮 */
					IO_ChangeReq(now_status.before_fail_status);
					break;
				case MODE_MONITOR: /* �Ď��� */
					IO_ChangeReq(STATUS_P1);/* �ʏ�ɖ߂� */
					break;
			}
		}
		break;

	case POWER_OFF_REQUEST:/* ��d�������N�G�X�g */
		sprintf(str, "��d�������N�G�X�g");
		switch (now_status.mode) {
			case MODE_REMOTE: /* ���u */
				switch (now_status.status) {
					case STATUS_P3:/* �ψ� */
						init_queue(IO_QUEUE);/* �ً}���ԂȂ̂ŃL���[����ɂ��� */
						IO_ChangeReq(PX_POWEROFF);
						now_status.power_outage_move_flag = 1;/* ��d�����Ńt�F�C���ֈړ������t���O */
						break;
					default:
						strcat(str, " �ψڂł͂Ȃ��̂ŉ������Ȃ�");
						break;
				}
				break;
			case MODE_MANUAL: /*  */
				switch (now_status.status) {
					case STATUS_P3:/* �ψ� */
						init_queue(IO_QUEUE);/* �ً}���ԂȂ̂ŃL���[����ɂ��� */
						IO_ChangeReq(PX_POWEROFF);
						now_status.power_outage_move_flag = 1;/* ��d�����Ńt�F�C���ֈړ������t���O */
						break;
					default:
						strcat(str, " �ψڂł͂Ȃ��̂ŉ������Ȃ�");
						break;
				}
				break;
			default:
				strcat(str, " �������Ȃ�");
				break;
		}
		//20170207 PcPower(1);/* PCPOWER�̏o�͏��� */
		break;
	case POWER_RECOVER_REQUEST:/* ��d�������A���N�G�X�g */
		printf("**POWER_RECOVER_REQUEST\n");
		sprintf(str, "��d�������A���N�G�X�g");
		/* �����Œ�����߂� */
		//if (now_status.power_outage_move_flag == 1) {
		if (1) {
		printf("2**POWER_RECOVER_REQUEST %d\n",power_off_bef_status);
			power_off_bef_status = GetNowRemoteStatus();
		printf("2**POWER_RECOVER_REQUEST %d\n",power_off_bef_status);
			switch (power_off_bef_status) {
				case STATUS_P1:/* �ʏ� */
				case STATUS_P3P2:/* 20170225 �ψڂ���̕ω����̈�| */
					if (param.fail_pattern != STATUS_P1) {
						IO_ChangeReq(PX_P1);
					}
					break;
				case STATUS_P2:/* ��| */
					if (param.fail_pattern != STATUS_P2) {
						IO_ChangeReq(PX_P2);
					}
					break;
				case STATUS_P3:/* �ψ� */
				case STATUS_P1P2:/* 20170225 �ʏ킩��̕ω����̈�| */
					if (param.fail_pattern != STATUS_P3) {
						IO_ChangeReq(P2_P2);/* ��|���o�R���� 20170224 */
						IO_ChangeReq(PX_P3);
					}
					break;
				default:
					break;
			}
			now_status.power_outage_move_flag = 0;/* ��d�����Ńt�F�C���ֈړ������t���O�N���A */
		} else {
			strcat(str, " �L�����Z��");
		}
		//20170207 PcPower(0);/* PCPOWER�̏o�͏��� */
		break;
	case TUUJOU_REQUEST:/* ����@B�̒ʏ탊�N�G�X�g */
		if (now_status.mode == MODE_REMOTE) {
			sprintf(str, "����@B�̒ʏ탊�N�G�X�g");
			IO_ChangeReq(PX_P1);
		} else {
			sprintf(str, "����@B�̒ʏ탊�N�G�X�g�@�L�����Z��");
		}
		break;
	case ISSOU_REQUEST:/* ����@B�̈�|���N�G�X�g */
		if (now_status.mode == MODE_REMOTE) {
			sprintf(str, "����@B�̈�|���N�G�X�g");
			IO_ChangeReq(PX_P2);
		} else {
			sprintf(str, "����@B�̈�|���N�G�X�g�@�L�����Z��");
		}
		break;
	case HENNI_REQUEST:/* ����@B�̕ψڃ��N�G�X�g */
		if (now_status.mode == MODE_REMOTE) {
			sprintf(str, "����@B�̕ψڃ��N�G�X�g");
			IO_ChangeReq(PX_P3);
		} else {
			sprintf(str, "����@B�̕ψڃ��N�G�X�g�@�L�����Z��");
		}
		break;
	case CONTB_FAIL_REQUEST:/* ����@B�̃t�F�C�����N�G�X�g */
		if (now_status.mode == MODE_REMOTE) {
			sprintf(str, "����@B�̃t�F�C�����N�G�X�g");
			IO_ChangeReq(PX_FAIL);
		} else {
			sprintf(str, "����@B�̃t�F�C�����N�G�X�g�@�L�����Z��");
		}
		break;
	}
	DebugPrint("", str, 1);
	
}

/*
 *===========================================================================================
 *					�����֐�
 *===========================================================================================
 */

/**
 *	@brief �^�p��~���N�G�X�g�ɂ���ԕύX�v������
 *
 *	@retval �Ȃ�
 */
static void EventMonitorRequest(void)
{
	switch (now_status.mode) {
		case MODE_REMOTE: /* ���u */
		case MODE_MONITOR: /* �Ď��Ղ���̎w�߃��[�h */
			switch (now_status.status) {
				case STATUS_P2:/* ��| */
				case STATUS_FAIL:/* �t�F�C���Ȃ�� */
					IO_ChangeReq(PX_P1);/* �ʏ�ɑJ�� */
					break;
				case STATUS_P3:/* �ψ� */
					IO_ChangeReq(PX_P2);/* ��|�Ɉړ� */
					IO_ChangeReq(P2_P2);/* �I�t�Z�b�g�^�C�} */
					IO_ChangeReq(PX_P1);/* �ʏ�ɑJ�� */
					break;
				default:
					break;
			}
			break;
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P2:/* ��| */
				case STATUS_FAIL:/* �t�F�C���Ȃ�� */
					IO_ChangeReq(PX_P1);
					break;
				case STATUS_P3:/* �ψ� */
					IO_ChangeReq(PX_P2);/* ��|�Ɉړ� */
					IO_ChangeReq(P2_P2);/* �I�t�Z�b�g�^�C�} */
					IO_ChangeReq(PX_P1);
					break;
				default:
					break;
			}
			break;
	}
	now_status.mode = MODE_MONITOR;
}

/**
 *	@brief �^�p��~�������N�G�X�g�ɂ���ԕύX�v������
 *
 *	@retval �Ȃ�
 */
static void EventMonitorReleaseRequest(void)
{
	switch (now_status.status) {
		case STATUS_P1:/* �ʏ�Ȃ�� */
			IO_ChangeReq(PX_P1);/* �ʏ�ɑJ�� */
			break;
		case STATUS_P2:/* ��| */
		case STATUS_FAIL:/* �t�F�C���Ȃ�� */
			IO_ChangeReq(P2_P2);/* �I�t�Z�b�g�^�C�} */
			break;
		case STATUS_P3:/* �ψ� */
			IO_ChangeReq(PX_P2);/* ��|�Ɉړ� */
			IO_ChangeReq(P2_P2);/* �I�t�Z�b�g�^�C�} */
			break;
		default:
			break;
	}
	now_status.mode = MODE_REMOTE;
}

/**
 *	@brief �蓮�ʏ탊�N�G�X�g�ɂ���ԕύX�v������
 *
 *	@retval �Ȃ�
 */
static void EventManualTuujouRequest(void)
{
	switch (now_status.mode) {
		case MODE_REMOTE: /* ���u */
			switch (now_status.status) {
				case STATUS_P1:/* �ʏ� */
					now_status.mode = MODE_MANUAL;
					break;
				case STATUS_P3:/* �ψ� */
					IO_ChangeReq(PX_P2);/* ��|�Ɉړ� */
					IO_ChangeReq(P2_P2);/* �I�t�Z�b�g�^�C�} */
					IO_ChangeReq(PX_P1);/* �ʏ�ɑJ�� */
					now_status.mode = MODE_MANUAL;
					break;
				default:
					break;
			}
			break;
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P2:/* ��| */
				case STATUS_FAIL:/* �t�F�C���Ȃ�� */
					IO_ChangeReq(PX_P1);
					break;
				case STATUS_P3:/* �ψ� */
					IO_ChangeReq(PX_P2);/* ��|�Ɉړ� */
					IO_ChangeReq(P2_P2);/* �I�t�Z�b�g�^�C�} */
					IO_ChangeReq(PX_P1);/* �ʏ�ɑJ�� */
					break;
				default:
					break;
			}
			break;
	}
}

/**
 *	@brief �蓮��|���N�G�X�g�ɂ���ԕύX�v������
 *
 *	@retval �Ȃ�
 */
static void EventManualIssouRequest(void)
{
	switch (now_status.mode) {
		case MODE_REMOTE: /* ���u */
			switch (now_status.status) {
				case STATUS_P1:/* �ʏ� */
					IO_ChangeReq(PX_P2);
					now_status.mode = MODE_MANUAL;
					break;
				case STATUS_P3:/* �ψ� */
					IO_ChangeReq(PX_P2);
					now_status.mode = MODE_MANUAL;
					break;
				default:
					break;
			}
			break;
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P1:/* �ʏ�Ȃ�� */
				case STATUS_P3:/* �ψڂȂ�� */
				case STATUS_FAIL:/* �t�F�C���Ȃ�� */
					IO_ChangeReq(PX_P2);/* ��|�֑J�� */
					break;
				default:
					break;
			}
			break;
	}
}

/**
 *	@brief �蓮�ψڃ��N�G�X�g�ɂ���ԕύX�v������
 *
 *	@retval �Ȃ�
 */
static void EventManualHenniRequest(void)
{
	switch (now_status.mode) {
		case MODE_REMOTE: /* ���u */
			switch (now_status.status) {
				case STATUS_P1:/* �ʏ� */
					IO_ChangeReq(PX_P2);/* ��|�Ɉړ� */
					IO_ChangeReq(P2_P2);/* �I�t�Z�b�g�^�C�} */
					IO_ChangeReq(PX_P3);/* �ψڂɈړ� */
					now_status.mode = MODE_MANUAL;
					break;
				case STATUS_P3:/* �ψ� */
					now_status.mode = MODE_MANUAL;
					break;
				default:
					break;
			}
			break;
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P1:/* �ʏ�Ȃ�� */
					IO_ChangeReq(PX_P2);/* ��|�Ɉړ� */
					IO_ChangeReq(P2_P2);/* �I�t�Z�b�g�^�C�} */
					IO_ChangeReq(PX_P3);/* �ψڂ֑J�� */
					break;
				case STATUS_P2:/* ��|�Ȃ�� */
				case STATUS_FAIL:/* �t�F�C���Ȃ�� */
					IO_ChangeReq(PX_P3);/* �ψڂ֑J�� */
					break;
				default:
					break;
			}
			break;
	}
}

/**
 *	@brief SW�蓮�ʏ탊�N�G�X�g�ɂ���ԕύX�v������
 *
 *	@retval �Ȃ�
 */
static int EventSWManualTuujouRequest(void)
{
	int flg = 0;
	switch (now_status.mode) {
		case MODE_REMOTE: /* ���u */
			switch (now_status.status) {
				case STATUS_P1:/* �ʏ�Ȃ�� */
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				case STATUS_FAIL:/* 20170210 ���u�t�F�C���Ȃ�� */
					IO_ChangeReq(PX_P1);
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				case STATUS_P2:/* ��|�Ȃ�� */
					IO_ChangeReq(PX_P1);/* �ʏ�ɑJ�� */
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				default:
					break;
			}
			break;
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P1:/* �蓮�ʏ�ł� 20170205 */
				case STATUS_P2:/* �蓮��|�Ȃ�� */
				case STATUS_FAIL:/* �蓮�t�F�C���Ȃ�� */
					IO_ChangeReq(PX_P1);/* �ʏ�ɑJ�� */
					flg = 1;
					break;
				default:
					break;
			}
			break;
	}
	return flg;
}

/**
 *	@brief SW�蓮��|���N�G�X�g�ɂ���ԕύX�v������
 *
 *	@retval �Ȃ�
 */
static int EventSWManualIssouRequest(void)
{
	int flg = 0;
	switch (now_status.mode) {
		case MODE_REMOTE: /* ���u */
			switch (now_status.status) {
				case STATUS_P1:/* ���u�ʏ�Ȃ�� */
					IO_ChangeReq(PX_P2);
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				case STATUS_FAIL:/* 20170210 ���u�t�F�C���Ȃ�� */
					IO_ChangeReq(PX_P2);
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				case STATUS_P3:/* ���u�ψڂȂ�� */
					IO_ChangeReq(PX_P2);
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				default:
					break;
			}
			break;
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P1:/* �蓮�ʏ�Ȃ�� */
				case STATUS_P2:/* �蓮��|�ł� 20170205 */
				case STATUS_P3:/* �蓮�ψڂȂ�� */
				case STATUS_FAIL:/* �蓮�t�F�C���Ȃ�� */
					IO_ChangeReq(PX_P2);
					flg = 1;
					break;
				default:
					break;
			}
			break;
	}
	return flg;
}

/**
 *	@brief SW�蓮�ψڃ��N�G�X�g�ɂ���ԕύX�v������
 *
 *	@retval �Ȃ�
 */
static int EventSWManualHenniRequest(void)
{
	int flg = 0;
	switch (now_status.mode) {
		case MODE_REMOTE: /* ���u */
			switch (now_status.status) {
				case STATUS_P2:/* ��|�Ȃ�� */
					IO_ChangeReq(PX_P3);
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				case STATUS_FAIL:/* 20170210 ���u�t�F�C���Ȃ�� */
					IO_ChangeReq(PX_P3);
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				case STATUS_P3:/* ���u�ψڂȂ�� */
					now_status.mode = MODE_MANUAL;
					flg = 1;
					break;
				default:
					break;
			}
			break;
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P2:/* ��|�Ȃ�� */
				case STATUS_P3:/* �ψقł� 20170205 */
				case STATUS_FAIL:/* �t�F�C���Ȃ�� */
					IO_ChangeReq(PX_P3);
					flg = 1;
					break;
				default:
					break;
			}
			break;
	}
	return flg;
}

/**
 *	@brief �蓮�t�F�C�����N�G�X�g�ɂ���ԕύX�v������
 *
 *	@retval �Ȃ�
 */
static int EventManualFailRequest(void)
{
	int flg = 0;
	switch (now_status.mode) {
		case MODE_REMOTE: /* ���u */
			switch (now_status.status) {
				case STATUS_P1:/* ���u�ʏ�Ȃ�� */
				case STATUS_P2:/* ���u��|�Ȃ�� */
				case STATUS_P3:/* ���u�ψڂȂ�� */
					now_status.mode = MODE_MANUAL;
					IO_ChangeReq(PX_FAIL);
					flg = 1;
					break;
				default:
					break;
			}
			break;
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P1:/* �ʏ�Ȃ�� */
				case STATUS_P2:/* ��|�Ȃ�� */
				case STATUS_P3:/* �ψڂȂ�� */
					IO_ChangeReq(PX_FAIL);
					flg = 1;
					break;
				default:
					break;
			}
			break;
	}
	return flg;
}

/**
 *	@brief �ʏ펞�ԓ��ł̎蓮�����u���N�G�X�g�ɂ���ԕύX�v������
 *         �ʏ펞�ԓ��ł̃t�F�C������̕��A�����Ă΂��
 *
 *	@retval �Ȃ�
 */
static void EventRemoteTuujouRequest(void)
{
	switch (now_status.mode) {
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P1:/* �ʏ� */
					now_status.mode = MODE_REMOTE;
					break;
				case STATUS_P2:/* ��| */
					now_status.mode = MODE_REMOTE;
					IO_ChangeReq(P2_P2);/* 20170226 �I�t�Z�b�g�^�C�} */
					IO_ChangeReq(PX_P1);/* �ʏ�Ɉړ� */
					break;
				case STATUS_P3:/* �ψ� */
					now_status.mode = MODE_REMOTE;
					IO_ChangeReq(PX_P2);/* ��|�Ɉړ� */
					IO_ChangeReq(P2_P2);/* �I�t�Z�b�g�^�C�} */
					IO_ChangeReq(PX_P1);/* �ʏ�Ɉړ� */
					break;
				case STATUS_FAIL:/* �t�F�C�� */
					/* �t�F�C������̕��A������ꍇ�@�����͂悭�킩��Ȃ� */
					IO_ChangeReq(PX_P1);/* �ʏ�Ɉړ� */
					break;
				default:
					break;
			}
			break;
		case MODE_REMOTE: /*  */
			switch (now_status.status) {
				case STATUS_FAIL:/* �t�F�C�� */
					/* �t�F�C������̕��A������ꍇ�@�����͂悭�킩��Ȃ� */
					IO_ChangeReq(PX_P2);/* 20170225 ��|�Ɉړ� */
					IO_ChangeReq(P2_P2);/* 20170225  �I�t�Z�b�g�^�C�} */
					IO_ChangeReq(PX_P1);/* �ʏ�Ɉړ� */
					break;
				default:
					break;
			}
			break;
	}
}

/**
 *	@brief �ψڎ��ԓ��ł̎蓮�����u���N�G�X�g�ɂ���ԕύX�v������
 *         �ψڎ��ԓ��ł̃t�F�C������̕��A�����Ă΂��
 *
 *	@retval �Ȃ�
 */
static void EventRemoteHenniRequest(void)
{
	switch (now_status.mode) {
		case MODE_MANUAL: /*  */
			switch (now_status.status) {
				case STATUS_P1:/* �ʏ� */
					now_status.mode = MODE_REMOTE;
					IO_ChangeReq(PX_P2);/* ��|�Ɉړ� */
					IO_ChangeReq(P2_P2);/* �I�t�Z�b�g�^�C�} */
					IO_ChangeReq(PX_P3);/* �ψڂɈړ� */
					break;
				case STATUS_P2:/* ��| */
					IO_ChangeReq(P2_P2);/* 20170226 �I�t�Z�b�g�^�C�} */
					IO_ChangeReq(PX_P3);/* �ψڂɈړ� */
					now_status.mode = MODE_REMOTE;
					break;
				case STATUS_P3:/* �ψ� */
					now_status.mode = MODE_REMOTE;
					break;
				default:
					break;
			}
			break;
		case MODE_REMOTE: /* 20170212 */
			switch (now_status.status) {
				case STATUS_FAIL:/* �t�F�C�� */
					/* �t�F�C������̕��A������ꍇ�@�����͂悭�킩��Ȃ� */
					IO_ChangeReq(PX_P2);/* 20170225 ��|�Ɉړ� */
					IO_ChangeReq(P2_P2);/* 20170225  �I�t�Z�b�g�^�C�} */
					IO_ChangeReq(PX_P3);/* �ψڂɈړ� */
					break;
				default:
					break;
			}
			break;
	}
}

/**
 *	@brief ���u���ψڎ��ԊJ�n���N�G�X�g�ɂ���ԕύX�v������
 *
 *	@retval �Ȃ�
 */
static void EventRemoteTuujou2HenniRequest(void)
{
	switch (now_status.mode) {
		case MODE_REMOTE: /* ���u */
			if (CheckErrorStatus() != 0) {
				/* �ُ��Ԃ̏ꍇ�� */
//#ifdef	kaji20170208
//�ُ펞�͎��ԂŃC�x���g���Ȃ�
				IO_ChangeReq(PX_FAIL);/* �t�F�C���Ɉړ� */
//#endif
			} else {
				switch (now_status.status) {
					case STATUS_P1:/* �ʏ� */
						IO_ChangeReq(PX_P2);/* ��|�Ɉړ� */
						IO_ChangeReq(P2_P2);/* �I�t�Z�b�g�^�C�} */
						IO_ChangeReq(PX_P3);/* �ψڂɈړ� */
						break;
					default:
						break;
				}
			}
			break;
	}
}

/**
 *	@brief ���u���ʏ펞�ԊJ�n���N�G�X�g�ɂ���ԕύX�v������
 *
 *	@retval �Ȃ�
 */
static void EventRemoteHenni2TuujouRequest(void)
{
	switch (now_status.mode) {
		case MODE_REMOTE: /* ���u */
			switch (now_status.status) {
				case STATUS_P3:/* �ψ� */
					IO_ChangeReq(PX_P2);/* ��|�Ɉړ� */
					IO_ChangeReq(P2_P2);/* �I�t�Z�b�g�^�C�} */
					IO_ChangeReq(PX_P1);/* �ʏ�Ɉړ� */
					break;
				case STATUS_FAIL:/* �t�F�C�� */
					/* �t�F�C������̕��A������ꍇ�@�����͂悭�킩��Ȃ� */
					/*
					�G���[�v�����`�F�b�N���A�G���[�����Ȃ��
					�t�F�C���ɂȂ����v���͕ψڒ��̕ۑ��̐���@�̃t�F�C���ɂ����̂Ȃ̂Œʏ�Ɉړ�����
					*/
//#ifdef	kaji20170208
//�t�F�C���͎��ԂŖ߂��Ȃ�
				//201070209 if (CheckErrorStatus() == 0) {
						/* �ُ�ł͖����ꍇ�� */
						IO_ChangeReq(PX_P1);/* �ʏ�Ɉړ� */
				//20170209	}
//#endif
					break;
				default:
					break;
			}
			break;
	}
}
