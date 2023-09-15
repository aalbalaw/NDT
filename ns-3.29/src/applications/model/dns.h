/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DNS_H
#define DNS_H

#include <cassert>                                                                                        
#include <list>                                                                                           
#include <sys/types.h>

#include "ns3/ipv4-address.h"
#include "ns3/timer.h"
#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ipv4.h"
#include "ns3/net-device.h"
#include "ns3/output-stream-wrapper.h"

namespace ns3 {	
class SRVRecordEntry
{
	public:
		SRVRecordEntry (void);
		/*
		 * /brief constructor 
		 * /param rName the name of the DNS record
		 * /param rTTL the TTL value of the record
		 * /param rClass the class of the record
		 * /param rType the type of the record
		 * /rData the IP address of the record //FIXME the support all type of records*/
		SRVRecordEntry ( std::string rName = std::string (""), 
								uint32_t rTTL = 0, 
								uint16_t rClass = 0, 
								uint16_t rType = 0, 
								std::string rData = std::string (""));

		~SRVRecordEntry ();

		/*
		 * /brief Get and set the record name*/
		void SetRecordName (std::string rName)
		{
				m_recordName =  rName;
		}
		std::string GetRecordName (void) const
		{
			return m_recordName;
		}

		/*
		 * /brief Get and Set the TTL value*/
		void SetTTL (uint32_t rTTL)
		{
			m_recordTimeToLive = rTTL;
		}
		uint32_t GetTTL (void) const
		{
			return m_recordTimeToLive;
		}

		/*
		 * /brief Get and set the class*/
		void SetClass (uint16_t rClass)
		{
			m_recordClass = rClass;
		}
		uint16_t GetClass (void) const
		{
			return m_recordClass;
		}

		/*
		 * /brief Get and Set the type*/
		void SetType (uint16_t rType)
		{
			m_recordType = rType;
		}
		uint16_t GetType (void) const
		{
			return m_recordType;
		}
			
		/*
		 * /brief Get and set the record data (This has to be fixed)*/
		void SetRData (std::string rData)
		{
			m_rData = rData;
		}
		std::string GetRData (void) const
		{
			return m_rData;
		}
		
	private:
		std::string m_recordName; //!< the name of the record
  	uint32_t m_recordTimeToLive; //!< TTL value of the record
  	uint16_t m_recordClass; //!< class of the record
  	uint16_t m_recordType; //!< type of the record
    std::string m_rData; //!< either IN record or a NS record or a CNAME
};// end of SRVRECORD class

std::ostream& operator<< (std::ostream& os, SRVRecordEntry const& srv);

class SRVTable
{
	public:
	SRVTable ();
	~SRVTable ();

  /// Container for a RR
	typedef std::pair <SRVRecordEntry*, EventId> SRVRecordPair;

	/// Container for an instance of a RR table
	typedef std::list<std::pair <SRVRecordEntry*, EventId> > SRVRecordInstance;

	/// Iterator for an RR
  typedef std::list<std::pair <SRVRecordEntry*, EventId> >::iterator SRVRecordI;

	/// Constant Iterator for an RR
  typedef std::list<std::pair <SRVRecordEntry*, EventId> >::const_iterator SRVRecordCI;

	
	void AddRecord (std::string name, uint16_t nsClass, uint16_t type, uint32_t TTL, std::string rData);
	void AddZone (std::string name, uint16_t nsClass, uint16_t type, uint32_t TTL, std::string rData);	
	
	bool DeleteRecord (SRVRecordEntry* record);	
	bool UpdateRecordForTTL(SRVRecordEntry* record, uint32_t newTTL);
  bool UpdateRdata (SRVRecordEntry* record, std::string rData);	
  
	SRVTable::SRVRecordI FindARecord (std::string name, bool &found);
	bool FindRecordsFor (std::string name, SRVTable::SRVRecordInstance &instance);
	
  SRVTable::SRVRecordI FindARecordMatches (std::string name, bool &found); // Need RR

	SRVTable::SRVRecordI FindARecordHas (std::string name, bool &found); // Need RR
	bool FindAllRecordsHas (std::string name, SRVTable::SRVRecordInstance &instance);
	
  void SwitchServersRoundRobin (void);

	void SynchronizeTTL(void);
	
	void DoDispose ()
	{
		m_recordsTable.clear ();
	}
  void AssignIpv4(Ptr<Ipv4> ipv4)
	{
		m_ipv4 = ipv4;
		m_nodeId = m_ipv4->GetObject<Node> ()->GetId ();
	}

  void AssignStream (int64_t stream)
	{
  	m_rng = CreateObject<UniformRandomVariable> ();
	  m_rng->SetStream (stream);
	}
	
	private:

		SRVRecordInstance m_recordsTable; //!< RR tabl; //!< RR tablee
		Ptr<UniformRandomVariable> m_rng; //!< Rng stream.
		Ptr<Ipv4> m_ipv4; //!< Ipv4 pointer
		Ptr<Node> m_node; //!< node the routing protocol is running on 
		uint32_t m_nodeId; //!< node id
};


}// end of namespace ns3

#endif /* DNS_H */

