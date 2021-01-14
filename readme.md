# 基于WiFi的智能硬件实现

插入SD卡，MUSIC文件夹放置WAV文件，PICTURE文件夹放置jpg文件，通过文件顺序匹配专辑封面

在app.c中，配置主机与从机
```c
#include "app.h"

char *peer_ip;
int DEVICE_ID = 1;  // 1表示主机，2表示从机
int run_greet = 0;  // 0表示不建立WIFI连接（WIFI模块仍需插入），1表示建立WIFI连接
```

处于音乐播放状态时，按下KEY1发送文件，按下WAKE_UP发送温度与光敏数据。主机先按下，从机后按下。
