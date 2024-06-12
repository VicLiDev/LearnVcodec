gcc main.c -I ../comm/ -I ../onvif/ ../onvif/duration.c  ../onvif/soapC.c ../onvif/soapClient.c ../onvif/stdsoap2.c ../onvif/wsaapi.c ../comm/onvif_dump.c ../comm/onvif_comm.c ../onvif/wsseapi.c ../onvif/smdevp.c ../onvif/mecevp.c  ../onvif/threads.c ../onvif/dom.c  $(pkg-config --libs --cflags libavcodec libavdevice libavfilter libavformat libavutil openssl)

# 这里编译遇到了问题：
# 1. 需要将onvif_dump.c文件中的 va_start 函数行和 va_end 函数行注释掉，不清楚会不会带来不好的影响
# 2. 在 stdsoap2.h 文件中加入 #define WITH_OPENSSL
