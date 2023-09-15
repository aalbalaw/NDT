#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/callback.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-route.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/net-device.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/object-vector.h"
#include "ns3/ipv4-header.h"
#include "ns3/boolean.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/traffic-control-layer.h"
#include "ns3/string.h"
#include "loopback-net-device.h"
#include "arp-l3-protocol.h"
#include "arp-cache.h"
#include "ipv4-l3-protocol.h"
#include "icmpv4-l4-protocol.h"
#include "ipv4-interface.h"
#include "ipv4-raw-socket-impl.h"
#include "ns3/udp-header.h"
#include "ns3/itp-header.h"
#include "ns3/cctpCache.h"
#include "ns3/double.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("CctpCache");

NS_OBJECT_ENSURE_REGISTERED(CctpCache);

TypeId CctpCache::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::CctpCache").SetParent<Object>().SetGroupName(
					"Internet").AddConstructor<CctpCache>().AddAttribute(
					"PayloadSize",
					"The destination port of the outbound packets",
					UintegerValue(1500),
					MakeUintegerAccessor(&CctpCache::m_payloadSize),
					MakeUintegerChecker<uint32_t>()).AddAttribute(
					"CachedOldChunks",
					"The destination port of the outbound packets",
					UintegerValue(0),
					MakeUintegerAccessor(&CctpCache::m_oldChunks),
					MakeUintegerChecker<uint32_t>()).AddAttribute("COSize",
					"The destination port of the outbound packets",
					UintegerValue(10000),
					MakeUintegerAccessor(&CctpCache::m_coSize),
					MakeUintegerChecker<uint32_t>())

					;
	return tid;
}

CctpCache::CctpCache() {

}

CctpCache::~CctpCache() {

}

MyTag CctpCache::PreserveAddress(UdpHeader udp, Ipv4Header ip){

	MyTag cctpTag;
	cctpTag.SetSourceIP(ip.GetSource());
	cctpTag.SetDestinationIP(ip.GetDestination());
	cctpTag.SetSourcePort(udp.GetSourcePort());
	cctpTag.SetDestinationPort(udp.GetDestinationPort());
	return cctpTag;

}
Ptr<Packet> CctpCache::CheckPacket(Ptr<Packet> p) {

	//NS_LOG_INFO("Packet Size before remv headers: "<<p->GetSize());
	Ipv4Header ipHeader;
	p->RemoveHeader(ipHeader);
	//NS_LOG_INFO("Packet Size: "<<p->GetSize()<<" payload size in ip "<<ipHeader.GetPayloadSize());
	UdpHeader udpHdr;
	p->RemoveHeader(udpHdr);
	ItpHeader rcvHdr;
	p->RemoveHeader(rcvHdr);

	if (rcvHdr.GetType() == 2) {
		p->AddHeader(rcvHdr);
		p->AddHeader(udpHdr);
		p->AddHeader(ipHeader);
		return OnInterest(p);
	} else if (rcvHdr.GetType() == 3 || rcvHdr.GetType() == 4) {
		p->AddHeader(rcvHdr);
		p->AddHeader(udpHdr);
		p->AddHeader(ipHeader);
	//	OnData(rcvHdr.GetSeqNumber());
	}

	p->AddHeader(rcvHdr);
	p->AddHeader(udpHdr);
	p->AddHeader(ipHeader);
	return p;
}

/*
 * Check list to see if we have cached content, then check vector to see if we have cached chunk
 */
Ptr<Packet> CctpCache::OnInterest(Ptr<Packet> interest) {

	Ipv4Header ipHdr;
	interest->RemoveHeader(ipHdr);
	UdpHeader udpHdr;
	interest->RemoveHeader(udpHdr);
	ItpHeader itpHdr;
	interest->RemoveHeader(itpHdr);

	uint32_t contentName = itpHdr.GetName();
	bool found = false;
	NS_LOG_INFO(
			"Received Interest: "<<itpHdr.GetSeqNumber() << " Count: "<<itpHdr.GetCounter());

	if (m_oneContent[itpHdr.GetSeqNumber()] == 0) {
		found = true;
		m_satesfiedRequests += 1;
		NS_LOG_INFO("m_satesfiedRequests "<<m_satesfiedRequests);
		NS_LOG_INFO(
				"Satisfied Interest: "<<itpHdr.GetSeqNumber() << " Count: "<<itpHdr.GetCounter());
		ItpHeader mHdr;
		mHdr.SetType(2);
		mHdr.SetSeqNumber(itpHdr.GetSeqNumber());
		mHdr.SetName(itpHdr.GetName());
		// mHdr.SetPayloadSize (itpHdr.GetPayloadSize ());
		mHdr.SetCounter(itpHdr.GetCounter());
		Ptr<Packet> p = Create<Packet>(512);
		p->AddHeader(mHdr);
		p->AddHeader(GetNewUdpHdr(udpHdr));
		p->AddHeader(GetNewIpHdr(ipHdr, p->GetSize()));
    MyTag tagg;
//      tagg.SetFlag ();
//      p->AddPacketTag (tagg);

    p->AddPacketTag(PreserveAddress(udpHdr, ipHdr));
		return p;
	}
	if (!found) {
		//NS_LOG_INFO("Data is not cached");
	}

	interest->AddHeader(itpHdr);
	interest->AddHeader(udpHdr);
	interest->AddHeader(ipHdr);

	//NS_LOG_INFO("Packet Size: "<<interest->GetSize()<<" payload size in ip "<<ipHdr.GetPayloadSize());
	return interest;
}

Ptr<Packet> CctpCache::OnData(Ptr<Packet> data) {

	Ipv4Header ipHdr;
	data->RemoveHeader(ipHdr);
	UdpHeader udpHdr;
	data->RemoveHeader(udpHdr);
	ItpHeader itpHdr;
	data->RemoveHeader(itpHdr);

	uint32_t contentName = itpHdr.GetName();
	NS_LOG_INFO("Content name: "<<contentName<<" seq: "<<itpHdr.GetSeqNumber());
	bool found = false;
	for (int i = 0; i < m_cachedContents.size(); i++) {
		if (contentName == m_cachedContents[i].contentName) {
			found = true;
			if (m_cachedContents[i].seq.size() < itpHdr.GetSeqNumber()) {
				// NS_LOG_INFO("we have to resize it");
				int newSize = itpHdr.GetSeqNumber()
						- m_cachedContents[i].seq.size();
				m_cachedContents[i].seq.resize(newSize, 0);
				m_cachedContents[i].seq[itpHdr.GetSeqNumber()] = 1;
			} else {
				m_cachedContents[i].seq[itpHdr.GetSeqNumber()] = 1;
			}
			m_cachedContents[i].seq[itpHdr.GetSeqNumber()] = 1;
			NS_LOG_INFO(
					"Cached Chunk for content: "<<contentName<<" seq: "<<itpHdr.GetSeqNumber());
			break;
		}
	}

	if (!found) {
		//NS_LOG_INFO("Caching");
		struct CctpCache::CctpCacheEntry entry;
		entry.contentName = contentName;
		if (entry.seq.size() < itpHdr.GetSeqNumber()) {
			NS_LOG_INFO("we have to resize it");
			int newSize = itpHdr.GetSeqNumber() - entry.seq.size();
			entry.seq.resize(newSize, 0);
			entry.seq[itpHdr.GetSeqNumber()] = 1;
			NS_LOG_INFO(
					"Cached Chunk for content: "<<contentName<<" seq: "<<itpHdr.GetSeqNumber());
		} else {
			entry.seq[itpHdr.GetSeqNumber()] = 1;
			NS_LOG_INFO(
					"Cached Chunk for content: "<<contentName<<" seq: "<<itpHdr.GetSeqNumber());
		}
		m_cachedContents.push_back(entry);
	}

	data->AddHeader(itpHdr);
	data->AddHeader(udpHdr);
	data->AddHeader(ipHdr);
	//NS_LOG_INFO("Packet Size: "<<data->GetSize()<<" payload size in ip "<<ipHdr.GetPayloadSize());
	return data;
}

UdpHeader CctpCache::GetNewUdpHdr(UdpHeader oldUdpHdr) {
	UdpHeader newUdpHdr;
	NS_LOG_INFO("Source port "<<oldUdpHdr.GetSourcePort());
	NS_LOG_INFO("Dest port "<<oldUdpHdr.GetDestinationPort());
	newUdpHdr.SetDestinationPort(oldUdpHdr.GetSourcePort());
	newUdpHdr.SetSourcePort(oldUdpHdr.GetDestinationPort());

	return newUdpHdr;
}

Ipv4Header CctpCache::GetNewIpHdr(Ipv4Header oldIpHdr, int payloadSize) {

	Ipv4Header newIpHdr;
	NS_LOG_INFO("New source "<<oldIpHdr.GetSource());
	NS_LOG_INFO("New destination "<<m_proxyAddress);
	newIpHdr.SetDestination(Ipv4Address(m_proxyAddress.c_str()));
	newIpHdr.SetSource(oldIpHdr.GetSource());
	newIpHdr.SetPayloadSize(payloadSize);
	newIpHdr.SetTos(oldIpHdr.GetTos());
	newIpHdr.SetTtl(oldIpHdr.GetTtl());
	newIpHdr.SetProtocol(oldIpHdr.GetProtocol());
	return newIpHdr;
}

void CctpCache::OnData(uint32_t seq) {
	m_oneContent[seq] = 1;
}

void CctpCache::populateContentStore(std::vector<int> contentStore) {
	m_oneContent = contentStore;
}

void CctpCache::populateContentStore(std::string addr) {
	m_oneContent.resize(m_coSize, 0);
	m_proxyAddress= addr;
}

}

