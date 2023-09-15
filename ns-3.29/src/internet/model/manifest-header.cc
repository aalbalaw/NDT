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

#include "manifest-header.h"
#include "ns3/address-utils.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ManifestHeader);

/* The magic values below are used only for debugging.
 * They can be used to easily detect memory corruption
 * problems so you can see the patterns in memory.
 */
ManifestHeader::ManifestHeader ()
  : m_numberOfChunks (0)
{
}
ManifestHeader::~ManifestHeader ()
{

}
void
ManifestHeader::SetServerAddress( Ipv4Address serverAddress )
{
  m_serverAddress = serverAddress;
}

void
ManifestHeader::SetNumberOfChunks( uint32_t numberOfChunks )
{
  m_numberOfChunks = numberOfChunks;
}

uint32_t
ManifestHeader::GetName () const
 {
	return m_nameHash;

 }


void ManifestHeader::SetName (uint32_t nameHash)
 {
	m_nameHash = nameHash;

 }

uint32_t
ManifestHeader::GetNumberOfChunks(void) const
{
  return m_numberOfChunks;
}

Ipv4Address
ManifestHeader::GetServerAddress(void) const
{
  return m_serverAddress;
}

TypeId 
ManifestHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ManifestHeader")
    .SetParent<Header> ()
    .SetGroupName ("Internet")
    .AddConstructor<ManifestHeader> ()
  ;
  return tid;
}

TypeId 
ManifestHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void 
ManifestHeader::Print (std::ostream &os) const
{
  os << "length: " << GetSerializedSize ()
     << " " 
     << m_numberOfChunks << " > " << m_serverAddress
  ;
}

uint32_t 
ManifestHeader::GetSerializedSize (void) const
{
  return 12;
}

void
ManifestHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteHtonU32(m_numberOfChunks);
  i.WriteHtonU32(m_serverAddress.Get());
  i.WriteHtonU32(m_nameHash);
}
uint32_t
ManifestHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_numberOfChunks = i.ReadNtohU32 ();
  m_serverAddress.Set(i.ReadNtohU32 ());
  m_nameHash = i.ReadNtohU32();

  return GetSerializedSize ();
}


} // namespace ns3
