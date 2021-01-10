#include "app.h"

char *peer_ip;
int DEVICE_ID = 1;


char *get_peer_ip(void) {
    return peer_ip;
}

int get_device_id(void) {
    return DEVICE_ID;
}

void set_peer_ip(char *p) {
    peer_ip = p;
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
        if (atk_8266_send_cmd((u8 *)cmd, (u8 *)"OK", timeout + cnt * 200) == 0)
            flag = 1;
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

void music_player()
{
    u8 res;
    DIR wavdir; //目录
    DIR imgdir;

    FILINFO wavfileinfo; //文件信息
    FILINFO imgfileinfo;

    u8 *fn;    //长文件名
    u8 *pname; //带路径的文件名
    u8 *imgname;

    u16 totwavnum; //图片文件总数
    u16 curindex;  //当前索引

    u8 key; //键值
    u16 temp;
    u16 *wavindextbl; //音乐索引表
    u16 *imgindextbl; //图片索引表

    WM8978_ADDA_Cfg(1, 0);     //开启DAC
    WM8978_Input_Cfg(0, 0, 0); //关闭输入通道
    WM8978_Output_Cfg(1, 0);   //开启DAC输出

    while (f_opendir(&wavdir, "0:/MUSIC"))
    { //打开音乐文件夹
        Show_Str(60, 190, 240, 16, "MUSIC文件夹错误!", 16, 0);
        delay_ms(200);
        LCD_Fill(60, 190, 240, 206, WHITE); //清除显示
        delay_ms(200);
    }
    totwavnum = audio_get_tnum("0:/MUSIC"); //得到总有效文件数
    while (totwavnum == NULL)
    { //音乐文件总数为0
        Show_Str(60, 190, 240, 16, "没有音乐文件!", 16, 0);
        delay_ms(200);
        LCD_Fill(60, 190, 240, 146, WHITE); //清除显示
        delay_ms(200);
    }
    wavfileinfo.lfsize = _MAX_LFN * 2 + 1;                     //长文件名最大长度
    wavfileinfo.lfname = mymalloc(SRAMIN, wavfileinfo.lfsize); //为长文件缓存区分配内存
    pname = mymalloc(SRAMIN, wavfileinfo.lfsize);              //为带路径的文件名分配内存

    imgfileinfo.lfsize = _MAX_LFN * 2 + 1;
    imgfileinfo.lfname = mymalloc(SRAMIN, imgfileinfo.lfsize);
    imgname = mymalloc(SRAMIN, imgfileinfo.lfsize);

    wavindextbl = mymalloc(SRAMIN, 2 * totwavnum);                                                    //申请2*totwavnum个字节的内存,用于存放音乐文件索引
    imgindextbl = mymalloc(SRAMIN, 2 * totwavnum);                                                    //申请图片索引表内存
    while (wavfileinfo.lfname == NULL || pname == NULL || wavindextbl == NULL || imgindextbl == NULL) //内存分配出错
    {
        Show_Str(60, 190, 240, 16, "内存分配失败!", 16, 0);
        delay_ms(200);
        LCD_Fill(60, 190, 240, 146, WHITE); //清除显示
        delay_ms(200);
    }

    //记录索引
    res = f_opendir(&wavdir, "0:/MUSIC"); //打开目录
    if (res == FR_OK)
    {
        curindex = 0; //当前索引为0
        while (1)
        {                                           //全部查询一遍
            temp = wavdir.index;                    //记录当前index
            res = f_readdir(&wavdir, &wavfileinfo); //读取目录下的一个文件
            if (res != FR_OK || wavfileinfo.fname[0] == 0)
                break; //错误了/到末尾了,退出
            fn = (u8 *)(*wavfileinfo.lfname ? wavfileinfo.lfname : wavfileinfo.fname);
            res = f_typetell(fn);
            if ((res & 0XFF) == T_WAV) //取高四位,看看是不是音乐文件
            {
                wavindextbl[curindex] = temp; //记录索引
                curindex++;
            }
        }
    }
    res = f_opendir(&imgdir, "0:/PICTURE");
    if (res == FR_OK)
    {
        curindex = 0;
        while (1)
        {
            temp = imgdir.index;
            res = f_readdir(&imgdir, &imgfileinfo);
            if (res != FR_OK || imgfileinfo.fname[0] == 0)
                break;
            fn = (u8 *)(*imgfileinfo.lfname ? imgfileinfo.lfname : imgfileinfo.fname);
            res = f_typetell(fn);
            if ((res & 0XF0) == T_JPG || T_BMP || T_JPEG)
            {
                imgindextbl[curindex] = temp;
                curindex++;
            }
        }
    }

    u8 music_dir_res, picture_dir_res;
    curindex = 0;                                        //从0开始显示
    res = f_opendir(&wavdir, (const TCHAR *)"0:/MUSIC"); //打开目录
    res = f_opendir(&imgdir, (const TCHAR *)"0:/PICTURE");
    while (res == FR_OK) //打开成功
    {
        dir_sdi(&wavdir, wavindextbl[curindex]); //改变当前目录索引
        dir_sdi(&imgdir, imgindextbl[curindex]);
        res = f_readdir(&wavdir, &wavfileinfo); //读取目录下的一个文件
        if (res != FR_OK || wavfileinfo.fname[0] == 0)
            break; //错误了/到末尾了,退出
        fn = (u8 *)(*wavfileinfo.lfname ? wavfileinfo.lfname : wavfileinfo.fname);
        strcpy((char *)pname, "0:/MUSIC/");         //复制路径(目录)
        strcat((char *)pname, (const char *)fn);    //将文件名接在后面
        LCD_Fill(30, 190, 240, 190 + 16, WHITE);    //清除之前的显示
        Show_Str(30, 190, 240 - 60, 16, fn, 16, 0); //显示歌曲名字
        index_show(curindex + 1, totwavnum);

        res = f_readdir(&imgdir, &imgfileinfo);
        if (res != FR_OK || imgfileinfo.fname[0] == 0)
            break; //错误了/到末尾了,退出
        fn = (u8 *)(*imgfileinfo.lfname ? imgfileinfo.lfname : imgfileinfo.fname);
        strcpy((char *)imgname, "0:/PICTURE/");    //复制路径(目录)
        strcat((char *)imgname, (const char *)fn); //将文件名接在后面
        printf("%s\n", imgname);
        LCD_Fill(30, 280, 330, 600, WHITE);
        ai_load_picfile(imgname, 30, 300, 300, 300, 1);

        key = audio_play_song(pname); //播放这个音频文件

        if (key == KEY2_PRES) //上一曲
        {
            if (curindex)
                curindex--;
            else
                curindex = totwavnum - 1;
        }
        else if (key == KEY0_PRES) //下一曲
        {
            curindex++;
            if (curindex >= totwavnum)
                curindex = 0; //到末尾的时候,自动从头开始
        }
        else if (key == KEY1_PRES) {
            receive_music();
        } else if (key == WKUP_PRES) {
            send_music();
        }
        else
            break; //产生了错误
    }
    myfree(SRAMIN, wavfileinfo.lfname); //释放内存
    myfree(SRAMIN, pname);              //释放内存
    myfree(SRAMIN, wavindextbl);        //释放内存
    myfree(SRAMIN, imgfileinfo.lfname);
    myfree(SRAMIN, imgindextbl);
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

void send_music() {

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

int send_packet(char *s, int len) {
    char cmd[30];
    char res[30];
    sprintf(cmd, "AT+CIPSEND=0,%d", len);
    send_command_util_success(cmd, 500, 1, NULL);
    send_command_util_success(s, 500, 1, NULL);
    wait_for_data(1, res);
    return 1;
}

int wait_for_packet(char *res) {
    while (1) {
        if (USART3_RX_STA & 0X8000)
        {
            USART3_RX_BUF[USART3_RX_STA & 0X7FFF] = 0;
            sprintf(res, "%s", USART3_RX_BUF);
            USART3_RX_STA = 0;
        }
    }
    
    wait_for_data(1, res);
    send_command_util_success("AT+CIPSEND=0,8", 500, 1, NULL);
    send_command_util_success("received", 500, 1, NULL);
}
