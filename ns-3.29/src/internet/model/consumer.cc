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
#include "ns3/consumer.h"
#include "ns3/itp-header.h"
#include <string>
#include <limits>
#include "ns3/hash.h"
#include <math.h>
#include "ns3/trace-source-accessor.h"
#include "ns3/mytag.h"
#include "ns3/global-value.h"
#include "ns3/double.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("consumer");

NS_OBJECT_ENSURE_REGISTERED(consumer);

static GlobalValue g_totalTime  = GlobalValue ("TotalTimeForConsumers",
                                                      "A global switch to enable all checksums for all protocols",
                                                      DoubleValue (0.0),
                                                      MakeDoubleChecker<double>());

static GlobalValue g_totalConsumers  = GlobalValue ("TotalConsumers",
                                                      "A global switch to enable all checksums for all protocols",
                                                      DoubleValue (0.0),
                                                      MakeDoubleChecker<double>());

TypeId consumer::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::consumer").SetParent<Object>().SetGroupName("Internet").AddConstructor<
					consumer>().AddAttribute("RetxTimer",
					"Timeout defining how frequent retransmission timeouts should be checked",
					TimeValue(MilliSeconds(50)),
					MakeTimeAccessor(&consumer::GetRetxTimer,
							&consumer::SetRetxTimer), MakeTimeChecker()).AddTraceSource(
					"CongestionWindow",
					"The ITP connection's congestion window",
					MakeTraceSourceAccessor(&consumer::m_cWndTrace),
					"ns3::TracedValueCallback::Uint32");
	return tid;
}

consumer::consumer() {
	m_window = 1;
	m_inFlight = 0;
	m_rtt = CreateObject<RttMeanDeviation1>();
	m_threshold = UINT32_MAX;
	m_threshold = 350;
	m_done = false;
	m_contentName = 0;
	m_lostPackets = 0;
	all = std::vector<uint32_t>(10000, 0);
	m_fastRTO = false;

}

consumer::~consumer() {
	NS_LOG_FUNCTION(this);
	Time difference_manifest = Seconds(0.03);//time from producer to receiver
	m_lastDataReceived = Simulator::Now().GetSeconds()+ 0.03;
	std::cout
			<< (m_sum * 1500 * 8) / (m_lastDataReceived - m_fistInterestSent)
					/ 1024 / 1024 << std::endl;
	std::cout << m_lostPackets << std::endl;
	std::cout << ((m_sum * 1500) / 1000000.0) << std::endl;

	NS_LOG_INFO("Total Time: "<<m_lastDataReceived - m_fistInterestSent);
	NS_LOG_INFO("Total Received Bytes: "<<(m_sum*1500)/1000000.0);
	NS_LOG_INFO("Total lost packets: "<<m_lostPackets);
	NS_LOG_INFO(
			"Average Throughput: "<<(m_sum*1500*8)/( m_lastDataReceived-m_fistInterestSent)/ 1024 / 1024);
	NS_LOG_INFO("");
}

void consumer::OnManifest(ManifestHeader manifestRecord) {

	m_curSeqNo = 1;
	m_done = false;
//	all = std::vector<uint32_t>(m_maxSeqNo, 0);
//	std::fill(all.begin(), all.end(), 0);
	m_maxSeqNo = manifestRecord.GetNumberOfChunks();
	NS_LOG_INFO("Max Seq Number: "<< m_maxSeqNo);
	m_contentName = manifestRecord.GetName();
	m_lastSeqNoReceived = 0;
	m_producer = InetSocketAddress(manifestRecord.GetServerAddress(), 81);
	m_rtt->SetFastRetransmitCallback(
			MakeCallback(&consumer::FastRetansmit, this));
	m_rtt->SetSequenceCounterSize(m_maxSeqNo);
	ScheduleNextPacket();
}

void consumer::OnManifest(Ptr<Packet> pkt, Address producer) {
	ItpHeader rcvHdr;
	pkt->RemoveHeader(rcvHdr);
	uint8_t *buffer = new uint8_t[pkt->GetSize()];
	pkt->CopyData(buffer, pkt->GetSize());
	m_curSeqNo = 1;
	m_sum = 0;
	m_maxSeqNo = std::stoi((char *) buffer);
	all = std::vector<uint32_t>(m_maxSeqNo, 0);
	std::fill(all.begin(), all.end(), 0);
	//m_maxSeqNo = UINT32_MAX;
	//NS_LOG_INFO("Max Seq Number: "<< m_maxSeqNo);
	m_contentName = rcvHdr.GetName();
	m_lastSeqNoReceived = 0;
	m_producer = producer;
	m_rtt->SetFastRetransmitCallback(
			MakeCallback(&consumer::FastRetansmit, this));
	m_rtt->SetSequenceCounterSize(m_maxSeqNo);
	ScheduleNextPacket();
}

void consumer::SetSendCallback(Callback<void, Ptr<Packet>, Address> sendData) {
	m_sendData = sendData;
}

void consumer::SetRecvCallback(Callback<void, Address> receivedData) {
	m_receiveData = receivedData;
}
/*
 * TODO: There is a bug where an itp application never stops even when done. This might be related to the events.
 * Therefore, we should cancel these events and submit it in a seperate commit.
 */
void consumer::Send(void) {
	//NS_LOG_INFO(this);
	Ptr<Packet> p;

	p = Create<Packet>();
	uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid
	while (m_retxSeqs.size()) {
		seq = *m_retxSeqs.begin();
		m_retxSeqs.erase(m_retxSeqs.begin());
		break;
	}

	if (seq == std::numeric_limits<uint32_t>::max()) {
		if (m_curSeqNo > m_maxSeqNo) {
			if (m_seqTimeouts.empty()) {
				if (!m_done) {
					m_lastDataReceived = Simulator::Now().GetSeconds();
//					NS_LOG_INFO(
//							"Total Time: "<<m_lastDataReceived - m_fistInterestSent);
					DoubleValue tmp;
					g_totalTime.GetValue(tmp);

					const double& y = (m_lastDataReceived - m_fistInterestSent) + tmp.Get();
					tmp.Set(y);
					g_totalTime.SetValue(tmp);


					DoubleValue tmp2;
					g_totalConsumers.GetValue(tmp2);

					const double& y2 = 1.0 + tmp2.Get();
					tmp2.Set(y2);
					g_totalConsumers.SetValue(tmp2);

					NS_LOG_INFO("Global Total Time: "<< tmp.Get());
					NS_LOG_INFO("Total Consumers: "<< tmp2.Get());

					//NS_LOG_INFO("retx Size: "<< m_retxSeqs.size());
					//NS_LOG_INFO(
						//	"Number of Unacked Packets: "<<m_rtt->m_outOfOrder.size());
					//NS_LOG_INFO("MAX seq: "<<m_maxSeqNo);
					//NS_LOG_INFO("Total Received Packets: "<<m_sum);
					//NS_LOG_INFO("Total lost packets: "<<m_lostPackets);
					//NS_LOG_INFO(
							//"Average Throughput: "<<(m_maxSeqNo*1500*8)/( m_lastDataReceived-m_fistInterestSent)/ 1024 / 1024);
					NS_LOG_INFO("");

					m_done = true;
					Simulator::Cancel(m_sendEvent);
					Simulator::Cancel(m_retxEvent);
					Simulator::Cancel(m_rtt->m_fastRetxEvent);
					m_receiveData(m_producer);
				}
			}
			return;
		}
		seq = m_curSeqNo;
		m_curSeqNo++;
	}
	if (seq == 1) {
		m_fistInterestSent = Simulator::Now().GetSeconds();
	}
	ItpHeader mHdr;
	mHdr.SetType(2);
	mHdr.SetSeqNumber(seq);
	mHdr.SetName(m_contentName);
	mHdr.SetCounter(m_rtt->GetSequenceCounter(seq));
	p->AddHeader(mHdr);
	m_inFlight++;
	m_seqTimeouts.insert(
			SeqTimeout(seq, m_rtt->GetSequenceCounter(seq), Simulator::Now()));
	m_rtt->SentSeq(SequenceNumber32(seq));
	m_rtt->SetSequenceCounter(seq);
	//NS_LOG_INFO("Sending Interest "<<seq);
	m_sendData(p, m_producer);
	//NS_LOG_INFO("here");
	ScheduleNextPacket();
}

/*
 * Packets received from a cache shouldn't cause out order and their rtt shouldn't measured
 */
void consumer::OnData(Ptr<Packet> pkt, Address from) {

	//NS_LOG_INFO(this);

	ItpHeader rcvHdr;
	pkt->RemoveHeader(rcvHdr);

	//NS_LOG_INFO("We received Data "<<rcvHdr.GetSeqNumber());

	if (all[rcvHdr.GetSeqNumber()] == 1) {
		NS_LOG_INFO("We have a duplicate for seq: "<<rcvHdr.GetSeqNumber());
	} else {
		all[rcvHdr.GetSeqNumber()] = 1;
	}

	if (m_inFlight > static_cast<uint32_t>(0))
		m_inFlight--;

	m_seqTimeouts.erase(rcvHdr.GetSeqNumber());
	m_retxSeqs.erase(rcvHdr.GetSeqNumber());

	if (rcvHdr.GetType() == 4) {
		m_cacheRTT = m_rtt->MeasureRTTSeqCache(
				SequenceNumber32(rcvHdr.GetSeqNumber()), rcvHdr.GetCounter());
		m_rtt->DeleteSeq(SequenceNumber32(rcvHdr.GetSeqNumber()));
		increaseWindow(true);
	} else {
		m_rtt->AckSeq(SequenceNumber32(rcvHdr.GetSeqNumber()),
				rcvHdr.GetCounter());
		increaseWindow(false);
	}

	if (m_fastRetransmit > static_cast<uint32_t>(0)) {
		m_fastRetransmit--;
	}

	m_sum += 1;
	ScheduleNextPacket();
}

void consumer::SetWindow(uint32_t window) {
	m_initialWindow = window;
	m_window = m_initialWindow;
}

uint32_t consumer::GetSeqMax() const {
	return m_maxSeqNo;
}

void consumer::SetSeqMax(uint32_t seqMax) {
	if (m_maxSeqNo < 0)
		m_maxSeqNo = seqMax;
}

void consumer::ScheduleNextPacket() {
	//NS_LOG_INFO(this);
	if (m_window == static_cast<uint32_t>(0)) {
		Simulator::Remove(m_sendEvent);
		m_sendEvent = Simulator::Schedule(
				Seconds(
						std::min<double>(0.5,
								m_rtt->RetransmitTimeout().ToDouble(Time::S))),
				&consumer::Send, this);
	} else if (m_inFlight >= static_cast<uint32_t>(m_window)) {
	} else {
		if (m_sendEvent.IsRunning()) {
			Simulator::Remove(m_sendEvent);
		}
		m_sendEvent = Simulator::ScheduleNow(&consumer::Send, this);
	}
}

/*
 * Set a time to check to timeout Interests
 */
void consumer::SetRetxTimer(Time retxTimer) {
	m_retxTimer = retxTimer;
	if (m_retxEvent.IsRunning()) {
		Simulator::Remove(m_retxEvent);
	}

	m_retxEvent = Simulator::Schedule(m_retxTimer, &consumer::CheckRetxTimeout,
			this);

}

Time consumer::GetRetxTimer() const {
	return m_retxTimer;
}

/*
 * We loop over m_seqTimeouts every m_retxTimer and check if any interest is timedout
 */
void consumer::CheckRetxTimeout() {

	Time now = Simulator::Now();
	Time rto = m_rtt->RetransmitTimeout();
	bool timeout = false;

	while (!m_seqTimeouts.empty()) {
		SeqTimeoutsContainer::index<i_timestamp>::type::iterator entry =
				m_seqTimeouts.get<i_timestamp>().begin();
		if (entry->time + (rto) <= now) // timeout expired?
				{
			timeout = true;
			uint32_t seqNo = entry->seq;
			uint32_t count = entry->count;
			m_seqTimeouts.get<i_timestamp>().erase(entry);
			OnTimeout(seqNo, count);
		} else
			break; // nothing else to do. All later packets need not be retransmitted
	}
//  if(timeout){
//      timeout=false;
//      //m_rtt->IncreaseMultiplier (); // Double the next RTO
//      NS_LOG_INFO("RTO After Doubling: "<<m_rtt->RetransmitTimeout ().GetSeconds());
//  }

	m_retxEvent = Simulator::Schedule(m_retxTimer, &consumer::CheckRetxTimeout,
			this);
}

/* RFC5681 Section3.1
 * On Timeout:
 * ssthresh = max (FlightSize / 2, 2*SMSS)            (4)
 * todo: On the other hand, when a TCP sender detects segment loss using the
 retransmission timer and the given segment has already been
 retransmitted by way of the retransmission timer at least once, the
 value of ssthresh is held constant.
 * Furthermore, upon a timeout (as specified in [RFC2988]) cwnd MUST be
 set to no more than the loss window, LW, which equals 1 full-sized
 segment (regardless of the value of IW).  Therefore, after
 retransmitting the dropped segment the TCP sender uses the slow start
 algorithm to increase the window from 1 full-sized segment to the new
 value of ssthresh, at which point congestion avoidance again takes
 over.
 */

/*
 * TODO: We have to delete based on seq & counter
 * TODO: We have to delete it from fast retransmit list too
 */
void consumer::OnTimeout(uint32_t seq, uint32_t count) {
	NS_LOG_INFO("\t\t\t\t timeout "<<seq<<" Node: "<<m_nodeId);
	m_lostPackets += 1;
	if (m_inFlight > static_cast<uint32_t>(0))
		m_inFlight--;
	if (m_fastRetransmit > static_cast<uint32_t>(0))
		m_fastRetransmit--;
	m_threshold = std::max<uint32_t>(2, m_inFlight / 2);
	m_window = 1;
	m_seqTimeouts.erase(seq);
	m_rtt->DeleteSeq(SequenceNumber32(seq));
	m_retxSeqs.insert(seq);
	Send();

}

/*
 * we should erase it and add it with new transmission time to the m_seqTimeouts
 *
 */
void consumer::FastRetansmit(SequenceNumber32 seq) {
	m_lostPackets += 1;
	if (m_inFlight > static_cast<uint32_t>(0))
		m_inFlight--;
	Time m = Seconds(0.0);
	if (m_fastRetransmit == 0) {
		m_fastRetransmit = m_inFlight;
		m_window = std::max<uint32_t>(1, m_inFlight / 2);
		m_threshold = m_window;
		Send();
		m_fastRTO = true;
	} else {
		if (m_fastRetransmit > static_cast<uint32_t>(0))
			m_fastRetransmit--;
		ScheduleNextPacket();
	}
	m_retxSeqs.insert(seq.GetValue());
	m_seqTimeouts.erase(seq.GetValue());

}

void consumer::SlowStart() {
	m_window += 1;
}

void consumer::CongestionAvoidance() {
	m_window += (1.0 / m_window);
}

void consumer::GetWindow() {
	double thoughput = (double) m_window * 1500 * 8;
	Time m = Seconds(0.0);
	if (m_cacheRTT != m) {
		thoughput = thoughput / m_cacheRTT.GetSeconds();
		thoughput = thoughput / 1024 / 1024;
		NS_LOG_INFO(
				"\t"<<m_window <<"\t"<<m_cacheRTT.GetSeconds()<<"\t"<<thoughput<<"\t");
	} else {
		thoughput = thoughput / m_rtt->m_lastEstimatedRtt.GetSeconds();
		thoughput = thoughput / 1024 / 1024;
		NS_LOG_INFO(
				"\t"<<m_window <<"\t"<<m_cacheRTT.GetSeconds()<<"\t"<<thoughput<<"\t");
	}

	if (!m_done) {
		Simulator::Schedule(Seconds((0.05)), &consumer::GetWindow, this);
	}
}
void consumer::StopConsumer() {
	Simulator::Cancel(m_sendEvent);
	Simulator::Cancel(m_retxEvent);
}

void consumer::SetNodeId(uint32_t id) {

	m_nodeId = id;
}

void consumer::increaseWindow(bool cache) {

	if (cache) {
		if (m_window < m_threshold) {
			m_window += 0.5;
		} else {
			m_window += (0.5 / m_window);
		}
	} else {
		if (m_window < m_threshold) {
			SlowStart();
		} else {
			CongestionAvoidance();
		}
	}
}
}
;
