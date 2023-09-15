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
//       n0    n1   n2
//       | ====  |  ====  |
//              LAN
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
#include "ns3/tcp-socket.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RequestResponseExample");

std::ostringstream oss, oss1, oss2, oss3;
static uint32_t CWND = 0;
static uint32_t inFlight = 0;
static uint32_t QUEUE = 0;
static double rtt = 0;
static int counter = 0;
static Ptr<Ipv4FlowClassifier> classifier;
static Ptr<FlowMonitor> monitor;
static uint32_t lastPackets = 0;
static double jitter = 0;
static double delay = 0;

void handler(void) {
	double thorughout = (double) CWND * 8;
	thorughout = thorughout / rtt;
	thorughout = thorughout / 1000000;
	std::cout << "\t" << double(CWND / 1500.0) << "\t" << rtt << "\t"
			<< thorughout << "\t" << delay << "\t" << QUEUE << "\n";
	Simulator::Schedule(Seconds((0.05)), &handler);
}

static void CwndChange(uint32_t oldCwnd, uint32_t newCwnd) {
	CWND = newCwnd;
}

static void RTTChange(Time oldRTT, Time newRTT) {
	rtt = newRTT.ToDouble(Time::S);
}

static void InFLightChange(uint32_t oldCwnd, uint32_t newCwnd) {
	inFlight = newCwnd;
}

static void Queues(uint32_t oldSize, uint32_t newSize) {
	QUEUE = newSize;
}

void CwndConnect() {
	Config::ConnectWithoutContext(oss.str(), MakeCallback(&CwndChange));
}

void RTTConnect() {
	Config::ConnectWithoutContext(oss1.str(), MakeCallback(&RTTChange));
}

void InFLightConnect() {
	Config::ConnectWithoutContext(oss2.str(), MakeCallback(&InFLightChange));
}

static void StateChange(TcpSocketState::TcpCongState_t x,
		TcpSocketState::TcpCongState_t newValue) {
	std::cout << Simulator::Now().GetSeconds() << "s\t" << x << "\t" << newValue
			<< std::endl;
}

void StateConnect() {
	Config::ConnectWithoutContext(oss3.str(), MakeCallback(&StateChange));
}

void GetPackets() {
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
			stats.begin(); i != stats.end(); ++i) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
		if (t.sourceAddress == Ipv4Address("10.0.0.1")
				&& t.destinationAddress == Ipv4Address("10.0.2.2")) {
			std::cout << i->second.rxPackets - lastPackets << "\n";
			lastPackets = i->second.rxPackets;
		}
	}
	Simulator::Schedule(Seconds((0.1)), &GetPackets);
}

void GetQueue() {
//  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
//  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
//      stats.begin (); i != stats.end (); ++i)
//    {
//      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
//      if (t.sourceAddress == Ipv4Address ("10.0.0.1")
//	  && t.destinationAddress == Ipv4Address ("10.0.2.2"))
//	{
	//std::cout << "\t" << QUEUE << "\n";
//	}
//    }

	Simulator::Schedule(Seconds((0.01)), &GetQueue);
}

void GetDelay() {
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
			stats.begin(); i != stats.end(); ++i) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
		if (t.sourceAddress == Ipv4Address("10.0.0.1")
				&& t.destinationAddress == Ipv4Address("10.0.2.2")) {
			delay = i->second.lastDelay.GetSeconds();
		}
	}
	Simulator::Schedule(Seconds((0.01)), &GetDelay);
}

void GetJitter() {
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
			stats.begin(); i != stats.end(); ++i) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
		if (t.sourceAddress == Ipv4Address("10.0.0.1")
				&& t.destinationAddress == Ipv4Address("10.0.2.2")) {
			std::cout << "Jitter:\t"
					<< i->second.jitterSum.GetSeconds() - jitter << "\n";
			jitter = i->second.jitterSum.GetSeconds();
		}
	}

	Simulator::Schedule(Seconds((0.1)), &GetJitter);
}

void GetStats() {
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
			stats.begin(); i != stats.end(); ++i) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);

		if (t.sourceAddress == Ipv4Address("10.0.0.1")
				&& t.destinationAddress == Ipv4Address("10.0.1.2")) {
			std::cout << "Consumer 1:\t"
					<< (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds()
					<< std::endl;
			std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
			std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

		} else if (t.sourceAddress == Ipv4Address("10.0.0.1")
				&& t.destinationAddress == Ipv4Address("10.0.2.2")) {
			std::cout << "\nConsumer 2:\t"
					<< (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds()
					<< std::endl;
			std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
			std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

		} else if (t.sourceAddress == Ipv4Address("10.0.0.1")
				&& t.destinationAddress == Ipv4Address("10.0.3.2")) {
			std::cout << "\nConsumer 3:\t"
					<< (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds()
					<< std::endl;
			std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
			std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

		} else if (t.sourceAddress == Ipv4Address("10.0.0.1")
				&& t.destinationAddress == Ipv4Address("10.0.4.2")) {
			std::cout << "\nConsumer 4:\t"
					<< (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds()
					<< std::endl;
			std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
			std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

		} else if (t.sourceAddress == Ipv4Address("10.0.0.1")
				&& t.destinationAddress == Ipv4Address("10.0.5.2")) {
			std::cout << "\nConsumer 5:\t"
					<< (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds()
					<< std::endl;
			std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
			std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

		} else if (t.sourceAddress == Ipv4Address("10.0.0.1")
				&& t.destinationAddress == Ipv4Address("10.0.6.2")) {
			std::cout << "\nConsumer 6:\t"
					<< (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds()
					<< std::endl;
			std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
			std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

		} else if (t.sourceAddress == Ipv4Address("10.0.0.1")
				&& t.destinationAddress == Ipv4Address("10.0.7.2")) {
			std::cout << "\nConsumer 7:\t"
					<< (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds()
					<< std::endl;
			std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
			std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

		} else if (t.sourceAddress == Ipv4Address("10.0.0.1")
				&& t.destinationAddress == Ipv4Address("10.0.8.2")) {
			std::cout << "\nConsumer 6:\t"
					<< (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds()
					<< std::endl;
			std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
			std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

		} else if (t.sourceAddress == Ipv4Address("10.0.0.1")
				&& t.destinationAddress == Ipv4Address("10.0.9.2")) {
			std::cout << "\nConsumer 9:\t"
					<< (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds()
					<< std::endl;
			std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
			std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

		} else if (t.sourceAddress == Ipv4Address("10.0.0.1")
				&& t.destinationAddress == Ipv4Address("10.0.10.2")) {
			std::cout << "\nConsumer 10:\t"
					<< (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds()
					<< std::endl;
			std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
			std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

		}

	}
}

int main(int argc, char *argv[]) {

	Config::SetDefault("ns3::QueueBase::MaxSize",
			QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, 1)));
	Config::SetDefault("ns3::TcpL4Protocol::SocketType",
			TypeIdValue(TypeId::LookupByName("ns3::TcpNewReno")));
	Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1000));
	Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(10000000));
	Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(10000000));
	Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(1));
	Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(1));
	//Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (false));
	Config::SetDefault("ns3::TcpSocket::InitialSlowStartThreshold",
			UintegerValue(30000));

	double c2Start, c3Start, c4Start, c5Start, c6Start;
	double c2Stop, c3Stop, c4Stop, c5Stop, c6Stop;

	CommandLine cmd;
	int index;
	cmd.AddValue("index", "an int argument", index);
	cmd.Parse(argc, argv);

	c2Start = 0;
	c3Start = 0;
	c4Start = 0;
	c5Start = 0;
	c6Start = 0;

	c2Stop = c2Start + 100;
	c3Stop = c3Start + 100;
	c4Stop = c4Start + 100;
	c5Stop = c5Start + 100;
	c6Stop = c6Start + 100;

	uint32_t fileSize = 6000000;

	NodeContainer gwNode;
	gwNode.Create(1);

	NodeContainer srcNode;
	srcNode.Create(1);

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

	PointToPointHelper accessLink;
	accessLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
	accessLink.SetChannelAttribute("Delay", StringValue("5ms"));
	accessLink.SetQueue("ns3::DropTailQueue", "MaxSize",
			StringValue("140000B"));
	accessLink.SetDeviceAttribute("Mtu", UintegerValue(1600));
	NetDeviceContainer devicesAccessLink;
	devicesAccessLink = accessLink.Install(srcNode.Get(0), gwNode.Get(0));

	PointToPointHelper endLink1;
	endLink1.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
	endLink1.SetChannelAttribute("Delay", StringValue("2ms"));
	endLink1.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("400000B"));
	endLink1.SetDeviceAttribute("Mtu", UintegerValue(1600));

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

	InternetStackHelper internet;
	internet.InstallAll();

// We've got the "hardware" in place.  Now we need to add IP addresses.
//
	TrafficControlHelper tch1;

	NS_LOG_INFO("Assign IP Addresses.");

	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.0.0.0", "255.255.255.0");
	ipv4.Assign(devicesAccessLink);

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
	ipv4.Assign(endLinkConsumer6);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();
	tch1.Uninstall(devicesAccessLink);
	tch1.Uninstall(endLinkConsumer1);
	tch1.Uninstall(endLinkConsumer2);
	tch1.Uninstall(endLinkConsumer3);
	tch1.Uninstall(endLinkConsumer4);
	tch1.Uninstall(endLinkConsumer5);
	tch1.Uninstall(endLinkConsumer6);

	Ptr<Node> node;
	Ptr<Ipv4> addr;
	Ipv4Address contentProducer, client1, client2, client3, client4, client5,
			client6, gwnodeAdd;

	node = srcNode.Get(0); // Get pointer to ith node in container
	addr = node->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	contentProducer = addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	node = consumer1.Get(0); // Get pointer to ith node in container
	addr = node->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	client1 = addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	node = consumer2.Get(0); // Get pointer to ith node in container
	addr = node->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	client2 = addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	node = consumer3.Get(0); // Get pointer to ith node in container
	addr = node->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	client3 = addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	node = consumer4.Get(0); // Get pointer to ith node in container
	addr = node->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	client4 = addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	node = consumer5.Get(0); // Get pointer to ith node in container
	addr = node->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	client5 = addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	node = consumer6.Get(0); // Get pointer to ith node in container
	addr = node->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	client6 = addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	////// Consumer 1 ///////////////////////////////////
	uint16_t port = 9;
	BulkSendHelper source("ns3::TcpSocketFactory",
			InetSocketAddress(client1, port));
	// Set the amount of data to send in bytes.  Zero is unlimited.
	source.SetAttribute("MaxBytes", UintegerValue(fileSize));
	source.SetAttribute("SendSize", UintegerValue(fileSize));
	ApplicationContainer sourceApps = source.Install(srcNode.Get(0));
	sourceApps.Start(Seconds(0.02));
	sourceApps.Stop(Seconds(1000));

	PacketSinkHelper sink("ns3::TcpSocketFactory",
			InetSocketAddress(client1, port));
	ApplicationContainer sinkApps = sink.Install(consumer1.Get(0));
	sinkApps.Start(Seconds(0.01));
	sinkApps.Stop(Seconds(1000));
	/////////////////////////////////////

	////// Consumer 2 ///////////
	uint16_t port2 = 9;
	BulkSendHelper source2("ns3::TcpSocketFactory",
			InetSocketAddress(client2, port2));
	// Set the amount of data to send in bytes.  Zero is unlimited.
	source2.SetAttribute("MaxBytes", UintegerValue(fileSize));
	source2.SetAttribute("SendSize", UintegerValue(fileSize));
	ApplicationContainer sourceApps2 = source2.Install(srcNode.Get(0));
	sourceApps2.Start(Seconds(c2Start));
	sourceApps2.Stop(Seconds(1000));

	PacketSinkHelper sink2("ns3::TcpSocketFactory",
			InetSocketAddress(client2, port2));
	ApplicationContainer sinkApps2 = sink2.Install(consumer2.Get(0));
	sinkApps2.Start(Seconds(c2Start));
	sinkApps2.Stop(Seconds(1000));
	//////////////////////////////////////////////////////////////////////////

	////// Consumer 3 /////////////////////////////////////////////////////
	uint16_t port3 = 9;
	BulkSendHelper source3("ns3::TcpSocketFactory",
			InetSocketAddress(client3, port3));
	// Set the amount of data to send in bytes.  Zero is unlimited.
	source3.SetAttribute("MaxBytes", UintegerValue(fileSize));
	source3.SetAttribute("SendSize", UintegerValue(fileSize));
	ApplicationContainer sourceApps3 = source3.Install(srcNode.Get(0));
	sourceApps3.Start(Seconds(c3Start));
	sourceApps3.Stop(Seconds(1000));

	PacketSinkHelper sink3("ns3::TcpSocketFactory",
			InetSocketAddress(client3, port3));
	ApplicationContainer sinkApps3 = sink3.Install(consumer3.Get(0));
	sinkApps3.Start(Seconds(c3Start ));
	sinkApps3.Stop(Seconds(1000));
	//////////////////////////////////////////////////////////////////////////

	////// Consumer 4 /////////////////////////////////////////////////////
	uint16_t port4 = 9;
	BulkSendHelper source4("ns3::TcpSocketFactory",
			InetSocketAddress(client4, port4));
	// Set the amount of data to send in bytes.  Zero is unlimited.
	source4.SetAttribute("MaxBytes", UintegerValue(fileSize));
	source4.SetAttribute("SendSize", UintegerValue(fileSize));
	ApplicationContainer sourceApps4 = source4.Install(srcNode.Get(0));
	sourceApps4.Start(Seconds(c4Start));
	sourceApps4.Stop(Seconds(1000.0));

	PacketSinkHelper sink4("ns3::TcpSocketFactory",
			InetSocketAddress(client4, port4));
	ApplicationContainer sinkApps4 = sink4.Install(consumer4.Get(0));
	sinkApps4.Start(Seconds(c4Start));
	sinkApps4.Stop(Seconds(1000.0));
	//////////////////////////////////////////////////////////////////////////

	////// Consumer 5 /////////////////////////////////////////////////////
	uint16_t port5 = 9;
	BulkSendHelper source5("ns3::TcpSocketFactory",
			InetSocketAddress(client5, port5));
	// Set the amount of data to send in bytes.  Zero is unlimited.
	source5.SetAttribute("MaxBytes", UintegerValue(fileSize));
	source5.SetAttribute("SendSize", UintegerValue(fileSize));
	ApplicationContainer sourceApps5 = source5.Install(srcNode.Get(0));
	sourceApps5.Start(Seconds(c5Start));
	sourceApps5.Stop(Seconds(1000.0));

	PacketSinkHelper sink5("ns3::TcpSocketFactory",
			InetSocketAddress(client5, port5));
	ApplicationContainer sinkApps5 = sink5.Install(consumer5.Get(0));
	sinkApps5.Start(Seconds(c5Start ));
	sinkApps5.Stop(Seconds(1000.0));
	//////////////////////////////////////////////////////////////////////////

	////// Consumer 6 /////////////////////////////////////////////////////
	uint16_t port6 = 9;
	BulkSendHelper source6("ns3::TcpSocketFactory",
			InetSocketAddress(client6, port6));
	// Set the amount of data to send in bytes.  Zero is unlimited.
	source6.SetAttribute("MaxBytes", UintegerValue(fileSize));
	source6.SetAttribute("SendSize", UintegerValue(fileSize));
	ApplicationContainer sourceApps6 = source6.Install(srcNode.Get(0));
	sourceApps6.Start(Seconds(c6Start));
	sourceApps6.Stop(Seconds(1000.0));

	PacketSinkHelper sink6("ns3::TcpSocketFactory",
			InetSocketAddress(client6, port6));
	ApplicationContainer sinkApps6 = sink6.Install(consumer6.Get(0));
	sinkApps6.Start(Seconds(c6Start));
	sinkApps6.Stop(Seconds(1000.0));
	//////////////////////////////////////////////////////////////////////////


	oss << "/NodeList/" << srcNode.Get(0)->GetId()
			<< "/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow";

	oss1 << "/NodeList/" << srcNode.Get(0)->GetId()
			<< "/$ns3::TcpL4Protocol/SocketList/0/RTT";

	oss2 << "/NodeList/" << srcNode.Get(0)->GetId()
			<< "/$ns3::TcpL4Protocol/SocketList/0/BytesInFlight";

	oss3 << "/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/CongState";

//  Ptr<Queue<Packet> > queue = StaticCast<PointToPointNetDevice> (
//      devicesBottleneckLink.Get (0))->GetQueue ();

	FlowMonitorHelper flowmon;
	monitor = flowmon.InstallAll();
	monitor = flowmon.GetMonitor();

	monitor->CheckForLostPackets();
	classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());

	NS_LOG_INFO("Run Simulation.");
	Simulator::Stop(Seconds(1000.0));
	Simulator::Schedule(Seconds(0.1), &RTTConnect);
	Simulator::Schedule(Seconds(0.03), &CwndConnect);
	Simulator::Schedule(Seconds(0.1), &GetDelay);
	Simulator::Schedule(Seconds(0.1), &GetQueue);
	//Simulator::Schedule (Seconds (0.02), &handler);

	Simulator::Run();
	GetStats();
	Simulator::Destroy();

}
