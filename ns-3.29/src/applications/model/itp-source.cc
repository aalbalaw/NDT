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
#include "itp-source.h"

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
#include "ns3/ipv4.h"
#include "ns3/pointer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ItpSource");

NS_OBJECT_ENSURE_REGISTERED(ItpSource);

TypeId
ItpSource::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::ItpSource").SetParent<Application> ().SetGroupName (
	  "Applications").AddConstructor<ItpSource> ().AddAttribute (
	  "MaxPackets",
	  "The maximum number of packets the application will send",
	  UintegerValue (100),
	  MakeUintegerAccessor (&ItpSource::m_count),
	  MakeUintegerChecker<uint32_t> ()).AddAttribute (
	  "Interval", "The time to wait between packets",
	  TimeValue (Seconds (1.0)),
	  MakeTimeAccessor (&ItpSource::m_interval),
	  MakeTimeChecker ()).AddAttribute (
	  "RemoteAddress", "The destination Address of the outbound packets",
	  AddressValue (),
	  MakeAddressAccessor (&ItpSource::m_peerAddress),
	  MakeAddressChecker ()).AddAttribute (
	  "RemotePort", "The destination port of the outbound packets",
	  UintegerValue (0),
	  MakeUintegerAccessor (&ItpSource::m_peerPort),
	  MakeUintegerChecker<uint16_t> ()).AddAttribute (
	  "PacketSize",
	  "Size of echo data in outbound packets",
	  UintegerValue (100),
	  MakeUintegerAccessor (&ItpSource::SetDataSize,
				&ItpSource::GetDataSize),
	  MakeUintegerChecker<uint32_t> ());
  return tid;
}

ItpSource::ItpSource ()
{
  NS_LOG_FUNCTION(this);
  m_sent = 0;
  m_sendEvent = EventId ();
  m_data = 0;
  m_dataSize = 0;
  m_itp = CreateObject<itp> ();
  server2 = false;
}

ItpSource::~ItpSource ()
{
  delete[] m_data;
  m_data = 0;
  m_dataSize = 0;
}

void
ItpSource::SetRemote (Address ip, uint16_t port)
{
  m_peerAddress = ip;
  m_peerPort = port;
}

void
ItpSource::SetRemote (Address addr)
{
  m_peerAddress = addr;
}

void
ItpSource::DoDispose (void)
{
  Application::DoDispose ();
}

void
ItpSource::StartApplication (void)
{
  m_itp->CreateSocket (9, GetNode ());
  m_itp->SetRecvCallback (
      MakeCallback (&ItpSource::HandleRead, this));
  ScheduleTransmit (Seconds (0.));
}

void
ItpSource::StopApplication ()
{
  m_itp->CloseSocket ();
  Simulator::Cancel (m_sendEvent);
}

void
ItpSource::SetDataSize (uint32_t dataSize)
{
  delete[] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = dataSize;
}

uint32_t
ItpSource::GetDataSize (void) const
{
  return m_size;
}

void
ItpSource::ScheduleTransmit (Time dt)
{
  m_sendEvent = Simulator::Schedule (dt, &ItpSource::Send, this);
}

void
ItpSource::Send (void)
{
  //NS_ASSERT (m_sendEvent.IsExpired ());
  Ptr < Packet > p;
  if (m_dataSize)
    {
      NS_ASSERT_MSG(
	  m_dataSize == m_size,
	  "ItpSource::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG(m_data,
		    "ItpSource::Send(): m_dataSize but no m_data");
      p = Create < Packet > (m_data, m_dataSize);
    }
  else
    {
      p = Create < Packet > (m_size);
    }

  InetSocketAddress remote = InetSocketAddress (
      Ipv4Address::ConvertFrom (m_peerAddress), m_peerPort);
  NS_LOG_INFO("Sending Manifest to Client at: "<<Simulator::Now().GetSeconds());
  m_itp->SendContent (p, remote);
  ++m_sent;

  if (m_sent < m_count)
    {
      ScheduleTransmit (m_interval);
    }
}

void
ItpSource::HandleRead (Address from)
{
  NS_LOG_INFO("Node ID: "<<this->GetNode()->GetId());
  Ptr < Ipv4 > ipv4 = this->GetNode ()->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  Address client = ipv4->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.

  NS_LOG_INFO(
      "At time " << Simulator::Now ().GetSeconds () << " Client "<<client<<" received data from" << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());
}

} // Namespace ns3
