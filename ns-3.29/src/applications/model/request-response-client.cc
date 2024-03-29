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
#include "request-response-client.h"
#include "ns3/boolean.h"
#include "dns-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RequestResponseClientApplication");

NS_OBJECT_ENSURE_REGISTERED (RequestResponseClient);

TypeId
RequestResponseClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RequestResponseClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<RequestResponseClient> ()
    .AddAttribute ("MaxPackets", 
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&RequestResponseClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval", 
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&RequestResponseClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemoteAddress", 
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&RequestResponseClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", 
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RequestResponseClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&RequestResponseClient::SetDataSize,
                                         &RequestResponseClient::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("EnableDNS",
				   "Enable DNS at this application",
				   BooleanValue (false),
				   MakeBooleanAccessor (&RequestResponseClient::m_enableDNS),
				   MakeBooleanChecker ())
	.AddAttribute ("LocalDNSAddress",
				   "The destination Address of the local DNS server",
				   AddressValue (),
				   MakeAddressAccessor (&RequestResponseClient::m_localDnsAddress),
				   MakeAddressChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&RequestResponseClient::m_txTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

RequestResponseClient::RequestResponseClient ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
  m_data = 0;
  m_dataSize = 0;
}

RequestResponseClient::~RequestResponseClient()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;

  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
}

void 
RequestResponseClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void 
RequestResponseClient::SetRemote (Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address (ip);
  m_peerPort = port;
}

void 
RequestResponseClient::SetRemote (Ipv6Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address (ip);
  m_peerPort = port;
}

void
RequestResponseClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
RequestResponseClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
        {
          m_socket->Bind();
        }
      else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
        {
          m_socket->Bind6();
          m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&RequestResponseClient::HandleRead, this));

  if (m_dnsSocket == 0){
	  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	  m_dnsSocket = Socket::CreateSocket (GetNode (), tid);
	  m_dnsSocket->Bind();
	  m_dnsSocket->SetRecvCallback (MakeCallback (&RequestResponseClient::ResolveDns, this));
	  GetHostByName(m_url);
  }
  //ScheduleTransmit (Seconds (0.));
}

void 
RequestResponseClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }

  Simulator::Cancel (m_sendEvent);
}

void 
RequestResponseClient::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << dataSize);

  //
  // If the client is setting the echo packet data size this way, we infer
  // that she doesn't care about the contents of the packet at all, so 
  // neither will we.
  //
  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = dataSize;
}

uint32_t 
RequestResponseClient::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_size;
}

void 
RequestResponseClient::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (this << fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void 
RequestResponseClient::SetFill (uint8_t fill, uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << fill << dataSize);
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memset (m_data, fill, dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void 
RequestResponseClient::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << fill << fillSize << dataSize);
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  if (fillSize >= dataSize)
    {
      memcpy (m_data, fill, dataSize);
      m_size = dataSize;
      return;
    }

  //
  // Do all but the final fill.
  //
  uint32_t filled = 0;
  while (filled + fillSize < dataSize)
    {
      memcpy (&m_data[filled], fill, fillSize);
      filled += fillSize;
    }

  //
  // Last fill may be partial
  //
  memcpy (&m_data[filled], fill, dataSize - filled);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void 
RequestResponseClient::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  m_sendEvent = Simulator::Schedule (dt, &RequestResponseClient::Send, this);
}

void 
RequestResponseClient::Send (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());

  Ptr<Packet> p;
  if (m_dataSize)
    {
      //
      // If m_dataSize is non-zero, we have a data buffer of the same size that we
      // are expected to copy and send.  This state of affairs is created if one of
      // the Fill functions is called.  In this case, m_size must have been set
      // to agree with m_dataSize
      //
      NS_ASSERT_MSG (m_dataSize == m_size, "RequestResponseClient::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "RequestResponseClient::Send(): m_dataSize but no m_data");
      p = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      //
      // If m_dataSize is zero, the client has indicated that it doesn't care
      // about the data itself either by specifying the data size by setting
      // the corresponding attribute or by not calling a SetFill function.  In
      // this case, we don't worry about it either.  But we do allow m_size
      // to have a value different from the (zero) m_dataSize.
      //
      p = Create<Packet> (m_size);
    }
  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_txTrace (p);
  m_socket->SendTo(p,0,InetSocketAddress(Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));

  ++m_sent;

  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent " << m_size << " bytes to " <<
                   Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
    }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent " << m_size << " bytes to " <<
                   Ipv6Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
    }

  if (m_sent < m_count) 
    {
      ScheduleTransmit (m_interval);
    }
}

void
RequestResponseClient::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client received " << packet->GetSize () << " bytes from " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client received " << packet->GetSize () << " bytes from " <<
                       Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
                       Inet6SocketAddress::ConvertFrom (from).GetPort ());
        }
    }
}

void RequestResponseClient::SetURL(std::string url){
	m_url = url;
}

void RequestResponseClient::GetHostByName(std::string name){
	if(m_enableDNS){
		NS_LOG_INFO("DNS is enabled. Resolving URL: "<<m_url);
		if(!m_dnsResolved){
			Ptr<Packet> requestRR = Create<Packet> ();

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
			m_dnsSocket->SendTo(requestRR,0,InetSocketAddress(Ipv4Address::ConvertFrom(m_localDnsAddress), 53));
		}
		NS_LOG_INFO("Here");
		m_dnsResolved=true;
	}
}

void RequestResponseClient::ResolveDns(Ptr<Socket> dnsSocket){
	  Ptr<Packet> packet;
	  Address from;
	  while ((packet = dnsSocket->RecvFrom (from)))
	    {
	      if (InetSocketAddress::IsMatchingType (from))
	        {
	          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client received " << packet->GetSize () << " bytes from " <<
	                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
	                       InetSocketAddress::ConvertFrom (from).GetPort ());
	       }
	  DNSHeader DnsHeader;
	  packet->RemoveHeader(DnsHeader);
	  std::string forwardingAddress;
	  std::list <ResourceRecordHeader> answerList;
	  answerList = DnsHeader.GetAnswerList ();
	  forwardingAddress = answerList.begin ()->GetRData ().c_str();
	  NS_LOG_INFO("Receives Answer: "<<Ipv4Address(forwardingAddress.c_str()));
	  m_peerAddress = Ipv4Address(forwardingAddress.c_str());
	  ScheduleTransmit (Seconds (0.));
	    }
}
} // Namespace ns3
