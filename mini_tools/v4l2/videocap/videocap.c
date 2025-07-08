#include <stdio.h>    
#include <stdlib.h>    
#include <string.h>    
#include <assert.h>    
#include <getopt.h>             /* getopt_long() */    
#include <fcntl.h>              /* low-level i/o */    
#include <unistd.h>    
#include <errno.h>    
#include <malloc.h>    
#include <sys/stat.h>    
#include <sys/types.h>    
#include <sys/time.h>    
#include <sys/mman.h>    
#include <sys/ioctl.h>    
#include <asm/types.h>          /* for videodev2.h */    
#include <linux/videodev2.h>

#define CODEC_NODE  "/dev/video0"
#define LCD_WIDTH  320 
#define LCD_HEIGHT 240 
#define YUV_FRAME_BUFFER_SIZE (LCD_WIDTH*LCD_HEIGHT)+(LCD_WIDTH*LCD_HEIGHT)/2  /* YCBCR 420 */

static int cam_c_init(void);
static void exit_from_app() ;
static int cam_c_fp = -1;

static void exit_from_app() 
{
  close(cam_c_fp);
 
}

static int cam_c_init(void)  
{
 int dev_fp = -1;

 dev_fp = open(CODEC_NODE, O_RDWR);

 if (dev_fp < 0) {
  perror(CODEC_NODE);
  printf("CODEC : Open Failed \n");
  return -1;
 }
 return dev_fp;
}


int main()

{  
 
   
     struct v4l2_capability cap;
     struct v4l2_format codec_fmt;
/* Camera codec initialization */
 if ((cam_c_fp = cam_c_init()) < 0)
  exit_from_app();
/* Codec set */
 /* Get capability */
 int ret = ioctl(cam_c_fp , VIDIOC_QUERYCAP, &cap);
 if (ret < 0) {
  printf("V4L2 : ioctl on VIDIOC_QUERYCAP failled\n");
  exit(1);
 }
 //printf("V4L2 : Name of the interface is %s\n", cap.driver);


 /* Check the type - preview(OVERLAY) */
 if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
  printf("V4L2 : Can not capture(V4L2_CAP_VIDEO_CAPTURE is false)\n");
  exit(1);
 }

 /* Set format */
 codec_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 codec_fmt.fmt.pix.width = LCD_WIDTH; 
 codec_fmt.fmt.pix.height = LCD_HEIGHT;  
 codec_fmt.fmt.pix.pixelformat=  V4L2_PIX_FMT_YUYV;//V4L2_PIX_FMT_YUV420;(The average captrue equiment can't support. ) 
 ret = ioctl(cam_c_fp , VIDIOC_S_FMT, &codec_fmt);
 if (ret < 0) {
  printf("V4L2 : ioctl on VIDIOC_S_FMT failled\n");
  exit(1);
 }
        int start=1;
        ret = ioctl(cam_c_fp, VIDIOC_STREAMON, &start);
 if (ret < 0) {
  printf("V4L2 : ioctl on VIDIOC_STREAMON failed\n");
  exit(1);
 }

        unsigned char yuv_buf[YUV_FRAME_BUFFER_SIZE];
        char YUV_name[100];
        static FILE *YUV_fp;
        sprintf(&YUV_name[0], "Cam%dx%d-%d.yuv", LCD_WIDTH, LCD_HEIGHT, 888);
        YUV_fp = fopen(&YUV_name[0], "wb");
   if (!YUV_fp) {
    perror(&YUV_name[0]);
   }

      


        int i=0;
        int framecount=200;
           for(i=0;i<framecount;i++)
         {
             /* read YUV frame from camera device */
  if (read(cam_c_fp, yuv_buf, YUV_FRAME_BUFFER_SIZE) < 0) 
               {
   perror("read()");
  }
               
              //write down YUV 
             fwrite(yuv_buf, 1, YUV_FRAME_BUFFER_SIZE, YUV_fp);

                 
          }///for

     

      
        ret = ioctl(cam_c_fp, VIDIOC_STREAMOFF, &start);
 if (ret < 0)
         {
  printf("V4L2 : ioctl on VIDIOC_STREAMOFF failed\n");
  exit(1);
  }
      
        fclose(YUV_fp);
       
        exit_from_app();
  
}
