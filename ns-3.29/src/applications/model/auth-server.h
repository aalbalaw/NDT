#ifndef AUTH_SERVER_H
#define AUTH_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/random-variable-stream.h"

#include "ns3/dns.h"
#include "ns3/dns-header.h"


namespace ns3 {

class AuthServer: public Object {
public:

	static TypeId GetTypeId(void);

	AuthServer (void);

	virtual ~AuthServer ();

	void DoDispose(void) {
		m_nsCache.DoDispose();
	}

	void AddZone(std::string zone_name,  uint16_t ns_class,
			uint16_t type, uint32_t TTL, std::string rData);

	void AddManifestRecord(std::string content_name,
			uint16_t ns_class, uint16_t type, uint32_t TTL, std::string manifestRecord);
	  /*
	   *Call the upper layer to send packet
	   */
	  void
	  SetReplyCallback (Callback<void, Ptr<Packet>, Address>);

	  Callback<void, Ptr<Packet>, Address> m_replyQuery;

	  void AuthServerService(Ptr<Packet> nsQuery, Address toAddress);

	  void HandleManifestQuery(Ptr<Packet> nsQuery, Address toAddress);

	  void HandleDNSQuery(Ptr<Packet> nsQuery, Address toAddress);


private:
	virtual void StartApplication(void);
	virtual void StopApplication(void);

	SRVTable m_nsCache; //!< the Cache for nameserver records
	SRVTable m_mrCache;

};
}
#endif /* BIND_SERVER_H */
