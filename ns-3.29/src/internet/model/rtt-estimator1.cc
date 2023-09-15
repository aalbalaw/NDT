#include	<iostream>

#include	"rtt-estimator1.h"
#include	"ns3/simulator.h"
#include	"ns3/double.h"
#include	"ns3/integer.h"
#include	"ns3/uinteger.h"
#include	"ns3/log.h"

NS_LOG_COMPONENT_DEFINE("RttEstimator1");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(RttEstimator1);

static const double TOLERANCE = 1e-6;

TypeId
RttEstimator1::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::RttEstimator1").SetParent<Object> ().AddAttribute (
	  "MaxMultiplier", "Maximum RTO Multiplier", UintegerValue (64),
	  MakeUintegerAccessor (&RttEstimator1::m_maxMultiplier),
	  MakeUintegerChecker<uint16_t> ()).AddAttribute (
	  "InitialEstimation", "Initial RTT estimation",
	  TimeValue (Seconds (1.0)),
	  MakeTimeAccessor (&RttEstimator1::m_initialEstimatedRtt),
	  MakeTimeChecker ()).AddAttribute (
	  "MinRTO",
	  "Minimum retransmit timeout value",
	  TimeValue (Seconds (1.0)), // RFC2988 says min RTO=1 sec, but Linux uses 200ms. See http://www.postel.org/pipermail/end2end-interest/2004-November/004402.html
	  MakeTimeAccessor (&RttEstimator1::SetMinRto,
			    &RttEstimator1::GetMinRto),
	  MakeTimeChecker ()).AddAttribute( "FastRetxTimer",
						  "Timeout defining how frequent FasterRetransmission timeouts should be checked",
						  TimeValue (MilliSeconds (5)),
						  MakeTimeAccessor (&RttEstimator1::GetFastRetxTimer, &RttEstimator1::SetFastRetxTimer),
						  MakeTimeChecker ());
  return tid;
}

void
RttEstimator1::SetMinRto (Time minRto)
{
 // NS_LOG_FUNCTION(this << minRto);
  m_minRto = minRto;
  //m_lastEstimatedRtt=0;
}

Time
RttEstimator1::GetMinRto (void) const
{
  return m_minRto;
}

Time
RttEstimator1::GetEstimate  (void) const
{
  return m_estimatedRtt;
}

Time
RttEstimator1::GetVariation (void) const
{
  return m_estimatedVariation;
}

//RttHistory methods
RttHistory1::RttHistory1 (SequenceNumber32 s, uint32_t c, Time t) :
    seq (s), count (c), time (t), retx (false)
{
  // NS_LOG_FUNCTION (this);
}

RttHistory1::RttHistory1 (const RttHistory1& h) :
    seq (h.seq), count (h.count), time (h.time), retx (h.retx)
{
}

// Base class methods

RttEstimator1::RttEstimator1 () :
    m_next (1), m_history (), m_history2 (), m_nSamples (0), m_multiplier (1)
{
  //note next=1 everywhere since first segment will have sequence 1

  // We need attributes initialized here, not later, so use the
  // ConstructSelf() technique documented in the manual
  ObjectBase::ConstructSelf (
  AttributeConstructionList ());m_estimatedRtt = m_initialEstimatedRtt;
  //NS_LOG_DEBUG ("Initialize m_estimatedRtt to " << m_estimatedRtt.GetSeconds () << " sec.");
}

RttEstimator1::RttEstimator1 (const RttEstimator1& c) :
    Object (c), m_next (c.m_next), m_history (c.m_history), m_history2 (
	c.m_history2), m_maxMultiplier (c.m_maxMultiplier), m_initialEstimatedRtt (
	c.m_initialEstimatedRtt), m_estimatedRtt (
	c.m_estimatedRtt), m_minRto (c.m_minRto), m_nSamples (
	c.m_nSamples), m_multiplier (c.m_multiplier)
{
 // NS_LOG_FUNCTION(this);
}

RttEstimator1::~RttEstimator1 ()
{
}

TypeId
RttEstimator1::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
RttEstimator1::showdq ()
{
  RttHistory_t2::iterator it;
  for (it = m_history2.begin (); it != m_history2.end (); ++it)
    {
      std::cout<<"m_history2: Seq: "<<it->seq <<" counter: "<<it->count<<std::endl;
    }
}

void
RttEstimator1::DeleteSeq (SequenceNumber32 seq, uint32_t count)
{
  RttHistory_t2::iterator it;
  for (it = m_history2.begin (); it != m_history2.end ();)
    {
      if (it->seq == seq && it->count == count)
	{
	 // NS_LOG_INFO("Deleting seq: "<<it->seq << " with count: "<<count);
	  m_history2.erase (it);
	  return;
	  break;
	}
      else
	{
	  ++it;
	}
    }
}

//Once we receive a seq we delete it from all the lists
void
RttEstimator1::DeleteSeq (SequenceNumber32 seq )
{
  RttHistory_t2::iterator it;
  for (it = m_history2.begin (); it != m_history2.end ();)
    {
      if (it->seq == seq )
	{
	 // NS_LOG_INFO("Deleting seq: "<<it->seq );
	  it = m_history2.erase (it);
	}
      else
	{
	  ++it;
	}
    }

  for (it = m_outOfOrder.begin (); it != m_outOfOrder.end ();)
      {
        if (it->seq == seq )
  	{
  	  //NS_LOG_INFO("Deleting seq: "<<it->seq );
  	  it = m_outOfOrder.erase (it);
  	}
        else
  	{
  	  ++it;
  	}
      }
}

//Once we receive a seq we delete it from all the lists
Time
RttEstimator1::MeasureRTTSeqCache(SequenceNumber32 seq, uint32_t counter)
{
  RttHistory_t2::iterator it;
  Time m;
//  showdq();
  for (it = m_history2.begin (); it != m_history2.end ();)
    {
	  if (it->seq == seq && it->count == counter)
	  	{
	  	m = Simulator::Now () - it->time; // Elapsed time
	  	return m;
	  	break;
	  }
	  else
	  {
	  ++it;
	  }
	}
}

Time
RttEstimator1::MeasureRTTSeq(SequenceNumber32 seq, uint32_t counter)
{
  RttHistory_t2::iterator it;
  Time m;
  for (it = m_history2.begin (); it != m_history2.end ();)
    {
    if (it->seq == seq && it->count == counter)
      {
      m = Simulator::Now () - it->time; // Elapsed time
      return m;
      break;
    }
      else
      {
      ++it;
      }
    }
}
/*
 * For every seq number sent, it will be added to this list
 * The list preserve the order of data packets based on their position in the list
 * An out of order occur when the following case happens:
 * List: 1--2--3--4
 * 2 arrive before 1.
 */
void
RttEstimator1::SentSeq (SequenceNumber32 seq)
{
  uint32_t counter = GetSequenceCounter (seq.GetValue ());
 // NS_LOG_INFO("Seq: "<<seq << " count: "<<counter);
  m_history2.push_back (RttHistory1 (seq, counter, Simulator::Now ()));
  //m_history2.back ().retx = true;
}

/*
 * For every data packet arrives, the consumer compare the seq & counter
 * When we detect a packet is out of order, we check is the time is elapsed to retransmit it.
 * TODO: Create a fastRetransmit timer and we should check it from time to time.
 * TODO: Can an out of order delivery cause many retransmits.
 * Ex: 1--2--3--4--5
 * 3 arrives first, this should cause fastRetransmit to 1 & 2
 * these will be added to a list and we checked from time to time on when we need to retransmit them
 * because now we only check them when we receive an out order delivery.
 * TODO: We should check the outOfOrder list just in case.Once we found it we will return true and avoid this step
 * Case 1:
 * 	1. When a packet arrives we check the outOfOrder list just in case it's already there.
 * 	2. If it's there we don't need to loop over the history list since we DIDN'T sent yet.
 * Case 2:
 * 	1. The order of seq in the history list is the order of when they were sent.
 * 	2. An out of order delivery will be triggered if the received data packet is NOT on the top of the history
 * 	list AND exist in the list.
 * Case 2:
 * 	1. When a data packet arrives and it's not in the list ( got deleted due to timeout or out of order delivery)
 * 	this packet shouldn't cause out of order delivery bcs it will not be found in the list.
 * 	2. to avoid looping twice to check if we have it, we will set a flag that push all out of order seq
 * 	only when the packet is found in our list.
 * Case 3:
 * 	1. An out of order packet will be added to the outOfOrder list and from time to time it will be checked
 * 	to see if it meets the right condition to resends it again.
 * 	2. Once it meets the right condition the consumer will be invoked to send it and then the packet will be
 * 	removed from this list.
 * Case 4:
 * 	1. When a packet correctly arrives (counter matches) it's important that we delete any packet with the
 * 	same seq number since we don't need it anymore. (Double check this one)
 */
Time
RttEstimator1::AckSeq (SequenceNumber32 ackSeq, uint32_t counter)
{

  if ( AckOutOfOrder(ackSeq, counter) ) {
      return Seconds (0.0);
  }
  Time m = Seconds (0.0);
  RttHistory_t2::iterator it;
  RttHistory_t2 tmp;
  it = m_history2.begin ();
  bool found=false;
  for (it = m_history2.begin (); it != m_history2.end ();)
    {
      if (it->seq == ackSeq && it->count == counter)
	{
//	NS_LOG_INFO(
//	    "Ok to use this sample: "<<ackSeq <<" with counter: "<<counter);
	m = Simulator::Now () - it->time; // Elapsed time
	Measurement (m);                // Log the measurement
	ResetMultiplier ();       // Reset multiplier on valid measurement
	m_lastEstimatedRtt = m;
	DeleteSeq(ackSeq);
	found = true;
	break;}
      else
	{
	NS_LOG_INFO(
	    "Arrived: Seq: "<<ackSeq <<" Counter: "<<counter << " OutOfOrder: Seq: "<<it->seq << " Counter: "<< it->count);
	tmp.push_back(RttHistory1 (it->seq, it->count, it->time));
	it = m_history2.erase (it);
	}
    }

  if(found) {
      m_outOfOrder.insert(m_outOfOrder.end(), tmp.begin(), tmp.end());
      CheckFastRetxTimeout();
  }
 // showdq();
  return m;
}

/*
 * Set a time to check to timeout Interests
 */
void
RttEstimator1::SetFastRetxTimer (Time fastRetxTimer)
{
  m_fastRetxTimer = fastRetxTimer;
  if (m_fastRetxEvent.IsRunning ())
    {
      Simulator::Remove (m_fastRetxEvent);
    }

  m_fastRetxEvent = Simulator::Schedule (m_fastRetxTimer, &RttEstimator1::CheckFastRetxTimeout,
				     this);

}

Time
RttEstimator1::GetFastRetxTimer () const
{
  return m_fastRetxTimer;
}

void
RttEstimator1::CheckFastRetxTimeout ()
{
  Time now = Simulator::Now ();
  Time rto = m_lastEstimatedRtt;

  RttHistory_t2::iterator it;
    it = m_outOfOrder.begin ();
    for (it = m_outOfOrder.begin (); it != m_outOfOrder.end ();)
      {
	Time elapsed_Time = Simulator::Now () - it->time;
	NS_LOG_INFO("Out of Order Seq: "<<it->seq<<" elapsed_Time: "<<elapsed_Time.GetSeconds()<<" Rto: "<<rto.GetSeconds());
        if (elapsed_Time > rto ) {
            m_lostSeq (it->seq);
            it = m_outOfOrder.erase (it);
  	}else {
  	    it++;
  	}
     }
  m_fastRetxEvent = Simulator::Schedule (m_fastRetxTimer, &RttEstimator1::CheckFastRetxTimeout,				     this);
}

void
RttEstimator1::ClearSent ()
{
  // Clear all history entries
  m_next = 1;
  m_history.clear ();
}

void
RttEstimator1::IncreaseMultiplier ()
{
  m_multiplier =
      (m_multiplier * 2 < m_maxMultiplier) ? m_multiplier * 2 : m_maxMultiplier;
 // NS_LOG_DEBUG("Multiplier increased to " << m_multiplier);
}

void
RttEstimator1::ResetMultiplier ()
{
  m_multiplier = 1;
}

void
RttEstimator1::SetFastRetransmitCallback (
    Callback<void, SequenceNumber32> lostSeq)
{
  m_lostSeq = lostSeq;
}

void
RttEstimator1::Reset ()
{
  // Reset to initial state
  m_next = 1;
  m_estimatedRtt = m_initialEstimatedRtt;
  m_history.clear ();         // Remove all info from the history
  m_nSamples = 0;
  ResetMultiplier ();
}

/*
 * For every manifest a consumer receives for a CO it will set a counter as vector
 * This counter will keep track of how many times we sent a seq number
 * It will be used to distinguish between retransmitted packets
 */
void
RttEstimator1::SetSequenceCounterSize (uint64_t maxSeq)
{
  m_seqCounter.resize (maxSeq, 0);
}

void
RttEstimator1::SetSequenceCounter (uint64_t seq)
{
  m_seqCounter[(seq - 1)]++;
}

uint32_t
RttEstimator1::GetSequenceCounter (uint64_t seq)
{
  return m_seqCounter[(seq - 1)];
}

/*
 * We should ack any interest with the same seq number
 * and mearure the rtt for the correct counter number.
 */
bool
RttEstimator1::AckOutOfOrder (SequenceNumber32 ackSeq, uint32_t counter)
{
  RttHistory_t2::iterator it;
  Time m = Seconds (0.0);
  for (it = m_outOfOrder.begin (); it != m_outOfOrder.end ();)
    {
      if (it->seq == ackSeq && it->count == counter)
	{
	 // NS_LOG_INFO("Ack ourOfOrder seq: "<<it->seq << " with count: "<<counter);
	  m = Simulator::Now () - it->time; // Elapsed time
	  Measurement (m);                // Log the measurement
	  ResetMultiplier ();       // Reset multiplier on valid measurement
	  m_lastEstimatedRtt = m;
	  m_outOfOrder.erase (it);
	  DeleteSeq(ackSeq);
	  return true;
	}
      else
	{
	  ++it;
	}
    }

  return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Mean-Deviation Estimator

NS_OBJECT_ENSURE_REGISTERED(RttMeanDeviation1);

TypeId
RttMeanDeviation1::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::RttMeanDeviation1").SetParent<RttEstimator1> ().AddConstructor<
	  RttMeanDeviation1> ();
  return tid;
}

RttMeanDeviation1::RttMeanDeviation1 () :
    m_alpha(0.125),
    m_beta(0.25)
{
}

RttMeanDeviation1::RttMeanDeviation1 (const RttMeanDeviation1& c) :
    RttEstimator1 (c), m_alpha (c.m_alpha), m_beta(c.m_beta)
{
}

TypeId
RttMeanDeviation1::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
RttMeanDeviation1::Measurement (Time m)
{

 // NS_LOG_FUNCTION (this << m);
     if (m_nSamples)
       {
         // If both alpha and beta are reciprocal powers of two, updating can
         // be done with integer arithmetic according to Jacobson/Karels paper.
         // If not, since class Time only supports integer multiplication,
         // must convert Time to floating point and back again
         uint32_t rttShift = CheckForReciprocalPowerOfTwo (m_alpha);
         uint32_t variationShift = CheckForReciprocalPowerOfTwo (m_beta);
         if (rttShift && variationShift)
           {
             IntegerUpdate (m, rttShift, variationShift);
           }
         else
           {
             FloatingPointUpdate (m);
           }
       }
     else
       { // First sample
         m_estimatedRtt = m;               // Set estimate to current
         m_estimatedVariation = m / 2;  // And variation to current / 2
        // NS_LOG_DEBUG ("(first sample) m_estimatedVariation += " << m);
       }
     m_nSamples++;
}

Time
RttMeanDeviation1::RetransmitTimeout ()
{
  // RFC 6298, clause 2.4
  Time rto;
  rto = Max (GetEstimate () + Max (m_clockGranularity, GetVariation () * 4), m_minRto);

  return rto;
////NS_LOG_DEBUG ("RetransmitTimeout:  var " << m_estimatedVariation.GetSeconds () << " est " << m_estimatedRtt.GetSeconds () << " multiplier " << m_multiplier);
//// RTO = srtt + 4* rttvar
//  int64_t temp = m_estimatedRtt.ToInteger (Time::MS)
//      + 4 * m_estimatedVariation.ToInteger (Time::MS);
//  if (temp < m_minRto.ToInteger (Time::MS))
//    {
//      temp = m_minRto.ToInteger (Time::MS);
//    }
//  temp = temp * m_multiplier; // Apply backoff
//  Time retval = Time::FromInteger (temp, Time::MS);
////NS_LOG_DEBUG ("RetransmitTimeout:  return " << retval.GetSeconds ());
//  return (retval);
}

Ptr<RttEstimator1>
RttMeanDeviation1::Copy () const
{
  return CopyObject < RttMeanDeviation1 > (this);
}

void
RttMeanDeviation1::Reset ()
{
// Reset to initial state
  m_estimatedVariation = Seconds (0);
  RttEstimator1::Reset ();
}

uint32_t
 RttMeanDeviation1::CheckForReciprocalPowerOfTwo (double val) const
 {
  //NS_LOG_FUNCTION (this << val);
     if (val < TOLERANCE)
       {
         return 0;
       }
     // supports 1/32, 1/16, 1/8, 1/4, 1/2
     if (std::abs (1/val - 8) < TOLERANCE)
       {
         return 3;
       }
     if (std::abs (1/val - 4) < TOLERANCE)
       {
         return 2;
       }
     if (std::abs (1/val - 32) < TOLERANCE)
       {
         return 5;
       }
     if (std::abs (1/val - 16) < TOLERANCE)
       {
         return 4;
       }
     if (std::abs (1/val - 2) < TOLERANCE)
       {
         return 1;
       }
     return 0;
 }

void
 RttMeanDeviation1::FloatingPointUpdate (Time m)
 {
   //NS_LOG_FUNCTION (this << m);

   // EWMA formulas are implemented as suggested in
   // Jacobson/Karels paper appendix A.2

   // SRTT <- (1 - alpha) * SRTT + alpha *  R'
   Time err (m - m_estimatedRtt);
   double gErr = err.ToDouble (Time::S) * m_alpha;
   m_estimatedRtt += Time::FromDouble (gErr, Time::S);

   // RTTVAR <- (1 - beta) * RTTVAR + beta * |SRTT - R'|
   Time difference = Abs (err) - m_estimatedVariation;
   m_estimatedVariation += Time::FromDouble (difference.ToDouble (Time::S) * m_beta, Time::S);

   return;
 }

 void
 RttMeanDeviation1::IntegerUpdate (Time m, uint32_t rttShift, uint32_t variationShift)
 {
  // NS_LOG_FUNCTION (this << m << rttShift << variationShift);
   // Jacobson/Karels paper appendix A.2
   int64_t meas = m.GetInteger ();
   int64_t delta = meas - m_estimatedRtt.GetInteger ();
   int64_t srtt = (m_estimatedRtt.GetInteger () << rttShift) + delta;
   m_estimatedRtt = Time::From (srtt >> rttShift);
   if (delta < 0)
     {
       delta = -delta;
     }
   delta -= m_estimatedVariation.GetInteger ();
   int64_t rttvar = m_estimatedVariation.GetInteger () << variationShift;
   rttvar += delta;
   m_estimatedVariation = Time::From (rttvar >> variationShift);
   return;
 }

} //namespace ns3
