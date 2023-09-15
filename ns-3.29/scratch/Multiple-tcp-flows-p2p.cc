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
      	  && t.destinationAddress == Ipv4Address ("10.0.1.2"))
      	{
      	  std::cout<<"Consumer 1:\t"
      	      << (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds ()
      	      << std::endl;
      	  std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
      	  std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

            }
      else if (t.sourceAddress == Ipv4Address ("10.0.0.1")
	  && t.destinationAddress == Ipv4Address ("10.0.2.2"))
	{
	  std::cout<<"\nConsumer 2:\t"
	      << (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds ()
	      << std::endl;
	  std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
	  std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

      }else if (t.sourceAddress == Ipv4Address ("10.0.0.1")
		&& t.destinationAddress == Ipv4Address ("10.0.3.2"))
	      {
		std::cout<<"\nConsumer 3:\t"
		    << (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds ()
		    << std::endl;
		std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
		std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

      } else if (t.sourceAddress == Ipv4Address ("10.0.0.1")
		&& t.destinationAddress == Ipv4Address ("10.0.4.2"))
	      {
		std::cout<<"\nConsumer 4:\t"
		    << (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds ()
		    << std::endl;
		std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
		std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

      } else if (t.sourceAddress == Ipv4Address ("10.0.0.1")
		&& t.destinationAddress == Ipv4Address ("10.0.5.2"))
	      {
		std::cout<<"\nConsumer 5:\t"
		    << (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds ()
		    << std::endl;
		std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
		std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

      } else if (t.sourceAddress == Ipv4Address ("10.0.0.1")
		&& t.destinationAddress == Ipv4Address ("10.0.6.2"))
	      {
		std::cout<<"\nConsumer 6:\t"
		    << (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds ()
		    << std::endl;
		std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
		std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

	      } else if (t.sourceAddress == Ipv4Address ("10.0.0.1")
			&& t.destinationAddress == Ipv4Address ("10.0.7.2"))
		      {
			std::cout<<"\nConsumer 7:\t"
			    << (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds ()
			    << std::endl;
			std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
			std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

		      } else if (t.sourceAddress == Ipv4Address ("10.0.0.1")
				&& t.destinationAddress == Ipv4Address ("10.0.8.2"))
			      {
				std::cout<<"\nConsumer 6:\t"
				    << (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds ()
				    << std::endl;
				std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
				std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

			      }
		      else if (t.sourceAddress == Ipv4Address ("10.0.0.1")
		     		&& t.destinationAddress == Ipv4Address ("10.0.9.2"))
		     	      {
		     		std::cout<<"\nConsumer 9:\t"
		     		    << (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds ()
		     		    << std::endl;
		     		std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
		     		std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

		     	      }
		      else if (t.sourceAddress == Ipv4Address ("10.0.0.1")
		     		&& t.destinationAddress == Ipv4Address ("10.0.10.2"))
		     	      {
		     		std::cout<<"\nConsumer 10:\t"
		     		    << (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds ()
		     		    << std::endl;
		     		std::cout << "transmitted bytes:\t" << i->second.txBytes << "\n";
		     		std::cout << "received bytes:\t" << i->second.rxBytes << "\n";

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
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1000));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (10000000));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (10000000));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (1));
  Config::SetDefault ("ns3::PacketSink::FileSize", UintegerValue (1500*4000));
  //Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (false));
  Config::SetDefault ("ns3::TcpSocket::InitialSlowStartThreshold", UintegerValue (30000));


	 //// Scenario 1 /////
	  double client2Scenario2Start[] ={137.155, 300.655, 178.897, 11.4698, 30.6659, 126.266, 126.311, 314.064,
			  32.0122, 133.839} ;

	//// Scenario 2 /////

	  double client2Scenario3Start[] ={59.3406, 31.2019, 30.5825,201.772,259.967,62.9985 ,107.74 ,150.431 ,29.5425
			  ,170.675} ;
	  double client3Scenario3Start[] ={239.822 ,247.797 ,336.028 ,243.759 ,47.2615 ,217.272 ,4.67128 ,192.427
			  ,49.3966 ,12.2001} ;


	//// Scenario 3 /////
	  double client2Scenario4Start[] ={5.35818
			  ,187.671
			  ,200.526
			  ,17.2831
			  ,307.3
			  ,40.8674
			  ,363.648
			  ,223.828
			  ,209.574
			  ,39.584} ;
	  double client3Scenario4Start[] ={255.026
			  ,415.41
			  ,174.645
			  ,115.467
			  ,173.665
			  ,88.6387
			  ,21.4351
			  ,276.287
			  ,470.653
			  ,387.785} ;
	  double client4Scenario4Start[] ={397.133
			  ,92.7125
			  ,41.5117
			  ,23.3973
			  ,64.8272
			  ,348.541
			  ,348.722
			  ,106.122
			  ,67.8528
			  ,381.679} ;


	//// Scenario 4 /////
	  double client2Scenario5Start[] ={205.301
			  ,403.01
			  ,281.988
			  ,152.821
			  ,15.7685
			  ,151.424
			  ,296.702
			  ,194.858
			  ,99.4684
			  ,318.842} ;
	  double client3Scenario5Start[] ={50.3997
			  ,94.9224
			  ,240.641
			  ,115.69
			  ,136.242
			  ,253.027
			  ,133.966
			  ,321.211
			  ,296.971
			  ,51.1706} ;
	  double client4Scenario5Start[] ={28.9706
			  ,533.894
			  ,52.8541
			  ,48.5233
			  ,106.723
			  ,314.11
			  ,542.734
			  ,87.2373
			  ,201.792
			  ,148.176} ;
	  double client5Scenario5Start[] ={221.523
			  ,261.907
			  ,39.3479
			  ,270.26
			  ,83.4878
			  ,587.377
			  ,133.457
			  ,39.1804
			  ,87.5193
			  ,153.123} ;



	//// Scenario 5 /////
	  double client2Scenario6Start[] ={461.944
			  ,232.508
			  ,195.071
			  ,98.4491
			  ,367.916
			  ,249.76
			  ,159.846
			  ,498.856
			  ,360.463
			  ,241.187} ;
	  double client3Scenario6Start[] ={371.844
			  ,101.417
			  ,394.166
			  ,5.12245
			  ,185.662
			  ,337.745
			  ,242.008
			  ,59.8722
			  ,402.556
			  ,171.125} ;
	  double client4Scenario6Start[] ={8.07358
			  ,40.2248
			  ,325.374
			  ,340.872
			  ,546.072
			  ,352.358
			  ,535.51
			  ,178.382
			  ,420.806
			  ,61.5242} ;
	  double client5Scenario6Start[] ={133.388
			  ,66.0318
			  ,64.6423
			  ,263.104
			  ,52.025
			  ,142.844
			  ,277.478
			  ,174.652
			  ,62.3171
			  ,266.148} ;
	  double client6Scenario6Start[] ={31.8775
			  ,39.8543
			  ,128.086
			  ,35.8169
			  ,103.387
			  ,9.32952
			  ,9.41645
			  ,466.316
			  ,108.564
			  ,24.9175} ;


	//// Scenario 6 /////
	  double client2Scenario7Start[] ={130.973
			  ,332.083
			  ,74.8954
			  ,193.623
			  ,241.227
			  ,69.7019
			  ,352.232
			  ,13.627
			  ,94.065
			  ,194.943} ;
	  double client3Scenario7Start[] ={161.617
			  ,297.623
			  ,76.2444
			  ,203.803
			  ,51.5054
			  ,203.515
			  ,113.602
			  ,581.92
			  ,433.393
			  ,550.698} ;
	  double client4Scenario7Start[] ={119.639
			  ,539.909
			  ,575.402
			  ,5.98395
			  ,111.761
			  ,192.094
			  ,195.761
			  ,217.003
			  ,25.665
			  ,377.974} ;
	  double client5Scenario7Start[] ={227.132
			  ,248.913
			  ,443.895
			  ,565.109
			  ,5.7806
			  ,66.0732
			  ,108.828
			  ,189.782
			  ,507.543
			  ,5.92058} ;
	  double client6Scenario7Start[] ={418.413
			  ,112.105
			  ,76.9127
			  ,496.772
			  ,124.697
			  ,34.4984
			  ,138.936
			  ,241.737
			  ,65.1022
			  ,127.248} ;
	  double client7Scenario7Start[] ={100.338
			  ,72.7136
			  ,254.652
			  ,299.331
			  ,562.253
			  ,67.8409
			  ,363.922
			  ,442.736
			  ,131.199
			  ,427.704} ;

	//// Scenario 7 /////

	  double client2Scenario8Start[] ={116.424
			  ,195.805
			  ,213.395
			  ,445.28
			  ,593.594
			  ,510.279
			  ,11.6137
			  ,69.4067
			  ,265.745
			  ,357.474} ;
	  double client3Scenario8Start[] ={64.2988
			  ,30.6034
			  ,367.773
			  ,367.601
			  ,446.548
			  ,163.457
			  ,23.4932
			  ,160.266
			  ,29.5174
			  ,154.882} ;
	  double client4Scenario8Start[] ={237.658
			  ,380.905
			  ,454.271
			  ,6.84956
			  ,281.612
			  ,80.0308
			  ,73.5411
			  ,92.0776
			  ,597.782
			  ,84.8197} ;
	  double client5Scenario8Start[] ={14.579
			  ,144.605
			  ,574.808
			  ,81.7912
			  ,99.3561
			  ,88.1953
			  ,155.703
			  ,266.331
			  ,316.25
			  ,85.1965} ;
	  double client6Scenario8Start[] ={10.8154
			  ,538.697
			  ,231.787
			  ,35.6248
			  ,459.765
			  ,215.005
			  ,449.206
			  ,15.8851
			  ,1.6302
			  ,179.844} ;
	  double client7Scenario8Start[] ={47.0835
			  ,207.466
			  ,89.7064
			  ,305.624
			  ,147.641
			  ,546.854
			  ,44.5223
			  ,68.3451
			  ,262.709
			  ,520.556} ;
	  double client8Scenario8Start[] ={189.192
			  ,227.597
			  ,477.881
			  ,48.7749
			  ,522.196
			  ,140.596
			  ,140.778
			  ,271.821
			  ,155.683
			  ,173.735} ;

	//// Scenario 8 /////
	  double client2Scenario9Start[] ={ 400.505
			  ,23.5217
			  ,66.0538
			  ,372.027
			  ,59.1703
			  ,356.357
			  ,215.023
			  ,377.219
			  ,158.838
			  ,175.923} ;
	  double client3Scenario9Start[] ={28.9654
			  ,102.439
			  ,475.161
			  ,38.3481
			  ,190.158
			  ,332.302
			  ,330.535
			  ,442.709
			  ,33.42
			  ,39.6777} ;
	  double client4Scenario9Start[] ={404.751
			  ,80.0116
			  ,539.479
			  ,189.654
			  ,18.5541
			  ,128.122
			  ,139.249
			  ,334.726
			  ,562.446
			  ,458.767} ;
	  double client5Scenario9Start[] ={340.304
			  ,496.59
			  ,580.299
			  ,7.71604
			  ,353.466
			  ,216.104
			  ,184.709
			  ,325.394
			  ,15.0434
			  ,119.548} ;
	  double client6Scenario9Start[] ={250.204
			  ,349.643
			  ,378.82
			  ,219.231
			  ,89.1248
			  ,11.1721
			  ,38.2071
			  ,184.787
			  ,34.2731
			  ,316.722} ;
	  double client7Scenario9Start[] ={232.851
			  ,105.443
			  ,30.5582
			  ,141.465
			  ,402.766
			  ,523.892
			  ,120.367
			  ,60.6428
			  ,42.157
			  ,451.703} ;
	  double client8Scenario9Start[] ={231.922
			  ,348.087
			  ,103.07
			  ,189.797
			  ,172.486
			  ,251.966
			  ,85.6316
			  ,577.647
			  ,99.1164
			  ,107.685} ;
	  double client9Scenario9Start[] ={315.991
			  ,565.485
			  ,6.44559
			  ,331.519
			  ,486.863
			  ,236.849
			  ,14.2379
			  ,163.612
			  ,182.34
			  ,38.198} ;

	//// Scenario 9 /////
	  double client2Scenario10Start[] ={156.955
			  ,492.804
			  ,555.218
			  ,443.233
			  ,307.031
			  ,306.103
			  ,183.416
			  ,3.2946
			  ,171.778
			  ,471.677} ;
	  double client3Scenario10Start[] ={455.003
			  ,541.233
			  ,64.1976
			  ,79.9906
			  ,24.095
			  ,173.426
			  ,105.918
			  ,2.46287
			  ,503.112
			  ,210.833} ;
	  double client4Scenario10Start[] ={402.954
			  ,161.933
			  ,507.867
			  ,188.58
			  ,73.7179
			  ,221.353
			  ,583.561
			  ,5.24755
			  ,80.1798
			  ,354.473} ;
	  double client5Scenario10Start[] ={282.204
			  ,31.3389
			  ,56.8953
			  ,42.7901
			  ,214.67
			  ,96.5147
			  ,557.665
			  ,302.253
			  ,37.374
			  ,87.9405} ;
	  double client6Scenario10Start[] ={49.4802
			  ,139.196
			  ,186.788
			  ,8.58666
			  ,32.4139
			  ,472.059
			  ,6.59966
			  ,25.1346
			  ,530.837
			  ,17.8778} ;
	  double client7Scenario10Start[] ={111.013
			  ,195.069
			  ,74.0441
			  ,480.28
			  ,392.821
			  ,511.775
			  ,88.7603
			  ,399.013
			  ,54.2596
			  ,110.9} ;
	  double client8Scenario10Start[] ={61.0408
			  ,234.578
			  ,32.6972
			  ,306.465
			  ,273.909
			  ,45.0852
			  ,382.263
			  ,113.269
			  ,249.306
			  ,112.901} ;
	  double client9Scenario10Start[] ={13.5809
			  ,325.955
			  ,117.057
			  ,106.441
			  ,199.572
			  ,106.166
			  ,334.79
			  ,210.754
			  ,89.0266
			  ,453.615} ;
	  double client10Scenario10Start[] ={539.97
			  ,53.9632
			  ,84.6471
			  ,62.3179
			  ,455.249
			  ,379.433
			  ,379.933
			  ,84.2575
			  ,211.603
			  ,482.091} ;


	    double c2Start, c3Start, c4Start, c5Start, c6Start, c7Start,c8Start, c9Start, c10Start;
	    double c2Stop, c3Stop, c4Stop, c5Stop, c6Stop, c7Stop, c8Stop, c9Stop, c10Stop;

	    CommandLine cmd;
	    int index;
	    cmd.AddValue ("index",  "an int argument", index);
	    cmd.Parse (argc, argv);

	    c2Start= client2Scenario10Start[index];
	    c3Start= client3Scenario10Start[index];
	    c4Start= client4Scenario10Start[index];
	    c5Start= client5Scenario10Start[index];
	    c6Start= client6Scenario10Start[index];
	    c7Start= client7Scenario10Start[index];
	    c8Start= client8Scenario10Start[index];
	    c9Start= client9Scenario10Start[index];
	    c10Start=client10Scenario10Start[index];

	    c2Stop = c2Start+10;
	    c3Stop = c3Start+10;
	    c4Stop = c4Start+10;
	    c5Stop = c5Start+10;
	    c6Stop = c6Start+10;
	    c7Stop = c7Start+10;
	    c8Stop = c8Start+10;
	    c9Stop = c9Start+10;
	    c10Stop= c10Start+10;

  std::cout<<"######### Scenario "<<index+1<<" #########\n";
  std::cout<<"Client 2 start: "<<c2Start<<std::endl;
  std::cout<<"Client 3 start: "<<c3Start<<std::endl;
  std::cout<<"Client 4 start: "<<c4Start<<std::endl;
  std::cout<<"Client 5 start: "<<c5Start<<std::endl;
  std::cout<<"Client 6 start: "<<c6Start<<std::endl;
  std::cout<<"Client 7 start: "<<c7Start<<std::endl;
  std::cout<<"Client 8 start: "<<c8Start<<std::endl;
  std::cout<<"Client 9 start: "<<c9Start<<std::endl;
  std::cout<<"Client 10 start: "<<c10Start<<std::endl;


  uint32_t fileSize = 6000*1000;

  NodeContainer gwNode;
  gwNode.Create (1);

  NodeContainer srcNode;
  srcNode.Create (1);

  NodeContainer consumer1;
  consumer1.Create (1);

  NodeContainer consumer2;
  consumer2.Create (1);

  NodeContainer consumer3;
  consumer3.Create (1);

  NodeContainer consumer4;
  consumer4.Create (1);

  NodeContainer consumer5;
  consumer5.Create (1);

  NodeContainer consumer6;
  consumer6.Create (1);

  NodeContainer consumer7;
     consumer7.Create (1);

   NodeContainer consumer8;
     consumer8.Create (1);

   NodeContainer consumer9;
     consumer9.Create (1);

   NodeContainer consumer10;
     consumer10.Create (1);

  PointToPointHelper accessLink;
  accessLink.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  accessLink.SetChannelAttribute ("Delay", StringValue ("5ms"));
  accessLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("140000B"));
  accessLink.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesAccessLink;
  devicesAccessLink = accessLink.Install (srcNode.Get (0), gwNode.Get (0));

  PointToPointHelper endLink1;
  endLink1.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  endLink1.SetChannelAttribute ("Delay", StringValue ("2ms"));
  endLink1.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("400000B"));
  endLink1.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesEndLink1;
  devicesEndLink1 = endLink1.Install (gwNode.Get (0), consumer1.Get (0));


  PointToPointHelper endLink2;
  endLink2.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  endLink2.SetChannelAttribute ("Delay", StringValue ("2ms"));
  endLink2.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("400000B"));
  endLink2.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesEndLink2;
  devicesEndLink2 = endLink2.Install (gwNode.Get (0), consumer2.Get (0));

  PointToPointHelper endLink3;
  endLink3.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  endLink3.SetChannelAttribute ("Delay", StringValue ("2ms"));
  endLink3.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("400000B"));
  endLink3.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesEndLink3;
  devicesEndLink3 = endLink3.Install (gwNode.Get (0), consumer3.Get (0));

  PointToPointHelper endLink4;
  endLink4.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  endLink4.SetChannelAttribute ("Delay", StringValue ("2ms"));
  endLink4.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("400000B"));
  endLink4.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesEndLink4;
  devicesEndLink4 = endLink4.Install (gwNode.Get (0), consumer4.Get (0));

  PointToPointHelper endLink5;
  endLink5.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  endLink5.SetChannelAttribute ("Delay", StringValue ("2ms"));
  endLink5.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("400000B"));
  endLink5.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesEndLink5;
  devicesEndLink5 = endLink5.Install (gwNode.Get (0), consumer5.Get (0));

  PointToPointHelper endLink6;
  endLink6.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  endLink6.SetChannelAttribute ("Delay", StringValue ("2ms"));
  endLink6.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("400000B"));
  endLink6.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesEndLink6;
  devicesEndLink6 = endLink6.Install (gwNode.Get (0), consumer6.Get (0));

  PointToPointHelper endLink7;
  endLink7.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  endLink7.SetChannelAttribute ("Delay", ("Delay", StringValue ("2ms")));
  endLink7.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("400000B"));
  endLink7.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesEndLink7;
  devicesEndLink7 = endLink7.Install (gwNode.Get (0), consumer7.Get (0));

  PointToPointHelper endLink8;
  endLink8.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  endLink8.SetChannelAttribute ("Delay", ("Delay", StringValue ("2ms")));
  endLink8.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("400000B"));
  endLink8.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesEndLink8;
  devicesEndLink8 = endLink8.Install (gwNode.Get (0), consumer8.Get (0));


  PointToPointHelper endLink9;
  endLink9.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  endLink9.SetChannelAttribute ("Delay", ("Delay", StringValue ("2ms")));
  endLink9.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("400000B"));
  endLink9.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesEndLink9;
  devicesEndLink9 = endLink9.Install (gwNode.Get (0), consumer9.Get (0));

  PointToPointHelper endLink10;
  endLink10.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  endLink10.SetChannelAttribute ("Delay", ("Delay", StringValue ("2ms")));
  endLink10.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("400000B"));
  endLink10.SetDeviceAttribute ("Mtu", UintegerValue (1600));
  NetDeviceContainer devicesEndLink10;
  devicesEndLink10 = endLink10.Install (gwNode.Get (0), consumer10.Get (0));



  InternetStackHelper internet;
  internet.InstallAll ();


// We've got the "hardware" in place.  Now we need to add IP addresses.
//
  NS_LOG_INFO("Assign IP Addresses.");

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.255.0");
  ipv4.Assign (devicesAccessLink);
  ipv4.NewNetwork ();
  ipv4.Assign (devicesEndLink1);

  ipv4.NewNetwork ();
  ipv4.Assign (devicesEndLink2);

  ipv4.NewNetwork ();
  ipv4.Assign (devicesEndLink3);

  ipv4.NewNetwork ();
  ipv4.Assign (devicesEndLink4);

  ipv4.NewNetwork ();
  ipv4.Assign (devicesEndLink5);

  ipv4.NewNetwork ();
  ipv4.Assign (devicesEndLink6);

  ipv4.NewNetwork ();
  ipv4.Assign (devicesEndLink7);

  ipv4.NewNetwork ();
  ipv4.Assign (devicesEndLink8);

  ipv4.NewNetwork ();
  ipv4.Assign (devicesEndLink9);

  ipv4.NewNetwork ();
  ipv4.Assign (devicesEndLink10);



  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  TrafficControlHelper tch1;
  tch1.Uninstall(devicesAccessLink);
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



  Ptr<Node> node;
  Ptr<Ipv4> addr;
  Ipv4Address contentProducer, client1, client2, client3, client4, client5, client6, client7, client8, client9, client10, gwnodeAdd;

  node = srcNode.Get (0); // Get pointer to ith node in container
  addr = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  contentProducer = addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.

  node = consumer1.Get (0); // Get pointer to ith node in container
  addr = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  client1 = addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.

  node = consumer2.Get (0); // Get pointer to ith node in container
  addr = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  client2 = addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.

  node = consumer3.Get (0); // Get pointer to ith node in container
  addr = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  client3 = addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.

  node = consumer4.Get (0); // Get pointer to ith node in container
  addr = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  client4 = addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.

  node = consumer5.Get (0); // Get pointer to ith node in container
  addr = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  client5 = addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.

  node = consumer6.Get (0); // Get pointer to ith node in container
  addr = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  client6 = addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.

  node = consumer7.Get (0); // Get pointer to ith node in container
  addr = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  client7 = addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.

  node = consumer8.Get (0); // Get pointer to ith node in container
  addr = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  client8 = addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.

  node = consumer9.Get (0); // Get pointer to ith node in container
  addr = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  client9 = addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.

  node = consumer10.Get (0); // Get pointer to ith node in container
  addr = node->GetObject<Ipv4> (); // Get Ipv4 instance of the node
  client10 = addr->GetAddress (1, 0).GetLocal (); // Get Ipv4InterfaceAddress of xth interface.


//  std::cout << "source address: " << contentProducer << std::endl;
//  std::cout << "client1 address: " << client1 << std::endl;
//  std::cout << "client2 address: " << client2 << std::endl;
//  std::cout << "client3 address: " << client3 << std::endl;
//  std::cout << "client4 address: " << client4 << std::endl;
//  std::cout << "client5 address: " << client5 << std::endl;
//  std::cout << "client6 address: " << client6 << std::endl;


  ////// Consumer 1 ///////////////////////////////////
  uint16_t port = 9;
  BulkSendHelper source ("ns3::TcpSocketFactory",
			 InetSocketAddress (client1, port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (fileSize));
  source.SetAttribute ("SendSize", UintegerValue (fileSize));
  ApplicationContainer sourceApps = source.Install (srcNode.Get (0));
  sourceApps.Start (Seconds (0.02));
  sourceApps.Stop (Seconds (1000));

  PacketSinkHelper sink ("ns3::TcpSocketFactory",
			 InetSocketAddress (client1, port));
  ApplicationContainer sinkApps = sink.Install (consumer1.Get (0));
  sinkApps.Start (Seconds (0.01));
  sinkApps.Stop (Seconds (1000));
  /////////////////////////////////////


  ////// Consumer 2 ///////////
  uint16_t port2 = 9;
  BulkSendHelper source2 ("ns3::TcpSocketFactory",
			 InetSocketAddress (client2, port2));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source2.SetAttribute ("MaxBytes", UintegerValue (fileSize));
  source2.SetAttribute ("SendSize", UintegerValue (fileSize));
  ApplicationContainer sourceApps2 = source2.Install (srcNode.Get (0));
  sourceApps2.Start (Seconds (c2Start));
  sourceApps2.Stop (Seconds (1000));

  PacketSinkHelper sink2 ("ns3::TcpSocketFactory",
			 InetSocketAddress (client2, port2));
  ApplicationContainer sinkApps2 = sink2.Install (consumer2.Get (0));
  sinkApps2.Start (Seconds (c2Start-0.01));
  sinkApps2.Stop (Seconds (1000));
  //////////////////////////////////////////////////////////////////////////


  ////// Consumer 3 /////////////////////////////////////////////////////
  uint16_t port3 = 9;
  BulkSendHelper source3 ("ns3::TcpSocketFactory",
			 InetSocketAddress (client3, port3));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source3.SetAttribute ("MaxBytes", UintegerValue (fileSize));
  source3.SetAttribute ("SendSize", UintegerValue (fileSize));
  ApplicationContainer sourceApps3 = source3.Install (srcNode.Get (0));
  sourceApps3.Start (Seconds (c3Start));
  sourceApps3.Stop (Seconds (1000));

  PacketSinkHelper sink3 ("ns3::TcpSocketFactory",
			 InetSocketAddress (client3, port3));
  ApplicationContainer sinkApps3 = sink3.Install (consumer3.Get (0));
  sinkApps3.Start (Seconds (c3Start-0.01));
  sinkApps3.Stop (Seconds (1000));
  //////////////////////////////////////////////////////////////////////////


  ////// Consumer 4 /////////////////////////////////////////////////////
  uint16_t port4 = 9;
  BulkSendHelper source4 ("ns3::TcpSocketFactory",
			 InetSocketAddress (client4, port4));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source4.SetAttribute ("MaxBytes", UintegerValue (fileSize));
  source4.SetAttribute ("SendSize", UintegerValue (fileSize));
  ApplicationContainer sourceApps4 = source4.Install (srcNode.Get (0));
  sourceApps4.Start (Seconds (c4Start));
  sourceApps4.Stop (Seconds (1000.0));

  PacketSinkHelper sink4 ("ns3::TcpSocketFactory",
			 InetSocketAddress (client4, port4));
  ApplicationContainer sinkApps4 = sink4.Install (consumer4.Get (0));
  sinkApps4.Start (Seconds (c4Start-0.01));
  sinkApps4.Stop (Seconds (1000.0));
  //////////////////////////////////////////////////////////////////////////


  ////// Consumer 5 /////////////////////////////////////////////////////
  uint16_t port5 = 9;
  BulkSendHelper source5 ("ns3::TcpSocketFactory",
			 InetSocketAddress (client5, port5));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source5.SetAttribute ("MaxBytes", UintegerValue (fileSize));
  source5.SetAttribute ("SendSize", UintegerValue (fileSize));
  ApplicationContainer sourceApps5 = source5.Install (srcNode.Get (0));
  sourceApps5.Start (Seconds (c5Start));
  sourceApps5.Stop (Seconds (1000.0));

  PacketSinkHelper sink5 ("ns3::TcpSocketFactory",
			 InetSocketAddress (client5, port5));
  ApplicationContainer sinkApps5 = sink5.Install (consumer5.Get (0));
  sinkApps5.Start (Seconds (c5Start-0.01));
  sinkApps5.Stop (Seconds (1000.0));
  //////////////////////////////////////////////////////////////////////////


  ////// Consumer 6 /////////////////////////////////////////////////////
   uint16_t port6 = 9;
   BulkSendHelper source6 ("ns3::TcpSocketFactory",
 			 InetSocketAddress (client6, port6));
   // Set the amount of data to send in bytes.  Zero is unlimited.
   source6.SetAttribute ("MaxBytes", UintegerValue (fileSize));
   source6.SetAttribute ("SendSize", UintegerValue (fileSize));
   ApplicationContainer sourceApps6 = source6.Install (srcNode.Get (0));
   sourceApps6.Start (Seconds (c6Start));
   sourceApps6.Stop (Seconds (1000.0));

   PacketSinkHelper sink6 ("ns3::TcpSocketFactory",
 			 InetSocketAddress (client6, port6));
   ApplicationContainer sinkApps6 = sink6.Install (consumer6.Get (0));
   sinkApps6.Start (Seconds (c6Start-0.01));
   sinkApps6.Stop (Seconds (1000.0));
   //////////////////////////////////////////////////////////////////////////


   ////// Consumer 7 /////////////////////////////////////////////////////
    uint16_t port7 = 9;
    BulkSendHelper source7 ("ns3::TcpSocketFactory",
  			 InetSocketAddress (client7, port7));
    // Set the amount of data to send in bytes.  Zero is unlimited.
    source7.SetAttribute ("MaxBytes", UintegerValue (fileSize));
    source7.SetAttribute ("SendSize", UintegerValue (fileSize));
    ApplicationContainer sourceApps7 = source7.Install (srcNode.Get (0));
    sourceApps7.Start (Seconds (c7Start));
    sourceApps7.Stop (Seconds (1000.0));

    PacketSinkHelper sink7 ("ns3::TcpSocketFactory",
  			 InetSocketAddress (client7, port7));
    ApplicationContainer sinkApps7 = sink7.Install (consumer7.Get (0));
    sinkApps7.Start (Seconds (c7Start-0.01));
    sinkApps7.Stop (Seconds (1000.0));
    //////////////////////////////////////////////////////////////////////////


    ////// Consumer 8 /////////////////////////////////////////////////////
     uint16_t port8 = 9;
     BulkSendHelper source8 ("ns3::TcpSocketFactory",
   			 InetSocketAddress (client8, port8));
     // Set the amount of data to send in bytes.  Zero is unlimited.
     source8.SetAttribute ("MaxBytes", UintegerValue (fileSize));
     source8.SetAttribute ("SendSize", UintegerValue (fileSize));
     ApplicationContainer sourceApps8 = source8.Install (srcNode.Get (0));
     sourceApps8.Start (Seconds (c8Start));
     sourceApps8.Stop (Seconds (1000.0));

     PacketSinkHelper sink8 ("ns3::TcpSocketFactory",
   			 InetSocketAddress (client8, port8));
     ApplicationContainer sinkApps8 = sink8.Install (consumer8.Get (0));
     sinkApps8.Start (Seconds (c8Start-0.01));
     sinkApps8.Stop (Seconds (1000.0));
     //////////////////////////////////////////////////////////////////////////

     ////// Consumer 9 /////////////////////////////////////////////////////
      uint16_t port9 = 9;
      BulkSendHelper source9 ("ns3::TcpSocketFactory",
    			 InetSocketAddress (client9, port9));
      // Set the amount of data to send in bytes.  Zero is unlimited.
      source9.SetAttribute ("MaxBytes", UintegerValue (fileSize));
      source9.SetAttribute ("SendSize", UintegerValue (fileSize));
      ApplicationContainer sourceApps9 = source9.Install (srcNode.Get (0));
      sourceApps9.Start (Seconds (c9Start));
      sourceApps9.Stop (Seconds (1000.0));

      PacketSinkHelper sink9 ("ns3::TcpSocketFactory",
    			 InetSocketAddress (client9, port9));
      ApplicationContainer sinkApps9 = sink9.Install (consumer9.Get (0));
      sinkApps9.Start (Seconds (c9Start-0.01));
      sinkApps9.Stop (Seconds (1000.0));
      ////////////////////////////////////////////////////////

      ////// Consumer 10 /////////////////////////////////////////////////////
       uint16_t port10 = 9;
       BulkSendHelper source10 ("ns3::TcpSocketFactory",
     			 InetSocketAddress (client10, port10));
       // Set the amount of data to send in bytes.  Zero is unlimited.
       source10.SetAttribute ("MaxBytes", UintegerValue (fileSize));
       source10.SetAttribute ("SendSize", UintegerValue (fileSize));
       ApplicationContainer sourceApps10 = source10.Install (srcNode.Get (0));
       sourceApps10.Start (Seconds (c10Start));
       sourceApps10.Stop (Seconds (1000.0));

       PacketSinkHelper sink10 ("ns3::TcpSocketFactory",
     			 InetSocketAddress (client10, port9));
       ApplicationContainer sinkApps10 = sink10.Install (consumer10.Get (0));
       sinkApps10.Start (Seconds (c10Start-0.01));
       sinkApps10.Stop (Seconds (1000.0));



  oss << "/NodeList/" << srcNode.Get (0)->GetId ()
      << "/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow";

  oss1 << "/NodeList/" << srcNode.Get (0)->GetId ()
      << "/$ns3::TcpL4Protocol/SocketList/0/RTT";

  oss2 << "/NodeList/" << srcNode.Get (0)->GetId()<< "/$ns3::TcpL4Protocol/SocketList/0/BytesInFlight";

  oss3 << "/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/CongState";

//  Ptr<Queue<Packet> > queue = StaticCast<PointToPointNetDevice> (
//      devicesBottleneckLink.Get (0))->GetQueue ();

  FlowMonitorHelper flowmon;
  monitor = flowmon.InstallAll ();
  monitor = flowmon.GetMonitor ();

  monitor->CheckForLostPackets ();
  classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());

  NS_LOG_INFO("Run Simulation.");
  Simulator::Stop (Seconds (1000.0));
  Simulator::Schedule (Seconds (0.1), &RTTConnect);
  Simulator::Schedule (Seconds (0.03), &CwndConnect);
  Simulator::Schedule (Seconds (0.1), &GetDelay);
  Simulator::Schedule (Seconds (0.1), &GetQueue);
  //Simulator::Schedule (Seconds (0.02), &handler);


  Simulator::Run ();
  GetStats();
  Simulator::Destroy ();

}
