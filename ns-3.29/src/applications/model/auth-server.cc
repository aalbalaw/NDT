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
#include "ns3/auth-server.h"
#include "ns3/dns.h"
#include "ns3/dns-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AuthServer");
NS_OBJECT_ENSURE_REGISTERED(AuthServer);
TypeId AuthServer::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::AuthServer").SetParent<Object>().SetGroupName(
					"Application").AddConstructor<AuthServer>();
	return tid;
}

AuthServer::AuthServer(void) {
	/* cstrctr */
}
AuthServer::~AuthServer() {
	/* dstrctr */
}

void AuthServer::AddZone(std::string zone_name, uint16_t ns_class,
		uint16_t type, uint32_t TTL, std::string rData) {
	NS_LOG_FUNCTION(this << zone_name << ns_class << type << TTL <<rData);
	m_nsCache.AddZone(zone_name, ns_class, type, TTL, rData);
}

void AuthServer::AddManifestRecord(std::string content_name, uint16_t ns_class,
		uint16_t type, uint32_t TTL, std::string manifestRecord) {
	NS_LOG_FUNCTION(
			this << content_name << ns_class << type << TTL <<manifestRecord);
	m_mrCache.AddZone(content_name, ns_class, type, TTL, manifestRecord);
}

void AuthServer::StartApplication(void) {
	NS_LOG_FUNCTION(this);
	// Start expiration of the DNS records after TTL values.
	m_nsCache.SynchronizeTTL();
}

void AuthServer::StopApplication() {
	NS_LOG_FUNCTION(this);

	DoDispose();
}

void AuthServer::SetReplyCallback(
		Callback<void, Ptr<Packet>, Address> sendData) {
	m_replyQuery = sendData;
}

void AuthServer::AuthServerService(Ptr<Packet> nsQuery, Address toAddress) {
	DNSHeader DnsHeader;
	uint16_t qType;
	nsQuery->PeekHeader(DnsHeader);

	// Assume that only one question is attached to the DNS header
	std::list<QuestionSectionHeader> questionList;
	questionList = DnsHeader.GetQuestionList();
	if (questionList.begin()->GetqType() == 2020) {
		HandleManifestQuery(nsQuery, toAddress);
	} else {
		HandleDNSQuery(nsQuery, toAddress);
	}
}

void AuthServer::HandleDNSQuery(Ptr<Packet> nsQuery, Address toAddress) {
	DNSHeader DnsHeader;
	uint16_t qType, qClass;
	std::string qName;
	bool foundInCache = false;			//, foundAARecords = false;

	nsQuery->RemoveHeader(DnsHeader);

	// Assume that only one question is attached to the DNS header
	std::list<QuestionSectionHeader> questionList;
	questionList = DnsHeader.GetQuestionList();

	qName = questionList.begin()->GetqName();
	qType = questionList.begin()->GetqType();
	qClass = questionList.begin()->GetqClass();
	NS_LOG_INFO("Receive NS Query for: "<<qName);
	NS_UNUSED(foundInCache);

	SRVTable::SRVRecordInstance instance;
	// Find the query in the nameserver cache
	//foundInCache = m_nsCache.FindRecordsFor (qName, instance);
	foundInCache = m_nsCache.FindAllRecordsHas(qName, instance);

	if (foundInCache) {
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
			NS_LOG_INFO(
					"Adding things: "<<"Name: "<<iter->GetName()<<" "<<iter->GetRData());
			DnsHeader.AddARecord(additionalRecord);
		}
		// Clear the existing answer list
		DnsHeader.ClearAnswers();

		// Now, add the server list as the new answer list
		NS_LOG_INFO(
				"Add the content server list as the new answer list of the DNS header.");
		// Create the response
		Ptr<Packet> authResponse = Create<Packet>();

		// Get the found record list and add the records to the DNS header according to the Type
		for (SRVTable::SRVRecordI it = instance.begin(); it != instance.end();
				it++) {
			NS_LOG_INFO(
					"Name: "<<it->first->GetRecordName()<<" type: "<<it->first->GetType());
			// Assume that Number of DNS records will note results packet segmentation
			if (it->first->GetType() == 1)	// A host record or a CNAME record
					{
				ResourceRecordHeader rrHeader;

				rrHeader.SetName(it->first->GetRecordName());
				rrHeader.SetClass(it->first->GetClass());
				rrHeader.SetType(it->first->GetType());
				rrHeader.SetTimeToLive(it->first->GetTTL());
				rrHeader.SetRData(it->first->GetRData());
				NS_LOG_INFO(
						"Type 1: "<<"Name: "<<it->first->GetRecordName()<<" "<<it->first->GetRData());
				DnsHeader.AddAnswer(rrHeader);
			}
			if (it->first->GetType() == 2) // A Authoritative Name server record
					{
				ResourceRecordHeader nsRecord;

				nsRecord.SetName(it->first->GetRecordName());
				nsRecord.SetClass(it->first->GetClass());
				nsRecord.SetType(it->first->GetType());
				nsRecord.SetTimeToLive(it->first->GetTTL());
				nsRecord.SetRData(it->first->GetRData());
				NS_LOG_INFO("Here 2");
				DnsHeader.AddNsRecord(nsRecord);
			}
			if (it->first->GetType() == 5) // A Authoritative Name server record
					{
				ResourceRecordHeader rrRecord;

				rrRecord.SetName(it->first->GetRecordName());
				rrRecord.SetClass(it->first->GetClass());
				rrRecord.SetType(it->first->GetType());
				rrRecord.SetTimeToLive(it->first->GetTTL());
				rrRecord.SetRData(it->first->GetRData());
				NS_LOG_INFO("Here 5");
				DnsHeader.AddNsRecord(rrRecord);
			}
		}

		DnsHeader.SetQRbit(0);
		DnsHeader.SetAAbit(1);
		DnsHeader.ResetOpcode();
		DnsHeader.SetOpcode(6);

		authResponse->AddHeader(DnsHeader);
		m_replyQuery(authResponse, toAddress);

		// Change the order of server according to the round robin algorithm
		m_nsCache.SwitchServersRoundRobin();
	} else {
		// TODO
		// Send a reply contains RCODE = 3
	}
}

void AuthServer::HandleManifestQuery(Ptr<Packet> nsQuery, Address toAddress) {
	DNSHeader DnsHeader;
	uint16_t qType, qClass;
	std::string qName;
	bool foundInCache = false;			//, foundAARecords = false;

	nsQuery->RemoveHeader(DnsHeader);

	// Assume that only one question is attached to the DNS header
	std::list<QuestionSectionHeader> questionList;
	questionList = DnsHeader.GetQuestionList();

	qName = questionList.begin()->GetqName();
	qType = questionList.begin()->GetqType();
	qClass = questionList.begin()->GetqClass();
	NS_LOG_INFO("Receive NS Query for: "<<qName);
	NS_UNUSED(foundInCache);

	SRVTable::SRVRecordInstance instance;
	NS_LOG_INFO("Received Manifest Interest for: "<<qName);
	SRVTable::SRVRecordI cachedRecord = m_mrCache.FindARecordHas(qName,
			foundInCache);
	if (foundInCache) {
		NS_LOG_INFO("Found a Manifest record in the local cache. Replying..");

		Ptr<Packet> dnsResponse = Create<Packet>();
		ResourceRecordHeader answer;
		answer.SetName(cachedRecord->first->GetRecordName());
		answer.SetClass(cachedRecord->first->GetClass());
		answer.SetType(cachedRecord->first->GetType());
		answer.SetTimeToLive(25);	//cachedRecord->first->GetTTL ());
		answer.SetRData(cachedRecord->first->GetRData());

		DnsHeader.AddAnswer(answer);
		DnsHeader.SetRAbit(0);
		DnsHeader.ResetOpcode();
		DnsHeader.SetOpcode(7);
		dnsResponse->AddHeader(DnsHeader);

		m_replyQuery(dnsResponse, toAddress);
		return;

	} else {
		NS_LOG_INFO("ERROR: Manifest Record is not in cache. return");
		return;
	}
}
}
