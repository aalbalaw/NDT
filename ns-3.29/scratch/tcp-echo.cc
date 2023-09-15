/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Federal University of Uberlandia
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
 * along with this program; if not, see <echo://www.gnu.org/licenses/>.
 *
 * Author: Saulo da Mata <damata.saulo@gmail.com>
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-helper.h"

//#include "ns3/lte-helper.h"
//#include "ns3/epc-helper.h"
//#include "ns3/network-module.h"
//#include "ns3/ipv4-global-routing-helper.h"
//#include "ns3/mobility-module.h"
//#include "ns3/lte-module.h"
//#include "ns3/config-store.h"
//#include "ns3/random-variable-stream.h"
//#include "ns3/buildings-helper.h"
//#include "ns3/flow-monitor-helper.h"
//#include "ns3/evalvid-client-server-helper.h"
//#include <map>
//#include "ns3/output-stream-wrapper.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("HttpClientServerExample");

int main(int argc, char *argv[]) {
	//Enabling logging

	Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(500));
	Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(10000000));
	Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(10000000));
	Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(1));
	Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(1));
	//Config::SetDefault("ns3::TcpSocket::TcpNoDelay", BooleanValue(true));

	NS_LOG_INFO("Creating nodes...");

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

	std::stringstream localDNSAddress;
	std::string serverAddressStr;

	uint32_t contentSize = 2000000;
	uint16_t port = 81;

	ApplicationContainer echoServerApps;
	ApplicationContainer echoClientApps;

	TcpEchoServerHelper echoServer(port);
	echoServerApps.Add(echoServer.Install(serverNode));
	echoServerApps.Start(Seconds(0.0));
	echoServerApps.Stop(Seconds(50.0));

	BindServerHelper authNs(BindServer::AUTH_SERVER);
	authNs.SetAttribute("SetServerAddress", Ipv4AddressValue(authAddress));
	ApplicationContainer appsAuth = authNs.Install(authNode);
	std::stringstream authDNSAddress;
	authDNSAddress << serverAddress;
	serverAddressStr = authDNSAddress.str();
	authNs.AddNSRecord(appsAuth.Get(0), "www.example.com/index.html", 3500, 1,
						1, serverAddressStr);

	appsAuth.Start(Seconds(0.0));
	appsAuth.Stop(Seconds(50.0));

	TcpEchoClientHelper echoClient(localDnsAddress, port, true);
	echoClientApps.Add(echoClient.Install(clientNode));
	echoClient.SetURL(echoClientApps.Get(0), "www.example.com/index.html");
	echoClientApps.Start(Seconds(0.0));
	echoClientApps.Stop(Seconds(50.0));

	BindServerHelper localNs(BindServer::LOCAL_SERVER);
	localNs.SetAttribute("SetRecursiveSupport",
			EnumValue(BindServer::RA_AVAILABLE));
	localNs.SetAttribute("SetServerAddress", Ipv4AddressValue(localDnsAddress));
	localNs.SetAttribute("RootServerAddress", Ipv4AddressValue(rootAddress));
	ApplicationContainer appsLocal = localNs.Install(localDNS);

	localDNSAddress << serverAddress;
	serverAddressStr = localDNSAddress.str();
	localNs.AddNSRecord(appsLocal.Get(0), "www.example.com/index.html", 3500, 1,
			1, serverAddressStr);
	appsLocal.Start(Seconds(0.0));
	appsLocal.Stop(Seconds(50.0));

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

	Simulator::Stop(Seconds(50.0));

	NS_LOG_INFO("Starting Simulation...");
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_INFO("\ndone!");
	return 0;

}
