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
	double client2Scenario2Start[] = { 137.155, 300.655, 178.897, 11.4698,
			30.6659, 126.266, 126.311, 314.064, 32.0122, 133.839 };

	//// Scenario 2 /////

	double client2Scenario3Start[] = { 59.3406, 31.2019, 30.5825, 201.772,
			259.967, 62.9985, 107.74, 150.431, 29.5425, 170.675 };
	double client3Scenario3Start[] = { 239.822, 247.797, 336.028, 243.759,
			47.2615, 217.272, 4.67128, 192.427, 49.3966, 12.2001 };

	//// Scenario 3 /////
	double client2Scenario4Start[] = { 5.35818, 187.671, 200.526, 17.2831,
			307.3, 40.8674, 363.648, 223.828, 209.574, 39.584 };
	double client3Scenario4Start[] = { 255.026, 415.41, 174.645, 115.467,
			173.665, 88.6387, 21.4351, 276.287, 470.653, 387.785 };
	double client4Scenario4Start[] = { 397.133, 92.7125, 41.5117, 23.3973,
			64.8272, 348.541, 348.722, 106.122, 67.8528, 381.679 };

	//// Scenario 4 /////
	double client2Scenario5Start[] = { 205.301, 403.01, 281.988, 152.821,
			15.7685, 151.424, 296.702, 194.858, 99.4684, 318.842 };
	double client3Scenario5Start[] = { 50.3997, 94.9224, 240.641, 115.69,
			136.242, 253.027, 133.966, 321.211, 296.971, 51.1706 };
	double client4Scenario5Start[] = { 28.9706, 533.894, 52.8541, 48.5233,
			106.723, 314.11, 542.734, 87.2373, 201.792, 148.176 };
	double client5Scenario5Start[] = { 221.523, 261.907, 39.3479, 270.26,
			83.4878, 587.377, 133.457, 39.1804, 87.5193, 153.123 };

	//// Scenario 6 /////
	double client2Scenario6Start[] = { 461.944, 232.508, 195.071, 98.4491,
			367.916, 249.76, 159.846, 498.856, 360.463, 241.187 };
	double client3Scenario6Start[] = { 371.844, 101.417, 394.166, 5.12245,
			185.662, 337.745, 242.008, 59.8722, 402.556, 171.125 };
	double client4Scenario6Start[] = { 8.07358, 40.2248, 325.374, 340.872,
			546.072, 352.358, 535.51, 178.382, 420.806, 61.5242 };
	double client5Scenario6Start[] = { 133.388, 66.0318, 64.6423, 263.104,
			52.025, 142.844, 277.478, 174.652, 62.3171, 266.148 };
	double client6Scenario6Start[] = { 31.8775, 39.8543, 128.086, 35.8169,
			103.387, 9.32952, 9.41645, 466.316, 108.564, 24.9175 };

	//// Scenario 6 /////
	double client2Scenario7Start[] = { 130.973, 332.083, 74.8954, 193.623,
			241.227, 69.7019, 352.232, 13.627, 94.065, 194.943 };
	double client3Scenario7Start[] = { 161.617, 297.623, 76.2444, 203.803,
			51.5054, 203.515, 113.602, 581.92, 433.393, 550.698 };
	double client4Scenario7Start[] = { 119.639, 539.909, 575.402, 5.98395,
			111.761, 192.094, 195.761, 217.003, 25.665, 377.974 };
	double client5Scenario7Start[] = { 227.132, 248.913, 443.895, 565.109,
			5.7806, 66.0732, 108.828, 189.782, 507.543, 5.92058 };
	double client6Scenario7Start[] = { 418.413, 112.105, 76.9127, 496.772,
			124.697, 34.4984, 138.936, 241.737, 65.1022, 127.248 };
	double client7Scenario7Start[] = { 100.338, 72.7136, 254.652, 299.331,
			562.253, 67.8409, 363.922, 442.736, 131.199, 427.704 };

	//// Scenario 7 /////

	double client2Scenario8Start[] = { 116.424, 195.805, 213.395, 445.28,
			593.594, 510.279, 11.6137, 69.4067, 265.745, 357.474 };
	double client3Scenario8Start[] = { 64.2988, 30.6034, 367.773, 367.601,
			446.548, 163.457, 23.4932, 160.266, 29.5174, 154.882 };
	double client4Scenario8Start[] = { 237.658, 380.905, 454.271, 6.84956,
			281.612, 80.0308, 73.5411, 92.0776, 597.782, 84.8197 };
	double client5Scenario8Start[] = { 14.579, 144.605, 574.808, 81.7912,
			99.3561, 88.1953, 155.703, 266.331, 316.25, 85.1965 };
	double client6Scenario8Start[] = { 10.8154, 538.697, 231.787, 35.6248,
			459.765, 215.005, 449.206, 15.8851, 1.6302, 179.844 };
	double client7Scenario8Start[] = { 47.0835, 207.466, 89.7064, 305.624,
			147.641, 546.854, 44.5223, 68.3451, 262.709, 520.556 };
	double client8Scenario8Start[] = { 189.192, 227.597, 477.881, 48.7749,
			522.196, 140.596, 140.778, 271.821, 155.683, 173.735 };

	//// Scenario 8 /////
	double client2Scenario9Start[] = { 400.505, 23.5217, 66.0538, 372.027,
			59.1703, 356.357, 215.023, 377.219, 158.838, 175.923 };
	double client3Scenario9Start[] = { 28.9654, 102.439, 475.161, 38.3481,
			190.158, 332.302, 330.535, 442.709, 33.42, 39.6777 };
	double client4Scenario9Start[] = { 404.751, 80.0116, 539.479, 189.654,
			18.5541, 128.122, 139.249, 334.726, 562.446, 458.767 };
	double client5Scenario9Start[] = { 340.304, 496.59, 580.299, 7.71604,
			353.466, 216.104, 184.709, 325.394, 15.0434, 119.548 };
	double client6Scenario9Start[] = { 250.204, 349.643, 378.82, 219.231,
			89.1248, 11.1721, 38.2071, 184.787, 34.2731, 316.722 };
	double client7Scenario9Start[] = { 232.851, 105.443, 30.5582, 141.465,
			402.766, 523.892, 120.367, 60.6428, 42.157, 451.703 };
	double client8Scenario9Start[] = { 231.922, 348.087, 103.07, 189.797,
			172.486, 251.966, 85.6316, 577.647, 99.1164, 107.685 };
	double client9Scenario9Start[] = { 315.991, 565.485, 6.44559, 331.519,
			486.863, 236.849, 14.2379, 163.612, 182.34, 38.198 };

	//// Scenario 9 /////
	double client2Scenario10Start[] = { 156.955, 492.804, 555.218, 443.233,
			307.031, 306.103, 183.416, 3.2946, 171.778, 471.677 };
	double client3Scenario10Start[] = { 455.003, 541.233, 64.1976, 79.9906,
			24.095, 173.426, 105.918, 2.46287, 503.112, 210.833 };
	double client4Scenario10Start[] = { 402.954, 161.933, 507.867, 188.58,
			73.7179, 221.353, 583.561, 5.24755, 80.1798, 354.473 };
	double client5Scenario10Start[] = { 282.204, 31.3389, 56.8953, 42.7901,
			214.67, 96.5147, 557.665, 302.253, 37.374, 87.9405 };
	double client6Scenario10Start[] = { 49.4802, 139.196, 186.788, 8.58666,
			32.4139, 472.059, 6.59966, 25.1346, 530.837, 17.8778 };
	double client7Scenario10Start[] = { 111.013, 195.069, 74.0441, 480.28,
			392.821, 511.775, 88.7603, 399.013, 54.2596, 110.9 };
	double client8Scenario10Start[] = { 61.0408, 234.578, 32.6972, 306.465,
			273.909, 45.0852, 382.263, 113.269, 249.306, 112.901 };
	double client9Scenario10Start[] = { 13.5809, 325.955, 117.057, 106.441,
			199.572, 106.166, 334.79, 210.754, 89.0266, 453.615 };
	double client10Scenario10Start[] = { 539.97, 53.9632, 84.6471, 62.3179,
			455.249, 379.433, 379.933, 84.2575, 211.603, 482.091 };

	double c2Start, c3Start, c4Start, c5Start, c6Start, c7Start, c8Start,
			c9Start, c10Start;
	double c2Stop, c3Stop, c4Stop, c5Stop, c6Stop, c7Stop, c8Stop, c9Stop,
			c10Stop;

//	  CommandLine cmd;
	int index;
//	  cmd.AddValue ("index",  "an int argument", index);
//	  cmd.Parse (argc, argv);
	index = 7;

	c2Start = client2Scenario10Start[index];
	c3Start = client3Scenario10Start[index];
	c4Start = client4Scenario10Start[index];
	c5Start = client5Scenario10Start[index];
	c6Start = client6Scenario10Start[index];
	c7Start = client7Scenario10Start[index];
	c8Start = client8Scenario10Start[index];
	c9Start = client9Scenario10Start[index];
	c10Start = client10Scenario10Start[index];

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
	accessLink1.SetChannelAttribute("Delay", StringValue("5ms"));
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

	NetDeviceContainer linkToProxy1 = cacheLink.Install(gwNode.Get(0),
			proxy1.Get(0));

	NetDeviceContainer linkToProxy2 = cacheLink.Install(gwNode2.Get(0),
			proxy2.Get(0));

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
	Ptr<Ipv4L3Protocol> ipv4Instance =
			gwNode.Get(0)->GetObject<Ipv4L3Protocol>();
	ipv4Instance->SetCctpCache(proxyAddress1);

//	std::string proxyAddress2 = "10.0.16.2";
//	ipv4Instance =
//				gwNode2.Get(0)->GetObject<Ipv4L3Protocol>();
//		ipv4Instance->SetCctpCache(proxyAddress2);

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
	uint32_t maxPacketCount = 1;
	Time interPacketInterval = Seconds(0);
	ItpSourceHelper source(addr, port);
	source.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	source.SetAttribute("Interval", TimeValue(interPacketInterval));
	source.SetAttribute("PacketSize", UintegerValue(fileSize));
	apps = source.Install(srcNode1.Get(0));
	apps.Start(Seconds(0));
	apps.Stop(Seconds(50.0));

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

	 ItpSourceHelper source6(addr6, port6);
	 source6.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	 source6.SetAttribute("Interval", TimeValue(interPacketInterval));
	 source6.SetAttribute("PacketSize", UintegerValue(fileSize));
	 apps6 = source6.Install(srcNode1.Get(0));
	 apps6.Start(Seconds(c6Start));
	 apps6.Stop(Seconds(c6Stop));
	 //////////////////////////////

	 //////// Consumer 7////////
	 uint16_t port7 = 7;  // well-known echo port number
	 ItpSinkHelper sink7(port7);
	 ApplicationContainer apps7 = sink7.Install(consumer7.Get(0));
	 apps7.Start(Seconds(c7Start));
	 apps7.Stop(Seconds(c7Stop));

	 Ipv4Address addr7;
	 ipv4Addr = consumer7.Get(0)->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	 addr7 = ipv4Addr->GetAddress(1, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.

	 ItpSourceHelper source7(addr7, port7);
	 source7.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	 source7.SetAttribute("Interval", TimeValue(interPacketInterval));
	 source7.SetAttribute("PacketSize", UintegerValue(fileSize));
	 apps7 = source7.Install(srcNode1.Get(0));
	 apps7.Start(Seconds(c7Start));
	 apps7.Stop(Seconds(c7Stop));
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

	 ItpSourceHelper source8(addr8, port8);
	 source8.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	 source8.SetAttribute("Interval", TimeValue(interPacketInterval));
	 source8.SetAttribute("PacketSize", UintegerValue(fileSize));
	 apps8 = source8.Install(srcNode1.Get(0));
	 apps8.Start(Seconds(c8Start));
	 apps8.Stop(Seconds(c8Stop));
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

	 ItpSourceHelper source9(addr9, port9);
	 source9.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	 source9.SetAttribute("Interval", TimeValue(interPacketInterval));
	 source9.SetAttribute("PacketSize", UintegerValue(fileSize));
	 apps9 = source9.Install(srcNode1.Get(0));
	 apps9.Start(Seconds(c6Start));
	 apps9.Stop(Seconds(c9Stop));
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

	 ItpSourceHelper source10(addr10, port10);
	 source10.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	 source10.SetAttribute("Interval", TimeValue(interPacketInterval));
	 source10.SetAttribute("PacketSize", UintegerValue(fileSize));
	 apps10 = source10.Install(srcNode1.Get(0));
	 apps10.Start(Seconds(c6Start));
	 apps10.Stop(Seconds(c10Stop));
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
