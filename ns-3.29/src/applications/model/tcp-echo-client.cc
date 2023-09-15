/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "tcp-echo-client.h"
#include "ns3/boolean.h"
#include "dns-header.h"
#include "ns3/ipv4.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("TcpEchoClientApplication");
NS_OBJECT_ENSURE_REGISTERED(TcpEchoClient);

TypeId TcpEchoClient::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::TcpEchoClient").SetParent<Application>().AddConstructor<
					TcpEchoClient>().AddAttribute("RemoteAddress",
					"The destination Ipv4Address of the outbound packets",
					AddressValue(),
					MakeAddressAccessor(&TcpEchoClient::m_peerAddress),
					MakeAddressChecker()).AddAttribute("RemotePort",
					"The destination port of the outbound packets",
					UintegerValue(0),
					MakeUintegerAccessor(&TcpEchoClient::m_peerPort),
					MakeUintegerChecker<uint16_t>()).AddAttribute("PacketSize",
					"Size of echo data in outbound packets", UintegerValue(100),
					MakeUintegerAccessor(&TcpEchoClient::SetDataSize,
							&TcpEchoClient::GetDataSize),
					MakeUintegerChecker<uint32_t>()).AddAttribute("EnableDNS",
					"Enable DNS at this application", BooleanValue(false),
					MakeBooleanAccessor(&TcpEchoClient::m_enableDNS),
					MakeBooleanChecker()).AddAttribute("LocalDNSAddress",
					"The destination Address of the local DNS server",
					AddressValue(),
					MakeAddressAccessor(&TcpEchoClient::m_localDnsAddress),
					MakeAddressChecker()).AddTraceSource("Tx",
					"A new packet is created and is sent",
					MakeTraceSourceAccessor(&TcpEchoClient::m_txTrace));
	return tid;
}

TcpEchoClient::TcpEchoClient() {
	NS_LOG_FUNCTION_NOARGS ();
	m_sent = 0;
	m_socket = 0;
	m_sendEvent = EventId();
	m_data = 0;
	m_dataSize = 0;
	m_bytes = 0;
}

TcpEchoClient::~TcpEchoClient() {
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;

	delete[] m_data;
	m_data = 0;
	m_dataSize = 0;
}

void TcpEchoClient::SetRemote(Ipv4Address ip, uint16_t port) {
	m_peerAddress = ip;
	m_peerPort = port;
}

void TcpEchoClient::DoDispose(void) {
	NS_LOG_FUNCTION_NOARGS ();
	Application::DoDispose();
}

void TcpEchoClient::StartApplication(void) {
	NS_LOG_FUNCTION_NOARGS ();

	m_numOfInlineObjects = 0;
	if (!m_enableDNS) {
		if (m_socket == 0) {
			TypeId tid = TypeId::LookupByName("ns3::TcpSocketFactory");
			m_socket = Socket::CreateSocket(GetNode(), tid);
			m_socket->Bind();
			m_socket->Connect(
					InetSocketAddress(Ipv4Address::ConvertFrom(m_peerAddress),
							m_peerPort));
			NS_LOG_INFO("Establishing connection with server");
			m_socket->SetConnectCallback(
					MakeCallback(&TcpEchoClient::ConnectionSucceeded, this),
					MakeCallback(&TcpEchoClient::ConnectionFailed, this));
			m_startTime = Simulator::Now();
		}
//		m_socket->SetRecvCallback(
//				MakeCallback(&TcpEchoClient::HandleRead, this));
//		ScheduleTransmit(Seconds(0.));
	} else {
		NS_LOG_INFO("Resolving hostname using DNS");
		if (m_dnsSocket == 0) {
			TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
			m_dnsSocket = Socket::CreateSocket(GetNode(), tid);
			m_dnsSocket->Bind();
			m_dnsSocket->SetRecvCallback(
					MakeCallback(&TcpEchoClient::ResolveDns, this));
			m_startTime = Simulator::Now();
		}

		GetHostByName(m_url);
	}

}

void TcpEchoClient::ConnectionSucceeded(Ptr<Socket> socket) {
	NS_LOG_FUNCTION("Connection to server succeeded");

	Ipv4Address temp =
			socket->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

	m_socket->SetRecvCallback(MakeCallback(&TcpEchoClient::HandleRead, this));
	m_socket->SetCloseCallbacks(
			MakeCallback(&TcpEchoClient::ConnectionClosed, this),
			MakeCallback(&TcpEchoClient::ConnectionFailed, this));
	ScheduleTransmit(Seconds(0.));

}

void TcpEchoClient::ConnectionFailed(Ptr<Socket> socket) {
	NS_LOG_FUNCTION("Connection to server failed");

}

void TcpEchoClient::ConnectionClosed(Ptr<Socket> socket) {
	NS_LOG_INFO(
			"Total Time: "<<(Simulator::Now()-m_startTime).GetMilliSeconds());

}

void TcpEchoClient::StopApplication() {
	NS_LOG_FUNCTION_NOARGS ();

	if (m_socket != 0) {
		NS_LOG_INFO("Here1");
		m_socket->Close();
		NS_LOG_INFO("Here2");
		m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
		m_socket = 0;
	}
	NS_LOG_INFO("Here");

	Simulator::Cancel(m_sendEvent);
}

void TcpEchoClient::SetDataSize(uint32_t dataSize) {
	NS_LOG_FUNCTION(dataSize);

	//
	// If the client is setting the echo packet data size this way, we infer
	// that she doesn't care about the contents of the packet at all, so
	// neither will we.
	//
	delete[] m_data;
	m_data = 0;
	m_dataSize = 0;
	m_size = dataSize;
}

uint32_t TcpEchoClient::GetDataSize(void) const {
	NS_LOG_FUNCTION_NOARGS ();
	return m_size;
}

void TcpEchoClient::SetURL(std::string url) {
	m_url = url;
}

void TcpEchoClient::GetHostByName(std::string name) {
	if (m_enableDNS) {
		NS_LOG_INFO("DNS is enabled. Resolving URL: "<<m_url);
		if (!m_dnsResolved) {
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
			nsQuery.SetqName(m_url);
			nsQuery.SetqType(1);
			nsQuery.SetqClass(1);

			nsHeader.AddQuestion(nsQuery);
			requestRR->AddHeader(nsHeader);
			m_dnsSocket->SendTo(requestRR, 0,
					InetSocketAddress(
							Ipv4Address::ConvertFrom(m_localDnsAddress), 53));
		}
		NS_LOG_INFO("Here");
		m_dnsResolved = true;
	} else {
		ScheduleTransmit(Seconds(0.));
	}
}

void TcpEchoClient::ResolveDns(Ptr<Socket> dnsSocket) {
	NS_LOG_INFO("Here1");
	Ptr<Packet> packet;
	Address from;
	while ((packet = dnsSocket->RecvFrom(from))) {
		if (InetSocketAddress::IsMatchingType(from)) {
			NS_LOG_INFO(
					"At time " << Simulator::Now ().GetSeconds () << "s client received " << packet->GetSize () << " bytes from " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());
		}
		DNSHeader DnsHeader;
		packet->RemoveHeader(DnsHeader);
		std::string forwardingAddress;
		std::list<ResourceRecordHeader> answerList;
		answerList = DnsHeader.GetAnswerList();
		forwardingAddress = answerList.begin()->GetRData().c_str();
		NS_LOG_INFO(
				"Receives Answer: "<<Ipv4Address(forwardingAddress.c_str()));
		m_peerAddress = Ipv4Address(forwardingAddress.c_str());
	}
	Connect();
}

void TcpEchoClient::Connect() {

	if (m_socket == 0) {
		TypeId tid = TypeId::LookupByName("ns3::TcpSocketFactory");
		m_socket = Socket::CreateSocket(GetNode(), tid);
		m_socket->Bind();
		m_socket->Connect(
				InetSocketAddress(Ipv4Address::ConvertFrom(m_peerAddress),
						m_peerPort));
	}

	m_socket->SetRecvCallback(MakeCallback(&TcpEchoClient::HandleRead, this));
	ScheduleTransmit(Seconds(0.));
}
void TcpEchoClient::ScheduleTransmit(Time dt) {
	NS_LOG_FUNCTION_NOARGS ();
	m_sendEvent = Simulator::Schedule(dt, &TcpEchoClient::Send, this);
}

void TcpEchoClient::Send(void) {

	NS_LOG_INFO("Sending HTTP request");
	Ptr<Packet> p;
	p = Create<Packet>(m_size);
	int retVal = m_socket->Send(p);
	NS_LOG_INFO(retVal);

}

void TcpEchoClient::HandleRead(Ptr<Socket> socket) {
	//NS_LOG_FUNCTION(this << socket);
	Ptr<Packet> packet;
	Address from;
	while (packet = socket->RecvFrom(from)) {
		if (InetSocketAddress::IsMatchingType(from)) {
//			NS_LOG_INFO(
//					"Received " << packet->GetSize () << " bytes from " << InetSocketAddress::ConvertFrom (from).GetIpv4 ());
			m_bytes += packet->GetSize();
		}
	}


	std::vector<std::vector<int>> vec { { 2073 },
		{ 10762, 33375 },
		{ 16396,6646, 3982 },
		{ 119538, 2471, 15290, 36732 },
		{ 14781,3998,3011,1753,1372 } };


//		std::vector<std::vector<int>> vec{ { 500 },
//		                         { 500,500 },
//		                         { 500,500,500 },
//								 { 500,500,500,500 },
//								 { 500,500,500,500 }};
//

	//This is for deciding the content size;

	uint32_t contentSize = 0;
	int scenario =4;

	//Persistent connection
//
//	if (m_bytes == vec[scenario][m_numOfInlineObjects]) {
//		NS_LOG_INFO("Total bytes received: " << m_bytes <<" Object Size: "<<vec[scenario][m_numOfInlineObjects]);
//		m_bytes = 0;
//		m_numOfInlineObjects++;
//		if (m_numOfInlineObjects <=4) {
//			Send();
//		}else{
//
//			NS_LOG_INFO("Total Time: " << (Simulator::Now() - m_startTime).GetMilliSeconds());
//		}
//
//	}

	//Pipelining
	if (m_bytes == vec[scenario][0]) {
		NS_LOG_INFO("Total bytes received: " << m_bytes <<" Object Size: "<<vec[scenario][m_numOfInlineObjects]);
		while(m_numOfInlineObjects < 4) {
			Send();
			m_numOfInlineObjects++;
		}
	}
	NS_LOG_INFO("Total Time: " << (Simulator::Now() - m_startTime).GetMilliSeconds());
	/*

	 if (m_numOfInlineObjects != 0) {
	 m_numOfInlineObjects--;
	 m_url = "inline/object";
	 Send();
	 } else {
	 NS_LOG_INFO(
	 "Total Time: "<<(Simulator::Now()-m_startTime).GetMilliSeconds());
	 //StopApplication();
	 NS_LOG_INFO("Total bytes received: "<<m_bytes);
	 }

	 */
}

} // Namespace ns3
