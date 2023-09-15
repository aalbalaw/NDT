/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Federal University of Uberlandia
 *
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
 * along with this program; if not, see <echo://www.gnu.org/licenses/>.
 *
 * Author: Saulo da Mata <damata.saulo@gmail.com>
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/random-variable-stream.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("HttpClientServerExample");
static std::vector<std::list<int>> pendingConsumers;
static std::vector<int> cachedContent;

//1. check if data already cached
void OnInterest(int seq, int consumer) {

	if(cachedContent[seq]){
		std::cout<<"Data already cached, sending data to "<<consumer<<"\n";
	}else{
		//add to pending list
		pendingConsumers[seq].push_back(consumer);
	}

}

//1 cache the chunk
//check for pending consumers
//send data
void OnData(int seq) {

	cachedContent[seq] = 1;
	int pending = pendingConsumers[seq].size();

	//as long as we have pending consumers we should send the data
	while(!pendingConsumers[seq].empty()){
		std::cout<<"Sending Data to: "<<pendingConsumers[seq].front()<<std::endl;
		pendingConsumers[seq].pop_front();
	}

}

int main(int argc, char *argv[]) {
	//Enabling logging

	std::vector<std::vector<int>>objectsSize(10,vector<int>(10,0));
	objectsSize[0][0]=1000;


	Ptr<LogNormalRandomVariable> m_logNormal;
	Ptr<WeibullRandomVariable> m_mainObjectSizeStream;

	m_logNormal = CreateObject<
			LogNormalRandomVariable>();
	m_logNormal->SetAttribute("Mu", DoubleValue(8.91365));
	m_logNormal->SetAttribute("Sigma", DoubleValue(1.24816));

	m_mainObjectSizeStream = CreateObject<
			WeibullRandomVariable>();
	m_mainObjectSizeStream->SetAttribute("Scale", DoubleValue(19104.9));
	m_mainObjectSizeStream->SetAttribute("Shape", DoubleValue(0.771807));



	double min = 1.0;
	double max = 5.0;
	Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
	x->SetAttribute ("Min", DoubleValue (min));
	x->SetAttribute ("Max", DoubleValue (max));
	// The values returned by a uniformly distributed random
	// variable should always be within the range
	//
	//     [min, max)  .
	//

	for(int i=0;i<10;i++){
		std::cout<<x->GetValue ()<<", ";
	}

	std::cout<<"\n";

	for(int i=0;i<10;i++){
		std::cout<<m_logNormal->GetInteger()<<"  ";
	}

	std::cout<<"\n";


	vector<vector<int>> vec{ { 2073 },
	                         { 10762,33375 },
	                         { 16396,6646,3982 },
							 { 119538,2471,15290,36732 },
							 { 14781,3998,3011,1753,1372 }};



	//This is for deciding the content size;
	int scenario = 1;
	double contentSize=0;
	for(int i=0;i<vec[scenario].size();i++){
		std::cout<<i<<": "<<objectsSize[scenario][i]<<std::endl;
		contentSize+=vec[scenario][i];
	}
	std::cout<<contentSize<<std::endl;


	//std::cout<<pendingConsumers[0]<<std::endl;

	Simulator::Stop(Seconds(10.0));

	NS_LOG_INFO("Starting Simulation...");
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_INFO("\ndone!");
	return 0;

}
