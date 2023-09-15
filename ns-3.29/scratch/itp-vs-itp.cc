
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

static void
Queues (uint32_t oldSize, uint32_t newSize)
{
  QUEUE = newSize;
  //std::cout<<Simulator::Now ().GetSeconds ()<<"s\t"<<QUEUE<<std::endl;
}

static void
CwndChange2 (uint32_t oldCwnd, uint32_t newCwnd)
{
  CWNDSource2 = newCwnd;
  double x = (newCwnd/1200.0);
  std::cout << "\t" << x << "\t" << Source2rtt << "\n";
}

static void
RTTChange2 (Time oldRTT, Time newRTT)
{

  Source2rtt = newRTT.ToDouble (Time::S);
}

void
CwndConnect2 ()
{

  Config::ConnectWithoutContext (source2traceCWND.str (),
				 MakeCallback (&CwndChange2));
}

void
RTTConnect2 ()
{

  Config::ConnectWithoutContext (source2traceRTT.str (),
				 MakeCallback (&RTTChange2));
}

void
handler (void)
{
  std::cout << Simulator::Now ().GetSeconds () << "\t" << CWNDSource2 << "\n";
  Simulator::Schedule (Seconds ((0.05)), &handler);
}

void
GetStatsFlow1 ()
{
  std::cout << "Flow 1" << std::endl;
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
      stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

      if (t.sourceAddress == Ipv4Address ("10.0.0.1")
	  && t.destinationAddress == Ipv4Address ("10.0.2.2"))
	{
	  std::cout << "Throughput: "
	      << i->second.rxBytes * 8.0
		  / (i->second.timeLastRxPacket.GetSeconds ()
		      - i->second.timeFirstTxPacket.GetSeconds ()) / 1024 / 1024
	      << " Mbps\n";
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

void
GetStatsFlow2 ()
{
  std::cout << "Flow 2" << std::endl;
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
      stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

      if (t.sourceAddress == Ipv4Address ("10.0.3.1")
	  && t.destinationAddress == Ipv4Address ("10.0.4.2"))
	{
	  std::cout << "Throughput: "
	      << i->second.rxBytes * 8.0
		  / (i->second.timeLastRxPacket.GetSeconds ()
		      - i->second.timeFirstTxPacket.GetSeconds ()) / 1024 / 1024
	      << " Mbps\n";
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

static void
StateChange (TcpSocketState::TcpCongState_t x, TcpSocketState::TcpCongState_t newValue )
{
  std::cout<<Simulator::Now ().GetSeconds ()<<"s\t"<<x<<"\t"<< newValue<<std::endl;
}

void
StateConnect ()
{
  Config::ConnectWithoutContext (oss3.str (), MakeCallback (&StateChange));
}

int
main (int argc, char *argv[])
{

  Config::SetDefault ("ns3::QueueBase::MaxSize",
		      QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, 1)));
  Config::SetDefault ("ns3::producer::PayloadSize", UintegerValue (1000));
  Config::SetDefault ("ns3::ItpCache::PayloadSize", UintegerValue (1000));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1000));
 // Config::SetDefault ("ns3::TcpSocketBase::ReTxThreshold", UintegerValue (3));
  //Config::SetDefault ("ns3::TcpSocketBase::Sack",  BooleanValue ("false"));
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (10000000));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (10000000));
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
    Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (1));
    Config::SetDefault ("ns3::PacketSink::FileSize", UintegerValue (15000));
    Config::SetDefault ("ns3::ItpCache::COSize", UintegerValue (6000));
   // Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (false));






  bool useV6 = false;
  Address sinkAddress;

  CommandLine cmd;
  cmd.AddValue ("useIpv6", "Use Ipv6", useV6);
  cmd.Parse (argc, argv);

  NS_LOG_INFO("Create nodes.");

  uint32_t fileSize = 3146*1000;

  NodeContainer gwNode;
  gwNode.Create (1);

  NodeContainer gwNode2;
  gwNode2.Create (1);

  NodeContainer srcNode1;
  srcNode1.Create (1);

  NodeContainer sinkNode1;
  sinkNode1.Create (1);

  NodeContainer srcNode2;
  srcNode2.Create (1);

  NodeContainer sinkNode2;
  sinkNode2.Create (1);

  PointToPointHelper accessLink1;
  accessLink1.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  accessLink1.SetChannelAttribute ("Delay", StringValue ("10ms"));
  accessLink1.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
  accessLink1.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesAccessLink1;
  devicesAccessLink1 = accessLink1.Install (srcNode1.Get (0), gwNode.Get (0));

  PointToPointHelper accessLink2;
  accessLink2.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  accessLink2.SetChannelAttribute ("Delay", StringValue ("10ms"));
  accessLink2.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
  accessLink2.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesAccessLink2;
  devicesAccessLink2 = accessLink2.Install (srcNode2.Get (0), gwNode.Get (0));

  PointToPointHelper BottleneckLink;
  BottleneckLink.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  BottleneckLink.SetChannelAttribute ("Delay", StringValue ("10ms"));
  BottleneckLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
  BottleneckLink.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesBottleneckLink;
  devicesBottleneckLink = BottleneckLink.Install (gwNode.Get (0),
						  gwNode2.Get (0));

  PointToPointHelper endLink1;
  endLink1.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  endLink1.SetChannelAttribute ("Delay", StringValue ("10ms"));
  endLink1.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
  endLink1.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesEndLink1;
  devicesEndLink1 = endLink1.Install (gwNode2.Get (0), sinkNode1.Get (0));

  PointToPointHelper endLink2;
  endLink2.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  endLink2.SetChannelAttribute ("Delay", StringValue ("10ms"));
  endLink2.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
  endLink2.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesEndLink2;
  devicesEndLink2 = endLink2.Install (gwNode2.Get (0), sinkNode2.Get (0));

  InternetStackHelper internet;
  internet.InstallAll ();

  Ptr<Ipv4L3Protocol> ipv4Instance = gwNode.Get(0)->GetObject<Ipv4L3Protocol> ();
  //ipv4Instance->SetCache();

  TrafficControlHelper tch1;
   uint16_t handle = tch1.SetRootQueueDisc ("ns3::FifoQueueDisc");
   tch1.AddInternalQueues (
        handle, 1, "ns3::DropTailQueue", "MaxSize",
        QueueSizeValue (QueueSize (QueueSizeUnit::BYTES,20000)));
   QueueDiscContainer qdiscs = tch1.Install (devicesBottleneckLink);


// We've got the "hardware" in place.  Now we need to add IP addresses.
//
  NS_LOG_INFO("Assign IP Addresses.");

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.255.0");
  ipv4.Assign (devicesAccessLink1);

  ipv4.NewNetwork ();
  ipv4.Assign (devicesBottleneckLink);

  ipv4.NewNetwork ();
  ipv4.Assign (devicesEndLink1);

  ipv4.NewNetwork ();
  ipv4.Assign (devicesAccessLink2);

  ipv4.NewNetwork ();
  ipv4.Assign (devicesEndLink2);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  Ptr<Node> node;
  Ptr<Ipv4> ipv4Addr;
  Ipv4Address sinkAddr1, sinkAddr2, srcAddr1, srcAddr2;

  node = srcNode1.Get (0);
  ipv4Addr = node->GetObject<Ipv4> ();
  srcAddr1 = ipv4Addr->GetAddress (1, 0).GetLocal ();
  std::cout << "Source1 Node ID" << srcNode1.Get (0)->GetId ()
      << " address on access link: " << srcAddr1 << std::endl;

  node = sinkNode1.Get (0);
  ipv4Addr = node->GetObject<Ipv4> ();
  sinkAddr1 = ipv4Addr->GetAddress (1, 0).GetLocal ();
  std::cout << "Sink 1 Node ID:" << sinkNode1.Get (0)->GetId ()
      <<"sinkAddr1 Node IP address on access link: " << sinkAddr1
      << std::endl;

  node = srcNode2.Get (0);
  ipv4Addr = node->GetObject<Ipv4> ();
  srcAddr2 = ipv4Addr->GetAddress (1, 0).GetLocal ();
  std::cout << "Source1 Node ID:" << srcNode2.Get (0)->GetId ()
      << " address on access link: " << srcAddr2 << std::endl;

  node = sinkNode2.Get (0);
  ipv4Addr = node->GetObject<Ipv4> ();
  sinkAddr2 = ipv4Addr->GetAddress (1, 0).GetLocal ();
  std::cout << "Sink 2 Node ID:" << sinkNode2.Get (0)->GetId ()<<
      "sinkNode2 IP address on bottleneck link: " << sinkAddr2
      << std::endl;

  // Create Sink application on node one. The name is server but it acts as a sink in this application
  uint16_t port = 9;  // well-known echo port number
  ItpSinkHelper sink (port);
  ApplicationContainer apps = sink.Install (sinkNode1.Get (0));
  apps.Start (Seconds (0.01));
  apps.Stop (Seconds (60.0));

  Ipv4Address addr;
  ipv4Addr = sinkNode1.Get (0)->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  addr = ipv4Addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.
  uint32_t maxPacketCount = 1;
  Time interPacketInterval = Seconds (0);
  ItpSourceHelper source (addr, port);
  source.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  source.SetAttribute ("Interval", TimeValue (interPacketInterval));
  source.SetAttribute ("PacketSize", UintegerValue (fileSize));
  apps = source.Install (srcNode1.Get (0));
  apps.Start (Seconds (0.02));
  apps.Stop (Seconds (60.0));


  uint16_t port2 = 9;  // well-known echo port number
  ItpSinkHelper sink2 (port2);
  ApplicationContainer apps2 = sink2.Install (sinkNode2.Get (0));
  apps2.Start (Seconds (10.01));
  apps2.Stop (Seconds (60.0));

  Ipv4Address addr2;
  ipv4Addr = sinkNode2.Get (0)->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  addr2 = ipv4Addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.


  ItpSourceHelper source2 (addr2, port2);
  source2.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  source2.SetAttribute ("Interval", TimeValue (interPacketInterval));
  source2.SetAttribute ("PacketSize", UintegerValue (fileSize));
  apps2 = source2.Install (srcNode2.Get (0));
  apps2.Start (Seconds (10.02));
  apps2.Stop (Seconds (60.0));


  source2traceCWND << "/NodeList/" << srcNode2.Get (0)->GetId ()
      << "/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow";

  source2traceRTT << "/NodeList/" << srcNode2.Get (0)->GetId ()
      << "/$ns3::TcpL4Protocol/SocketList/0/RTT";

  oss3 << "/NodeList/"<< srcNode2.Get (0)->GetId ()
  	      <<"/$ns3::TcpL4Protocol/SocketList/0/CongState";

  FlowMonitorHelper flowmon;
  monitor = flowmon.InstallAll ();
  monitor = flowmon.GetMonitor ();

  monitor->CheckForLostPackets ();
  classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());

  NS_LOG_INFO("Run Simulation.");
  Simulator::Stop (Seconds (60.0));
 // Simulator::Schedule (Seconds (0.1), &StateConnect);
 // Simulator::Schedule (Seconds (0.1), &CwndConnect2);
  //Simulator::Schedule(Seconds(0.1), &handler);
  Simulator::Run ();
  GetStatsFlow1();
  GetStatsFlow2();
  Simulator::Destroy ();
  NS_LOG_INFO("Done.");


 // std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
}
