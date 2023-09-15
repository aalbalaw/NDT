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

#ifndef MANIFEST_HEADER_H
#define MANIFEST_HEADER_H

#include <stdint.h>
#include <string>
#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"

namespace ns3 {
/**
 * \ingroup udp
 * \brief Packet header for MANIFEST packets
 *
 * This class has fields corresponding to those in a network MANIFEST header
 * (port numbers, payload size, checksum) as well as methods for serialization
 * to and deserialization from a byte buffer.
 */
class ManifestHeader : public Header
{
public:

  /**
   * \brief Constructor
   *
   * Creates a null header
   */
  ManifestHeader ();
  ~ManifestHeader ();



  void SetNumberOfChunks (uint32_t number_of_chunks);

  void SetServerAddress (Ipv4Address serverAddress);

  /**
   * \return The source port for this ManifestHeader
   */
  uint32_t GetNumberOfChunks (void) const;

  /**
   * \return The source port for this ManifestHeader
   */
  Ipv4Address GetServerAddress (void) const;


  uint32_t GetName () const;

  void SetName (uint32_t hash);

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);


private:
  /**
   * \brief Calculate the header checksum
   * \param size packet size
   * \returns the checksum
   */
  uint32_t m_numberOfChunks;      //!< number of chunks to request
  Ipv4Address m_serverAddress;           //!< Source IP address
  uint32_t m_nameHash;
};

} // namespace ns3

#endif /* MANIFEST_HEADER */
