/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	contb.c
 *	�T�v
 *  ����@B���䕔
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
#include "event.h"
#include "io.h"

/*
 *===========================================================================================
 *					�����萔��`�E�^��`�E�}�N����`
 *===========================================================================================
 */
 
//define.h�ֈړ� #define MUSEN_TIMEOUT_VAL (45) /* �����ʐM�̎�M�G���[�^�C���A�E�g�l(�b)  kaji20170301 60��45 */

/*
 *===========================================================================================
 *					�����ϐ���`
 *===========================================================================================
 */

static BROADCAST_COMMAND broadcast_command;/* ����@A����̓���w�߂̓d�� */
static NORMAL_COMMAND normal_command;/* ����@A����̒ʏ�w�߂̓d�� */
static RESPONSE_COMMAND response_command;/* ����@A�ւ̊Ď������̓d�� */
static RESPONSE_COMMAND response_command_send_buffer;/* ����@A�ւ̊Ď������̑��M����d�� */
static int normal_command_timer;/* �ʏ�w�ߎ�M�҂��^�C�} */
static unsigned char cmp_my_linkage;/* kaji20170407 BROADCAST_COMMAND��linkage(sub_adr)flag�Ǝ��ǂ̔�r�p */

/*
 *===========================================================================================
 *					�O���ϐ���`
 *===========================================================================================
 */

extern int my_tanmatsu_type;/* �[���^�C�v 1:A,2:B... */
extern HANDLE hComm1;       /* �V���A���|�[�g�̃n���h�� */
extern STATUS now_status;/* ���݂̐ݒ�l */

/*
 *===========================================================================================
 *					����	�֐���`
 *===========================================================================================
 */

static void RcvControlerBSrv(void);
//static void BroadcastCommand(BROADCAST_COMMAND *com);/* ����w�ߎ�M���� */
static void NormalCommand(NORMAL_COMMAND *com) ;/* �ʏ�w�ߎ�M���� */
static void SendResponse(void);/* �Ď������̑��M���� */

/*
 *===========================================================================================
 *					�O���[�o���֐���`
 *===========================================================================================
 */

void ControlerBInit(void );
void ControlerBSrv(void);
void TimerIntContB(int count);/* ����@B�̃^�C�}���荞�ݏ��� */

void BroadcastDisp ( void );
void NormalDisp ( void);
void ResponseDisp ( void);

/*
 *===========================================================================================
 *					�O��	�֐���`
 *===========================================================================================
 */

extern void SendCom1(HANDLE h, unsigned char *p, int size);

/*
 *===========================================================================================
 *					�O���[�o���֐�
 *===========================================================================================
 */

/**
 *	@brief ����@B�̃^�C�}���荞�ݏ���
 *
 *	@retval �Ȃ�
 */
void TimerIntContB(int count)
{
	char str[256];
	if (param.no_musen_error_flag == 0) {
		normal_command_timer += count;/* �ʏ�w�ߎ�M�҂��^�C�} */
//20170305	if (normal_command_timer > (MUSEN_TIMEOUT_VAL*1000)) {
		if (normal_command_timer > (param.musen_timeout_val*1000)) {
			now_status.musen_status = 1;
		} else {
			now_status.musen_status = 0;
		}
		if (now_status.before_musen_status != now_status.musen_status) {
			if (param.no_fail_flag == 0) {/* ���̏ꍇ�̓f�o�b�O�p�Ƀt�F�C���ɂ͂��Ȃ� */
				if (now_status.before_musen_status == 0) {
					sprintf(str,"�����ُ�ɕω�");
					DebugPrint("", str, 1);
					EventRequest(FAIL_REQUEST);/* �ُ탊�N�G�X�g */
					cont_led_status[LED_MUSEN] = LED_STATUS_TOGGLE;
				} else {
					sprintf(str, "��������ɕω�\n");
					DebugPrint("", str, 1);
#ifdef	kaji20170208
�����ł̓t�F�C�����甲���锻�f���s��Ȃ�
					if ((now_status.status == STATUS_FAIL) && (CheckErrorStatus() == 0)) {
						EventRequest(FAIL_RECOVER_REQUEST);/* �ُ한�A���N�G�X�g */
					}
#endif
					cont_led_status[LED_MUSEN] = LED_STATUS_OFF;
				}
			}
		}
		now_status.before_musen_status = now_status.musen_status;
	}
}


/**
 *	@brief ����@B�̏���������
 *
 *	@retval �Ȃ�
 */
void ControlerBInit(void )
{
	int	i;
	
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
	response_command.h.no = 0;/* �K�i�ԍ� 00H�Œ� */
	response_command.h.dst = 1;/* ����A�h���X  01H�Œ� */
	response_command.h.src = my_tanmatsu_type;/* ���M���A�h���X 01H�`08H */
	response_command.h.sub_adr = 0;/* �T�u�A�h���X 00H�Œ� */
	response_command.h.priority = 2;/* �D�惌�x�� 02H�Œ� */
	response_command.h.s_no = 0;/* �ʔ� 00H�Œ� */
	response_command.h.contoroler_no = 0x19;/* �[����� 19H */
	response_command.h.info = 0x11;/* ����� 11H */
	response_command.h.div_no = 0x81;/* �����ԍ� 81H */
	response_command.h.length = 15;/* �f�[�^�� 15(0FH) */

	normal_command_timer = 0;/* �ʏ�w�ߎ�M�҂��^�C�} */

	cmp_my_linkage = 0x80;	/* kaji20170407 */
	for ( i=1; i<my_tanmatsu_type; i++ ) {
		cmp_my_linkage >>= 1;
	}
}

/**
 *	@brief ����@B�̏���
 *
 *	@retval �Ȃ�
 */
void ControlerBSrv(void)
{
	RcvControlerBSrv();
	ContSrv(&response_command);/* ����@���ʂ̏��� */
}

/*
 *===========================================================================================
 *					�����֐�
 *===========================================================================================
 */

/**
 *	@brief ����@B�̐���@A����̃f�[�^�̎�M����
 *
 *	@retval �Ȃ�
 */
static void RcvControlerBSrv(void) {
	char str[256];
	char str2[20];
	unsigned char contb_rcv_buf[RCV_BUFF_SIZE];

	while(!empty(CONTROLER_QUEUE)) {
		unsigned char d = peek(CONTROLER_QUEUE, 0);/* �擪�f�[�^�͂O���H */
		if ( d != 0) {
			dequeue(CONTROLER_QUEUE);
			continue;
		}
		if (lenqueue(CONTROLER_QUEUE) >= 10) {
			int contb_wait_count = peek(CONTROLER_QUEUE, 9) + 10;
			if ((contb_wait_count != 21) && (contb_wait_count != 28)){
				dequeue(CONTROLER_QUEUE);
			} else {
				//printf("%d\n",lenqueue(CONTROLER_QUEUE));
				if (lenqueue(CONTROLER_QUEUE) >= contb_wait_count) {
				//printf("X");
					/* �f�[�^�p�P�b�g��M */
					int i;
					for(i = 0 ; i < contb_wait_count; i++) {
						contb_rcv_buf[i] = dequeue(CONTROLER_QUEUE);/* ��M�f�[�^���o�� */
					}
					CONTROL_HEADER *h =(CONTROL_HEADER *)contb_rcv_buf;
					if (h->dst == 0) {/* ���񐧌�w�� */
						BROADCAST_COMMAND * rp = (BROADCAST_COMMAND *) contb_rcv_buf;
						int bcc = rp->h.s_no;/* Bcc���o�� */
						rp->h.s_no = 0;
						int cbcc = CalcRealBcc(contb_rcv_buf,sizeof(BROADCAST_COMMAND));
						cbcc &= 0xff;
						if ( bcc == cbcc) {
//						if ( 1 ) {
							if ( (rp->h.sub_adr & cmp_my_linkage) != 0 ) {/* kaji20170407 linkage(sub_adr)�Ɏ��ǂ��܂܂�Ă��邩���� */
								memmove(&broadcast_command, contb_rcv_buf, sizeof(BROADCAST_COMMAND));
								BROADCAST_COMMAND *com = (BROADCAST_COMMAND *)contb_rcv_buf;
								BroadcastCommand(com);/* ����w�ߎ�M���� */
							}
							else {
								sprintf(str, "RcvControlerBSrv���񐧌�w��(linkage�s��v�̂��ߏ������� %X, %X)", rp->h.sub_adr, cmp_my_linkage);/* kaji20170407 */
								DebugPrint("", str, 2);
							}
						} else {
							sprintf(str, "RcvControlerBSrv���񐧌�w�� Bcc�G���[ %d,%X<->%X",h->info,bcc, cbcc);
							DebugPrint("", str, 2);
							str[0] = '\0';
							for ( i = 0; i < sizeof(BROADCAST_COMMAND);i++) {
								sprintf(str2,"%02X ",contb_rcv_buf[i]);
								strcat(str, str2);
							}
							DebugPrint("", str, 2);
						}
					} else {
						if (h->info == 1) {/* �ʏ퐧��w�� */
							if (h->dst == my_tanmatsu_type) {
							//if (1) {
								NORMAL_COMMAND * rp = (NORMAL_COMMAND *) contb_rcv_buf;
								int bcc = rp->h.s_no;/* Bcc���o�� */
								rp->h.s_no = 0;
								int cbcc = CalcRealBcc(contb_rcv_buf,sizeof(NORMAL_COMMAND));
								cbcc &= 0xff;
								if ( bcc == cbcc) {
//								if ( 1) {
									memmove(&normal_command, contb_rcv_buf, sizeof(NORMAL_COMMAND));
									NORMAL_COMMAND *com = (NORMAL_COMMAND *)contb_rcv_buf;
									NormalCommand(com);/* �ʏ�w�ߎ�M���� */
									normal_command_timer = 0;/* �ʏ�w�ߎ�M�҂��^�C�} */
								} else {
									sprintf(str, "RcvControlerBSrv�ʏ퐧��w�� Bcc�G���[ %d,%X<->%X",h->info,bcc, cbcc);
									DebugPrint("", str, 2);
									str[0] = '\0';
									for ( i = 0; i < sizeof(NORMAL_COMMAND);i++) {
										sprintf(str2,"%02X ",contb_rcv_buf[i]);
										strcat(str, str2);
									}
									DebugPrint("", str, 2);
								}
							} 
						} else {
							//��M�G���[
							sprintf(str, "RcvControlerBSrv ����ʃG���[ %02X",h->info);
							DebugPrint("", str, 2);
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
 *	@brief �ʏ�w�ߎ�M����
 *
 *	@retval �Ȃ�
 */
static void NormalCommand(NORMAL_COMMAND *com) {

	char str[256];
	char str2[256];
	
	str2[0] = '\0';
	if (com->light_command.issei != 0) {
		/* ������Ďw�ߗL��M */
		if (com->light_command.choukou_iri != 0) {
			/* �����������i��j */
			sprintf(str2, " ������������j");
			//now_status.cds_status = 1; /* CDS��� */
			now_status.cds_status = 0; /* CDS��� */
			ChoukouWrite(0);/* �����������o�͏��� */
			response_command.response.byte7.choukou_iri =1;/* �������i���j */
			response_command.response.byte7.choukou_kiri=0;/* �����؁i��j */
		} else if (com->light_command.choukou_kiri != 0) {
			/* ���������؁i���j */
			sprintf(str2, " ���������؍��j");
			//now_status.cds_status = 0; /* CDS��� */
			now_status.cds_status = 1; /* CDS��� */
			ChoukouWrite(1);/* ���������؏o�͏��� */
			response_command.response.byte7.choukou_iri =0;/* �������i���j */
			response_command.response.byte7.choukou_kiri=1;/* �����؁i��j */
		} else {
			sprintf(str2, " ���� ?????");
		}
		ByouWrite(now_status.status);//20170128 yamazaki
		ToukaWrite(my_tanmatsu_type);//20170129 yamazaki
	}
	sprintf(str, "�ʏ퐧��w�ߎ�M %d %d %02X,%02X,%02X", com->h.dst, my_tanmatsu_type
		, com->command.byte&0xf
		, com->light_command.byte&0xF
		, com->status.byte&0xf);
	strcat(str, str2);
	DebugPrint("NormalCommand", str, 4);
//yamazaki20170217 kaji20170216 ��
	if ((now_status.mode == MODE_REMOTE) && (com->command.shudou == 0)){
		/* ����@AB�Ƃ��ɉ��u�̎� */
		if (com->command.teishi != 0) {/* kaji20170301 */
			EventRequest(MONITOR_REQUEST);/* �^�p��~�������N�G�X�g kaji20170301 */
			DebugPrint("NormalCommand","�^�p��~���N�G�X�g��M", 1);/* kaji20170301 */
		} else if (com->command.tuujou != 0) {
			if (now_status.status != STATUS_P1) {
				EventRequest(TUUJOU_REQUEST);/* ����@B�̒ʏ탊�N�G�X�g */
				DebugPrint("NormalCommand","�ʏ탊�N�G�X�g��M", 1);
			}
		} else if (com->command.issou != 0) {
			if (now_status.status != STATUS_P2) {
				EventRequest(ISSOU_REQUEST);/* ����@B�̈�|���N�G�X�g */
				DebugPrint("NormalCommand","��|���N�G�X�g��M", 1);
			}
		} else if (com->command.henni != 0) {
			if (now_status.status != STATUS_P3) {
				EventRequest(HENNI_REQUEST);/* ����@B�̕ψڃ��N�G�X�g */
				DebugPrint("NormalCommand","�ψڃ��N�G�X�g��M", 1);
			}
		} else if (com->command.fail != 0) {
			/* �t�F�C����M(�����肱�ڂ��t�H���[) */
			if (now_status.status != STATUS_FAIL) {
				//20170225 EventRequest(FAIL_REQUEST);/* �ُ탊�N�G�X�g */
				EventRequest(CONTB_FAIL_REQUEST);/* �ُ탊�N�G�X�g */
				DebugPrint("NormalCommand","�t�F�C�����N�G�X�g��M", 1);
			}
		} else { /* 20170217 */
			/* ���u��M(�����肱�ڂ��t�H���[) */
			if (now_status.status == STATUS_FAIL) {
				EventRequest(REMOTE_REQUEST);/* ���u���N�G�X�g */
				DebugPrint("NormalCommand","���u���N�G�X�g��M", 1);
			}
		}
	}
//yamazaki20170217 kaji20170216 ��
// kaji20170301 ��
	else if (now_status.mode == MODE_MONITOR) {
		if (com->command.teishi == 0) {
			EventRequest(MONITOR_RELEASE_REQUEST);/* �^�p��~�������N�G�X�g */
			DebugPrint("NormalCommand","�^�p��~������Ԏ�M", 1);
		}
	}
// �S���蓮�`�S�������@�̎�肱�ڂ��t�H���[���A�܂��o���Ă��Ȃ�
	else if (now_status.mode == MODE_MANUAL) {
//now_status.manual_status = 0;/* �蓮��� 0:SW�ɂ��蓮,1:�^�pPC����̎蓮�ɂ�� */
	}
// kaji20170301 ��
	
	if (com->light_command.time_req != 0) {
		/* �����C���v����M */
		SetTime(&(com->t));/* �����ݒ菈�� */
		SaveRTC(&(com->t));/* �s�����pRTC�ɏ������� */
	}
	SetMode(&now_status, &response_command);/* ���݂̃��[�h���Z�b�g */
	response_command.h.src = com->h.dst;
	if (now_status.schedule == 0) {
		/* �X�P�W���[���o�^�˗����M */
		DebugPrint("NormalCommand","�X�P�W���[���o�^�˗����M", 1);
		response_command.response.byte7.schedule_req = 1;
	} else {
		response_command.response.byte7.schedule_req = 0;
	}
	if (now_status.time_req == 0) {
		/* �����C���˗����M */
		DebugPrint("NormalCommand","�����C���˗����M", 1);
		response_command.response.byte7.time_req = 1;
	} else {
		response_command.response.byte7.time_req = 0;
	}
	int diff_time = CheckDiffTime(&(com->t));
	//printf("***diff_time=%d\n",diff_time);
	if (diff_time > TIME_ERROR_VAL) {
		/* �����G���[�ƂȂ鎞�Ԃ̍�(�b) */
		now_status.time_status = 1;
		now_status.schedule = 0;/* 20170224 ����ŃX�P�W���[���o�^�v���𑗐M���� */
		now_status.time_req = 0;/* 20170224 ����Ŏ����ݒ�v���𑗐M���� */
	} else {
		now_status.time_status = 0;
	}
	memmove(&response_command_send_buffer, &response_command, sizeof(RESPONSE_COMMAND));
	//if (now_status.gps_status != 0) {
	//	/* ���v�ُ� */
	//	response_command_send_buffer.response.byte2.tanmatu_error = 1;/* �[������@�ُ� */
	//}
	
	sprintf(str, "RcvContolerSrv ���䉞�����M mode=%02X,%02X,%02X,%02X"
		,response_command_send_buffer.response.status[0]
		,response_command_send_buffer.response.status[1]
		,response_command_send_buffer.response.status[2]
		,response_command_send_buffer.response.status[3]
	);
	DebugPrint("NormalCommand ", str, 4);
	SendResponse( );
}
		
/* �Ď������̑��M���� */
static void SendResponse(void) {
	int bcc;
	SetNowTime(&response_command_send_buffer.t);/* �����Z�b�g */
	response_command_send_buffer.h.s_no = 0;/* �ʔ� 00H�Œ� */
	bcc = CalcRealBcc((char *)&response_command_send_buffer,sizeof(RESPONSE_COMMAND));
	response_command_send_buffer.h.s_no = bcc;/* 20170305 Bcc��ʔԂɃZ�b�g���� */
	SendCom1(hComm1, (unsigned char *)&response_command_send_buffer, sizeof(RESPONSE_COMMAND));
}
/**
 *	@brief ���񐧌�w�߂̕\������
 *
 *	@retval �Ȃ�
 */
void BroadcastDisp ( void )
{
	BROADCAST_COMMAND *p;
	p = &broadcast_command;
	printf("���������񐧌�w�߁�����\n");
	printf("���� %02X%02X/%02X/%02X %02X:%02X:%02X\n",p->t.year[0]
											,p->t.year[1]
											,p->t.month
											,p->t.day
											,p->t.hour
											,p->t.min
											,p->t.sec);
	printf("�x��(DB7 1)�A�j�� %02X\n",p->t.holiday_week.byte);
	printf("����w�� %02X\n",p->command.byte);
	printf("�����w�� %02X\n",p->light_command.byte);
	printf("�[������@�i�T�j��� %02X\n",p->status.byte);
	printf("�J�n���� %02X:%02X �w�� %02X\n",p->schedule.start_time[0]
										,p->schedule.start_time[1]
										,p->schedule.start_command);
	printf("�I������ %02X:%02X �w�� %02X\n",p->schedule.end_time[0]
										,p->schedule.end_time[1]
										,p->schedule.end_command);
	printf("�I�t�Z�b�g�^�C�} %02X0Sec\n",p->schedule.offset_timer);
}
/**
 *	@brief �ʏ퐧��w�߂̕\������
 *
 *	@retval �Ȃ�
 */
void NormalDisp ( void )
{
	NORMAL_COMMAND *p;
	p = &normal_command ;
	printf("�������ʏ퐧��w�߁�����\n");
	printf("���� %02X%02X/%02X/%02X %02X:%02X:%02X\n",p->t.year[0]
											,p->t.year[1]
											,p->t.month
											,p->t.day
											,p->t.hour
											,p->t.min
											,p->t.sec);
	printf("�x��(DB7 1)�A�j�� %02X\n",p->t.holiday_week.byte);
	printf("����w�� %02X\n",p->command.byte);
	printf("�����w�� %02X\n",p->light_command.byte);
	printf("�[������@�i�T�j��� %02X\n",p->status.byte);
}
/**
 *	@brief �Ď������̕\������
 *
 *	@retval �Ȃ�
 */
void ResponseDisp ( void )
{
	RESPONSE_COMMAND *p;
	p = &response_command_send_buffer ;
	printf("�������Ď����� ������\n");
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

