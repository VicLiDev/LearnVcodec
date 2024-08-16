# 简介

RTP、GSOAP、ONVIF、Live555、RTMP、RTSP和Webcam之间存在一定的关系和联系，它们各自
在流媒体传输、控制、开发和应用中扮演着不同的角色。以下是对它们之间关系的详细介绍：

1. RTP（Real-time Transport Protocol）
* 定义：RTP是一种网络传输协议，用于在互联网上传输具有实时属性的数据，如音频和视频。
        它详细说明了在网络中传输音频和视频的标准数据包格式。
* 作用：RTP主要负责多媒体数据的实时传输，通常与RTCP（实时传输控制协议）一起使用，
        以确保传输过程中的质量控制和反馈。
* 关系：RTP是流媒体传输的基础协议之一，与其他协议（如RTSP、RTMP）配合使用，以提供
        完整的流媒体服务。

2. GSOAP
* 定义：GSOAP是一个C/C++的SOAP/XML开发工具包，它提供了SOAP/XML协议的实现，使得
        C/C++语言开发Web服务或客户端程序变得简单。
* 作用：GSOAP主要用于Web服务的开发，支持SOAP 1.1/1.2等标准，并提供了丰富的数据传输
        和安全性支持。
* 关系：虽然GSOAP本身不直接参与流媒体传输，但它可以作为ONVIF等协议实现的一部分，
        用于Web服务的SOAP通信。

3. ONVIF（Open Network Video Interface Forum）
* 定义：ONVIF是一个全球性的开放行业论坛，致力于推广基于IP的网络视频产品的标准化接口。
* 作用：ONVIF制定了一系列的标准和规范，以实现不同品牌、不同制造商的网络视频设备
        之间的互操作性。
* 关系：ONVIF标准中使用了多种协议，包括RTP、RTSP等，用于实现网络视频设备的实时数据
        传输和控制。同时，ONVIF的Web服务部分也可能使用GSOAP等工具进行开发。

4. Live555
* 定义：Live555是一组开源的多媒体流媒体库，用C++编写，主要用于实现实时媒体流的传输。
* 作用：Live555支持多种流媒体协议，包括RTP/RTCP、RTSP等，提供了从各种源接收和播放
        多媒体内容的能力，也支持将内容推送到服务器。
* 关系：Live555是流媒体传输中常用的工具库之一，它实现了RTP、RTCP和RTSP等协议的具体
        功能，使得开发者能够轻松构建流媒体应用。

5. RTMP（Real-time Messaging Protocol）
* 定义：RTMP是一种用于实时数据传输的协议，通常用于在互联网上传输音频、视频和数据流。
* 作用：RTMP提供了低延迟的实时数据传输能力，广泛应用于直播、视频会议等领域。
* 关系：RTMP与RTP在功能上有所重叠，但RTMP更多地关注于实时通信的完整性和可靠性，而
        RTP则更侧重于数据的实时传输。RTMP在某些应用场景下可能替代RTP使用。

6. RTSP（Real Time Streaming Protocol）
* 定义：RTSP是一种网络控制协议，用于控制实时数据流的传输。
* 作用：RTSP定义了如何建立和控制媒体会话，包括播放、暂停、停止等操作。
* 关系：RTSP与RTP/RTCP配合使用，RTP负责数据的传输，RTCP负责传输过程中的质量控制和
        反馈，而RTSP则负责控制这些传输过程。

7. Webcam
* 定义：Webcam是网络摄像头的简称，是一种可以连接互联网并传输视频数据的设备。
* 作用：Webcam广泛应用于视频通话、远程监控、在线直播等领域。
* 关系：Webcam作为流媒体数据的采集端，可以通过RTP/RTSP等协议将视频数据传输到流媒体
        服务器或客户端。同时，流媒体服务器或客户端也可以使用Live555等工具库来处理
        这些传输过来的数据。

# 流媒体服务器搭建

## mediamtx

### 环境部署

* 下载:
```
git clone https://github.com/bluenviron/mediamtx
```

* 编译：
```
git clone https://github.com/bluenviron/mediamtx
cd mediamtx
# ./... 是 Go 命令中的一个模式，表示当前目录及其所有子目录中的包
go generate ./...
CGO_ENABLED=0 go build .

```

编译过程中遇到缺少go包的问题： `package crypto/ecdh is not in GOROOT`，可以通过
更新go语言环境解决：
```
查看当前go版本
go version

安装gvm管理go
bash < <(curl -s -S -L https://raw.githubusercontent.com/moovweb/gvm/master/binscripts/gvm-installer)

安装新的go版本，当前问题需要go版本大于1.2，可以去官网（https://go.dev/dl/）找一个
最新的稳定版本
gvm install go1.23

列出已安装的Go版本
gvm list

切换Go版本
gvm use go<版本号>

设置默认Go版本
gvm use go<版本号> --default

卸载Go版本
gvm uninstall go<版本号>

注意，安装gvm后，可能会增加系统负载，导致在切换目录时卡顿，可以通过注释掉bashrc/zshrc
中的初始化命令解决
# [[ -s "/home/lhj/.gvm/scripts/gvm" ]] && source "/home/lhj/.gvm/scripts/gvm"
```
The command will produce the mediamtx binary.

* 运行：
```
./mediamtx

如果有端口冲突，可以在mediamtx.yml中修改端口，推流和拉流的端口是一样的
```

### 推流/拉流

mystream名字可以随便取，但是要保证推流端和拉流端一致

推流：
```
ffmpeg -re -stream_loop -1 -i ./Big_Buck_Bunny_1080_10s_1MB.mp4 -c copy -f rtsp rtsp://localhost:8556/mystream

-re: 读取输入文件时，按照文件的原始帧率读取。这对于实时流或模拟实时流很有用。
-stream_loop -1: 循环播放输入文件，-stream_loop 不是官方标准选项。可以考虑使用
     -loop 1 或类似选项，但 FFmpeg 的 -loop 选项通常用于处理图像序列或 GIF 文件，
     并不直接适用于视频文件。对于视频文件，通常通过外部脚本或 FFmpeg 的其他功能
     （如拼接多个文件）来实现循环。
-i file.ts: 指定输入文件为 file.ts。
-c copy: 不重新编码视频和音频流，直接复制。
-f rtsp: 指定输出格式为 RTSP。
rtsp://localhost:8554/mystream: RTSP 流的地址和端口。
```

拉流
```
vlc播放网络流： rtsp://localhost:8556/mystream

ffplay rtsp://192.168.7.233:8556/mystream
ffplay rtmp://192.168.7.233:1935/mystream
```

### RtspSimpleServer

* 下载:
```
git clone https://github.com/bhaney/rtsp-simple-server.git
```

rtsp-simple-server 是从 mediamtx 中 fork出来的，后续的编译、部署、使用都可以参考
mediamtx

## rtsp-test-server

https://snapcraft.io/install/rtsp-test-server/ubuntu

* Based on GStreamer RTSP Server.

* Config file location: /var/snap/rtsp-test-server/common/rtsp-test-server.conf

* Available urls:
```
rtsp://localhost:8554/bars
rtsp://localhost:8554/white
rtsp://localhost:8554/black
rtsp://localhost:8554/red
rtsp://localhost:8554/green
rtsp://localhost:8554/blue
rtsp://localhost:8554/bars-vp8
rtsp://localhost:8554/white-vp8
rtsp://localhost:8554/black-vp8
rtsp://localhost:8554/red-vp8
rtsp://localhost:8554/green-vp8
rtsp://localhost:8554/blue-vp8
```

* install: `sudo snap install rtsp-test-server`


## SRS（Simple RTMP Server）

## Live555

* 官网: http://www.live555.com/

* 下载bin:`wget http://www.live555.com/mediaServer/linux/live555MediaServer`

* 下载source:`wget http://www.live555.com/liveMedia/public/live555-latest.tar.gz`

* 编译：
```
./genMakefiles linux-64bit

make -j


遇到问题：
error: ‘struct std::atomic_flag’ has no member named ‘test’

解决方法：
修改 config.linux-64bit
在第一行末尾添加 -DNO_STD_LIB，添加完后为：
COMPILE_OPTS =		$(INCLUDES) -m64  -fPIC -I/usr/local/include -I. -O2 -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -DNO_STD_LIB
然后重新生成makefie，make即可
```

* 启动：`cd mediaServer && ./live555MediaServer`可以看到支持的格式


## GStreamer RTSP Server
