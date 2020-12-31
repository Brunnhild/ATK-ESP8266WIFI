#include "app.h"


int get_response(int clear, char *res) {
	if (USART3_RX_STA & 0X8000) {								//接收到一次数据了
		USART3_RX_BUF[USART3_RX_STA&0X7FFF] = 0;	//添加结束符
		sprintf(res, "%s", USART3_RX_BUF);				//发送到串口
		if (clear) USART3_RX_STA = 0;
		return 1;
	} else return 0;
}

void erase_data() {
	USART3_RX_STA = 0;
}

int send_command_with_retry(char *cmd, int timeout, int retry, int erase, char *res) {
	int flag = 0, cnt = 0;
	while (flag == 0 && cnt < retry) {
		if (atk_8266_send_cmd((u8 *) cmd, (u8 *) "OK", timeout + cnt * 200) == 0) flag = 1;
		cnt++;
		printf("Command %s failed, retrying...\n", cmd);
	}
	if (flag) {
		printf("Command %s succeed\n", cmd);
		if (res != NULL) get_response(erase, res);
		else if (erase) erase_data();
	} else printf("Command %s failed after %d times of retry.\n", cmd, retry);
	return flag;
}

void send_command_util_success(char *cmd, int timeout, int erase, char *res) {
	int flag = 0;
	printf("Begin sending command %s\n", cmd);
	while (flag == 0) {
		if (atk_8266_send_cmd((u8 *) cmd, (u8 *) "OK", timeout) == 0) flag = 1;
		// printf("Command %s failed, retrying...\n", cmd);
	}
	printf("Command %s succeed\n", cmd);
	if (res != NULL) get_response(erase, res);
	else if (erase) erase_data();
}

void wait_for_data(int clear, char *res) {
	int flag = 0;
	while (flag == 0) {
		if (get_response(clear, res) == 1) flag = 1;
		delay_ms(200);
	}
}

void extract_peer_ip(char *res, char *ip) {
	int len = strlen(res);
	int i = 0, j = 0, flag = 0;
	for (; i < len; i++) {
		if (flag == 0 && res[i] == '1') {
			flag = 1;
		} else if (res[i] == ',') break;
		else if (flag == 0) continue;
		ip[j++] = res[i];
	}
}
