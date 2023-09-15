// -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*-
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: George F. Riley<riley@ece.gatech.edu>
//

#ifndef ITP_CACHE_H
#define ITP_CACHE_H

#include <list>
#include <map>
#include <vector>
#include <stdint.h>
#include "ns3/ipv4-address.h"
#include "ns3/ptr.h"
#include "ns3/net-device.h"
#include "ns3/ipv4.h"
#include "ns3/traced-callback.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include <vector>
#include "ns3/udp-header.h"
#include "ns3/itp-header.h"

namespace ns3 {

class Packet;
class NetDevice;
class Ipv4Interface;
class Ipv4Address;
class Ipv4Header;
class Node;
class IpL4Protocol;


class ItpCache: public Object
{
public:

	static TypeId GetTypeId (void);
	ItpCache();
	~ItpCache();


   /*
    * Check Packet type
    */
   Ptr<Packet> CheckPacket(Ptr<Packet> p);

   /*
    * Cache the received data
    */
   Ptr<Packet> OnData(Ptr<Packet> data);

   void OnData( uint32_t seq);

   Ptr<Packet>  OnInterest(Ptr<Packet> interest);

   UdpHeader GetNewUdpHdr(UdpHeader oldUdpHdr);

   Ipv4Header GetNewIpHdr(Ipv4Header oldIpHdr, int payloadSize);
   ItpHeader GetNewItpHdr (ItpHeader oldItpHdr);

   void populateContentStore (std::vector<int>contentStore);
   void populateContentStore ();


private:
   uint32_t m_payloadSize;
   struct ItpCacheEntry {
   		 uint32_t contentName;
   		 std::vector<uint32_t> seq=std::vector<uint32_t>(14000,0);
   	   };


   typedef std::vector<struct ItpCache::ItpCacheEntry> CachedContentsList;

   CachedContentsList m_cachedContents;
   uint32_t m_satesfiedRequests;
   std::vector<int> m_oneContent;
   bool flag;
   int m_oldChunks;
   int m_coSize;
};
}
#endif

