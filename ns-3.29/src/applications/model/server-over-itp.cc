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

#include "server-over-itp.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ServerOverItp");

NS_OBJECT_ENSURE_REGISTERED(ServerOverItp);

TypeId ServerOverItp::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::ServerOverItp").SetParent<Application>().SetGroupName(
					"Applications").AddConstructor<ServerOverItp>()
					.AddAttribute(
					"Port", "Port on which we listen for incoming packets.",
					UintegerValue(9),
					MakeUintegerAccessor(&ServerOverItp::m_port),
					MakeUintegerChecker<uint16_t>())
					.AddAttribute("ContentSize",
							"Size of echo data in outbound packets", UintegerValue(1000000),
							MakeUintegerAccessor(&ServerOverItp::m_size),
							MakeUintegerChecker<uint32_t>());
	return tid;
}

ServerOverItp::ServerOverItp() {
	m_itp = CreateObject<itp> ();
}

ServerOverItp::~ServerOverItp() {
}

void ServerOverItp::DoDispose(void) {
	Application::DoDispose();
}

void ServerOverItp::StartApplication(void) {
	m_itp->CreateSocket(m_port, GetNode());
	m_itp->SetRecvContentCallback(
			MakeCallback(&ServerOverItp::HandleRequests, this));
}

void ServerOverItp::StopApplication() {

	m_itp->CloseSocket();
}

void ServerOverItp::HandleRequests(Ptr<Packet> request, Address from) {
	Ptr<Packet> packet;
	packet = Create<Packet>(m_size);
	NS_LOG_INFO(
			"At time " << Simulator::Now ().GetSeconds () << "s server received request from "
			<< InetSocketAddress::ConvertFrom (from).GetIpv4 () << " For content with size "<<packet->GetSize());

	m_itp->SendContent(packet, from);
}
} // Namespace ns3
