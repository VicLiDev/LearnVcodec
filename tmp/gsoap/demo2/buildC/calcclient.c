// File: calcclient.c
#include "soapH.h"    // include the generated declarations
#include "calc.nsmap" // include the generated namespace table
int main() 
{
  struct soap *soap = soap_new(); 
  double result; 
  if (soap_call_ns2__add(soap, NULL, NULL, 1.0, 2.0, &result) == SOAP_OK) 
    printf("The sum of 1.0 and 2.0 is %lg\n", result); 
  else 
    soap_print_fault(soap, stderr); 
  soap_destroy(soap); // delete managed objects
  soap_end(soap);     // delete managed data and temporaries 
  soap_free(soap);    // finalize and delete the context
}