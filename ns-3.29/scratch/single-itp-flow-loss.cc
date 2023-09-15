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
static Ptr<Ipv4FlowClassifier> classifier;
static Ptr<FlowMonitor> monitor;
static uint64_t QUEUE;
static double jitter = 0;
static uint32_t lastPackets = 0;
static uint32_t CWND = 0;
//std::vector<int> index;
static int counter=-1;

void
PrintNodeInfo (std::string name, Ptr<Node> node)
{
  Ptr<Ipv4> ipv4Addr;
  Ipv4Address addr;
  ipv4Addr = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  addr = ipv4Addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.
  std::cout << name<<" Node ID " << node->GetId ()
      << " IP address on access link: " << addr << std::endl;

}
static void
EnQueues (uint64_t Id)
{
  //index.push_back(Id);
 // std::cout << "\t"<<Simulator::Now().GetSeconds()<<"\t" << Id <<"\t"<<counter<< "\n";
  counter++;
  //QUEUE = newSize;
}

static void
DeQueues (uint64_t Id)
{
 // std::cout << "\t"<<Simulator::Now().GetSeconds()<<"\t\t" << Id << "\n";
  //QUEUE = newSize;
}

static void
DropID (uint64_t Id)
{
  //index.push_back(Id);
  counter++;
  //std::cout << "\t"<<Simulator::Now().GetSeconds()<<"\t\t\t"<< Id <<"\t"<<counter<< "\n";
  //QUEUE = newSize;
}

void
GetQueue ()
{

 // std::cout << "\t"<<Simulator::Now().GetMilliSeconds()<<"\t" << QUEUE << "\n";
  //Simulator::Schedule (Seconds ((0.1)), &GetQueue);
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
	  std::cout << "Delay:\t" << i->second.lastDelay.GetSeconds () << "\n";
	}
    }
  Simulator::Schedule (Seconds ((0.1)), &GetDelay);
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
	  std::cout << i->second.txPackets - lastPackets << "\n";
	  lastPackets = i->second.txPackets;
	}
    }
  Simulator::Schedule (Seconds ((0.1)), &GetPackets);
}

void
GetStats ()
{
  std::cout << "From source(producer) to sink(consumer)" << std::endl;
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
	  std::cout << "<bytesDropped:\t" << i->second.lostPackets << std::endl;
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

void
GetStats2 ()
{
  std::cout << "From sink(consumer) to source(producer)" << std::endl;
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
      stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

      if (t.sourceAddress == Ipv4Address ("10.0.2.2")
	  && t.destinationAddress == Ipv4Address ("10.0.0.1"))
	{
	  std::cout << "Total Time:\t"
	      << (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds ()
	      << std::endl;
	  std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
	  std::cout << "received bytes:\t" << i->second.rxBytes << "\n";
	  std::cout << "<bytesDropped:\t" << i->second.lostPackets << std::endl;
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

static void
CwndChange (uint32_t newCwnd)
{
  CWND = newCwnd;
  std::cout<<"Here"<<CWND<<std::endl;
}

void
CwndConnect ()
{
  Config::ConnectWithoutContext ("/NodeList/*/ApplicationList/*/$ns3::ItpSource/ITP/CongestionWindow", MakeCallback (&CwndChange));
}


int
main (int argc, char *argv[])
{

  Config::SetDefault ("ns3::UdpSocket::RcvBufSize", UintegerValue (100000000));
  Config::SetDefault ("ns3::QueueBase::MaxSize",
		      QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, 1)));
  Config::SetDefault ("ns3::producer::PayloadSize", UintegerValue (1500));
  Config::SetDefault ("ns3::ItpCache::PayloadSize", UintegerValue (1500));
  uint32_t fileSize = 1*1500;
      //7999500;

  NodeContainer gwNode;
  gwNode.Create (1);

  NodeContainer gwNode2;
  gwNode2.Create (1);

  NodeContainer srcNode;
  srcNode.Create (1);

  NodeContainer sinkNode;
  sinkNode.Create (1);

  PointToPointHelper accessLink;
  accessLink.SetDeviceAttribute ("DataRate", StringValue ("10000Mbps"));
  accessLink.SetChannelAttribute ("Delay", StringValue ("33ms"));
  accessLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
  accessLink.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesAccessLink;
  devicesAccessLink = accessLink.Install (srcNode.Get (0), gwNode.Get (0));

  PointToPointHelper BottleneckLink;
  BottleneckLink.SetDeviceAttribute ("DataRate", StringValue ("10000Mbps"));
  BottleneckLink.SetChannelAttribute ("Delay", StringValue ("5ms"));
  BottleneckLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
  BottleneckLink.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesBottleneckLink;
  devicesBottleneckLink = BottleneckLink.Install (gwNode.Get (0),
						  gwNode2.Get (0));

  PointToPointHelper endLink;
  endLink.SetDeviceAttribute ("DataRate", StringValue ("10000Mbps"));
  endLink.SetChannelAttribute ("Delay", StringValue ("2ms"));
  endLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
  endLink.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesEndLink;
  devicesEndLink = endLink.Install (gwNode2.Get (0), sinkNode.Get (0));

  InternetStackHelper internet;
  internet.InstallAll ();

  Ptr<Ipv4L3Protocol> ipv4Instance = gwNode2.Get(0)->GetObject<Ipv4L3Protocol> ();
 // ipv4Instance->SetCache();

  Ptr<Ipv4L3Protocol> ipv4Instance2 = gwNode.Get(0)->GetObject<Ipv4L3Protocol> ();
 // ipv4Instance2->SetCache();

  /*
   * Make sure the queue size is in bytes in order to not cause any interest to be dropped
   */
  TrafficControlHelper tch1;
  uint16_t handle = tch1.SetRootQueueDisc ("ns3::FifoQueueDisc");
  tch1.AddInternalQueues (
       handle, 1, "ns3::DropTailQueue", "MaxSize",
       QueueSizeValue (QueueSize (QueueSizeUnit::BYTES, 180000)));
  QueueDiscContainer qdiscs = tch1.Install (devicesBottleneckLink);

  //Assign IP Addresses

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.255.0");
  ipv4.Assign (devicesAccessLink);
  ipv4.NewNetwork ();
  ipv4.Assign (devicesBottleneckLink);
  ipv4.NewNetwork ();
  ipv4.Assign (devicesEndLink);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

// Create Sink application on node one. The name is server but it acts as a sink in this application
  uint16_t port = 9;  // well-known echo port number
  ItpSinkHelper sink (port);
  ApplicationContainer apps = sink.Install (sinkNode.Get (0));
  apps.Start (Seconds (0.01));
  apps.Stop (Seconds (60.0));

// Create Client application to send traffic to Server
  Ptr<Ipv4> ipv4Addr;
  Ipv4Address addr;
  ipv4Addr = sinkNode.Get (0)->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  addr = ipv4Addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.
  uint32_t maxPacketCount = 1;
  Time interPacketInterval = Seconds (0);
  ItpSourceHelper source (addr, port);
  source.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  source.SetAttribute ("Interval", TimeValue (interPacketInterval));
  source.SetAttribute ("PacketSize", UintegerValue (fileSize));
  apps = source.Install (srcNode.Get (0));
  apps.Start (Seconds (0.02));
  apps.Stop (Seconds (60.0));

  PrintNodeInfo("src", srcNode.Get(0));
  PrintNodeInfo("gateway1", gwNode.Get(0));
  PrintNodeInfo("gateway2", gwNode2.Get(0));
  PrintNodeInfo("sink", sinkNode.Get(0));

  /*Setting our error model
   * Every node has at least two net devices
   * One for loopback int and the other for ppp
   * You can only corrupt received packets based on their id
   * net device 1 is for the loopback interface
   */
//  std::list<uint32_t> sampleList;
//  sampleList.push_back (183);
////  // This time, we'll explicitly create the error model we want
//  Ptr<ListErrorModel> pem = CreateObject<ListErrorModel> ();
//  pem->SetList (sampleList);
  //gwNode.Get(0)->GetDevice(0)->SetAttribute ("ReceiveErrorModel", PointerValue (pem));

  //Build trace source for queue size
  Ptr<QueueDisc> myq = qdiscs.Get (0);
  myq->TraceConnectWithoutContext ("EnqueueID", MakeCallback (&EnQueues));
  myq->TraceConnectWithoutContext ("DequeueID", MakeCallback (&DeQueues));
  myq->TraceConnectWithoutContext ("DropID", MakeCallback (&DropID));

  //Build flow monitor to get stats
  FlowMonitorHelper flowmon;
  monitor = flowmon.InstallAll ();
  monitor = flowmon.GetMonitor ();
  classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  monitor->CheckForLostPackets ();

  NS_LOG_INFO("Run Simulation.");
  Simulator::Stop (Seconds (40.0));
  //Simulator::Schedule(Seconds(0.1), &GetPackets);
  //Simulator::Schedule(Seconds(0.1), &GetDelay);
  //Simulator::Schedule(Seconds(0.1), &GetQueue);
  //Simulator::Schedule(Seconds(0.1), &GetJitter);

  Simulator::Run ();
  GetStats();
  Simulator::Destroy ();
}
