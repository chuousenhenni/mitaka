/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  EXS CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	Console.c
 *	概要
 *
 * コンソールの制御を行う。
 * ＳＷ１＝ＯＮで起動
 *
 * キー入力を受付、デバッグ用の変更、表示処理を行う
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
#include "maintenance.h"					/* 自分自身のヘッダはここでインクルード */

/*
 *===========================================================================================
 *					内部定数定義・型定義・マクロ定義
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
 *					内部変数定義
 *===========================================================================================
 */

/*
	thread_debug_flgの説明
	DB0:送受信データを表示する
	DB1:エラーを表示する
	DB2:All Dataログを表示する
	DB3:センサID管理機能ログを表示する
	
	関数DebugPrintと組み合わせて使用
*/
//int debug_flg = 0xf;/* デバッグ用の表示を行う場合に0以外をセットする */
int debug_flg_before=0;/* デバッグ用のフラグ保持用 */
static int count = 0;
static int bs_flg = 0;

/* キー入力履歴用バッファ */
static int hp = 0;
static int hsavep = 0;/* 履歴をセーブするポイント */
static int hp_count = 0;/* 履歴トータル数 */
static int key_count = 0;/* 一回のコマンド時のキー入力数 */
#define HP_MAX (10)
static char history[HP_MAX][100];
int swtest;/* SWテスト用データ */

int command_count = 0;
char command [ 20 ][ 20 ];
int bef_ms_timer;

/*
 *===========================================================================================
 *					外部変数定義
 *===========================================================================================
 */

extern int my_tanmatsu_type;
extern int rtc_sync_timer;
extern int board_chatter_timer[0];/* チャタ防止標識版ＩＯリードタイマー */

/*
 *===========================================================================================
 *					内部	関数定義
 *===========================================================================================
 */

static char *DebugTimePrint(void);
static void HistoryUp(void);/* ↑矢印が押された時の処理 */
static void HistoryDown(void);/* ↓矢印が押された時の処理 */
static void HistoryInc(void);/* 履歴取り出しポイント＋１処理 */
static void HistoryDec(void);/* 履歴取り出しポイントー１処理 */

static void CommandSrv(char *str);/* コマンド解析実行処理 */
static void CommandA(void);/* 制御応答コマンド表示処理 */
static void CommandHi(void);/* 入力履歴表示処理 */
static void CommandB(void);/* 同報制御コマンドの表示 */
static void CommandR(void);/* テスト用ランダムデータ送信処理 */
static void CommandN(void);/* 通常制御コマンドの表示 */
static void CommandMv(void);/* 可変表示板回転処理 */
static void CommandMd(void);/* メモリダンプ処理 */
static void CommandDefault(void);/* パラメータディフォルト設定処理 */
static void CommandStatus(void);/* ステータス表示処理 */
static void CommandParam(void);/* パラメータ表示処理 */
static void CommandErase(void);/* フラッシュメモリセクター消去処 */
static void CommandFwd(void);/* Fwdコマンド処理 */
//static void CommandFws(void);/* Fwsコマンド処理 */
static void CommandOut(void);/* IO出力処理 */
static void CommandIn(void);/* IO入力処理 */
static void E2ROM_Write(void);/* EEPROM書き込み処理 */
static void E2ROM_Read(void);/* EEPROM書き込み処理 */
static void CommandLed(void);/* BSPLEDの表示処理 */
static void CommandTime(void);/* 時刻設定表示処理 */
static void CommandI(void);/* デバッグ用の表示板入力値の設定処理 */
static void CommandDgsw(void);/* BSPデジスイッチの読み込み処理 */
static void CommandSw(void);/* デバッグ用のスイッチ設定表示処理 */
static void CommandP(void);/* デバッグ用の停電ステータス設定表示処理 */
static void CommandMtest(void);/* 可変表示板モータの制御処理 */
static void CommandVersion(void);/* バージョン表示処理 */
static void CommandH(void);/* ヘルプ表示処理 */
static void CommandW(void);/* テスト用無限ループ処理 */
static void CommandD(void);/* デバッグフラグ切り替え処理 */
static void CommandCheckStartTime(void);/* その日の状態を獲得する処理 */
static void CommandTest(void);/* 処理 */
static void CommandQ(void);/* exit処理 */
static TComandList g_command_list[] = {
    {"a", 1, CommandA, "監視応答ダンプ"},
    {"hi", 2, CommandHi, "入力履歴"},
    {"b", 1, CommandB, "同報指令ダンプ"},
    {"r", 1, CommandR, "テスト用ランダムデータ送信"},
    {"n", 1, CommandN, "通常指令ダンプ"},
    {"mv", 2, CommandMv, "可変boardモーターオン"},
    {"md", 2, CommandMd, "メモリダンプ"},
    {"default", 5, CommandDefault, "パラメータディフォルト設定処理"},
    {"status", 2, CommandStatus, "ステータス処理"},
    {"param", 2, CommandParam, "パラメータ処理"},
    {"erase", 5, CommandErase, "フラッシュメモリセクターイレーズ処理"},
    {"fwd", 3, CommandFwd, "フラッシュメモリライトコマンド"},
//    {"fws", 3, CommandFws, "コマンド"},
    {"out", 1, CommandOut, "IO出力処理"},
    {"in", 2, CommandIn, "IO入力処理"},
	{"eew", 3, E2ROM_Write, "EEPROM書き込み処理"},
	{"eer", 3, E2ROM_Read, "EEPROM読み出し処理"},
    {"led", 1, CommandLed, "LED状態,BSPLEDの制御処理"},
    {"test", 4, CommandTest, "test処理"},
    {"time", 1, CommandTime, "時刻設定処理"},
    {"i", 1, CommandI, "デバッグ用の入力値の設定処理"},
    {"dgsw", 2, CommandDgsw, "BSPデジスイッチの読み込み処理"},
    {"sw", 2, CommandSw, "デバッグ用のスイッチ設定"},
    {"p", 2, CommandP, "デバッグ用の停電ステータス設定処理"},
    {"mtest", 2, CommandMtest, "デバッグ用の運用管理PC停止フラグ設定"},
    {"version", 3, CommandVersion, "バージョン処理"},
    {"h", 1, CommandH, "ヘルプ処理"},
    {"w", 1, CommandW, "テスト用無限ループ処理"},
    {"d", 1, CommandD, "デバッグフラグ切り替え処理"},
    {"c", 1, CommandCheckStartTime, "その日の状態を獲得する処理"},
    {"q", 1, CommandQ, "exit処理"},
};
static unsigned long ToDec(const char str[ ]);/* 16 進文字列を 10 進数に変換する処理 */

/*
 *===========================================================================================
 *					グローバル関数定義
 *===========================================================================================
 */

void MaintenanceInit(void );/* デバッグ用のキー入力による処理の初期化処理 */
void MaintenanceSrv(void);/* デバッグ用のキー入力処理 */
void DebugPrint(char *str1, char *str2, int mask);/* デバッグ用の表示処理 */

/*
 *===========================================================================================
 *					外部	関数定義
 *===========================================================================================
 */

/*
 *===========================================================================================
 *					グローバル関数
 *===========================================================================================
 */

/**
 *	@brief デバッグ用のキー入力による処理の初期化処理
 *
 *	@retval なし
 */
void MaintenanceInit(void )
{
	/* 履歴をあらかじめ登録しておく */
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
 *	@brief デバッグ用のキー入力による処理
 *
 *	@retval なし
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
				//printf("UP\n");//↑矢印
				HistoryUp();/* ↑矢印の処理 */
			} else if (c == 0x50) {
				//printf("DOWN\n");//↓矢印
				HistoryDown();/* ↓矢印の処理 */
			}
			c = dequeue(CONSOLE_QUEUE);
			c = dequeue(CONSOLE_QUEUE);
			return;
		} else if( c == 0x1B) {
			/* テラタームの場合はESCシーケンスとなる */
			/*
			上矢印は1B,5B,41
			下矢印は1B,5B,42
			
			*/
			if (lenqueue(CONSOLE_QUEUE) < 3) {
				return;
			}
			if ((peek(CONSOLE_QUEUE, 1) == 0x5B)  && (peek(CONSOLE_QUEUE, 2) == 0x41)){
				//printf("UP\n");//↑矢印
				HistoryUp();/* ↑矢印の処理 */
			} else if ((peek(CONSOLE_QUEUE, 1) == 0x5B)  && (peek(CONSOLE_QUEUE, 2) == 0x42)){
				//printf("DOWN\n");//↓矢印
				HistoryDown();/* ↓矢印の処理 */
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
		if ( c==10 || c==13 ) { /* '\r'や'\n'を受信すると文字列を閉じる */
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
					HistoryInc();/* マイナスしたんでプラスする */
				} else {
					HistoryDec();/* プラスしたんでマイナスする */
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
 *	@brief ↑矢印が押された時の処理
 *
 *	@retval なし
 */
static void HistoryUp(void)
{
	int i;
	HistoryDec();/* 履歴取り出しポイントー１処理 */
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
 *	@brief ↓矢印が押された時の処理
 *
 *	@retval なし
 */
static void HistoryDown(void)
{
	int i;
	HistoryInc();/* 履歴取り出しポイント＋１処理 */
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
 *	@brief 履歴取り出しポイント＋１処理
 *
 *	@retval なし
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
 *	@brief 履歴取り出しポイントー１処理
 *
 *	@retval なし
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
 *	@brief コマンド解析実行処理
 *
 *	@retval なし
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
 * @brief Aコマンド処理
 *
 * @return なし
 */
static void CommandA(void)
{
	ResponseDisp( );/* 送信用 */
}

/**
 * @brief Hiコマンド処理
 *
 * @return なし
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
 * @brief Bコマンド処理
 *
 * @return なし */

static void CommandB(void)
{
	/* 同報制御コマンドの表示 */
	BroadcastDisp();/* 送信用 */
}

/**
 * @brief Rコマンド処理
 *
 * @return なし
 */
static void CommandR(void)
{
// kaji20170221↓
#ifdef windows	
	extern HANDLE hComm1;       /* シリアルポートのハンドル */
	extern void SendCom1(HANDLE h, unsigned char *p, int size);
	char random[100];
	int i;
	srand((unsigned)time(NULL));
	for(i=0;i<100;i++){  random[i] = rand();  }
	printf("send random data\n");
	SendCom1(hComm1, (unsigned char *)random, sizeof(random));
#endif
// kaji20170221↑
}

/**
 * @brief Nコマンド処理
 *
 * @return なし
 */
static void CommandN(void)
{
	/* 通常制御コマンドの表示 */
	NormalDisp( );/* 送信用 */
}

/**
 * @brief Mvコマンド処理
 *
 * @return なし
 */
static void CommandMv(void)
{
#ifndef windows
	/* 可変表示版制御コマンドの表示 */
	extern int ms_timer;/* msタイマ */
	extern int sec_timer;/* 秒タイマ */

	int no = 1;
	int d = 0;
	if (command_count >= 2) {
		no = atoi(command[1]);
		if ((no<=0) || (no>8)) {
			printf("illeagal board no%d\n",no);
			return;
		}
	}
	//yamazakiBoardWrite(no, 3);/* ＯＮコマンド */
	BoardWrite(no-1, 2);/* ＯＮコマンド */
	printf("MOVE NO. %d UNTIL PUSH ANY KEY\n", no);
	board_chatter_timer[no-1] = 0;/* チャタ防止標識版ＩＯリードタイマー */
	d = BoardRead( no-1 );
	printf("%d.%.03dsec REG = %.04X\n", sec_timer, ms_timer, d);
	init_queue(CONSOLE_QUEUE);/* まず空にする */
	while(empty(CONSOLE_QUEUE)) {
		PLIB_WDT_TimerClear(WDT_ID_0);/* ウォッチドッグクリア */
		if (d != BoardRead( no-1 )) {
			d = BoardRead( no-1 );
//			printf("%d.%.05d lap = %.05dms REG = %.04X\n", sec_timer, ms_timer, ms_timer - bef_ms_timer, d);
			printf("%.05d lap = %.05dms REG = %.04X\n", ms_timer, ms_timer - bef_ms_timer, d);
			bef_ms_timer = ms_timer;
		}
	}
	dequeue(CONSOLE_QUEUE);
	BoardWrite(no-1, 0);/* ＯＦＦコマンド */
#endif
	printf("END\n");
}

/**
 * @brief Mdコマンド処理
 *
 * @return なし
 */
static void CommandMd(void)
{
	/* メモリダンプコマンドの表示 */
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
			adr |= 0x80000000;/* VMEMに変更 */
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
 * @brief Defaultコマンド処理
 *
 * @return なし
 */
static void CommandDefault(void)
{
	printf("ディフォルトパラメータ設定\n");
	SetDefault();/* ディフォルトパラメータ設定 */
}

/**
 * @brief Statusコマンド処理
 *
 * @return なし
 */
static void CommandStatus(void)
{

	if (my_tanmatsu_type == 0x10) {
		MonitorStatusDisp();/* ステータスの表示処理 */
	} else {
		if (command_count >= 2) {
			if (strncmp(command[1], "write", 1) == 0) {
				printf("ステータスのセーブ処理\n");
				SaveStatus();/* ステータスのセーブ処理 */
			}
			if (strncmp(command[1], "read", 1) == 0) {
				printf("ステータスのロード処理\n");
				LoadStatus();/* ステータスのロード処理 */
			}
		}
		StatusDisp();/* ステータスの表示処理 */
	}
}

/**
 * @brief Paramコマンド処理
 *
 * @return なし
 */
static void CommandParam(void)
{
	extern PARAM param;
	if (command_count == 2) {
		if (strncmp(command[1], "write", 1) == 0) {
			printf("パラメータのセーブ処理\n");
			SaveParam();/* パラメータのセーブ処理 */
			printf("パラメータのセーブ処理 終了\n");
		}else if (strncmp(command[1], "read", 1) == 0) {
			printf("パラメータのロード処理\n");
			LoadParam();/* パラメータのロード処理 */
		}else if (strncmp(command[1], "default", 1) == 0) {
			printf("パラメータのディフォルト設定処理\n");
			SetDefault();
		}
	} else if (command_count == 3) {
		if (strncmp(command[1], "offset_timer", 2) == 0) {
			/* オフセットタイマの値 */
			int t = atoi(command[2]);
			param.offset_timer = t;
		} else if (strncmp(command[1], "nomusen", 6) == 0) {
			/* 無線エラー時エラーとするや否や 1:無線エラーとしない */
			int t = atoi(command[2]);
			param.no_musen_error_flag = t;
		} else if (strncmp(command[1], "nomove", 6) == 0) {
			/* 標識のdisplay内容が同じ場合移動するや否や 1:移動しない */
			int t = atoi(command[2]);
			param.same_nomove_flag = t;
		} else if (strncmp(command[1], "nofail", 6) == 0) {
			/* ファイル発生時フェイルとするや否や 1:フェイルとしない */
			int t = atoi(command[2]);
			param.no_fail_flag = t;
		} else if (strncmp(command[1], "noboard", 6) == 0) {
			/* boardエラー時エラーとするや否や 1:エラーとしない */
			int t = ToDec(command[2]);
			param.no_board_error_flag = t;
		} else if (strncmp(command[1], "nopc", 4) == 0) {
			/* 運用管理ＰＣ間通信異常を判定をチェックするや否や 1:エラーとしない */
			int t = ToDec(command[2]);
			param.no_pc_check_flag = t;
		} else if (strncmp(command[1], "linkage", 4) == 0) {
			/* 
				制御機Bの有効無効設定
				例) 00011000 制御機4,5が有効(5は制御機Aなので必須)
			*/
			if (strlen(command[2]) == CONTROLER_MAX) {
				int t = strtol(command[2], 0, 16);
				param.linkage_status = t;
			} else {
				printf("linkage length is not %d\n",CONTROLER_MAX);
			}
		} else if (strncmp(command[1], "delay", 3) == 0) {
			/* MDM_CS出力の遅延時間(ms) */
			int t = atoi(command[2]);
			param.mdmcs_delay_time = t;/* MDM_CS出力の遅延時間(ms) */
		} else if (strncmp(command[1], "reset", 5) == 0) {
			/* reset_count */
			int t = atoi(command[2]);
			param.reset_count = t;/* リセットカウント */
		} else if (strncmp(command[1], "respi", 5) == 0) {
			/* 20170305 制御機Aからの要求から制御機Bからの応答受信確認までの最小待ち時間(ms) */
			int t = atoi(command[2]);
			param.response_interval_time_val = t;
		} else if (strncmp(command[1], "respt", 5) == 0) {
			/* 20170305 制御機Aからの要求から制御機Bからの応答受信までのタイムアウト値(ms) */
			int t = atoi(command[2]);
			param.response_timeout_val = t;
		} else if (strncmp(command[1], "musent", 6) == 0) {
			/* 20170305 無線通信の受信エラータイムアウト値(秒) */
			int t = atoi(command[2]);
			param.musen_timeout_val = t;
		} else if (strncmp(command[1], "preamble", 3) == 0) {
			/* 20170308 L2.cのプリアンブルパターン */
			int t = strtol(command[2], 0, 16) & 0xffffUL;
			param.preamble_ptn = t;
		}
	} else if (command_count == 4) {
		if (strncmp(command[1], "start_time", 2) == 0) {
			/* 変移開始時間 */
			int hour = atoi(command[2]);
			int min = atoi(command[3]);
			param.start_time = 60 * hour + min;
		} else if (strncmp(command[1], "end_time", 2) == 0) {
			/* 変移終了時間 */
			int hour = atoi(command[2]);
			int min = atoi(command[3]);
			param.end_time = 60 * hour + min;
		}
	}
	printf("変移開始,終了       = %.02d:%.02d %.02d:%.02d %d\n"
														, (param.start_time/60) / 60, (param.start_time/60) % 60//yamazaki
														, (param.end_time/60) / 60, (param.end_time/60) % 60//yamazaki
														, param.offset_timer);
	//printf("時刻修正時刻        = %d\n",param.time_correction_time);
	//printf("時刻修正要求時刻    = %d\n",param.time_correction_request_time);
	//printf("日・休パターン      = %d\n",param.holiday_pattern);
	//printf("日・休開始,終了     = %.02d:%.02d %.02d:%.02d\n"
	//										,(param.holiday_start_time/60) / 60, (param.holiday_start_time/60) % 60
	//										,(param.holiday_end_time/60) / 60, (param.holiday_end_time/60) % 60);
	//printf("土曜日,休日モード   = %d,%d\n",param.saturday_mode,param.holiday_mode);
	printf("フェイルパターン    = %d\n",param.fail_pattern);
	printf("linkage_status      = %.08X (12345678)\n",param.linkage_status);
//	printf("オフセットタイマ = %d\n",param.offset_timer);
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
 * @brief Eraseコマンド処理
 *
 * @return なし
 */
static void CommandErase(void)
{
#ifndef windows
	/* フラッシュイレーズ */
	if (command_count >= 2) {
		int adr = ToDec(command[ 1 ]);
		printf("flash sector erase ADDRESS = %.8X",adr);
		flash_sector_erase(adr);
	}
#endif
}

/**
 * @brief Fwdコマンド処理
 *
 * @return なし
 */
static void CommandFwd(void)
{
#ifndef windows
	/* フラッシュライトテスト */	
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
 * @brief Fwsコマンド処理
 *
 * @return なし
 */
static void CommandFws(void)
{
#ifndef windows
	/* フラッシュセクターライトテスト */	
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
 * @brief Outコマンド処理
 *
 * @return なし
 */
static void CommandOut(void)
{
	/* IO出力 */
	if (command_count > 2) {
		//int adr = strlen(command[1]);
		int adr = ToDec(command[ 1 ]);
		int d = ToDec(command[2]);
		RegWrite(adr, d);
		printf ("out %X = %04X\n",adr, d);
	}
}

/**
 * @brief Inコマンド処理
 *
 * @return なし
 */
static void CommandIn(void)
{
	/* IO入力 */
	if (command_count > 1) {
		int adr = ToDec(command[ 1 ]);
		int d;
		d = RegRead(adr);
		printf ("in %X = %04X\n",adr, d);
	}
}

/**
 * @brief EEPROM書き込みコマンド処理
 *
 * @return なし
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
 * @brief EEPROM読み出しコマンド処理
 *
 * @return なし
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
 * @brief Ledコマンド処理
 *
 * @return なし
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
	/* IO入力 */
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
 * @brief Timeコマンド処理
 *
 * @return なし
 */
static void CommandTime(void)
{
	TIME_INFO t,t1,t2;
	char str[256];

	/* 時刻設定 112233 */
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
			SetTime(&t);/* PIC内蔵RTCに書き込む */
			SaveRTC(&t);/* 不揮発用RTCに書き込む */
		} else {
			printf("parameter error len=14 exp. 20170102030405\n");
		}
	}
	
	LoadRTC(&t1);/* 不揮発用RTCから読み出す */
	SetNowTime(&t2);/* PIC内蔵RTCから読み出す */
	sprintf(str,"時刻(RTC)　%02X%02X/%02X/%02X(%d) %02X:%02X:%02X"
		,t1.year[0]
		,t1.year[1]
		,t1.month
		,t1.day
		,t1.holiday_week.week
		,t1.hour
		,t1.min
		,t1.sec);
	printf("%s\n", str);
	sprintf(str,"時刻(CPU)　%02X%02X/%02X/%02X(%d) %02X:%02X:%02X"
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
 * @brief Iコマンド処理
 *
 * @return なし
 */
static void CommandI(void)
{
	/* 擬似のIO入力データを設定する */
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
 * @brief Dgswコマンド処理
 *
 * @return なし
 */
static void CommandDgsw(void)
{
	/* ボード上DGSW状態の表示 */
	extern int DGSWRead();
	int dgsw = DGSWRead();
		printf("dgsw = %02X\n",dgsw);
}

/**
 * @brief Swコマンド処理
 *
 * @return なし
 */
static void CommandSw(void)
{
	/* SW制御コマンドの表示(テスト用) */
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
 * @brief Pコマンド処理
 *
 * @return なし
 */
static void CommandP(void)
{
	extern int io_power_outage_flag;/* 停電発生フラグ */
	if (command_count > 1) {
		io_power_outage_flag = strtol(command[1], 0, 16);
	}
	printf("io_power_outage_flag = %02X\n",io_power_outage_flag);
}

/**
 * @brief Mtestコマンド処理
 *
 * @return なし
 */
static void CommandMtest(void)
{
	extern int monitor_command_flg;
	extern int monitor_timeout_flag;
	extern int monitor_led_status[ MONITOR_LED_MAX_COUNT];
	extern MONITOR_COMMAND monitor_command;/* 制御機からの制御指令の電文  */
	int i;
	if (command_count == 2) {
		monitor_command_flg = strtol(command[1], 0, 16);
	} else if (command_count == 4) {
		/* monitor_commandパッチ */
		int no = atoi(command[1]);
		int sts = atoi(command[2]);
		int d = atoi(command[3]);
		monitor_command.command[no][sts] = d;
	}
	MonitorCommandDisp();/* 監視盤への制御指令表示処理 */
	printf("monitor_timeout_flag = %d\n",monitor_timeout_flag);
	printf("monitor_command_flg = %d\n",monitor_command_flg);
	for(i=0;i< MONITOR_LED_MAX_COUNT;i++) {
		printf("%d ",monitor_led_status[i]);
	}
	printf("\n");
}

/**
 * @brief Versionコマンド処理
 *
 * @return なし
 */
static void CommandVersion(void)
{
	/* バージョン表示 */
	printf("version        = \"%s %s\"\n",VERSION_DATE, VERSION_TIME);
	printf("FPGAversion    = %.8X\n",FPGAVersionRead());
	printf("CPUFPGAversion = %.8X\n",CPUFPGAVersionRead());
}

/**
 * @brief Hコマンド処理
 *
 * @return なし
 */
static void CommandH(void)
{
	printf("a                監視応答ダンプ\n");
	printf("hi               入力履歴\n");
	printf("b                同報指令ダンプ\n");
	printf("r                com1へのテスト用ランダムデータ送信処理\n");
	printf("n                通常指令ダンプ\n");
	printf("mv [no]          可変boardモーターオン\n");
	printf("md adr [size]    メモリダンプ\n");
	printf("default           ディフォルトパラメータ設定\n");
	printf("status write|read ステータスのロード、セーブ\n");
	printf("param write|read パラメータのロード、セーブ\n");
	printf("erase adr        フラッシュイレーズ\n");
	printf("fwd adr          フラッシュライトテスト\n");
	//printf("fws adr data       フラッシュ	ライトテスト\n");
	printf("o xx            IOポート出力\n");
	printf("in              IOポート入力\n");
	printf("eew adr data    E2PROM書き込み\n");
	printf("eer adr         E2PROM読み出し\n");
	printf("led 34|7|8 [0|1] LED状態,BSPLED制御\n");
	printf("test            test処理\n");
	printf("time [hhmmss]   時刻設定読み出し\n");
	printf("i               デバッグ用の入力値の設定\n");
	printf("dgsw            BSPデジスイッチの読み込み処理\n");
	printf("sw              デバッグ用のスイッチ設定\n");
	printf("p [0|1]         デバッグ用の停電ステータス設定処理\n");
	printf("mtest [0|1]     デバッグ用の運用管理PC停止フラグ設定\n");
	printf("version         バージョン\n");
	printf("w               テスト用無限ループ処理\n");
	printf("d XX            デバッグフラグ切り替え\n");
	printf("c YYYYMMDD      その日の状態を獲得する処理\n");
	printf("quit application exit !!!\n");
	printf("help ヘルプ\n");
}

/**
 * @brief Wコマンド処理
 *
 * @return なし
 */
static void CommandW(void)
{
    printf("waitkey\n");
#ifndef windows	
	init_queue(CONSOLE_QUEUE);/* まず空にする */
	while(empty(CONSOLE_QUEUE)) {
	}
	dequeue(CONSOLE_QUEUE);
#endif
	printf("waitkey end \n");
}

/**
 * @brief Dコマンド処理
 *
 * @return なし
 */
static void CommandD(void)
{
	/* d コマンド　デバッグフラグセット */
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
 * @brief その日の状態を獲得する処理
 *
 * @return なし
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
				tb.week = subZeller(tb.year, tb.month, tb.day)+1;//日曜日を１とする
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
		tb.week = subZeller(tb.year, tb.month, tb.day)+1;//日曜日を１とする
	
		ret = CheckStartTime(&tb);
		printf("CommandCheckStartTime ret = %d %s\n",ret,ret ? "TODAY_IS_HOLIDAY" : "TODAY_IS_NORMAL_DAY");
	} else {
		extern HOLIDAY_DATA holiday_data;/* 変移休止日管理用 */
		int i;
		printf("祝日リスト\n");
		for(i=0; i < holiday_data.holiday_count;i++){
			printf("%2d:%2d %2d %d\n",i+1
				,holiday_data.holiday[i].month
				,holiday_data.holiday[i].day
				,holiday_data.holiday[i].type);
		}
		printf("変移休止日リスト\n");
		for(i=0; i < holiday_data.kyuushi_count;i++){
			printf("%2d:%2d %2d %d\n",i+1
				,holiday_data.kyuushi[i].month
				,holiday_data.kyuushi[i].day
				,holiday_data.kyuushi[i].type);
		}
	}
}


/**
 * @brief testコマンド処理
 *
 * @return なし
 */
static void CommandTest(void)
{
	printf("rtc_sync_timer=%d\n",rtc_sync_timer);
	printf("CommandTest %X %X\n",rtc_sync_timer,&rtc_sync_timer);
#ifndef windows //yamazaki*	
	//if (command_count == 1) {
	//    PLIB_RTCC_Enable(RTCC_ID_0);/* RTCイネーブル*/
	//} else if (command_count == 2) {
	//    PLIB_RTCC_AlarmEnable(RTCC_ID_0); /* Enable alarm */
	//} else {
	//    PLIB_RTCC_Enable(RTCC_ID_0);/* RTCイネーブル*/
	//    PLIB_RTCC_AlarmEnable(RTCC_ID_0); /* Enable alarm */
	//}
	//if (command_count == 2) {
	//	int d = strtol(command[1], 0, 16);
	//	printf("PcPower %d\n",d);
	//	PcPower(d);
	//}

	
	//extern HOLIDAY_DATA holiday_data;/* 変移休止日管理用 */
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
 * @brief Qコマンド処理
 *
 * @return なし
 */
static void CommandQ(void)
{
	exit(0);
}

/**
 *	@brief デバッグ用の表示処理
 *         時刻情報を付加して表示する
 *
 *	@param [ char *str1 ]  文字列１格納ポインタ
 *	@param [ char *str2 ]  文字列２格納ポインタ
 *	@param [ int mask ]  表示する、
 *
 *	@retval なし
 */
void DebugPrint(char *str1, char *str2, int mask) {
	if ((mask == 0) || ((param.debug_flg & mask) != 0)) {
		printf("%s %X:%s %s\n",  DebugTimePrint(), mask, str1, str2);
	}
}

/**
 *	@brief デバッグ用の時刻文字列を返す処理
 *
 *	@retval なし
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
 *	@brief 16 進文字列を 10 進数に変換する処理
 *
 *	@param [ const char str[ ] ]  文字列格納ポインタ
 *
 *	@retval なし
 */
static unsigned long ToDec(const char str[ ])
{
    short i = 0;        /* 配列の添字として使用 */
    short n;
    unsigned long x = 0;
    char c;

    while (str[i] != '\0') {        /* 文字列の末尾でなければ */

            /* '0' から '9' の文字なら */
        if ('0' <= str[i] && str[i] <= '9')
            n = str[i] - '0';        /* 数字に変換 */

            /* 'a' から 'f' の文字なら */
        else if ('a' <= (c = tolower(str[i])) && c <= 'f')
            n = c - 'a' + 10;        /* 数字に変換 */

        else {        /* それ以外の文字なら */
            printf("無効な文字です。\n");
            exit(0);        /* プログラムを終了させる */
        }
        i++;        /* 次の文字を指す */

        x = x *16 + n;    /* 桁上がり */
    }
    return (x);
}

