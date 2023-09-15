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
#include "ns3/bind-server.h"
#include "ns3/dns.h"
#include "ns3/dns-header.h"
#include "ns3/encode-decode.h"
#include "ns3/manifest-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BindServer");
NS_OBJECT_ENSURE_REGISTERED(BindServer);
TypeId BindServer::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::BindServer").SetParent<Application>().AddConstructor<
					BindServer>().AddAttribute("SetServerAddress",
					"IP address of the server.", Ipv4AddressValue(),
					MakeIpv4AddressAccessor(&BindServer::m_localAddress),
					MakeIpv4AddressChecker()).AddAttribute("RootServerAddress",
					"Ip address of the Root Server (This is only needed for Local NS).",
					Ipv4AddressValue(),
					MakeIpv4AddressAccessor(&BindServer::m_rootAddress),
					MakeIpv4AddressChecker()).AddAttribute("SetNetMask",
					"Network Mask of the server.", Ipv4MaskValue(),
					MakeIpv4MaskAccessor(&BindServer::m_netMask),
					MakeIpv4MaskChecker()).AddAttribute("NameServerType",
					" Type of the name server.", EnumValue(AUTH_SERVER),
					MakeEnumAccessor(&BindServer::m_serverType),
					MakeEnumChecker(LOCAL_SERVER, "LOCAL NAME SERVER",
							ROOT_SERVER, "ROOT NAME SERVER", TLD_SERVER,
							"TOP-LEVEL DOMAIN SERVER", ISP_SERVER,
							"ISP'S NAME SERVER", AUTH_SERVER,
							"AUTHORITATIVE NAME SERVER")).AddAttribute(
					"SetRecursiveSupport",
					"Set the name server support recursive IP resolution",
					EnumValue(BindServer::RA_UNAVAILABLE),
					MakeEnumAccessor(&BindServer::m_raType),
					MakeEnumChecker(BindServer::RA_UNAVAILABLE,
							"Does not support", BindServer::RA_AVAILABLE,
							"Support"));
	return tid;
}

BindServer::BindServer(void) {
	m_localAddress = Ipv4Address();
	m_netMask = Ipv4Mask();
	m_socket = 0;
	m_rootServer = CreateObject<RootServer>();
	m_tldServer = CreateObject<TldServer>();
	m_ispServer = CreateObject<IspServer>();
	m_authServer = CreateObject<AuthServer>();
	/* cstrctr */
}
BindServer::~BindServer() {
	/* dstrctr */
}

void BindServer::AddZone(std::string zone_name, uint16_t ns_class,
		uint16_t type, uint32_t TTL, std::string rData) {

	NS_LOG_FUNCTION(this << zone_name << ns_class << type <<TTL<<rData);
	if (m_serverType == ROOT_SERVER) {
		m_rootServer->AddZone(zone_name, ns_class, type, TTL, rData);
	} else if (m_serverType == TLD_SERVER) {
		m_tldServer->AddZone(zone_name, ns_class, type, TTL, rData);
	} else if (m_serverType == ISP_SERVER) {
		m_ispServer->AddZone(zone_name, ns_class, type, TTL, rData);
	} else if (m_serverType == AUTH_SERVER) {
		m_authServer->AddZone(zone_name, ns_class, type, TTL, rData);
	} else if (m_serverType == LOCAL_SERVER) {
		m_nsCache.AddZone(zone_name, ns_class, type, TTL, rData);
	}
}

void BindServer::AddManifestRecord(std::string content_name, uint16_t ns_class,
		uint16_t type, uint32_t TTL, std::string manifestRecord) {
//	NS_LOG_FUNCTION(
//			this << content_name << ns_class << type <<TTL << manifestRecord);
	if (m_serverType == AUTH_SERVER) {
		m_authServer->AddManifestRecord(content_name, ns_class, type, TTL,
				manifestRecord);
	} else if (m_serverType == LOCAL_SERVER) {
		m_mrCache.AddZone(content_name, ns_class, type, TTL, manifestRecord);
	}
}

void BindServer::StartApplication(void) {
	//NS_LOG_FUNCTION(this);
	// Start expiration of the DNS records after TTL values.
	m_nsCache.SynchronizeTTL();

	if (m_socket == 0) {
		TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket(GetNode(), tid);
		InetSocketAddress local = InetSocketAddress(m_localAddress, DNS_PORT);
		m_socket->Bind(local);
	}
	m_socket->SetRecvCallback(MakeCallback(&BindServer::HandleQuery, this));
	if (m_serverType == ROOT_SERVER) {
		m_rootServer->SetReplyCallback(
				MakeCallback(&BindServer::ReplyQuery, this));
	} else if (m_serverType == TLD_SERVER) {
		m_tldServer->SetReplyCallback(
				MakeCallback(&BindServer::ReplyQuery, this));
	} else if (m_serverType == ISP_SERVER) {
		m_ispServer->SetReplyCallback(
				MakeCallback(&BindServer::ReplyQuery, this));
	} else if (m_serverType == AUTH_SERVER) {
		m_authServer->SetReplyCallback(
				MakeCallback(&BindServer::ReplyQuery, this));
	}
}

void BindServer::StopApplication() {
	NS_LOG_FUNCTION(this);

	if (m_socket != 0) {
		m_socket->Close();
		m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
	}
	DoDispose();
}

void BindServer::HandleQuery(Ptr<Socket> socket) {

	Ptr<Packet> message;
	Address from;

	while ((message = socket->RecvFrom(from))) {
		if (InetSocketAddress::IsMatchingType(from)) {

		}

		message->RemoveAllPacketTags();
		message->RemoveAllByteTags();

		if (m_serverType == LOCAL_SERVER) {
			LocalServerService(message, from);
		} else if (m_serverType == ROOT_SERVER) {
			m_rootServer->RootServerService(message, from);
		} else if (m_serverType == TLD_SERVER) {
			m_tldServer->TLDServerService(message, from);
		} else if (m_serverType == ISP_SERVER) {
			m_ispServer->IspServerService(message, from);
		} else if (m_serverType == AUTH_SERVER) {
			m_authServer->AuthServerService(message, from);
		} else {
			NS_ABORT_MSG(
					"Name server should have a type. Hint: Set NameserverType. Aborting");
		}
	}
}

void BindServer::LocalServerService(Ptr<Packet> nsQuery, Address toAddress) {
	DNSHeader DnsHeader;
	uint16_t qType;
	bool foundInCache = false;
	bool nsQuestion = false;

	nsQuery->PeekHeader(DnsHeader);

	if ((nsQuestion = DnsHeader.GetQRbit())) // if NS query
	{
		// retrieve the question list
		std::list<QuestionSectionHeader> questionList;
		questionList = DnsHeader.GetQuestionList();
		qType = questionList.begin()->GetqType();

		if (qType == 1) {
			HandleDNSQuery(nsQuery, toAddress);
		} else if (qType == 2020) {
			HandleManifestQuery(nsQuery, toAddress);
		}

	} else { // // NS response !rHeader.GetQRbit ()

		// NOTE
		//	In this implementation, the remaining bits of the OPCODE are used to specify the reply types.
		//	3 for the replies from a ROOT server
		//	4 for the replies from a TLD server
		//	5 for the replies from a ISP's name server
		//	6 for the replies from a Authoritative name server (in this case the AA bit is also considered)
		//	7 for ManifestRecord from Authoritative name server

		// All answers are append to the DNS header.
		// However, only the relevant answer is taken according to the OPCODE value.
		// Furthermore, we implemented the servers to add the resource record to the top of the answer section.

		if (DnsHeader.GetOpcode() == 3) {
			HandleRootServerReply(nsQuery, toAddress);
		} else if (DnsHeader.GetOpcode() == 4) {
			HandleTLDServerReply(nsQuery, toAddress);
		} else if (DnsHeader.GetOpcode() == 5) {
			HandleISPServerReply(nsQuery, toAddress);
		} else if (DnsHeader.GetOpcode() == 6 || DnsHeader.GetOpcode() == 7) {
			HandleAuthServerReply(nsQuery, toAddress);
		} else {
			NS_LOG_INFO("ERROR: Unknown upcode");
		}
	}
}

void BindServer::HandleDNSQuery(Ptr<Packet> nsQuery, Address toAddress) {

	DNSHeader DnsHeader;
	uint16_t qType, qClass;
	std::string qName;
	bool foundInCache = false;
	bool nsQuestion = false;

	nsQuery->RemoveHeader(DnsHeader);
	// retrieve the question list
	std::list<QuestionSectionHeader> questionList;
	questionList = DnsHeader.GetQuestionList();
	NS_LOG_INFO("DNS query for RR: "<<questionList.begin()->GetqName());
	// Although the header supports multiple questions at a time
	// the local DNS server is not yet implemented to resolve multiple questions at a time.
	// We assume that clients generate separate DNS messages for each
	// host that they wanted to resolve.

	// NOTE
	//	We assumed that the local DNS does the recursive resolution process (i.e., in CDN networks)

	qName = questionList.begin()->GetqName();
	qType = questionList.begin()->GetqType();
	qClass = questionList.begin()->GetqClass();

	SRVTable::SRVRecordI cachedRecord = m_nsCache.FindARecordHas(qName,
			foundInCache);

	if (foundInCache) {
		NS_LOG_INFO(
				"Sending RR: "<< cachedRecord->first->GetRecordName()<<" : "<<cachedRecord->first->GetRData());

		// Local Server always returns the server address according to the RR manner.
		Ptr<Packet> dnsResponse = Create<Packet>();

		ResourceRecordHeader answer;
		answer.SetName(cachedRecord->first->GetRecordName());
		answer.SetClass(cachedRecord->first->GetClass());
		answer.SetType(cachedRecord->first->GetType());
		answer.SetTimeToLive(25);	//cachedRecord->first->GetTTL ());
		answer.SetRData(cachedRecord->first->GetRData());

		DnsHeader.AddAnswer(answer);
		DnsHeader.SetRAbit(1);
		dnsResponse->AddHeader(DnsHeader);
		ReplyQuery(dnsResponse, toAddress);

		m_nsCache.SwitchServersRoundRobin();
		return;
	} else if (!foundInCache && (m_raType == RA_AVAILABLE)) {

		Ptr<Packet> requestRR = Create<Packet>();
		std::string tld;
		std::string::size_type found = 0;
		bool foundTLDinCache = false;

		// find the TLD of the query
		found = qName.find_last_of('.');
		tld = qName.substr(found);
		//Find the TLD in the nameserver cache
		SRVTable::SRVRecordI cachedTLDRecord = m_nsCache.FindARecord(tld,
				foundTLDinCache);

		requestRR->AddHeader(DnsHeader);

		//Add the recursive request in to the list
		m_recursiveQueryList[qName] = toAddress;

		if (foundTLDinCache) {
			NS_LOG_INFO(
					"Send RR Query to: "<<tld<<" : "<<cachedTLDRecord->first->GetRData());
			SendQuery(requestRR,
					InetSocketAddress(
							Ipv4Address(
									cachedTLDRecord->first->GetRData().c_str()),
							DNS_PORT));
		} else {
			NS_LOG_INFO(
					"Send RR Query to root at: "<<m_rootAddress<<" : "<<questionList.begin()->GetqName());
			SendQuery(requestRR, InetSocketAddress(m_rootAddress, DNS_PORT));
		}
		return;
	}
}

void BindServer::HandleManifestQuery(Ptr<Packet> nsQuery, Address toAddress) {
	DNSHeader DnsHeader;
	uint16_t qType, qClass;
	std::string qName;
	bool foundInCache = false;
	bool nsQuestion = false;

	nsQuery->RemoveHeader(DnsHeader);
	// retrieve the question list
	std::list<QuestionSectionHeader> questionList;
	questionList = DnsHeader.GetQuestionList();

	qName = questionList.begin()->GetqName();
	qType = questionList.begin()->GetqType();
	qClass = questionList.begin()->GetqClass();

	NS_LOG_INFO("Received Manifest Interest for "<<qName);
	SRVTable::SRVRecordI cachedRecord = m_mrCache.FindARecordHas(qName,
			foundInCache);
	if (foundInCache) {
		NS_LOG_INFO(
				"Sending ManifestRecord: "<< cachedRecord->first->GetRecordName()<<" : "<<cachedRecord->first->GetRData());

		Ptr<Packet> dnsResponse = Create<Packet>();
		ResourceRecordHeader answer;
		answer.SetName(cachedRecord->first->GetRecordName());
		answer.SetClass(cachedRecord->first->GetClass());
		answer.SetType(cachedRecord->first->GetType());
		answer.SetTimeToLive(25);	//cachedRecord->first->GetTTL ());
		answer.SetRData(cachedRecord->first->GetRData());
		DnsHeader.AddAnswer(answer);
		DnsHeader.SetRAbit(1);
		dnsResponse->AddHeader(DnsHeader);
		ReplyQuery(dnsResponse, toAddress);
		return;
	} else {

		Ptr<Packet> requestRR = Create<Packet>();
		std::string contentName;
		std::string tld;
		std::string hostName;
		std::string::size_type found = 0;
		bool foundAuthinCache = false;
		bool foundTLDinCache = false;
		// find the TLD of the query
		found = qName.find_last_of('/');
		contentName = qName.substr(found);
		hostName = qName.substr(0, found);
		found = hostName.find_last_of('.');
		tld = hostName.substr(found);

		NS_LOG_INFO(
				"Need RR for HostName server: "<< hostName<<" for Manifest: "<<contentName);

		SRVTable::SRVRecordI cachedAuthRecord = m_nsCache.FindARecord(hostName,
				foundAuthinCache);
		if (foundAuthinCache) {

			m_recursiveQueryList[qName] = toAddress;
			m_contentsQueryList[hostName] = contentName;
			NS_LOG_INFO("Adding "<<contentName <<" to "<<hostName);

			//Create DNS header with Manifest Interest type. We should send Manifest query
			DNSHeader nsHeader;
			nsHeader.SetId(1);
			nsHeader.SetQRbit(1);
			nsHeader.SetOpcode(0);
			nsHeader.SetAAbit(0);
			nsHeader.SetTCbit(0);
			nsHeader.SetRDbit(0);
			nsHeader.SetRAbit(0);
			nsHeader.SetRcode(0);
			nsHeader.SetZcode();

			QuestionSectionHeader miQuery;
			miQuery.SetqName(m_contentsQueryList[qName]);
			miQuery.SetqType(2020);
			miQuery.SetqClass(1);
			nsHeader.AddQuestion(miQuery);
			requestRR->AddHeader(nsHeader);

			NS_LOG_INFO(
					"Send Manifest Interest: "<<nsHeader.GetQuestionList().begin()->GetqName()<<" : "
					<<cachedAuthRecord->first->GetRData().c_str());
			SendQuery(requestRR,
					InetSocketAddress(
							Ipv4Address(
									cachedAuthRecord->first->GetRData().c_str()),
							DNS_PORT));
		} else {

			// check if we have hostname in cache, else send query for hostname to tld server.

			DNSHeader nsHeader;
			nsHeader.SetId(1);
			nsHeader.SetQRbit(1);
			nsHeader.SetOpcode(0);
			nsHeader.SetAAbit(0);
			nsHeader.SetTCbit(0);
			nsHeader.SetRDbit(0);
			nsHeader.SetRAbit(0);
			nsHeader.SetRcode(0);
			nsHeader.SetZcode();

			QuestionSectionHeader nsQuery;
			nsQuery.SetqName(hostName);
			nsQuery.SetqType(1);
			nsQuery.SetqClass(1);
			nsHeader.AddQuestion(nsQuery);
			requestRR->AddHeader(nsHeader);

			m_recursiveQueryList[qName] = toAddress;
			m_contentsQueryList[hostName] = contentName;
			NS_LOG_INFO("Adding "<<contentName <<" to "<<hostName);

			SRVTable::SRVRecordI cachedTLDRecord = m_nsCache.FindARecord(tld,
					foundTLDinCache);
			if (foundTLDinCache) {
				NS_LOG_INFO(
						"Send RR Query to: "<<tld<<" : "<<cachedTLDRecord->first->GetRData());
				SendQuery(requestRR,
						InetSocketAddress(
								Ipv4Address(
										cachedTLDRecord->first->GetRData().c_str()),
								DNS_PORT));
			} else {
				NS_LOG_INFO(
						"Send RR Query to root at: "<<m_rootAddress<<" : "<<hostName);
				SendQuery(requestRR,
						InetSocketAddress(m_rootAddress, DNS_PORT));
			}
			return;

		}
	}
	//Search for manifest record that mataches the name.
	//hard code a manifest, serlize it, add it to the dns record and send it.
}

void BindServer::HandleRootServerReply(Ptr<Packet> nsReply, Address toAddress) {

	DNSHeader DnsHeader;
	std::string qName;
	nsReply->RemoveHeader(DnsHeader);

	std::string forwardingAddress;

// retrieve the Answer list
	std::list<ResourceRecordHeader> answerList;
	answerList = DnsHeader.GetAnswerList();

// Always use the most recent answer, so that the previous server is considered.
// However, the answer list contains all answers recursive name servers added.
	qName = answerList.begin()->GetName();
	forwardingAddress = answerList.begin()->GetRData();

	std::string tld;
	std::string::size_type foundAt = 0;
	foundAt = qName.find_last_of('.');
	tld = qName.substr(foundAt);

// add the record about TLD to the Local name server cache
	NS_LOG_INFO("Add TLD RR: "<<tld<<" : "<<answerList.begin()->GetRData());
	m_nsCache.AddRecord(tld, answerList.begin()->GetClass(),
			answerList.begin()->GetType(), answerList.begin()->GetTimeToLive(),
			answerList.begin()->GetRData());

// create a packet to send to TLD
	Ptr<Packet> sendToTLD = Create<Packet>();
	DnsHeader.ResetOpcode();
	DnsHeader.SetOpcode(0);
	DnsHeader.SetQRbit(1);
	sendToTLD->AddHeader(DnsHeader);
	NS_LOG_INFO(
			"Send RR Query: "<<DnsHeader.GetQuestionList().begin()->GetqName()<<" : "<<answerList.begin()->GetRData());
	SendQuery(sendToTLD,
			InetSocketAddress(Ipv4Address(forwardingAddress.c_str()),
			DNS_PORT));

}

void BindServer::HandleTLDServerReply(Ptr<Packet> nsReply, Address toAddress) {

	DNSHeader DnsHeader;
	std::string qName;
	nsReply->RemoveHeader(DnsHeader);

	std::string forwardingAddress;

// retrieve the Answer list
	std::list<ResourceRecordHeader> answerList;
	answerList = DnsHeader.GetAnswerList();

	Ptr<Packet> requestRR = Create<Packet>();
	qName = answerList.begin()->GetName();
	forwardingAddress = answerList.begin()->GetRData();
	//TODO: Add Authorative Server Address to list of servers to contact
	NS_LOG_INFO(
			"AuthServer RR: "<<qName<<" : "<<answerList.begin()->GetRData());

	m_nsCache.AddRecord(qName, answerList.begin()->GetClass(),
			answerList.begin()->GetType(), answerList.begin()->GetTimeToLive(),
			answerList.begin()->GetRData());

	NS_LOG_INFO("Maifest pending "<< m_contentsQueryList.size());
	if (m_contentsQueryList.count(qName)) {
		NS_LOG_INFO(
				"Manifest Record Pending "<<qName<<" : "<<m_contentsQueryList[qName]);
		//Create DNS header with Manifest Interest type. We should send Manifest query
		DNSHeader nsHeader;
		nsHeader.SetId(1);
		nsHeader.SetQRbit(1);
		nsHeader.SetOpcode(0);
		nsHeader.SetAAbit(0);
		nsHeader.SetTCbit(0);
		nsHeader.SetRDbit(0);
		nsHeader.SetRAbit(0);
		nsHeader.SetRcode(0);
		nsHeader.SetZcode();

		QuestionSectionHeader miQuery;
		miQuery.SetqName(m_contentsQueryList[qName]);
		miQuery.SetqType(2020);
		miQuery.SetqClass(1);
		nsHeader.AddQuestion(miQuery);
		requestRR->AddHeader(nsHeader);

		NS_LOG_INFO(
				"Send Manifest Interest: "<<nsHeader.GetQuestionList().begin()->GetqName()<<" : "<<forwardingAddress);
		SendQuery(requestRR,
				InetSocketAddress(Ipv4Address(forwardingAddress.c_str()),
				DNS_PORT));
	} else {

		Ptr<Packet> sendToISP = Create<Packet>();

		DnsHeader.ResetOpcode();
		DnsHeader.SetOpcode(0);
		DnsHeader.SetQRbit(1);
		sendToISP->AddHeader(DnsHeader);
		NS_LOG_INFO(
				"Send RR Query: "<<DnsHeader.GetQuestionList().begin()->GetqName()<<" : "<<forwardingAddress);
		SendQuery(sendToISP,
				InetSocketAddress(Ipv4Address(forwardingAddress.c_str()),
				DNS_PORT));
	}
}

void BindServer::HandleISPServerReply(Ptr<Packet> nsReply, Address toAddress) {
	NS_LOG_FUNCTION(this);

	DNSHeader DnsHeader;
	std::string qName;
	nsReply->RemoveHeader(DnsHeader);

	std::string forwardingAddress;

// retrieve the Answer list
	std::list<ResourceRecordHeader> answerList;
	answerList = DnsHeader.GetAnswerList();
// If the ISP's name server says that it has the authoritative records,
// cache it and pass it to the user.
	if (DnsHeader.GetAAbit()) {
		NS_LOG_INFO("Add the Auth records in to the server cache");

		std::list<ResourceRecordHeader> answerList;
		answerList = DnsHeader.GetAnswerList();

		// Store all answers, i.e., server records, to the Local DNS cache
		for (std::list<ResourceRecordHeader>::iterator iter =
				answerList.begin(); iter != answerList.end(); iter++) {
			m_nsCache.AddRecord(iter->GetName(), iter->GetClass(),
					iter->GetType(),
					/*iter->GetTimeToLive ()*/40, iter->GetRData());
		}
		// Clear the existing answer list
		DnsHeader.ClearAnswers();

		// Get the recent query from the cache.
		//TODO: This approach can be optimized
		bool foundInCache = false;
		SRVTable::SRVRecordI cachedRecord = m_nsCache.FindARecordHas(qName,
				foundInCache);

		// Create the Answer and reply it back to the client

		ResourceRecordHeader answer;

		answer.SetName(cachedRecord->first->GetRecordName());
		answer.SetClass(cachedRecord->first->GetClass());
		answer.SetType(cachedRecord->first->GetType());
		answer.SetTimeToLive(25);//(cachedRecord->first->GetTTL ());		// bypassed for testing purposes
		answer.SetRData(cachedRecord->first->GetRData());

		DnsHeader.AddAnswer(answer);

		// create a packet to send to TLD
		Ptr<Packet> replyToClient = Create<Packet>();

		DnsHeader.ResetOpcode();
		DnsHeader.SetOpcode(0);
		DnsHeader.SetQRbit(0);
		DnsHeader.SetAAbit(1);
		replyToClient->AddHeader(DnsHeader);

		// Find the actual client query that stores in recursive list
		std::list<QuestionSectionHeader> questionList;
		questionList = DnsHeader.GetQuestionList();
		qName = questionList.begin()->GetqName();

		ReplyQuery(replyToClient, m_recursiveQueryList.find(qName)->second);

		m_recursiveQueryList.erase(qName);
		m_nsCache.SwitchServersRoundRobin();
	} else {
		Ptr<Packet> sendToAUTH = Create<Packet>();

		DnsHeader.ResetOpcode();
		DnsHeader.SetOpcode(0);
		DnsHeader.SetQRbit(1);
		sendToAUTH->AddHeader(DnsHeader);

		SendQuery(sendToAUTH,
				InetSocketAddress(Ipv4Address(forwardingAddress.c_str()),
				DNS_PORT));
		NS_LOG_INFO("Contact Authoritative name server");
	}

}

void BindServer::HandleAuthServerReply(Ptr<Packet> reply, Address toAddress) {

	DNSHeader DnsHeader;
	uint16_t qType, qClass;
	std::string qName;
	bool foundInCache = false;
	bool nsQuestion = false;

	reply->PeekHeader(DnsHeader);

	std::string forwardingAddress;

	// retrieve the Answer list
	std::list<ResourceRecordHeader> answerList;
	answerList = DnsHeader.GetAnswerList();

	// Always use the most recent answer, so that the previous server is considered.
	// However, the answer list contains all answers recursive name servers added.
	qName = answerList.begin()->GetName();
	qType = answerList.begin()->GetType();
	qClass = answerList.begin()->GetClass();
	forwardingAddress = answerList.begin()->GetRData();

	if (DnsHeader.GetOpcode() == 6) {

		std::list<ResourceRecordHeader> answerList;
		answerList = DnsHeader.GetAnswerList();

		// Store all answers, i.e., server records, to the Local DNS cache
		for (std::list<ResourceRecordHeader>::iterator iter =
				answerList.begin(); iter != answerList.end(); iter++) {
			NS_LOG_INFO(
					"RR Cache Adding: "<<iter->GetName()<<" "<<iter->GetRData());
			m_nsCache.AddRecord(iter->GetName(), iter->GetClass(),
					iter->GetType(), iter->GetTimeToLive()/*40*/,
					iter->GetRData());
		}
		// Clear the existing answer list
		DnsHeader.ClearAnswers();

		// Get the recent query from the cache.
		// TODO: This approach can be optimized
		bool foundInCache = false;
		SRVTable::SRVRecordI cachedRecord = m_nsCache.FindARecordHas(qName,
				foundInCache);

		NS_LOG_INFO(
				"Sending: "<<cachedRecord->first->GetRecordName()<<" "<<cachedRecord->first->GetRData());
		// Create the Answer and reply it back to the client

		ResourceRecordHeader answer;

		answer.SetName(cachedRecord->first->GetRecordName());
		answer.SetClass(cachedRecord->first->GetClass());
		answer.SetType(cachedRecord->first->GetType());
		answer.SetTimeToLive(25);//(cachedRecord->first->GetTTL ());		// bypassed for testing purposes
		answer.SetRData(cachedRecord->first->GetRData());

		DnsHeader.AddAnswer(answer);

		// create a packet to send to TLD
		Ptr<Packet> replyToClient = Create<Packet>();

		DnsHeader.ResetOpcode();
		DnsHeader.SetOpcode(0);
		DnsHeader.SetQRbit(0);
		DnsHeader.SetAAbit(1);
		replyToClient->AddHeader(DnsHeader);

		// Find the actual client query that stores in recursive list
		std::list<QuestionSectionHeader> questionList;
		questionList = DnsHeader.GetQuestionList();
		qName = questionList.begin()->GetqName();

		ReplyQuery(replyToClient, m_recursiveQueryList.find(qName)->second);

		m_recursiveQueryList.erase(qName);
		m_nsCache.SwitchServersRoundRobin();
	} else if (DnsHeader.GetOpcode() == 7) {

		NS_LOG_INFO("Received Manifest Record: "<<qName);

		std::list<ResourceRecordHeader> answerList;
		answerList = DnsHeader.GetAnswerList();

		// Store all answers, i.e., server records, to the Local DNS cache
		for (std::list<ResourceRecordHeader>::iterator iter =
				answerList.begin(); iter != answerList.end(); iter++) {
			NS_LOG_INFO(
					"Manifest Cache Adding: "<<iter->GetName()<<" "<<iter->GetRData().c_str());
			m_mrCache.AddRecord(iter->GetName(), iter->GetClass(),
					iter->GetType(), iter->GetTimeToLive()/*40*/,
					iter->GetRData());
		}
		// Clear the existing answer list
		DnsHeader.ClearAnswers();
		;
		bool foundInCache = false;
		SRVTable::SRVRecordI cachedRecord = m_mrCache.FindARecordHas(qName,
				foundInCache);
		if (!foundInCache) {
			NS_LOG_INFO(
					"Coudn't Find Manifest Record in Cache "<<cachedRecord->first->GetType());
		}

		// Create the Answer and reply it back to the client

		ResourceRecordHeader answer;
		NS_LOG_INFO(cachedRecord->first->GetRecordName());
		answer.SetName(cachedRecord->first->GetRecordName());
		answer.SetClass(cachedRecord->first->GetClass());
		answer.SetType(cachedRecord->first->GetType());
		answer.SetTimeToLive(25);//(cachedRecord->first->GetTTL ());		// bypassed for testing purposes
		answer.SetRData(cachedRecord->first->GetRData());

		DnsHeader.AddAnswer(answer);

		// create a packet to send to TLD
		Ptr<Packet> replyToClient = Create<Packet>();

		DnsHeader.ResetOpcode();
		DnsHeader.SetOpcode(0);
		DnsHeader.SetQRbit(0);
		DnsHeader.SetAAbit(1);
		replyToClient->AddHeader(DnsHeader);

		NS_LOG_INFO(
				"Sending Manifest Record "<<cachedRecord->first->GetRData().c_str()<<
				" to "<<m_recursiveQueryList.find(qName)->second);
		ReplyQuery(replyToClient, m_recursiveQueryList.find(qName)->second);
		m_recursiveQueryList.erase(qName);

	} else {
		NS_LOG_INFO("ERROR");
	}

}

void BindServer::SendQuery(Ptr<Packet> requestRecord, Address toAddress) {
//	NS_LOG_INFO(
//			"Server " << m_localAddress << " send a reply to " << InetSocketAddress::ConvertFrom (toAddress).GetIpv4 ());
	m_socket->SendTo(requestRecord, 0, toAddress);

}

void BindServer::ReplyQuery(Ptr<Packet> nsQuery, Address toAddress) {
//	NS_LOG_INFO(
//			"Server " << m_localAddress << " send a reply to " << InetSocketAddress::ConvertFrom (toAddress).GetIpv4 ());
	m_socket->SendTo(nsQuery, 0, toAddress);
}

}
