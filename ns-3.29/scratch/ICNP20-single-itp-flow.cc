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
static double delay = 0;

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
Queues (uint32_t oldSize, uint32_t newSize)
{
  QUEUE = newSize;
}
static void
EnQueues (uint64_t Id)
{
  std::cout << "\t"<<Simulator::Now().GetSeconds()<<"\t" << Id << "\n";
  //QUEUE = newSize;
}

static void
DeQueues (uint64_t Id)
{
  std::cout << "\t"<<Simulator::Now().GetSeconds()<<"\t\t" << Id << "\n";
  //QUEUE = newSize;
}

static void
DropID (uint64_t Id)
{
  std::cout << "\t"<<Simulator::Now().GetSeconds()<<"\t\t\t"<< Id << "\n";
  //QUEUE = newSize;
}

void
GetQueue ()
{

  std::cout << "\t" << delay<<"\t"<<QUEUE << "\n";
  Simulator::Schedule (Seconds ((0.05)), &GetQueue);
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
	 delay = i->second.lastDelay.GetSeconds ();
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
	  std::cout << "<Lost packets:\t" << i->second.lostPackets << std::endl;
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

void populateArray (std::vector<int>&contentStore, int coSize, int cachedChunks)
{
  double min = 0.0;
  double max = double(coSize-1);
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (min));
  x->SetAttribute ("Max", DoubleValue (max));
    for (int count = 0; count < cachedChunks; count++)
    {
	int y = x->GetInteger();
	if(contentStore[y]==1){
	    count--;
	}else{
	    contentStore[y]=1;
	}
    }
}
int
main (int argc, char *argv[])
{

  Config::SetDefault ("ns3::UdpSocket::RcvBufSize", UintegerValue (100000000));
  Config::SetDefault ("ns3::QueueBase::MaxSize",
		      QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, 1)));
  Config::SetDefault ("ns3::producer::PayloadSize", UintegerValue (1500));
  Config::SetDefault ("ns3::ItpCache::PayloadSize", UintegerValue (1500));
//  Config::SetDefault ("ns3::ItpCache::COSize", UintegerValue (7000));
//  Config::SetDefault ("ns3::ItpCache::CachedOldChunks", UintegerValue (4000));
  uint32_t fileSize = 100000*1500;

  double arr[] ={4.77559, 3.80026, 4.34133, 1.36966, 1.30947, 2.62596, 4.33498, 4.77051, 1.00044, 2.22108};
    int index=9;
    double startTime = arr[index];

  NodeContainer gwNode;
  gwNode.Create (1);

  NodeContainer gwNode2;
  gwNode2.Create (1);

  NodeContainer srcNode;
  srcNode.Create (1);

  NodeContainer sinkNode;
  sinkNode.Create (1);

  PointToPointHelper accessLink;
  accessLink.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  accessLink.SetChannelAttribute ("Delay", StringValue ("10ms"));
  accessLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
  accessLink.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesAccessLink;
  devicesAccessLink = accessLink.Install (srcNode.Get (0), gwNode.Get (0));

  PointToPointHelper BottleneckLink;
  BottleneckLink.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  BottleneckLink.SetChannelAttribute ("Delay", StringValue ("10ms"));
  BottleneckLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
  BottleneckLink.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesBottleneckLink;
  devicesBottleneckLink = BottleneckLink.Install (gwNode.Get (0),
						  gwNode2.Get (0));

  PointToPointHelper endLink;
  endLink.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  endLink.SetChannelAttribute ("Delay", StringValue ("10ms"));
  endLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));
  endLink.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesEndLink;
  devicesEndLink = endLink.Install (gwNode2.Get (0), sinkNode.Get (0));

  InternetStackHelper internet;
  internet.InstallAll ();

  /*
   * Make sure the queue size is in bytes in order to not cause any interest to be dropped
   */
  TrafficControlHelper tch1;
  uint16_t handle = tch1.SetRootQueueDisc ("ns3::FifoQueueDisc");
  tch1.AddInternalQueues (
       handle, 1, "ns3::DropTailQueue", "MaxSize",
       QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS,60)));
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

  tch1.Uninstall(devicesEndLink);

// Create Sink application on node one. The name is server but it acts as a sink in this application
  uint16_t port = 9;  // well-known echo port number
  ItpSinkHelper sink (port);
  ApplicationContainer apps = sink.Install (sinkNode.Get (0));
  apps.Start (Seconds (0.01));
  apps.Stop (Seconds (30.0));

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
  apps.Start (Seconds (startTime)); //set randomly
  apps.Stop (Seconds (30.0));

  PrintNodeInfo("src", srcNode.Get(0));
  PrintNodeInfo("gateway1", gwNode.Get(0));
  PrintNodeInfo("gateway2", gwNode2.Get(0));
  PrintNodeInfo("sink", sinkNode.Get(0));

  //Build trace source for queue size

  Ptr<Queue<Packet> > queue = StaticCast<PointToPointNetDevice> (
      devicesBottleneckLink.Get (0))->GetQueue ();

  //Build flow monitor to get stats
  FlowMonitorHelper flowmon;
  monitor = flowmon.InstallAll ();
  monitor = flowmon.GetMonitor ();
  classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  monitor->CheckForLostPackets ();

  NS_LOG_INFO("Run Simulation.");
  Simulator::Stop (Seconds (30.0));
  //Simulator::Schedule(Seconds(0.1), &GetPackets);
  //Simulator::Schedule(Seconds(0.1), &GetDelay);
  //Simulator::Schedule(Seconds(0.1), &GetQueue);
  //Simulator::Schedule(Seconds(0.1), &GetJitter);

  Simulator::Run ();
  //GetStats();
  Simulator::Destroy ();
  NS_LOG_INFO("Done.");
}
