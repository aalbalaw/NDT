#include "ns3/log.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/string.h"
#include "ns3/producer.h"
#include "ns3/itp-header.h"
#include <string>
#include <iostream>
#include "ns3/hash.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("producer");

NS_OBJECT_ENSURE_REGISTERED (producer);

TypeId
producer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::producer")
    .SetParent<Object> ()
    .SetGroupName("Internet")
    .AddConstructor<producer> ()
	.AddAttribute ("PayloadSize",
	                   "The destination port of the outbound packets",
	                   UintegerValue (1000),
	                   MakeUintegerAccessor (&producer::m_payloadSize),
	                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

producer::producer ()
{
 // NS_LOG_FUNCTION (this);
}

producer::~producer()
{
  //NS_LOG_FUNCTION (this);
}

void
producer::SetSendCallback (Callback<void, Ptr<Packet>, Address > sendData)
{
	m_sendData = sendData;
}

void producer::ReceiveContent(int size, Address to){

	SendManifest(size,to);
}

void producer::SendManifest(int size, Address to){

	int maxSeq = size/m_payloadSize;
	NS_LOG_INFO("size: "<<size<<"payloadSize: "<<m_payloadSize);
	std::string fill = std::to_string(maxSeq);
	uint32_t dataSize = fill.size () + 1;
	NS_LOG_INFO("Max Seq: "<<fill);
	uint8_t* m_data = new uint8_t [dataSize];
	memcpy (m_data, fill.c_str (), dataSize);
	Ptr<Packet> p = Create<Packet> (m_data, dataSize);
	ItpHeader mHdr;
	mHdr.SetType(1);
	mHdr.SetSeqNumber(0);
	mHdr.SetName(Hash32("Hello"));
	p->AddHeader(mHdr);
	//NS_LOG_INFO("Manifest Size: "<<p->GetSize());
	m_sendData(p,to);
}

void producer::OnInterest(Ptr<Packet> pkt, Address from){

	ItpHeader rcvHdr;
	Ptr<Packet> p = Create<Packet> (m_payloadSize);
	pkt->RemoveHeader(rcvHdr);
	ItpHeader mHdr;
	mHdr.SetType(3);
	mHdr.SetSeqNumber(rcvHdr.GetSeqNumber());
	//NS_LOG_INFO("Interest : "<<rcvHdr.GetSeqNumber() <<" from: "<<InetSocketAddress::ConvertFrom (from).GetIpv4 () << "id: "<<pkt->GetUid());
	mHdr.SetName(rcvHdr.GetName());
	mHdr.SetCounter(rcvHdr.GetCounter());
	p->AddHeader(mHdr);
	//NS_LOG_INFO("Sending Data: "<< mHdr.GetSeqNumber() <<" counter: "<<mHdr.GetCounter());
	m_sendData(p,from);
	m_sum+=1;
//	if(m_sum==1){
//		SendManifest(1000,from);
//	}
}

}
