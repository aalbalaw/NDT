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

#include "server-over-cctp.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ServerOverCCTP");

NS_OBJECT_ENSURE_REGISTERED(ServerOverCCTP);

TypeId ServerOverCCTP::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::ServerOverCCTP").SetParent<Application>().SetGroupName(
					"Applications").AddConstructor<ServerOverCCTP>()
					.AddAttribute(
					"Port", "Port on which we listen for incoming packets.",
					UintegerValue(9),
					MakeUintegerAccessor(&ServerOverCCTP::m_port),
					MakeUintegerChecker<uint16_t>())
					.AddAttribute("ContentSize",
							"Size of echo data in outbound packets", UintegerValue(1000000),
							MakeUintegerAccessor(&ServerOverCCTP::m_size),
							MakeUintegerChecker<uint32_t>());
	return tid;
}

ServerOverCCTP::ServerOverCCTP() {
	m_cctp = CreateObject<CCTP> ();
}

ServerOverCCTP::~ServerOverCCTP() {
}

void ServerOverCCTP::DoDispose(void) {
	Application::DoDispose();
}

void ServerOverCCTP::StartApplication(void) {
	m_cctp->CreateSocket(m_port, GetNode());
	m_cctp->SetRecvContentCallback(
			MakeCallback(&ServerOverCCTP::HandleRequests, this));
}

void ServerOverCCTP::StopApplication() {

	m_cctp->CloseSocket();
}

void ServerOverCCTP::HandleRequests(Ptr<Packet> request, Address from) {
	Ptr<Packet> packet;
	packet = Create<Packet>(m_size);
	NS_LOG_INFO(
			"At time " << Simulator::Now ().GetSeconds () << "s server received request from "
			<< InetSocketAddress::ConvertFrom (from).GetIpv4 () << " For content with size "<<packet->GetSize());

	m_cctp->SendContent(packet, from);
}
} // Namespace ns3
