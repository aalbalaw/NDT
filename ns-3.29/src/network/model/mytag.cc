#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include <iostream>
#include "mytag.h"

namespace ns3 {



TypeId
MyTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MyTag")
    .SetParent<Tag> ()
    .AddConstructor<MyTag> ()
  ;
  return tid;
}
TypeId
MyTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t
MyTag::GetSerializedSize (void) const
{
  return 13;
}
void
MyTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_isPW);
  i.WriteU16(m_dstPort);
  i.WriteU16(m_srcPort);
  i.WriteU32(m_dstIP.Get());
  i.WriteU32(m_srcIP.Get());
}
void
MyTag::Deserialize (TagBuffer i)
{
	m_isPW = i.ReadU8 ();
	m_dstPort = i.ReadU16();
	m_srcPort = i.ReadU16();
	m_dstIP  = Ipv4Address(i.ReadU32());
	m_srcIP  = Ipv4Address(i.ReadU32());

}
void
MyTag::Print (std::ostream &os) const
{
  os << "v=" << (bool)m_isPW;
}
void
MyTag::SetFlag ()
{
	m_isPW = true;
}
bool
MyTag::GetFlag (void) const
{
  return m_isPW;
}

void MyTag::SetDestinationIP(Ipv4Address dest){
	m_dstIP = dest;
}

Ipv4Address MyTag::GetDestinationIP(){
	return m_dstIP;
}

void MyTag::SetSourceIP(Ipv4Address src){
	m_srcIP = src;
}

Ipv4Address MyTag::GetSourceIP(){
	return m_srcIP;
}


void MyTag::SetDestinationPort(uint16_t dest){
	m_dstPort = dest;
}

uint16_t MyTag::GetDestinationPort(){
	return m_dstPort;
}

void MyTag::SetSourcePort(uint16_t src){
	m_srcPort = src;
}

uint16_t MyTag::GetSourcePort(){
	return m_srcPort;
}

}
