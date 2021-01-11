#include "app.h"

char *peer_ip;
int DEVICE_ID = 1;
int run_greet = 0;


char *get_peer_ip(void) {
    return peer_ip;
}

int get_device_id(void) {
    return DEVICE_ID;
}

void set_peer_ip(char *p) {
    peer_ip = p;
}

int get_run_greet() {
    return run_greet;
}

int get_response(int clear, char *res)
{
    if (USART3_RX_STA & 0X8000)
    {                                              //���յ�һ��������
        USART3_RX_BUF[USART3_RX_STA & 0X7FFF] = 0; //��ӽ�����
        sprintf(res, "%s", USART3_RX_BUF);         //���͵�����
        if (clear)
            USART3_RX_STA = 0;
        return 1;
    }
    else
        return 0;
}

void erase_data()
{
    USART3_RX_STA = 0;
}

int send_command_with_retry(char *cmd, int timeout, int retry, int erase, char *res)
{
    int flag = 0, cnt = 0;
    while (flag == 0 && cnt < retry)
    {
        if (atk_8266_send_cmd((u8 *)cmd, (u8 *)"OK", timeout + cnt * 200) == 0) {
            flag = 1;
            break;
        }
        cnt++;
        printf("Command %s failed, retrying...\n", cmd);
    }
    if (flag)
    {
        printf("Command %s succeed\n", cmd);
        if (res != NULL)
            get_response(erase, res);
        else if (erase)
            erase_data();
    }
    else
        printf("Command %s failed after %d times of retry.\n", cmd, retry);
    return flag;
}

void send_command_util_success(char *cmd, int timeout, int erase, char *res)
{
    int flag = 0;
    printf("Begin sending command %s\n", cmd);
    while (flag == 0)
    {
        if (atk_8266_send_cmd((u8 *)cmd, (u8 *)"OK", timeout) == 0)
            flag = 1;
        // printf("Command %s failed, retrying...\n", cmd);
    }
    printf("Command %s succeed\n", cmd);
    if (res != NULL)
        get_response(erase, res);
    else if (erase)
        erase_data();
}

void wait_for_data(int clear, char *res)
{
    int flag = 0;
    while (flag == 0)
    {
        if (get_response(clear, res) == 1)
            flag = 1;
        delay_ms(200);
    }
}

int wait_for_data_with_timeout(int clear, char *res, int time_out) {
    int flag = 0, time_count = 0;
    while (flag == 0)
    {
        if (get_response(clear, res) == 1)
            flag = 1;
        delay_ms(200);
        time_count += 200;
        if (time_count > time_out) return 0;
    }
    return 1;
}

void extract_peer_ip(char *res, char *ip)
{
    int len = strlen(res);
    int i = 0, j = 0, flag = 0;
    for (; i < len; i++)
    {
        if (flag == 0 && res[i] == '1')
        {
            flag = 1;
        }
        else if (res[i] == ',')
            break;
        else if (flag == 0)
            continue;
        ip[j++] = res[i];
    }
}

void extarct_ap_ip(char *res, char *ip) {
    int len = strlen(res);
    int i = 0, j = 0, flag = 0;
    for (; i < len; i++)
    {
        if (flag == 0 && res[i] == '1')
        {
            flag = 1;
        }
        else if (res[i] == '\"' && flag == 1)
            break;
        else if (flag == 0)
            continue;
        ip[j++] = res[i];
    }
}

void index_show(u16 index, u16 total)
{
    //��ʾ��ǰ��Ŀ������,������Ŀ��
    LCD_ShowxNum(30 + 0, 230, index, 3, 16, 0X80); //����
    LCD_ShowChar(30 + 24, 230, '/', 16, 0);
    LCD_ShowxNum(30 + 32, 230, total, 3, 16, 0X80); //����Ŀ
}

int get_music_file_names(char **music_names, char **picture_names) {
    u16 total_file_count, *file_index_tbl;
    DIR dir;
    u8 curindex, *fn, ff_res;

    while (f_opendir(&dir, "0:/MUSIC") || f_opendir(&dir, "0:/PICTURE"))
    {
        Show_Str(60, 190, 240, 16, "MUSIC�ļ��д���!", 16, 0);
        delay_ms(200);
        LCD_Fill(60, 190, 240, 206, WHITE);
        delay_ms(200);
    }
    total_file_count = audio_get_tnum("0:/MUSIC");
    while (total_file_count == NULL)
    {
        Show_Str(60, 190, 240, 16, "û�������ļ�!", 16, 0);
        delay_ms(200);
        LCD_Fill(60, 190, 240, 146, WHITE);
        delay_ms(200);
    }

    FILINFO fileinfo;
    fileinfo.lfsize = _MAX_LFN * 2 + 1;                     //���ļ�����󳤶�
    fileinfo.lfname = mymalloc(SRAMIN, fileinfo.lfsize); //Ϊ���ļ������������ڴ�

    file_index_tbl = mymalloc(SRAMIN, 2 * total_file_count);

    ff_res = f_opendir(&dir, "0:/MUSIC");
    if (ff_res == FR_OK)
    {
        curindex = 0;
        while (1)
        {
            ff_res = f_readdir(&dir, &fileinfo);
            if (ff_res != FR_OK || fileinfo.fname[0] == 0)
                break;
            fn = (u8 *)(*fileinfo.lfname ? fileinfo.lfname : fileinfo.fname);
            strcat(music_names[curindex], "0:/MUSIC/");
            strcat(music_names[curindex], fn);
            curindex++;
        }
    }
    ff_res = f_opendir(&dir, "0:/PICTURE");
    if (ff_res == FR_OK)
    {
        curindex = 0;
        while (1)
        {
            ff_res = f_readdir(&dir, &fileinfo);
            if (ff_res != FR_OK || fileinfo.fname[0] == 0)
                break;
            fn = (u8 *)(*fileinfo.lfname ? fileinfo.lfname : fileinfo.fname);
            strcat(picture_names[curindex], "0:/PICTURE/");
            strcat(picture_names[curindex], fn);
            curindex++;
        }
    }

    return total_file_count;
}

void music_player()
{
    u8 res, *music_name, *img_name, key;
    u16 curindex;  //��ǰ����

    WM8978_ADDA_Cfg(1, 0);     //����DAC
    WM8978_Input_Cfg(0, 0, 0); //�ر�����ͨ��
    WM8978_Output_Cfg(1, 0);   //����DAC���

    char **music_names, **picture_names;
    music_names = (char**) mymalloc(SRAMIN, 5 * sizeof(char**));
    picture_names = (char**) mymalloc(SRAMIN, 5 * sizeof(char**));
    for (int i = 0; i < 5; i++) music_names[i] = (char*) mymalloc(SRAMIN, 30 * sizeof(char*));
    for (int i = 0; i < 5; i++) picture_names[i] = (char*) mymalloc(SRAMIN, 30 * sizeof(char*));

    int total_file_count = get_music_file_names(music_names, picture_names);

    curindex = 0;                                        //��0��ʼ��ʾ
    while (1) //�򿪳ɹ�
    {
        music_name = music_names[curindex];
        img_name = picture_names[curindex];

        LCD_Fill(30, 190, 240, 190 + 16, WHITE);    //���֮ǰ����ʾ
        Show_Str(30, 190, 240 - 60, 16, music_name, 16, 0); //��ʾ��������
        index_show(curindex + 1, total_file_count);

        LCD_Fill(30, 280, 330, 600, WHITE);
        ai_load_picfile(img_name, 30, 300, 300, 300, 1);

        key = audio_play_song(music_name); //���������Ƶ�ļ�

        if (key == KEY2_PRES) //��һ��
        {
            if (curindex)
                curindex--;
            else
                curindex = total_file_count - 1;
        }
        else if (key == KEY0_PRES) //��һ��
        {
            curindex++;
            if (curindex >= total_file_count)
                curindex = 0; //��ĩβ��ʱ��,�Զ���ͷ��ʼ
        }
        else if (key == KEY1_PRES) {
            receive_music();
        } else if (key == WKUP_PRES) {
            send_music(music_names, picture_names);
        }
        else
            break; //�����˴���
    }
}

void test_write_file()
{
    FIL fil;
    char fname[20] = "0:/test.txt";
    char wtext[20] = "Test text";
    u32 cnt;

    FRESULT res = f_open(&fil, fname, FA_CREATE_ALWAYS | FA_WRITE);
    if (res)
    {
        printf("Open file error : %d\r\n", res);
        return;
    }
    res = f_write(&fil, wtext, sizeof(wtext), (void *)&cnt);
    if (res)
    {
        printf("Write file error : %d\r\n", res);
        return;
    }
    res = f_close(&fil);
    if (res)
    {
        printf("Close file error : %d\r\n", res);
        return;
    }
}

int open_big_file(char *fname, FIL *fp)
{
    FRESULT res = f_open(fp, fname, FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
    return res;
}

int write_data(u8 *data, int len, FIL *fp)
{
    data[len] = '\0';
    int r_len;
    FRESULT res = f_write(fp, data, sizeof(data), (void *)&r_len);
    if (r_len != len || res)
        return res || 1;
    else
        return res;
}

int read_data(u8 *data, int chalk_size, FIL *fp) {
    return 0;
}

int end_data(FIL *fp)
{
    FRESULT res = f_close(fp);
    return res;
}

void receive_music()
{
}

// ������send_packet��wait_for_packet���շ����ݰ���һ�����ݰ���󳤶���256���ֽڡ�
// ʹ��FATFS�Ķ�д����ȡ��д���ļ������Բο�test_write_file()����������Ҳ���ˣ������Ļ�û���
void send_music(char **music_names, char **picture_names) {

}

void ui_show()
{
    // LCD_Color_Fill(0, 0, 170, 480, WHITE);
    LCD_Clear(WHITE);

    POINT_COLOR = RED;
    LCD_ShowString(30, 50, 200, 16, 16, "Explorer STM32F4");
    LCD_ShowString(30, 70, 200, 16, 16, "2021/1/7");
    POINT_COLOR = BLUE; //��������Ϊ��ɫ
    LCD_ShowString(30, 90, 200, 16, 16, "LSENS_VAL:");

    POINT_COLOR = BLUE;                                        //��������Ϊ��ɫ
    LCD_ShowString(30, 110, 200, 16, 16, "TEMPERATE: 00.00C"); //���ڹ̶�λ����ʾС����

    POINT_COLOR = RED;
    Show_Str(30, 130, 200, 16, "KEY0:NEXT   KEY2:PREV", 16, 0);
    Show_Str(30, 150, 200, 16, "KEY_UP:PAUSE/PLAY", 16, 0);
}

void send_packet(char *s, int len) {
    char cmd[30], res[30];
    int cmd_res;
    sprintf(cmd, "AT+CIPSEND=0,%d", len);
    while (1) {
        cmd_res = send_command_with_retry(cmd, 200, 3, 1, NULL);
        if (!cmd_res) continue;
        cmd_res = send_command_with_retry(s, 2000, 1, 1, NULL);
        if (cmd_res) break;
    }
    wait_for_data(1, res);
}

// Be sure no data is already received
void wait_for_packet(char *res) {
    char buffer[MAX_PACKET_LEN + 1];
    erase_data();
    wait_for_data(1, buffer);
    delay_ms(PACKET_INTERVAL);

    int flag = 0, i = 0, cmd_res;
    while (buffer[i] != 0) {
        if (buffer[i] == ':') flag = 1;
        if (flag) res[i] = buffer[i];
        i++;
    }
    res[i] = 0;
    while (1) {
        cmd_res = send_command_with_retry("AT+CIPSEND=0,2", 200, 3, 1, NULL);
        if (!cmd_res) continue;
        cmd_res = send_command_with_retry("OK", 2000, 1, 1, NULL);
        if (cmd_res) break;
    }
}
