/* srv.c */
aaaaa
#define STRICT
#include <stdio.h>
#include <stdlib.h>
#include <conio.h> //for kbhit
#include <string.h>
#include <time.h>

#include "mitaka_src/define.h"
#include "mitaka_src/app.h"
extern int 	my_tanmatsu_type;
extern int sw_test_flg;
extern int sw_test_data;
int ccc;
main( int argc, char *argv[] )
{
	my_tanmatsu_type = 2;
	if (argc > 1) {
		my_tanmatsu_type = atoi(argv[1]);
	}
	if (argc > 2) {
		/* テスト用に使用 */
		sw_test_flg = 1;
		sw_test_data = strtol(argv[2], 0,16);
	} else {
		sw_test_flg = 0;
	}
	APP_Initialize( );

	    while ( 1 )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        APP_Tasks ( );

    }
}
