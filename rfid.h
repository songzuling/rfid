#ifndef RFID_H_
#define RFID_H_

#include"stddef.h"
#include"serial.h"
#include"stdbool.h"


#define TAG_BUFFER_SIZE              200
#define RX_BUFFER_SIZE               2048

typedef enum{
    CMD_HELLO = 0x01,
    CMD_HEART_BEAT = 0x02,
    CMD_GET_MODULE_INFO = 0x03,
    CMD_SINGLE_ID = 0x22,
    CMD_MULTI_ID = 0x27,
    CMD_STOP_MULTI = 0x28,
    CMD_READ_DATA = 0x39,
    CMD_WRITE_DATA = 0x49,
    CMD_LOCK_UNLOCK = 0x82,
    CMD_KILL= 0x65,
    CMD_SET_REGION = 0x07,
    CMD_INSERT_FHSS_CHANNEL = 0xA9,
    CMD_GET_RF_CHANNEL = 0xbb,
    CMD_SET_RF_CHANNEL = 0xAB,
    CMD_SET_CHN2_CHANNEL= 0xAF,
    CMD_SET_US_CHANNEL = 0xAC,
    CMD_OPEN_PA= 0xAE,
    CMD_SET_FHSS = 0xAD,
    CMD_SET_POWER = 0xB6,
    CMD_GET_POWER = 0xB7,
    CMD_GET_SELECT_PARA = 0x0B,
    CMD_SET_SELECT_PARA = 0x0C,
    CMD_GET_QUERY_PARA = 0x0D,
    CMD_SET_QUERY_PARA = 0x0E,
    CMD_SET_CW = 0xB0,
    CMD_SET_BLF = 0xBF,
    CMD_FAIL = 0xFF,
    CMD_SUCCESS = 0x00,
    CMD_SET_SFR = 0xFE,
    CMD_READ_SFR = 0xFD,
    CMD_INIT_SFR = 0xEC,
    CMD_CAL_MX = 0xEA,
    CMD_READ_MEM = 0xFB,
    CMD_SET_INV_MODE = 0x12,
    CMD_SET_UART_BAUDRATE = 0x11,
    CMD_SCAN_JAMMER = 0xF2,
    CMD_SCAN_RSSI = 0xF3,
    CMD_AUTO_ADJUST_CH = 0xF4,
    CMD_SET_MODEM_PARA = 0xF0,
    CMD_READ_MODEM_PARA = 0xF1,
    CMD_SET_ENV_MODE = 0xF5,
    CMD_TEST_RESET = 0x55,
    CMD_POWERDOWN_MODE = 0x17,
    CMD_SET_SLEEP_TIME = 0x1D,
    CMD_IO_CONTROL = 0x1A,
    CMD_RESTART = 0x19,
    CMD_LOAD_NV_CONFIG = 0x0A,
    CMD_SAVE_NV_CONFIG = 0x09,
    CMD_ENABLE_FW_ISP_UPDATE = 0x1F,
    CMD_SET_READ_ADDR = 0x14
}cmd_code;

typedef struct
{    
    unsigned char PC[2];
    unsigned char RSSI[2];
    unsigned char RESERVE[32];
    unsigned char EPC[32];
    unsigned char TID[32];
    unsigned char USER[32];
}tag_t;

void send_command(int fd, cmd_code cmd, unsigned int pl, unsigned char *para);
void rfid_init(void);
void debuge(void);


void select_tag(unsigned char selParam, unsigned char *ptr, unsigned int maskBits, unsigned char isTrunc, unsigned char *mask);
void write_tag(unsigned char *accessPasswords, unsigned char memBank, unsigned int offWords, unsigned int datlenWords, unsigned char *data);
void read_tag(unsigned char *accessPasswords, unsigned char memBank, unsigned int offWords, unsigned int datlenWords);


void read_user_data(void);
bool write_user_data(long int data, unsigned char *ap,unsigned int n, unsigned int intervalms);

void clear_epc_buffer(void);

#endif
