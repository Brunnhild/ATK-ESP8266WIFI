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

//ALIENTEK 探索者STM32F407开发板 实验19
//内部温度传感器实验 -库函数版本
//技术支持：www.openedv.com
//淘宝店铺：http://eboard.taobao.com
//广州市星翼电子科技有限公司
//作者：正点原子 @ALIENTEK

char *peer_ip;
int DEVICE_ID = 1;

int main(void)
{
    u8 key, fontok = 0;
    usart3_init(115200); //初始化串口3波特率为115200
    KEY_Init();          //按键初始化
    W25QXX_Init();       //初始化W25Q128
    // tp_dev.init();				//初始化触摸屏
    usmart_dev.init(168);                           //初始化USMART
    my_mem_init(SRAMIN);                            //初始化内部内存池
    my_mem_init(SRAMCCM);                           //初始化CCM内存池
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置系统中断优先级分组2
    delay_init(168);                                //初始化延时函数
    uart_init(115200);                              //初始化串口波特率为115200

    LED_Init();               //初始化LED
    LCD_Init();               //液晶初始化
    Adc_Init();               //内部温度传感器ADC初始化
    W25QXX_Init();            //初始化W25Q128
    WM8978_Init();            //初始化WM8978
    WM8978_HPvol_Set(40, 40); //耳机音量设置
    WM8978_SPKvol_Set(50);    //喇叭音量设置
    exfuns_init();            //为fatfs相关变量申请内存
    f_mount(fs[0], "0:", 1);  //挂载SD卡
    f_mount(fs[1], "1:", 1);  //挂载FLASH.
    piclib_init();            //初始化画图
    key = KEY_Scan(0);

    if (key == KEY0_PRES) //强制校准
    {
        LCD_Clear(WHITE); //清屏
        TP_Adjust();      //屏幕校准
        TP_Save_Adjdata();
        LCD_Clear(WHITE); //清屏
    }
    fontok = font_init();           //检查字库是否OK
    if (fontok || key == KEY1_PRES) //需要更新字库
    {
        LCD_Clear(WHITE);  //清屏
        POINT_COLOR = RED; //设置字体为红色
        LCD_ShowString(60, 50, 200, 16, 16, "ALIENTEK STM32");
        while (SD_Init()) //检测SD卡
        {
            LCD_ShowString(60, 70, 200, 16, 16, "SD Card Failed!");
            delay_ms(200);
            LCD_Fill(60, 70, 200 + 60, 70 + 16, WHITE);
            delay_ms(200);
        }
        LCD_ShowString(60, 70, 200, 16, 16, "SD Card OK");
        LCD_ShowString(60, 90, 200, 16, 16, "Font Updating...");
        key = update_font(20, 110, 16, "0:"); //从SD卡更新
        while (key)                           //更新失败
        {
            LCD_ShowString(60, 110, 200, 16, 16, "Font Update Failed!");
            delay_ms(200);
            LCD_Fill(20, 110, 200 + 20, 110 + 16, WHITE);
            delay_ms(200);
        }
        LCD_ShowString(60, 110, 200, 16, 16, "Font Update Success!");
        delay_ms(1500);
        LCD_Clear(WHITE); //清屏
    }
    // atk_8266_test();		//进入ATK_ESP8266测试
    // test_write_file();

    while (atk_8266_send_cmd("AT", "OK", 20)) //检查WIFI模块是否在线
    {
        atk_8266_quit_trans();                        //退出透传
        atk_8266_send_cmd("AT+CIPMODE=0", "OK", 200); //关闭透传模式
        Show_Str(40, 55, 200, 16, "未检测到模块!!!", 16, 0);
        delay_ms(800);
        LCD_Fill(40, 55, 200, 55 + 16, WHITE);
        Show_Str(40, 55, 200, 16, "尝试连接模块...", 16, 0);
    }
    LCD_Clear(WHITE); //清屏

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
    Lsens_Init(); //初始化光敏传感器
    POINT_COLOR = RED;
    LCD_ShowString(30, 50, 200, 16, 16, "Explorer STM32F4");
    LCD_ShowString(30, 70, 200, 16, 16, "LSENS TEST");
    LCD_ShowString(30, 90, 200, 16, 16, "ATOM@ALIENTEK");
    LCD_ShowString(30, 110, 200, 16, 16, "2014/5/7");
    POINT_COLOR = BLUE; //设置字体为蓝色
    LCD_ShowString(30, 130, 200, 16, 16, "LSENS_VAL:");

    short temp;
    POINT_COLOR = RED;
    POINT_COLOR = BLUE;                                        //设置字体为蓝色
    LCD_ShowString(30, 150, 200, 16, 16, "TEMPERATE: 00.00C"); //先在固定位置显示小数点

    // while (1)
    // {
    //     adcx = Lsens_Get_Val();
    //     LCD_ShowxNum(30 + 10 * 8, 130, adcx, 3, 16, 0); //显示ADC的值
    //     LED0 = !LED0;
    //     temp = Get_Temprate(); //得到温度值
    //     if (temp < 0)
    //     {
    //         temp = -temp;
    //         LCD_ShowString(30 + 10 * 8, 140, 16, 16, 16, "-"); //显示负号
    //     }
    //     else
    //         LCD_ShowString(30 + 10 * 8, 140, 16, 16, 16, " "); //无符号

    //     LCD_ShowxNum(30 + 11 * 8, 150, temp / 100, 2, 16, 0); //显示整数部分
    //     LCD_ShowxNum(30 + 14 * 8, 150, temp % 100, 2, 16, 0); //显示小数部分

    //     printf("Temperature now is: %d\n", temp);
    //     printf("Lightness now is: %d\n", adcx);
    //     printf("\n");

    //     delay_ms(250);
    // }

    POINT_COLOR = RED;
    while (font_init()) //检查字库
    {
        LCD_ShowString(30, 50, 200, 16, 16, "Font Error!");
        delay_ms(200);
        LCD_Fill(30, 50, 240, 66, WHITE); //清除显示
        delay_ms(200);
    }
    POINT_COLOR = RED;
    Show_Str(60, 50, 200, 16, "Explorer STM32F4开发板", 16, 0);
    Show_Str(60, 70, 200, 16, "音乐播放器实验", 16, 0);
    Show_Str(60, 90, 200, 16, "正点原子@ALIENTEK", 16, 0);
    Show_Str(60, 110, 200, 16, "2014年5月24日", 16, 0);
    Show_Str(60, 130, 200, 16, "KEY0:NEXT   KEY2:PREV", 16, 0);
    Show_Str(60, 150, 200, 16, "KEY_UP:PAUSE/PLAY", 16, 0);
    while (1)
    {
        music_player();
    }
}
