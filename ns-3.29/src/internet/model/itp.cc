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
#include "ns3/itp.h"
#include "ns3/itp-header.h"
#include "ipv4.h"
#include "ns3/mytag.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/pointer.h"
#include "dns-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("itp");

NS_OBJECT_ENSURE_REGISTERED(itp);

TypeId itp::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::itp").SetParent<Socket>().SetGroupName(
			"Internet").AddConstructor<itp>().AddAttribute("consumer",
			"Consumer", PointerValue(), MakePointerAccessor(&itp::m_consumer),
			MakePointerChecker<Object>()).AddTraceSource("CongestionWindow",
			"The ITP connection's congestion window",
			MakeTraceSourceAccessor(&itp::m_cWndTrace),
			"ns3::TracedValueCallback::Uint32");
	;
	return tid;
}

itp::itp() {
	NS_LOG_FUNCTION(this);
	m_socket = 0;
	m_producer = CreateObject<producer>();
	m_consumer = CreateObject<consumer>();
}

itp::~itp() {
	NS_LOG_FUNCTION(this);
	// m_socket = 0;
}

void itp::CreateSocket(uint16_t port, Ptr<Node> node) {
	m_node = node;
	NS_LOG_INFO(m_node->GetId());

	if (m_socket == 0) {
		TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket(node, tid);
		InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(),
				port);
		m_socket->Bind(local);

	}
	m_socket->SetRecvCallback(MakeCallback(&itp::ReceveFrom, this));
	m_producer->SetSendCallback(MakeCallback(&itp::SendTo, this));
	m_consumer->SetSendCallback(MakeCallback(&itp::SendTo, this));
	m_consumer->SetRecvCallback(MakeCallback(&itp::ReceivedContent, this));
	m_consumer->SetNodeId(m_node->GetId());
}

void itp::CloseSocket() {
	NS_LOG_FUNCTION(this<<m_node->GetId());
	if (m_socket != 0) {
		m_socket->Close();
		m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
		m_socket = 0;
	}
	m_consumer->StopConsumer();
}

void itp::SetRecvCallback(Callback<void, Address> receivedData) {
	m_receiveData = receivedData;
}

void itp::SendContent(Ptr<Packet> p, InetSocketAddress to) {
	m_producer->ReceiveContent(p->GetSize(), to);
}

void itp::SendContent(Ptr<Packet> p, Address to) {
	m_producer->ReceiveContent(p->GetSize(), to);
}

void itp::ReceivedContent(Address from) {
	m_receiveData(from);
}

void itp::SendTo(Ptr<Packet> p, Address to) {
	Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	Ipv4Address addr = ipv4->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	ItpHeader rcvHdr;
	p->PeekHeader(rcvHdr);
	if (rcvHdr.GetType() == 1) {
		NS_LOG_INFO(
				"Sending Manifest from: "<<addr<<" to: "<<InetSocketAddress::ConvertFrom (to).GetIpv4 ());
	} else if (rcvHdr.GetType() == 2) {
		NS_LOG_INFO(
				"Sending Interest from:" <<addr<<" to: "<<InetSocketAddress::ConvertFrom (to).GetIpv4 ());
	} else if (rcvHdr.GetType() == 5) {
		NS_LOG_INFO(
				"Force Sending Data from: "<<addr<<" to: "<<InetSocketAddress::ConvertFrom (to).GetIpv4 ());
	} else {
		NS_LOG_INFO(
				"Sending Data from: "<<addr<<" to: "<<InetSocketAddress::ConvertFrom (to).GetIpv4 ());
	}

//	MyTag tagg;
//	tagg.SetFlag();
//	p->AddPacketTag(tagg);

	m_socket->SendTo(p, 0, to);
}

void itp::ReceveFrom(Ptr<Socket> socket) {
	NS_LOG_INFO("Here");
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
		case 6:
			m_producer->OnInterest(packet, from);
			break;
		case 3:
		case 4:
			NS_LOG_INFO("Received Data");
			m_cWndTrace(1);
			m_consumer->OnData(packet, from);
			break;
		case 5:
			packet->RemoveHeader(rcvHdr);
			m_receivedContent(packet, from);

		}
	}
}

void itp::ForceSend(Ptr<Packet> p, InetSocketAddress to) {
	ItpHeader sndHdr;
	sndHdr.SetType(5);
	p->AddHeader(sndHdr);
	SendTo(p, to);

}

void itp::SetRecvContentCallback(
		Callback<void, Ptr<Packet>, Address> receivedData) {
	m_receivedContent = receivedData;
}

}
;
