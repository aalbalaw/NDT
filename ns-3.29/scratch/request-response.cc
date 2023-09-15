/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

// Network topology
//
//       n0    n1   n2   n3
//       |     |    |    |
//       =================
//              LAN
//
// - UDP flows from n0 to n1 and back
// - DropTail queues
// - Tracing of queues and packet receptions to file "request-response.tr"

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4.h"



using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RequestResponseExample");

int
main (int argc, char *argv[])
{
//
// Users may find it convenient to turn on explicit debugging
// for selected modules; the below lines suggest how to do this
//
#if 0
  LogComponentEnable ("RequestResponseExample", LOG_LEVEL_INFO);
  LogComponentEnable ("RequestResponseClientApplication", LOG_LEVEL_ALL);
  LogComponentEnable ("RequestResponseServerApplication", LOG_LEVEL_ALL);
#endif


  Ipv4Address serverAddress;
  Ipv4Address localDnsAddress;
  Ipv4Address authAddress;
  Ipv4Address tldAddress;
  Ipv4Address rootAddress;


//
// Explicitly create the nodes required by the topology (shown above).
//
  NS_LOG_INFO ("Create nodes.");
  Ptr<Node> clientNode = CreateObject<Node> ();
  Ptr<Node> serverNode = CreateObject<Node> ();
  Ptr<Node> authNode = CreateObject<Node> ();
  Ptr<Node> tldNode = CreateObject<Node> ();
  Ptr<Node> rootNode = CreateObject<Node> ();
  Ptr<Node>	localDNS = CreateObject<Node> ();
  Ptr<Node>	gw = CreateObject<Node> ();

  NodeContainer clientGW(clientNode,gw);
  NodeContainer localDnsGW(localDNS,gw);
  NodeContainer GWSever(gw, serverNode);
  NodeContainer rootGW(rootNode,gw);
  NodeContainer tldGW(gw, tldNode);
  NodeContainer authGW(authNode,gw);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));


  NetDeviceContainer nd1 = p2p.Install(clientGW);
  NetDeviceContainer nd2 = p2p.Install(localDnsGW);
  NetDeviceContainer nd3 = p2p.Install(GWSever);
  NetDeviceContainer nd4 = p2p.Install(rootGW);
  NetDeviceContainer nd5 = p2p.Install(tldGW);
  NetDeviceContainer nd6 = p2p.Install(authGW);

  InternetStackHelper internet;
  internet.InstallAll();

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.255.0");
  ipv4.Assign (nd1);
  ipv4.NewNetwork ();
  ipv4.Assign (nd2);
  ipv4.NewNetwork ();
  ipv4.Assign (nd3);
  ipv4.NewNetwork ();
  ipv4.Assign (nd4);
  ipv4.NewNetwork ();
  ipv4.Assign (nd5);
  ipv4.NewNetwork ();
  ipv4.Assign (nd6);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  serverAddress = serverNode->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.

  localDnsAddress = localDNS->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.

  authAddress = authNode->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.

  tldAddress = tldNode->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.

  rootAddress = rootNode->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.


  NS_LOG_INFO ("Create Applications.");
  std::cout<<"Server: "<<serverAddress<<"\t Auth.:"<<authAddress<<"\t tld: "<<tldAddress<<"\t root: "<<rootAddress<<"\t local:"<<localDnsAddress<<std::endl;
//
// Create a RequestResponseServer application on node one.
//
  uint16_t port = 9;  // well-known echo port number
  RequestResponseServerHelper server (port);
  ApplicationContainer apps = server.Install (serverNode);
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0));

//
// Create a RequestResponseClient application to send UDP datagrams from node zero to
// node one.
//
  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 1;
  Time interPacketInterval = Seconds (1.);
  RequestResponseClientHelper client (localDnsAddress, port, true);
  client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
  ApplicationContainer appClient = client.Install (clientNode);
  client.SetURL(appClient.Get(0), ".soe.ucsc.edu");
  appClient.Start (Seconds (2.0));
  appClient.Stop (Seconds (10.0));

  BindServerHelper localNs (BindServer::LOCAL_SERVER);
  localNs.SetAttribute ("SetRecursiveSupport", EnumValue (BindServer::RA_AVAILABLE));
  localNs.SetAttribute ("SetServerAddress",Ipv4AddressValue (localDnsAddress));
  localNs.SetAttribute ("RootServerAddress", Ipv4AddressValue (rootAddress));
  ApplicationContainer appsLocal = localNs.Install (localDNS);

  std::stringstream localDNSAddress;
  std::string serverAddressStr;

  localDNSAddress << authAddress;
  serverAddressStr = localDNSAddress.str ();
  localNs.AddNSRecord (appsLocal.Get (0),
					  ".ucsc.edu",
					  1,
					  1,
					  3500,
					  serverAddressStr);
  appsLocal.Start (Seconds (0.0));
  appsLocal.Stop (Seconds (10.0));

  BindServerHelper rootNs (BindServer::ROOT_SERVER);
  rootNs.SetAttribute ("SetServerAddress", Ipv4AddressValue (rootAddress));
  ApplicationContainer appsRoot = rootNs.Install (rootNode);\
  serverAddressStr = "";
  std::stringstream rootDNSAddress;
  rootDNSAddress << tldAddress;
  serverAddressStr = rootDNSAddress.str ();
  rootNs.AddNSRecord (appsRoot.Get (0),
						  ".edu", /*domain*/
						  1, /*class*/
						  1, /*type*/
						  86400, /*TTL*/
						  serverAddressStr /*resource record*/);
	appsRoot.Start (Seconds (0.0));
	appsRoot.Stop (Seconds (10.0));

  BindServerHelper tldNs (BindServer::TLD_SERVER);
  tldNs.SetAttribute ("SetServerAddress", Ipv4AddressValue (tldAddress));
  ApplicationContainer appsTld = tldNs.Install (tldNode);
  serverAddressStr = "";
  std::stringstream tldDNSAddress;
  tldDNSAddress << authAddress;
  serverAddressStr = tldDNSAddress.str ();
  tldNs.AddNSRecord (appsTld.Get (0),
                          ".ucsc.edu",
                          1,
                          1,
						  86400,
                          serverAddressStr);

  appsTld.Start (Seconds (0.0));
  appsTld.Stop (Seconds (10.0));

  BindServerHelper authNs (BindServer::AUTH_SERVER);
  authNs.SetAttribute ("SetServerAddress",Ipv4AddressValue (authAddress));
  ApplicationContainer appsAuth = authNs.Install (authNode);
  std::stringstream authDNSAddress;
  authDNSAddress << serverAddress;
  serverAddressStr = authDNSAddress.str ();
  authNs.AddNSRecord (appsAuth.Get (0),
					  ".soe.ucsc.edu",
					  1,
					  1,
					  3500,
					  serverAddressStr);

  appsAuth.Start (Seconds (0.0));
  appsAuth.Stop (Seconds (10.0));


//
// Now, do the actual simulation.
//
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
