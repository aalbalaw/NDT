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

#include "itp-sink.h"

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


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ItpSink");

NS_OBJECT_ENSURE_REGISTERED (ItpSink);

TypeId
ItpSink::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ItpSink")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<ItpSink> ()
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&ItpSink::m_port),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

ItpSink::ItpSink ()
{
  //NS_LOG_FUNCTION (this);
  m_itp = CreateObject<itp> ();
}

ItpSink::~ItpSink()
{
  //NS_LOG_FUNCTION (this);

}

void
ItpSink::DoDispose (void)
{
  //NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
ItpSink::StartApplication (void)
{
 // NS_LOG_FUNCTION (this);
  m_itp->CreateSocket(m_port,GetNode());
  m_itp->SetRecvCallback(MakeCallback(&ItpSink::HandleRead, this));
}

void 
ItpSink::StopApplication ()
{
 // NS_LOG_FUNCTION (this);
  m_itp->CloseSocket();
}

void 
ItpSink::HandleRead (Address from)
{

	NS_LOG_INFO("Node ID: "<<this->GetNode()->GetId());
	NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << " Sink received data from "<<InetSocketAddress::ConvertFrom (from).GetIpv4 ());
	StopApplication();
	//Ptr<Packet> p = Create<Packet>(7996800);
	//m_itp->SendContent(p,InetSocketAddress(InetSocketAddress::ConvertFrom (from).GetIpv4 (),InetSocketAddress::ConvertFrom (from).GetPort()));

}

} // Namespace ns3
