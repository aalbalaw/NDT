/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DNS_HELPER_H
#define DNS_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/enum.h"

//#include "ns3/dns.h"
#include "ns3/bind-server.h"
namespace ns3 {

class BindServerHelper
{
public:
	BindServerHelper (BindServer::ServerType serverType);
	void SetAttribute (std::string name, const AttributeValue &value);
  ApplicationContainer Install (Ptr<Node> node) const;
  ApplicationContainer Install (std::string nodeName) const;
  ApplicationContainer Install (NodeContainer c) const;
	void AddNSRecord (Ptr<Application> app, 
	                  std::string name,
	                  uint16_t nsClass, 
	                  uint16_t type, 
					  uint32_t TTL,
	                  std::string rData);
	void AddManifestRecord (Ptr<Application> app,
	                  std::string content_name,
	                  uint16_t nsClass,
	                  uint16_t type,
					  uint32_t TTL,
	                  std::string manifestRecord);
private:
  Ptr<Application> InstallPriv (Ptr<Node> node) const;
  ObjectFactory m_factory; //!< Object factory.
};
}
#endif /* DNS_HELPER_H */

