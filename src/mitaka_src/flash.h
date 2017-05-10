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
int FlashWrite(int adr, int data);/* �t���b�V���������ւ̏������ݏ��� */
int FlashSectorWrite(int adr, char *buff);/* �t���b�V���������̃Z�N�^�[�ւ̏������ݏ��� */
int FlashSectorErase(int adr);/* �t���b�V���������̃Z�N�^�[�������� */

