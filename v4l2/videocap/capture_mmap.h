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
#define CLEAR(x) memset (&(x), 0, sizeof (x))    
typedef   enum  {   
    IO_METHOD_READ, IO_METHOD_MMAP, IO_METHOD_USERPTR,   
} io_method;   
struct  buffer {   
    void  * start;   
    size_t  length; //buffer's length is different from cap_image_size    
};   
static   char  * dev_name = NULL;   
static  io_method io = IO_METHOD_MMAP; //IO_METHOD_READ;//IO_METHOD_MMAP;    
static   int  fd = -1;   
struct  buffer * buffers = NULL;   
static  unsigned  int  n_buffers = 0;   
static   FILE  * outf = 0;   
static  unsigned  int  cap_image_size = 0; //to keep the real image size!! 

static   void  open_device( void );
static   void  init_device( void );
static   void  init_mmap( void ) ;
static   void  start_capturing( void );
static   void  mainloop( void );
static   int  read_frame( void );
static   void  process_image( const   void  * p,  int  len);
static   void  errno_exit( const   char  * s);
static   int  xioctl( int  fd,  int  request,  void  * arg);
static   void  stop_capturing( void );
static   void  uninit_device( void );
static   void  close_device( void );