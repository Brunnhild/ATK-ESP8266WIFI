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
    //��ʾ��ǰ��Ŀ������,������Ŀ��
    LCD_ShowxNum(30 + 0, 230, index, 3, 16, 0X80); //����
    LCD_ShowChar(30 + 24, 230, '/', 16, 0);
    LCD_ShowxNum(30 + 32, 230, total, 3, 16, 0X80); //����Ŀ
}

void music_player()
{
    u8 res;
    DIR wavdir; //Ŀ¼
    DIR imgdir;

    FILINFO wavfileinfo; //�ļ���Ϣ
    FILINFO imgfileinfo;

    u8 *fn;    //���ļ���
    u8 *pname; //��·�����ļ���
    u8 *imgname;

    u16 totwavnum; //ͼƬ�ļ�����
    u16 curindex;  //��ǰ����

    u8 key; //��ֵ
    u16 temp;
    u16 *wavindextbl; //����������
    u16 *imgindextbl; //ͼƬ������

    WM8978_ADDA_Cfg(1, 0);     //����DAC
    WM8978_Input_Cfg(0, 0, 0); //�ر�����ͨ��
    WM8978_Output_Cfg(1, 0);   //����DAC���

    while (f_opendir(&wavdir, "0:/MUSIC"))
    { //�������ļ���
        Show_Str(60, 190, 240, 16, "MUSIC�ļ��д���!", 16, 0);
        delay_ms(200);
        LCD_Fill(60, 190, 240, 206, WHITE); //�����ʾ
        delay_ms(200);
    }
    totwavnum = audio_get_tnum("0:/MUSIC"); //�õ�����Ч�ļ���
    while (totwavnum == NULL)
    { //�����ļ�����Ϊ0
        Show_Str(60, 190, 240, 16, "û�������ļ�!", 16, 0);
        delay_ms(200);
        LCD_Fill(60, 190, 240, 146, WHITE); //�����ʾ
        delay_ms(200);
    }
    wavfileinfo.lfsize = _MAX_LFN * 2 + 1;                     //���ļ�����󳤶�
    wavfileinfo.lfname = mymalloc(SRAMIN, wavfileinfo.lfsize); //Ϊ���ļ������������ڴ�
    pname = mymalloc(SRAMIN, wavfileinfo.lfsize);              //Ϊ��·�����ļ��������ڴ�

    imgfileinfo.lfsize = _MAX_LFN * 2 + 1;
    imgfileinfo.lfname = mymalloc(SRAMIN, imgfileinfo.lfsize);
    imgname = mymalloc(SRAMIN, imgfileinfo.lfsize);

    wavindextbl = mymalloc(SRAMIN, 2 * totwavnum);                                                    //����2*totwavnum���ֽڵ��ڴ�,���ڴ�������ļ�����
    imgindextbl = mymalloc(SRAMIN, 2 * totwavnum);                                                    //����ͼƬ�������ڴ�
    while (wavfileinfo.lfname == NULL || pname == NULL || wavindextbl == NULL || imgindextbl == NULL) //�ڴ�������
    {
        Show_Str(60, 190, 240, 16, "�ڴ����ʧ��!", 16, 0);
        delay_ms(200);
        LCD_Fill(60, 190, 240, 146, WHITE); //�����ʾ
        delay_ms(200);
    }

    //��¼����
    res = f_opendir(&wavdir, "0:/MUSIC"); //��Ŀ¼
    if (res == FR_OK)
    {
        curindex = 0; //��ǰ����Ϊ0
        while (1)
        {                                           //ȫ����ѯһ��
            temp = wavdir.index;                    //��¼��ǰindex
            res = f_readdir(&wavdir, &wavfileinfo); //��ȡĿ¼�µ�һ���ļ�
            if (res != FR_OK || wavfileinfo.fname[0] == 0)
                break; //������/��ĩβ��,�˳�
            fn = (u8 *)(*wavfileinfo.lfname ? wavfileinfo.lfname : wavfileinfo.fname);
            res = f_typetell(fn);
            if ((res & 0XFF) == T_WAV) //ȡ����λ,�����ǲ��������ļ�
            {
                wavindextbl[curindex] = temp; //��¼����
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
    curindex = 0;                                        //��0��ʼ��ʾ
    res = f_opendir(&wavdir, (const TCHAR *)"0:/MUSIC"); //��Ŀ¼
    res = f_opendir(&imgdir, (const TCHAR *)"0:/PICTURE");
    while (res == FR_OK) //�򿪳ɹ�
    {
        dir_sdi(&wavdir, wavindextbl[curindex]); //�ı䵱ǰĿ¼����
        dir_sdi(&imgdir, imgindextbl[curindex]);
        res = f_readdir(&wavdir, &wavfileinfo); //��ȡĿ¼�µ�һ���ļ�
        if (res != FR_OK || wavfileinfo.fname[0] == 0)
            break; //������/��ĩβ��,�˳�
        fn = (u8 *)(*wavfileinfo.lfname ? wavfileinfo.lfname : wavfileinfo.fname);
        strcpy((char *)pname, "0:/MUSIC/");         //����·��(Ŀ¼)
        strcat((char *)pname, (const char *)fn);    //���ļ������ں���
        LCD_Fill(30, 190, 240, 190 + 16, WHITE);    //���֮ǰ����ʾ
        Show_Str(30, 190, 240 - 60, 16, fn, 16, 0); //��ʾ��������
        index_show(curindex + 1, totwavnum);

        res = f_readdir(&imgdir, &imgfileinfo);
        if (res != FR_OK || imgfileinfo.fname[0] == 0)
            break; //������/��ĩβ��,�˳�
        fn = (u8 *)(*imgfileinfo.lfname ? imgfileinfo.lfname : imgfileinfo.fname);
        strcpy((char *)imgname, "0:/PICTURE/");    //����·��(Ŀ¼)
        strcat((char *)imgname, (const char *)fn); //���ļ������ں���
        printf("%s\n", imgname);
        LCD_Fill(30, 280, 330, 600, WHITE);
        ai_load_picfile(imgname, 30, 300, 300, 300, 1);

        key = audio_play_song(pname); //���������Ƶ�ļ�

        if (key == KEY2_PRES) //��һ��
        {
            if (curindex)
                curindex--;
            else
                curindex = totwavnum - 1;
        }
        else if (key == KEY0_PRES) //��һ��
        {
            curindex++;
            if (curindex >= totwavnum)
                curindex = 0; //��ĩβ��ʱ��,�Զ���ͷ��ʼ
        }
        else if (key == KEY1_PRES) {
            receive_music();
        } else if (key == WKUP_PRES) {
            send_music();
        }
        else
            break; //�����˴���
    }
    myfree(SRAMIN, wavfileinfo.lfname); //�ͷ��ڴ�
    myfree(SRAMIN, pname);              //�ͷ��ڴ�
    myfree(SRAMIN, wavindextbl);        //�ͷ��ڴ�
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
    POINT_COLOR = BLUE; //��������Ϊ��ɫ
    LCD_ShowString(30, 90, 200, 16, 16, "LSENS_VAL:");

    POINT_COLOR = BLUE;                                        //��������Ϊ��ɫ
    LCD_ShowString(30, 110, 200, 16, 16, "TEMPERATE: 00.00C"); //���ڹ̶�λ����ʾС����

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
