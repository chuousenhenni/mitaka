/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	define.h
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

#ifndef	___DEFINE_H___
#define	___DEFINE_H___

#ifndef windows	
typedef int HANDLE ; //yamazaki
typedef long DWORD ; //yamazaki
#endif

/* �萔 */

/* ����@�`�̃A�h���X */
#define CONTA_ADDRESS  (5)

/*
 �ϕ\���ɕ\������p�^�[��
 IO���͂̃X�e�[�^�X�l
*/
				/* S1	S2	S3 */
#define P1  (5)	/* ON	OFF	ON */
#define P2  (6)	/* OFF	ON	ON */
#define P3  (7)	/* ON	ON	ON */

/*�f�B�t�H���g�l */

#define CONTROLER_MAX (8) /* ����@�T�� + ����@�U�� */
#define DISPLAY_BOARD_MAX (8) /* �P����@������̉ϕW���̐� */

#define DEFAULT_START_TIME (7 * 60 * 60 ) /* �ψڊJ�n����(�b�Őݒ�) */
#define DEFAULT_END_TIME (9 * 60 * 60) /* �ψڏI������(�b�Őݒ�) */
#define DEFAULT_TIME_CORRECTION_TIME (0 * 60 + 0) /* �����C������ */
#define DEFAULT_TIME_CORRECTION_REQUEST_TIME (0) /* �����C���v������ */
#define DEFAULT_HOLIDAY_DISPLAY_PATTERN (0) /* ���E�x�\���p�^�[�� P3 */
#define DEFAULT_HOLIDAY_MODE (0) /* �Ȃ� */
#define DEFAULT_HOLIDAY_START_TIME (0) /* �Ȃ��@���E�x�J�n���� */
#define DEFAULT_HOLIDAY_END_TIME (0) /* �Ȃ��@���E�x�I������ */
#define DEFAULT_SATURDAY_MODE (1) /* �ψڂ���i���`���Ɠ����j */
#define DEFAULT_HOLIDAY_MODE (0) /* �ψڂȂ��i�ʏ�j */
#define DEFAULT_FAIL_PATTERN (P2) /* �ϕ\���� P2 */
#define DEFAULT_OFFSET_TIMER (120) /* �I�t�Z�b�g�^�C�} */
#define DEFAULT_LINKAGE (0x11111111) /* �A���ݒ� */

#define RESPONSE_INTERVAL_TIME_VAL (250)	/* ����@A����̗v�����琧��@B����̉�����M�m�F�܂ł̍ŏ��҂�����(ms) */
#define RESPONSE_TIMEOUT_VAL (500)			/* ����@A����̗v�����琧��@B����̉�����M�܂ł̃^�C���A�E�g�l(ms) */
//20170305 #define MUSEN_TIMEOUT_VAL (30) /* �����ʐM�̎�M�G���[�^�C���A�E�g�l(�b)  kaji20170303 60��30 */
#define MUSEN_TIMEOUT_VAL (60) /* �����ʐM�̎�M�G���[�^�C���A�E�g�l(�b)  kaji20170305c 60�b */

/* -------------------- */

/* �����ُ�D��� */
#define TIME_ERROR_VAL (10) /* �����G���[�ƂȂ鎞�Ԃ̍�(�b) */

//#define UNMATCH_TIMEOUT_VALUE (30*1000)
#define UNMATCH_TIMEOUT_VALUE (50*1000)		/* �ϕ\������̃^�C���A�E�g�l[ms] */

#define BROADCAST_RETRY_VAL (3)		/* kaji20170308 ����w�߂̕����񑗐M�� */

#define PREAMBLE_PTN (0x55aa)		/* kaji20170308 �v���A���u���p�^�[�� */


/* �����ԓd���̃w�b�_�[�f�[�^ */
typedef struct {
	unsigned char no;/* �K�i�ԍ� 00H�Œ� */
	unsigned char dst;/* ����A�h���X 01H�Œ�i���g�p�j */
	unsigned char src;/* ���M���A�h���X  01H�Œ� */
	unsigned char sub_adr;/* �T�u�A�h���X 00H�Œ� */
	unsigned char priority;/* �D�惌�x�� 02H�Œ� */
	unsigned char s_no;/* �ʔ� 00H�Œ� */
	unsigned char contoroler_no;/* �[����� 19H */
	unsigned char info;/* ����� 01H */
	unsigned char div_no;/* �����ԍ� 81H */
	unsigned char length;/* �f�[�^�� */
} CONTROL_HEADER;

/* �������̍\���� */
typedef struct {
	unsigned char year[2];/* BCD �N */
	unsigned char month;/* BCD �� */
	unsigned char day;/* BCD �� */
	unsigned char hour;/* BCD �� */
	unsigned char min;/* BCD �� */
	unsigned char sec;/* BCD �b */
	union {
		unsigned char byte;/* �x��(DB7 1)�A�j�� */
		struct {
			char week:7;/* ����7�r�b�g������� */
			char holiday:1;/* ���1�r�b�g */
		};
	} holiday_week;
} TIME_INFO;

/* BIN�`���̎������̍\���� */
typedef struct {
	int year;/* �N */
	int month;/* �� */
	int day;/* �� */
	int hour;/* �� */
	int min;/* �� */
	int sec;/* �b */
	int week;/*  �T(1:��,2:��...7:�y)*/
} TIME_INFO_BIN;

typedef struct {
	union {
		unsigned char status[6];/* ����w�� */
		struct {
			union {
				unsigned char byte;/*  */
				struct {
					char tuujou:1;/* �ʏ� ����1�r�b�g������� */
					char issou:1;/* ��| */
					char henni:1;/* �ψ� */
					char teishi:1;/* �^�p��~ */
					char test:1;/* �`���e�X�g */
					char fail:1;/* �t�F�C���w�� */
					char tanmatu_shudou:1;/* �[���蓮 */
					char tanni_shudou:1;/* �P�ʎ蓮 ��ʃr�b�g */
				};
			} byte1;/* ��T�o�C�g */
			union {
				unsigned char byte;/*  */
				struct {
					char jimaku:1;/* �����ړ��� ����1�r�b�g������� */
					char yobi1:1;/* �\�� */
					char tanmatu_error:1;/* �[������@�ُ� */
					char yobi2:1;/* �\�� */
					char kiten_error:1;/* �N�_�ē��W���ُ� */
					//char yobi3:1;/* �\�� */
					char musen_error:1;/* �����ُ� */
		
					char pc_tuushin_error:1;/* �^�p�Ǘ�PC�ԒʐM�ُ� */
					char moniter_tuushin_error:1;/* �Ď��ՊԒʐM�ُ� ��ʃr�b�g */
				};
			} byte2; /* ��Q�o�C�g */
			union {
				unsigned char byte;/*  */
				struct {
					char board_error1:1;/* �W���łP�ُ� */
					char board_error2:1;/* �W���łQ�ُ� */
					char board_error3:1;/* �W���łR�ُ� */
					char board_error4:1;/* �W���łS�ُ� */
					char board_error5:1;/* �W���łT�ُ� */
					char board_error6:1;/* �W���łU�ُ� */
					char board_error7:1;/* �W���łV�ُ� */
					char board_error8:1;/* �W���łW�ُ� */
				};
			} byte3; /* ��R�o�C�g */
			union {
				unsigned char byte;/*  */
				struct {
					char yobi:2;/* �\�� */
					char byou1_error:1;/* ���H�����e1�ُ� */
					char byou2_error:1;/* ���H�����e2�ُ� */
					char yob2:4;/* �\�� */
				};
			} byte4; /* ��S�o�C�g */
			union {
				unsigned char byte;/*  */
			} byte5; /* ��T�o�C�g */
			union {
				unsigned char byte;/*  */
			} byte6; /* ��U�o�C�g */
		};
	} response;/* �Ď��������e */
} RESPONSE;

/* �Ď��ՊԂւ̓d�� ����w�߂̓d���t�H�[�}�b�g */
typedef struct {
	CONTROL_HEADER h;/* �w�b�_�[ */
//	unsigned char reserved1[6];/* �\�� */
	TIME_INFO t;/* ���� */
	union {
		RESPONSE responce[8];
		unsigned char command[8][6];/* ����w�ߓ��e */
	};
	//	unsigned char reserved2[10];/* �\�� */
} MONITOR_COMMAND;

/* �Ď��ՊԂւ̓d�� �^�p�w�߂̓d���t�H�[�}�b�g */
typedef struct {
	CONTROL_HEADER h;/* �w�b�_�[ */
//	unsigned char reserved1[6];/* �\�� */
	TIME_INFO t;/* ���� */
	unsigned char  command;/* ����w�ߓ��e */
//	unsigned char reserved2[10];/* �\�� */
} MONITOR_OPERATE_COMMAND;


/* �����ԓd�� ����w�߂̓d���t�H�[�}�b�g */
typedef struct {
	CONTROL_HEADER h;/* �w�b�_�[ */
	TIME_INFO t;/* ���� */
	union {
		unsigned char byte;/*  */
		struct {
			char tuujou:1;/* �ʏ� ����1�r�b�g������� */
			char issou:1;/* ��| */
			char henni:1;/* �ψ� */
//			char yobi1:1;/* �Ȃ� */
			char teishi:1;/* �^�p��~ */
			char test:1;/* �`���e�X�g */
			char fail:1;/* �t�F�C���w�� */
			char yobi2:1;/* �\�� �^�p��~���� */
			char shudou:1;/* �蓮 ��ʃr�b�g */
		};
	} command;/* ����w�� */
	union {
		unsigned char byte;/*  */
		struct {
			char issei:1;/* ������Ďw�ߗL ����1�r�b�g */
			char choukou_iri:1;/* �����������i��j */
			char choukou_kiri:1;/* ���������؁i���j */
			char time_req:1;/* �����C���v�� */
			char sreq:1;/* �X�P�W���[���o�^�v�� */
			char rendou_req:1;/* �A���ݒ�v�� 20170320 */
			char yobi:2;/* �蓮 ��ʃr�b�g */
		};
	} light_command;/* �����w�� */
	union {
		unsigned char byte;/*  */
		struct {
			char monitor_error:1;/* �ΊĎ��Փ`���ُ� ����1�r�b�g */
			char yobi:7;/* �蓮 ��ʃr�b�g */
		};
	} status;/* �[������@�i�T�j��� */
	struct {
		unsigned char start_time[2];/* �J�n���� */
		unsigned char start_command;/* ����w�� �J�n���\������ */
		unsigned char end_time[2];/* �I������ */
		unsigned char end_command;/* ����w�� �I�����\������ */
		unsigned char offset_timer;/* �I�t�Z�b�g�^�C�}�iBCD) */
	} schedule;
} BROADCAST_COMMAND;

/* �����i�U�j�ւ̓d�� �ʏ�w�߂̓d���t�H�[�}�b�g */
typedef struct {
	CONTROL_HEADER h;/* �w�b�_�[ */
	TIME_INFO t;/* ���� */
	union {
		unsigned char byte;/*  */
		struct {
			char tuujou:1;/* �ʏ� ����1�r�b�g������� */
			char issou:1;/* ��| */
			char henni:1;/* �ψ� */
//			char yobi1:1;/* �Ȃ� */
			char teishi:1;/* �^�p��~ */
			char test:1;/* �`���e�X�g */
			char fail:1;/* �t�F�C���w�� */
			char yobi2:1;/* �\�� �^�p��~���� */
			char shudou:1;/* �蓮 ��ʃr�b�g */
		};
	} command;/* ����w�� */
	union {
		unsigned char byte;/*  */
		struct {
			char issei:1;/* ������Ďw�ߗL ����1�r�b�g */
			char choukou_iri:1;/* �����������i��j */
			char choukou_kiri:1;/* ���������؁i���j */
			char time_req:1;/* �����C���v�� */
			char yobi:4;/* �蓮 ��ʃr�b�g */
		};
	} light_command;/* �����w�� */
	union {
		unsigned char byte;/*  */
		struct {
			char monitor_error:1;/* �ΊĎ��Փ`���ُ� ����1�r�b�g */
			char yobi:7;/* �蓮 ��ʃr�b�g */
		};
	} status;/* �[������@�i�T�j��� */
} NORMAL_COMMAND;

/* �����i�U�j����̓d�� �Ď������̓d���t�H�[�}�b�g */
typedef struct {
	CONTROL_HEADER h;/* �w�b�_�[ */
	TIME_INFO t;/* ���� */
	union {
		unsigned char status[7];/* ����w�� */
		struct {
			union {
				unsigned char byte;/*  */
				struct {
					char tuujou:1;/* �ʏ� ����1�r�b�g������� */
					char issou:1;/* ��| */
					char henni:1;/* �ψ� */
					char teishi:1;/* �^�p��~ */
					char test:1;/* �`���e�X�g */
					char fail:1;/* �t�F�C���w�� */
					char tanmatu_shudou:1;/* �[���蓮 */
					char tanni_shudou:1;/* �P�ʎ蓮 ��ʃr�b�g */
				};
			} byte1;/* ��T�o�C�g */
			union {
				unsigned char byte;/*  */
				struct {
					char jimaku:1;/* �����ړ��� ����1�r�b�g������� */
					char hoshu_status:1;/* 1:�ێ�/0:�ʏ� */
					char tanmatu_error:1;/* �[������@�ُ� */
					char yobi2:1;/* �\�� */
					char kiten_error:1;/* �N�_�ē��W���ُ� */
					//char yobi3:1;/* �\�� */
					char musen_error:1;/* �����ُ� */
		
					char pc_tuushin_error:1;/* �^�p�Ǘ�PC�ʐM�ُ� */
					char moniter_tuushin_error:1;/* �Ď��ՊԒʐM�ُ� ��ʃr�b�g */
				};
			} byte2; /* ��Q�o�C�g */
			union {
				unsigned char byte;/*  */
				struct {
					char board_error1:1;/* �W���łP�ُ� */
					char board_error2:1;/* �W���łQ�ُ� */
					char board_error3:1;/* �W���łR�ُ� */
					char board_error4:1;/* �W���łS�ُ� */
					char board_error5:1;/* �W���łT�ُ� */
					char board_error6:1;/* �W���łU�ُ� */
					char board_error7:1;/* �W���łV�ُ� */
					char board_error8:1;/* �W���łW�ُ� */
				};
			} byte3; /* ��R�o�C�g */
			union {
				unsigned char byte;/*  */
				struct {
					char yobi:2;/* �\�� */
					char byou1_error:1;/* ���H�����e1�ُ� */
					char byou2_error:1;/* ���H�����e2�ُ� */
					char yob2:4;/* �\�� */
				};
			} byte4; /* ��S�o�C�g */
			union {
				unsigned char byte;/*  */
			} byte5; /* ��T�o�C�g */
			union {
				unsigned char byte;/*  */
			} byte6; /* ��U�o�C�g */
			union {
				unsigned char byte;/*  */
				struct {
					char yobi1:1;/* �\�� ����1�r�b�g������� */
					char choukou_iri:1;/* �������i���j */
					char choukou_kiri:1;/* �����؁i��j */
					char time_req:1;/* �����C���˗� */
					char schedule_req:1;/* �X�P�W���[���o�^�˗� */
					char yobi2:1;/* �\�� */
					char schedule_disp:1;/* �X�P�W���[���\���� */
					char control_disp:1;/* ����w�ߕ\���� ��ʃr�b�g */
				};
			} byte7;/* ��V�o�C�g */
		};
	} response;/* �Ď��������e */
} RESPONSE_COMMAND;


/* �ψڋx�~���Ǘ��\���� */
typedef struct {
	char holiday_count;
	char kyuushi_count;
	struct {
		char month;
		char day;
		char type;
	} holiday[40];
	struct {
		char month;
		char day;
		char type;
	} kyuushi[20];
	int bcc;
} HOLIDAY_DATA;

/* �����A�ւ̓d�� �ψڋx�~���ʒm�̓d���t�H�[�}�b�g */
typedef struct {
	CONTROL_HEADER h;/* �w�b�_�[ */
	char d[182];
} HOLIDAY_COMMAND;

/* �ݒ�l�\���� */
typedef struct {
	int start_time; /* �ψڊJ�n���� */
	int end_time; /* �ψڏI������ */
	int time_correction_time; /* �����C������ */
	int time_correction_request_time; /* �����C���v������ */
	int holiday_pattern; /* ���E�x�\���p�^�[�� P3 */
	int holiday_start_time; /* �Ȃ��@���E�x�J�n���� */
	int holiday_end_time; /* �Ȃ��@���E�x�I������ */
	int saturday_mode; /* �y�j�����[�h�@�ψڂ���i���`���Ɠ����j */
	int holiday_mode; /* �x�����[�h�@�ψڂȂ��i�ʏ�j */
	int fail_pattern; /* �ϕ\���� P2 �t�@�C���p�^�[�� */
	int offset_timer; /* �I�t�Z�b�g�^�C�} */
	int no_musen_error_flag; /* �����ُ���`�F�b�N�����ۂ� 0:����,1:���Ȃ� */
	int same_nomove_flag; /* �����ꍇ�͉ϕ\���ł�ω������Ȃ����[�h */
	int no_fail_flag; /* �t�F�C���ɂ��Ȃ����[�h */
	int mdmcs_delay_time; /* MDM_CS�o�͂̒x������(ms) */
	int linkage_status; /* �A���ݒ� */
	int no_board_error_flag; /* �W���ňُ���`�F�b�N�����ۂ� 0:����,1:���Ȃ� */
	int no_pc_check_flag; /* �^�p�Ǘ��o�b�ԒʐM�ُ�𔻒���`�F�b�N�����ۂ� 0:����,1:���Ȃ� */
	int debug_flg;/* �f�o�b�O�p�̕\�����s���ꍇ��0�ȊO���Z�b�g���� */
	int response_interval_time_val;/* 20170305 ����@A����̗v�����琧��@B����̉�����M�m�F�܂ł̍ŏ��҂�����(ms) */
	int response_timeout_val;/* 20170305 ����@A����̗v�����琧��@B����̉�����M�܂ł̃^�C���A�E�g�l(ms) */
	int musen_timeout_val;/* 20170305 �����ʐM�̎�M�G���[�^�C���A�E�g�l(�b) */
	int reset_count;/* ���Z�b�g�̂��тɁ{�P */
	int preamble_ptn;/* �v���A���u���p�^�[�����w�肷�� */
	int bcc;/* BCC�l */
} PARAM;

/* ����p�����[�^ */
/*
��d���͂��̒l��ێ����Ă���
*/
typedef struct {
	int mode;/* ���u�A�蓮�A�Ď��Ղ���̎w�� */
	int status;/* �t�F�C���A�ʏ�A��|�A�ψځA���E�x */
	int bef_status;/* ����@B�̃X�e�[�^�X�ϊ�����p */
	int before_fail_status;/* �t�F�C���ɂȂ�O�̏�Ԃ�ێ� */
	int schedule;/* �X�P�W���[���o�^�ς݃t���O */
	int time_req;/* �����ݒ�ς݃t���O */
	int start_time;/* �ʏ큨��|�J�n���� */
	int end_time;/* �ψځ���|�J�n���� */
	int offset_timer;/* �I�t�Z�b�g�^�C�}�l */
	int power_outage_flag;/* ��d�����t���O */
	int power_outage_move_flag;/* ��d�����Ńt�F�C���ֈړ������t���O */
	int test_flag;/* �e�X�g���t���O */
	int board_error;/* �\���ُ픭���t���O */
	int jimaku_ido_flag;/* �����ړ����t���O */
	int hoshu_status; /* 1:�ێ�/0:�ʏ� */
	int keikoutou_status; /* 1:�u������/0:���� */
	int cds_status; /* CDS��� */
	int time_status;/* ������� 0:����,1:�ُ� */
	int gps_status;/* GPS��� 0:����,1:�ُ� */
	int before_gps_status;/* �O���GPS��� 0:����,1:�ُ� */
	int musen_status;/* �����ʐM��� 0:����,1:�ُ� */
	int before_musen_status;/* �O��̖����ʐM��� 0:����,1:�ُ� */
	int byou_status;/* �����e��� 0:����,1:�ُ� */
	int before_byou_status;/* �O��̔����e��� 0:����,1:�ُ� */
	int byou1_status;/* �����e1��� 0:����,1:�ُ� */
	int byou2_status;/* �����e2��� 0:����,1:�ُ� */
	int pc_tuushin_status;/* �^�p�Ǘ�PC�ԒʐM��� 0:����,1:�ُ� */
	int moniter_tuushin_status;/* �Ď��ՊԒʐM��� 0:����,1:�ُ� */
	int manual_status;/* �蓮��� 0:SW�ɂ��蓮,1:�^�pPC����̎蓮�ɂ�� */
	int tanmatu_error;/* �[���G���[��Ԃ�ێ� 0:�G���[����,1:�G���[�L�� */
	int power_outage_flag2;/* ��d�����t���O2 */
	int bcc;/* BCC�l */
} STATUS;


/* ����p�����[�^ */
/*
�\������IO���p�\����
*/
typedef struct {
	int display_board_count;/* �ϕ\���� */
	int light_count;/* �����e�� */
	int out_status[DISPLAY_BOARD_MAX];/* �ϕ\���ւ̏o�̓f�[�^ */
	int in_status[DISPLAY_BOARD_MAX];/* �ϕ\������̓��̓f�[�^ */
	int kiten_annai[DISPLAY_BOARD_MAX];/* �N�_�ē��W�����ǂ�����ێ�����f�[�^ */
	int allowed_pattern_p1[DISPLAY_BOARD_MAX][2];/* P1�̋������p�^�[��(P1�݂̂܂���P1,P2) */
	int allowed_pattern_p2[DISPLAY_BOARD_MAX][2];/* P2�̋������p�^�[��(P1,P2�܂���P1,P3) */
	int allowed_pattern_p3[DISPLAY_BOARD_MAX][2];/* P3�̋������p�^�[��(P3�݂̂܂���P2,P3) */
	int muji_pattern_p1[DISPLAY_BOARD_MAX];/* P1�����n�p�^�[�����ǂ��� */
	int muji_pattern_p2[DISPLAY_BOARD_MAX];/* P2�����n�p�^�[�����ǂ��� */
	int muji_pattern_p3[DISPLAY_BOARD_MAX];/* P3�����n�p�^�[�����ǂ��� */
	
} IO_INFO;

/* �L���[�̔z��ԍ� */
#ifdef windows
#define CONTROLER_QUEUE (0)	/* ����@�ԒʐM uart1 */
#define PC_QUEUE (0)        /* �^�p�Ǘ�PC�ԒʐM uart 2 */
#define MONITOR_QUEUE (0)   /* �Ď��ՊԒʐM uart 3 */
#define CONSOLE_QUEUE (3)   /* �����e�i���XPC�ԒʐM uart4 */
#define IO_QUEUE (4)
#define UART1_QUEUE (0)		/* UART1�̎�M�f�[�^ kaji20170306 */
#define UART3_QUEUE (0)		/* UART3�̎�M�f�[�^ kaji20170310 */
#else
#define CONTROLER_QUEUE (0)	/* ����@�ԒʐM uart1 */
#define PC_QUEUE (1)        /* �^�p�Ǘ�PC�ԒʐM uart 2 */
#define MONITOR_QUEUE (2)   /* �Ď��ՊԒʐM uart 3 */
#define CONSOLE_QUEUE (3)   /* �����e�i���XPC�ԒʐM uart4 */
#define IO_QUEUE (4)
#define UART1_QUEUE (5)		/* UART1�̎�M�f�[�^ kaji20170305 */
#define UART3_QUEUE (6)		/* UART3�̎�M�f�[�^ kaji20170310 */
#endif

#define QUEUE_MAX	7     // �L���[�̌� kaji20170310

/* LED�̏�ԕϐ� */
enum _LedStatus{
	LED_STATUS_OFF = 0,
	LED_STATUS_GREEN,
	LED_STATUS_ORANGE,
	LED_STATUS_RED,
	LED_STATUS_GREEN_TOGGLE,
	LED_STATUS_ORANGE_TOGGLE,
	LED_STATUS_RED_TOGGLE,
};

/* LED�̏�ԕϐ� */
enum _LedStatus_old{
	LED_STATUS_OFF_old = 0,
	LED_STATUS_ON,
	LED_STATUS_TOGGLE
};

#define CONT_LED_MAX_COUNT (16) /* LED����ő吔(���ۂ�9�����r�b�g�ʒu�̂���) */
#define MONITOR_LED_MAX_COUNT (2 * CONTROLER_MAX + 3) /* LED����ő吔(�e�n�_�p�Ɖ^�p��~LED�ƃu�U�[��~LED�ƌx�񃉃��v) */
#define UNYOU_TEISHI_LED (2 * CONTROLER_MAX) /* �^�p��~LED�̔z��|�C���^ */
#define BUZZER_TEISHI_LED (2 * CONTROLER_MAX + 1) /* �u�U�[��~LED�̔z��|�C���^ */
#define ALARM_LAMP_LED (2 * CONTROLER_MAX + 2) /* �x�񃉃��vLED�̔z��|�C���^ */


/* �\����Ԃ̏�ԕϐ� */
enum _DispStatus{
	STATUS_P1 = 1,//�ʏ�
	STATUS_P2,//��|
	STATUS_P3,//�ψ�
	STATUS_HOLIDAY,//���E�x
	STATUS_FAIL,//�t�F�C��
	STATUS_P1P2,//�ʏ킩��̕ω����̈�|
	STATUS_P3P2,//�ψڂ���̕ω����̈�|

};

/* �^�p��Ԃ̏�ԕϐ� */
enum _ModeStatus{
	MODE_REMOTE = 0,/* ���u */
	MODE_MANUAL,/* �蓮 */
	MODE_MONITOR,/* �Ď��Ղ���̎w�� */
};

/* �ϕ\������̏�Ԋ֐� */
enum _board_rotate_state {
	BOARD_ROTATE_READY = 0,
	BOARD_ROTATE_START,
	BOARD_ROTATE_WAIT,
	BOARD_ROTATE_END,
	BOARD_ROTATE_OFFSET_TIMER,
	BOARD_ROTATE_OFFSET_TIMER_END,
	BOARD_ROTATE_IDLE
};


/* �C�x���g�R�[�h */
enum _event{
	REMOTE_REQUEST = 0,			/* ���u���N�G�X�g */
	REMOTE_TUUJOU_REQUEST,		/* �ʏ펞�ԓ��ł̉��u���N�G�X�g */
	REMOTE_HENNI_REQUEST,		/* �ψڎ��ԓ��ł̉��u���N�G�X�g */
	REMOTE_TUUJOU2HENNI_REQUEST,/* ���u�ʏ킩��ψڃ��N�G�X�g */
	REMOTE_HENNI2TUUJOU_REQUEST,/* ���u�ψڂ���ʏ탊�N�G�X�g */
	MANUAL_TUUJOU_REQUEST,		/* �蓮�ʏ탊�N�G�X�g */
	MANUAL_ISSOU_REQUEST,		/* �蓮��|���N�G�X�g */
	MANUAL_HENNI_REQUEST,		/* �蓮�ψڃ��N�G�X�g */
	SW_MANUAL_TUUJOU_REQUEST,	/* SW�蓮�ʏ탊�N�G�X�g */
	SW_MANUAL_ISSOU_REQUEST,	/* SW�蓮��|���N�G�X�g */
	SW_MANUAL_HENNI_REQUEST,	/* SW�蓮�ψڃ��N�G�X�g */
	MANUAL_FAIL_REQUEST,		/* �蓮�t�F�C�����N�G�X�g */
	MONITOR_REQUEST,			/* �^�p��~���N�G�X�g */
	MONITOR_RELEASE_REQUEST,	/* �^�p��~�������N�G�X�g */
	FAIL_REQUEST,				/* �\����,GPS,�����ُ탊�N�G�X�g */
	POWER_OFF_REQUEST,			/* ��d�������N�G�X�g */
	POWER_RECOVER_REQUEST,		/* ��d�������A���N�G�X�g */
	FAIL_RECOVER_REQUEST,		/* �t�F�C���ُ한�A�i���v�AGPS�j���N�G�X�g */
	FAIL2TUUJOU_REQUEST,		/* �t�F�C�����̒ʏ�ւ̕��A���N�G�X�g */
	TUUJOU_REQUEST,				/* ����@B�̒ʏ탊�N�G�X�g */
	ISSOU_REQUEST,				/* ����@B�̈�|���N�G�X�g */
	HENNI_REQUEST,				/* ����@B�̕ψڃ��N�G�X�g */
	CONTB_FAIL_REQUEST,			/* ����@B�̃t�F�C�����N�G�X�g */
};

/* �ω���� */
enum {
	P2_P2 = 0,/* �I�t�Z�b�g�^�C�}���ԑ҂� */
	PX_P1,/* �ʏ�� */
	PX_P2,/* ��|�� */
	PX_P3,/* �ψڂ� */
	PX_FAIL,
	PX_POWEROFF,
	PX_POWER_RECOVER
};

enum {/* ���̓��̏�� */
	TODAY_IS_NORMAL_DAY = 0,
	TODAY_IS_HOLIDAY,
};

/* kaji20170225�� */
enum {/* �t�F�C�����ǂ������Ă��邩�̏�ԕϐ� */
	FAIL_HND_NONE = 0,	/* �t�F�C����ԁ@���� */
	FAIL_HND_FAIL,		/* �t�F�C����� */
	FAIL_HND_P1_PARK	/* �ʏ펞�ɔ��������t�F�C���Ȃ̂ŕω������Ȃ� */
};
/* kaji20170225�� */



#define RCV_BUFF_SIZE (200 + 10) /* ��M�o�b�t�@�[�T�C�Y */

/* LED��� */

#define LED_ON (1)	/* LED�_�� */
#define LED_OFF (0)	/* LED���� */
#define TOGGLE_COUNT_MAX (500) /* 0.5�b */

 /* �A�h���X */
#define FLASH_TOP_ADDRESS (0xE0000000) /* �X�e�[�^�X�i�[�A�h���X */
#define HOLIDAY_ADDRESS (0xE0000000) /* �ψڋx�~���i�[�A�h���X */
#if 1
#define STATUS_ADDRESS (0xE00F8000) /* �X�e�[�^�X�i�[�A�h���X */
#else
#define STATUS_ADDRESS (0xBD1E0000) /* �X�e�[�^�X�i�[�A�h���X */
#endif


#if 1
#define PARAM_ADDRESS  (0xE00FA000) /* �p�����[�^�i�[�A�h���X */
#else
#define PARAM_ADDRESS  (0xBD1F0000) /* �p�����[�^�i�[�A�h���X */
#endif


#endif	/* ___DEFINE_H___ */

