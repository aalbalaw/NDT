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

#ifndef ITP_SOURCE_H
#define ITP_SOURCE_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/itp.h"

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup requestresponse
 * \brief A Request Response client
 *
 * Every packet sent should be returned by the server and received here.
 */
class ItpSource : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  ItpSource ();

  virtual ~ItpSource ();

  /**
   * \brief set the remote address and port
   * \param ip remote IP address
   * \param port remote port
   */
  void SetRemote (Address ip, uint16_t port);
  /**
   * \brief set the remote address
   * \param addr remote address
   */
  void SetRemote (Address addr);

  /**
   * Set the data size of the packet (the number of bytes that are sent as data
   * to the server).  The contents of the data are set to unspecified (don't
   * care) by this call.
   *
   * \warning If you have set the fill data for the echo client using one of the
   * SetFill calls, this will undo those effects.
   *
   * \param dataSize The size of the echo data you want to sent.
   */
  void SetDataSize (uint32_t dataSize);

  /**
   * Get the number of data bytes that will be sent to the server.
   *
   * \warning The number of bytes may be modified by calling any one of the 
   * SetFill methods.  If you have called SetFill, then the number of 
   * data bytes will correspond to the size of an initialized data buffer.
   * If you have not called a SetFill method, the number of data bytes will
   * correspond to the number of don't care bytes that will be sent.
   *
   * \returns The number of data bytes.
   */
  uint32_t GetDataSize (void) const;

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * \brief Schedule the next packet transmission
   * \param dt time interval between packets.
   */
  void ScheduleTransmit (Time dt);
  /**
   * \brief Send a packet
   */
  void Send (void);

  /**
   * \brief Handle a packet reception.
   *
   * This function is called by lower layers.
   *
   * \param socket the socket the packet was received to.
   */
 // void HandleRead (Ptr<Socket> socket);


  void HandleRead (Address from);

  uint32_t m_count; //!< Maximum number of packets the application will send
  Time m_interval; //!< Packet inter-send time
  uint32_t m_size; //!< Size of the sent packet

  uint32_t m_dataSize; //!< packet payload size (must be equal to m_size)
  uint8_t *m_data; //!< packet payload data

  uint32_t m_sent; //!< Counter for sent packets

  Ptr<itp> m_itp;
  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort; //!< Remote peer port
  EventId m_sendEvent; //!< Event to send the next packet
  bool server2;

};

} // namespace ns3

#endif /* REQUEST_RESPONSE_CLIENT_H */
