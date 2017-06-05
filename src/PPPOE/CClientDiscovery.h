#ifndef CCLIENTDISCOVERY_H
#define CCLIENTDISCOVERY_H

#include "CReferenceControl.h"
#include "ace/Time_Value.h"
#include "pppoe.h"
#include "BaseDefines.h"

class CClientDiscovery : public CReferenceControl
{
public:
    CClientDiscovery(BYTE clientMac[ETH_ALEN]);
    virtual ~CClientDiscovery();

    BYTE *GetClientMac() {return m_clientMac;}
    WORD64 GetClientDiscoveryId();
    void SetClientMac(BYTE clientMac[ETH_ALEN]);
    WORD16 GetSession() {return m_sess;}
    void SetSession(WORD16 sess) {m_sess = sess;}
    const ACE_Time_Value &GetStartTime() {return m_startTime;}
    void SetStartTime(const ACE_Time_Value &startTime) {m_startTime = startTime;}
    void GetSvcName(CHAR svcName[PPPOE_MAX_SERVICE_NAME_LENGTH]);
    void SetSvcName(CHAR svcName[PPPOE_MAX_SERVICE_NAME_LENGTH]);
    WORD16 GetReqMtu() {return m_requestedMtu;}
    void SetReqMtu(WORD16 reqMtu) {m_requestedMtu = reqMtu;}
    PPPoETag &GetRelayId() {return m_relayId;}
    
private:
    BYTE m_clientMac[ETH_ALEN];
    WORD16 m_sess;		/* Session number */ // TBD!!!! May be unnecessary
    ACE_Time_Value m_startTime; //time_t startTime;		/* When session started */ // �Ʒ�ʱ����UserMgr���𣬲���Ҫpppoe
    CHAR m_serviceName[PPPOE_MAX_SERVICE_NAME_LENGTH];	/* Service name */ // !!!!����rp-pppoe�Ĵ��뼰��϶�pppoe 
                                                                           // rfc����⣬���ֶ�ò�Ʋ���Ҫ���档
    WORD16 m_requestedMtu;     /* Requested PPP_MAX_PAYLOAD  per RFC 4638 */

    //��Ͽ�Դ��Ŀrp-pppoe-3.11
    //���˶�m_relayId�����: ��PPPoE discovery�׶Σ����ĳ��client�ı���Я�����ֶΣ�Ӧ���Ա��档�Ա��ں����PADT��Я����
    //����rp-pppoe�У���client��server�����˲�ͬ�ķ�����
    //client: ��Ӧÿ��struct PPPoEConnectionStruct���ڽ���PADO��PADSʱ������relayId
    //server: ��ȫ�ֱ���relayId���棬����ζ�ţ���δ�����client��relayId��
    //���ڵĽ������: server�������client��relayId�����͵�PADT�в�Я����ѡ�
    //���������Ҫ��һ�ֲο��������: 
    // CPPPOEDiscoveryHandler::processPADR()�У���relayId����CSession�У������Ǵ����У���Ϊ����漰����CClientDiscovery
    // �Ĺ���
    PPPoETag m_relayId;
    
};

#endif // CCLIENTDISCOVERY_H

