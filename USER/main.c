#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "lcd.h"
#include "adc.h"
#include "key.h"
#include "usmart.h"
#include "sram.h"
#include "malloc.h"
#include "w25qxx.h"
#include "sdio_sdcard.h"
#include "ff.h"
#include "exfuns.h"
#include "fontupd.h"
#include "text.h"
#include "usmart.h"
#include "touch.h"
#include "usart3.h"
#include "common.h"
#include "adc3.h"
#include "lsens.h"
#include "app.h"

//ALIENTEK ̽����STM32F407������ ʵ��19
//�ڲ��¶ȴ�����ʵ�� -�⺯���汾
//����֧�֣�www.openedv.com
//�Ա����̣�http://eboard.taobao.com
//������������ӿƼ����޹�˾
//���ߣ�����ԭ�� @ALIENTEK

char *peer_ip;
int DEVICE_ID = 1;

int main(void)
{
    u8 key, fontok = 0;
    usart3_init(115200); //��ʼ������3������Ϊ115200
    KEY_Init();          //������ʼ��
    W25QXX_Init();       //��ʼ��W25Q128
    // tp_dev.init();				//��ʼ��������
    usmart_dev.init(168);                           //��ʼ��USMART
    my_mem_init(SRAMIN);                            //��ʼ���ڲ��ڴ��
    my_mem_init(SRAMCCM);                           //��ʼ��CCM�ڴ��
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //����ϵͳ�ж����ȼ�����2
    delay_init(168);                                //��ʼ����ʱ����
    uart_init(115200);                              //��ʼ�����ڲ�����Ϊ115200

    LED_Init();               //��ʼ��LED
    LCD_Init();               //Һ����ʼ��
    Adc_Init();               //�ڲ��¶ȴ�����ADC��ʼ��
    W25QXX_Init();            //��ʼ��W25Q128
    WM8978_Init();            //��ʼ��WM8978
    WM8978_HPvol_Set(40, 40); //������������
    WM8978_SPKvol_Set(50);    //������������
    exfuns_init();            //Ϊfatfs��ر��������ڴ�
    f_mount(fs[0], "0:", 1);  //����SD��
    f_mount(fs[1], "1:", 1);  //����FLASH.
    piclib_init();            //��ʼ����ͼ
    key = KEY_Scan(0);

    if (key == KEY0_PRES) //ǿ��У׼
    {
        LCD_Clear(WHITE); //����
        TP_Adjust();      //��ĻУ׼
        TP_Save_Adjdata();
        LCD_Clear(WHITE); //����
    }
    fontok = font_init();           //����ֿ��Ƿ�OK
    if (fontok || key == KEY1_PRES) //��Ҫ�����ֿ�
    {
        LCD_Clear(WHITE);  //����
        POINT_COLOR = RED; //��������Ϊ��ɫ
        LCD_ShowString(60, 50, 200, 16, 16, "ALIENTEK STM32");
        while (SD_Init()) //���SD��
        {
            LCD_ShowString(60, 70, 200, 16, 16, "SD Card Failed!");
            delay_ms(200);
            LCD_Fill(60, 70, 200 + 60, 70 + 16, WHITE);
            delay_ms(200);
        }
        LCD_ShowString(60, 70, 200, 16, 16, "SD Card OK");
        LCD_ShowString(60, 90, 200, 16, 16, "Font Updating...");
        key = update_font(20, 110, 16, "0:"); //��SD������
        while (key)                           //����ʧ��
        {
            LCD_ShowString(60, 110, 200, 16, 16, "Font Update Failed!");
            delay_ms(200);
            LCD_Fill(20, 110, 200 + 20, 110 + 16, WHITE);
            delay_ms(200);
        }
        LCD_ShowString(60, 110, 200, 16, 16, "Font Update Success!");
        delay_ms(1500);
        LCD_Clear(WHITE); //����
    }
    // atk_8266_test();		//����ATK_ESP8266����
    // test_write_file();

    while (atk_8266_send_cmd("AT", "OK", 20)) //���WIFIģ���Ƿ�����
    {
        atk_8266_quit_trans();                        //�˳�͸��
        atk_8266_send_cmd("AT+CIPMODE=0", "OK", 200); //�ر�͸��ģʽ
        Show_Str(40, 55, 200, 16, "δ��⵽ģ��!!!", 16, 0);
        delay_ms(800);
        LCD_Fill(40, 55, 200, 55 + 16, WHITE);
        Show_Str(40, 55, 200, 16, "��������ģ��...", 16, 0);
    }
    LCD_Clear(WHITE); //����

    int run_greet = 0;
    // Start the server and client and send greetings
    if (DEVICE_ID == 1 && run_greet)
    {
        send_command_with_retry("AT+CWMODE=2", 200, 3, 1, NULL);
        send_command_with_retry("AT+CWSAP=\"ATK-ESP8266\",\"12345678\",1,4", 200, 3, 1, NULL);
        send_command_with_retry("AT+CIPMUX=1", 200, 3, 1, NULL);
        send_command_with_retry("AT+CIPSERVER=1,8086", 200, 3, 1, NULL);

        // Wait for the peer to join
        peer_ip = (char *)malloc(20);
        char res[50];
        while (1)
        {
            send_command_util_success("AT+CWLIF", 200, 1, res);
            extract_peer_ip(res, peer_ip);
            if (peer_ip[3] == '.' && peer_ip[7] == '.')
                break;
            delay_ms(500);
        }
        printf("The peer ip is %s\n", peer_ip);

        char *cmd_tmp = (char *)malloc(100);
        sprintf(cmd_tmp, "AT+CIPSTART=0,\"TCP\",\"%s\",8086", peer_ip);
        send_command_with_retry(cmd_tmp, 500, 5, 1, NULL);

        send_command_with_retry("AT+CIPSEND=0,17", 200, 3, 1, NULL);
        send_command_with_retry("the LORD your God", 200, 3, 1, NULL);
        send_command_with_retry("AT+CIPCLOSE=0", 200, 3, 1, NULL);
        printf("Greeting sent\n");

        //		wait_for_data(1, res);
        printf("Greeting from %s: %s\n", peer_ip, res);
    }
    else if (DEVICE_ID == 2 && run_greet)
    {
        send_command_with_retry("AT+CWMODE=3", 200, 3, 1, NULL);
        send_command_util_success("AT+CWJAP=\"ATK-ESP8266\",\"12345678\",1,4", 500, 1, NULL);
        send_command_with_retry("AT+CIPMUX=1", 200, 3, 1, NULL);
        send_command_with_retry("AT+CIPSERVER=1,8086", 200, 3, 1, NULL);

        peer_ip = (char *)malloc(20);
        char res[50];
        while (1)
        {
            send_command_util_success("AT+CIPAP?", 200, 1, res);
            extract_peer_ip(res, peer_ip);
            if (peer_ip[3] == '.' && peer_ip[7] == '.')
                break;
            delay_ms(500);
        }
        printf("The peer ip is %s\n", peer_ip);

        char *cmd_tmp = (char *)malloc(100);
        sprintf(cmd_tmp, "AT+CIPSTART=0,\"TCP\",\"%s\",8086", peer_ip);
        send_command_with_retry(cmd_tmp, 500, 5, 1, NULL);

        wait_for_data(1, res);
        printf("Greeting from %s: %s\n", peer_ip, res);

        send_command_with_retry("AT+CIPSEND=0,17", 200, 3, 1, NULL);
        send_command_with_retry("the LORD your God", 200, 3, 1, NULL);
        send_command_with_retry("AT+CIPCLOSE=0", 200, 3, 1, NULL);

        printf("Greeting sent\n");
    }

    u8 adcx;
    Lsens_Init(); //��ʼ������������
    POINT_COLOR = RED;
    LCD_ShowString(30, 50, 200, 16, 16, "Explorer STM32F4");
    LCD_ShowString(30, 70, 200, 16, 16, "LSENS TEST");
    LCD_ShowString(30, 90, 200, 16, 16, "ATOM@ALIENTEK");
    LCD_ShowString(30, 110, 200, 16, 16, "2014/5/7");
    POINT_COLOR = BLUE; //��������Ϊ��ɫ
    LCD_ShowString(30, 130, 200, 16, 16, "LSENS_VAL:");

    short temp;
    POINT_COLOR = RED;
    POINT_COLOR = BLUE;                                        //��������Ϊ��ɫ
    LCD_ShowString(30, 150, 200, 16, 16, "TEMPERATE: 00.00C"); //���ڹ̶�λ����ʾС����

    // while (1)
    // {
    //     adcx = Lsens_Get_Val();
    //     LCD_ShowxNum(30 + 10 * 8, 130, adcx, 3, 16, 0); //��ʾADC��ֵ
    //     LED0 = !LED0;
    //     temp = Get_Temprate(); //�õ��¶�ֵ
    //     if (temp < 0)
    //     {
    //         temp = -temp;
    //         LCD_ShowString(30 + 10 * 8, 140, 16, 16, 16, "-"); //��ʾ����
    //     }
    //     else
    //         LCD_ShowString(30 + 10 * 8, 140, 16, 16, 16, " "); //�޷���

    //     LCD_ShowxNum(30 + 11 * 8, 150, temp / 100, 2, 16, 0); //��ʾ��������
    //     LCD_ShowxNum(30 + 14 * 8, 150, temp % 100, 2, 16, 0); //��ʾС������

    //     printf("Temperature now is: %d\n", temp);
    //     printf("Lightness now is: %d\n", adcx);
    //     printf("\n");

    //     delay_ms(250);
    // }

    POINT_COLOR = RED;
    while (font_init()) //����ֿ�
    {
        LCD_ShowString(30, 50, 200, 16, 16, "Font Error!");
        delay_ms(200);
        LCD_Fill(30, 50, 240, 66, WHITE); //�����ʾ
        delay_ms(200);
    }
    POINT_COLOR = RED;
    Show_Str(60, 50, 200, 16, "Explorer STM32F4������", 16, 0);
    Show_Str(60, 70, 200, 16, "���ֲ�����ʵ��", 16, 0);
    Show_Str(60, 90, 200, 16, "����ԭ��@ALIENTEK", 16, 0);
    Show_Str(60, 110, 200, 16, "2014��5��24��", 16, 0);
    Show_Str(60, 130, 200, 16, "KEY0:NEXT   KEY2:PREV", 16, 0);
    Show_Str(60, 150, 200, 16, "KEY_UP:PAUSE/PLAY", 16, 0);
    while (1)
    {
        music_player();
    }
}
