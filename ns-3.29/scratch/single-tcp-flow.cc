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


void
handler (void)
{
  double thorughout = (double) CWND * 8;
  thorughout = thorughout / rtt;
  thorughout = thorughout / 1000000;
  std::cout <<"\t"<<double(CWND/1500.0) << "\t" << rtt << "\t" << thorughout << "\t"<<delay<<"\t"<<QUEUE<<"\n";
  Simulator::Schedule (Seconds ((0.05)), &handler);
}

static void
CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
{
  CWND = newCwnd;
}

static void
RTTChange (Time oldRTT, Time newRTT)
{
  rtt = newRTT.ToDouble (Time::S);
}

static void
InFLightChange (uint32_t oldCwnd, uint32_t newCwnd)
{
  inFlight = newCwnd;
}

static void
Queues (uint32_t oldSize, uint32_t newSize)
{
  QUEUE = newSize;
}

void
CwndConnect ()
{
  Config::ConnectWithoutContext (oss.str (), MakeCallback (&CwndChange));
}

void
RTTConnect ()
{
  Config::ConnectWithoutContext (oss1.str (), MakeCallback (&RTTChange));
}

void
InFLightConnect ()
{
  Config::ConnectWithoutContext (oss2.str (), MakeCallback (&InFLightChange));
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

void
GetPackets ()
{
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
      stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      if (t.sourceAddress == Ipv4Address ("10.0.0.1")
	  && t.destinationAddress == Ipv4Address ("10.0.2.2"))
	{
	  std::cout << i->second.rxPackets - lastPackets << "\n";
	  lastPackets = i->second.rxPackets;
	}
    }
  Simulator::Schedule (Seconds ((0.1)), &GetPackets);
}

void
GetQueue ()
{
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

  Simulator::Schedule (Seconds ((0.01)), &GetQueue);
}

void
GetDelay ()
{
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
      stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      if (t.sourceAddress == Ipv4Address ("10.0.0.1")
	  && t.destinationAddress == Ipv4Address ("10.0.2.2"))
	{
	  delay =  i->second.lastDelay.GetSeconds ();
	}
    }
  Simulator::Schedule (Seconds ((0.01)), &GetDelay);
}

void
GetJitter ()
{
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
      stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      if (t.sourceAddress == Ipv4Address ("10.0.0.1")
	  && t.destinationAddress == Ipv4Address ("10.0.2.2"))
	{
	  std::cout << "Jitter:\t" << i->second.jitterSum.GetSeconds () - jitter
	      << "\n";
	  jitter = i->second.jitterSum.GetSeconds ();
	}
    }

  Simulator::Schedule (Seconds ((0.1)), &GetJitter);
}

void
GetStats ()
{
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
      stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

      if (t.sourceAddress == Ipv4Address ("10.0.0.1")
	  && t.destinationAddress == Ipv4Address ("10.0.2.2"))
	{
	  std::cout << "Total Time:\t"
	      << (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds ()
	      << std::endl;
	  std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
	  std::cout << "received bytes:\t" << i->second.rxBytes << "\n";
	  std::cout << "<lost packets:\t" << i->second.lostPackets << std::endl;
	  std::cout << "Throughput:\t"
	      << i->second.rxBytes * 8.0
		  / (i->second.timeLastRxPacket.GetSeconds ()
		      - i->second.timeFirstTxPacket.GetSeconds ()) / 1024 / 1024
	      << " Mbps\n";
	  std::cout << "delay sum:\t" << i->second.delaySum.GetSeconds ()
	      << "\n";
	  std::cout << "Jitter sum:\t" << i->second.jitterSum.GetSeconds ()
	      << "\n";

	}
    }
}
int
main (int argc, char *argv[])
{

  Config::SetDefault ("ns3::QueueBase::MaxSize",
		      QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, 1)));
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType",
		      TypeIdValue (TypeId::LookupByName ("ns3::TcpNewReno")));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1500));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (10000000));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (10000000));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (1));
  Config::SetDefault ("ns3::PacketSink::FileSize", UintegerValue (1500*4000));
  //Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (false));
  //Config::SetDefault ("ns3::TcpSocket::InitialSlowStartThreshold", UintegerValue (3000));


  uint32_t fileSize = 4000*1500;

  NodeContainer gwNode;
  gwNode.Create (1);

  NodeContainer gwNode2;
  gwNode2.Create (1);

  NodeContainer srcNode;
  srcNode.Create (1);

  NodeContainer sinkNode;
  sinkNode.Create (1);

  PointToPointHelper accessLink;
  accessLink.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  accessLink.SetChannelAttribute ("Delay", StringValue ("33ms"));
  accessLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
  accessLink.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesAccessLink;
  devicesAccessLink = accessLink.Install (srcNode.Get (0), gwNode.Get (0));

  PointToPointHelper BottleneckLink;
  BottleneckLink.SetDeviceAttribute ("DataRate", StringValue ("1.5Mbps"));
  BottleneckLink.SetChannelAttribute ("Delay", StringValue ("5ms"));
  BottleneckLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
  BottleneckLink.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesBottleneckLink;
  devicesBottleneckLink = BottleneckLink.Install (gwNode.Get (0),
						  gwNode2.Get (0));

  PointToPointHelper endLink;
  endLink.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  endLink.SetChannelAttribute ("Delay", StringValue ("2ms"));
  endLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
  endLink.SetDeviceAttribute ("Mtu", UintegerValue (1600));

  NetDeviceContainer devicesEndLink;
  devicesEndLink = endLink.Install (gwNode2.Get (0), sinkNode.Get (0));

  InternetStackHelper internet;
  internet.InstallAll ();

  Ptr<Ipv4L3Protocol> ipv4Instance2 = gwNode.Get(0)->GetObject<Ipv4L3Protocol> ();
   // ipv4Instance2->SetCache();

  TrafficControlHelper tch1;
  uint16_t handle = tch1.SetRootQueueDisc ("ns3::FifoQueueDisc");
  tch1.AddInternalQueues (
      handle, 1, "ns3::DropTailQueue", "MaxSize",
      QueueSizeValue (QueueSize (QueueSizeUnit::BYTES, 18000)));
  QueueDiscContainer qdiscs = tch1.Install (devicesBottleneckLink);

// We've got the "hardware" in place.  Now we need to add IP addresses.
//
  NS_LOG_INFO("Assign IP Addresses.");

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.255.0");
  ipv4.Assign (devicesAccessLink);
  ipv4.NewNetwork ();
  ipv4.Assign (devicesBottleneckLink);
  ipv4.NewNetwork ();
  ipv4.Assign (devicesEndLink);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Ptr<Node> node;
  Ptr<Ipv4> ipv4Addr;
  Ipv4Address addr;

  node = srcNode.Get (0); // Get pointer to ith node in container
  ipv4Addr = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  addr = ipv4Addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.
  std::cout << "Source Node IP address on access link: " << addr << std::endl;

  node = gwNode.Get (0); // Get pointer to ith node in container
  ipv4Addr = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  addr = ipv4Addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.
  std::cout << "Gateway Node IP address on access link: " << addr << std::endl;
  addr = ipv4Addr->GetAddress (2, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface
  std::cout << "Gateway Node IP address on bottleneck link: " << addr
      << std::endl;

  node = sinkNode.Get (0); // Get pointer to ith node in container
  ipv4Addr = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  addr = ipv4Addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.
  std::cout << "sinkNode Node IP address on bottleneck link: " << addr
      << std::endl;

  uint16_t port = 9;
  BulkSendHelper source ("ns3::TcpSocketFactory",
			 InetSocketAddress (addr, port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (fileSize));
  source.SetAttribute ("SendSize", UintegerValue (fileSize));
  ApplicationContainer sourceApps = source.Install (srcNode.Get (0));
  sourceApps.Start (Seconds (0.02));
  sourceApps.Stop (Seconds (60.0));

  PacketSinkHelper sink ("ns3::TcpSocketFactory",
			 InetSocketAddress (addr, port));
  ApplicationContainer sinkApps = sink.Install (sinkNode.Get (0));
  sinkApps.Start (Seconds (0.01));
  sinkApps.Stop (Seconds (60.0));

  oss << "/NodeList/" << srcNode.Get (0)->GetId ()
      << "/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow";

  oss1 << "/NodeList/" << srcNode.Get (0)->GetId ()
      << "/$ns3::TcpL4Protocol/SocketList/0/RTT";

  oss2 << "/NodeList/" << srcNode.Get (0)->GetId()<< "/$ns3::TcpL4Protocol/SocketList/0/BytesInFlight";

  oss3 << "/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/CongState";

  Ptr<Queue<Packet> > queue = StaticCast<PointToPointNetDevice> (
      devicesBottleneckLink.Get (0))->GetQueue ();
  Ptr<QueueDisc> myq = qdiscs.Get (0);
  myq->TraceConnectWithoutContext ("BytesInQueue", MakeCallback (&Queues));

  FlowMonitorHelper flowmon;
  monitor = flowmon.InstallAll ();
  monitor = flowmon.GetMonitor ();

  monitor->CheckForLostPackets ();
  classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());

  NS_LOG_INFO("Run Simulation.");
  Simulator::Stop (Seconds (40.0));
  Simulator::Schedule (Seconds (0.1), &RTTConnect);
  Simulator::Schedule (Seconds (0.03), &CwndConnect);
  Simulator::Schedule (Seconds (0.1), &GetDelay);
  //Simulator::Schedule (Seconds (0.1), &GetQueue);
  //Simulator::Schedule (Seconds (0.02), &handler);

  Simulator::Run ();
  GetStats();
  Simulator::Destroy ();

}
