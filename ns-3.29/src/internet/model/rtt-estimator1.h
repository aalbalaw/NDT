/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: Rajib Bhattacharjea<raj.b@gatech.edu>
//

// Georgia Tech Network Simulator - Round Trip Time Estimation Class
// George F. Riley.  Georgia Tech, Spring 2002


#ifndef RTT_ESTIMATOR1_H
#define RTT_ESTIMATOR1_H


#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/event-id.h"
#include <deque>
#include "ns3/sequence-number.h"

namespace ns3 {



class RttHistory1 {
	public:
	  RttHistory1 (SequenceNumber32 s, uint32_t c, Time t);
	  RttHistory1 (const RttHistory1& h); // Copy constructor
	public:
	  SequenceNumber32  seq;  // First sequence number in packet sent
	  uint32_t        count;  // Number of bytes sent
	  Time            time;   // Time this one was sent
	  bool            retx;   // True if this has been retransmitted
	};

	typedef std::deque<RttHistory1> RttHistory_t1;

	typedef std::list<RttHistory1> RttHistory_t2;



/**
 * \ingroup tcp
 *
 * \brief Base class for all RTT Estimators
 *
 * The RTT Estimator class computes an estimate of the round trip time
 * observed in a series of Time measurements.  The estimate is provided in
 * the form of an estimate and a sample variation.  Subclasses can implement
 * different algorithms to provide values for the estimate and variation.
 */
class RttEstimator1 : public Object {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  RttEstimator1();
  /**
   * \brief Copy constructor
   * \param r the object to copy
   */
  RttEstimator1 (const RttEstimator1& r);

  virtual ~RttEstimator1();

  virtual TypeId GetInstanceTypeId (void) const;

  virtual void SentSeq (SequenceNumber32 seq);

  virtual Time AckSeq (SequenceNumber32 ackSeq, uint32_t counter);

  virtual void ClearSent ();

  virtual void  Measurement (Time t) = 0;

  virtual Time RetransmitTimeout () = 0;

  virtual Ptr<RttEstimator1> Copy () const = 0;

  virtual void IncreaseMultiplier ();

  virtual void ResetMultiplier ();

  virtual void Reset ();

  void SetMinRto (Time minRto);

  void showdq();

  void DeleteSeq(SequenceNumber32 seq, uint32_t count);

  void DeleteSeq( SequenceNumber32 seq );

  Time GetMinRto (void) const;

  Time GetEstimate (void) const;

  Time GetVariation (void) const;

  Callback<void, SequenceNumber32> m_lostSeq;

  void SetFastRetransmitCallback (Callback<void, SequenceNumber32 >);

  void SetSequenceCounterSize ( uint64_t maxSeq);

  void SetSequenceCounter ( uint64_t seq);

  uint32_t GetSequenceCounter ( uint64_t seq);

  bool AckOutOfOrder( SequenceNumber32 ackSeq, uint32_t counter);
  Time MeasureRTTSeq ( SequenceNumber32 ackSeq, uint32_t counter);
  Time  MeasureRTTSeqCache(SequenceNumber32 ackSeq, uint32_t counter);

  void
  CheckFastRetxTimeout ();

  void
  SetFastRetxTimer (Time fastRetxTimer);

  Time
  GetFastRetxTimer () const;


  Time         m_estimatedRtt;     // Current estimate
  Time 	       m_estimatedVariation;
  Time	       m_lastEstimatedRtt;
  RttHistory_t2 m_outOfOrder;     // List of out of order interests
  EventId m_fastRetxEvent;

   private:
       SequenceNumber32 m_next;    // Next expected sequence to be sent
       RttHistory_t1 m_history;     // List of sent packet
       RttHistory_t2 m_history2;     // List of sent packet

       uint16_t m_maxMultiplier;
       Time m_initialEstimatedRtt;
       std::vector<uint64_t> m_seqCounter;
       Time m_fastRetxTimer;

     protected:
       Time         m_minRto;                  // minimum value of the timeout
       uint32_t     m_nSamples;                // Number of samples
       uint16_t     m_multiplier;              // RTO Multiplier
     };

class RttMeanDeviation1 : public RttEstimator1 {
  public:
    static TypeId GetTypeId (void);

    RttMeanDeviation1 ();

    RttMeanDeviation1 (const RttMeanDeviation1&);

	virtual TypeId GetInstanceTypeId (void) const;

	void Measurement (Time measure);

	Time RetransmitTimeout ();

	Ptr<RttEstimator1> Copy () const;

	void Reset ();

	void FloatingPointUpdate (Time m);

	void IntegerUpdate (Time m, uint32_t rttShift, uint32_t variationShift);

	uint32_t CheckForReciprocalPowerOfTwo (double val) const;

  private:
	double       m_alpha;       // Filter gain
	double       m_beta;       // Filter gain
	Time m_clockGranularity {Seconds (0.001)};
};
} // namespace ns3

#endif /* RTT_ESTIMATOR_H */
