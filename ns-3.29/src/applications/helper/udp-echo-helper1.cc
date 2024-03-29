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
#include "udp-echo-helper1.h"
#include "ns3/udp-echo-server1.h"
#include "ns3/udp-echo-client1.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

UdpEchoServer1Helper::UdpEchoServer1Helper (uint16_t port)
{
  m_factory.SetTypeId (UdpEchoServer1::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
UdpEchoServer1Helper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
UdpEchoServer1Helper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpEchoServer1Helper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpEchoServer1Helper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
UdpEchoServer1Helper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<UdpEchoServer1> ();
  node->AddApplication (app);

  return app;
}

UdpEchoClient1Helper::UdpEchoClient1Helper (Address address, uint16_t port)
{
  m_factory.SetTypeId (UdpEchoClient1::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

UdpEchoClient1Helper::UdpEchoClient1Helper (Address address)
{
  m_factory.SetTypeId (UdpEchoClient1::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
}

void 
UdpEchoClient1Helper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
UdpEchoClient1Helper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<UdpEchoClient1>()->SetFill (fill);
}

void
UdpEchoClient1Helper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
  app->GetObject<UdpEchoClient1>()->SetFill (fill, dataLength);
}

void
UdpEchoClient1Helper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
{
  app->GetObject<UdpEchoClient1>()->SetFill (fill, fillLength, dataLength);
}

ApplicationContainer
UdpEchoClient1Helper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpEchoClient1Helper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
UdpEchoClient1Helper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
UdpEchoClient1Helper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<UdpEchoClient1> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
