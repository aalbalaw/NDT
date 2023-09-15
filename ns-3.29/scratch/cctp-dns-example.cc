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
#include "ns3/encode-decode.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RequestResponseExample");

int main(int argc, char *argv[]) {

	Config::SetDefault("ns3::producer::PayloadSize", UintegerValue(500));
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
	Ptr<Node> localDNS = CreateObject<Node>();
	Ptr<Node> gw1 = CreateObject<Node>();
	Ptr<Node> gw2 = CreateObject<Node>();
	Ptr<Node> serverNode = CreateObject<Node>();
	Ptr<Node> authNode = CreateObject<Node>();
	Ptr<Node> rootNode = CreateObject<Node>();

	NodeContainer clientGw1(clientNode, gw1);
	NodeContainer localDnsGw1(localDNS, gw1);

	NodeContainer gw1gw2(gw1, gw2);

	NodeContainer gw2Sever(gw2, serverNode);
	NodeContainer authGw2(gw2, authNode);

	NodeContainer rootClient(rootNode, gw1);

	PointToPointHelper p2pClient;
	p2pClient.SetDeviceAttribute("DataRate", StringValue("1000Mbps"));
	p2pClient.SetChannelAttribute("Delay", StringValue("1ms"));

	PointToPointHelper p2pGW;
	p2pGW.SetDeviceAttribute("DataRate", StringValue("1000Mbps"));
	p2pGW.SetChannelAttribute("Delay", StringValue("19ms"));

	PointToPointHelper p2pServer;
	p2pServer.SetDeviceAttribute("DataRate", StringValue("1000Mbps"));
	p2pServer.SetChannelAttribute("Delay", StringValue("10ms"));

	NetDeviceContainer nd0 = p2pClient.Install(rootClient);
	NetDeviceContainer nd1 = p2pClient.Install(clientGw1);
	NetDeviceContainer nd2 = p2pClient.Install(localDnsGw1);

	NetDeviceContainer nd3 = p2pGW.Install(gw1gw2);

	NetDeviceContainer nd4 = p2pServer.Install(gw2Sever);
	NetDeviceContainer nd5 = p2pServer.Install(authGw2);

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
	ipv4.Assign(nd0);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	serverAddress = serverNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	localDnsAddress = localDNS->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	authAddress = authNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	rootAddress = rootNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	NS_LOG_INFO("Create Applications.");
	std::cout << "Server: " << serverAddress << "\t Auth.:" << authAddress
			<< "\t local:" << localDnsAddress << "\t root:" << rootAddress
			<< std::endl;
//
// Server Network
//
	std::stringstream localDNSAddress;
	std::string serverAddressStr;

	uint32_t contentSize = 2000000;
	uint16_t port = 81;

	CCTPServerHelper server(port);
	server.SetAttribute("ContentSize", UintegerValue(contentSize = 1000000));
	ApplicationContainer apps = server.Install(serverNode);
	apps.Start(Seconds(1.0));
	apps.Stop(Seconds(30.0));

	BindServerHelper authNs(BindServer::AUTH_SERVER);
	authNs.SetAttribute("SetServerAddress", Ipv4AddressValue(authAddress));
	ApplicationContainer appsAuth = authNs.Install(authNode);
	std::stringstream authDNSAddress;
	authDNSAddress << serverAddress;
	serverAddressStr = authDNSAddress.str();
	authNs.AddManifestRecord(appsAuth.Get(0), "www.example.com/index.html", 1,
			2020, 3500, ManifestToString(1, serverAddress));
	authNs.AddManifestRecord(appsAuth.Get(0), "www.example.com/index1.html", 1,
			2020, 3500, ManifestToString(1, serverAddress));

	appsAuth.Start(Seconds(0.0));
	appsAuth.Stop(Seconds(30.0));

//
// Client Network
//
	uint32_t packetSize = 100;
	Time interPacketInterval = Seconds(1.);
	CCTPClientHelper client(localDnsAddress, port, true);
	client.SetAttribute("Interval", TimeValue(interPacketInterval));
	client.SetAttribute("PacketSize", UintegerValue(packetSize));
	ApplicationContainer appClient = client.Install(clientNode);
	client.SetURL(appClient.Get(0), "www.example.com/index.html");
	appClient.Start(Seconds(2.0));
	appClient.Stop(Seconds(30.0));

	BindServerHelper localNs(BindServer::LOCAL_SERVER);
	localNs.SetAttribute("SetRecursiveSupport",
			EnumValue(BindServer::RA_AVAILABLE));
	localNs.SetAttribute("SetServerAddress", Ipv4AddressValue(localDnsAddress));
	localNs.SetAttribute("RootServerAddress", Ipv4AddressValue(rootAddress));
	ApplicationContainer appsLocal = localNs.Install(localDNS);

	localDNSAddress << serverAddress;
	serverAddressStr = localDNSAddress.str();
//	localNs.AddManifestRecord(appsLocal.Get(0), "http://www.example.com/index.html", 1, 2020,
//			3500, ManifestToString(1, serverAddress));
//	localNs.AddManifestRecord(appsLocal.Get(0), "http://www.example.com/index1.html", 1, 2020,
//			3500, ManifestToString(1, serverAddress));

	BindServerHelper rootNs(BindServer::TLD_SERVER);
	rootNs.SetAttribute("SetServerAddress", Ipv4AddressValue(rootAddress));
	ApplicationContainer appsRoot = rootNs.Install(rootNode);
	serverAddressStr = "";
	std::stringstream rootDNSAddress;
	rootDNSAddress << authAddress;
	serverAddressStr = rootDNSAddress.str();
	rootNs.AddNSRecord(appsRoot.Get(0), "www.example.com", /*domain*/
	1, /*class*/
	1, /*type*/
	86400, /*TTL*/
	serverAddressStr /*resource record*/);
	appsRoot.Start(Seconds(0.0));
	appsRoot.Stop(Seconds(10.0));

//
// Now, do the actual simulation.
//
	NS_LOG_INFO("Run Simulation.");
	Simulator::Run();
	Simulator::Stop(Seconds(30.0));
	NS_LOG_INFO("Done.");
}
