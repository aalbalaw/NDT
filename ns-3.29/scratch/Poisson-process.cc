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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

static  Ptr<ExponentialRandomVariable> y;
NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

void GenerateClient2(){
       std::cout<<y->GetValue()<<std::endl;
}

void GenerateClient3(){
       std::cout<<y->GetValue()<<"\t"<<y->GetValue()<<std::endl;
}

void GenerateClient4(){
       std::cout<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<std::endl;
}

void GenerateClient5(){
       std::cout<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<std::endl;
}

void GenerateClient6(){
       std::cout<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<std::endl;
}

void GenerateClient7(){
       std::cout<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"
	   <<y->GetValue()<<"\t"<<y->GetValue()<<std::endl;
}

void GenerateClient8(){
       std::cout<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()
	   <<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<std::endl;
}

void GenerateClient9(){
  std::cout<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"
	   <<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<std::endl;
}

void GenerateClient10(){
  std::cout<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"
	   <<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<"\t"<<y->GetValue()<<
	   "\t"<<y->GetValue()<<std::endl;
}

int
main (int argc, char *argv[])
{

  //d
  RngSeedManager::SetSeed (5);  // Changes seed from default of 1 to 3
  double lambda = (6.0);

  y = CreateObject<ExponentialRandomVariable> ();
  y->SetAttribute("Bound", DoubleValue (6));
  y->SetAttribute ("Mean", DoubleValue (lambda));
 // RngSeedManager::SetRun (7);   // Changes run number from default of 1 to 7
  // Now, create random variables


  GenerateClient6();
  GenerateClient6();
  GenerateClient6();

  GenerateClient6();
  GenerateClient6();

  GenerateClient6();
  GenerateClient6();


  GenerateClient6();
  GenerateClient6();



//  double sum =0;
//  for (int i=0;i<1000000;i++){
//      sum+=y->GetValue();
//  }
 // std::cout<<sum/1000000.0<<std::endl;

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
