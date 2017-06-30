/***********************************************************************
	Copyright (c) 2017, The OpenBRAS project authors. All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are
	met:

	  * Redistributions of source code must retain the above copyright
		notice, this list of conditions and the following disclaimer.

	  * Redistributions in binary form must reproduce the above copyright
		notice, this list of conditions and the following disclaimer in
		the documentation and/or other materials provided with the
		distribution.

	  * Neither the name of OpenBRAS nor the names of its contributors may
		be used to endorse or promote products derived from this software
		without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**********************************************************************/

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

