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
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/mytag.h"
#include "ns3/itp-header.h"
#include "cctp-proxy.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("CctpProxy");

NS_OBJECT_ENSURE_REGISTERED(CctpProxy);

TypeId CctpProxy::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::CctpProxy").SetParent<Application>().SetGroupName(
					"Applications").AddConstructor<CctpProxy>().AddAttribute(
					"Port", "Port on which we listen for incoming packets.",
					UintegerValue(9), MakeUintegerAccessor(&CctpProxy::m_port),
					MakeUintegerChecker<uint16_t>());
	return tid;
}

CctpProxy::CctpProxy() {
}

CctpProxy::~CctpProxy() {
	m_socket = 0;
}

void CctpProxy::DoDispose(void) {
	NS_LOG_FUNCTION(this);
	Application::DoDispose();
}

void CctpProxy::StartApplication(void) {
	NS_LOG_FUNCTION(this);

	if (m_socket == 0) {
		TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket(GetNode(), tid);
		InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(),
				m_port);
		NS_LOG_INFO("Local address "<<local.GetIpv4());
		m_socket->Bind(local);
		if (addressUtils::IsMulticast(m_local)) {
			Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket>(m_socket);
			if (udpSocket) {
				// equivalent to setsockopt (MCAST_JOIN_GROUP)
				udpSocket->MulticastJoinGroup(0, m_local);
			} else {
				NS_FATAL_ERROR("Error: Failed to join multicast group");
			}
		}
	}

	m_socket->SetRecvCallback(MakeCallback(&CctpProxy::HandleRead, this));
	m_pit.resize(60000);
	m_contentStore.resize(60000);
}

void CctpProxy::StopApplication() {
	NS_LOG_FUNCTION(this);

	if (m_socket != 0) {
		m_socket->Close();
		m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
	}
}

void CctpProxy::HandleRead(Ptr<Socket> socket) {
	NS_LOG_FUNCTION(this << socket);

	Ptr<Packet> packet;
	Address from;
	while ((packet = socket->RecvFrom(from))) {
		if (InetSocketAddress::IsMatchingType(from)) {

			ItpHeader rcvHdr;
			packet->PeekHeader(rcvHdr);

			if (rcvHdr.GetType() == 2) {
				NS_LOG_INFO(
						"Received Interest from:" <<InetSocketAddress::ConvertFrom (from).GetIpv4 ());
				OnInterest(packet);

			} else {
				NS_LOG_INFO(
						"Received Data from: "<<InetSocketAddress::ConvertFrom (from).GetIpv4 ());
				OnData(packet);
			}

		}
	}
}

/*
 * 1. Check cache, if exists send chunk back to consumer based on tag info
 * 2. If not, contact producer if we didn't already sent an interest. For now we can assume packets don't get lost
 * 		Solution: If we receive an interest with src ip address already exist in table, we should try to contact the
 * 		producer again since this is a retransmit.
 * 3.Else, we add consumer IP address to pending
 */
void CctpProxy::OnInterest(Ptr<Packet> interest) {

	MyTag routerTag;
	interest->RemovePacketTag(routerTag);

	NS_LOG_INFO(
			"srcIP: "<<routerTag.GetSourceIP()<<" dstIP: "<<routerTag.GetDestinationIP());
	NS_LOG_INFO(
			"srcPort: "<<routerTag.GetSourcePort()<<" dstPort: "<<routerTag.GetDestinationPort());

	ItpHeader rcvHdr;
	interest->RemoveHeader(rcvHdr);

	if (m_contentStore[rcvHdr.GetSeqNumber()] == 1) {
		NS_LOG_INFO("Data already cached");
		InetSocketAddress toConsumer = InetSocketAddress(routerTag.GetSourceIP(),
				routerTag.GetSourcePort());
		Ptr<Packet> p = Create<Packet>(1000);
		ItpHeader mHdr;
		mHdr.SetType(3);
		mHdr.SetSeqNumber(rcvHdr.GetSeqNumber());
		mHdr.SetName(rcvHdr.GetName());
		mHdr.SetCounter(rcvHdr.GetCounter());
		p->AddHeader(mHdr);
		m_socket->SendTo(p, 0, toConsumer);

	} else if (m_pit[rcvHdr.GetSeqNumber()].empty()) {
		NS_LOG_INFO("Creating a new Pending Interest Table");
		m_pit[rcvHdr.GetSeqNumber()].push_back(
				InetSocketAddress(routerTag.GetSourceIP(),
						routerTag.GetSourcePort()));

		InetSocketAddress toProducer = InetSocketAddress(
				routerTag.GetDestinationIP(), routerTag.GetDestinationPort());

		Ptr<Packet> p = Create<Packet>();
		ItpHeader mHdr;
		mHdr.SetType(6);
		mHdr.SetSeqNumber(rcvHdr.GetSeqNumber());
		mHdr.SetName(rcvHdr.GetName());
		mHdr.SetCounter(rcvHdr.GetCounter());
		p->AddHeader(mHdr);
		m_socket->SendTo(p, 0, toProducer);
	}else{
		NS_LOG_INFO("Adding Consumer to already existing Pending Interest");
		m_pit[rcvHdr.GetSeqNumber()].push_back(
						InetSocketAddress(routerTag.GetSourceIP(),
								routerTag.GetSourcePort()));
	}
}

void CctpProxy::OnData(Ptr<Packet> data) {

	ItpHeader rcvHdr;
	data->RemoveHeader(rcvHdr);

	m_contentStore[rcvHdr.GetSeqNumber()] =1;

	while (!m_pit[rcvHdr.GetSeqNumber()].empty()) {

		InetSocketAddress to = m_pit[rcvHdr.GetSeqNumber()].front();
		NS_LOG_INFO("Satesfying Interest for consumer "<<to.GetIpv4());
		m_pit[rcvHdr.GetSeqNumber()].pop_front();
		data->AddHeader(rcvHdr);
		m_socket->SendTo(data, 0, to);
	}

}

} // Namespace ns3
