/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005 INRIA
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
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "itp-header.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("itpHeader");
NS_OBJECT_ENSURE_REGISTERED (ItpHeader);

/* The magic values below are used only for debugging.
 * They can be used to easily detect memory corruption
 * problems so you can see the patterns in memory.
 */
ItpHeader::ItpHeader ()
  : m_type (0xfffd),
    m_seqNum (0xfffd)

{
}
ItpHeader::~ItpHeader ()
{
  m_type = 0xff;
  m_seqNum = 0xfffe;
}

void 
ItpHeader::SetSeqNumber (uint16_t seq)
{
  m_seqNum = seq;
}

void 
ItpHeader::SetType (uint16_t type)
{
  m_type = type;
}

void
 ItpHeader::SetName (uint32_t name)
 {

  m_nameHash=name;

 }

void
ItpHeader::SetCounter (uint32_t count)
{
  m_count = count;
}

uint16_t
ItpHeader::GetSeqNumber (void) const
{
  return m_seqNum;
}

uint16_t
ItpHeader::GetType (void) const
{
  return m_type;
}

uint32_t
 ItpHeader::GetName () const
 {
	return m_nameHash;

 }

uint32_t
ItpHeader::GetCounter (void) const
{
  return m_count;
}

TypeId 
ItpHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ItpHeader")
    .SetParent<Header> ()
    .SetGroupName ("Internet")
    .AddConstructor<ItpHeader> ()
  ;
  return tid;
}

TypeId 
ItpHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void 
ItpHeader::Print (std::ostream &os) const
{
  os << "length: " << GetSerializedSize ()
     << " " 
     << m_seqNum << " > " << m_type
  ;
}

uint32_t 
ItpHeader::GetSerializedSize (void) const
{
  return 12;
}

void
ItpHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteHtonU16 (m_type);
  i.WriteHtonU16 (m_seqNum);
  i.WriteHtonU32(m_nameHash);
  i.WriteHtonU32(m_count);

}
uint32_t
ItpHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_type = i.ReadNtohU16 ();
  m_seqNum = i.ReadNtohU16 ();
  m_nameHash=i.ReadNtohU32();
  m_count = i.ReadNtohU32();
  return GetSerializedSize ();
}

} // namespace ns3
