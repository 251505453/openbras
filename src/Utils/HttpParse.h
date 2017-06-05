#ifndef _HTTP_PARSE_H_
#define _HTTP_PARSE_H_
#include "aceinclude.h"
#include "BaseDefines.h"

typedef enum
{
   HTTP_PARSE_FIELD_URL     = 0,    /*��ȡ�ͻ����������Դ*/
   HTTP_PARSE_FIELD_HOST    = 1,    /*��ȡhttp������host�ֶ�*/
   HTTP_PARSE_FIELD_REQTYPE = 2,    /*��ȡhttp������������GET POST*/ 
   HTTP_PARSE_FIELD_MAX     = 0xFF,   
}HTTP_FIELD_NAME;

typedef enum
{
   HTTP_PARSE_SUCCESS       = 0,  /*�ɹ�*/
   HTTP_PARSE_PARAM_ERROR   = 1,  /*��������*/
   HTTP_PARSE_NOT_FOUND     = 2,  /*δ���ҵ����ҵ��ֶ�*/
}HTTP_PARSE_RET;

/*****************************************************************************
* �� �� ��  : http_get_field_value
* ��������  : ��ȡhttp������ָ���Ĳ���
* �������  :  
               pHttpPkt   : http ����
               dwPktLen   : ���ĵĳ���
               ucFieldFlag: ��ȡ�����ֶ�
* �������  : 
               ppFieldValue:��ȡ��������ʼ��ַ
               pdwValueLen :�����ĳ���
* �� �� ֵ  : 
* ���ú���  : 
* ��������  : 
* 
* �޸���ʷ  :
* ��    ��  :
* ��    ��  : linlizeng
* �޸�����  : 
*
*****************************************************************************/
HTTP_PARSE_RET  http_get_field_value(char *pHttpPkt,
                                          uint32_t          dwPktLen,
                                          HTTP_FIELD_NAME ucFieldFlag,
                                          char          **ppFieldValue,
                                          uint32_t         *pdwValueLen);
#endif

