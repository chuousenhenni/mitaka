#define STRICT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdint.h>
#include <conio.h> //for kbhit
#include <time.h>

#ifdef windows
#include <unistd.h>
#include <windows.h>
#else
#include "app.h"
#include "system_definitions.h"
#include "peripheral/devcon/plib_devcon.h"
#include "peripheral/rtcc/plib_rtcc.h"
#include "peripheral/wdt/plib_wdt.h"
#include <xc.h>
#endif

