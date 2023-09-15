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
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "tcp-echo-server.h"
#include "ns3/random-variable-stream.h"
#include "ns3/double.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("TcpEchoServerApplication");
NS_OBJECT_ENSURE_REGISTERED(TcpEchoServer);

TypeId TcpEchoServer::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::TcpEchoServer").SetParent<Application>().AddConstructor<
					TcpEchoServer>().AddAttribute("Local",
					"The Address on which to Bind the rx socket.",
					Ipv4AddressValue(),
					MakeIpv4AddressAccessor(&TcpEchoServer::m_local),
					MakeIpv4AddressChecker()).AddAttribute("Port",
					"Port on which we listen for incoming packets.",
					UintegerValue(80),
					MakeUintegerAccessor(&TcpEchoServer::m_port),
					MakeUintegerChecker<uint16_t>()).AddAttribute("ContentSize",
					"Size of echo data in outbound packets",
					UintegerValue(1500),
					MakeUintegerAccessor(&TcpEchoServer::m_size),
					MakeUintegerChecker<uint32_t>());
	return tid;
}

TcpEchoServer::TcpEchoServer() {
	NS_LOG_FUNCTION_NOARGS ();
	index=0;
}

TcpEchoServer::~TcpEchoServer() {
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
	index=0;
}

void TcpEchoServer::DoDispose(void) {
	NS_LOG_FUNCTION_NOARGS ();
	Application::DoDispose();
}

void TcpEchoServer::StartApplication(void) {
	NS_LOG_FUNCTION_NOARGS ();

	m_url = "main/object";
	if (!m_socket) {
		TypeId tid = TypeId::LookupByName("ns3::TcpSocketFactory");

		m_socket = Socket::CreateSocket(GetNode(), tid);

		// Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
		if (m_socket->GetSocketType() != Socket::NS3_SOCK_STREAM
				&& m_socket->GetSocketType() != Socket::NS3_SOCK_SEQPACKET) {
			NS_FATAL_ERROR("Using HttpServer with an incompatible socket type. "
					"HttpServer requires SOCK_STREAM or SOCK_SEQPACKET. "
					"In other words, use TCP instead of UDP.");
		}

		InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(),
				m_port);
		m_socket->Bind(local);
		m_socket->Listen();
		m_socket->SetAcceptCallback(
				MakeCallback(&TcpEchoServer::HandleRequest, this),
				MakeCallback(&TcpEchoServer::HandleAccept, this));
	}
	m_socket->SetCloseCallbacks(MakeCallback(&TcpEchoServer::HandleClose, this),
			MakeCallback(&TcpEchoServer::HandleClose, this));
}

void TcpEchoServer::HandleClose(Ptr<Socket> s1) {

	if (m_socket != 0) {
		m_socket->Close();
	} else {
		NS_LOG_WARN(
				"HttpServerApplication found null socket to close in StopApplication");
	}
}

bool TcpEchoServer::HandleRequest(Ptr<Socket> s, const Address& from) {
	NS_LOG_INFO(
			" HANDLE ACCEPT REQUEST FROM " << InetSocketAddress::ConvertFrom(from));
	return true;
}

void TcpEchoServer::HandleAccept(Ptr<Socket> s, const Address& from) {

	NS_LOG_DEBUG(
			"HttpServer >> Connection with Client (" << InetSocketAddress::ConvertFrom (from).GetIpv4 () << ") successfully established!");
	s->SetRecvCallback(MakeCallback(&TcpEchoServer::HandleReceive, this));
}

void TcpEchoServer::StopApplication() {
	NS_LOG_FUNCTION_NOARGS ();

	if (m_socket != 0) {
		m_socket->Close();
		m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
	}
}

void TcpEchoServer::HandleReceive(Ptr<Socket> socket) {
	Ptr<Packet> packet;
	Address from;
	Ptr<Packet> serverPkt;
	int pktSize;
	while (packet = socket->RecvFrom(from)) {
		pktSize = packet->GetSize();
//		NS_LOG_INFO(
//				m_local << "Received " << packet->GetSize () << " bytes from " << InetSocketAddress::ConvertFrom (from).GetIpv4 ());

	}

	int numberOfRequests = pktSize/100.0;
	NS_LOG_INFO("Number of requests "<<numberOfRequests);

	std::vector<std::vector<int>> vec{ { 2073 },
	                         { 10762,33375 },
	                         { 16396,6646,3982 },
							 { 119538,2471,15290,36732 },
							 { 14781,3998,3011,1753,1372 }};

//
//	std::vector<std::vector<int>> vec{ { 500 },
//	                         { 500,500 },
//	                         { 500,500,500 },
//							 { 500,500,500,500 },
//							 { 500,500,500,500,500 }};
//
//

	//This is for deciding the content size;

	uint32_t contentSize=0;
	int scenario = 4;
	//index=0;

//	if (m_url == "main/object") {
//		contentSize=vec[scenario][index];
//		NS_LOG_INFO("Sending Size "<<contentSize);
//		index++;
//		m_url = "inline/object";
//	} else {
//			contentSize+=vec[scenario][index];
//			NS_LOG_INFO("Sending Size "<<contentSize);
//			index++;
//	}

	//Pipeline
	if (m_url == "main/object") {
		contentSize=vec[scenario][index];
		NS_LOG_INFO("Sending Size "<<contentSize);
		index++;
		m_url = "inline/object";
	} else {
		while(numberOfRequests>=1){
			NS_LOG_INFO("index "<<index);
			contentSize+=vec[scenario][index];
			NS_LOG_INFO("Chunk Size "<<vec[scenario][index]);
			NS_LOG_INFO("Sending Size "<<contentSize);
			numberOfRequests--;
			index++;
		}
	}

	NS_LOG_INFO("Content Size "<<contentSize);
	serverPkt = Create<Packet>(contentSize);
	socket->Send(serverPkt);

}

} // Namespace ns3
