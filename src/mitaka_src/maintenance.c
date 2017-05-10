/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  EXS CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	Console.c
 *	�T�v
 *
 * �R���\�[���̐�����s���B
 * �r�v�P���n�m�ŋN��
 *
 * �L�[���͂���t�A�f�o�b�O�p�̕ύX�A�\���������s��
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
#include "common.h"
#include "cont.h"
#include "contb.h"
#include "queue.h"
#include "version.h"
#include "monitor.h"
#include "io.h"
#include "maintenance.h"					/* �������g�̃w�b�_�͂����ŃC���N���[�h */

/*
 *===========================================================================================
 *					�����萔��`�E�^��`�E�}�N����`
 *===========================================================================================
 */

#define MAX_COMMAND_LENGTH (100)
struct TComandList_ {
    char *fCommand;
    char fLength;
    void (*fAction)(void);
    char *fDescription;
};
typedef struct TComandList_ TComandList;

/*
 *===========================================================================================
 *					�����ϐ���`
 *===========================================================================================
 */

/*
	thread_debug_flg�̐���
	DB0:����M�f�[�^��\������
	DB1:�G���[��\������
	DB2:All Data���O��\������
	DB3:�Z���TID�Ǘ��@�\���O��\������
	
	�֐�DebugPrint�Ƒg�ݍ��킹�Ďg�p
*/
//int debug_flg = 0xf;/* �f�o�b�O�p�̕\�����s���ꍇ��0�ȊO���Z�b�g���� */
int debug_flg_before=0;/* �f�o�b�O�p�̃t���O�ێ��p */
static int count = 0;
static int bs_flg = 0;

/* �L�[���͗���p�o�b�t�@ */
static int hp = 0;
static int hsavep = 0;/* �������Z�[�u����|�C���g */
static int hp_count = 0;/* �����g�[�^���� */
static int key_count = 0;/* ���̃R�}���h���̃L�[���͐� */
#define HP_MAX (10)
static char history[HP_MAX][100];
int swtest;/* SW�e�X�g�p�f�[�^ */

int command_count = 0;
char command [ 20 ][ 20 ];
int bef_ms_timer;

/*
 *===========================================================================================
 *					�O���ϐ���`
 *===========================================================================================
 */

extern int my_tanmatsu_type;
extern int rtc_sync_timer;
extern int board_chatter_timer[0];/* �`���^�h�~�W���łh�n���[�h�^�C�}�[ */

/*
 *===========================================================================================
 *					����	�֐���`
 *===========================================================================================
 */

static char *DebugTimePrint(void);
static void HistoryUp(void);/* ����󂪉����ꂽ���̏��� */
static void HistoryDown(void);/* ����󂪉����ꂽ���̏��� */
static void HistoryInc(void);/* �������o���|�C���g�{�P���� */
static void HistoryDec(void);/* �������o���|�C���g�[�P���� */

static void CommandSrv(char *str);/* �R�}���h��͎��s���� */
static void CommandA(void);/* ���䉞���R�}���h�\������ */
static void CommandHi(void);/* ���͗���\������ */
static void CommandB(void);/* ���񐧌�R�}���h�̕\�� */
static void CommandR(void);/* �e�X�g�p�����_���f�[�^���M���� */
static void CommandN(void);/* �ʏ퐧��R�}���h�̕\�� */
static void CommandMv(void);/* �ϕ\����]���� */
static void CommandMd(void);/* �������_���v���� */
static void CommandDefault(void);/* �p�����[�^�f�B�t�H���g�ݒ菈�� */
static void CommandStatus(void);/* �X�e�[�^�X�\������ */
static void CommandParam(void);/* �p�����[�^�\������ */
static void CommandErase(void);/* �t���b�V���������Z�N�^�[������ */
static void CommandFwd(void);/* Fwd�R�}���h���� */
//static void CommandFws(void);/* Fws�R�}���h���� */
static void CommandOut(void);/* IO�o�͏��� */
static void CommandIn(void);/* IO���͏��� */
static void E2ROM_Write(void);/* EEPROM�������ݏ��� */
static void E2ROM_Read(void);/* EEPROM�������ݏ��� */
static void CommandLed(void);/* BSPLED�̕\������ */
static void CommandTime(void);/* �����ݒ�\������ */
static void CommandI(void);/* �f�o�b�O�p�̕\�����͒l�̐ݒ菈�� */
static void CommandDgsw(void);/* BSP�f�W�X�C�b�`�̓ǂݍ��ݏ��� */
static void CommandSw(void);/* �f�o�b�O�p�̃X�C�b�`�ݒ�\������ */
static void CommandP(void);/* �f�o�b�O�p�̒�d�X�e�[�^�X�ݒ�\������ */
static void CommandMtest(void);/* �ϕ\�����[�^�̐��䏈�� */
static void CommandVersion(void);/* �o�[�W�����\������ */
static void CommandH(void);/* �w���v�\������ */
static void CommandW(void);/* �e�X�g�p�������[�v���� */
static void CommandD(void);/* �f�o�b�O�t���O�؂�ւ����� */
static void CommandCheckStartTime(void);/* ���̓��̏�Ԃ��l�����鏈�� */
static void CommandTest(void);/* ���� */
static void CommandQ(void);/* exit���� */
static TComandList g_command_list[] = {
    {"a", 1, CommandA, "�Ď������_���v"},
    {"hi", 2, CommandHi, "���͗���"},
    {"b", 1, CommandB, "����w�߃_���v"},
    {"r", 1, CommandR, "�e�X�g�p�����_���f�[�^���M"},
    {"n", 1, CommandN, "�ʏ�w�߃_���v"},
    {"mv", 2, CommandMv, "��board���[�^�[�I��"},
    {"md", 2, CommandMd, "�������_���v"},
    {"default", 5, CommandDefault, "�p�����[�^�f�B�t�H���g�ݒ菈��"},
    {"status", 2, CommandStatus, "�X�e�[�^�X����"},
    {"param", 2, CommandParam, "�p�����[�^����"},
    {"erase", 5, CommandErase, "�t���b�V���������Z�N�^�[�C���[�Y����"},
    {"fwd", 3, CommandFwd, "�t���b�V�����������C�g�R�}���h"},
//    {"fws", 3, CommandFws, "�R�}���h"},
    {"out", 1, CommandOut, "IO�o�͏���"},
    {"in", 2, CommandIn, "IO���͏���"},
	{"eew", 3, E2ROM_Write, "EEPROM�������ݏ���"},
	{"eer", 3, E2ROM_Read, "EEPROM�ǂݏo������"},
    {"led", 1, CommandLed, "LED���,BSPLED�̐��䏈��"},
    {"test", 4, CommandTest, "test����"},
    {"time", 1, CommandTime, "�����ݒ菈��"},
    {"i", 1, CommandI, "�f�o�b�O�p�̓��͒l�̐ݒ菈��"},
    {"dgsw", 2, CommandDgsw, "BSP�f�W�X�C�b�`�̓ǂݍ��ݏ���"},
    {"sw", 2, CommandSw, "�f�o�b�O�p�̃X�C�b�`�ݒ�"},
    {"p", 2, CommandP, "�f�o�b�O�p�̒�d�X�e�[�^�X�ݒ菈��"},
    {"mtest", 2, CommandMtest, "�f�o�b�O�p�̉^�p�Ǘ�PC��~�t���O�ݒ�"},
    {"version", 3, CommandVersion, "�o�[�W��������"},
    {"h", 1, CommandH, "�w���v����"},
    {"w", 1, CommandW, "�e�X�g�p�������[�v����"},
    {"d", 1, CommandD, "�f�o�b�O�t���O�؂�ւ�����"},
    {"c", 1, CommandCheckStartTime, "���̓��̏�Ԃ��l�����鏈��"},
    {"q", 1, CommandQ, "exit����"},
};
static unsigned long ToDec(const char str[ ]);/* 16 �i������� 10 �i���ɕϊ����鏈�� */

/*
 *===========================================================================================
 *					�O���[�o���֐���`
 *===========================================================================================
 */

void MaintenanceInit(void );/* �f�o�b�O�p�̃L�[���͂ɂ�鏈���̏��������� */
void MaintenanceSrv(void);/* �f�o�b�O�p�̃L�[���͏��� */
void DebugPrint(char *str1, char *str2, int mask);/* �f�o�b�O�p�̕\������ */

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
 *	@brief �f�o�b�O�p�̃L�[���͂ɂ�鏈���̏���������
 *
 *	@retval �Ȃ�
 */
void MaintenanceInit(void )
{
	/* ���������炩���ߓo�^���Ă��� */
	static char chistory[][30] = {
		"md 0000",
		"md 0002",
		"d",
		"sw",
		"pa",
		"st",
		""
	};
	int i = 0;
	for( i = 0; i< HP_MAX; i++){
		if ((strcmp(chistory[i],"")) == 0 ) {
			break;
		}
		strcpy(history[hsavep],chistory[i]);
		hsavep++;
		hp_count++;
		hp++;
	}
	

}

/**
 *	@brief �f�o�b�O�p�̃L�[���͂ɂ�鏈��
 *
 *	@retval �Ȃ�
 */
char str[100];
int direction_flg=0;
int direction=0;
void  MaintenanceSrv(void)
{
	unsigned char c;
	while(!empty(CONSOLE_QUEUE)) {
		c = peek(CONSOLE_QUEUE, 0);
		if( c == 0xe0) {
			if (lenqueue(CONSOLE_QUEUE) < 2) {
				return;
			}
			//printf("A");
			c = peek(CONSOLE_QUEUE, 1);
			if (c == 0x48) {
				//printf("UP\n");//�����
				HistoryUp();/* �����̏��� */
			} else if (c == 0x50) {
				//printf("DOWN\n");//�����
				HistoryDown();/* �����̏��� */
			}
			c = dequeue(CONSOLE_QUEUE);
			c = dequeue(CONSOLE_QUEUE);
			return;
		} else if( c == 0x1B) {
			/* �e���^�[���̏ꍇ��ESC�V�[�P���X�ƂȂ� */
			/*
			�����1B,5B,41
			������1B,5B,42
			
			*/
			if (lenqueue(CONSOLE_QUEUE) < 3) {
				return;
			}
			if ((peek(CONSOLE_QUEUE, 1) == 0x5B)  && (peek(CONSOLE_QUEUE, 2) == 0x41)){
				//printf("UP\n");//�����
				HistoryUp();/* �����̏��� */
			} else if ((peek(CONSOLE_QUEUE, 1) == 0x5B)  && (peek(CONSOLE_QUEUE, 2) == 0x42)){
				//printf("DOWN\n");//�����
				HistoryDown();/* �����̏��� */
			}
			c = dequeue(CONSOLE_QUEUE);
			c = dequeue(CONSOLE_QUEUE);
			c = dequeue(CONSOLE_QUEUE);
			return;
		} else {
			c = dequeue(CONSOLE_QUEUE);
		}
		//yamazaki 
		putchar(c);
		if ( c==10 || c==13 ) { /* '\r'��'\n'����M����ƕ��������� */
			str[ count ] = '\0';
			count++;
			if (bs_flg != 0) {
				printf("%s\n",str);
			}
			
			if (key_count > 0) {
				key_count = 0;
				strcpy(history[hsavep],str);
				hsavep++;
				if (hsavep >= HP_MAX) {
					hsavep = 0;
				}
				
				hp_count++;
				hp = hsavep;
			}
			CommandSrv(str);
			//printf("[hp=%d,hp_count=%d]\n",hp,hp_count);
			if (direction_flg == 1) {
				direction_flg = 0;
				if (direction==0) {
					HistoryInc();/* �}�C�i�X������Ńv���X���� */
				} else {
					HistoryDec();/* �v���X������Ń}�C�i�X���� */
				}
			}
			//printf("[[hp=%d,hp_count=%d]\n",hp,hp_count);
		} else if ( c == 8 ){/* BS? */
			if (count > 0) {
				putchar(' ');
				putchar('\b');
				bs_flg = 1;
				count--;
			}
		//20170217 } else {
		} else if (isprint(c)) {
			//if (isalnum(c)) {
				str[ count ] = c;
				count++;
				key_count++;
			//}
		}
		if (count > 100) {
			//yamazaki printf("MaintenanceSrv buffer over flow 1\n");
			count--;
		}
	}
}

/**
 *	@brief ����󂪉����ꂽ���̏���
 *
 *	@retval �Ȃ�
 */
static void HistoryUp(void)
{
	int i;
	HistoryDec();/* �������o���|�C���g�[�P���� */
	strcpy(str,history[hp]);
	for(i=0;i<count;i++){
		putchar('\b');
	}
	count = strlen(str);
	printf("%s            \b\b\b\b\b\b\b\b\b\b\b\b",str);
	direction_flg = 1;
	direction = 0;
}

/**
 *	@brief ����󂪉����ꂽ���̏���
 *
 *	@retval �Ȃ�
 */
static void HistoryDown(void)
{
	int i;
	HistoryInc();/* �������o���|�C���g�{�P���� */
	strcpy(str,history[hp]);
	for(i=0;i<count;i++){
		putchar('\b');
	}
	count = strlen(str);
	printf("%s            \b\b\b\b\b\b\b\b\b\b\b\b",str);
	direction_flg = 1;
	direction = 1;
}

/**
 *	@brief �������o���|�C���g�{�P����
 *
 *	@retval �Ȃ�
 */
static void HistoryInc(void)
{
	hp++;
	int hp_max = (hp_count < HP_MAX ? hp_count:HP_MAX);
	if (hp >= hp_max) {
		hp = 0;
	}
}

/**
 *	@brief �������o���|�C���g�[�P����
 *
 *	@retval �Ȃ�
 */
static void HistoryDec(void)
{
	hp--;
	if (hp < 0) {
		int hp_max = (hp_count < HP_MAX ? hp_count:HP_MAX);
		hp = hp_max-1 ;
	}
}

/**
 *	@brief �R�}���h��͎��s����
 *
 *	@retval �Ȃ�
 */
static void  CommandSrv(char *str)
{
#define ARRAYSIZE(name) (sizeof(name)/sizeof(name[0]))
	int i;
	int len;
	int ret;
	printf("command=%s\n", str);
	command_count = 0;
	char *tok = strtok( str, " " );
	while( tok != NULL ){
		strcpy(command[command_count], tok );
		command_count++;
		if (command_count > 20) {
			printf("MaintenanceSrv buffer over flow 2\n");
			command_count--;
		}
		tok = strtok( NULL, " " );
	}
	count = 0;
	len = ARRAYSIZE(g_command_list);
	for (i = 0; i < len; i++) {
		ret = strncmp(g_command_list[i].fCommand, command[0], g_command_list[i].fLength);
		if (ret == 0) {
			(*g_command_list[i].fAction)();
			break;
		}
	}
	printf(">>");
	return ;
}

/**
 * @brief A�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandA(void)
{
	ResponseDisp( );/* ���M�p */
}

/**
 * @brief Hi�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandHi(void)
{
	int i;
	printf("hp=%d,hp_count=%d\n",hp,hp_count);
	for(i=0;i<HP_MAX;i++){
		printf("%d:%s\n",i+1,history[i]);
	}
}

/**
 * @brief B�R�}���h����
 *
 * @return �Ȃ� */

static void CommandB(void)
{
	/* ���񐧌�R�}���h�̕\�� */
	BroadcastDisp();/* ���M�p */
}

/**
 * @brief R�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandR(void)
{
// kaji20170221��
#ifdef windows	
	extern HANDLE hComm1;       /* �V���A���|�[�g�̃n���h�� */
	extern void SendCom1(HANDLE h, unsigned char *p, int size);
	char random[100];
	int i;
	srand((unsigned)time(NULL));
	for(i=0;i<100;i++){  random[i] = rand();  }
	printf("send random data\n");
	SendCom1(hComm1, (unsigned char *)random, sizeof(random));
#endif
// kaji20170221��
}

/**
 * @brief N�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandN(void)
{
	/* �ʏ퐧��R�}���h�̕\�� */
	NormalDisp( );/* ���M�p */
}

/**
 * @brief Mv�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandMv(void)
{
#ifndef windows
	/* �ϕ\���Ő���R�}���h�̕\�� */
	extern int ms_timer;/* ms�^�C�} */
	extern int sec_timer;/* �b�^�C�} */

	int no = 1;
	int d = 0;
	if (command_count >= 2) {
		no = atoi(command[1]);
		if ((no<=0) || (no>8)) {
			printf("illeagal board no%d\n",no);
			return;
		}
	}
	//yamazakiBoardWrite(no, 3);/* �n�m�R�}���h */
	BoardWrite(no-1, 2);/* �n�m�R�}���h */
	printf("MOVE NO. %d UNTIL PUSH ANY KEY\n", no);
	board_chatter_timer[no-1] = 0;/* �`���^�h�~�W���łh�n���[�h�^�C�}�[ */
	d = BoardRead( no-1 );
	printf("%d.%.03dsec REG = %.04X\n", sec_timer, ms_timer, d);
	init_queue(CONSOLE_QUEUE);/* �܂���ɂ��� */
	while(empty(CONSOLE_QUEUE)) {
		PLIB_WDT_TimerClear(WDT_ID_0);/* �E�H�b�`�h�b�O�N���A */
		if (d != BoardRead( no-1 )) {
			d = BoardRead( no-1 );
//			printf("%d.%.05d lap = %.05dms REG = %.04X\n", sec_timer, ms_timer, ms_timer - bef_ms_timer, d);
			printf("%.05d lap = %.05dms REG = %.04X\n", ms_timer, ms_timer - bef_ms_timer, d);
			bef_ms_timer = ms_timer;
		}
	}
	dequeue(CONSOLE_QUEUE);
	BoardWrite(no-1, 0);/* �n�e�e�R�}���h */
#endif
	printf("END\n");
}

/**
 * @brief Md�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandMd(void)
{
	/* �������_���v�R�}���h�̕\�� */
#ifndef windows
	int i;
	if (command_count >= 2) {
		int size = 16;
		//int adr = strtol(command[ 1 ], 0, 16);
		unsigned int adr = ToDec(command[ 1 ]);
		if (command_count >= 3) {
			size = strtol(command[ 2 ], 0, 16);
		}
		if ((adr>>24) == 0) {
			adr |= 0x80000000;/* VMEM�ɕύX */
		}
		
		printf("ADDRESS = %.8X\n",adr);
		if (((adr>>28) != 0xB) && ((adr>>28) != 0xE) && ((adr>>28) != 0x8)){
			printf("ADDRESS ERROR\n");
		}
		
		char *p = (char *)adr;
		for( i = 0; i < size; i++){
			if((i % 16)==0){
				printf("\n%.4X:", i + adr);
			}
			printf("%.2X ", p[i]&0xff);
		}
		printf("\n");
	}
#endif
}

/**
 * @brief Default�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandDefault(void)
{
	printf("�f�B�t�H���g�p�����[�^�ݒ�\n");
	SetDefault();/* �f�B�t�H���g�p�����[�^�ݒ� */
}

/**
 * @brief Status�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandStatus(void)
{

	if (my_tanmatsu_type == 0x10) {
		MonitorStatusDisp();/* �X�e�[�^�X�̕\������ */
	} else {
		if (command_count >= 2) {
			if (strncmp(command[1], "write", 1) == 0) {
				printf("�X�e�[�^�X�̃Z�[�u����\n");
				SaveStatus();/* �X�e�[�^�X�̃Z�[�u���� */
			}
			if (strncmp(command[1], "read", 1) == 0) {
				printf("�X�e�[�^�X�̃��[�h����\n");
				LoadStatus();/* �X�e�[�^�X�̃��[�h���� */
			}
		}
		StatusDisp();/* �X�e�[�^�X�̕\������ */
	}
}

/**
 * @brief Param�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandParam(void)
{
	extern PARAM param;
	if (command_count == 2) {
		if (strncmp(command[1], "write", 1) == 0) {
			printf("�p�����[�^�̃Z�[�u����\n");
			SaveParam();/* �p�����[�^�̃Z�[�u���� */
			printf("�p�����[�^�̃Z�[�u���� �I��\n");
		}else if (strncmp(command[1], "read", 1) == 0) {
			printf("�p�����[�^�̃��[�h����\n");
			LoadParam();/* �p�����[�^�̃��[�h���� */
		}else if (strncmp(command[1], "default", 1) == 0) {
			printf("�p�����[�^�̃f�B�t�H���g�ݒ菈��\n");
			SetDefault();
		}
	} else if (command_count == 3) {
		if (strncmp(command[1], "offset_timer", 2) == 0) {
			/* �I�t�Z�b�g�^�C�}�̒l */
			int t = atoi(command[2]);
			param.offset_timer = t;
		} else if (strncmp(command[1], "nomusen", 6) == 0) {
			/* �����G���[���G���[�Ƃ����ۂ� 1:�����G���[�Ƃ��Ȃ� */
			int t = atoi(command[2]);
			param.no_musen_error_flag = t;
		} else if (strncmp(command[1], "nomove", 6) == 0) {
			/* �W����display���e�������ꍇ�ړ������ۂ� 1:�ړ����Ȃ� */
			int t = atoi(command[2]);
			param.same_nomove_flag = t;
		} else if (strncmp(command[1], "nofail", 6) == 0) {
			/* �t�@�C���������t�F�C���Ƃ����ۂ� 1:�t�F�C���Ƃ��Ȃ� */
			int t = atoi(command[2]);
			param.no_fail_flag = t;
		} else if (strncmp(command[1], "noboard", 6) == 0) {
			/* board�G���[���G���[�Ƃ����ۂ� 1:�G���[�Ƃ��Ȃ� */
			int t = ToDec(command[2]);
			param.no_board_error_flag = t;
		} else if (strncmp(command[1], "nopc", 4) == 0) {
			/* �^�p�Ǘ��o�b�ԒʐM�ُ�𔻒���`�F�b�N�����ۂ� 1:�G���[�Ƃ��Ȃ� */
			int t = ToDec(command[2]);
			param.no_pc_check_flag = t;
		} else if (strncmp(command[1], "linkage", 4) == 0) {
			/* 
				����@B�̗L�������ݒ�
				��) 00011000 ����@4,5���L��(5�͐���@A�Ȃ̂ŕK�{)
			*/
			if (strlen(command[2]) == CONTROLER_MAX) {
				int t = strtol(command[2], 0, 16);
				param.linkage_status = t;
			} else {
				printf("linkage length is not %d\n",CONTROLER_MAX);
			}
		} else if (strncmp(command[1], "delay", 3) == 0) {
			/* MDM_CS�o�͂̒x������(ms) */
			int t = atoi(command[2]);
			param.mdmcs_delay_time = t;/* MDM_CS�o�͂̒x������(ms) */
		} else if (strncmp(command[1], "reset", 5) == 0) {
			/* reset_count */
			int t = atoi(command[2]);
			param.reset_count = t;/* ���Z�b�g�J�E���g */
		} else if (strncmp(command[1], "respi", 5) == 0) {
			/* 20170305 ����@A����̗v�����琧��@B����̉�����M�m�F�܂ł̍ŏ��҂�����(ms) */
			int t = atoi(command[2]);
			param.response_interval_time_val = t;
		} else if (strncmp(command[1], "respt", 5) == 0) {
			/* 20170305 ����@A����̗v�����琧��@B����̉�����M�܂ł̃^�C���A�E�g�l(ms) */
			int t = atoi(command[2]);
			param.response_timeout_val = t;
		} else if (strncmp(command[1], "musent", 6) == 0) {
			/* 20170305 �����ʐM�̎�M�G���[�^�C���A�E�g�l(�b) */
			int t = atoi(command[2]);
			param.musen_timeout_val = t;
		} else if (strncmp(command[1], "preamble", 3) == 0) {
			/* 20170308 L2.c�̃v���A���u���p�^�[�� */
			int t = strtol(command[2], 0, 16) & 0xffffUL;
			param.preamble_ptn = t;
		}
	} else if (command_count == 4) {
		if (strncmp(command[1], "start_time", 2) == 0) {
			/* �ψڊJ�n���� */
			int hour = atoi(command[2]);
			int min = atoi(command[3]);
			param.start_time = 60 * hour + min;
		} else if (strncmp(command[1], "end_time", 2) == 0) {
			/* �ψڏI������ */
			int hour = atoi(command[2]);
			int min = atoi(command[3]);
			param.end_time = 60 * hour + min;
		}
	}
	printf("�ψڊJ�n,�I��       = %.02d:%.02d %.02d:%.02d %d\n"
														, (param.start_time/60) / 60, (param.start_time/60) % 60//yamazaki
														, (param.end_time/60) / 60, (param.end_time/60) % 60//yamazaki
														, param.offset_timer);
	//printf("�����C������        = %d\n",param.time_correction_time);
	//printf("�����C���v������    = %d\n",param.time_correction_request_time);
	//printf("���E�x�p�^�[��      = %d\n",param.holiday_pattern);
	//printf("���E�x�J�n,�I��     = %.02d:%.02d %.02d:%.02d\n"
	//										,(param.holiday_start_time/60) / 60, (param.holiday_start_time/60) % 60
	//										,(param.holiday_end_time/60) / 60, (param.holiday_end_time/60) % 60);
	//printf("�y�j��,�x�����[�h   = %d,%d\n",param.saturday_mode,param.holiday_mode);
	printf("�t�F�C���p�^�[��    = %d\n",param.fail_pattern);
	printf("linkage_status      = %.08X (12345678)\n",param.linkage_status);
//	printf("�I�t�Z�b�g�^�C�} = %d\n",param.offset_timer);
	printf("nomusen_err,nomove,nofail,db,del,no_board_error,nopc,rst_cnt = %d,%d,%d,%XH,%dms,%XH,%d,%d\n"
		,param.no_musen_error_flag
		,param.same_nomove_flag
		,param.no_fail_flag
		,param.debug_flg
		,param.mdmcs_delay_time
		,param.no_board_error_flag
		,param.no_pc_check_flag
		,param.reset_count);

	if (my_tanmatsu_type != 0x10) {
		printf("response_interval_time_val,response_timeout_val,musen_timeout_val,preamble = %d,%d,%d,%04X\n"
			,param.response_interval_time_val
			,param.response_timeout_val
			,param.musen_timeout_val
			,param.preamble_ptn
		);
	}
}

/**
 * @brief Erase�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandErase(void)
{
#ifndef windows
	/* �t���b�V���C���[�Y */
	if (command_count >= 2) {
		int adr = ToDec(command[ 1 ]);
		printf("flash sector erase ADDRESS = %.8X",adr);
		flash_sector_erase(adr);
	}
#endif
}

/**
 * @brief Fwd�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandFwd(void)
{
#ifndef windows
	/* �t���b�V�����C�g�e�X�g */	
	if (command_count >= 3) {
		//int adr = strtol(command[1], 0,16);
		int adr = ToDec(command[ 1 ]);
		//int data = strtol(command[2], 0,16);
		int data = ToDec(command[ 2 ]);
		flash_write(adr, data);
		printf("flash write ADDRESS = %.8X data = %.4X\n", adr, data);
	}
#endif
}

#if 0
/**
 * @brief Fws�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandFws(void)
{
#ifndef windows
	/* �t���b�V���Z�N�^�[���C�g�e�X�g */	
	static char buf[4096];
	int i;
	for(i=0;i<4096;i++){
		buf[i]=i;
	}
	if (command_count >= 2) {
		//int adr = strtol(command[1], 0,16);
		int adr = ToDec(command[ 1 ]);
		printf("ADDRESS = %.8X\r\n",adr);
//		FlashSectorWrite(adr, buf);
	}
#endif
}
#endif

/**
 * @brief Out�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandOut(void)
{
	/* IO�o�� */
	if (command_count > 2) {
		//int adr = strlen(command[1]);
		int adr = ToDec(command[ 1 ]);
		int d = ToDec(command[2]);
		RegWrite(adr, d);
		printf ("out %X = %04X\n",adr, d);
	}
}

/**
 * @brief In�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandIn(void)
{
	/* IO���� */
	if (command_count > 1) {
		int adr = ToDec(command[ 1 ]);
		int d;
		d = RegRead(adr);
		printf ("in %X = %04X\n",adr, d);
	}
}

/**
 * @brief EEPROM�������݃R�}���h����
 *
 * @return �Ȃ�
 */
static void E2ROM_Write(void)
{
#ifndef windows
	if (command_count == 3) {
		//int adr = strlen(command[1]);
		int adr = ToDec(command[ 1 ]);
		unsigned char d = ToDec(command[2]);
		SPI_E2ROM_Write(adr, d);
		printf ("E2ROM wr: %X = %02X\n",adr, d);
	}
#endif
}

/**
 * @brief EEPROM�ǂݏo���R�}���h����
 *
 * @return �Ȃ�
 */
static void E2ROM_Read(void)
{
#ifndef windows
	if (command_count == 2) {
		int adr = ToDec(command[ 1 ]);
		unsigned char d;
		d = SPI_E2ROM_Read(adr);
		printf ("E2ROM rd: %X = %02X\n",adr, d);
	}
#endif
}

/**
 * @brief Led�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandLed(void)
{
	int i;
	int no;
	int mode;
	printf("LED:");
	if (my_tanmatsu_type == 0x10) {
		extern int monitor_led_status[];
		extern int monitor_output_data;
		if (command_count > 2) {
			no = atoi(command[1]);
			mode = atoi(command[2]);
			monitor_led_status[no] = mode;
		}
		for (i = 0; i < MONITOR_LED_MAX_COUNT ; i++) {
			printf("%d ", monitor_led_status[i]);
		}
		printf("%.8X ", monitor_output_data);
	} else {
		extern int cont_led_status[];
		extern int cont_output_data;
		if (command_count > 2) {
			no = atoi(command[1]);
			mode = atoi(command[2]);
			cont_led_status[no] = mode;
		}
		for (i = 0; i < CONT_LED_MAX_COUNT ; i++) {
			printf("%d ", cont_led_status[i]);
		}
		printf("%.4X ", cont_output_data);
	}
	printf("\n");

#ifndef windows
	/* IO���� */
	if (command_count > 2) {
		int no = atoi(command[1]);
		int mode = atoi(command[2]);
		printf("BSP_LED%s(BSP_LED_%d)\n", mode ? "On" : "Off",no );
		if (mode == 0) {
			if (no == 3) {
				no = BSP_LED_3;
			} else if (no == 4) {
				no = BSP_LED_4;
			} else if (no == 7) {
				no = BSP_LED_7;
			} else if (no == 8) {
				no = BSP_LED_8;
			}
			BSP_LEDOff(no);
		} else if (mode == 1) {
			if (no == 3) {
				no = BSP_LED_3;
			} else if (no == 4) {
				no = BSP_LED_4;
			} else if (no == 7) {
				no = BSP_LED_7;
			} else if (no == 8) {
				no = BSP_LED_8;
			}
			BSP_LEDOn(no);
		}
	}
#endif
}

/**
 * @brief Time�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandTime(void)
{
	TIME_INFO t,t1,t2;
	char str[256];

	/* �����ݒ� 112233 */
	if (command_count > 1) {
		char str[3];
		str[2] = '\0';
		if (strlen(command[1]) == 14) {
			t.holiday_week.week = 1;
			strncpy(str,&command[1][0], 2);
			t.year[0] = strtol(str, 0,16);
			strncpy(str,&command[1][2], 2);
			t.year[1] = strtol(str, 0,16);
			strncpy(str,&command[1][4], 2);
			t.month = strtol(str, 0,16);
			strncpy(str,&command[1][6], 2);
			t.day = strtol(str, 0,16);
			strncpy(str,&command[1][8], 2);
			t.hour = strtol(str, 0,16);
			strncpy(str,&command[1][10], 2);
			t.min = strtol(str, 0,16);
			strncpy(str,&command[1][12], 2);
			t.sec = strtol(str, 0,16);
			SetTime(&t);/* PIC����RTC�ɏ������� */
			SaveRTC(&t);/* �s�����pRTC�ɏ������� */
		} else {
			printf("parameter error len=14 exp. 20170102030405\n");
		}
	}
	
	LoadRTC(&t1);/* �s�����pRTC����ǂݏo�� */
	SetNowTime(&t2);/* PIC����RTC����ǂݏo�� */
	sprintf(str,"����(RTC)�@%02X%02X/%02X/%02X(%d) %02X:%02X:%02X"
		,t1.year[0]
		,t1.year[1]
		,t1.month
		,t1.day
		,t1.holiday_week.week
		,t1.hour
		,t1.min
		,t1.sec);
	printf("%s\n", str);
	sprintf(str,"����(CPU)�@%02X%02X/%02X/%02X(%d) %02X:%02X:%02X"
		,t2.year[0]
		,t2.year[1]
		,t2.month
		,t2.day
		,t2.holiday_week.week
		,t2.hour
		,t2.min
		,t2.sec);
	printf("%s\n", str);
}

/**
 * @brief I�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandI(void)
{
	/* �[����IO���̓f�[�^��ݒ肷�� */
	int i;
	extern int input_data[];
	extern int display_board_count;
	if (command_count > 1) {
		int len = strlen(command[1]);
		for(i=0;i<len;i++) {
			input_data[i] = command[1][i]&0xf;
		}
	}
	printf("input_data = ");
	for(i=0;i<display_board_count;i++) {
		printf("%02X ",input_data[i]);
	}
	printf("\n");
}

/**
 * @brief Dgsw�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandDgsw(void)
{
	/* �{�[�h��DGSW��Ԃ̕\�� */
	extern int DGSWRead();
	int dgsw = DGSWRead();
		printf("dgsw = %02X\n",dgsw);
}

/**
 * @brief Sw�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandSw(void)
{
	/* SW����R�}���h�̕\��(�e�X�g�p) */
	extern int SWRead();
	extern int btn_status;
	int sw;
	if (my_tanmatsu_type == 0x10) {
		if (command_count > 1) {
			btn_status = strtol(command[1], 0, 16);
		}
		sw = MonitorBtnRead();
	} else {
		if (command_count > 1) {
			swtest = strtol(command[1], 0, 16);
		}
		sw = SWRead();
	}
	printf("sw = %04X\n",sw);
}

/**
 * @brief P�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandP(void)
{
	extern int io_power_outage_flag;/* ��d�����t���O */
	if (command_count > 1) {
		io_power_outage_flag = strtol(command[1], 0, 16);
	}
	printf("io_power_outage_flag = %02X\n",io_power_outage_flag);
}

/**
 * @brief Mtest�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandMtest(void)
{
	extern int monitor_command_flg;
	extern int monitor_timeout_flag;
	extern int monitor_led_status[ MONITOR_LED_MAX_COUNT];
	extern MONITOR_COMMAND monitor_command;/* ����@����̐���w�߂̓d��  */
	int i;
	if (command_count == 2) {
		monitor_command_flg = strtol(command[1], 0, 16);
	} else if (command_count == 4) {
		/* monitor_command�p�b�` */
		int no = atoi(command[1]);
		int sts = atoi(command[2]);
		int d = atoi(command[3]);
		monitor_command.command[no][sts] = d;
	}
	MonitorCommandDisp();/* �Ď��Ղւ̐���w�ߕ\������ */
	printf("monitor_timeout_flag = %d\n",monitor_timeout_flag);
	printf("monitor_command_flg = %d\n",monitor_command_flg);
	for(i=0;i< MONITOR_LED_MAX_COUNT;i++) {
		printf("%d ",monitor_led_status[i]);
	}
	printf("\n");
}

/**
 * @brief Version�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandVersion(void)
{
	/* �o�[�W�����\�� */
	printf("version        = \"%s %s\"\n",VERSION_DATE, VERSION_TIME);
	printf("FPGAversion    = %.8X\n",FPGAVersionRead());
	printf("CPUFPGAversion = %.8X\n",CPUFPGAVersionRead());
}

/**
 * @brief H�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandH(void)
{
	printf("a                �Ď������_���v\n");
	printf("hi               ���͗���\n");
	printf("b                ����w�߃_���v\n");
	printf("r                com1�ւ̃e�X�g�p�����_���f�[�^���M����\n");
	printf("n                �ʏ�w�߃_���v\n");
	printf("mv [no]          ��board���[�^�[�I��\n");
	printf("md adr [size]    �������_���v\n");
	printf("default           �f�B�t�H���g�p�����[�^�ݒ�\n");
	printf("status write|read �X�e�[�^�X�̃��[�h�A�Z�[�u\n");
	printf("param write|read �p�����[�^�̃��[�h�A�Z�[�u\n");
	printf("erase adr        �t���b�V���C���[�Y\n");
	printf("fwd adr          �t���b�V�����C�g�e�X�g\n");
	//printf("fws adr data       �t���b�V��	���C�g�e�X�g\n");
	printf("o xx            IO�|�[�g�o��\n");
	printf("in              IO�|�[�g����\n");
	printf("eew adr data    E2PROM��������\n");
	printf("eer adr         E2PROM�ǂݏo��\n");
	printf("led 34|7|8 [0|1] LED���,BSPLED����\n");
	printf("test            test����\n");
	printf("time [hhmmss]   �����ݒ�ǂݏo��\n");
	printf("i               �f�o�b�O�p�̓��͒l�̐ݒ�\n");
	printf("dgsw            BSP�f�W�X�C�b�`�̓ǂݍ��ݏ���\n");
	printf("sw              �f�o�b�O�p�̃X�C�b�`�ݒ�\n");
	printf("p [0|1]         �f�o�b�O�p�̒�d�X�e�[�^�X�ݒ菈��\n");
	printf("mtest [0|1]     �f�o�b�O�p�̉^�p�Ǘ�PC��~�t���O�ݒ�\n");
	printf("version         �o�[�W����\n");
	printf("w               �e�X�g�p�������[�v����\n");
	printf("d XX            �f�o�b�O�t���O�؂�ւ�\n");
	printf("c YYYYMMDD      ���̓��̏�Ԃ��l�����鏈��\n");
	printf("quit application exit !!!\n");
	printf("help �w���v\n");
}

/**
 * @brief W�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandW(void)
{
    printf("waitkey\n");
#ifndef windows	
	init_queue(CONSOLE_QUEUE);/* �܂���ɂ��� */
	while(empty(CONSOLE_QUEUE)) {
	}
	dequeue(CONSOLE_QUEUE);
#endif
	printf("waitkey end \n");
}

/**
 * @brief D�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandD(void)
{
	/* d �R�}���h�@�f�o�b�O�t���O�Z�b�g */
	if (command_count == 1) {
		if (param.debug_flg == 0) {
			param.debug_flg = debug_flg_before;
		} else {
			debug_flg_before = param.debug_flg;
			param.debug_flg = 0;
		}
	} else {
		debug_flg_before = param.debug_flg;
		param.debug_flg = strtol(command[1], 0, 16);
	}
	printf("%s debug_flg = %XH,bef=%XH\n", DebugTimePrint(), param.debug_flg, debug_flg_before);
}

/**
 * @brief ���̓��̏�Ԃ��l�����鏈��
 *
 * @return �Ȃ�
 */
static void CommandCheckStartTime(void)
{
	extern int CheckStartTime(TIME_INFO_BIN *t);
	int ret;
	TIME_INFO_BIN tb;
	int year,month,day;
	int daymax;
	static int month_day[12]={0x31,0x28,0x31,0x30,0x31,0x30,0x31,0x31,0x30,0x31,0x30,0x31};

	if (command_count == 2) {
		FILE *fp = fopen("test.txt", "w");
	    if (fp == NULL) {
			printf("%s:%s is not opened!\n",__func__,"test.txt");
	    }
		year = atoi(command[1]);
		for( year = 2017; year <= 2030 ; year++) {
		for( month = 1; month <= 12 ; month++) {
			if ( month == 2) {
				int y = year;
				daymax = 28 + (1 / (y - y / 4 * 4 + 1)) * (1 - 1 / (y - y / 100 * 100 + 1))
					+ (1 / (y - y / 400 * 400 + 1));	
			} else {
				daymax = BIN(month_day[month - 1]);
			}
			for( day = 1; day <= daymax ; day++) {
				tb.year = year;
				tb.month = month;
				tb.day = day;
				//t.week = atoi(command[4]);
				tb.week = subZeller(tb.year, tb.month, tb.day)+1;//���j�����P�Ƃ���
				ret = CheckStartTime(&tb);
				if (ret != 0) {
					printf("%04d%02d%02d\n",year,month,day);
					fprintf(fp,"%04d%02d%02d\n",year,month,day);
				}
			}
		}
		}
		fclose(fp);
		
	} else if (command_count >= 4) {
		tb.year = atoi(command[1]);
		tb.month = atoi(command[2]);
		tb.day = atoi(command[3]);
		//t.week = atoi(command[4]);
		tb.week = subZeller(tb.year, tb.month, tb.day)+1;//���j�����P�Ƃ���
	
		ret = CheckStartTime(&tb);
		printf("CommandCheckStartTime ret = %d %s\n",ret,ret ? "TODAY_IS_HOLIDAY" : "TODAY_IS_NORMAL_DAY");
	} else {
		extern HOLIDAY_DATA holiday_data;/* �ψڋx�~���Ǘ��p */
		int i;
		printf("�j�����X�g\n");
		for(i=0; i < holiday_data.holiday_count;i++){
			printf("%2d:%2d %2d %d\n",i+1
				,holiday_data.holiday[i].month
				,holiday_data.holiday[i].day
				,holiday_data.holiday[i].type);
		}
		printf("�ψڋx�~�����X�g\n");
		for(i=0; i < holiday_data.kyuushi_count;i++){
			printf("%2d:%2d %2d %d\n",i+1
				,holiday_data.kyuushi[i].month
				,holiday_data.kyuushi[i].day
				,holiday_data.kyuushi[i].type);
		}
	}
}


/**
 * @brief test�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandTest(void)
{
	printf("rtc_sync_timer=%d\n",rtc_sync_timer);
	printf("CommandTest %X %X\n",rtc_sync_timer,&rtc_sync_timer);
#ifndef windows //yamazaki*	
	//if (command_count == 1) {
	//    PLIB_RTCC_Enable(RTCC_ID_0);/* RTC�C�l�[�u��*/
	//} else if (command_count == 2) {
	//    PLIB_RTCC_AlarmEnable(RTCC_ID_0); /* Enable alarm */
	//} else {
	//    PLIB_RTCC_Enable(RTCC_ID_0);/* RTC�C�l�[�u��*/
	//    PLIB_RTCC_AlarmEnable(RTCC_ID_0); /* Enable alarm */
	//}
	//if (command_count == 2) {
	//	int d = strtol(command[1], 0, 16);
	//	printf("PcPower %d\n",d);
	//	PcPower(d);
	//}

	
	//extern HOLIDAY_DATA holiday_data;/* �ψڋx�~���Ǘ��p */
	//printf("SaveHoliday 1\n");
	//flash_sector_erase(HOLIDAY_ADDRESS);
	//printf("SaveHoliday 2\n");
	//flash_write_buf(HOLIDAY_ADDRESS, (int)&holiday_data, sizeof(HOLIDAY_DATA) / 2);
	//printf("SaveHoliday 3\n");
	
	if (command_count == 1) {
		extern void NetInit( void);
//		NetInit();
//		printf("NetInit\n");
	} else {
		extern void NetInit2( void);
//		NetInit2();
//		printf("NetInit2\n");
	}
#endif
}


/**
 * @brief Q�R�}���h����
 *
 * @return �Ȃ�
 */
static void CommandQ(void)
{
	exit(0);
}

/**
 *	@brief �f�o�b�O�p�̕\������
 *         ��������t�����ĕ\������
 *
 *	@param [ char *str1 ]  ������P�i�[�|�C���^
 *	@param [ char *str2 ]  ������Q�i�[�|�C���^
 *	@param [ int mask ]  �\������A
 *
 *	@retval �Ȃ�
 */
void DebugPrint(char *str1, char *str2, int mask) {
	if ((mask == 0) || ((param.debug_flg & mask) != 0)) {
		printf("%s %X:%s %s\n",  DebugTimePrint(), mask, str1, str2);
	}
}

/**
 *	@brief �f�o�b�O�p�̎����������Ԃ�����
 *
 *	@retval �Ȃ�
 */
static char *DebugTimePrint(void) {
	static char buff[ 100 ]="";

#ifdef windows	
	time_t now = time(NULL);
	struct tm *pnow = localtime(&now);
	struct timeval  tv;
	gettimeofday(&tv, NULL);
	sprintf(buff, "%02d:%02d:%02d.%03d"
		, pnow->tm_hour
		, pnow->tm_min
		, pnow->tm_sec
		, (int)tv.tv_usec/1000);
	//sprintf(buff, "%d/%02d/%02d %02d:%02d:%02d.%03d"
	//	, pnow->tm_year + 1900
	//    , pnow->tm_mon + 1
	//    , pnow->tm_mday
	//	, pnow->tm_hour
	//	, pnow->tm_min
	//	, pnow->tm_sec
	//	, (int)tv.tv_usec/1000);
#else
	uint32_t data,data2;
	struct tm now;
	struct tm *pnow = &now;
	data = PLIB_RTCC_RTCDateGet (RTCC_ID_0 );
	data2 = PLIB_RTCC_RTCTimeGet ( RTCC_ID_0 );
	sprintf(buff, "%02X%02X/%02X/%02X %02X:%02X:%02X.%03d"
		, 0x20
		, (data >> 24) &0xff
	    , (data >> 16) &0xff
	    , (data >> 8) &0xff
		, (data2 >> 24) &0xff
		, (data2 >> 16) &0xff
		, (data2 >> 8) &0xff
		, 0);
//		, rtc_sync_timer);
#endif
	return buff;
}

/**
 *	@brief 16 �i������� 10 �i���ɕϊ����鏈��
 *
 *	@param [ const char str[ ] ]  ������i�[�|�C���^
 *
 *	@retval �Ȃ�
 */
static unsigned long ToDec(const char str[ ])
{
    short i = 0;        /* �z��̓Y���Ƃ��Ďg�p */
    short n;
    unsigned long x = 0;
    char c;

    while (str[i] != '\0') {        /* ������̖����łȂ���� */

            /* '0' ���� '9' �̕����Ȃ� */
        if ('0' <= str[i] && str[i] <= '9')
            n = str[i] - '0';        /* �����ɕϊ� */

            /* 'a' ���� 'f' �̕����Ȃ� */
        else if ('a' <= (c = tolower(str[i])) && c <= 'f')
            n = c - 'a' + 10;        /* �����ɕϊ� */

        else {        /* ����ȊO�̕����Ȃ� */
            printf("�����ȕ����ł��B\n");
            exit(0);        /* �v���O�������I�������� */
        }
        i++;        /* ���̕������w�� */

        x = x *16 + n;    /* ���オ�� */
    }
    return (x);
}

