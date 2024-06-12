#include <sys/ioctl.h>
#include<stdio.h> 
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/videodev2.h> 
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <SDL2/SDL.h>

#define LOAD_BGRA    0  
#define LOAD_RGB24   0  
#define LOAD_BGR24   0  
#define LOAD_YUV422P 1  
  
//Bit per Pixel  
#if LOAD_BGRA  
const int bpp=32;  
#elif LOAD_RGB24|LOAD_BGR24  
const int bpp=24;  
#elif LOAD_YUV422P  
const int bpp=16;  
#endif  
int screen_w=640,screen_h=480;  
const int pixel_w=320,pixel_h=240;  
unsigned char buffer[pixel_w*pixel_h*bpp/8];  
unsigned char buffer_convert[pixel_w*pixel_h*4];  
struct buffers{
	void* start;
	unsigned int length;
}*buffers;
void errmsg(const char* fun_name)
{
	printf("%s error\n",fun_name);
	exit(-1);
}
int main()
{
	int fd = open("/dev/video0",O_RDWR);
	if(fd < 0)
		errmsg("open");
	int ret = -1;
	struct v4l2_capability cap;
	ret = ioctl(fd,VIDIOC_QUERYCAP,&cap);
	if(ret < 0 ){
		perror("ioctl");
		return -1;
	}
	printf("%s \n%s\n%s\n %08x %08x\n",cap.driver,cap.card,cap.bus_info,cap.version,cap.capabilities);
	printf("V4L2_CAP_VIDEO_CAPTURE = %08x    %d \n",V4L2_CAP_VIDEO_CAPTURE,V4L2_CAP_VIDEO_CAPTURE&cap.capabilities);
	
	if(!(V4L2_CAP_VIDEO_CAPTURE&cap.capabilities)){
		errmsg( "capture not support");
		exit(-1);
	}
	//  
    struct v4l2_fmtdesc fmtdesc;
	fmtdesc.index=0; 
	fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	printf("Support format:\n");
	while(ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1){
		printf("%d.%s type=%08x  flags=%08x pixelformat=%08x\n",fmtdesc.index+1,fmtdesc.description,fmtdesc.type,fmtdesc.flags,fmtdesc.pixelformat);
		fmtdesc.index++;
	}
	struct v4l2_requestbuffers req; 
	req.count=4; req.type=V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	req.memory=V4L2_MEMORY_MMAP; 
	ret =  ioctl(fd,VIDIOC_REQBUFS,&req);
	if(ret < 0 ){
		perror("ioctl23");
		exit(-1);
	}
	buffers = (struct buffers*)calloc (req.count, sizeof (*buffers));
	if (!buffers){
		fprintf (stderr, "Out of memory/n");
		exit (-1);
	}
	struct v4l2_buffer buf;
	for (unsigned int n_buffers = 0; n_buffers < req.count; ++n_buffers){
		memset(&buf,0,sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;
		if (-1 == ioctl (fd, VIDIOC_QUERYBUF, &buf)){
			perror("ioctl33");
			exit(-1);
		}
		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start =mmap (NULL,buf.length,PROT_READ | PROT_WRITE ,MAP_SHARED,fd, buf.m.offset);
		if (MAP_FAILED == buffers[n_buffers].start){
			perror("mmap");
			exit(-1);
		}
	}
	unsigned int i;
	enum v4l2_buf_type type;
	for (i = 0; i < 4; ++i){
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		ret  =  ioctl (fd, VIDIOC_QBUF, &buf);
		if(ret < 0){
			perror("ioctl 44");
			exit(-1);
		}
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl (fd, VIDIOC_STREAMON, &type);
	if(ret < 0){
		perror("ioctl 44");
		exit(-1);
	}
	if(SDL_Init(SDL_INIT_VIDEO)){
		printf( "Could not initialize SDL - %s\n", SDL_GetError());   
		return -1;  
	}   
	SDL_Window *screen;   
	//SDL 2.0 Support for multiple windows  
    screen = SDL_CreateWindow("Simplest Video Play SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,  
			screen_w, screen_h,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);  
	if(!screen){
		printf("SDL: could not create window - exiting:%s\n",SDL_GetError());    
		return -1;  
	}  
	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, 0);    
	Uint32 pixformat=0;  
#if LOAD_BGRA  
	//Note: ARGB8888 in "Little Endian" system stores as B|G|R|A  
	pixformat= SDL_PIXELFORMAT_ARGB8888;    
#elif LOAD_RGB24  
	pixformat= SDL_PIXELFORMAT_RGB888;    
#elif LOAD_BGR24  
	pixformat= SDL_PIXELFORMAT_BGR888;    
#elif LOAD_YUV422P  
	//IYUV: Y + U + V  (3 planes)  
	//YV12: Y + V + U  (3 planes)  
	pixformat= SDL_PIXELFORMAT_YUY2; // SDL_PIXELFORMAT_IYUV;    
#endif  
	SDL_Texture* sdlTexture = SDL_CreateTexture(sdlRenderer,pixformat, SDL_TEXTUREACCESS_STREAMING,pixel_w,pixel_h);
	SDL_Rect sdlRect;     
	//struct v4l2_buffer buf; CLEAR (buf);
	while(1){
		memset(&buf,0,sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		ioctl (fd, VIDIOC_DQBUF, &buf); 
		if(ret < 0){
			perror("ioctl 56");
			exit(-1);
		}
		//process_image (buffers[buf.index.]start); //
		printf("%d\n",buffers[buf.index].length);
#if LOAD_BGRA  
		//We don't need to change Endian  
		//Because input BGRA pixel data(B|G|R|A) is same as ARGB8888 in Little Endian (B|G|R|A)  
		SDL_UpdateTexture( sdlTexture, NULL, buffer, pixel_w*4);    
#elif LOAD_RGB24|LOAD_BGR24  
		//change 24bit to 32 bit  
		//and in Windows we need to change Endian  
		CONVERT_24to32(buffer,buffer_convert,pixel_w,pixel_h);  
		SDL_UpdateTexture( sdlTexture, NULL, buffer_convert, pixel_w*4);    
#elif LOAD_YUV422P  
		SDL_UpdateTexture( sdlTexture, NULL, buffers[buf.index].start, pixel_w*2);    
#endif  
		//FIX: If window is resize  
		sdlRect.x = 0;    
		sdlRect.y = 0;    
		sdlRect.w = screen_w;    
		sdlRect.h = screen_h;    
		SDL_RenderClear( sdlRenderer );     
		SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, &sdlRect);    
		SDL_RenderPresent( sdlRenderer );    
		//Delay 40ms  
		SDL_Delay(40);  
		ret = ioctl (fd,VIDIOC_QBUF,&buf); 
		if(ret < 0){
			perror("ioctl 67");
			exit(-1);
		}
	}
	close(fd);
	return  0 ;
}
