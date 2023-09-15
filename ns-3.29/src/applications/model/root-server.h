#ifndef ROOT_SERVER_H
#define ROOT_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/random-variable-stream.h"

#include "ns3/dns.h"
#include "ns3/dns-header.h"


namespace ns3 {

class RootServer: public Object {
public:

	static TypeId GetTypeId(void);

	RootServer (void);

	virtual ~RootServer ();

	void DoDispose(void) {
		m_nsCache.DoDispose();
	}

	void AddZone(std::string zone_name, uint16_t ns_class,
			uint16_t type, uint32_t TTL, std::string rData);

	  /*
	   *Call the upper layer to send packet
	   */
	  void
	  SetReplyCallback (Callback<void, Ptr<Packet>, Address>);

	  Callback<void, Ptr<Packet>, Address> m_replyQuery;

	  void RootServerService(Ptr<Packet> nsQuery, Address toAddress);


private:
	virtual void StartApplication(void);
	virtual void StopApplication(void);

	SRVTable m_nsCache; //!< the Cache for nameserver records


};
}
#endif /* BIND_SERVER_H */
