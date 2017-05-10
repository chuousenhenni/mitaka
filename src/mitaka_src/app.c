/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	app.c
 *	�T�v
 *
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
#include "L2.h"
#include "L2_monitor.h"

/*
 *===========================================================================================
 *					�����萔��`�E�^��`�E�}�N����`
 *===========================================================================================
 */

/*
00:00�ɂȂ���
���̓��̃X�P�W���[�����`�F�b�N
�x�����H
�y�j�����H�@�x�@�\�����e���o��

�j�����H�@�x�@�L���H
�o�^����Ă�������H
�ψڋx�~�����H
�A����A���ݒ�

�Ǘ��\���̂̒�`���K�v
*/
/*
�S�������@�X�P�W���[���ɂ��������ĕ\������
�S���蓮�@�[���`����̎w�߂ɂ��������ĕ\������
	
�[��������@�a�̗����͎�M�d���̕ω������ēo�^����
�[��������@�`�͋@��̃X�e�[�^�X�̕ω������ēo�^����
	
	
�^��_�@�ψڊJ�n�A�I�����Ԃ͒[�����ɐݒ肷��̂�	
*/

/*
���u���[�h�̏ꍇ
	�`���e�X�g���[�h�@�w�߂��ꂽ�ݒ�ɂ���
		
	�X�P�W���[���o�^�ς݂̏ꍇ
		���̓��̐�������ɂ�蓮������肷��
			���G�x��
			�y�j��
			�ʏ펞
				�ʏ�J�n����
				�ψڊJ�n����
				���̕ω����ɂh�n������s��
	�X�P�W���[�����o�^�̏ꍇ
		�[������@�`����̎�M�d���ɂ�铮����s��

�蓮���[�h�̏ꍇ
	�X�C�b�`�ɂ�鑀�삪�s��ꂽ�ꍇ
		�X�C�b�`�ɂ�铮����s��

���Ԃɂ�鐧��
	�O�O�F�O�O�ɃX�P�W���[�����N���A
	�X�P�W���[���o�^�˗��𑗂�

����@���[�h���O��̃��[�h�Ɠ����ꍇ�͉������Ȃ�
�@�@�@�@�@�@�@�O��̃��[�h�ƈقȂ�ꍇ�͂h�n������s��

�����@�[������@����̎�M�f�[�^��O��̎�M�f�[�^�Ɣ�r���A�C�x���g�A�G���[��Ԃɕω�������ꍇ�͗������Ƃ��Ďc���B



���ׂ�����
����
�@�ُ�֕ω������Ƃ��̃��O�i�ُ�A�����j
���u����̊m�F
�N�����̓���
��d���A��d���A���̓���
�h�n����̐���
�ݒ�p�����[�^�̎���
	
*/

/*
 *===========================================================================================
 *					�����ϐ���`
 *===========================================================================================
 */

int my_tanmatsu_type;/* �[���^�C�v 1:A,2:B... */
HANDLE hComm1;       /* �V���A���|�[�g�̃n���h�� */
int ms_timer;/* ms�^�C�} */
int sec_timer;/* �b�^�C�} */
static clock_t start,end;

static char cont_name [][10] = {/* ����@�� */
	"B1",
	"C1",
	"D ",
	"B2",
	"A ",
	"C2",
	"B3",
	"C3"
};

/*
 *===========================================================================================
 *					�O���ϐ���`
 *===========================================================================================
 */


/*
 *===========================================================================================
 *					����	�֐���`
 *===========================================================================================
 */

void NetInit( void);
static void Init(void );/* ���������� */
static void ComInit(void);/* �ʐM�|�[�g������ */
static HANDLE ComCreate(char *com_name);
int DGSWRead(void);/* SW��Ԃ̓ǂݍ��� */
void LedOut(int d);/* LED�\������ */
static void TimeInit(void);/* �����ݒ� */
void Send(HANDLE h, unsigned char *p, int size);
//void SendCom1(HANDLE h, unsigned char *p, int size);/* ����@�ԒʐM(USART_ID_1 64000bps ODD)�ւ̑��M���� */
void SendCom2(HANDLE h, unsigned char *p, int size);/* �^�p�Ǘ�PC�ԒʐM(USART_ID_2 115200bps ODD)�ւ̑��M���� */
void SendCom3(HANDLE h, unsigned char *p, int size);/* �Ď��ՊԒʐM(USART_ID_3 1200bps ODD)�ւ̑��M���� */
void RcvIntCom(void *s);//��M���荞�ݏ���(�V���A���o�R)
void TimerInt(void);/* 10MS�����^�C�}���荞�ݏ��� */

/*
 *===========================================================================================
 *					�O���[�o���֐���`
 *===========================================================================================
 */
/**
 *	@brief ����������
 *
 *	@retval �Ȃ�
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
//    appData.state = APP_STATE_INIT;

#ifndef windows //yamazaki*	
	IPV4_ADDR           ipAddr;
	BSP_Initialize();           // add Adv-M.Saito
	APP_Commands_Init();		// add LAN
	flash_init();
	//printf("\033[2J"); //��ʃN���A
	//�����ŃE�H�b�`�h�b�O�̗L�����������肷��
	PLIB_WDT_Enable(WDT_ID_0);
	int dgsw = DGSWRead();/* SW��Ԃ̓ǂݍ��� */
	dgsw &= 0xf;
	if ((dgsw >= 1) && (dgsw <=8)) {
		my_tanmatsu_type = dgsw;
	} else {
		my_tanmatsu_type = 0x10;
	}
	LedOut(dgsw);/* LED�\������ */
    PLIB_RTCC_Enable(RTCC_ID_0);/* RTC�C�l�[�u��*/
    PLIB_RTCC_AlarmEnable(RTCC_ID_0); /* Enable alarm */
#endif
	Init( );
	if (my_tanmatsu_type == CONTA_ADDRESS) {
		/* ����@A */
		printf("����@%s %d\n", cont_name[my_tanmatsu_type-1], my_tanmatsu_type);
		ContIOInit();/* ����@��IO���������� */
		RcvCom1Init;/* kaji20170307 */
		RcvCom3Init;/* kaji20170310 */
		ControlerInit();
		ControlerAInit();
	} else if (my_tanmatsu_type == 0x10) {
		/* �Ď��� */
		printf("�Ď���\n");
		MonitorIOInit();/* �Ď��Ղ�IO���������� */
		RcvCom3Init;/* kaji20170310 */
		MonitorInit();/* �Ď��Ղ�LED���������� */
	} else {
		/* ����@B */
		printf("����@%s %d\n", cont_name[my_tanmatsu_type-1], my_tanmatsu_type);
		ContIOInit();/* IO���������� */
		RcvCom1Init;/* kaji20170307 */
		ControlerInit();
		ControlerBInit();
	}
	printf(">>");
    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
}

#ifndef windows //yamazaki*	
void NetInit( void)
{
    SYS_STATUS          tcpipStat;
    const char          *netName, *netBiosName;
    static IPV4_ADDR    dwLastIP[2] = { {-1}, {-1} };
    IPV4_ADDR           ipAddr;
    TCPIP_NET_HANDLE    netH;
    int                 i, nNets;

	
			/* TCP/IP protocol stack init */
            tcpipStat = TCPIP_STACK_Status(sysObj.tcpip);
            if(tcpipStat < 0)
            {   // some error occurred
                SYS_MESSAGE("APP: TCP/IP stack initialization failed!\r\n");
                appData.state = APP_TCPIP_ERROR;
            }
            else if(tcpipStat == SYS_STATUS_READY)
            {
                // now that the stack is ready we can check the
                // available interfaces
                nNets = TCPIP_STACK_NumberOfNetworksGet();
                for(i = 0; i < nNets; i++)
                {

                    netH = TCPIP_STACK_IndexToNet(i);
                    netName = TCPIP_STACK_NetNameGet(netH);
                    netBiosName = TCPIP_STACK_NetBIOSName(netH);

#if defined(TCPIP_STACK_USE_NBNS)
                    SYS_PRINT("\r\nInterface %s on host %s - NBNS enabled\r\n", netName, netBiosName);
#else
                    SYS_PRINT("\r\nInterface %s on host %s - NBNS disabled\r\n", netName, netBiosName);
#endif  // defined(TCPIP_STACK_USE_NBNS)

                }
                appData.state = APP_TCPIP_WAIT_FOR_IP;
                SYS_MESSAGE("APP: TCP/IP stack initialization complete!\n\r");

            }
	
}
void NetInit2( void)
{
    SYS_STATUS          tcpipStat;
    const char          *netName, *netBiosName;
    static IPV4_ADDR    dwLastIP[2] = { {-1}, {-1} };
    IPV4_ADDR           ipAddr;
    TCPIP_NET_HANDLE    netH;
    int                 i, nNets;

	
	
            // if the IP address of an interface has changed
            // display the new value on the system console
            nNets = TCPIP_STACK_NumberOfNetworksGet();

            for (i = 0; i < nNets; i++)
            {
                netH = TCPIP_STACK_IndexToNet(i);
                ipAddr.Val = TCPIP_STACK_NetAddress(netH);
                if(dwLastIP[i].Val != ipAddr.Val)
                {
                    dwLastIP[i].Val = ipAddr.Val;

                    SYS_MESSAGE(TCPIP_STACK_NetNameGet(netH));
                    SYS_MESSAGE("IP Address: ");
                    SYS_PRINT("%d.%d.%d.%d \r\n", ipAddr.v[0], ipAddr.v[1], ipAddr.v[2], ipAddr.v[3]);
                    if (ipAddr.v[0] != 0 && ipAddr.v[0] != 169) // Wait for a Valid IP
                    {
                        appData.state = APP_STATE_WAIT_FOR_ALARM;
                        SYS_MESSAGE("This version is ping response only!!\r\n");
                    }
                }
            }
	
}
#endif
void APP_Tasks ( void )
{
	//int dgsw= DGSWRead();
	//if ((dgsw&0x80) == 0) {
		if (my_tanmatsu_type == CONTA_ADDRESS) {
			ControlerASrv();/* ����@A */
			RcvCom1Srv();/* kaji20170307 */
			RcvCom3Srv();/* kaji20170310 */
		} else if (my_tanmatsu_type == 0x10) {
			MonitorSrv();/* �Ď��� */
			RcvCom3Srv();/* kaji20170310 */
		} else {
			ControlerBSrv();/* ����@B */
			RcvCom1Srv();/* kaji20170307 */
		}
	//}
	MaintenanceSrv();/* �����e�i���X�i�L�[���̓R�}���h�j���� */
	
#ifdef windows	
	TimerInt();/* �^�C�}�X�V���� */
#else
	if (ms_timer >= 1000) {
        ms_timer = 0;
		sec_timer++;
		if (sec_timer >= 2) {/* 2�b�Ԃ�DGSW�̏�Ԃ�\�����Ă��� */
	        if ((sec_timer%2) == 0){
	             BSP_LEDOff(BSP_LED_3);
	        } else {
	             BSP_LEDOn(BSP_LED_3);
	        }
		}
	}
    PLIB_WDT_TimerClear(WDT_ID_0);/* �E�H�b�`�h�b�O�N���A */

#endif
}

/*
 *===========================================================================================
 *					�����֐���`
 *===========================================================================================
 */
/**
 *	@brief ����������
 *
 *	@retval �Ȃ�
 */
static void Init(void)
{
#ifdef windows	
	ComInit();/* �ʐM�|�[�g������ */
#endif
	MaintenanceInit();/* �f�o�b�O�p�̃L�[���͂ɂ�鏈���̏��������� */
	TimeInit();/* �����ݒ� */
	init_all_queue();
#ifdef windows	
	_beginthread( RcvIntCom, 0,0 );
#endif
}

/**
 *	@brief �ʐM�|�[�g����������
 *
 *	@retval �Ȃ�
 */
static void ComInit(void)
{
	if (my_tanmatsu_type == CONTA_ADDRESS) {
		hComm1 = ComCreate("COM3");/* ����@A */
	} else {
		hComm1 = ComCreate("COM2");/* ����@ */
	}
}

//�V���A���|�[�g�C�j�V����
HANDLE ComCreate(char *com_name)
{
	HANDLE handle;
#ifdef windows	
	DCB dcb;            /* �ʐM�p�����[�^ */
	COMMTIMEOUTS cto;
	
	handle = CreateFile(
	    com_name,                       /* �V���A���|�[�g�̎w�� */
	    GENERIC_READ | GENERIC_WRITE, /* �A�N�Z�X���[�h */
	    0,                            /* ���L���[�h */
	    NULL,                         /* �Z�L�����e�B���� */
	    OPEN_EXISTING,                /* �쐬�t���O */
	    FILE_ATTRIBUTE_NORMAL,        /* ���� */
	    NULL                          /* �e���v���[�g�̃n���h�� */
	); 
	if ( handle == INVALID_HANDLE_VALUE ) {
		printf("%s Open Error!\n", com_name);
	} else {
		printf("%s Opend!\n", com_name);
	}
	printf("%s\n",com_name);
	GetCommState(handle, &dcb); /* DCB ���擾 */
//	dcb.BaudRate = 9600;        //�ʐM���x
	dcb.BaudRate = 115200;        //�ʐM���x
	dcb.ByteSize = 8;            //�f�[�^��
	dcb.Parity = ODDPARITY;       // �p���e�B�r�b�g�FEVENPARITY,MARKPARITY,NOPARITY,ODDPARITY
	dcb.StopBits = ONESTOPBIT;   // �X�g�b�v�r�b�g�FONESTOPBIT,ONE5STOPBITS,TWOSTOPBITS
	SetCommState(handle, &dcb); /* DCB ��ݒ� */
	/* ----------------------------------------------
	    �V���A���|�[�g�̃^�C���A�E�g��ԑ���
	---------------------------------------------- */
	GetCommTimeouts( handle, &cto );           // �^�C���A�E�g�̐ݒ��Ԃ��擾
	cto.ReadIntervalTimeout = 10;       //ms(0�Ń^�C���A�E�g�Ȃ�)
	cto.ReadTotalTimeoutMultiplier = 0;   //Read : 1�o�C�g�ɑ΂���^�C���A�E�g�搔(0�Ń^�C���A�E�g�Ȃ�)
	cto.ReadTotalTimeoutConstant = 10;  //Read : 0�o�C�g���̃^�C���A�E�g�萔(0�Ń^�C���A�E�g�Ȃ�)
	cto.WriteTotalTimeoutMultiplier = 0;  //Write : 1�o�C�g�ɑ΂���^�C���A�E�g�搔(0�Ń^�C���A�E�g�Ȃ�)
	cto.WriteTotalTimeoutConstant = 0;    //Write : 0�o�C�g���̃^�C���A�E�g�萔(0�Ń^�C���A�E�g�Ȃ�)
	SetCommTimeouts( handle, &cto );           // �^�C���A�E�g�̏�Ԃ�ݒ�
#endif
	return handle;
}

/**
*	@brief ����@�ԒʐM(USART_ID_1 64000bps ODD)�ւ̑��M����
 *
 *	@param [p]  ���M�f�[�^�i�[�|�C���^�Z���TID
 *	@param [sizie]  �i�[�T�C�Y
 *
 *	@retval �Ȃ�
 */
void Send(HANDLE h, unsigned char *p, int size) {
}
//void SendCom1(HANDLE h, unsigned char *p, int size) {  kaji20170306 L2.c�ֈڍs
static void non_SendCom1(HANDLE h, unsigned char *p, int size) {
	DWORD writesize;    /* �|�[�g�֏������񂾃o�C�g�� */
	//DebugPrint("", "����@�ԒʐM", 0x20);
	//int i;
	//for(i=0;i<size;i++) {
	//	printf("%02X ",p[i]);
	//}
	//printf("\n");
#ifdef windows	
	WriteFile(hComm1, p, size, &writesize, NULL);
#else
	/* �����Ńf�[�^�𑗐M���� */
	int i = 0;
	//printf("SendCom1 size=%d\n",size);
	while(1) {//�S�����肫��ɂ͂��ꂪ�K�v
		/* Write a character at a time, only if transmitter is empty */
		while (PLIB_USART_TransmitterIsEmpty(USART_ID_1)) {
			/* Send character */
			PLIB_USART_TransmitterByteSend(USART_ID_1, p[i]);
			//printf("%02x ",p[i]);
			/* Increment to address of next character */
			i++;
			if(i == size) {
				//printf("\n");
				return;
			}
		}
	}
#endif
	
	
}

/**
 *	@brief �^�p�Ǘ�PC�ԒʐM(USART_ID_2 115200bps ODD)�ւ̑��M����
 *
 *	@param [p]  ���M�f�[�^�i�[�|�C���^�Z���TID
 *	@param [sizie]  �i�[�T�C�Y
 *
 *	@retval �Ȃ�
 */
void SendCom2(HANDLE h, unsigned char *p, int size) {
	DWORD writesize;    /* �|�[�g�֏������񂾃o�C�g�� */
unsigned char uart2_tx_buf[128];/* kaji20170310 */
	//int i;
	//for(i=0;i<size;i++) {
	//	printf("%02X ",p[i]);
	//}
	//printf("\n");
	uart2_tx_buf[0] = 0x5a;/* kaji20170310 */
	uart2_tx_buf[1] = 0xff;/* kaji20170310 */
	memmove(&uart2_tx_buf[2], p, size);/* kaji20170310 */
	size += 2;/* kaji20170310 */
#ifdef windows	
//	WriteFile(hComm1, p, size, &writesize, NULL);
	WriteFile(hComm1, uart2_tx_buf, size, &writesize, NULL);/* kaji20170310 */
#else
	/* �����Ńf�[�^�𑗐M���� */
	int i = 0;
	/* Write a character at a time, only if transmitter is empty */
	while(1) {//�S�����肫��ɂ͂��ꂪ�K�v
		while (PLIB_USART_TransmitterIsEmpty(USART_ID_2)) {
	//	while (PLIB_USART_TransmitterIsEmpty(USART_ID_1)) {
			/* Send character */
//			PLIB_USART_TransmitterByteSend(USART_ID_2, p[i]);
			PLIB_USART_TransmitterByteSend(USART_ID_2, uart2_tx_buf[i]);
	//		PLIB_USART_TransmitterByteSend(USART_ID_1, p[i]);
			//printf("X%02x ",p[i]);
			/* Increment to address of next character */
			i++;
			if(i == size) {
				//printf("\n");
				return;
			}
		}
	}
#endif
}

/**
 *	@brief �Ď��ՊԒʐM(USART_ID_3 1200bps ODD)�ւ̑��M����
 *
 *	@param [p]  ���M�f�[�^�i�[�|�C���^�Z���TID
 *	@param [sizie]  �i�[�T�C�Y
 *
 *	@retval �Ȃ�
 */
int send_com3_p = 0;
//void SendCom3(HANDLE h, unsigned char *p, int size) {
static void nonSendCom3(HANDLE h, unsigned char *p, int size) {
	DWORD writesize;    /* �|�[�g�֏������񂾃o�C�g�� */
	//DebugPrint("", "�Ď��ՊԒʐM", 0x20);
	//int i;
	//for(i=0;i<size;i++) {
	//	printf("%02X ",p[i]);
	//}
	//printf("\n");
#ifdef windows	
	WriteFile(hComm1, p, size, &writesize, NULL);
	send_com3_p = size;
#else
	/* �����Ńf�[�^�𑗐M���� */
	//printf("SendCom3 size=%d\n",size);
	/* Write a character at a time, only if transmitter is empty */
	while (PLIB_USART_TransmitterIsEmpty(USART_ID_3)) {
	//while (PLIB_USART_TransmitterIsEmpty(USART_ID_1)) {
		/* Send character */
		PLIB_USART_TransmitterByteSend(USART_ID_3, p[send_com3_p]);
//		PLIB_USART_TransmitterByteSend(USART_ID_1, p[i]);
		//printf("%02x ",p[send_com3_p]);
		/* Increment to address of next character */
		send_com3_p++;
		if(send_com3_p == size) {
			//printf("\n");
//			MdmcsWrite(0);/* MDM_CS�̏o�͏��� */
			return;
		}
	}

#endif
}

void SendCom3_all(HANDLE h, unsigned char *p, int size) {
	DWORD writesize;    /* �|�[�g�֏������񂾃o�C�g�� */
	//int i;
	//for(i=0;i<size;i++) {
	//	printf("%02X ",p[i]);
	//}
	//printf("\n");
#ifdef windows	
	WriteFile(hComm1, p, size, &writesize, NULL);
#else
	/* �����Ńf�[�^�𑗐M���� */
	int i = 0;
//	printf("size=%d\n",size);
	/* Write a character at a time, only if transmitter is empty */
	while(1) {//�S�����肫��ɂ͂��ꂪ�K�v
		while (PLIB_USART_TransmitterIsEmpty(USART_ID_3)) {
		//while (PLIB_USART_TransmitterIsEmpty(USART_ID_1)) {
			/* Send character */
			PLIB_USART_TransmitterByteSend(USART_ID_3, p[i]);
	//		PLIB_USART_TransmitterByteSend(USART_ID_1, p[i]);
			//printf("%02x ",p[i]);
			/* Increment to address of next character */
			i++;
			if(i == size) {
				//printf("\n");
				return;
			}
		}
	}

#endif
}

/* ��M���荞�ݏ���(�V���A���o�R) */
void RcvIntCom(void* sss)
{
#ifdef windows	
	int i;
	unsigned long nrecv;
	char rbuf[BUFSIZ+1];
	while(1) {
		ReadFile( hComm1, rbuf, 1, &nrecv, 0 ); // �V���A���|�[�g�ɑ΂���ǂݍ���
		if (nrecv !=0) {
//			printf("yamazaki nrecv = %d\n",nrecv);
		}
		for ( i = 0; i < nrecv; i++) {
			//printf("%X ",rbuf[i]&0xff);
//			enqueue(CONTROLER_QUEUE, rbuf[i]);
			enqueue(UART1_QUEUE, rbuf[i]);/* kaji20170306 */
		}
		//printf("\n");
	}
#endif
}

//-------------------------------------------------------------------------------------------
/* 10MS�����^�C�}���荞�ݏ��� */
void TimerInt(void)
{
	int count = 0;
#ifdef windows	
	end = clock();
	if ((end - start) > 0) {
		count = end - start;
		start = clock();
	}
	
	/* �����ŃL�[���͂̃L���[�C���O���s�� */
	if (kbhit()) {
		enqueue(CONSOLE_QUEUE, getch());
	}
	
#else
	/* yamazaki */
	/* �����Ń^�C�}�[�̍X�V���s�� */
    count = 1;
#endif
	
	if (my_tanmatsu_type == CONTA_ADDRESS) {
		/* ����@A */
		TimerIntIO(count);/* IO�֘A�̃^�C�}���荞�ݏ��� */
		TimerIntCont(count);/* ����@���ʂ̃^�C�}���荞�ݏ��� */
		TimerIntContA(count);/* ����@A�̃^�C�}���荞�ݏ��� */
		TimerIntL2(count);/* kaji20170306 */
		TimerIntL2_monitor(count);/* kaji20170310 */
	} else if (my_tanmatsu_type == 0x10) {
		/* �Ď��� */
		TimerIntMonitor(count);/* �Ď��Ղ̃^�C�}���荞�ݏ��� */
		TimerIntL2_monitor(count);/* kaji20170310 */
	} else {
		/* ����@B */
		TimerIntIO(count);/* IO�֘A�̃^�C�}���荞�ݏ��� */
		TimerIntCont(count);/* ����@���ʂ̃^�C�}���荞�ݏ��� */
		TimerIntContB(count);/* ����@B�̃^�C�}���荞�ݏ��� */
		TimerIntL2(count);/* kaji20170306 */
	}
#ifndef windows	
    ms_timer++;//yamazaki
#endif
}

/**
 *	@brief SW��Ԃ̓ǂݍ��ݏ���
 *
 *	@retval �Ȃ�
 */
int DGSWRead(void)
{
#ifdef windows
	return 0;
#else
	PORTS_DATA_TYPE     tmp_data1, tmp_data2;
	PORTS_DATA_TYPE dipsw_data = 0;
	tmp_data1 = SYS_PORTS_Read( PORTS_ID_0, APP_DIPSW_PORT1 );
	tmp_data1 &= 0x0000000F;
	tmp_data2 = SYS_PORTS_Read( PORTS_ID_0, APP_DIPSW_PORT2 );
	tmp_data2 >>= 6;
	tmp_data2 &= 0x000000F0;
	dipsw_data = tmp_data2 | tmp_data1;
	return dipsw_data;
#endif
}

/**
 *	@brief LED�\������
 *
 *	@param [d]  LED�ɔ��f����f�[�^
 *
 *	@retval �Ȃ�
 */
void LedOut(int d)
{
#ifdef windows
	return ;
#else
	if (( d & 1) == 0) {
		BSP_LEDOff(BSP_LED_3);
	} else {
		BSP_LEDOn(BSP_LED_3);
	}
	if (( d & 2) == 0) {
		BSP_LEDOff(BSP_LED_4);
	} else {
		BSP_LEDOn(BSP_LED_4);
	}
	if (( d & 4) == 0) {
		BSP_LEDOff(BSP_LED_7);
	} else {
		BSP_LEDOn(BSP_LED_7);
	}
	if (( d & 8) == 0) {
		BSP_LEDOff(BSP_LED_8);
	} else {
		BSP_LEDOn(BSP_LED_8);
	}
#endif
}

/**
 *	@brief �����ݒ菈��
 *
 *	@retval �Ȃ�
 */
static void TimeInit(void)
{
	ms_timer = 0;
	sec_timer = 0;
#ifdef windows	
	start = clock();//1clock 1ms
#endif
}


