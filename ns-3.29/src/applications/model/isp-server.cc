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
#include "ns3/isp-server.h"
#include "ns3/dns.h"
#include "ns3/dns-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("IspServer");
NS_OBJECT_ENSURE_REGISTERED(IspServer);
TypeId IspServer::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::IspServer").SetParent<Object>().SetGroupName(
					"Application").AddConstructor<IspServer>();
	return tid;
}

IspServer::IspServer(void) {
	/* cstrctr */
}
IspServer::~IspServer() {
	/* dstrctr */
}

void IspServer::AddZone(std::string zone_name, uint16_t ns_class,
		uint16_t type, uint32_t TTL, std::string rData) {
	NS_LOG_FUNCTION(this << zone_name << TTL << ns_class << type << rData);
	m_nsCache.AddZone(zone_name, ns_class, type, TTL, rData);
}

void IspServer::StartApplication(void) {
	NS_LOG_FUNCTION(this);
	// Start expiration of the DNS records after TTL values.
	m_nsCache.SynchronizeTTL();
}

void IspServer::StopApplication() {
	NS_LOG_FUNCTION(this);

	DoDispose();
}

void IspServer::SetReplyCallback(Callback<void, Ptr<Packet>, Address> sendData) {
	m_replyQuery = sendData;
}

void IspServer::IspServerService(Ptr<Packet> nsQuery, Address toAddress) {
	DNSHeader DnsHeader;
	uint16_t qType, qClass;
	std::string qName;
	bool foundInCache = false, foundAuthRecordinCache = false;

	nsQuery->RemoveHeader(DnsHeader);

	// Assume that only one question is attached to the DNS header
	std::list<QuestionSectionHeader> questionList;
	questionList = DnsHeader.GetQuestionList();

	qName = questionList.begin()->GetqName();
	qType = questionList.begin()->GetqType();
	qClass = questionList.begin()->GetqClass();
	NS_LOG_INFO("Receive NS Query for: "<<qName);
	// Find for a record that exactly matches the query name.
	// if the query is exactly matches for a record in the ISP cache,
	// the record is treated as a Authoritative record for the client.
	// In case the ISP name server could not find a auth. record for the query,
	// the ISP name server returns IP addresses of the Authoritative name servers
	// for the requested name.
	// To make the load distribution, we assumed the RR implementation for authoritative records.
	SRVTable::SRVRecordI foundAuthRecord = m_nsCache.FindARecord(qName,
			foundAuthRecordinCache);

	// Return the first record that matches the requested qName
	// This supports the RR method
	SRVTable::SRVRecordI cachedRecord = m_nsCache.FindARecordMatches(qName,
			foundInCache);

	Ptr<Packet> ispResponse = Create<Packet>();

	if (foundAuthRecordinCache) {
		// Move the existing answer list to the Additional section.
		// 	This feature is implemented to track the recursive operation and
		// 	thus for debugging purposes.
		// Assume that only one question is attached to the DNS header
		NS_LOG_INFO(
				"Move the Existing recursive answer list in to additional section.");
		std::list<ResourceRecordHeader> answerList;
		answerList = DnsHeader.GetAnswerList();

		for (std::list<ResourceRecordHeader>::iterator iter =
				answerList.begin(); iter != answerList.end(); iter++) {
			ResourceRecordHeader additionalRecord;

			additionalRecord.SetName(iter->GetName());
			additionalRecord.SetClass(iter->GetClass());
			additionalRecord.SetType(iter->GetType());
			additionalRecord.SetTimeToLive(iter->GetTimeToLive());
			additionalRecord.SetRData(iter->GetRData());

			DnsHeader.AddARecord(additionalRecord);
		}
		// Clear the existing answer list
		DnsHeader.ClearAnswers();

		ResourceRecordHeader rrHeader;

		rrHeader.SetName(foundAuthRecord->first->GetRecordName());
		rrHeader.SetClass(foundAuthRecord->first->GetClass());
		rrHeader.SetType(foundAuthRecord->first->GetType());
		rrHeader.SetTimeToLive(foundAuthRecord->first->GetTTL());
		rrHeader.SetRData(foundAuthRecord->first->GetRData());

		DnsHeader.SetQRbit(0);
		DnsHeader.ResetOpcode();
		DnsHeader.SetOpcode(5);
		DnsHeader.SetAAbit(1);
		DnsHeader.AddAnswer(rrHeader);

		ispResponse->AddHeader(DnsHeader);

		m_replyQuery(ispResponse, toAddress);

		// Change the order of server according to the round robin algorithm
		m_nsCache.SwitchServersRoundRobin();
	} else if (foundInCache) {
		ResourceRecordHeader rrHeader;

		rrHeader.SetName(cachedRecord->first->GetRecordName());
		rrHeader.SetClass(cachedRecord->first->GetClass());
		rrHeader.SetType(cachedRecord->first->GetType());
		rrHeader.SetTimeToLive(cachedRecord->first->GetTTL());
		rrHeader.SetRData(cachedRecord->first->GetRData());

		DnsHeader.SetQRbit(0);
		DnsHeader.ResetOpcode();
		DnsHeader.SetOpcode(5);
		DnsHeader.AddAnswer(rrHeader);

		ispResponse->AddHeader(DnsHeader);

		m_replyQuery(ispResponse, toAddress);
	} else {
		//TODO
		// Send a reply contains RCODE = 3
	}

}
}
