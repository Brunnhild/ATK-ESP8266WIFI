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

#define MAX_PACKET_LEN 256
#define PACKET_INTERVAL 2000

char *get_peer_ip(void);
int get_device_id(void);
int get_run_greet(void);

void set_peer_ip(char *);

int get_response(int clear, char *res);
void erase_data(void);
int send_command_with_retry(char *cmd, int timeout, int retry, int erase, char *res);
void send_command_util_success(char *cmd, int timeout, int erase, char *res);
void wait_for_data(int clear, char *res);
int wait_for_data_with_timeout(int clear, char *res, int time_out);
void extract_peer_ip(char *res, char *ip);
void extarct_ap_ip(char *res, char *ip);
void index_show(u16 index,u16 total);
void music_player(void);
void test_write_file(void);

int open_big_file(char *fname, FIL *fp);
int write_data(u8 *data, int len, FIL *fp);
int read_data(u8 *data, int chalk_size, FIL *fp);
int close_file(FIL *fw);

void ui_show(void);

void send_packet(char *s, int len);
void wait_for_packet(char *res);

int get_music_file_names(char **music_names, char **picture_names);
void send_music(char **music_names, char **picture_names);
void receive_music(void);

#endif
