#include "app.h"

char *peer_ip;
int DEVICE_ID = 1;
int run_greet = 0;

int stretch_window = 0;
int window_cnt = 0;
int long_sending_mode = 0;

int my_temp, adc;

void set_temp_adc(int a, int b) {
    my_temp = a;
    adc = b;
}
void exchange() {
    delay_ms(500);
    char cmd[30];
    sprintf(cmd, "%d:%d", my_temp, adc);
    send_packet(cmd, strlen(cmd));
}
void receive_temp_adc() {
    char res[50];
    int a, b;
    wait_for_packet(res);
    sscanf(res, "%d:%d", &a, &b);
    if (a < 0)
        {
            a = -a;
            LCD_ShowString(30 + 10 * 16, 110, 16, 16, 16, "-");
        }
        else
            LCD_ShowString(30 + 10 * 16, 110, 16, 16, 16, " ");

        LCD_ShowxNum(30 + 11 * 16, 110, a / 100, 2, 16, 0);
        LCD_ShowxNum(30 + 14 * 16, 110, a % 100, 2, 16, 0);
    LCD_ShowxNum(30 + 10 * 16, 90, b, 3, 16, 0);
}

FIL fil;

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

int is_it() {
    if (stretch_window == window_cnt) {
        window_cnt = 0;
        return 1;
    } else if (stretch_window > 0) {
        window_cnt++;
        return 0;
    }
}

void enable_long_sending() {
    long_sending_mode = 1;
}

void disable_long_sending() {
    long_sending_mode = 0;
}

int get_long_sending() {
    return long_sending_mode;
}

int get_response(int clear, char *res)
{
    if (USART3_RX_STA & 0X8000)
    {                                              //接收到一次数据了
        USART3_RX_BUF[USART3_RX_STA & 0X7FFF] = 0; //添加结束符
        sprintf(res, "%s", USART3_RX_BUF);         //发送到串口
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
    //显示当前曲目的索引,及总曲目数
    LCD_ShowxNum(30 + 0, 230, index, 3, 16, 0X80); //索引
    LCD_ShowChar(30 + 24, 230, '/', 16, 0);
    LCD_ShowxNum(30 + 32, 230, total, 3, 16, 0X80); //总曲目
}

int get_music_file_names(char **music_names, char **picture_names) {
    u16 total_file_count, *file_index_tbl;
    DIR dir;
    u8 curindex, *fn, ff_res;

    while (f_opendir(&dir, "0:/MUSIC") || f_opendir(&dir, "0:/PICTURE"))
    {
        Show_Str(60, 190, 240, 16, "MUSIC文件夹错误!", 16, 0);
        delay_ms(200);
        LCD_Fill(60, 190, 240, 206, WHITE);
        delay_ms(200);
    }
    total_file_count = audio_get_tnum("0:/MUSIC");
    while (total_file_count == NULL)
    {
        Show_Str(60, 190, 240, 16, "没有音乐文件!", 16, 0);
        delay_ms(200);
        LCD_Fill(60, 190, 240, 146, WHITE);
        delay_ms(200);
    }

    FILINFO fileinfo;
    fileinfo.lfsize = _MAX_LFN * 2 + 1;                     //长文件名最大长度
    fileinfo.lfname = mymalloc(SRAMIN, fileinfo.lfsize); //为长文件缓存区分配内存

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
    u16 curindex;  //当前索引

    WM8978_ADDA_Cfg(1, 0);     //开启DAC
    WM8978_Input_Cfg(0, 0, 0); //关闭输入通道
    WM8978_Output_Cfg(1, 0);   //开启DAC输出

    char **music_names, **picture_names;
    music_names = (char**) mymalloc(SRAMIN, 5 * sizeof(char**));
    picture_names = (char**) mymalloc(SRAMIN, 5 * sizeof(char**));
    for (int i = 0; i < 5; i++) music_names[i] = (char*) mymalloc(SRAMIN, 30 * sizeof(char*));
    for (int i = 0; i < 5; i++) picture_names[i] = (char*) mymalloc(SRAMIN, 30 * sizeof(char*));

    int total_file_count = get_music_file_names(music_names, picture_names);

    curindex = 0;                                        //从0开始显示
    while (1) //打开成功
    {
        music_name = music_names[curindex];
        img_name = picture_names[curindex];

        LCD_Fill(30, 190, 240, 190 + 16, WHITE);    //清除之前的显示
        Show_Str(30, 190, 240 - 60, 16, music_name, 16, 0); //显示歌曲名字
        index_show(curindex + 1, total_file_count);

        LCD_Fill(30, 280, 330, 600, WHITE);
        ai_load_picfile(img_name, 30, 300, 300, 300, 1);

        key = audio_play_song(music_name); //播放这个音频文件

        if (key == KEY2_PRES) //上一曲
        {
            if (curindex)
                curindex--;
            else
                curindex = total_file_count - 1;
        }
        else if (key == KEY0_PRES) //下一曲
        {
            curindex++;
            if (curindex >= total_file_count)
                curindex = 0; //到末尾的时候,自动从头开始
        }
        else if (key == KEY1_PRES) {
            if (DEVICE_ID == 1) send_picture(music_names, picture_names);
            else receive_picture();
        } else if (key == WKUP_PRES) {
            if (DEVICE_ID == 1) exchange();
            else receive_temp_adc();
        }
        else
            break; //产生了错误
    }
}

void test_write_file()
{
    FIL fil;
    char fname[20] = "0:/PICTURE/1.jpg";
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

void extract_fname(char *res, char *fname)
{
    int len = strlen(res);
    int i = 2, j = 0, flag = 0;
    for (; i < len; i++)
    {
        if (flag == 0 && res[i - 1] == 'E' && res[i] == '/')
        {
            flag = 1;
        }
        else if (res[i - 2] == ('j') && res[i - 1] == ('p') && res[i] == ('g'))
        {
            flag = 0;
        }
        else if (flag == 0)
            continue;
        fname[j++] = res[i];
    }
    return;
}

int extract_end(char *res)
{
    int len = strlen(res);
    int i = 0;
    for (; i < len; i++)
    {
        if (res[i - 2] == 'E' && res[i - 1] == 'N' && res[i] == 'D')
        {
            return i - 3;
        }
    }
    return 256;
}

void do_stretch_window(void) {
    stretch_window = WINDOW_SIZE;
    window_cnt = 0;
}

void destretch_window(void) {
    stretch_window = 0;
    window_cnt = 0;
}

void write_to_picture(u8 *data, int len) {
    int cnt;
    f_write(&fil, data, len, (void *)&cnt);
    printf("%d:%d write\n", len, cnt);
}

void end_writing_picture(void) {
    int res1 = f_close(&fil);
    if (res1) {
        printf("Close file error : %d\r\n", res1);
    }
    disable_long_sending();
    printf("END! \n");
}

void receive_picture()
{
    LCD_Fill(30, 280, 330, 600, WHITE);
    LCD_ShowString(30, 300, 200, 16, 16, "RECEIVING...");
    char res[MAX_DATA_LEN + 1];
    u32 *cnt;
    FRESULT res1;
    int i;

    wait_for_packet(res);

    printf("%s\n", res);
    f_unlink(res);
    res1 = f_open(&fil, res, FA_CREATE_NEW | FA_WRITE | FA_READ);
    res[0] = 0;

    if (res1)
    {
        printf("Open file error : %d\r\n", res1);
        return;
    }
    erase_data();
    enable_long_sending();
    while (get_long_sending()) delay_ms(200);
}

// 可以用send_packet和wait_for_packet来收发数据包，一个数据包最大长度是256个字节。
// 使用FATFS的读写来读取与写入文件，可以参考test_write_file()，这个函数我测过了，其它的还没测过
void send_picture(char **music_names, char **picture_names)
{
    LCD_Fill(30, 280, 330, 600, WHITE);
    LCD_ShowString(30, 300, 200, 16, 16, "SEND...");
    FIL fil;
    char *fname;
    char rtext[MAX_DATA_LEN + 1];
    u32 cnt, fsize;
    long int i;
    FRESULT res;

    fname = picture_names[0];
    fname = "0:/test.bin";
    send_packet(fname, strlen(fname));
    res = f_open(&fil, fname, FA_OPEN_EXISTING | FA_READ);
    if (res)
    {
        printf("Open file error : %d\r\n", res);
        return;
    }
    fsize = fil.fsize;
    for (i = 0; i <= fsize / MAX_DATA_LEN; i++)
    {
        res = f_read(&fil, rtext, MAX_DATA_LEN, (void *)&cnt);
        rtext[cnt] = 0;
        if (res)
        {
            printf("Read file error : %d\r\n", res);
            return;
        }
        send_long_data(rtext, cnt);
    }
    send_long_data("END\n", strlen("END\n"));

    res = f_close(&fil);
    if (res)
    {
        printf("Close file error : %d\r\n", res);
        return;
    }
}

void ui_show()
{
    // LCD_Color_Fill(0, 0, 170, 480, WHITE);
    LCD_Clear(WHITE);

    POINT_COLOR = RED;
    LCD_ShowString(30, 50, 200, 16, 16, "Explorer STM32F4");
    LCD_ShowString(30, 70, 200, 16, 16, "2021/1/7");
    POINT_COLOR = BLUE; //设置字体为蓝色
    LCD_ShowString(30, 90, 200, 16, 16, "LSENS_VAL:");

    POINT_COLOR = BLUE;                                        //设置字体为蓝色
    LCD_ShowString(30, 110, 200, 16, 16, "TEMPERATE: 00.00C"); //先在固定位置显示小数点

    POINT_COLOR = RED;
    Show_Str(30, 130, 200, 16, "KEY0:NEXT   KEY2:PREV", 16, 0);
    Show_Str(30, 150, 200, 16, "KEY_UP:PAUSE/PLAY", 16, 0);
}

void base64_encode(u8 *src, u8 *res, int str_len)
{
    long len;
    int i, j;
    u8 *base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    if (str_len % 3 == 0)
        len = str_len / 3 * 4;
    else
        len = (str_len / 3 + 1) * 4;

    res[len] = '\0';

    for (i = 0, j = 0; i < len - 2; j += 3, i += 4)
    {
        res[i] = base64_table[src[j] >> 2];
        res[i + 1] = base64_table[(src[j] & 0x3) << 4 | (src[j + 1] >> 4)];
        res[i + 2] = base64_table[(src[j + 1] & 0xf) << 2 | (src[j + 2] >> 6)];
        res[i + 3] = base64_table[src[j + 2] & 0x3f];
    }

    switch (str_len % 3)
    {
    case 1:
        res[i - 2] = '=';
        res[i - 1] = '=';
        break;
    case 2:
        res[i - 1] = '=';
        break;
    }
}

void base64_decode(u8 *src, u8 *res)
{
    int table[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                   0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 0,
                   63, 52, 53, 54, 55, 56, 57, 58,
                   59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0,
                   1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                   13, 14, 15, 16, 17, 18, 19, 20, 21,
                   22, 23, 24, 25, 0, 0, 0, 0, 0, 0, 26,
                   27, 28, 29, 30, 31, 32, 33, 34, 35,
                   36, 37, 38, 39, 40, 41, 42, 43, 44,
                   45, 46, 47, 48, 49, 50, 51};
    long len;
    long str_len;
    int i, j;

    len = strlen(src);
    if (strstr(src, "=="))
        str_len = len / 4 * 3 - 2;
    else if (strstr(src, "="))
        str_len = len / 4 * 3 - 1;
    else
        str_len = len / 4 * 3;

    res[str_len] = '\0';

    for (i = 0, j = 0; i < len - 2; j += 3, i += 4)
    {
        res[j] = ((unsigned char)table[src[i]]) << 2 |
                 (((unsigned char)table[src[i + 1]]) >> 4);
        res[j + 1] = (((unsigned char)table[src[i + 1]]) << 4) | (((unsigned char)table[src[i + 2]]) >> 2);
        res[j + 2] = (((unsigned char)table[src[i + 2]]) << 6) |
                     ((unsigned char)table[src[i + 3]]);
    }
}

void send_long_data(char *s, int len) {
    printf("%d bytes are sending...\n", len);
    char cmd[30], res[30], base64[MAX_PACKET_LEN + 1];
    int cmd_res, tmp_len;
    base64_encode(s, base64, len);
    tmp_len = strlen(base64);
    base64[tmp_len] = '\n';
    base64[tmp_len + 1] = '\0';
    // printf("%s\n", base64);
    sprintf(cmd, "AT+CIPSEND=0,%d", strlen(base64));
    while (1) {
        cmd_res = send_command_with_retry(cmd, 200, 3, 1, NULL);
        if (!cmd_res) continue;
        cmd_res = send_command_with_retry(base64, 3000, 1, 1, NULL);
        if (cmd_res) break;
    }
    delay_ms(PACKET_INTERVAL);
}

void send_packet(char *s, int len) {
    printf("%d bytes are sending...\n", len);
    char cmd[30], res[30], base64[MAX_PACKET_LEN + 1];
    int cmd_res;
    base64_encode(s, base64, len);
    // printf("%s\n", base64);
    sprintf(cmd, "AT+CIPSEND=0,%d", strlen(base64));
    while (1) {
        cmd_res = send_command_with_retry(cmd, 200, 3, 1, NULL);
        if (!cmd_res) continue;
        cmd_res = send_command_with_retry(base64, 3000, 1, 1, NULL);
        if (cmd_res) break;
    }
    wait_for_data(1, res);
    delay_ms(PACKET_INTERVAL);
}

// Be sure no data is already received
void wait_for_packet(char *res) {
    char buffer[MAX_PACKET_LEN + 30];
    char *bp = buffer;
    erase_data();
    do_stretch_window();
    wait_for_data(1, buffer);
    destretch_window();
    while (*bp != ':') bp++;
    bp++;
    base64_decode(bp, res);
    int cnt = strlen(res), cmd_res;
    printf("Received %d bytes...\n", cnt);
    printf("%s\n", res);
    delay_ms(PACKET_INTERVAL);
    
    while (1) {
        cmd_res = send_command_with_retry("AT+CIPSEND=0,2", 200, 3, 1, NULL);
        if (!cmd_res) continue;
        cmd_res = send_command_with_retry("OK", 2000, 1, 1, NULL);
        if (cmd_res) break;
    }
}
