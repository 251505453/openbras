/***********************************************************************
 * Copyright (C) 2014, Nanjing WFNEX Technology Co., Ltd 
 **********************************************************************/

#include "CPPPOE.h"
#include "CSession.h"
#include "CClientDiscovery.h"
#include <string>
#include "aim_ex.h"
#include "CPPP.h"
#include "CPPPOEEventHandler.h"
#include "IEventReactor.h"

CPPPOE::CPPPOE()
    : m_DiscoveryHandle(*this)
    , m_SessionHandle(*this)
    , m_nSessionIdBase(1)
    , m_etherIntf(*this)
    , m_authType(BRAS_DEFAULT_AUTHTYPE)
    , m_hostName(BRAS_DEFAULT_HOSTNAME)
{
    ::memset(m_acName, 0, sizeof m_acName);
    ::memset(m_svcName, 0, sizeof m_svcName);
    m_psessionid.InIt(1, 60000);
}

CPPPOE::~CPPPOE()
{
    ACE_DEBUG ((LM_DEBUG, "CPPPOE::~CPPPOE\n"));

    ClearAllClientDiscovery();
    ClearAllSession();
    IAuthManager::instance().Close();
    ISessionManager::instance().Close();
    IEventReactor::Instance().Close();
}

void CPPPOE::ClearAllClientDiscovery()
{
    ACE_DEBUG ((LM_DEBUG, "CPPPOE::ClearAllClientDiscovery\n"));
    
    ClientDiscoveryType::iterator it = m_clientDiscoveryMgr.begin();
    while (it != m_clientDiscoveryMgr.end())
    {
        it = m_clientDiscoveryMgr.erase(it);
    } 
}

void CPPPOE::ClearAllSession()
{
    ACE_DEBUG ((LM_DEBUG, "CPPPOE::ClearAllSession\n"));
    
    // Currently, when PPPOE process is down, we don't send PADT to users, and don't notify session manager to
    // delete the user.
    SESSIONType::iterator it = m_sessionMgr.begin();
    while (it != m_sessionMgr.end())
    {
        it = m_sessionMgr.erase(it);
    } 
}

void CPPPOE::Init()
{
    ACE_DEBUG ((LM_INFO, "CPPPOE::Init\n"));
    
    IAuthManager::instance().OpenWithSink(this);
    ISessionManager::instance().openWithSink(this);
    
    #ifdef DEBUG_CONFIG_INTERFACE
    ::memcpy(m_acName, "JSNJ-WFNEX-BRAS", strlen("JSNJ-WFNEX-BRAS"));
    ::memcpy(m_svcName, "JSNJ-BROADBAND-ACCESS", strlen("JSNJ-BROADBAND-ACCESS"));

    SetVBUIIntfName("p2p1");
    SetVBUIIntfMtu(ETH_DATA_LEN);
    std::string intfIp("10.1.1.1");
    SetVBUIIntfIP(intfIp);
    #endif
}

int CPPPOE::Start()
{
    ACE_DEBUG ((LM_INFO, "CPPPOE::Start\n"));
    
    m_DiscoveryHandle.Init(); // 此函数需要m_etherIntf的接口名称
    m_SessionHandle.Init();

    int ret = m_etherIntf.SetIntfMacByName(); // 此函数需要m_DiscoveryHandle.Init()得到的raw socket fd
    if (-1 == ret)
    {
        ACE_DEBUG((LM_ERROR, "CPPPOE::Start(), m_etherIntf.SetIntfMacByName() failed.\n"));
        return -1;
    }
    
    return 0;
}

CPPPOEDiscoveryHandler &CPPPOE::GetPppoeDiscoveryHndl() 
{
    return m_DiscoveryHandle;
}

CPPPOESessionHandler &CPPPOE::GetPppoeSessionHndl() 
{
    return m_SessionHandle;
}

CEtherIntf &CPPPOE::GetEtherIntf() 
{
    return m_etherIntf;
}

// m_clientDiscoveries related operations
int CPPPOE::CreateClientDiscovery(BYTE clientMac[ETH_ALEN], CCmAutoPtr<CClientDiscovery> &discovery)
{
    if (NULL == clientMac)
    {
        ACE_DEBUG((LM_ERROR, "CPPPOE::CreateClientDiscovery(), clientMac NULL.\n"));
        return -1;
    }
    
    WORD64 clientDisId = 0;
    ::memcpy(&clientDisId, clientMac, ETH_ALEN);
    
    CCmAutoPtr<CClientDiscovery> clientDis(new CClientDiscovery(clientMac));
    
    if (AddClientDiscovery(clientDisId, clientDis) != -1)
    {
        discovery = clientDis;
        return 0;
    }

    return -1;
}

int CPPPOE::AddClientDiscovery(WORD64 clientMac, CCmAutoPtr<CClientDiscovery> &discovery)
{
    ACE_DEBUG ((LM_DEBUG, "CPPPOE::AddClientDiscovery, clientMac = %#x\n", clientMac));
    
    ACE_GUARD_RETURN (ACE_Thread_Mutex, g, m_mutex4ClientDiscoveryMgr, -1);
    
    ClientDiscoveryType::iterator it = m_clientDiscoveryMgr.find(clientMac);
    
    if (it == m_clientDiscoveryMgr.end())
    {
        m_clientDiscoveryMgr[clientMac] = discovery;
        return 0;
    }

    ACE_DEBUG ((LM_DEBUG, "CPPPOE::AddClientDiscovery(), clientMac(%#x) already exists\n", clientMac));
    
    return -1;
}

int CPPPOE::RemoveClientDiscovery(WORD64 clientMac)
{
    ACE_DEBUG ((LM_DEBUG, "CPPPOE::RemoveClientDiscovery, clientMac = %#x\n", clientMac));
    
    ACE_GUARD_RETURN (ACE_Thread_Mutex, g, m_mutex4ClientDiscoveryMgr, -1);
    
    ClientDiscoveryType::iterator it = m_clientDiscoveryMgr.find(clientMac);

    if (it == m_clientDiscoveryMgr.end())
    {
        ACE_DEBUG ((LM_ERROR, "CPPPOE::RemoveClientDiscovery, no such session with id %#x.\n", clientMac));
        return -1;
    }
    
    m_clientDiscoveryMgr.erase(it);
    
    return 0;  
}

int CPPPOE::RemoveClientDiscovery(CCmAutoPtr<CClientDiscovery> &discovery)
{
    WORD64 clientMac = discovery->GetClientDiscoveryId();
    return RemoveClientDiscovery(clientMac);
}

int CPPPOE::CreateSession(CCmAutoPtr<CSession> &session)
{
    WORD16 sessionId = CalculateValidSessId();
    if (0 == sessionId)
    {
        ACE_DEBUG((LM_ERROR, "CPPPOE::CreateSession(), server has reached maximum sessions.\n"));
        return -1;
    }
    
    CCmAutoPtr<CSession> sess(new CSession(*this, sessionId));
    sess->GetPPP().SetAuthType(m_authType);
    sess->GetPPP().SetHostName(m_hostName);
    
    if (AddSession(sessionId,sess) != -1)
    {
        session = sess;
        return 0;
    }

    return -1;
}

int CPPPOE::AddSession(WORD16 sessionid, CCmAutoPtr<CSession> &session)
{
    ACE_DEBUG ((LM_DEBUG, "CPPPOE::AddSession, sessionid = %#x\n", sessionid));
    
    ACE_GUARD_RETURN (ACE_Thread_Mutex, g, m_mutex4SessionMgr, -1);
    
    SESSIONType::iterator it = m_sessionMgr.find(sessionid);
    
    if (it == m_sessionMgr.end())
    {
        m_sessionMgr[sessionid]=session;
        return 0;
    }

    ACE_DEBUG ((LM_DEBUG, "CPPPOE::AddSession(), session(%u) already exists\n", sessionid));
    
    return -1;
}

int CPPPOE::RemoveSession(WORD16 sessionid)
{
    ACE_DEBUG ((LM_DEBUG, "CPPPOE::RemoveSession, sessionid=%#x\n", sessionid));

    ACE_GUARD_RETURN (ACE_Thread_Mutex, g, m_mutex4SessionMgr, -1);
    
    SESSIONType::iterator it = m_sessionMgr.find(sessionid);
    if (it == m_sessionMgr.end())
    {
        ACE_DEBUG ((LM_ERROR, "no such seesion in the session manager.\n"));
        return -1;
    }
    
    m_sessionMgr.erase(it);
    
    return 0;  
}

int CPPPOE::RemoveSession(CCmAutoPtr<CSession> &session)
{
    WORD16 sessionid = session->GetSessionId();
    return RemoveSession(sessionid);
}

CSession * CPPPOE::FindSession(WORD16 sessionid)
{
    ACE_GUARD_RETURN (ACE_Thread_Mutex, g, m_mutex4SessionMgr, NULL);

    SESSIONType::iterator it = m_sessionMgr.find(sessionid);
    if (it == m_sessionMgr.end())
    {
        ACE_DEBUG ((LM_DEBUG, "CPPPOE::FindSession, no such session with id %#x.\n", sessionid));
        return NULL;
    }
    
    CCmAutoPtr<CSession> &session = it->second;
    return session.Get();
}

WORD16 CPPPOE::CalculateValidSessId()
{
    ACE_DEBUG ((LM_DEBUG, "CPPPOE::CalculateValidSessId\n"));

    ACE_GUARD_RETURN (ACE_Thread_Mutex, g, m_mutex4SessionMgr, PPPOE_RESERVED_SESSION_ID);

    return m_psessionid.GetId();
}

void CPPPOE::OnLCPDown(WORD16 sessionId, const std::string &reason)
{
    ACE_DEBUG ((LM_DEBUG, "CPPPOE::OnLCPDown, sessionId=%#x, reason=%s\n", sessionId, reason.c_str()));
    
    CSession *session = FindSession(sessionId);
    if (NULL == session)
    {
        ACE_DEBUG ((LM_ERROR, "No such session with id %#x\n", sessionId));
        return;
    }    

    m_DiscoveryHandle.sendPADT(session, reason.c_str());
}

#ifdef DEBUG_CHAP
#include "../PPP/md5.h"

static int
chap_md5_verify_response(Auth_Request &authReq)
{
	MD5_CTX ctx;
	unsigned char idbyte = authReq.chapid;
	unsigned char hash[MD5_HASH_SIZE];
	int challenge_len, response_len;

	challenge_len = AUTHMGR_MAX_CHALLENGE_SIZE;
	response_len = strlen(authReq.userpasswd);

    ACE_DEBUG ((LM_DEBUG, "chap_md5_verify_response\n"));

    #define USERNAME "asdfsdfasdf123412340!@#$%^&*()_"
    #define PASSWD USERNAME
    CHAR *authenUsername = USERNAME;
    if (strncmp(authReq.username, authenUsername, strlen(authenUsername)) != 0)
    {
        ACE_DEBUG ((LM_ERROR, "Username wrong.\n"));
        return -1;
    }
    
	if (response_len == MD5_HASH_SIZE) {
		/* Generate hash of ID, secret, challenge */
		MD5_Init(&ctx);
		MD5_Update(&ctx, &idbyte, 1);
		MD5_Update(&ctx, (BYTE *)(PASSWD), strlen(PASSWD));
		MD5_Update(&ctx, authReq.challenge, challenge_len);
		MD5_Final(hash, &ctx);

		/* Test if our hash matches the peer's response */
		if (memcmp(hash, authReq.userpasswd, MD5_HASH_SIZE) == 0) 
        {
            ACE_DEBUG ((LM_INFO, "authentication succeeded.\n"));
			return 0;
		}
	}

    static CHAR buffer[50];
    ::memset(buffer, 0, sizeof(buffer));
    for (SWORD32 i = 0; i < MD5_HASH_SIZE; ++i)
    {
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "%2x ", hash[i]);
    }
    ACE_DEBUG ((LM_DEBUG, "hash=%s\n", buffer));
    
	return -1;
}
#endif

void CPPPOE::OnPPPOEAuthRequest(Auth_Request &authReq)
{
    ACE_DEBUG((LM_DEBUG, "CPPPOE::OnPPPOEAuthRequest\n"));
    
    authReq.user_type = USER_TYPE_PPPOE;
    authReq.aaaRequestType = AIM_REQ_TYPE_AUTHEN;
    authReq.nasPortType = RADIUS_NAS_PORT_TYPE_ETHERNET;

    BYTE clientEth[ETH_ALEN] = {0};
    CSession * session = FindSession(authReq.session);
    if (NULL == session)
    {
        ACE_DEBUG ((LM_ERROR, "No such session with id %#x\n", authReq.session));
        return;
    }
    session->GetClientEth(clientEth);
    ::memcpy(authReq.mac, clientEth, ETH_ALEN);

    authReq.framedProtocol = FRAMED_PROTOCOL_PPP;
    authReq.serviceType = SERVICE_TYPE_LOGIN;

    ACE_DEBUG ((LM_DEBUG, "dump:\n"));
    ACE_DEBUG ((LM_DEBUG, 
                "user_type=%#x, authtype=%#x, chapid=%#x, \nsessionid=%#x, mac=%2x:%2x:%2x:%2x:%2x:%2x, \n"
                "username=%s, framedProtocol=%#x, serviceType=%#x\n",
                authReq.user_type, authReq.authtype, authReq.chapid,
                authReq.session, authReq.mac[0], authReq.mac[1], authReq.mac[2], 
                authReq.mac[3], authReq.mac[4], authReq.mac[5],
                authReq.username, authReq.framedProtocol, authReq.serviceType));
    if (PPP_PAP == m_authType)
    {
        ACE_DEBUG ((LM_DEBUG, "userpasswd=%s\n", authReq.userpasswd));
    }
    else if (PPP_CHAP == m_authType)
    {
        static CHAR buffer[50];
        ::memset(buffer, 0, sizeof(buffer));
        for (SWORD32 i = 0; i < AUTHMGR_MAX_CHALLENGE_SIZE; ++i)
        {
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "%2x ", authReq.userpasswd[i]);
        }
        ACE_DEBUG ((LM_DEBUG, "CHAP response=%s\n", buffer));

        ::memset(buffer, 0, sizeof(buffer));
        for (SWORD32 i = 0; i < AUTHMGR_MAX_CHALLENGE_SIZE; ++i)
        {
            snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "%2x ", authReq.challenge[i]);
        }
        ACE_DEBUG ((LM_DEBUG, "CHAP challenge=%s\n", buffer));
    }
    
    ACE_DEBUG((LM_DEBUG, "IAuthManager::instance().AuthRequest\n"));
    IAuthManager::instance().AuthRequest(&authReq);

#if defined(DEBUG_CHAP) || defined(DEBUG_PAP)
    Auth_Response response;
    ::memset(&response, 0, sizeof(Auth_Response));

    response.session = authReq.session;
    ::memcpy(response.mac, authReq.mac, 6);

    int result = 1;

    #ifdef DEBUG_PAP
    #define USERNAME "asdfsdfasdf123412340!@#$%^&*()_"
    #define PASSWD USERNAME
    CHAR *authenUsername = USERNAME;
    CHAR *authenPasswd = PASSWD;
    if (strlen(authReq.username) == strlen(authenUsername)
        && (strncmp(authReq.username, authenUsername, strlen(authenUsername)) == 0)
        && strlen(authReq.userpasswd) == strlen(authenPasswd)
        && (strncmp(authReq.userpasswd, authenPasswd, strlen(authenPasswd)) == 0))
    {
        result = 0;
    }
    else
    {
        result = -1;
    }
    #endif

    #ifdef DEBUG_CHAP
    result = chap_md5_verify_response(authReq);
    #endif

    if (0 == result)
    {
        response.authResult = 0;
        ::strncpy(response.authResultDesc, "success", strlen("success"));
    }
    else
    {
        response.authResult = 1;
        ::strncpy(response.authResultDesc, "fail", strlen("fail"));
    }
    OnAuthResponse(&response);
#endif
}

int CPPPOE::OnAuthResponse(const Auth_Response *response)
{
    ACE_DEBUG ((LM_DEBUG, "CPPPOE::OnAuthResponse\n"));
    
    CPPPOEAuthRespEvntHndl * pevent = new CPPPOEAuthRespEvntHndl(*this, response);
    if (NULL == pevent)
    {
        ACE_DEBUG ((LM_ERROR, "Failed to new CPPPOEAuthRespEvntHndl.\n"));
        return -1;
    }

    if (IEventReactor::Instance().ScheduleEvent(pevent) != 0)
    {
        ACE_DEBUG ((LM_ERROR, "Failed to schedule event.\n"));
        return -1;
    }
    
    return 0;
}

int CPPPOE::HandleAuthResponse(const Auth_Response *response)
{
    ACE_DEBUG((LM_DEBUG, "CPPPOE::HandleAuthResponse\n"));

    if (NULL == response)
    {
        ACE_DEBUG ((LM_ERROR, "Arg NULL\n"));
        return -1;
    }

    ACE_DEBUG ((LM_DEBUG, "dump:\n"));
    ACE_DEBUG ((LM_DEBUG, 
                "session=%#x, mac=%2x:%2x:%2x:%2x:%2x:%2x, \n"
                "authResult=%u, authResultDesc=%s, \n"
                "subscriberIP=%#x, primaryDNS=%#x, secondaryDNS=%#x\n", 
                response->session, response->mac[0], response->mac[1], response->mac[2],
                response->mac[3], response->mac[4], response->mac[5],
                response->authResult, response->authResultDesc,
                response->subscriberIP, response->primaryDNS, response->secondaryDNS));
    
    WORD16 sessionid = response->session;
    CSession * psession = FindSession(sessionid);
    if (NULL == psession)
    {
        ACE_DEBUG((LM_ERROR, "CPPPOE::OnAuthResponse, no session with id %#x\n", sessionid));
        return -1;
    }
    
    // !!! TBD Check if response->mac matches session's clientEth.

    // 注意: IPCP相关的设置要放在psession->OnPPPOEAuthResponse之前，因为一认证响应，马上就协商IPCP了。
    CPPPIPCP &ipcp = psession->GetPPP().GetIPCP();
    ipcp.SetMyIP( m_etherIntf.GetIntfIp() );
    ipcp.SetSubscriberIP( htonl(response->subscriberIP) );
    ipcp.SetPrimaryDNS( htonl(response->primaryDNS) );
    ipcp.SetSecondaryDNS( htonl(response->secondaryDNS) );

#if defined(DEBUG_CHAP) || defined(DEBUG_PAP)
    //CPPPIPCP &ipcp = psession->GetPPP().GetIPCP();
    //ipcp.SetMyIP( m_etherIntf.GetIntfIp() );
    
    std::string hisIp("10.1.1.2");
    ipcp.SetSubscriberIP( hisIp );
    std::string dns1("20.1.1.1");
    ipcp.SetPrimaryDNS( dns1 );
    std::string dns2("20.1.1.2");
    ipcp.SetSecondaryDNS( dns2 );
#endif
    
    WORD32 result = response->authResult;
    std::string reason(response->authResultDesc);
    psession->OnPPPOEAuthResponse(result, reason);

    return 0;
}

int CPPPOE::OnAddUserResponse(const UM_RESPONSE &response)
{
    return 0;
}

int CPPPOE::OnDeleteUserResponse(const UM_RESPONSE &response)
{
    return 0;
}

int CPPPOE::OnModifyUserResponse(const UM_RESPONSE &response)
{
    return 0;
}

int CPPPOE::OnKickUserNotify(const Sm_Kick_User* kickInfo)
{
    ACE_DEBUG ((LM_DEBUG, "CPPPOE::onKickUserNotify\n"));
    
    CPPPOEKickUserEvntHndl * pevent = new CPPPOEKickUserEvntHndl(*this, kickInfo);
    if (NULL == pevent)
    {
        ACE_DEBUG ((LM_ERROR, "Failed to new CPPPOEKickUserEvntHndl.\n"));
        return -1;
    }

    if (IEventReactor::Instance().ScheduleEvent(pevent) != 0)
    {
        ACE_DEBUG ((LM_ERROR, "Failed to schedule event.\n"));
        return -1;
    }

    return 0;
}

int CPPPOE::HandleKickUserNotify(const Sm_Kick_User* kickInfo)
{
    ACE_DEBUG ((LM_DEBUG, "CPPPOE::HandleKickUserNotify\n"));
    
    if (NULL == kickInfo)
    {
        ACE_DEBUG ((LM_ERROR, "NULL arg\n"));
        return -1;
    }

    CSession * session = FindSession(kickInfo->session);
    if (NULL == session)
    {
        ACE_DEBUG ((LM_ERROR, "Can't find session(%#x)\n", kickInfo->session));
        return -1;
    }

    BYTE clientEth[ETH_ALEN];
    ::memset(clientEth, 0, ETH_ALEN);
    session->GetClientEth(clientEth);

    if (memcmp(clientEth, kickInfo->mac, ETH_ALEN) != 0)
    {
        ACE_DEBUG ((LM_ERROR, 
                    "invalid subscriber mac"
                    "MAC in local memory: %02x:%02x:%02x:%02x:%02x:%02x, "
                    "MAC in kickInfo: %02x:%02x:%02x:%02x:%02x:%02x\n",
                    clientEth[0], clientEth[1], clientEth[2], clientEth[3], clientEth[4], clientEth[5],
                    kickInfo->mac[0], kickInfo->mac[1], kickInfo->mac[2],
                    kickInfo->mac[3], kickInfo->mac[4], kickInfo->mac[5]));
        return -1;
    }
    
    m_DiscoveryHandle.sendPADT(session, "Kick User");
    
    return 0;
}

SWORD32 CPPPOE::AddSubscriber(Session_User_Ex &sInfo)
{
    ACE_DEBUG ((LM_DEBUG, "CPPPOE::AddSubscriber, dump:\n"));
    ACE_DEBUG ((LM_DEBUG, 
                "session=%#x, mac=%2x:%2x:%2x:%2x:%2x:%2x, \nuser_type=%#x, auth_type=%#x\n"
                "username=%s, userIp=%#x,\nprimaryDNS=%#x, secondaryDNS=%#x\n", 
                sInfo.session, 
                sInfo.mac[0], sInfo.mac[1], sInfo.mac[2], 
                sInfo.mac[3], sInfo.mac[4], sInfo.mac[5],
                sInfo.user_type, sInfo.auth_type, sInfo.user_name, 
                sInfo.user_ip, sInfo.user_ip, sInfo.primary_dns, sInfo.secondary_dns
                ));
    return ISessionManager::instance().addUserRequest(&sInfo);
}

SWORD32 CPPPOE::DelSubscriber(Session_Offline &sInfo)
{
    ACE_DEBUG ((LM_DEBUG, "CPPPOE::DelSubscriber, dump:\n"));
    ACE_DEBUG ((LM_DEBUG, 
                "user_type=%#x, session=%#x\n"
                 "mac=%2x:%2x:%2x:%2x:%2x:%2x\n", 
                sInfo.user_type, sInfo.session,
                sInfo.mac[0], sInfo.mac[1], sInfo.mac[2], 
                sInfo.mac[3], sInfo.mac[4], sInfo.mac[5]));
    return ISessionManager::instance().deleteUserRequest(&sInfo);
}

CHAR * CPPPOE::GetACName()
{
    return m_acName;
}

CHAR * CPPPOE::GetSvcName()
{
    return m_svcName;
}

void CPPPOE::SetACName(const CHAR acName[PPPOE_MAX_ACNAME_LENGTH])
{
    if (acName != NULL)
    {
        ::strncpy(m_acName, acName, sizeof m_acName);
        m_acName[PPPOE_MAX_ACNAME_LENGTH - 1] = 0;
    }
}

void CPPPOE::SetSvcName(const CHAR svcName[PPPOE_MAX_SERVICE_NAME_LENGTH])
{
    if (svcName != NULL)
    {
        ::strncpy(m_svcName, svcName, sizeof m_svcName);
        m_svcName[PPPOE_MAX_SERVICE_NAME_LENGTH - 1] = 0;
    }
}


void CPPPOE::GetVBUIIntfName(CHAR acIntfName[ETHER_INTF_NAME_SIZE+1])
{
    m_etherIntf.GetIntfName(acIntfName);
}

void CPPPOE::SetVBUIIntfName(const CHAR acIntfName[ETHER_INTF_NAME_SIZE+1])
{
    m_etherIntf.SetIntfName(acIntfName);
}

WORD16 CPPPOE::GetVBUIIntfMtu()
{
    return m_etherIntf.GetIntfMtu();
}

void CPPPOE::SetVBUIIntfMtu(WORD16 mtu)
{
    m_etherIntf.SetIntfMtu(mtu);
}

WORD32 CPPPOE::GetVBUIIntfIP()
{
    return m_etherIntf.GetIntfIp();
}

void CPPPOE::SetVBUIIntfIP(WORD32 ipAddr)
{
    m_etherIntf.SetIntfIp(ipAddr);
}

void CPPPOE::SetVBUIIntfIP(std::string &ipAddr)
{
    m_etherIntf.SetIntfIp(ipAddr);
}

void CPPPOE::SetAuthType(BYTE authType)
{
    m_authType = authType;
}

void CPPPOE::SetHostName(std::string &hostName)
{
    m_hostName = hostName;
}
void CPPPOE::FreeId(uint16_t id)
{
    if(id >= m_psessionid.Getstartid() && id <= m_psessionid.Getendid())
        m_psessionid.GetSessionId().push_back(id);
    else
        ACE_DEBUG((LM_ERROR, "CPPPOE::FreeId(), m_SessionId NULL.\n"));
}


