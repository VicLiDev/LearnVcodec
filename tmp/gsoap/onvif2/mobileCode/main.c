 #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "soapStub.h"
#include "MobileCodeWSSoap.nsmap"

void getMobileCodeInfo(char *mobileCode){
	struct soap *soap = NULL;
	const char *endpoint = "http://ws.webxml.com.cn/WebServices/MobileCodeWS.asmx";
	struct _ns1__getMobileCodeInfo req;
	struct _ns1__getMobileCodeInfoResponse resp;

	soap = soap_new(); // allocate and initalize a context

	soap_set_mode(soap, SOAP_C_UTFSTRING); // support multibyte string(for Chinese)
	
	memset(&req, 0x00, sizeof(req));
	req.mobileCode = mobileCode;
	req.userID = NULL;
	
	if(SOAP_OK == soap_call___ns1__getMobileCodeInfo(soap, endpoint, NULL, &req, &resp)){
		if (NULL != resp.getMobileCodeInfoResult){
			printf("%s\n", resp.getMobileCodeInfoResult);
		}
	}

	soap_destroy(soap); // delete deserialized objects
	soap_end(soap); // delete allocated data
	soap_free(soap); // free the soap struct context data
}

int main(int argc, char **argv)
{
	if (argc < 2){
        printf("usage: %s <number> \n", argv[0]);
		return 0;
	}
	getMobileCodeInfo(argv[1]);

	return 0;
}
