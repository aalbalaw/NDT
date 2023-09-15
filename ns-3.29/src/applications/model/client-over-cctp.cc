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
#include "client-over-cctp.h"
#include "ns3/boolean.h"
#include "dns-header.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ClientOverCCTP");

NS_OBJECT_ENSURE_REGISTERED(ClientOverCCTP);

TypeId ClientOverCCTP::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::ClientOverCCTP").SetParent<Application>().SetGroupName(
					"Applications").AddConstructor<ClientOverCCTP>()
					.AddAttribute("Interval",
					"The time to wait between packets", TimeValue(Seconds(1.0)),
					MakeTimeAccessor(&ClientOverCCTP::m_interval),
					MakeTimeChecker())
					.AddAttribute("RemoteAddress",
					"The destination Address of the outbound packets",
					AddressValue(),
					MakeAddressAccessor(&ClientOverCCTP::m_peerAddress),
					MakeAddressChecker())
					.AddAttribute("RemotePort",
					"The destination port of the outbound packets",
					UintegerValue(0),
					MakeUintegerAccessor(&ClientOverCCTP::m_peerPort),
					MakeUintegerChecker<uint16_t>())
					.AddAttribute("PacketSize",
					"Size of echo data in outbound packets", UintegerValue(100),
					MakeUintegerAccessor(&ClientOverCCTP::m_size),
					MakeUintegerChecker<uint32_t>())
					.AddAttribute("EnableDNS",
					"Enable DNS at this application", BooleanValue(false),
					MakeBooleanAccessor(&ClientOverCCTP::m_enableDNS),
					MakeBooleanChecker())
					.AddAttribute("LocalDNSAddress",
					"The destination Address of the local DNS server",
					AddressValue(),
					MakeAddressAccessor(&ClientOverCCTP::m_localDnsAddress),
					MakeAddressChecker())
					.AddTraceSource("Tx",
					"A new packet is created and is sent",
					MakeTraceSourceAccessor(&ClientOverCCTP::m_txTrace),
					"ns3::Packet::TracedCallback");
	return tid;
}

ClientOverCCTP::ClientOverCCTP() {

	m_cctp = CreateObject<CCTP> ();
	m_sendEvent = EventId();
}

ClientOverCCTP::~ClientOverCCTP() {
}

void ClientOverCCTP::SetRemote(Address ip, uint16_t port) {
	NS_LOG_FUNCTION(this << ip << port);
	m_peerAddress = ip;
	m_peerPort = port;
}

void ClientOverCCTP::SetRemote(Ipv4Address ip, uint16_t port) {
	NS_LOG_FUNCTION(this << ip << port);
	m_peerAddress = Address(ip);
	m_peerPort = port;
}

void ClientOverCCTP::DoDispose(void) {

	Application::DoDispose();
}

void ClientOverCCTP::StartApplication(void) {

    Ptr<ExponentialRandomVariable> numOfInlineObjStream = CreateObject<ExponentialRandomVariable> ();
    numOfInlineObjStream->SetAttribute ("Mean", DoubleValue (31.9291));
   // m_numOfInlineObjects = numOfInlineObjStream->GetInteger();
    m_numOfInlineObjects = 0;

	m_cctp->CreateSocket(81, this->GetNode());
	m_cctp->SetRecvCallback(MakeCallback(&ClientOverCCTP::HandleResponse, this));

	if (m_dnsSocket == 0) {
		TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
		m_dnsSocket = Socket::CreateSocket(GetNode(), tid);
		m_dnsSocket->Bind();
		m_dnsSocket->SetRecvCallback(
				MakeCallback(&ClientOverCCTP::ResolveDns, this));
		m_startTime=Simulator::Now();
		m_url = "main/object";
		m_cctp->GetContentByName(m_url, m_localDnsAddress);
	}
}

void ClientOverCCTP::StopApplication() {
	m_cctp->CloseSocket();
	Simulator::Cancel(m_sendEvent);
}

void ClientOverCCTP::SetURL(std::string url) {
	m_url = url;
}

void ClientOverCCTP::GetHostByName(std::string name) {
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

void ClientOverCCTP::ResolveDns(Ptr<Socket> dnsSocket) {
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
		ScheduleTransmit(Seconds(0.));
	}
}

void ClientOverCCTP::ScheduleTransmit(Time dt) {
	m_sendEvent = Simulator::Schedule(dt, &ClientOverCCTP::Send, this);
}

void ClientOverCCTP::Send(void) {

	Ptr<Packet> p;
	p = Create<Packet>(m_size);
	InetSocketAddress remote = InetSocketAddress(
			Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort);
	m_cctp->ForceSend(p, remote);
	if (Ipv4Address::IsMatchingType(m_peerAddress)) {
		NS_LOG_INFO(
				"At time " << Simulator::Now ().GetSeconds () << "s client sent " << m_size << " bytes to " << Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
	}
}

void ClientOverCCTP::HandleResponse(Address from) {

//	NS_LOG_INFO(
//			"At time " << Simulator::Now ().GetSeconds () << InetSocketAddress::ConvertFrom (from).GetIpv4 ());

	if(m_numOfInlineObjects!=0){
		m_numOfInlineObjects--;
		m_url = "inline/object";
		NS_LOG_INFO("Total Time: "<<(Simulator::Now()-m_startTime).GetMilliSeconds());
		m_cctp->GetContentByName(m_url, m_localDnsAddress);
	}else{
		NS_LOG_INFO("Total Time: "<<(Simulator::Now()-m_startTime).GetMilliSeconds());
	}
	//StopApplication();
}
} // Namespace ns3
