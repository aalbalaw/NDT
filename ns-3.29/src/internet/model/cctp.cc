#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/string.h"
#include "cctp.h"
#include "ns3/itp-header.h"
#include "ipv4.h"
#include "ns3/mytag.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/pointer.h"
#include "dns-header.h"
#include "ns3/encode-decode.h"
#include "ns3/manifest-header.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("CCTP");

NS_OBJECT_ENSURE_REGISTERED(CCTP);

TypeId CCTP::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::CCTP").SetParent<Socket>().SetGroupName(
			"Internet").AddConstructor<CCTP>().AddAttribute("consumer",
			"Consumer", PointerValue(), MakePointerAccessor(&CCTP::m_consumer),
			MakePointerChecker<Object>()).AddTraceSource("CongestionWindow",
			"The ITP connection's congestion window",
			MakeTraceSourceAccessor(&CCTP::m_cWndTrace),
			"ns3::TracedValueCallback::Uint32").AddAttribute("EnableDNS",
			"Enable DNS at this application", BooleanValue(true),
			MakeBooleanAccessor(&CCTP::m_enableDNS), MakeBooleanChecker());
	;
	return tid;
}

CCTP::CCTP() {
	NS_LOG_FUNCTION(this);
	m_socket = 0;
	m_producer = CreateObject<producer>();
	m_consumer = CreateObject<consumer>();
	m_resolvedURL = false;
	m_contentSize=0;
}

CCTP::~CCTP() {
	NS_LOG_FUNCTION(this);
	// m_socket = 0;
}

void CCTP::CreateSocket(uint16_t port, Ptr<Node> node) {

	m_logNormal = CreateObject<
			LogNormalRandomVariable>();
	m_logNormal->SetAttribute("Mu", DoubleValue(8.91365));
	m_logNormal->SetAttribute("Sigma", DoubleValue(1.24816));

	m_mainObjectSizeStream = CreateObject<
			WeibullRandomVariable>();
	m_mainObjectSizeStream->SetAttribute("Scale", DoubleValue(19104.9));
	m_mainObjectSizeStream->SetAttribute("Shape", DoubleValue(0.771807));
	m_numberOfInlineObjects=0;

	m_node = node;
	NS_LOG_INFO(m_node->GetId());

	if (m_socket == 0) {
		TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket(node, tid);
		InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(),
				port);
		m_socket->Bind(local);

	}
	m_socket->SetRecvCallback(MakeCallback(&CCTP::ReceveFrom, this));
	m_producer->SetSendCallback(MakeCallback(&CCTP::SendTo, this));
	m_consumer->SetSendCallback(MakeCallback(&CCTP::SendTo, this));
	m_consumer->SetRecvCallback(MakeCallback(&CCTP::ReceivedContent, this));
	m_consumer->SetNodeId(m_node->GetId());

	if (m_dnsSocket == 0) {
		TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
		m_dnsSocket = Socket::CreateSocket(node, tid);
		m_dnsSocket->Bind();
		m_dnsSocket->SetRecvCallback(MakeCallback(&CCTP::ResolveDns, this));
	}
}

void CCTP::CloseSocket() {
	NS_LOG_FUNCTION(this<<m_node->GetId());
	if (m_socket != 0) {
		m_socket->Close();
		m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
		m_socket = 0;
	}
	m_consumer->StopConsumer();
}

void CCTP::SetRecvCallback(Callback<void, Address> receivedData) {
	m_receiveData = receivedData;
}

void CCTP::SendContent(Ptr<Packet> p, InetSocketAddress to) {
	m_producer->ReceiveContent(p->GetSize(), to);
}

void CCTP::SendContent(Ptr<Packet> p, Address to) {
	m_producer->ReceiveContent(p->GetSize(), to);
}

void CCTP::ReceivedContent(Address from) {
	m_receiveData(from);
}

void CCTP::SendTo(Ptr<Packet> p, Address to) {
	Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	Ipv4Address addr = ipv4->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	ItpHeader rcvHdr;
	p->PeekHeader(rcvHdr);
	if (rcvHdr.GetType() == 1) {
		NS_LOG_INFO(
				"Sending Manifest from: "<<addr<<" to: "<<InetSocketAddress::ConvertFrom (to).GetIpv4 ());
	} else if (rcvHdr.GetType() == 2) {
//		NS_LOG_INFO(
//				"Sending Interest from:" <<addr<<" to: "<<InetSocketAddress::ConvertFrom (to).GetIpv4 ());
	} else if (rcvHdr.GetType() == 5) {
		NS_LOG_INFO(
				"Force Sending Data from: "<<addr<<" to: "<<InetSocketAddress::ConvertFrom (to).GetIpv4 ());
	} else {
//		NS_LOG_INFO(
//				"Sending Data from: "<<addr<<" to: "<<InetSocketAddress::ConvertFrom (to).GetIpv4 ());
	}

	MyTag tagg;
	tagg.SetFlag();
	p->AddPacketTag(tagg);

	m_socket->SendTo(p, 0, to);
}

void CCTP::ReceveFrom(Ptr<Socket> socket) {
	Ptr<Packet> packet;
	ItpHeader rcvHdr;

	Address from;
	Address localAddress;
	while ((packet = socket->RecvFrom(from))) {
		Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4>(); // Get Ipv4 instance of the node
		Ipv4Address addr = ipv4->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.
		packet->PeekHeader(rcvHdr);
		switch (rcvHdr.GetType()) {
		case 1:
			m_consumer->OnManifest(packet, from);
			break;
		case 2:
			m_producer->OnInterest(packet, from);
			break;
		case 3:
		case 4:
			m_consumer->OnData(packet, from);
			break;
		case 5:
			packet->RemoveHeader(rcvHdr);
			m_receivedContent(packet, from);

		}
	}
}

void CCTP::ForceSend(Ptr<Packet> p, InetSocketAddress to) {
	ItpHeader sndHdr;
	sndHdr.SetType(5);
	p->AddHeader(sndHdr);
	SendTo(p, to);

}

void CCTP::SetRecvContentCallback(
		Callback<void, Ptr<Packet>, Address> receivedData) {
	m_receivedContent = receivedData;
}

void CCTP::GetContentByName(std::string url, Address localDnsAddress) {

	m_localDnsAddress = localDnsAddress;
	if (url == "main/object") {
		m_url = "main/object";
		url = "www.example.com/index.html";
		GetHostByName(url);
	} else {
		m_url = "inline/object";
		for(int i =1;i <=1; i++){
			url = "www.example.com/index1.html";
			GetHostByName(url);
		}
	}

//	m_resolvedURL = false; //testing without dns
//	if (m_resolvedURL) {
//		OnManifestRecord(m_mHdr);
//	} else {
//		url = "www.example.com/index.html";
//		GetHostByName(url);
//	}
}
void CCTP::GetHostByName(std::string url) {
	if (m_enableDNS) {
		NS_LOG_INFO("DNS is enabled. Resolving URL: "<<url);
		Ptr<Packet> requestRR = Create<Packet>();
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
		nsQuery.SetqName(url);
		nsQuery.SetqType(2020);
		nsQuery.SetqClass(1);

		nsHeader.AddQuestion(nsQuery);
		requestRR->AddHeader(nsHeader);
		m_dnsSocket->SendTo(requestRR, 0,
				InetSocketAddress(Ipv4Address::ConvertFrom(m_localDnsAddress),
						53));
	}

}

void CCTP::OnManifestRecord(ManifestHeader mHdr) {

//	std::vector<std::vector<int>> vec{ { 2073 },
//	                         { 10762,33375 },
//	                         { 16396,6646,3982 },
//							 { 119538,2471,15290,36732 },
//							 { 14781,3998,3011,1753,1372 }};

	std::vector<std::vector<int>> vec{ { 500 },
	                         { 500,500 },
	                         { 500,500,500 },
							 { 500,500,500,500 },
							 { 500,500,500,500 }};


	//Only used with HM
//	int scenario = 0;
//	if (m_url == "main/object") {
//		m_contentSize=0;
//		for(int i=0;i<vec[scenario].size();i++){
//			m_contentSize+=vec[scenario][i];
//		}
//		mHdr.SetName(Hash32("Content " + std::to_string(0)));
//		mHdr.SetNumberOfChunks(ceil(m_contentSize / 500.0));
//		NS_LOG_INFO("Content Size: "<<m_contentSize);
//		NS_LOG_INFO("Number of chunks: "<<ceil(m_contentSize/500.0));
//		mHdr.SetServerAddress(Ipv4Address("10.0.3.2")); //add server address to test without dns
//		m_consumer->OnManifest(mHdr);
//	}


	//Only used when we aren't using hierarchical manifest
	//All inline objects are used in a single manifest
	int scenario = 1;
	if (m_url == "main/object") {
		mHdr.SetName(Hash32("Content "+std::to_string(0)));
		mHdr.SetNumberOfChunks(ceil(vec[scenario][0]/500.0));
		NS_LOG_INFO("Content Size: "<<vec[scenario][0]);
		NS_LOG_INFO("Number of chunks: "<<ceil(vec[scenario][0]/500.0));
		mHdr.SetServerAddress(Ipv4Address("10.0.3.2")); //add server address to test without dns
		m_consumer->OnManifest(mHdr);
	} else {

		m_contentSize=0;
		for(int i=1;i<vec[scenario].size();i++){
			m_contentSize+=vec[scenario][i];
		}
		mHdr.SetName(Hash32("Content "+std::to_string(m_contentSize)));
		mHdr.SetNumberOfChunks(ceil(m_contentSize/500.0));
		NS_LOG_INFO("Content Size: "<<m_contentSize);
		NS_LOG_INFO("Number of chunks: "<<ceil(m_contentSize/500.0));
		mHdr.SetServerAddress(Ipv4Address("10.0.3.2")); //add server address to test without dns
		m_consumer->OnManifest(mHdr);
	}

}

void CCTP::ResolveDns(Ptr<Socket> dnsSocket) {
	Ptr<Packet> packet;
	Address from;
	while ((packet = dnsSocket->RecvFrom(from))) {
		if (InetSocketAddress::IsMatchingType(from)) {
			NS_LOG_INFO(
					"At time " << Simulator::Now ().GetSeconds () << "ITP received dns response from " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());
		}


		DNSHeader DnsHeader;
		packet->RemoveHeader(DnsHeader);
		std::string rData;
		std::list<ResourceRecordHeader> answerList;
		answerList = DnsHeader.GetAnswerList();
		rData = answerList.begin()->GetRData().c_str();
		if (answerList.begin()->GetType() == 2020) {
			NS_LOG_INFO("Received Manifest Record");
			ManifestHeader manifest;
			StringToManifest(manifest, rData);
			m_mHdr = manifest;
			m_resolvedURL = true;

			if(m_url=="inline/object"){
				static int x =0;
				x++;
				if(x==1){
					OnManifestRecord(manifest);
				}
			}else{
				OnManifestRecord(manifest);
			}

		} else {
			NS_LOG_INFO("Type 1 record");
			m_conetntServerAddress = Ipv4Address(rData.c_str());
			Ptr<Packet> p;
			p = Create<Packet>(500);
			InetSocketAddress remote = InetSocketAddress(
					Ipv4Address::ConvertFrom(m_conetntServerAddress), 81);
			ForceSend(p, remote);
		}
	}
}

void CCTP::PopoulateConsumers(std::vector<Ptr<consumer>>& consumers){

	for(int i=0;i<consumers.size();i++){
		consumers[i] = CreateObject<consumer>();
		consumers[i]->SetSendCallback(MakeCallback(&CCTP::SendTo, this));
		consumers[i]->SetRecvCallback(MakeCallback(&CCTP::ReceivedContent, this));
		consumers[i]->SetNodeId(m_node->GetId());
	}
}

void CCTP::OnData(Ptr<Packet> pkt, Address from){

	ItpHeader rcvHdr;
	pkt->PeekHeader(rcvHdr);
	for(int i=0;i<m_listOfConsumers.size();i++){
		if(rcvHdr.GetName()==m_listOfConsumers[i]->m_contentName){
			//NS_LOG_INFO("This Interest belongs to consumer "<<i);
			m_listOfConsumers[i]->OnData(pkt, from);
			break;
		}
	}
}
}
;
