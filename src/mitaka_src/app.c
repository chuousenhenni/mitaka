/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	app.c
 *	概要
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
 *					内部定数定義・型定義・マクロ定義
 *===========================================================================================
 */

/*
00:00になった
その日のスケジュールをチェック
休日か？
土曜日か？　Ｙ　表示内容取り出し

祝日か？　Ｙ　有効？
登録されている日か？
変移休止日か？
連動非連動設定

管理構造体の定義が必要
*/
/*
全線自動　スケジュールにしたがって表示する
全線手動　端末Ａからの指令にしたがって表示する
	
端末差制御機Ｂの履歴は受信電文の変化を見て登録する
端末差制御機Ａは機器のステータスの変化を見て登録する
	
	
疑問点　変移開始、終了時間は端末毎に設定するのか	
*/

/*
遠隔モードの場合
	伝送テストモード　指令された設定にする
		
	スケジュール登録済みの場合
		その日の制御方式により動作を決定する
			日；休時
			土曜時
			通常時
				通常開始時間
				変移開始時間
				等の変化時にＩＯ制御を行う
	スケジュール未登録の場合
		端末制御機Ａからの受信電文による動作を行う

手動モードの場合
	スイッチによる操作が行われた場合
		スイッチによる動作を行う

時間による制御
	００：００にスケジュールをクリア
	スケジュール登録依頼を送る

制御　モードが前回のモードと同じ場合は何もしない
　　　　　　　前回のモードと異なる場合はＩＯ制御を行う

履歴　端末制御機からの受信データを前回の受信データと比較し、イベント、エラー状態に変化がある場合は履歴情報として残す。



やるべきこと
履歴
　異常へ変化したときのログ（異常、復旧）
遠隔動作の確認
起動時の動作
停電時、停電復帰時の動作
ＩＯ周りの制御
設定パラメータの実現
	
*/

/*
 *===========================================================================================
 *					内部変数定義
 *===========================================================================================
 */

int my_tanmatsu_type;/* 端末タイプ 1:A,2:B... */
HANDLE hComm1;       /* シリアルポートのハンドル */
int ms_timer;/* msタイマ */
int sec_timer;/* 秒タイマ */
static clock_t start,end;

static char cont_name [][10] = {/* 制御機名 */
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
 *					外部変数定義
 *===========================================================================================
 */


/*
 *===========================================================================================
 *					内部	関数定義
 *===========================================================================================
 */

void NetInit( void);
static void Init(void );/* 初期化処理 */
static void ComInit(void);/* 通信ポート初期化 */
static HANDLE ComCreate(char *com_name);
int DGSWRead(void);/* SW状態の読み込み */
void LedOut(int d);/* LED表示処理 */
static void TimeInit(void);/* 時刻設定 */
void Send(HANDLE h, unsigned char *p, int size);
//void SendCom1(HANDLE h, unsigned char *p, int size);/* 制御機間通信(USART_ID_1 64000bps ODD)への送信処理 */
void SendCom2(HANDLE h, unsigned char *p, int size);/* 運用管理PC間通信(USART_ID_2 115200bps ODD)への送信処理 */
void SendCom3(HANDLE h, unsigned char *p, int size);/* 監視盤間通信(USART_ID_3 1200bps ODD)への送信処理 */
void RcvIntCom(void *s);//受信割り込み処理(シリアル経由)
void TimerInt(void);/* 10MS周期タイマ割り込み処理 */

/*
 *===========================================================================================
 *					グローバル関数定義
 *===========================================================================================
 */
/**
 *	@brief 初期化処理
 *
 *	@retval なし
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
	//printf("\033[2J"); //画面クリア
	//ここでウォッチドッグの有効無効を決定する
	PLIB_WDT_Enable(WDT_ID_0);
	int dgsw = DGSWRead();/* SW状態の読み込み */
	dgsw &= 0xf;
	if ((dgsw >= 1) && (dgsw <=8)) {
		my_tanmatsu_type = dgsw;
	} else {
		my_tanmatsu_type = 0x10;
	}
	LedOut(dgsw);/* LED表示処理 */
    PLIB_RTCC_Enable(RTCC_ID_0);/* RTCイネーブル*/
    PLIB_RTCC_AlarmEnable(RTCC_ID_0); /* Enable alarm */
#endif
	Init( );
	if (my_tanmatsu_type == CONTA_ADDRESS) {
		/* 制御機A */
		printf("制御機%s %d\n", cont_name[my_tanmatsu_type-1], my_tanmatsu_type);
		ContIOInit();/* 制御機のIO初期化処理 */
		RcvCom1Init;/* kaji20170307 */
		RcvCom3Init;/* kaji20170310 */
		ControlerInit();
		ControlerAInit();
	} else if (my_tanmatsu_type == 0x10) {
		/* 監視盤 */
		printf("監視盤\n");
		MonitorIOInit();/* 監視盤のIO初期化処理 */
		RcvCom3Init;/* kaji20170310 */
		MonitorInit();/* 監視盤のLED初期化処理 */
	} else {
		/* 制御機B */
		printf("制御機%s %d\n", cont_name[my_tanmatsu_type-1], my_tanmatsu_type);
		ContIOInit();/* IO初期化処理 */
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
			ControlerASrv();/* 制御機A */
			RcvCom1Srv();/* kaji20170307 */
			RcvCom3Srv();/* kaji20170310 */
		} else if (my_tanmatsu_type == 0x10) {
			MonitorSrv();/* 監視盤 */
			RcvCom3Srv();/* kaji20170310 */
		} else {
			ControlerBSrv();/* 制御機B */
			RcvCom1Srv();/* kaji20170307 */
		}
	//}
	MaintenanceSrv();/* メンテナンス（キー入力コマンド）処理 */
	
#ifdef windows	
	TimerInt();/* タイマ更新処理 */
#else
	if (ms_timer >= 1000) {
        ms_timer = 0;
		sec_timer++;
		if (sec_timer >= 2) {/* 2秒間はDGSWの状態を表示しておく */
	        if ((sec_timer%2) == 0){
	             BSP_LEDOff(BSP_LED_3);
	        } else {
	             BSP_LEDOn(BSP_LED_3);
	        }
		}
	}
    PLIB_WDT_TimerClear(WDT_ID_0);/* ウォッチドッグクリア */

#endif
}

/*
 *===========================================================================================
 *					内部関数定義
 *===========================================================================================
 */
/**
 *	@brief 初期化処理
 *
 *	@retval なし
 */
static void Init(void)
{
#ifdef windows	
	ComInit();/* 通信ポート初期化 */
#endif
	MaintenanceInit();/* デバッグ用のキー入力による処理の初期化処理 */
	TimeInit();/* 時刻設定 */
	init_all_queue();
#ifdef windows	
	_beginthread( RcvIntCom, 0,0 );
#endif
}

/**
 *	@brief 通信ポート初期化処理
 *
 *	@retval なし
 */
static void ComInit(void)
{
	if (my_tanmatsu_type == CONTA_ADDRESS) {
		hComm1 = ComCreate("COM3");/* 制御機A */
	} else {
		hComm1 = ComCreate("COM2");/* 制御機 */
	}
}

//シリアルポートイニシャル
HANDLE ComCreate(char *com_name)
{
	HANDLE handle;
#ifdef windows	
	DCB dcb;            /* 通信パラメータ */
	COMMTIMEOUTS cto;
	
	handle = CreateFile(
	    com_name,                       /* シリアルポートの指定 */
	    GENERIC_READ | GENERIC_WRITE, /* アクセスモード */
	    0,                            /* 共有モード */
	    NULL,                         /* セキュリティ属性 */
	    OPEN_EXISTING,                /* 作成フラグ */
	    FILE_ATTRIBUTE_NORMAL,        /* 属性 */
	    NULL                          /* テンプレートのハンドル */
	); 
	if ( handle == INVALID_HANDLE_VALUE ) {
		printf("%s Open Error!\n", com_name);
	} else {
		printf("%s Opend!\n", com_name);
	}
	printf("%s\n",com_name);
	GetCommState(handle, &dcb); /* DCB を取得 */
//	dcb.BaudRate = 9600;        //通信速度
	dcb.BaudRate = 115200;        //通信速度
	dcb.ByteSize = 8;            //データ長
	dcb.Parity = ODDPARITY;       // パリティビット：EVENPARITY,MARKPARITY,NOPARITY,ODDPARITY
	dcb.StopBits = ONESTOPBIT;   // ストップビット：ONESTOPBIT,ONE5STOPBITS,TWOSTOPBITS
	SetCommState(handle, &dcb); /* DCB を設定 */
	/* ----------------------------------------------
	    シリアルポートのタイムアウト状態操作
	---------------------------------------------- */
	GetCommTimeouts( handle, &cto );           // タイムアウトの設定状態を取得
	cto.ReadIntervalTimeout = 10;       //ms(0でタイムアウトなし)
	cto.ReadTotalTimeoutMultiplier = 0;   //Read : 1バイトに対するタイムアウト乗数(0でタイムアウトなし)
	cto.ReadTotalTimeoutConstant = 10;  //Read : 0バイト時のタイムアウト定数(0でタイムアウトなし)
	cto.WriteTotalTimeoutMultiplier = 0;  //Write : 1バイトに対するタイムアウト乗数(0でタイムアウトなし)
	cto.WriteTotalTimeoutConstant = 0;    //Write : 0バイト時のタイムアウト定数(0でタイムアウトなし)
	SetCommTimeouts( handle, &cto );           // タイムアウトの状態を設定
#endif
	return handle;
}

/**
*	@brief 制御機間通信(USART_ID_1 64000bps ODD)への送信処理
 *
 *	@param [p]  送信データ格納ポインタセンサID
 *	@param [sizie]  格納サイズ
 *
 *	@retval なし
 */
void Send(HANDLE h, unsigned char *p, int size) {
}
//void SendCom1(HANDLE h, unsigned char *p, int size) {  kaji20170306 L2.cへ移行
static void non_SendCom1(HANDLE h, unsigned char *p, int size) {
	DWORD writesize;    /* ポートへ書き込んだバイト数 */
	//DebugPrint("", "制御機間通信", 0x20);
	//int i;
	//for(i=0;i<size;i++) {
	//	printf("%02X ",p[i]);
	//}
	//printf("\n");
#ifdef windows	
	WriteFile(hComm1, p, size, &writesize, NULL);
#else
	/* ここでデータを送信する */
	int i = 0;
	//printf("SendCom1 size=%d\n",size);
	while(1) {//全部送りきるにはこれが必要
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
 *	@brief 運用管理PC間通信(USART_ID_2 115200bps ODD)への送信処理
 *
 *	@param [p]  送信データ格納ポインタセンサID
 *	@param [sizie]  格納サイズ
 *
 *	@retval なし
 */
void SendCom2(HANDLE h, unsigned char *p, int size) {
	DWORD writesize;    /* ポートへ書き込んだバイト数 */
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
	/* ここでデータを送信する */
	int i = 0;
	/* Write a character at a time, only if transmitter is empty */
	while(1) {//全部送りきるにはこれが必要
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
 *	@brief 監視盤間通信(USART_ID_3 1200bps ODD)への送信処理
 *
 *	@param [p]  送信データ格納ポインタセンサID
 *	@param [sizie]  格納サイズ
 *
 *	@retval なし
 */
int send_com3_p = 0;
//void SendCom3(HANDLE h, unsigned char *p, int size) {
static void nonSendCom3(HANDLE h, unsigned char *p, int size) {
	DWORD writesize;    /* ポートへ書き込んだバイト数 */
	//DebugPrint("", "監視盤間通信", 0x20);
	//int i;
	//for(i=0;i<size;i++) {
	//	printf("%02X ",p[i]);
	//}
	//printf("\n");
#ifdef windows	
	WriteFile(hComm1, p, size, &writesize, NULL);
	send_com3_p = size;
#else
	/* ここでデータを送信する */
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
//			MdmcsWrite(0);/* MDM_CSの出力処理 */
			return;
		}
	}

#endif
}

void SendCom3_all(HANDLE h, unsigned char *p, int size) {
	DWORD writesize;    /* ポートへ書き込んだバイト数 */
	//int i;
	//for(i=0;i<size;i++) {
	//	printf("%02X ",p[i]);
	//}
	//printf("\n");
#ifdef windows	
	WriteFile(hComm1, p, size, &writesize, NULL);
#else
	/* ここでデータを送信する */
	int i = 0;
//	printf("size=%d\n",size);
	/* Write a character at a time, only if transmitter is empty */
	while(1) {//全部送りきるにはこれが必要
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

/* 受信割り込み処理(シリアル経由) */
void RcvIntCom(void* sss)
{
#ifdef windows	
	int i;
	unsigned long nrecv;
	char rbuf[BUFSIZ+1];
	while(1) {
		ReadFile( hComm1, rbuf, 1, &nrecv, 0 ); // シリアルポートに対する読み込み
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
/* 10MS周期タイマ割り込み処理 */
void TimerInt(void)
{
	int count = 0;
#ifdef windows	
	end = clock();
	if ((end - start) > 0) {
		count = end - start;
		start = clock();
	}
	
	/* ここでキー入力のキューイングを行う */
	if (kbhit()) {
		enqueue(CONSOLE_QUEUE, getch());
	}
	
#else
	/* yamazaki */
	/* ここでタイマーの更新を行う */
    count = 1;
#endif
	
	if (my_tanmatsu_type == CONTA_ADDRESS) {
		/* 制御機A */
		TimerIntIO(count);/* IO関連のタイマ割り込み処理 */
		TimerIntCont(count);/* 制御機共通のタイマ割り込み処理 */
		TimerIntContA(count);/* 制御機Aのタイマ割り込み処理 */
		TimerIntL2(count);/* kaji20170306 */
		TimerIntL2_monitor(count);/* kaji20170310 */
	} else if (my_tanmatsu_type == 0x10) {
		/* 監視盤 */
		TimerIntMonitor(count);/* 監視盤のタイマ割り込み処理 */
		TimerIntL2_monitor(count);/* kaji20170310 */
	} else {
		/* 制御機B */
		TimerIntIO(count);/* IO関連のタイマ割り込み処理 */
		TimerIntCont(count);/* 制御機共通のタイマ割り込み処理 */
		TimerIntContB(count);/* 制御機Bのタイマ割り込み処理 */
		TimerIntL2(count);/* kaji20170306 */
	}
#ifndef windows	
    ms_timer++;//yamazaki
#endif
}

/**
 *	@brief SW状態の読み込み処理
 *
 *	@retval なし
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
 *	@brief LED表示処理
 *
 *	@param [d]  LEDに反映するデータ
 *
 *	@retval なし
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
 *	@brief 時刻設定処理
 *
 *	@retval なし
 */
static void TimeInit(void)
{
	ms_timer = 0;
	sec_timer = 0;
#ifdef windows	
	start = clock();//1clock 1ms
#endif
}


