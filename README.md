### 简介
A simple player demo,supported to cache file,download progress
![Image text](https://github.com/what951006/YMediaPlayer/blob/dev/README/pic_0.png)

### 一、开发环境
vs2015+Qt5
### 二、编译方式
#### window
运行 \Platform\win32\YMediaCore\YMediaCore.sln \
包含test项目和YMediaCore项目
#### linux
正在开发中...
#### ios
正在开发中...

### 未解决的问题
1、播放rtmp如果源视频网络卡，即卡在获取流信息的地方，可能导致程序卡住,使用AVIContext替换原来的读流方式

