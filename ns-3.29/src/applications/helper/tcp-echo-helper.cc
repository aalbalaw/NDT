/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Federal University of Uberlandia
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Saulo da Mata <damata.saulo@gmail.com>
 */


#include "ns3/uinteger.h"
#include "tcp-echo-helper.h"
#include "ns3/boolean.h"
namespace ns3 {

TcpEchoServerHelper::TcpEchoServerHelper (uint16_t port)
{
  m_factory.SetTypeId (TcpEchoServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void
TcpEchoServerHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
TcpEchoServerHelper::Install (Ptr<Node> node)
{
  ApplicationContainer apps;
  Ptr<TcpEchoServer> server = m_factory.Create<TcpEchoServer> ();
  node->AddApplication (server);
  apps.Add (server);
  return apps;
}

ApplicationContainer
TcpEchoServerHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      Ptr<TcpEchoServer> server = m_factory.Create<TcpEchoServer> ();
      node->AddApplication (server);
      apps.Add (server);
    }
  return apps;
}



TcpEchoClientHelper::TcpEchoClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (TcpEchoClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

TcpEchoClientHelper::TcpEchoClientHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (TcpEchoClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port));
}

TcpEchoClientHelper::TcpEchoClientHelper (Ipv6Address address, uint16_t port)
{
  m_factory.SetTypeId (TcpEchoClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port));
}

TcpEchoClientHelper::TcpEchoClientHelper (Address localDnsaddress, uint16_t port, bool enableDNS)
{
  m_factory.SetTypeId (TcpEchoClient::GetTypeId ());
  SetAttribute ("LocalDNSAddress", AddressValue (localDnsaddress));
  SetAttribute ("RemotePort", UintegerValue (port));
  SetAttribute ("EnableDNS", BooleanValue (enableDNS));
}

void
TcpEchoClientHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
TcpEchoClientHelper::Install (Ptr<Node> node)
{
  ApplicationContainer apps;
  Ptr<TcpEchoClient> client = m_factory.Create<TcpEchoClient> ();
  node->AddApplication (client);
  apps.Add (client);
  return apps;
}

ApplicationContainer
TcpEchoClientHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      Ptr<TcpEchoClient> client = m_factory.Create<TcpEchoClient> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}

void
TcpEchoClientHelper::SetURL (Ptr<Application> app, std::string url)
{
  app->GetObject<TcpEchoClient>()->SetURL (url);
}

} // namespace ns3
