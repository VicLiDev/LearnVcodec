demo说明：

抓取程序已经测试过,没有问题,抓到的是YUYV存储格式的YUV文件,采样方式为YUV4:2:2
如果想将YUV文件转化成JPG文件需要使用 YUYV422TORGB.cpp ,这个程序可以转化成功,
但是会产生段错误,没有查找具体原因

程序 YUV2RGB_OpenCV.cpp 应该不是针对 YUYV存储格式的转换,具体没有探究

官方demo使用方法:
```
gcc -o offcial official_demo.c
./official -d /dev/video0 -o >> off.yuv
# 我个人的笔记本使用格式:YUYV:422 640x480  存在一个问题,图片上半部分是上一帧,
# 下半部分是这一帧,后来不知道为什么好了
```

# v4l2

`v4l2-ctl` 是一个强大的命令行工具，用于与 Linux 下的 V4L2（Video4Linux2）设备
进行交互，支持查询、设置摄像头参数、抓帧、录制等操作。适合日常调试和开发。

```
# 安装 v4l-utils 工具
sudo apt install v4l-utils
```

## 查看摄像头设备信息

### 查看连接的摄像头设备
```bash
v4l2-ctl --list-devices
```

> 输出会显示设备名称和 `/dev/video*` 路径。

### 查看设备支持的功能和格式
```bash
v4l2-ctl --device=/dev/video0 --all
```

## 查看和设置视频格式

### 查看支持的视频格式
```bash
v4l2-ctl --device=/dev/video0 --list-formats
```

### 查看支持的分辨率和帧率
```bash
v4l2-ctl --device=/dev/video0 --list-formats-ext
```

### 设置视频格式（如 YUYV 640x480）
```bash
v4l2-ctl --device=/dev/video0 --set-fmt-video=width=640,height=480,pixelformat=NV12
```

### 获取当前格式
```bash
v4l2-ctl --device=/dev/video0 --get-fmt-video
```

## 设置和获取摄像头参数（亮度、对比度等）

### 列出所有可控制参数
```bash
v4l2-ctl --device=/dev/video0 --list-ctrls
```

### 查看某个参数的当前值
```bash
v4l2-ctl --device=/dev/video0 --get-ctrl=brightness
```

### 设置参数值（如设置亮度为128）
```bash
v4l2-ctl --device=/dev/video0 --set-ctrl=brightness=128
```

---

## 采集图像或视频流

### 采集单帧图像（原始格式，一般是 YUYV）
```bash
v4l2-ctl --device=/dev/video0 --stream-mmap=1 --stream-count=1 --stream-to=frame.raw
```

> `--stream-mmap=1` 表示使用 mmap，`--stream-count=1` 表示采一帧。

你可以使用 `ffmpeg` 把 `.raw` 转成 `.jpg`：

```bash
ffmpeg -f rawvideo -pix_fmt yuyv422 -s 640x480 -i frame.raw frame.jpg
```

### 持续采集多帧并保存
```bash
v4l2-ctl --device=/dev/video0 --stream-mmap --stream-count=100 --stream-to=video.raw
```
也可以采集的时候设置相关信息，例如：
```
v4l2-ctl --device=/dev/video0 --set-fmt-video=width=1280,height=720,pixelformat=NV12 --stream-mmap --stream-count=100 --stream-to=video.raw

v4l2-ctl --device=/dev/video0 --set-fmt-video=width=640,height=480,pixelformat=NV12 --stream-mmap --stream-count=100 --stream-to=video.raw
```

## 测试功能

### 视频预览测试（配合 `ffplay`）
```bash
ffplay -f v4l2 -video_size 640x480 -i /dev/video0
也可以直接执行
ffplay /dev/video0
```

### 检查是否支持某个控制
```bash
v4l2-ctl --device=/dev/video0 --get-ctrl=focus_auto
```

## 实用技巧

### 设置自动对焦关闭 + 设置焦距
```bash
v4l2-ctl --set-ctrl=focus_auto=0
v4l2-ctl --set-ctrl=focus_absolute=20
```

### 设置曝光
```bash
v4l2-ctl --set-ctrl=exposure_auto=1          # 1: Manual
v4l2-ctl --set-ctrl=exposure_absolute=200    # 手动曝光值
```

## 总结表（速查）

| 操作       | 命令示例                                                 |
| ---------- | -------------------------------------------------------- |
| 列设备     | `v4l2-ctl --list-devices`                                |
| 查看格式   | `v4l2-ctl --list-formats-ext`                            |
| 设置格式   | `--set-fmt-video=width=640,height=480,pixelformat=YUYV`  |
| 列控制项   | `--list-ctrls`                                           |
| 设置控制项 | `--set-ctrl=brightness=128`                              |
| 采集图像   | `--stream-mmap=1 --stream-count=1 --stream-to=frame.raw` |

