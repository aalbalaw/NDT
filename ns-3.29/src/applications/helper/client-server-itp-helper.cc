/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
#include "client-server-itp-helper.h"
#include "ns3/client-over-itp.h"
#include "ns3/server-over-itp.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/boolean.h"

namespace ns3 {

ItpServerHelper::ItpServerHelper (uint16_t port)
{
  m_factory.SetTypeId (ServerOverItp::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void
ItpServerHelper::SetAttribute (
  std::string name,
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
ItpServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
ItpServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
ItpServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
ItpServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<ServerOverItp> ();
  node->AddApplication (app);

  return app;
}

ItpClientHelper::ItpClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (ClientOverItp::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

ItpClientHelper::ItpClientHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (ClientOverItp::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port));
}

ItpClientHelper::ItpClientHelper (Address localDnsaddress, uint16_t port, bool enableDNS)
{
  m_factory.SetTypeId (ClientOverItp::GetTypeId ());
  SetAttribute ("LocalDNSAddress", AddressValue (localDnsaddress));
  SetAttribute ("RemotePort", UintegerValue (port));
  SetAttribute ("EnableDNS", BooleanValue (enableDNS));
}

void
ItpClientHelper::SetAttribute (
  std::string name,
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
ItpClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

void
ItpClientHelper::SetURL (Ptr<Application> app, std::string url)
{
  app->GetObject<ClientOverItp>()->SetURL (url);
}

ApplicationContainer
ItpClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
ItpClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
ItpClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<ClientOverItp> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
