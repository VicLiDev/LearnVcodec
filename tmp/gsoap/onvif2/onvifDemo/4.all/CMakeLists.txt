# cmake needs this line
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Define project name
PROJECT(onvifDemo)

INCLUDE_DIRECTORIES(comm onvif)

SET(LIBS 
	avdevice m xcb xcb-shm xcb-shape xcb-xfixes asound SDL2 sndio Xv X11 Xext avfilter pthread swscale postproc avformat z avcodec lzma mp3lame theoraenc theoradec ogg vorbis vorbisenc x264 xvidcore swresample avutil 
	ssl crypto)

# Add subdirectory
ADD_SUBDIRECTORY(onvif)
ADD_SUBDIRECTORY(comm)
ADD_SUBDIRECTORY(discovery)
ADD_SUBDIRECTORY(deviceinfo)
ADD_SUBDIRECTORY(capabilities)
ADD_SUBDIRECTORY(systemtime)
ADD_SUBDIRECTORY(SetResolution)
ADD_SUBDIRECTORY(snapshot)
ADD_SUBDIRECTORY(avstream)
