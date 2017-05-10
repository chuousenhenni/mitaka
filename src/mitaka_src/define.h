/*
 ********************************************************************************************
 *
 *	Copyright (c) 2016  ADVANSE CO.,LTD.
 *					All rights reserved.
 *
 ********************************************************************************************
 */
/** @file	define.h
 *	ファイルの概要を記述する
 *
 *	ファイルの詳細を記述する
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

/* 定数 */

/* 制御機Ａのアドレス */
#define CONTA_ADDRESS  (5)

/*
 可変表示板に表示するパターン
 IO入力のステータス値
*/
				/* S1	S2	S3 */
#define P1  (5)	/* ON	OFF	ON */
#define P2  (6)	/* OFF	ON	ON */
#define P3  (7)	/* ON	ON	ON */

/*ディフォルト値 */

#define CONTROLER_MAX (8) /* 制御機Ⅰ数 + 制御機Ⅱ数 */
#define DISPLAY_BOARD_MAX (8) /* １制御機あたりの可変標識板の数 */

#define DEFAULT_START_TIME (7 * 60 * 60 ) /* 変移開始時間(秒で設定) */
#define DEFAULT_END_TIME (9 * 60 * 60) /* 変移終了時間(秒で設定) */
#define DEFAULT_TIME_CORRECTION_TIME (0 * 60 + 0) /* 時刻修正時刻 */
#define DEFAULT_TIME_CORRECTION_REQUEST_TIME (0) /* 時刻修正要求時刻 */
#define DEFAULT_HOLIDAY_DISPLAY_PATTERN (0) /* 日・休表示パターン P3 */
#define DEFAULT_HOLIDAY_MODE (0) /* なし */
#define DEFAULT_HOLIDAY_START_TIME (0) /* なし　日・休開始時間 */
#define DEFAULT_HOLIDAY_END_TIME (0) /* なし　日・休終了時間 */
#define DEFAULT_SATURDAY_MODE (1) /* 変移あり（月～金と同じ） */
#define DEFAULT_HOLIDAY_MODE (0) /* 変移なし（通常） */
#define DEFAULT_FAIL_PATTERN (P2) /* 可変表示板 P2 */
#define DEFAULT_OFFSET_TIMER (120) /* オフセットタイマ */
#define DEFAULT_LINKAGE (0x11111111) /* 連動設定 */

#define RESPONSE_INTERVAL_TIME_VAL (250)	/* 制御機Aからの要求から制御機Bからの応答受信確認までの最小待ち時間(ms) */
#define RESPONSE_TIMEOUT_VAL (500)			/* 制御機Aからの要求から制御機Bからの応答受信までのタイムアウト値(ms) */
//20170305 #define MUSEN_TIMEOUT_VAL (30) /* 無線通信の受信エラータイムアウト値(秒)  kaji20170303 60→30 */
#define MUSEN_TIMEOUT_VAL (60) /* 無線通信の受信エラータイムアウト値(秒)  kaji20170305c 60秒 */

/* -------------------- */

/* 時刻異常D情報 */
#define TIME_ERROR_VAL (10) /* 時刻エラーとなる時間の差(秒) */

//#define UNMATCH_TIMEOUT_VALUE (30*1000)
#define UNMATCH_TIMEOUT_VALUE (50*1000)		/* 可変表示板制御のタイムアウト値[ms] */

#define BROADCAST_RETRY_VAL (3)		/* kaji20170308 同報指令の複数回送信数 */

#define PREAMBLE_PTN (0x55aa)		/* kaji20170308 プリアンブルパターン */


/* 制御器間電文のヘッダーデータ */
typedef struct {
	unsigned char no;/* 規格番号 00H固定 */
	unsigned char dst;/* 宛先アドレス 01H固定（未使用） */
	unsigned char src;/* 発信元アドレス  01H固定 */
	unsigned char sub_adr;/* サブアドレス 00H固定 */
	unsigned char priority;/* 優先レベル 02H固定 */
	unsigned char s_no;/* 通番 00H固定 */
	unsigned char contoroler_no;/* 端末種別 19H */
	unsigned char info;/* 情報種別 01H */
	unsigned char div_no;/* 分割番号 81H */
	unsigned char length;/* データ長 */
} CONTROL_HEADER;

/* 時刻情報の構造体 */
typedef struct {
	unsigned char year[2];/* BCD 年 */
	unsigned char month;/* BCD 月 */
	unsigned char day;/* BCD 日 */
	unsigned char hour;/* BCD 時 */
	unsigned char min;/* BCD 分 */
	unsigned char sec;/* BCD 秒 */
	union {
		unsigned char byte;/* 休日(DB7 1)、曜日 */
		struct {
			char week:7;/* 下位7ビットから入る */
			char holiday:1;/* 上位1ビット */
		};
	} holiday_week;
} TIME_INFO;

/* BIN形式の時刻情報の構造体 */
typedef struct {
	int year;/* 年 */
	int month;/* 月 */
	int day;/* 日 */
	int hour;/* 時 */
	int min;/* 分 */
	int sec;/* 秒 */
	int week;/*  週(1:日,2:月...7:土)*/
} TIME_INFO_BIN;

typedef struct {
	union {
		unsigned char status[6];/* 制御指令 */
		struct {
			union {
				unsigned char byte;/*  */
				struct {
					char tuujou:1;/* 通常 下位1ビットから入る */
					char issou:1;/* 一掃 */
					char henni:1;/* 変移 */
					char teishi:1;/* 運用停止 */
					char test:1;/* 伝送テスト */
					char fail:1;/* フェイル指定 */
					char tanmatu_shudou:1;/* 端末手動 */
					char tanni_shudou:1;/* 単位手動 上位ビット */
				};
			} byte1;/* 第Ⅰバイト */
			union {
				unsigned char byte;/*  */
				struct {
					char jimaku:1;/* 字幕移動中 下位1ビットから入る */
					char yobi1:1;/* 予備 */
					char tanmatu_error:1;/* 端末制御機異常 */
					char yobi2:1;/* 予備 */
					char kiten_error:1;/* 起点案内標識異常 */
					//char yobi3:1;/* 予備 */
					char musen_error:1;/* 無線異常 */
		
					char pc_tuushin_error:1;/* 運用管理PC間通信異常 */
					char moniter_tuushin_error:1;/* 監視盤間通信異常 上位ビット */
				};
			} byte2; /* 第２バイト */
			union {
				unsigned char byte;/*  */
				struct {
					char board_error1:1;/* 標識版１異常 */
					char board_error2:1;/* 標識版２異常 */
					char board_error3:1;/* 標識版３異常 */
					char board_error4:1;/* 標識版４異常 */
					char board_error5:1;/* 標識版５異常 */
					char board_error6:1;/* 標識版６異常 */
					char board_error7:1;/* 標識版７異常 */
					char board_error8:1;/* 標識版８異常 */
				};
			} byte3; /* 第３バイト */
			union {
				unsigned char byte;/*  */
				struct {
					char yobi:2;/* 予備 */
					char byou1_error:1;/* 道路発光鋲1異常 */
					char byou2_error:1;/* 道路発光鋲2異常 */
					char yob2:4;/* 予備 */
				};
			} byte4; /* 第４バイト */
			union {
				unsigned char byte;/*  */
			} byte5; /* 第５バイト */
			union {
				unsigned char byte;/*  */
			} byte6; /* 第６バイト */
		};
	} response;/* 監視応答内容 */
} RESPONSE;

/* 監視盤間への電文 制御指令の電文フォーマット */
typedef struct {
	CONTROL_HEADER h;/* ヘッダー */
//	unsigned char reserved1[6];/* 予備 */
	TIME_INFO t;/* 時刻 */
	union {
		RESPONSE responce[8];
		unsigned char command[8][6];/* 制御指令内容 */
	};
	//	unsigned char reserved2[10];/* 予備 */
} MONITOR_COMMAND;

/* 監視盤間への電文 運用指令の電文フォーマット */
typedef struct {
	CONTROL_HEADER h;/* ヘッダー */
//	unsigned char reserved1[6];/* 予備 */
	TIME_INFO t;/* 時刻 */
	unsigned char  command;/* 制御指令内容 */
//	unsigned char reserved2[10];/* 予備 */
} MONITOR_OPERATE_COMMAND;


/* 制御器間電文 同報指令の電文フォーマット */
typedef struct {
	CONTROL_HEADER h;/* ヘッダー */
	TIME_INFO t;/* 時刻 */
	union {
		unsigned char byte;/*  */
		struct {
			char tuujou:1;/* 通常 下位1ビットから入る */
			char issou:1;/* 一掃 */
			char henni:1;/* 変移 */
//			char yobi1:1;/* なし */
			char teishi:1;/* 運用停止 */
			char test:1;/* 伝送テスト */
			char fail:1;/* フェイル指定 */
			char yobi2:1;/* 予備 運用停止解除 */
			char shudou:1;/* 手動 上位ビット */
		};
	} command;/* 制御指令 */
	union {
		unsigned char byte;/*  */
		struct {
			char issei:1;/* 調光一斉指令有 下位1ビット */
			char choukou_iri:1;/* 調光強制入（低） */
			char choukou_kiri:1;/* 調光強制切（高） */
			char time_req:1;/* 時刻修正要求 */
			char sreq:1;/* スケジュール登録要求 */
			char rendou_req:1;/* 連動設定要求 20170320 */
			char yobi:2;/* 手動 上位ビット */
		};
	} light_command;/* 調光指令 */
	union {
		unsigned char byte;/*  */
		struct {
			char monitor_error:1;/* 対監視盤伝送異常 下位1ビット */
			char yobi:7;/* 手動 上位ビット */
		};
	} status;/* 端末制御機（Ⅰ）状態 */
	struct {
		unsigned char start_time[2];/* 開始時刻 */
		unsigned char start_command;/* 制御指令 開始時表示項目 */
		unsigned char end_time[2];/* 終了時刻 */
		unsigned char end_command;/* 制御指令 終了時表示項目 */
		unsigned char offset_timer;/* オフセットタイマ（BCD) */
	} schedule;
} BROADCAST_COMMAND;

/* 制御器（Ⅱ）への電文 通常指令の電文フォーマット */
typedef struct {
	CONTROL_HEADER h;/* ヘッダー */
	TIME_INFO t;/* 時刻 */
	union {
		unsigned char byte;/*  */
		struct {
			char tuujou:1;/* 通常 下位1ビットから入る */
			char issou:1;/* 一掃 */
			char henni:1;/* 変移 */
//			char yobi1:1;/* なし */
			char teishi:1;/* 運用停止 */
			char test:1;/* 伝送テスト */
			char fail:1;/* フェイル指定 */
			char yobi2:1;/* 予備 運用停止解除 */
			char shudou:1;/* 手動 上位ビット */
		};
	} command;/* 制御指令 */
	union {
		unsigned char byte;/*  */
		struct {
			char issei:1;/* 調光一斉指令有 下位1ビット */
			char choukou_iri:1;/* 調光強制入（低） */
			char choukou_kiri:1;/* 調光強制切（高） */
			char time_req:1;/* 時刻修正要求 */
			char yobi:4;/* 手動 上位ビット */
		};
	} light_command;/* 調光指令 */
	union {
		unsigned char byte;/*  */
		struct {
			char monitor_error:1;/* 対監視盤伝送異常 下位1ビット */
			char yobi:7;/* 手動 上位ビット */
		};
	} status;/* 端末制御機（Ⅰ）状態 */
} NORMAL_COMMAND;

/* 制御器（Ⅱ）からの電文 監視応答の電文フォーマット */
typedef struct {
	CONTROL_HEADER h;/* ヘッダー */
	TIME_INFO t;/* 時刻 */
	union {
		unsigned char status[7];/* 制御指令 */
		struct {
			union {
				unsigned char byte;/*  */
				struct {
					char tuujou:1;/* 通常 下位1ビットから入る */
					char issou:1;/* 一掃 */
					char henni:1;/* 変移 */
					char teishi:1;/* 運用停止 */
					char test:1;/* 伝送テスト */
					char fail:1;/* フェイル指定 */
					char tanmatu_shudou:1;/* 端末手動 */
					char tanni_shudou:1;/* 単位手動 上位ビット */
				};
			} byte1;/* 第Ⅰバイト */
			union {
				unsigned char byte;/*  */
				struct {
					char jimaku:1;/* 字幕移動中 下位1ビットから入る */
					char hoshu_status:1;/* 1:保守/0:通常 */
					char tanmatu_error:1;/* 端末制御機異常 */
					char yobi2:1;/* 予備 */
					char kiten_error:1;/* 起点案内標識異常 */
					//char yobi3:1;/* 予備 */
					char musen_error:1;/* 無線異常 */
		
					char pc_tuushin_error:1;/* 運用管理PC通信異常 */
					char moniter_tuushin_error:1;/* 監視盤間通信異常 上位ビット */
				};
			} byte2; /* 第２バイト */
			union {
				unsigned char byte;/*  */
				struct {
					char board_error1:1;/* 標識版１異常 */
					char board_error2:1;/* 標識版２異常 */
					char board_error3:1;/* 標識版３異常 */
					char board_error4:1;/* 標識版４異常 */
					char board_error5:1;/* 標識版５異常 */
					char board_error6:1;/* 標識版６異常 */
					char board_error7:1;/* 標識版７異常 */
					char board_error8:1;/* 標識版８異常 */
				};
			} byte3; /* 第３バイト */
			union {
				unsigned char byte;/*  */
				struct {
					char yobi:2;/* 予備 */
					char byou1_error:1;/* 道路発光鋲1異常 */
					char byou2_error:1;/* 道路発光鋲2異常 */
					char yob2:4;/* 予備 */
				};
			} byte4; /* 第４バイト */
			union {
				unsigned char byte;/*  */
			} byte5; /* 第５バイト */
			union {
				unsigned char byte;/*  */
			} byte6; /* 第６バイト */
			union {
				unsigned char byte;/*  */
				struct {
					char yobi1:1;/* 予備 下位1ビットから入る */
					char choukou_iri:1;/* 調光入（昼） */
					char choukou_kiri:1;/* 調光切（夜） */
					char time_req:1;/* 時刻修正依頼 */
					char schedule_req:1;/* スケジュール登録依頼 */
					char yobi2:1;/* 予備 */
					char schedule_disp:1;/* スケジュール表示中 */
					char control_disp:1;/* 制御指令表示中 上位ビット */
				};
			} byte7;/* 第７バイト */
		};
	} response;/* 監視応答内容 */
} RESPONSE_COMMAND;


/* 変移休止日管理構造体 */
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

/* 制御器Aへの電文 変移休止日通知の電文フォーマット */
typedef struct {
	CONTROL_HEADER h;/* ヘッダー */
	char d[182];
} HOLIDAY_COMMAND;

/* 設定値構造体 */
typedef struct {
	int start_time; /* 変移開始時間 */
	int end_time; /* 変移終了時間 */
	int time_correction_time; /* 時刻修正時刻 */
	int time_correction_request_time; /* 時刻修正要求時刻 */
	int holiday_pattern; /* 日・休表示パターン P3 */
	int holiday_start_time; /* なし　日・休開始時間 */
	int holiday_end_time; /* なし　日・休終了時間 */
	int saturday_mode; /* 土曜日モード　変移あり（月～金と同じ） */
	int holiday_mode; /* 休日モード　変移なし（通常） */
	int fail_pattern; /* 可変表示板 P2 ファイルパターン */
	int offset_timer; /* オフセットタイマ */
	int no_musen_error_flag; /* 無線異常をチェックするや否や 0:する,1:しない */
	int same_nomove_flag; /* 同じ場合は可変表示版を変化させないモード */
	int no_fail_flag; /* フェイルにしないモード */
	int mdmcs_delay_time; /* MDM_CS出力の遅延時間(ms) */
	int linkage_status; /* 連動設定 */
	int no_board_error_flag; /* 標識版異常をチェックするや否や 0:する,1:しない */
	int no_pc_check_flag; /* 運用管理ＰＣ間通信異常を判定をチェックするや否や 0:する,1:しない */
	int debug_flg;/* デバッグ用の表示を行う場合に0以外をセットする */
	int response_interval_time_val;/* 20170305 制御機Aからの要求から制御機Bからの応答受信確認までの最小待ち時間(ms) */
	int response_timeout_val;/* 20170305 制御機Aからの要求から制御機Bからの応答受信までのタイムアウト値(ms) */
	int musen_timeout_val;/* 20170305 無線通信の受信エラータイムアウト値(秒) */
	int reset_count;/* リセットのたびに＋１ */
	int preamble_ptn;/* プリアンブルパターンを指定する */
	int bcc;/* BCC値 */
} PARAM;

/* 動作パラメータ */
/*
停電時はこの値を保持しておく
*/
typedef struct {
	int mode;/* 遠隔、手動、監視盤からの指令 */
	int status;/* フェイル、通常、一掃、変移、日・休 */
	int bef_status;/* 制御機Bのステータス変換判定用 */
	int before_fail_status;/* フェイルになる前の状態を保持 */
	int schedule;/* スケジュール登録済みフラグ */
	int time_req;/* 時刻設定済みフラグ */
	int start_time;/* 通常→一掃開始時間 */
	int end_time;/* 変移→一掃開始時間 */
	int offset_timer;/* オフセットタイマ値 */
	int power_outage_flag;/* 停電発生フラグ */
	int power_outage_move_flag;/* 停電発生でフェイルへ移動したフラグ */
	int test_flag;/* テスト中フラグ */
	int board_error;/* 表示板異常発生フラグ */
	int jimaku_ido_flag;/* 字幕移動中フラグ */
	int hoshu_status; /* 1:保守/0:通常 */
	int keikoutou_status; /* 1:蛍光灯入/0:自動 */
	int cds_status; /* CDS状態 */
	int time_status;/* 時刻状態 0:正常,1:異常 */
	int gps_status;/* GPS状態 0:正常,1:異常 */
	int before_gps_status;/* 前回のGPS状態 0:正常,1:異常 */
	int musen_status;/* 無線通信状態 0:正常,1:異常 */
	int before_musen_status;/* 前回の無線通信状態 0:正常,1:異常 */
	int byou_status;/* 発光鋲状態 0:正常,1:異常 */
	int before_byou_status;/* 前回の発光鋲状態 0:正常,1:異常 */
	int byou1_status;/* 発光鋲1状態 0:正常,1:異常 */
	int byou2_status;/* 発光鋲2状態 0:正常,1:異常 */
	int pc_tuushin_status;/* 運用管理PC間通信状態 0:正常,1:異常 */
	int moniter_tuushin_status;/* 監視盤間通信状態 0:正常,1:異常 */
	int manual_status;/* 手動状態 0:SWによる手動,1:運用PCからの手動による */
	int tanmatu_error;/* 端末エラー状態を保持 0:エラー無し,1:エラー有り */
	int power_outage_flag2;/* 停電発生フラグ2 */
	int bcc;/* BCC値 */
} STATUS;


/* 動作パラメータ */
/*
表示板等のIO情報用構造体
*/
typedef struct {
	int display_board_count;/* 可変表示板数 */
	int light_count;/* 発光鋲数 */
	int out_status[DISPLAY_BOARD_MAX];/* 可変表示板への出力データ */
	int in_status[DISPLAY_BOARD_MAX];/* 可変表示板からの入力データ */
	int kiten_annai[DISPLAY_BOARD_MAX];/* 起点案内標識かどうかを保持するデータ */
	int allowed_pattern_p1[DISPLAY_BOARD_MAX][2];/* P1の許されるパターン(P1のみまたはP1,P2) */
	int allowed_pattern_p2[DISPLAY_BOARD_MAX][2];/* P2の許されるパターン(P1,P2またはP1,P3) */
	int allowed_pattern_p3[DISPLAY_BOARD_MAX][2];/* P3の許されるパターン(P3のみまたはP2,P3) */
	int muji_pattern_p1[DISPLAY_BOARD_MAX];/* P1が無地パターンかどうか */
	int muji_pattern_p2[DISPLAY_BOARD_MAX];/* P2が無地パターンかどうか */
	int muji_pattern_p3[DISPLAY_BOARD_MAX];/* P3が無地パターンかどうか */
	
} IO_INFO;

/* キューの配列番号 */
#ifdef windows
#define CONTROLER_QUEUE (0)	/* 制御機間通信 uart1 */
#define PC_QUEUE (0)        /* 運用管理PC間通信 uart 2 */
#define MONITOR_QUEUE (0)   /* 監視盤間通信 uart 3 */
#define CONSOLE_QUEUE (3)   /* メンテナンスPC間通信 uart4 */
#define IO_QUEUE (4)
#define UART1_QUEUE (0)		/* UART1の受信データ kaji20170306 */
#define UART3_QUEUE (0)		/* UART3の受信データ kaji20170310 */
#else
#define CONTROLER_QUEUE (0)	/* 制御機間通信 uart1 */
#define PC_QUEUE (1)        /* 運用管理PC間通信 uart 2 */
#define MONITOR_QUEUE (2)   /* 監視盤間通信 uart 3 */
#define CONSOLE_QUEUE (3)   /* メンテナンスPC間通信 uart4 */
#define IO_QUEUE (4)
#define UART1_QUEUE (5)		/* UART1の受信データ kaji20170305 */
#define UART3_QUEUE (6)		/* UART3の受信データ kaji20170310 */
#endif

#define QUEUE_MAX	7     // キューの個数 kaji20170310

/* LEDの状態変数 */
enum _LedStatus{
	LED_STATUS_OFF = 0,
	LED_STATUS_GREEN,
	LED_STATUS_ORANGE,
	LED_STATUS_RED,
	LED_STATUS_GREEN_TOGGLE,
	LED_STATUS_ORANGE_TOGGLE,
	LED_STATUS_RED_TOGGLE,
};

/* LEDの状態変数 */
enum _LedStatus_old{
	LED_STATUS_OFF_old = 0,
	LED_STATUS_ON,
	LED_STATUS_TOGGLE
};

#define CONT_LED_MAX_COUNT (16) /* LED制御最大数(実際は9だがビット位置のため) */
#define MONITOR_LED_MAX_COUNT (2 * CONTROLER_MAX + 3) /* LED制御最大数(各地点用と運用停止LEDとブザー停止LEDと警報ランプ) */
#define UNYOU_TEISHI_LED (2 * CONTROLER_MAX) /* 運用停止LEDの配列ポインタ */
#define BUZZER_TEISHI_LED (2 * CONTROLER_MAX + 1) /* ブザー停止LEDの配列ポインタ */
#define ALARM_LAMP_LED (2 * CONTROLER_MAX + 2) /* 警報ランプLEDの配列ポインタ */


/* 表示状態の状態変数 */
enum _DispStatus{
	STATUS_P1 = 1,//通常
	STATUS_P2,//一掃
	STATUS_P3,//変移
	STATUS_HOLIDAY,//日・休
	STATUS_FAIL,//フェイル
	STATUS_P1P2,//通常からの変化時の一掃
	STATUS_P3P2,//変移からの変化時の一掃

};

/* 運用状態の状態変数 */
enum _ModeStatus{
	MODE_REMOTE = 0,/* 遠隔 */
	MODE_MANUAL,/* 手動 */
	MODE_MONITOR,/* 監視盤からの指令 */
};

/* 可変表示板制御の状態関数 */
enum _board_rotate_state {
	BOARD_ROTATE_READY = 0,
	BOARD_ROTATE_START,
	BOARD_ROTATE_WAIT,
	BOARD_ROTATE_END,
	BOARD_ROTATE_OFFSET_TIMER,
	BOARD_ROTATE_OFFSET_TIMER_END,
	BOARD_ROTATE_IDLE
};


/* イベントコード */
enum _event{
	REMOTE_REQUEST = 0,			/* 遠隔リクエスト */
	REMOTE_TUUJOU_REQUEST,		/* 通常時間内での遠隔リクエスト */
	REMOTE_HENNI_REQUEST,		/* 変移時間内での遠隔リクエスト */
	REMOTE_TUUJOU2HENNI_REQUEST,/* 遠隔通常から変移リクエスト */
	REMOTE_HENNI2TUUJOU_REQUEST,/* 遠隔変移から通常リクエスト */
	MANUAL_TUUJOU_REQUEST,		/* 手動通常リクエスト */
	MANUAL_ISSOU_REQUEST,		/* 手動一掃リクエスト */
	MANUAL_HENNI_REQUEST,		/* 手動変移リクエスト */
	SW_MANUAL_TUUJOU_REQUEST,	/* SW手動通常リクエスト */
	SW_MANUAL_ISSOU_REQUEST,	/* SW手動一掃リクエスト */
	SW_MANUAL_HENNI_REQUEST,	/* SW手動変移リクエスト */
	MANUAL_FAIL_REQUEST,		/* 手動フェイルリクエスト */
	MONITOR_REQUEST,			/* 運用停止リクエスト */
	MONITOR_RELEASE_REQUEST,	/* 運用停止解除リクエスト */
	FAIL_REQUEST,				/* 表示板,GPS,無線異常リクエスト */
	POWER_OFF_REQUEST,			/* 停電発生リクエスト */
	POWER_RECOVER_REQUEST,		/* 停電発生復帰リクエスト */
	FAIL_RECOVER_REQUEST,		/* フェイル異常復帰（時計、GPS）リクエスト */
	FAIL2TUUJOU_REQUEST,		/* フェイル時の通常への復帰リクエスト */
	TUUJOU_REQUEST,				/* 制御機Bの通常リクエスト */
	ISSOU_REQUEST,				/* 制御機Bの一掃リクエスト */
	HENNI_REQUEST,				/* 制御機Bの変移リクエスト */
	CONTB_FAIL_REQUEST,			/* 制御機Bのフェイルリクエスト */
};

/* 変化状態 */
enum {
	P2_P2 = 0,/* オフセットタイマ時間待ち */
	PX_P1,/* 通常へ */
	PX_P2,/* 一掃へ */
	PX_P3,/* 変移へ */
	PX_FAIL,
	PX_POWEROFF,
	PX_POWER_RECOVER
};

enum {/* その日の状態 */
	TODAY_IS_NORMAL_DAY = 0,
	TODAY_IS_HOLIDAY,
};

/* kaji20170225↓ */
enum {/* フェイルをどう扱っているかの状態変数 */
	FAIL_HND_NONE = 0,	/* フェイル状態　無し */
	FAIL_HND_FAIL,		/* フェイル状態 */
	FAIL_HND_P1_PARK	/* 通常時に発生したフェイルなので変化させない */
};
/* kaji20170225↑ */



#define RCV_BUFF_SIZE (200 + 10) /* 受信バッファーサイズ */

/* LED情報 */

#define LED_ON (1)	/* LED点灯 */
#define LED_OFF (0)	/* LED消灯 */
#define TOGGLE_COUNT_MAX (500) /* 0.5秒 */

 /* アドレス */
#define FLASH_TOP_ADDRESS (0xE0000000) /* ステータス格納アドレス */
#define HOLIDAY_ADDRESS (0xE0000000) /* 変移休止日格納アドレス */
#if 1
#define STATUS_ADDRESS (0xE00F8000) /* ステータス格納アドレス */
#else
#define STATUS_ADDRESS (0xBD1E0000) /* ステータス格納アドレス */
#endif


#if 1
#define PARAM_ADDRESS  (0xE00FA000) /* パラメータ格納アドレス */
#else
#define PARAM_ADDRESS  (0xBD1F0000) /* パラメータ格納アドレス */
#endif


#endif	/* ___DEFINE_H___ */

