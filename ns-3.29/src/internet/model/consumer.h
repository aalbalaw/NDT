#ifndef CONSUMER_H
#define CONSUMER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/udp-socket.h"
#include "ns3/address-utils.h"
#include "ns3/rtt-estimator1.h"
#include "ns3/traced-value.h"
#include "ns3/manifest-header.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

namespace ns3 {

class consumer : public Object
{

public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId
  GetTypeId (void);

  consumer ();

  ~consumer ();

  /*
   * Consumer received a Manifest for a new CO
   */
  void
  OnManifest (Ptr<Packet> pkt, Address producer);

  /*
   * Consumer received a Manifest Record from IDNS
   */
  void
  OnManifest (ManifestHeader ManifestRecord);

  /*
   * Send any pending Interests for a CO
   * TODO: Support multiple COs
   */
  void
  Send ();

  /*
   * Consumer received a data packet for one of its interests
   */
  void
  OnData (Ptr<Packet> pkt, Address producer);

  /*
   *Call ITP layer to send packet over UDP socket
   */
  void
  SetSendCallback (Callback<void, Ptr<Packet>, Address>);

  Callback<void, Ptr<Packet>, Address> m_sendData;

  /*
   * Notify ITP of the retrieval of a content object after satisfying all of its interests
   */
  void
  SetRecvCallback (Callback<void, Address>);

  Callback<void, Address> m_receiveData;

  /*
   * return the maximum number of Interests need to be transmitted to retrieve a CO
   * TODO: Must support multiple COs
   */
  uint32_t
  GetSeqMax () const;

  void
  SetWindow (uint32_t window);

  void increaseWindow(bool cache=false);

  /*
   * return the CWND for a CO
   * TODO: Must support multiple COs
   */
  //uint32_t GetWindow() const;
  void
  SetSeqMax (uint32_t maxSeq);

  /*
   * Takes care of when to schedule the transmission of next Interest including timeout Interests
   */
  void
  ScheduleNextPacket ();

  /*
   * Used to cancel all events at this consumer
   */
  void
  StopConsumer ();

  void
  CheckRetxTimeout ();

  void
  SetRetxTimer (Time retxTimer);

  Time
  GetRetxTimer () const;

  void
  OnTimeout (uint32_t seq, uint32_t count);

  void
  FastRetansmit (SequenceNumber32 seq);

  void
  SlowStart ();

  void
  CongestionAvoidance ();

  void
  GetWindow ();

  void SetNodeId( uint32_t id);

  /*
   * This function will be called whenever the consumer receives a complete data.
   * The consumer will immediately call on the application layer to deliver this data.
   */
  void ReceivedAtomicData(Ptr<Packet> pkt, Address from);

  struct RetxSeqsContainer : public std::set<uint32_t>

  {
  };

  RetxSeqsContainer m_retxSeqs;

  uint32_t m_contentName;
  /**
   * \struct This struct contains a pair of packet sequence number and its timeout
   */
  struct SeqTimeout
  {
    SeqTimeout (uint32_t _seq, uint32_t _count, Time _time) :
	seq (_seq), count (_count), time (_time)
    {
    }

    uint32_t seq;
    uint32_t count;
    Time time;
  };

  class i_seq
  {
  };
  class i_timestamp
  {
  };

  struct SeqTimeoutsContainer : public boost::multi_index::multi_index_container<
      SeqTimeout,
      boost::multi_index::indexed_by<
	  boost::multi_index::ordered_unique<boost::multi_index::tag<i_seq>,
	      boost::multi_index::member<SeqTimeout, uint32_t, &SeqTimeout::seq>>,
	  boost::multi_index::ordered_non_unique<
	      boost::multi_index::tag<i_timestamp>,
	      boost::multi_index::member<SeqTimeout, Time, &SeqTimeout::time>>>>
  {
  };

  SeqTimeoutsContainer m_seqTimeouts; ///< \brief multi-index for the set of SeqTimeout structs

  TracedCallback<uint32_t> m_cWndTrace;

private:
  uint32_t m_curSeqNo;
  uint32_t m_maxSeqNo;
  uint32_t m_lastSeqNoReceived;
  Address m_producer;
  uint32_t m_initialWindow;
  double m_window;
  Time m_cacheRTT;
  double adder;
  bool m_done;
  uint32_t m_threshold;
  uint32_t m_inFlight;
  EventId m_sendEvent;
  EventId m_retxEvent;
  uint32_t m_nodeId;
  uint32_t m_fastRetransmit;
  Ptr<RttEstimator1> m_rtt;
  uint32_t m_sum;
  Time m_retxTimer;
  Time m_fastRetRto;
  bool m_fastRTO;
  Time m_lastPacket;
  uint32_t m_chunkSize;
  uint32_t m_lostPackets;
  std::vector<uint32_t> all;
  double m_fistInterestSent;
  double m_lastDataReceived;

};

}

#endif /* IPRODUCER_H */
