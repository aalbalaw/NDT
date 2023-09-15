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

NS_LOG_COMPONENT_DEFINE("RequestResponseExample");

int main(int argc, char *argv[]) {
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
	NS_LOG_INFO("Create nodes.");
	Ptr<Node> clientNode = CreateObject<Node>();
	Ptr<Node> client2Node = CreateObject<Node>();
	Ptr<Node> serverNode = CreateObject<Node>();
	Ptr<Node> authNode = CreateObject<Node>();
	Ptr<Node> tldNode = CreateObject<Node>();
	Ptr<Node> rootNode = CreateObject<Node>();
	Ptr<Node> localDNS = CreateObject<Node>();
	Ptr<Node> gw = CreateObject<Node>();

	NodeContainer clientGW(clientNode, gw);
	NodeContainer localDnsGW(localDNS, gw);
	NodeContainer GWSever(gw, serverNode);
	NodeContainer rootGW(rootNode, gw);
	NodeContainer tldGW(gw, tldNode);
	NodeContainer authGW(authNode, gw);
	NodeContainer client2GW(client2Node, gw);

	PointToPointHelper p2p;
	p2p.SetDeviceAttribute("DataRate", StringValue("1000Mbps"));
	p2p.SetChannelAttribute("Delay", StringValue("1ms"));

	PointToPointHelper p2pDns;
	p2pDns.SetDeviceAttribute("DataRate", StringValue("1000Mbps"));
	p2pDns.SetChannelAttribute("Delay", StringValue("1ms"));

	PointToPointHelper p2pServer;
	p2pServer.SetDeviceAttribute("DataRate", StringValue("1000Mbps"));
	p2pServer.SetChannelAttribute("Delay", StringValue("500ms"));


	NetDeviceContainer nd1 = p2p.Install(clientGW);
	NetDeviceContainer nd2 = p2pDns.Install(localDnsGW);
	NetDeviceContainer nd3 = p2pServer.Install(GWSever);
	NetDeviceContainer nd4 = p2p.Install(rootGW);
	NetDeviceContainer nd5 = p2p.Install(tldGW);
	NetDeviceContainer nd6 = p2p.Install(authGW);
	NetDeviceContainer nd7 = p2p.Install(client2GW);

	InternetStackHelper internet;
	internet.InstallAll();

	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.0.0.0", "255.255.255.0");
	ipv4.Assign(nd1);
	ipv4.NewNetwork();
	ipv4.Assign(nd2);
	ipv4.NewNetwork();
	ipv4.Assign(nd3);
	ipv4.NewNetwork();
	ipv4.Assign(nd4);
	ipv4.NewNetwork();
	ipv4.Assign(nd5);
	ipv4.NewNetwork();
	ipv4.Assign(nd6);
	ipv4.NewNetwork();
	ipv4.Assign(nd7);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	serverAddress = serverNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	localDnsAddress = localDNS->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	authAddress = authNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	tldAddress = tldNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	rootAddress = rootNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	NS_LOG_INFO("Create Applications.");
	std::cout << "Server: " << serverAddress << "\t Auth.:" << authAddress
			<< "\t tld: " << tldAddress << "\t root: " << rootAddress
			<< "\t local:" << localDnsAddress << std::endl;
//
// Server application
//
	uint32_t contentSize = 2000000;
	uint16_t port = 81;

	ItpServerHelper server(port);
	server.SetAttribute("ContentSize", UintegerValue(contentSize = 1000));
	ApplicationContainer apps = server.Install(serverNode);
	apps.Start(Seconds(1.0));
	apps.Stop(Seconds(30.0));

//
// Client application
//
	uint32_t packetSize = 100;
	Time interPacketInterval = Seconds(1.);
	ItpClientHelper client(localDnsAddress, port, true);
	client.SetAttribute("Interval", TimeValue(interPacketInterval));
	client.SetAttribute("PacketSize", UintegerValue(packetSize));
	ApplicationContainer appClient = client.Install(clientNode);
	client.SetURL(appClient.Get(0), "www.example.com");
	appClient.Start(Seconds(2.0));
	appClient.Stop(Seconds(30.0));

	BindServerHelper localNs(BindServer::LOCAL_SERVER);
	localNs.SetAttribute("SetRecursiveSupport",
			EnumValue(BindServer::RA_AVAILABLE));
	localNs.SetAttribute("SetServerAddress", Ipv4AddressValue(localDnsAddress));
	localNs.SetAttribute("RootServerAddress", Ipv4AddressValue(rootAddress));
	ApplicationContainer appsLocal = localNs.Install(localDNS);

	std::stringstream localDNSAddress;
	std::string serverAddressStr;

//	localDNSAddress << serverAddress;
//	serverAddressStr = localDNSAddress.str();
//	localNs.AddNSRecord(appsLocal.Get(0), "www.example.com", 3500, 1, 1,
//			serverAddressStr);
	appsLocal.Start(Seconds(0.0));
	appsLocal.Stop(Seconds(30.0));

	BindServerHelper rootNs(BindServer::ROOT_SERVER);
	rootNs.SetAttribute("SetServerAddress", Ipv4AddressValue(rootAddress));
	ApplicationContainer appsRoot = rootNs.Install(rootNode);
	serverAddressStr = "";
	std::stringstream rootDNSAddress;
	rootDNSAddress << tldAddress;
	serverAddressStr = rootDNSAddress.str();
	rootNs.AddNSRecord(appsRoot.Get(0), ".com", /*domain*/
	1, /*class*/
	1, /*type*/
	86400, /*TTL*/
	serverAddressStr /*resource record*/);
	appsRoot.Start(Seconds(0.0));
	appsRoot.Stop(Seconds(30.0));

	BindServerHelper tldNs(BindServer::TLD_SERVER);
	tldNs.SetAttribute("SetServerAddress", Ipv4AddressValue(tldAddress));
	ApplicationContainer appsTld = tldNs.Install(tldNode);
	serverAddressStr = "";
	std::stringstream tldDNSAddress;
	tldDNSAddress << authAddress;
	serverAddressStr = tldDNSAddress.str();
	tldNs.AddNSRecord(appsTld.Get(0), "www.example.com", 1, 1,
			86400,serverAddressStr);

	appsTld.Start(Seconds(0.0));
	appsTld.Stop(Seconds(30.0));

	BindServerHelper authNs(BindServer::AUTH_SERVER);
	authNs.SetAttribute("SetServerAddress", Ipv4AddressValue(authAddress));
	ApplicationContainer appsAuth = authNs.Install(authNode);
	std::stringstream authDNSAddress;
	authDNSAddress << serverAddress;
	serverAddressStr = authDNSAddress.str();
	authNs.AddNSRecord(appsAuth.Get(0), "www.example.com", 1, 1,
			3500,serverAddressStr);
	appsAuth.Start(Seconds(0.0));
	appsAuth.Stop(Seconds(30.0));


//	ItpClientHelper client2(localDnsAddress, port, true);
//	client2.SetAttribute("Interval", TimeValue(interPacketInterval));
//	client2.SetAttribute("PacketSize", UintegerValue(packetSize));
//	ApplicationContainer appClient2 = client2.Install(client2Node);
//	client2.SetURL(appClient2.Get(0), "www.example.com");
//	appClient2.Start(Seconds(8.0));
//	appClient2.Stop(Seconds(30.0));

//
// Now, do the actual simulation.
//
	NS_LOG_INFO("Run Simulation.");
	Simulator::Run();
	Simulator::Stop(Seconds(10.0));
	NS_LOG_INFO("Done.");
}
