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
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4.h"
#include <string>
#include <fstream>
#include <iostream>
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RequestResponseExample");

std::ostringstream oss, oss1, oss2, oss3, source2traceCWND, source2traceRTT;
static uint32_t CWND = 0;
static uint32_t inFlight = 0;
static uint32_t QUEUE = 0;
static double rtt = 0;
static int counter = 0;

static uint32_t CWNDSource1 = 0;
static uint32_t CWNDSource2 = 0;
static double Source1rtt = 0;
static double Source2rtt = 0;

static Ptr<Ipv4FlowClassifier> classifier;
static Ptr<FlowMonitor> monitor;

static void Queues(uint32_t oldSize, uint32_t newSize) {
	QUEUE = newSize;
	//std::cout<<Simulator::Now ().GetSeconds ()<<"s\t"<<QUEUE<<std::endl;
}

static void CwndChange2(uint32_t oldCwnd, uint32_t newCwnd) {
	CWNDSource2 = newCwnd;
	double x = (newCwnd / 1200.0);
	std::cout << "\t" << x << "\t" << Source2rtt << "\n";
}

static void RTTChange2(Time oldRTT, Time newRTT) {

	Source2rtt = newRTT.ToDouble(Time::S);
}

void CwndConnect2() {

	Config::ConnectWithoutContext(source2traceCWND.str(),
			MakeCallback(&CwndChange2));
}

void RTTConnect2() {

	Config::ConnectWithoutContext(source2traceRTT.str(),
			MakeCallback(&RTTChange2));
}

void handler(void) {
	std::cout << Simulator::Now().GetSeconds() << "\n";
	Simulator::Schedule(Seconds((5)), &handler);
}

void GetStatsFlow1() {
	std::cout << "Flow 1" << std::endl;
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
			stats.begin(); i != stats.end(); ++i) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);

		if (t.sourceAddress == Ipv4Address("10.0.0.1")
				&& t.destinationAddress == Ipv4Address("10.0.2.2")) {
			std::cout << "Throughput: "
					<< i->second.rxBytes * 8.0
							/ (i->second.timeLastRxPacket.GetSeconds()
									- i->second.timeFirstTxPacket.GetSeconds())
							/ 1024 / 1024 << " Mbps\n";
			std::cout << "Jitter sum: " << i->second.jitterSum << "\n";
			std::cout << "delay sum: " << i->second.delaySum << "\n";
			std::cout << "received bytes: " << i->second.rxBytes << "\n";
			std::cout << "received packets: " << i->second.rxPackets << "\n";
			std::cout << "Lost Packets: " << i->second.lostPackets << "\n";
			std::cout << "Total Time: "
					<< (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds()
					<< std::endl;

		}
	}
}

void GetStatsFlow2() {
	std::cout << "Flow 2" << std::endl;
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
			stats.begin(); i != stats.end(); ++i) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);

		if (t.sourceAddress == Ipv4Address("10.0.0.1")
				&& t.destinationAddress == Ipv4Address("10.0.3.2")) {
			std::cout << "Throughput: "
					<< i->second.rxBytes * 8.0
							/ (i->second.timeLastRxPacket.GetSeconds()
									- i->second.timeFirstTxPacket.GetSeconds())
							/ 1024 / 1024 << " Mbps\n";
			std::cout << "Jitter sum: " << i->second.jitterSum << "\n";
			std::cout << "delay sum: " << i->second.delaySum << "\n";
			std::cout << "received bytes: " << i->second.rxBytes << "\n";
			std::cout << "received packets: " << i->second.rxPackets << "\n";
			std::cout << "Lost packets: " << i->second.lostPackets << "\n";
			std::cout << "Total Time: "
					<< (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds()
					<< std::endl;

		}
	}
}

static void StateChange(TcpSocketState::TcpCongState_t x,
		TcpSocketState::TcpCongState_t newValue) {
	std::cout << Simulator::Now().GetSeconds() << "s\t" << x << "\t" << newValue
			<< std::endl;
}

void StateConnect() {
	Config::ConnectWithoutContext(oss3.str(), MakeCallback(&StateChange));
}

int main(int argc, char *argv[]) {

	double c2Start, c3Start, c4Start, c5Start, c6Start;
	double c2Stop, c3Stop, c4Stop, c5Stop, c6Stop;

	c2Start = 0;
	c3Start = 0;
	c4Start = 0;
	c5Start = 0;
	c6Start = 0;

	c2Stop = c2Start + 50;
	c3Stop = c3Start + 50;
	c4Stop = c4Start + 50;
	c5Stop = c5Start + 50;
	c6Stop = c6Start+50;

	Config::SetDefault("ns3::QueueBase::MaxSize",
			QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, 1)));
	Config::SetDefault("ns3::producer::PayloadSize", UintegerValue(1000));
	Config::SetDefault("ns3::ItpCache::PayloadSize", UintegerValue(1000));
	Config::SetDefault("ns3::ItpCache::COSize", UintegerValue(6500));
	// Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (false));

	CommandLine cmd;
	int index;
	cmd.AddValue("index", "an int argument", index);
	cmd.Parse(argc, argv);

	uint32_t fileSize = 6000000;

	NodeContainer gwNode;
	gwNode.Create(1);

	NodeContainer gwNode2;
	gwNode2.Create(1);

	NodeContainer srcNode1;
	srcNode1.Create(1);

	NodeContainer consumer1;
	consumer1.Create(1);

	NodeContainer consumer2;
	consumer2.Create(1);

	NodeContainer consumer3;
	consumer3.Create(1);

	NodeContainer consumer4;
	consumer4.Create(1);

	NodeContainer consumer5;
	consumer5.Create(1);

	NodeContainer consumer6;
	consumer6.Create(1);

	NodeContainer proxy;
	proxy.Create(1);

	PointToPointHelper accessLink1;
	accessLink1.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
	accessLink1.SetChannelAttribute("Delay", StringValue("5ms"));
	accessLink1.SetQueue("ns3::DropTailQueue", "MaxSize",
			StringValue("140000B"));
	accessLink1.SetDeviceAttribute("Mtu", UintegerValue(1600));

	PointToPointHelper endLink1;
	endLink1.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
	endLink1.SetChannelAttribute("Delay", ("Delay", StringValue("2ms")));
	endLink1.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("400000B"));
	endLink1.SetDeviceAttribute("Mtu", UintegerValue(1600));

	PointToPointHelper cacheLink;
	cacheLink.SetDeviceAttribute("DataRate", StringValue("10000Mbps"));
	cacheLink.SetChannelAttribute("Delay", ("Delay", StringValue("0ms")));
	cacheLink.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("400000B"));
	cacheLink.SetDeviceAttribute("Mtu", UintegerValue(1600));

	NetDeviceContainer accessLink = accessLink1.Install(srcNode1.Get(0),
			gwNode.Get(0));
	NetDeviceContainer endLinkConsumer1 = endLink1.Install(gwNode.Get(0),
			consumer1.Get(0));
	NetDeviceContainer endLinkConsumer2 = endLink1.Install(gwNode.Get(0),
			consumer2.Get(0));
	NetDeviceContainer endLinkConsumer3 = endLink1.Install(gwNode.Get(0),
			consumer3.Get(0));
	NetDeviceContainer endLinkConsumer4 = endLink1.Install(gwNode.Get(0),
			consumer4.Get(0));
	NetDeviceContainer endLinkConsumer5 = endLink1.Install(gwNode.Get(0),
			consumer5.Get(0));
	NetDeviceContainer endLinkConsumer6 = endLink1.Install(gwNode.Get(0),
			consumer6.Get(0));
	NetDeviceContainer linkToProxy = cacheLink.Install(gwNode.Get(0),
			proxy.Get(0));

	InternetStackHelper internet;
	internet.InstallAll();

	Ptr<Ipv4L3Protocol> ipv4Instance =
			gwNode.Get(0)->GetObject<Ipv4L3Protocol>();

	std::string str = "10.0.6.2";
	ipv4Instance->SetCctpCache(str);

	TrafficControlHelper tch1;

	NS_LOG_INFO("Assign IP Addresses.");

	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.0.0.0", "255.255.255.0");
	ipv4.Assign(accessLink);

	ipv4.NewNetwork();
	ipv4.Assign(endLinkConsumer1);

	ipv4.NewNetwork();
	ipv4.Assign(endLinkConsumer2);

	ipv4.NewNetwork();
	ipv4.Assign(endLinkConsumer3);

	ipv4.NewNetwork();
	ipv4.Assign(endLinkConsumer4);

	ipv4.NewNetwork();
	ipv4.Assign(endLinkConsumer5);

	ipv4.NewNetwork();
	ipv4.Assign(linkToProxy);

	ipv4.NewNetwork();
	ipv4.Assign(endLinkConsumer6);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();
	tch1.Uninstall(accessLink);
	tch1.Uninstall(endLinkConsumer1);
	tch1.Uninstall(endLinkConsumer2);
	tch1.Uninstall(endLinkConsumer3);
	tch1.Uninstall(endLinkConsumer4);
	tch1.Uninstall(endLinkConsumer5);
	tch1.Uninstall(endLinkConsumer6);
	tch1.Uninstall(linkToProxy);

	Ptr<Node> node;
	Ptr<Ipv4> ipv4Addr;
	Ipv4Address sinkAddr1, sinkAddr2, srcAddr1, srcAddr2, cache;

	cache = proxy.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.
	std::cout << cache << std::endl;

	uint16_t port = 9;  // well-known echo port number
	CctpProxyHelper cctpProxy(port);
	ApplicationContainer cachApp = cctpProxy.Install(proxy.Get(0));
	cachApp.Start(Seconds(0));
	cachApp.Stop(Seconds(100.0));

	// Create Sink application on node one. The name is server but it acts as a sink in this application
	// well-known echo port number
	ItpSinkHelper sink(port);
	ApplicationContainer apps = sink.Install(consumer1.Get(0));
	apps.Start(Seconds(0));
	apps.Stop(Seconds(100.0));

	Ipv4Address addr;
	ipv4Addr = consumer1.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.
	uint32_t maxPacketCount = 1;
	Time interPacketInterval = Seconds(0);
	ItpSourceHelper source(addr, port);
	source.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	source.SetAttribute("Interval", TimeValue(interPacketInterval));
	source.SetAttribute("PacketSize", UintegerValue(fileSize));
	apps = source.Install(srcNode1.Get(0));
	apps.Start(Seconds(0));
	apps.Stop(Seconds(100.0));

	////// Consumer 2///////////////////////
	uint16_t port2 = 9;  // well-known echo port number
	ItpSinkHelper sink2(port2);
	ApplicationContainer apps2 = sink2.Install(consumer2.Get(0));
	apps2.Start(Seconds(c2Start));
	apps2.Stop(Seconds(c2Stop));

	Ipv4Address addr2;
	ipv4Addr = consumer2.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr2 = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	ItpSourceHelper source2(addr2, port2);
	source2.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	source2.SetAttribute("Interval", TimeValue(interPacketInterval));
	source2.SetAttribute("PacketSize", UintegerValue(fileSize));
	apps2 = source2.Install(srcNode1.Get(0));
	apps2.Start(Seconds(c2Start));
	apps2.Stop(Seconds(c2Stop));
	///////////////////////////////////////

	//////// Consumer 3////////////////////

	uint16_t port3 = 9;  // well-known echo port number
	ItpSinkHelper sink3(port3);
	ApplicationContainer apps3 = sink3.Install(consumer3.Get(0));
	apps3.Start(Seconds(c3Start));
	apps3.Stop(Seconds(c3Stop));

	Ipv4Address addr3;
	ipv4Addr = consumer3.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr3 = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	ItpSourceHelper source3(addr3, port3);
	source3.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	source3.SetAttribute("Interval", TimeValue(interPacketInterval));
	source3.SetAttribute("PacketSize", UintegerValue(fileSize));
	apps3 = source3.Install(srcNode1.Get(0));
	apps3.Start(Seconds(c3Start));
	apps3.Stop(Seconds(c3Stop));
	/////////////////////////////////////

	//////// Consumer 4//////////////////
	uint16_t port4 = 9;  // well-known echo port number
	ItpSinkHelper sink4(port4);
	ApplicationContainer apps4 = sink4.Install(consumer4.Get(0));
	apps4.Start(Seconds(c4Start));
	apps4.Stop(Seconds(c4Stop));

	Ipv4Address addr4;
	ipv4Addr = consumer4.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr4 = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	ItpSourceHelper source4(addr4, port4);
	source4.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	source4.SetAttribute("Interval", TimeValue(interPacketInterval));
	source4.SetAttribute("PacketSize", UintegerValue(fileSize));
	apps4 = source4.Install(srcNode1.Get(0));
	apps4.Start(Seconds(c4Start));
	apps4.Stop(Seconds(c4Stop));
	///////////////////////////////////////

	//////// Consumer 5/////////////////////
	uint16_t port5 = 9;  // well-known echo port number
	ItpSinkHelper sink5(port5);
	ApplicationContainer apps5 = sink5.Install(consumer5.Get(0));
	apps5.Start(Seconds(c5Start));
	apps5.Stop(Seconds(c5Stop));

	Ipv4Address addr5;
	ipv4Addr = consumer5.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr5 = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	ItpSourceHelper source5(addr5, port5);
	source5.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	source5.SetAttribute("Interval", TimeValue(interPacketInterval));
	source5.SetAttribute("PacketSize", UintegerValue(fileSize));
	apps5 = source5.Install(srcNode1.Get(0));
	apps5.Start(Seconds(c5Start));
	apps5.Stop(Seconds(c5Stop));

	//////// Consumer 6////////
	uint16_t port6 = 9;  // well-known echo port number
	ItpSinkHelper sink6(port6);
	ApplicationContainer apps6 = sink6.Install(consumer6.Get(0));
	apps6.Start(Seconds(c6Start));
	apps6.Stop(Seconds(c6Stop));

	Ipv4Address addr6;
	ipv4Addr = consumer6.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr6 = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	ItpSourceHelper source6(addr6, port6);
	source6.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	source6.SetAttribute("Interval", TimeValue(interPacketInterval));
	source6.SetAttribute("PacketSize", UintegerValue(fileSize));
	apps6 = source6.Install(srcNode1.Get(0));
	apps6.Start(Seconds(c6Start));
	apps6.Stop(Seconds(c6Stop));
	//////////////////////////////

//////////////////////////////////////
	NS_LOG_INFO("Run Simulation.");
	Simulator::Stop(Seconds(50.0));
	// Simulator::Schedule (Seconds (0.1), &StateConnect);
	// Simulator::Schedule (Seconds (0.1), &CwndConnect2);
	//Simulator::Schedule(Seconds(0.1), &handler);
	Simulator::Run();
	//GetStatsFlow1();
	//GetStatsFlow2();
	Simulator::Destroy();
	NS_LOG_INFO("Done.");

	// std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
}
