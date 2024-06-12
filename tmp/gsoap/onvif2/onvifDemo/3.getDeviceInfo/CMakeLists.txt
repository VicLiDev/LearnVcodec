# cmake needs this line
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Define project name
PROJECT(onvifDemo)

INCLUDE_DIRECTORIES(comm onvif)

SET(LIBS jrtp
	avcodec pthread m lzma z mp3lame m theoraenc theoradec ogg vorbis vorbisenc x264 xvidcore swresample m avutil
	avdevice m xcb xcb-shm xcb-shape xcb-xfixes avfilter swscale postproc avformat avcodec lzma mp3lame theoraenc theoradec ogg vorbis vorbisenc x264 xvidcore swresample avutil
	avfilter swscale postproc avformat avcodec lzma mp3lame theoraenc theoradec ogg vorbis vorbisenc xvidcore swresample avutil
	avformat avcodec lzma mp3lame theoraenc theoradec ogg vorbis vorbisenc xvidcore swresample avutil
	avutil
	ssl crypto
	)

# Add subdirectory
ADD_SUBDIRECTORY(onvif)
ADD_SUBDIRECTORY(comm)
ADD_SUBDIRECTORY(discovery)
ADD_SUBDIRECTORY(deviceinfo)
