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

	Config::SetDefault("ns3::QueueBase::MaxSize",
			QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, 1)));
	Config::SetDefault("ns3::producer::PayloadSize", UintegerValue(1000));
	Config::SetDefault("ns3::ItpCache::PayloadSize", UintegerValue(1000));
	Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1000));
	// Config::SetDefault ("ns3::TcpSocketBase::ReTxThreshold", UintegerValue (3));
	//Config::SetDefault ("ns3::TcpSocketBase::Sack",  BooleanValue ("false"));
	Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(10000000));
	Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(10000000));
	Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(1));
	Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(1));
	Config::SetDefault("ns3::PacketSink::FileSize", UintegerValue(15000));
	Config::SetDefault("ns3::ItpCache::COSize", UintegerValue(6500));
	// Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (false));

	//// Scenario 1 /////
	double client2Scenario1Start[] = { 5.98058, 5.78041, 4.21223, 2.99688,
			1.21613 };

	//// Scenario 2 /////
	double client2Scenario2Start[] = { 2.93422, 1.8217, 1.62157, 0.0533425,
			2.7431 };
	double client3Scenario2Start[] = { 2.33778, 2.37333, 4.26337, 2.64134,
			0.780135 };
	double client4Scenario2Start[] = { 1.70434, 2.29814, 4.88469, 2.39682,
			0.712898 };

	//// Scenario 3 /////
	double client2Scenario3Start[] = { 1.6911, 5.20755, 4.32035, 1.18681,
			4.79643 };
	double client3Scenario3Start[] = { 1.83058, 2.33351, 4.58353, 0.778544,
			1.21188 };
	double client4Scenario3Start[] =
			{ 3.6604, 4.08715, 1.10387, 3.9704, 4.04059 };//// Scenario 8 /////
	double client5Scenario3Start[] = { 1.59836, 0.571071, 4.60907, 0.797536,
			2.4519 };
	double client6Scenario3Start[] = { 1.58991, 1.80406, 5.80093, 0.630867,
			5.21583 };

	//// Scenario 4 /////
	double client2Scenario4Start[] = { 5.44482, 4.45046, 3.85062, 0.107164,
			5.1005 };
	double client3Scenario4Start[] = { 0.72581, 1.52211, 0.104522, 4.10622,
			1.67712 };
	double client4Scenario4Start[] = { 5.3825, 0.296327, 1.08883, 4.66325,
			4.09387 };
	double client5Scenario4Start[] = { 4.77887, 1.04336, 0.683548, 1.31865,
			2.2422 };
	double client6Scenario4Start[] = { 4.31223, 2.79775, 4.25389, 5.96488,
			2.18608 };
	double client7Scenario4Start[] = { 0.800247, 4.36944, 2.75708, 0.831122,
			3.94668 };
	double client8Scenario4Start[] = { 3.19541, 2.72094, 3.72346, 4.17393,
			2.64162 };

	//// Scenario 5 /////
	double client2Scenario5Start[] = { 0.474406, 4.10602, 1.00799, 0.579411,
			4.43047 };
	double client3Scenario5Start[] =
			{ 2.1017, 3.34969, 1.9717, 1.36654, 2.18149 };
	double client4Scenario5Start[] = { 1.64899, 4.92915, 1.39499, 1.1571,
			0.754676 };
	double client5Scenario5Start[] = { 3.24004, 4.15506, 4.11858, 2.96352,
			3.97744 };
	double client6Scenario5Start[] = { 3.01943, 0.337153, 1.63934, 0.628648,
			1.33398 };
	double client7Scenario5Start[] = { 0.648888, 0.910357, 1.45891, 2.03344,
			0.847217 };
	double client8Scenario5Start[] = { 0.576179, 0.950862, 2.07486, 5.03116,
			3.18301 };
	double client9Scenario5Start[] = { 1.67909, 5.77727, 2.61171, 2.23507,
			1.30279 };
	double client10Scenario5Start[] = { 2.93798, 0.110162, 0.475862, 1.23571,
			3.35787 };

	double c2Start, c3Start, c4Start, c5Start, c6Start, c7Start, c8Start,
			c9Start, c10Start;
	double c2Stop, c3Stop, c4Stop, c5Stop, c6Stop, c7Stop, c8Stop, c9Stop,
			c10Stop;

	bool NDTP, NP, my_DNS = false;

	NDTP = false;
	NP = true;

	CommandLine cmd;
	int index;
	cmd.AddValue("index", "an int argument", index);
	cmd.Parse(argc, argv);

	c2Start = client2Scenario5Start[index];
	c3Start = client3Scenario5Start[index];
	c4Start = client4Scenario5Start[index];
	c5Start = client5Scenario5Start[index];
	c6Start = client6Scenario5Start[index];
	c7Start = client7Scenario5Start[index];
	c8Start = client8Scenario5Start[index];
	c9Start = client9Scenario5Start[index];
	c10Start = client10Scenario5Start[index];

//	c5Start = 15;
//	c6Start = c5Start+15;
//	c2Start = c6Start+15;
//	c4Start = c2Start+15;
//	c3Start = c4Start+15;
	//c8Start = ;
	//c9Start = ;
	//c10Start = ;

	c2Stop = c2Start + 20;
	c3Stop = c3Start + 20;
	c4Stop = c4Start + 20;
	c5Stop = c5Start + 20;
	c6Stop = c6Start + 20;
	c7Stop = c7Start + 20;
	c8Stop = c8Start + 20;
	c9Stop = c9Start + 20;
	c10Stop = c10Start + 20;

	std::cout << "######### Scenario " << index + 1 << " #########\n";
	std::cout << "Client 2 start: " << c2Start << std::endl;
	std::cout << "Client 3 start: " << c3Start << std::endl;
	std::cout << "Client 4 start: " << c4Start << std::endl;
	std::cout << "Client 5 start: " << c5Start << std::endl;
	std::cout << "Client 6 start: " << c6Start << std::endl;
	std::cout << "Client 7 start: " << c7Start << std::endl;
	std::cout << "Client 8 start: " << c8Start << std::endl;
	std::cout << "Client 9 start: " << c9Start << std::endl;
	std::cout << "Client 10 start: " << c10Start << std::endl;

	uint32_t fileSize = 6000000;

	NodeContainer proxy1;
	proxy1.Create(1);

	NodeContainer proxy2;
	proxy2.Create(1);

	NodeContainer gwNode;
	gwNode.Create(1);

	NodeContainer gwNode2;
	gwNode2.Create(1);

	NodeContainer mainNode;
	mainNode.Create(1);

	NodeContainer srcNode1;
	srcNode1.Create(1);

	NodeContainer mirrorNode;
	mirrorNode.Create(1);

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

	NodeContainer consumer7;
	consumer7.Create(1);

	NodeContainer consumer8;
	consumer8.Create(1);

	NodeContainer consumer9;
	consumer9.Create(1);

	NodeContainer consumer10;
	consumer10.Create(1);

	PointToPointHelper accessLink1;
	accessLink1.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
	accessLink1.SetChannelAttribute("Delay", StringValue("10ms"));
	accessLink1.SetQueue("ns3::DropTailQueue", "MaxSize",
			StringValue("140000B"));
	accessLink1.SetDeviceAttribute("Mtu", UintegerValue(1600));
	NetDeviceContainer devicesAccessLink1;
	devicesAccessLink1 = accessLink1.Install(srcNode1.Get(0), mainNode.Get(0));

	PointToPointHelper endLink1;
	endLink1.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
	endLink1.SetChannelAttribute("Delay", ("Delay", StringValue("2ms")));
	endLink1.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("400000B"));
	endLink1.SetDeviceAttribute("Mtu", UintegerValue(1600));

	PointToPointHelper cacheLink;
	cacheLink.SetDeviceAttribute("DataRate", StringValue("10000Mbps"));
	cacheLink.SetChannelAttribute("Delay", ("Delay", StringValue("0ms")));
	cacheLink.SetQueue("ns3::DropTailQueue", "MaxSize",
			StringValue("4000000B"));
	cacheLink.SetDeviceAttribute("Mtu", UintegerValue(1600));

	PointToPointHelper mirrorLink;
	mirrorLink.SetDeviceAttribute("DataRate", StringValue("50Mbps"));
	mirrorLink.SetChannelAttribute("Delay", ("Delay", StringValue("10ms")));
	mirrorLink.SetQueue("ns3::DropTailQueue", "MaxSize",
			StringValue("1400000B"));
	mirrorLink.SetDeviceAttribute("Mtu", UintegerValue(1600));

	NetDeviceContainer linkToProxy1 = cacheLink.Install(gwNode.Get(0),
			proxy1.Get(0));

	NetDeviceContainer linkToProxy2 = cacheLink.Install(gwNode2.Get(0),
			proxy2.Get(0));

	NetDeviceContainer linkToMirror = mirrorLink.Install(mainNode.Get(0),
			mirrorNode.Get(0));

	NetDeviceContainer devicesEndLink1;
	devicesEndLink1 = accessLink1.Install(mainNode.Get(0), srcNode1.Get(0));

	NetDeviceContainer devicesEndLink2;
	devicesEndLink2 = endLink1.Install(mainNode.Get(0), gwNode.Get(0));

	NetDeviceContainer devicesEndLink3;
	devicesEndLink3 = endLink1.Install(mainNode.Get(0), gwNode2.Get(0));

	NetDeviceContainer devicesEndLink4;
	devicesEndLink4 = endLink1.Install(gwNode.Get(0), consumer1.Get(0));

	NetDeviceContainer devicesEndLink5;
	devicesEndLink5 = endLink1.Install(gwNode2.Get(0), consumer2.Get(0));

	NetDeviceContainer devicesEndLink6;
	devicesEndLink6 = endLink1.Install(gwNode.Get(0), consumer3.Get(0));

	NetDeviceContainer devicesEndLink7;
	devicesEndLink7 = endLink1.Install(gwNode2.Get(0), consumer4.Get(0));

	NetDeviceContainer devicesEndLink8;
	devicesEndLink8 = endLink1.Install(gwNode.Get(0), consumer5.Get(0));

	NetDeviceContainer devicesEndLink9;
	devicesEndLink9 = endLink1.Install(gwNode2.Get(0), consumer6.Get(0));

	NetDeviceContainer devicesEndLink10;
	devicesEndLink10 = endLink1.Install(gwNode.Get(0), consumer7.Get(0));

	NetDeviceContainer devicesEndLink11;
	devicesEndLink11 = endLink1.Install(gwNode2.Get(0), consumer8.Get(0));

	NetDeviceContainer devicesEndLink12;
	devicesEndLink12 = endLink1.Install(gwNode.Get(0), consumer9.Get(0));

	NetDeviceContainer devicesEndLink13;
	devicesEndLink13 = endLink1.Install(gwNode2.Get(0), consumer10.Get(0));

	InternetStackHelper internet;
	internet.InstallAll();

	std::string proxyAddress1 = "10.0.15.2";
	std::string proxyAddress2 = "10.0.16.2";

	if (NP) {
		Ptr<Ipv4L3Protocol> ipv4Instance = gwNode.Get(0)->GetObject<
				Ipv4L3Protocol>();
		ipv4Instance->SetCctpCache(proxyAddress1);
		ipv4Instance = gwNode2.Get(0)->GetObject<Ipv4L3Protocol>();
		ipv4Instance->SetCctpCache(proxyAddress2);
	}

	TrafficControlHelper tch1;
//   uint16_t handle = tch1.SetRootQueueDisc ("ns3::FifoQueueDisc");
//   tch1.AddInternalQueues (
//        handle, 1, "ns3::DropTailQueue", "MaxSize",
//        QueueSizeValue (QueueSize (QueueSizeUnit::BYTES,80000)));
//   tch1.Install (devicesAccessLink1);

// We've got the "hardware" in place.  Now we need to add IP addresses.
//
	NS_LOG_INFO("Assign IP Addresses.");

	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.0.0.0", "255.255.255.0");
	ipv4.Assign(devicesAccessLink1);

	ipv4.NewNetwork();
	//ipv4.Assign (devicesBottleneckLink);

	ipv4.NewNetwork();
	ipv4.Assign(devicesEndLink1);

	ipv4.NewNetwork();
	ipv4.Assign(devicesEndLink2);

	ipv4.NewNetwork();
	ipv4.Assign(devicesEndLink3);

	ipv4.NewNetwork();
	ipv4.Assign(devicesEndLink4);

	ipv4.NewNetwork();
	ipv4.Assign(devicesEndLink5);

	ipv4.NewNetwork();
	ipv4.Assign(devicesEndLink6);

	ipv4.NewNetwork();
	ipv4.Assign(devicesEndLink7);

	ipv4.NewNetwork();
	ipv4.Assign(devicesEndLink8);

	ipv4.NewNetwork();
	ipv4.Assign(devicesEndLink9);

	ipv4.NewNetwork();
	ipv4.Assign(devicesEndLink10);

	ipv4.NewNetwork();
	ipv4.Assign(devicesEndLink11);

	ipv4.NewNetwork();
	ipv4.Assign(devicesEndLink12);

	ipv4.NewNetwork();
	ipv4.Assign(devicesEndLink13);

	ipv4.NewNetwork();
	ipv4.Assign(linkToProxy1);

	ipv4.NewNetwork();
	ipv4.Assign(linkToProxy2);

	ipv4.NewNetwork();
	ipv4.Assign(linkToMirror);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();
	tch1.Uninstall(devicesEndLink1);
	tch1.Uninstall(devicesEndLink2);
	tch1.Uninstall(devicesEndLink3);
	tch1.Uninstall(devicesEndLink4);
	tch1.Uninstall(devicesEndLink5);
	tch1.Uninstall(devicesEndLink6);
	tch1.Uninstall(devicesEndLink7);
	tch1.Uninstall(devicesEndLink8);
	tch1.Uninstall(devicesEndLink9);
	tch1.Uninstall(devicesEndLink10);
	tch1.Uninstall(devicesEndLink11);
	tch1.Uninstall(devicesEndLink12);
	tch1.Uninstall(devicesEndLink13);
	tch1.Uninstall(linkToProxy1);
	tch1.Uninstall(linkToProxy2);
	tch1.Uninstall(linkToMirror);

	Ptr<Node> node;
	Ptr<Ipv4> ipv4Addr;
	Ipv4Address sinkAddr1, sinkAddr2, srcAddr1, srcAddr2;

	Ipv4Address addr1;
	ipv4Addr = proxy1.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr1 = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.
	std::cout << "Proxy1: " << addr1 << " ID: " << proxy1.Get(0)->GetId()
			<< std::endl;

	uint16_t port1 = 9;  // well-known echo port number
	CctpProxyHelper cctpProxy1(port1);
	ApplicationContainer cachApp1 = cctpProxy1.Install(proxy1.Get(0));
	cachApp1.Start(Seconds(0));
	cachApp1.Stop(Seconds(500.0));

	ipv4Addr = proxy2.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr1 = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.
	std::cout << "Proxy2: " << addr1 << " ID: " << proxy2.Get(0)->GetId()
			<< std::endl;

	ipv4Addr = mirrorNode.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr1 = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.
	std::cout << "Mirror: " << addr1 << " ID: " << mirrorNode.Get(0)->GetId()
			<< std::endl;

	// well-known echo port number
	CctpProxyHelper cctpProxy2(port1);
	ApplicationContainer cachApp2 = cctpProxy2.Install(proxy2.Get(0));
	cachApp2.Start(Seconds(0));
	cachApp2.Stop(Seconds(500.0));

	// Create Sink application on node one. The name is server but it acts as a sink in this application

	uint16_t port = 9;  // well-known echo port number
	ItpSinkHelper sink(port);
	ApplicationContainer apps = sink.Install(consumer1.Get(0));
	apps.Start(Seconds(0));
	apps.Stop(Seconds(50.0));

	Ipv4Address addr;
	ipv4Addr = consumer1.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.
	std::cout << "Consumer1: " << addr << " ID: " << consumer1.Get(0)->GetId()
			<< std::endl;

	//mirror node and server node
	uint32_t maxPacketCount = 1;
	Time interPacketInterval = Seconds(0);

	if (NDTP) {
		ItpSourceHelper source(addr, port);
		source.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		source.SetAttribute("Interval", TimeValue(interPacketInterval));
		source.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps = source.Install(srcNode1.Get(0));
		apps.Start(Seconds(0));
		apps.Stop(Seconds(50.0));
	} else {

		//// Mirror Node ///
		ItpSourceHelper mirror(addr, port);
		mirror.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		mirror.SetAttribute("Interval", TimeValue(interPacketInterval));
		mirror.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps = mirror.Install(mirrorNode.Get(0));
		apps.Start(Seconds(0));
		apps.Stop(Seconds(50.0));
	}

	////// Consumer 2///////////////////////

	uint16_t port2 = 9;  // well-known echo port number
	ItpSinkHelper sink2(port2);
	ApplicationContainer apps2 = sink2.Install(consumer2.Get(0));
	apps2.Start(Seconds(c2Start));
	apps2.Stop(Seconds(c2Stop));

	Ipv4Address addr2;
	ipv4Addr = consumer2.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr2 = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.
	std::cout << "Consumer 2: " << addr2 << " ID: " << consumer2.Get(0)->GetId()
			<< std::endl;
	if (NDTP) {
		ItpSourceHelper source2(addr2, port2);
		source2.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		source2.SetAttribute("Interval", TimeValue(interPacketInterval));
		source2.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps2 = source2.Install(srcNode1.Get(0));
		apps2.Start(Seconds(c2Start));
		apps2.Stop(Seconds(c2Stop));
	} else {
		ItpSourceHelper mirror2(addr2, port2);
		mirror2.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		mirror2.SetAttribute("Interval", TimeValue(interPacketInterval));
		mirror2.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps2 = mirror2.Install(mirrorNode.Get(0));
		apps2.Start(Seconds(c2Start));
		apps2.Stop(Seconds(50.0));
	}

	//	///////////////////////////////////////

	//////// Consumer 3////////////////////
	uint16_t port3 = 9;  // well-known echo port number
	ItpSinkHelper sink3(port3);
	ApplicationContainer apps3 = sink3.Install(consumer3.Get(0));
	apps3.Start(Seconds(0));
	apps3.Stop(Seconds(c3Stop));

	Ipv4Address addr3;
	ipv4Addr = consumer3.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr3 = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.
	std::cout << "Consumer3: " << addr3 << " ID: " << consumer3.Get(0)->GetId()
			<< std::endl;
	if (NDTP) {
		ItpSourceHelper source3(addr3, port3);
		source3.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		source3.SetAttribute("Interval", TimeValue(interPacketInterval));
		source3.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps3 = source3.Install(srcNode1.Get(0));
		apps3.Start(Seconds(c3Start));
		apps3.Stop(Seconds(c3Stop));
	} else {
		ItpSourceHelper mirror3(addr3, port3);
		mirror3.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		mirror3.SetAttribute("Interval", TimeValue(interPacketInterval));
		mirror3.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps3 = mirror3.Install(mirrorNode.Get(0));
		apps3.Start(Seconds(c3Start));
		apps3.Stop(Seconds(50.0));
	}
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
	std::cout << "Consumer4: " << addr4 << " ID: " << consumer4.Get(0)->GetId()
			<< std::endl;
	if (NDTP) {
		ItpSourceHelper source4(addr4, port4);
		source4.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		source4.SetAttribute("Interval", TimeValue(interPacketInterval));
		source4.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps4 = source4.Install(srcNode1.Get(0));
		apps4.Start(Seconds(c4Start));
		apps4.Stop(Seconds(c4Stop));
	} else {
		ItpSourceHelper mirror4(addr4, port4);
		mirror4.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		mirror4.SetAttribute("Interval", TimeValue(interPacketInterval));
		mirror4.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps4 = mirror4.Install(mirrorNode.Get(0));
		apps4.Start(Seconds(c4Start));
		apps4.Stop(Seconds(50.0));
	}

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
	std::cout << "Consumer5: " << addr5 << " ID: " << consumer5.Get(0)->GetId()
			<< std::endl;
	if (NDTP) {
		ItpSourceHelper source5(addr5, port5);
		source5.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		source5.SetAttribute("Interval", TimeValue(interPacketInterval));
		source5.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps5 = source5.Install(srcNode1.Get(0));
		apps5.Start(Seconds(c5Start));
		apps5.Stop(Seconds(c5Stop));
	} else {
		//// Mirror Node ///
		ItpSourceHelper mirror5(addr5, port5);
		mirror5.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		mirror5.SetAttribute("Interval", TimeValue(interPacketInterval));
		mirror5.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps5 = mirror5.Install(mirrorNode.Get(0));
		apps5.Start(Seconds(c5Start));
		apps5.Stop(Seconds(50.0));
	}
	//////////////////////////////////////
	//////// Consumer 6////////
	uint16_t port6 = 9;  // well-known echo port number
	ItpSinkHelper sink6(port6);
	ApplicationContainer apps6 = sink6.Install(consumer6.Get(0));
	apps6.Start(Seconds(c6Start));
	apps6.Stop(Seconds(c6Stop));

	Ipv4Address addr6;
	ipv4Addr = consumer6.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr6 = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.
	std::cout << "Consumer6: " << addr6 << " ID: " << consumer6.Get(0)->GetId()
			<< std::endl;
	if (NDTP) {
		ItpSourceHelper source6(addr6, port6);
		source6.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		source6.SetAttribute("Interval", TimeValue(interPacketInterval));
		source6.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps6 = source6.Install(srcNode1.Get(0));
		apps6.Start(Seconds(c6Start));
		apps6.Stop(Seconds(c6Stop));
	} else {
		//// Mirror Node ///
		ItpSourceHelper mirror6(addr6, port6);
		mirror6.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		mirror6.SetAttribute("Interval", TimeValue(interPacketInterval));
		mirror6.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps6 = mirror6.Install(mirrorNode.Get(0));
		apps6.Start(Seconds(c6Start));
		apps6.Stop(Seconds(50.0));
	}
	//////////////////////////////


	//////// Consumer 7////////
	uint16_t port7 = 9;  // well-known echo port number
	ItpSinkHelper sink7(port7);
	ApplicationContainer apps7 = sink7.Install(consumer7.Get(0));
	apps7.Start(Seconds(c7Start));
	apps7.Stop(Seconds(c7Stop));

	Ipv4Address addr7;
	ipv4Addr = consumer7.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr7 = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.
	std::cout << "Consumer7: " << addr7 << " ID: " << consumer7.Get(0)->GetId()
			<< std::endl;
	if (NDTP) {
		ItpSourceHelper source7(addr7, port7);
		source7.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		source7.SetAttribute("Interval", TimeValue(interPacketInterval));
		source7.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps7 = source7.Install(srcNode1.Get(0));
		apps7.Start(Seconds(c7Start));
		apps7.Stop(Seconds(c7Stop));
	} else {
		//// Mirror Node ///
		ItpSourceHelper mirror7(addr7, port7);
		mirror7.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		mirror7.SetAttribute("Interval", TimeValue(interPacketInterval));
		mirror7.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps7 = mirror7.Install(mirrorNode.Get(0));
		apps7.Start(Seconds(c7Start));
		apps7.Stop(Seconds(50.0));
	}
	//////////////////////////////

	//////// Consumer 8////////
	uint16_t port8 = 9;  // well-known echo port number
	ItpSinkHelper sink8(port8);
	ApplicationContainer apps8 = sink8.Install(consumer8.Get(0));
	apps8.Start(Seconds(c8Start));
	apps8.Stop(Seconds(c8Stop));

	Ipv4Address addr8;
	ipv4Addr = consumer8.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr8 = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.
	std::cout << "Consumer8: " << addr8 << " ID: " << consumer8.Get(0)->GetId()
			<< std::endl;
	if (NDTP) {
		ItpSourceHelper source8(addr8, port8);
		source8.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		source8.SetAttribute("Interval", TimeValue(interPacketInterval));
		source8.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps8 = source8.Install(srcNode1.Get(0));
		apps8.Start(Seconds(c8Start));
		apps8.Stop(Seconds(c8Stop));
	} else {
		//// Mirror Node ///
		ItpSourceHelper mirror8(addr8, port8);
		mirror8.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		mirror8.SetAttribute("Interval", TimeValue(interPacketInterval));
		mirror8.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps8 = mirror8.Install(mirrorNode.Get(0));
		apps8.Start(Seconds(c8Start));
		apps8.Stop(Seconds(50.0));
	}
	//////////////////////////////

	//////// Consumer 9////////
	uint16_t port9 = 9;  // well-known echo port number
	ItpSinkHelper sink9(port9);
	ApplicationContainer apps9 = sink9.Install(consumer9.Get(0));
	apps9.Start(Seconds(c9Start));
	apps9.Stop(Seconds(c9Stop));

	Ipv4Address addr9;
	ipv4Addr = consumer9.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr9 = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.
	std::cout << "Consumer9: " << addr9 << " ID: " << consumer9.Get(0)->GetId()
			<< std::endl;
	if (NDTP) {
		ItpSourceHelper source9(addr9, port9);
		source9.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		source9.SetAttribute("Interval", TimeValue(interPacketInterval));
		source9.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps9 = source9.Install(srcNode1.Get(0));
		apps9.Start(Seconds(c9Start));
		apps9.Stop(Seconds(c9Stop));
	} else {
		//// Mirror Node ///
		ItpSourceHelper mirror9(addr9, port9);
		mirror9.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		mirror9.SetAttribute("Interval", TimeValue(interPacketInterval));
		mirror9.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps9 = mirror9.Install(mirrorNode.Get(0));
		apps9.Start(Seconds(c9Start));
		apps9.Stop(Seconds(50.0));
	}
	//////////////////////////////

	//////// Consumer 10////////
	uint16_t port10 = 9;  // well-known echo port number
	ItpSinkHelper sink10(port10);
	ApplicationContainer apps10 = sink10.Install(consumer10.Get(0));
	apps10.Start(Seconds(c10Start));
	apps10.Stop(Seconds(c10Stop));

	Ipv4Address addr10;
	ipv4Addr = consumer10.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	addr10 = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.
	std::cout << "Consumer10: " << addr10 << " ID: " << consumer10.Get(0)->GetId()
			<< std::endl;
	if (NDTP) {
		ItpSourceHelper source10(addr10, port10);
		source10.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		source10.SetAttribute("Interval", TimeValue(interPacketInterval));
		source10.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps10 = source10.Install(srcNode1.Get(0));
		apps10.Start(Seconds(c10Start));
		apps10.Stop(Seconds(c10Stop));
	} else {
		//// Mirror Node ///
		ItpSourceHelper mirror10(addr10, port10);
		mirror10.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
		mirror10.SetAttribute("Interval", TimeValue(interPacketInterval));
		mirror10.SetAttribute("PacketSize", UintegerValue(fileSize));
		apps10 = mirror10.Install(mirrorNode.Get(0));
		apps10.Start(Seconds(c10Start));
		apps10.Stop(Seconds(50.0));
	}
	//////////////////////////////

//  source2traceCWND << "/NodeList/" << srcNode1.Get (0)->GetId ()
//      << "/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow";
//
//  source2traceRTT << "/NodeList/" << srcNode1.Get (0)->GetId ()
//      << "/$ns3::TcpL4Protocol/SocketList/0/RTT";
//
//  oss3 << "/NodeList/"<< srcNode1.Get (0)->GetId ()
//  	      <<"/$ns3::TcpL4Protocol/SocketList/0/CongState";
//
//  FlowMonitorHelper flowmon;
//  monitor = flowmon.InstallAll ();
//  monitor = flowmon.GetMonitor ();
//
//  monitor->CheckForLostPackets ();
//  classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
	NS_LOG_INFO("Run Simulation.");
	Simulator::Stop(Seconds(550.0));
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
