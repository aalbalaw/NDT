#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/enum.h"
#include "ns3/abort.h"
#include "ns3/root-server.h"
#include "ns3/dns.h"
#include "ns3/dns-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("RootServer");
NS_OBJECT_ENSURE_REGISTERED(RootServer);
TypeId RootServer::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::RootServer").SetParent<Object>().SetGroupName("Application").AddConstructor<
					RootServer>();
	return tid;
}

RootServer::RootServer(void) {
	/* cstrctr */
}
RootServer::~RootServer() {
	/* dstrctr */
}

void RootServer::AddZone(std::string zone_name, uint16_t ns_class,
		uint16_t type, uint32_t TTL, std::string rData) {
	NS_LOG_FUNCTION(this << zone_name << TTL << ns_class << type << rData);
	m_nsCache.AddZone(zone_name, ns_class, type, TTL, rData);
}


void RootServer::StartApplication(void) {
	NS_LOG_FUNCTION(this);
	// Start expiration of the DNS records after TTL values.
	m_nsCache.SynchronizeTTL();
}

void RootServer::StopApplication() {
	NS_LOG_FUNCTION(this);

	DoDispose();
}

void RootServer::SetReplyCallback(Callback<void, Ptr<Packet>, Address> sendData) {
	m_replyQuery = sendData;
}


void RootServer::RootServerService(Ptr<Packet> nsQuery, Address toAddress) {
	// Assumptions made to Create the ROOT name server
	// Root name servers never create any NS requests.

	DNSHeader DnsHeader;
	uint16_t qType, qClass;
	std::string qName;
	bool foundInCache = false;

	nsQuery->RemoveHeader(DnsHeader);

	// Assume that only one question is attached to the DNS header
	std::list<QuestionSectionHeader> questionList;
	questionList = DnsHeader.GetQuestionList();

	qName = questionList.begin()->GetqName();
	qType = questionList.begin()->GetqType();
	qClass = questionList.begin()->GetqClass();
	NS_LOG_INFO("Receive NS Query for: "<<qName);
	// Return the first record that matches the requested qName
	// This supports the RR method
	SRVTable::SRVRecordI cachedRecord = m_nsCache.FindARecordMatches(qName,
			foundInCache);

	if (foundInCache) {
		Ptr<Packet> rootResponse = Create<Packet>();

		ResourceRecordHeader rrHeader;

		rrHeader.SetName(cachedRecord->first->GetRecordName());
		rrHeader.SetClass(cachedRecord->first->GetClass());
		rrHeader.SetType(cachedRecord->first->GetType());
		rrHeader.SetTimeToLive(cachedRecord->first->GetTTL());
		rrHeader.SetRData(cachedRecord->first->GetRData());

		DnsHeader.SetQRbit(0);
		DnsHeader.ResetOpcode();
		DnsHeader.SetOpcode(3);
		DnsHeader.AddAnswer(rrHeader);

		rootResponse->AddHeader(DnsHeader);

		m_replyQuery(rootResponse, toAddress);
	} else {
		//TODO
		// Send a reply contains RCODE = 3
	}
}

}
