//程序来源:https://blog.csdn.net/gongluck93/article/details/79126801
#include "librtmp/rtmp_sys.h"
#include "librtmp/log.h"

#pragma comment(lib, "librtmp.lib")
#pragma comment(lib, "WS2_32.lib")

WORD version;
WSADATA wsaData;
version = MAKEWORD(1, 1);
WSAStartup(version, &wsaData);

int res = 0;
RTMP* rtmp = RTMP_Alloc();
RTMP_Init(rtmp);

res = RTMP_SetupURL(rtmp, "rtmp://192.168.34.40/live/test");//推流
PRINTERROR(res, 1, "RTMP_SetupURL error.\n");
//if unable,the AMF command would be 'play' instead of 'publish'
RTMP_EnableWrite(rtmp);//推流要设置写
res = RTMP_Connect(rtmp, NULL);
PRINTERROR(res, 1, "RTMP_Connect error.\n");
res = RTMP_ConnectStream(rtmp,0);
PRINTERROR(res, 1, "RTMP_ConnectStream error.\n");

//推流
FILE *fp_push=fopen("save.flv","rb");
FlvHeader flvheader;
fread(&flvheader, sizeof(flvheader), 1, fp_push);
int32_t preTagLen = 0;//前一个Tag长度
fread(&preTagLen, 4, 1, fp_push);
TagHeader tagHeader;
uint32_t begintime=RTMP_GetTime(),nowtime,pretimetamp = 0;
while (true) {
	fread(&tagHeader, sizeof(tagHeader), 1, fp_push);
	if(tagHeader.type != 0x09) {
		int num = FINT24TOINT(tagHeader.datalen);
		fseek(fp_push, FINT24TOINT(tagHeader.datalen)+4, SEEK_CUR);
		continue;
	}
	fseek(fp_push, -sizeof(tagHeader), SEEK_CUR);
	if((nowtime=RTMP_GetTime()-begintime)<pretimetamp) {
		printf("%d - %d\n", pretimetamp, nowtime);
		Sleep(pretimetamp-nowtime);
		continue;
	}
	char* pFileBuf=(char*)malloc(11+FINT24TOINT(tagHeader.datalen)+4);
	memset(pFileBuf,0,11+FINT24TOINT(tagHeader.datalen)+4);
	if(fread(pFileBuf,1,11+FINT24TOINT(tagHeader.datalen)+4,fp_push)!=11+FINT24TOINT(tagHeader.datalen)+4)
		break;
	if ((res = RTMP_Write(rtmp,pFileBuf,11+FINT24TOINT(tagHeader.datalen)+4)) <= 0) {
		printf("RTMP_Write end.\n");
		break;
	}
	pretimetamp = FINT24TOINT(tagHeader.timestamp);
	free(pFileBuf);
	pFileBuf=NULL; } //推流结束


res = RTMP_SetupURL(rtmp, "rtmp://live.hkstv.hk.lxdns.com/live//hks");//拉流
PRINTERROR(res, 1, "RTMP_SetupURL error.\n");
//if unable,the AMF command would be 'play' instead of 'publish'
//RTMP_EnableWrite(rtmp);//推流要设置写
res = RTMP_Connect(rtmp, NULL);
PRINTERROR(res, 1, "RTMP_Connect error.\n");
res = RTMP_ConnectStream(rtmp,0);
PRINTERROR(res, 1, "RTMP_ConnectStream error.\n");

//拉流
int nRead = 0, NRead = 0;
int bufsize = 1024*1024;
char* buf = (char*)malloc(bufsize);
FILE* fp_save = fopen("save.flv", "wb");
while(nRead=RTMP_Read(rtmp,buf,bufsize)) {
	fwrite(buf,1,nRead,fp_save);
	NRead += nRead;
	RTMP_LogPrintf("Receive: %5dByte, Total: %5.2fkB\n",nRead,NRead*1.0/1024);
}
//拉流结束



RTMP_Close(rtmp);
RTMP_Free(rtmp);
WSACleanup();
