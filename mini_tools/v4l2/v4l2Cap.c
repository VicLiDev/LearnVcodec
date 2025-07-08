//一个不错的文档http://wentao1213.com/2016/11/25/linux-v4l2-usb/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>

struct buffer {
        void *start;  
        size_t length;  
};  
   
struct buffer *buffers;  
unsigned long  n_buffers;  
unsigned long file_length;  
  
int file_fd;  
char *dev_name = "/dev/video0";   //这是我摄像头节点，根据自己的节点填写  
int fd;  
  
static int read_frame (void)  
{  
     struct v4l2_buffer buf;  
       
     /*帧出列*/  
     buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
     buf.memory = V4L2_MEMORY_MMAP;  
     ioctl (fd, VIDIOC_DQBUF, &buf);  //从驱动程序的输出缓冲区序列取出已填充（捕获）或显示（输出）缓冲区。
  
     write(file_fd,buffers[buf.index].start,buffers[buf.index].length);  
       
     /*buf入列*/  
     ioctl(fd, VIDIOC_QBUF, &buf);  
  
     return 0;  
}  
   
int main (int argc,char ** argv)  
{
	int ret;
	struct v4l2_capability cap;  
    struct v4l2_format fmt;  
    struct v4l2_requestbuffers req;  //在申请之前设置，用于存放申请信息，在申请时使用
    struct v4l2_buffer buf;          //在申请之后使用，用于存放查询得到的缓冲区信息
    unsigned int i;  
    enum v4l2_buf_type type; 

    file_fd = open("test.yuv", O_RDWR | O_CREAT, 0777); //用来保存图片  

    /* 1.打开摄像头*/
    fd = open (dev_name, O_RDWR | O_NONBLOCK);  //打开摄像头
    if (-1 == fd){
    	fprintf(stderr, "Cannot open '%s': %d, %s\n",
    		dev_name, errno, strerror(errno));
    	exit(EXIT_FAILURE);
    }

    /* 2.获取驱动信息*/
    ioctl (fd, VIDIOC_QUERYCAP, &cap);   //驱动信息保存在cap结构体  
    printf("Driver Name:%s\nCard Name:%s\nBus info:%s\nDevice Capalibilities Flags:%x\n\n",cap.driver,cap.card,cap.bus_info,cap.capabilities);  

    /* 3.1.设置视频的制式(PAL/NTSC)*/
    /* 3.2.设置视频图像的采集窗口大小*/
    /* 3.3.设置视频帧格式,包括帧的点阵格式,宽度和高度等*/
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;   //这是设置传输流类型  
    fmt.fmt.pix.width       = 1440;  //设置分辨率  
    fmt.fmt.pix.height      = 900;  
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;  
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG; //图像格式，此处是jpg //V4L2_PIX_FMT_MJPEG //V4L2_PIX_FMT_JPEG //V4L2_PIX_FMT_CPIA1

	printf("fmtinfo_before:%d\n",fmt.fmt.pix.pixelformat);

	ioctl(fd, VIDIOC_S_FMT, &fmt);

	printf("fmtinfo_after:%d\n",fmt.fmt.pix.pixelformat);

	/* 3.4.设置视频的帧率*/
	/* 3.5.设置视频的旋转方式*/

    /* 4.申请图像缓冲区*/
     req.count               = 4;  
     req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
     req.memory              = V4L2_MEMORY_MMAP;  
     ioctl (fd, VIDIOC_REQBUFS, &req);  //在此步申请缓存区，用于存放图像，申请信息存放在req中
     
     buffers = calloc (req.count, sizeof (*buffers));  //此处申请的缓存区是用于存放缓存区信息，而不是用于存放图像

     /* 5.把内核空间中的图像缓冲区映射到用户空间*/
     for (n_buffers = 0; n_buffers < req.count; ++n_buffers){   
           /*获取图像缓冲区的信息*/
           buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
           buf.memory      = V4L2_MEMORY_MMAP;  
           buf.index       = n_buffers;  
   
           ioctl (fd, VIDIOC_QUERYBUF, &buf);   //查询缓存区的相关信息
               
           buffers[n_buffers].length = buf.length;   
             
           // 把内核空间中的图像缓冲区映射到用户空间
           buffers[n_buffers].start = mmap (NULL ,    //通过mmap建立映射关系  
                                        buf.length,  
                                        PROT_READ | PROT_WRITE ,  
                                        MAP_SHARED ,  
                                        fd,  
                                        buf.m.offset);  
     }  
     
     /* 6.图像缓冲入队*/
     for (i = 0; i < n_buffers; ++i){  
             buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
             buf.memory      = V4L2_MEMORY_MMAP;  
             buf.index       = i;   
             ioctl (fd, VIDIOC_QBUF, &buf);  //將空（待捕获）或填充（待输出）缓冲区排入驱动的传入序列
               
     }  
      
    /* 7.开始捕捉图像数据*/
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    if(ioctl(fd, VIDIOC_STREAMON, &type) < 0){  //VIDIOC_STREAMON和VIDIOC_STREAMOFF在流式传输（内存映射或用户指针）I/O期间启动和停止捕获或输出进程
    	perror("Start cap error");
    }

    //select()机制中提供一fd_set的数据结构(所需头文件：sys/time.h)，实际上是一long类型的数组，每一个数组元素都能与一打开的文件句柄（不管是socket句柄，
    //还是其他文件或命名管道或设备句柄）建立联系，建立联系的工作由程序员完成，当调用select()时，由内核根据IO状态修改fd_set的内容，
    //由此来通知执行了select()的进程哪一socket或文件发生了可读或可写事件。
    fd_set fds;

    //和select模型紧密结合的四个宏，含义不解释了：
    //FD_ZERO(fd_set *fd); /* 清空该组文件描述符集合 */
    //FD_CLR(inr fd,fd_set *fd); /* 清除该组文件描述符集合中的指定文件描述符 */
    //FD_ISSET(int fd,fd_set *fd); /* 测试指定的文件描述符是否在该文件描述符集合中 */
    //FD_SET(int fd,fd_set *fd); /* 向该文件描述符集合中添加文件描述符 */
    FD_ZERO(&fds);  
    FD_SET(fd, &fds);  

    /*定义：
      int select(int fd_max, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
      fd_max：传入的监视文件描述符集合中最大的文件描述符数值 + 1，因为select是从0开始一直遍历到数值最大的标识符。
      *readfds：文件描述符集合，检查该组文件描述符的可读性。
      *writefds：文件描述符集合，检查该组文件描述符的可写性。
      *exceptfds：文件描述符集合，检查该组文件描述符的异常条件。
      *timeout： 时间结构体
     */
    /*
      *timeout的值为NULL，则将select()函数置为阻塞状态，当监视的文件描述符集合中的某一个描述符发生变化才会返回结果并向下执行。
      *timeout的值等于0，则将select()函数置为非阻塞状态，执行select()后立即返回，无论文件描述符是否发生变化。
      *timeout的值大于0，则将select()函数的超时时间设为这个值，在超时时间内阻塞，超时后返回结果。
     */
    /*返回值：
      -1：发生错误，并将所有描述符集合清0，可通过errno输出错误详情。
      0：超时。
      正数：发生变化的文件描述符数量。
     */
    //!!注意：每次调用完select()函数后需要将文件描述符集合清空并重新设置，也就是设置的文件描述符集合是一次性使用的。原因是调用完select()后文件描述符集合可能发生改变
    ret = select(fd + 1, &fds, NULL, NULL, NULL);
    if (-1 == ret){
    	if (EINTR == errno)
    		perror("select");
    }
    if (0 == ret) {
    	fprintf(stderr, "select timeout\n");
    	exit(EXIT_FAILURE);
    }
     
    /* 8.读取一幅图像(帧出列-帧入列)*/
    if(read_frame() != 0)
    	printf("read frame error\n");

    /* 9.停止捕捉图像数据*/
    if(ioctl(fd, VIDIOC_STREAMOFF, &type) < 0){  //VIDIOC_STREAMON和VIDIOC_STREAMOFF在流式传输（内存映射或用户指针）I/O期间启动和停止捕获或输出进程
    	perror("Start cap error");
    }
    /* 10.停止视频采集(取消内存映射)*/
    for (i = 0; i < n_buffers; ++i)  
       munmap (buffers[i].start, buffers[i].length);     
  
  
    close (fd);  
    close (file_fd);  
    printf("Camera Done.\n");  
  
    return 0;  
}  
