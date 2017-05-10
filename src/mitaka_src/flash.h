enum {
	NVM_ERASE_INIT = 0,/*  */
	NVM_ERASE_WAIT,/*  */
	NVM_ERASE_CHECK,/*  */
	NVM_ERASE_SUCCESS,/*  */
	NVM_ERASE_FAIL,/*  */
	NVM_WRITE,
	NVM_SECTOR_WRITE,
	NVM_WRITE_WAIT,
	NVM_WRITE_CHECK,
	NVM_WRITE_SUCCESS,
	NVM_WRITE_FAIL
};

int flash_erase_APP_Init ( int address );
int flash_erase_APP_Tasks ( void );
int flash_sector_write_APP_Init ( int address ,int databuff );
int flash_write_APP_Init ( int address ,int data );
int FlashWrite(int adr, int data);/* フラッシュメモリへの書き込み処理 */
int FlashSectorWrite(int adr, char *buff);/* フラッシュメモリのセクターへの書き込み処理 */
int FlashSectorErase(int adr);/* フラッシュメモリのセクター消去処理 */

