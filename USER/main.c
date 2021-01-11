#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "lcd.h"
#include "adc.h"
#include "key.h"
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
#include "timer.h"

//ALIENTEK 探索者STM32F407开发板 实验19
//内部温度传感器实验 -库函数版本
//技术支持：www.openedv.com
//淘宝店铺：http://eboard.taobao.com
//广州市星翼电子科技有限公司
//作者：正点原子 @ALIENTEK


int main(void)
{
    u8 key, fontok = 0;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置系统中断优先级分组2
    delay_init(168);                                //初始化延时函数
    uart_init(115200);                              //初始化串口波特率为115200
    usart3_init(115200);                            //初始化串口3波特率为115200
    LED_Init();                                     //初始化LED
    LCD_Init();                                     //液晶初始化
    Adc_Init();                                     //内部温度传感器ADC初始化
    KEY_Init();                                     //按键初始化
    W25QXX_Init();                                  //初始化W25Q128
    tp_dev.init();                                  //初始化触摸屏
    usmart_dev.init(168);                           //初始化USMART
    my_mem_init(SRAMIN);                            //初始化内部内存池
    my_mem_init(SRAMCCM);                           //初始化CCM内存池

    W25QXX_Init();            //初始化W25Q128
    WM8978_Init();            //初始化WM8978
    WM8978_HPvol_Set(40, 40); //耳机音量设置
    WM8978_SPKvol_Set(1);     //喇叭音量设置
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

    int run_greet = 1, greet_cnt = 0, packet_interval = 2000;
    char peer_ip[20], res[50], cmd_tmp[100], greeting_res[50], greet_text[30];

    // Start the server and client and send greetings
    if (get_device_id() == 1 && run_greet)
    {
        send_command_with_retry("AT+CWMODE=2", 200, 3, 1, NULL);
        send_command_with_retry("AT+RST", 200, 3, 1, NULL);
        delay_ms(5000);
        send_command_with_retry("AT+CIPMUX=1", 200, 3, 1, NULL);
        send_command_with_retry("AT+CIPSERVER=1,8086", 200, 3, 1, NULL);
        send_command_with_retry("AT+CWSAP=\"ATK WIFI\",\"12345678\",1,4", 200, 3, 1, NULL);

        // Wait for the peer to join
        while (1)
        {
            send_command_util_success("AT+CWLIF", 200, 1, res);
            extract_peer_ip(res, peer_ip);
            if (peer_ip[3] == '.' && peer_ip[7] == '.')
                break;
            delay_ms(1000);
        }
        printf("The peer ip is %s\n", peer_ip);
        set_peer_ip(peer_ip);
        // Wait for the peer to join
        delay_ms(5000);

        int connection_established = 0;
        while (1) {

            greeting_res[0] = 0;
            int _res;

            if (!connection_established) {
                sprintf(cmd_tmp, "AT+CIPSTART=0,\"TCP\",\"%s\",8086,1000", peer_ip);
                _res = send_command_with_retry(cmd_tmp, 400, 5, 1, NULL);
                if (!_res) continue;
                else connection_established = 1;
            }

            while (1) {
                _res = send_command_with_retry("AT+CIPSEND=0,9", 200, 3, 1, NULL);
                if (!_res) continue;
                delay_ms(100);
                sprintf(greet_text, "Greet: %02d", greet_cnt);
                _res = send_command_with_retry(greet_text, 2000, 1, 1, NULL);
                if (_res) break;
            }

            printf("Greeting sent\n");
            greet_cnt++;

            wait_for_data(1, res);
            printf("Greeting from %s: %s\n", peer_ip, res);
            delay_ms(packet_interval);
        }
        
    }
    else if (get_device_id() == 2 && run_greet)
    {
        send_command_with_retry("AT+CWMODE=1", 200, 3, 1, NULL);
        send_command_with_retry("AT+RST", 200, 3, 1, NULL);
        delay_ms(5000);
        send_command_with_retry("AT+CIPMUX=1", 200, 3, 1, NULL);
        send_command_with_retry("AT+CIPSERVER=1,8086", 200, 3, 1, NULL);
        send_command_with_retry("AT+CIPSTO=1200", 200, 3, 1, NULL);
        send_command_util_success("AT+CWJAP=\"ATK WIFI\",\"12345678\"", 2000, 1, NULL);

        while (1) {
            wait_for_data(1, res);
            printf("Greeting from %s: %s\n", peer_ip, res);
            delay_ms(packet_interval);
            int _res;
            while (1) {
                _res = send_command_with_retry("AT+CIPSEND=0,9", 200, 3, 1, NULL);
                if (!_res) continue;
                sprintf(greet_text, "Greet: %02d", greet_cnt);
                _res = send_command_with_retry(greet_text, 2000, 1, 1, NULL);
                if (_res) break;
            }
            
            printf("Greeting sent\n");
            greet_cnt++;
        }
    }
    LCD_Clear(WHITE); //清屏

    u8 adcx;
    Lsens_Init(); //初始化光敏传感器
    short temp;

    POINT_COLOR = RED;
    while (font_init()) //检查字库
    {
        LCD_ShowString(30, 50, 200, 16, 16, "Font Error!");
        delay_ms(200);
        LCD_Fill(30, 50, 240, 66, WHITE); //清除显示
        delay_ms(200);
    }
    ui_show();
    TIM3_Int_Init(5000 - 1, 8400 - 1);
    while (1)
    {
        music_player();
    }
}
