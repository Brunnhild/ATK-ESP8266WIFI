#ifndef __APP_H
#define __APP_H

#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "usmart.h"
#include "malloc.h"
#include "usart3.h"
#include "common.h"
#include "audioplay.h"
#include "piclib.h"

#include "ff.h"
#include "wm8978.h"
#include "i2s.h"
#include "led.h"
#include "lcd.h"
#include "key.h"
#include "exfuns.h"
#include "text.h"
#include "string.h"

int get_response(int clear, char *res);
void erase_data(void);
int send_command_with_retry(char *cmd, int timeout, int retry, int erase, char *res);
void send_command_util_success(char *cmd, int timeout, int erase, char *res);
void wait_for_data(int clear, char *res);
void extract_peer_ip(char *res, char *ip);
void index_show(u16 index,u16 total);
void music_player(void);
void test_write_file(void);

typedef struct {
    FIL fil;
} FileWriter;

int open_big_file(char *fname, FileWriter *fw);
int write_data(u8 *data, int len, FileWriter *fw);
int end_data(FileWriter *fw);

void receive_music(void);
void ui_show(void);

#endif
