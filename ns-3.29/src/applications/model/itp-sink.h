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

#ifndef ITP_SINK_H
#define ITP_SINK_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"
#include "ns3/socket.h"
#include "ns3/itp.h"

namespace ns3 {

//class Socket;
//class Packet;

/**
 * \ingroup applications 
 * \defgroup requestresponse RequestResponse
 */

/**
 * \ingroup requestresponse
 * \brief A Request Response server
 *
 * Every packet received is sent back.
 */
class ItpSink : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  ItpSink ();
  virtual ~ItpSink ();

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * \brief Handle a packet reception.
   *
   * This function is called by lower layers.
   *
   * \param socket the socket the packet was received to.
   */
  void HandleRead (Address from);

  uint16_t m_port; //!< Port on which we listen for incoming packets.
  Ptr<itp> m_itp;
  Address m_local; //!< local multicast address

};

} // namespace ns3

#endif /* REQUEST_RESPONSE_SERVER_H */

