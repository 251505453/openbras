/*
 * Copyright (c) 2017 The OpenBras project authors. All Rights Reserved.
 */

#ifndef CPPP_H
#define CPPP_H

#include "aceinclude.h"
#include "pppdef.h"
#include "CPPPLCP.h"
#include "CPPPIPCP.h"
#include "CPapProtocol.h"
#include <string.h>
#include "ISessionManager.h"
#include "CChapProtocol.h"
#include <string>
#include "CAuthen.h"

enum PPPPhase {
    PHASE_DEAD = 0,
    PHASE_INITIALIZE,
    PHASE_ESTABLISH,
    PHASE_AUTHENTICATE,
    PHASE_NETWORK,
    PHASE_TERMINATE
};

#define BRAS_DEFAULT_AUTHTYPE (PPP_CHAP)
#define BRAS_DEFAULT_HOSTNAME ("WFNEX")

class IPPPSink 
{
public:
    virtual ~IPPPSink(){}

    // Protocol related
    virtual void OnLCPDown(const std::string reason) = 0;

    // Packet related
    virtual void OnPPPOutput(unsigned char *packet, size_t size) = 0;  // ���PPP����
    virtual void OnPPPPayload(unsigned char *packet, size_t size) = 0; // ���յ�PPP���ݱ��ģ��������Ǵ�����Ϊ����ֻ����
                                                                       // PPPЭ�鱨�ģ���log��ֱ�ӷ���

    // Authentication Manager related
    virtual void OnAuthRequest(Auth_Request &authReq) = 0;
    
    // Subscriber Manager related
    virtual SWORD32 OnAddSubscriber(Session_User_Ex &sInfo) = 0;
    virtual SWORD32 OnDelSubscriber() = 0;
};

class CPPP : public IPPPLCPSink, 
                        public IPPPIPCPSink,
                        public IAuthenSvrSink
{
public:
    CPPP(IPPPSink *psink);
    virtual ~CPPP();
    
    void StartFms();

    void Init();

    void OnAuthResponse(WORD32 result, std::string &reason);
    
    virtual void Input(unsigned char *packet, size_t size);
    virtual void InputPayload(unsigned char *packet, size_t size);

    CPPPLCP &GetLCP() {return m_lcp;}
    CPPPIPCP &GetIPCP() {return m_ipcp;}
    void SetAuthType(BYTE authType) {m_lcp.SetAuthType(authType);}
    BYTE GetAuthType() {return m_lcp.GetAuthType();}
    void SetHostName(std::string &hostName) {m_hostName = hostName;}
    
protected:
    void NewPhase(PPPPhase phase);

    //IPPPLCPSink
    virtual void OnLCPUp();
    virtual void OnLCPDown();
    virtual void OnLCPTerminate();
    virtual void OnLCPOutput(unsigned char *packet, size_t size);

    //IPPPIPCPSink
    virtual void OnIPCPUp();
    virtual void OnIPCPDown();
    virtual void OnIPCPPayload(unsigned char *packet, size_t size);
    virtual void OnIPCPOutput(unsigned char *packet, size_t size);

    // For interface IAuthenSvrSink
    virtual void SendAuthRequest2AM(Auth_Request &authReq);
    virtual void OnAuthenOutput(unsigned char *packet, size_t size);
    virtual void OnAuthenResult(int result, int protocol);

private:
    CPPPLCP m_lcp;
    CPapProtocolSvr m_pap;
    CChapProtocolSvr m_chap;
    CPPPIPCP m_ipcp;
    IPPPSink *m_pSink;
    bool m_bLinkOk;
    PPPPhase m_phase;
    std::string m_hostName;
};

#endif//CPPP

