#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "usmart.h"
#include "malloc.h"
#include "usart3.h"
#include "common.h"


int get_response(int clear, char *res);
void erase_data();
int send_command_with_retry(char *cmd, int timeout, int retry, int erase, char *res);
void send_command_util_success(char *cmd, int timeout, int erase, char *res);
void wait_for_data(int clear, char *res);
void extract_peer_ip(char *res, char *ip);
