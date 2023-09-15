/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iomanip>

#include "dns.h"

#include "ns3/simulator.h" 
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/timer.h"

NS_LOG_COMPONENT_DEFINE ("DNSResouceRecord"); 

namespace ns3 {
// SRVRecordEntry

	SRVRecordEntry::SRVRecordEntry (void)
	{
		// nothing
	}

	SRVRecordEntry::SRVRecordEntry (std::string rName,
																	uint32_t rTTL,
																	uint16_t rClass,
																	uint16_t rType,
																	std::string rData) : m_recordName (rName),
																											 m_recordTimeToLive (rTTL),
																										   m_recordClass (rClass),
																											 m_recordType (rType),
																											 m_rData (rData)
	{
		/*     cstrctr     */
	} 

	SRVRecordEntry::~SRVRecordEntry ()
	{
		/*     Dstrctr     */
	}

// NS Record table
// TODO
// Explain the class in-detail
//

// SRVTable
//

	SRVTable::SRVTable ()
	{
		m_rng = CreateObject<UniformRandomVariable> ();
		m_rng->SetStream (1);
		// Cstrctr
	}
	
	SRVTable::~SRVTable ()
	{
		// Dstrctr
	}

	void 
	SRVTable::AddRecord (std::string name, uint16_t nsClass, uint16_t type, uint32_t TTL, std::string rData)
	{
		NS_LOG_FUNCTION ( this <<
										name <<
			 							nsClass <<
										type << 
										TTL <<
										rData);

		SRVRecordEntry* newEntry = new SRVRecordEntry (name, 
																										TTL,
																										nsClass,
																										type,
																										rData);
																										
		Time delay;
		EventId removeEvent;

	  delay = Seconds (TTL) + Seconds (m_rng->GetValue (0.0, 5.0));
	  removeEvent = Simulator::Schedule (delay, &SRVTable::DeleteRecord, this, newEntry);

		m_recordsTable.push_front (std::make_pair (newEntry, removeEvent));
	}
	
	// Add a zone name to the DNS server without starting the expiration timer.
	// This method is added to add initial zones to the server.
	// At the time DNS server starts, these records will be scheduled to expire after a time of TTL
	void 
	SRVTable::AddZone (std::string name, uint16_t nsClass, uint16_t type, uint32_t TTL, std::string rData)
	{
		NS_LOG_FUNCTION ( this <<
										name <<
			 							nsClass <<
										type << 
										TTL <<
										rData);

		SRVRecordEntry* newEntry = new SRVRecordEntry (name, 
																										TTL,
																										nsClass,
																										type,
																										rData);
																										
//		Time delay;
//		EventId removeEvent;

//	  delay = Seconds (TTL) + Seconds (m_rng->GetValue (0.0, 5.0));
//	  removeEvent = Simulator::Schedule (delay, &SRVTable::DeleteRecord, this, newEntry);

		m_recordsTable.push_front (std::make_pair (newEntry, EventId ()));
	}	
	
	bool 
	SRVTable::DeleteRecord (SRVRecordEntry* record)
	{
		NS_LOG_FUNCTION (this << record);

		bool retValue = false;

		for (SRVRecordI it = m_recordsTable.begin (); it != m_recordsTable.end (); it++)
		{
			if (it->first->GetRecordName () == record->GetRecordName () &&
					it->first->GetClass () == record->GetClass () &&
					it->first->GetType () == record->GetType () &&
					it->first->GetRData () == record->GetRData ()) // || (it->first->GetCData () == record->GetCData ())))
			{
				m_recordsTable.erase (it);
				retValue = false;
				break;
			}
		}
		return retValue;
	}
	
	bool 
	SRVTable::UpdateRecordForTTL(SRVRecordEntry* record, uint32_t newTTL)
	{
		NS_LOG_FUNCTION (this << record << newTTL);
		bool retValue = false;
		for (SRVRecordI it = m_recordsTable.begin (); it != m_recordsTable.end (); it++)
		{
		  if (it->first->GetRecordName () == record->GetRecordName () &&
		      it->first->GetRData () == record->GetRData ())// || (it->first->GetCData () == record->GetCData ())))
			{
				// Only TTL and data part can be updated
				it->first->SetTTL (newTTL);
				retValue = true;
				break;
			}
		}
		return retValue;
	}	

  // Start the expiration time at the beginning of the application.	
	void 
	SRVTable::SynchronizeTTL(void)
	{
		NS_LOG_FUNCTION (this);
		Time delay;
		EventId removeEvent;
		for (SRVRecordI it = m_recordsTable.begin (); it != m_recordsTable.end (); it++)
		{	
		  it->second.Cancel ();
		  delay = Seconds (it->first->GetTTL ()) + Seconds (m_rng->GetValue (0.0, 5.0));
		  removeEvent = Simulator::Schedule (delay, &SRVTable::DeleteRecord, this, it->first);

      it->second = removeEvent;
		}
	}		

  bool
	SRVTable::UpdateRdata (SRVRecordEntry* record, std::string rData)
	{
		NS_LOG_FUNCTION (this << record << rData);
		bool retValue = false;
		for (SRVRecordI it = m_recordsTable.begin (); it != m_recordsTable.end (); it++)
		{
			if (it->first->GetRecordName () == record->GetRecordName () &&
					it->first->GetClass () == record->GetClass () &&
					it->first->GetType () == record->GetType ())
			{
				it->first->SetRData (rData);
				retValue = true;
				break;
			}
		}
		return retValue;
	}
	
  // Find a record that exactly matches a given query name.	
  // This is always the first record matches the given name	
	SRVTable::SRVRecordI 
	SRVTable::FindARecord (std::string name, bool &found)
	{
		NS_LOG_FUNCTION (this << name);
		SRVRecordI foundRecord;
		bool retValue = false;
		// This method is implemented by assuming a name has a structure to store its RData fields.
		for (SRVRecordI it = m_recordsTable.begin (); it != m_recordsTable.end (); it ++)
		{
			if (it->first->GetRecordName () == name)
			{
				foundRecord = it;
				retValue = true;
				break;
			}
		}
		found  = retValue;
		return foundRecord;
	}
	
  // Find all records that exactly matches a given query name.	
	bool 
	SRVTable::FindRecordsFor (std::string name, SRVTable::SRVRecordInstance &instance)
	{
    NS_LOG_FUNCTION (this << name);
    
    bool retValue = false;

		for (SRVRecordI it = m_recordsTable.begin (); it != m_recordsTable.end (); it ++)
		{
			if (it->first->GetRecordName () == name)
			{
    		SRVRecordEntry* newEntry = new SRVRecordEntry (it->first->GetRecordName (), 
																										it->first->GetTTL (),
																										it->first->GetClass (),
																										it->first->GetType (),
																										it->first->GetRData ());
																										
        instance.push_front (std::make_pair (newEntry, EventId ()));
				retValue = true;
			}
		}		
		return retValue;
	}		

  // return a DNS records witch matches either part or full string of of the dns records
  // for example, if the query is west.sd.keio.ac.jp,
  // This function returns .ac.jp
	SRVTable::SRVRecordI
	SRVTable::FindARecordMatches (std::string name, bool &found)
	{
    NS_LOG_FUNCTION (this << name);
    SRVRecordI foundRecord;
    bool retValue = false;
		std::size_t foundAt = 0;

    // This method is implemented by assuming a name has a structure to store its RData feilds.
    for (SRVRecordI it = m_recordsTable.begin (); it != m_recordsTable.end (); it ++)
    {
			foundAt = name.find (it->first->GetRecordName ());
			if (foundAt == std::string::npos)
			{
				continue;
			}
			else 
			{
        foundRecord = it;
        retValue = true;
				break; // returns the first record in the table
			}
    }
    found  = retValue;
    return foundRecord;
	}
	
  // return all DNS records witch matches either part or full string of a name
  // for example, if the query is west.sd.keio.ac.jp,
  // This function returns server1.west.sd.keio.ac.jp
	bool
	SRVTable::FindAllRecordsHas (std::string name, SRVTable::SRVRecordInstance &instance)
	{
    NS_LOG_FUNCTION (this << name);

    bool retValue = false;
		std::size_t foundAt = 0;

    // This method is implemented by assuming a name has a structure to store its RData feilds.
    for (SRVRecordI it = m_recordsTable.begin (); it != m_recordsTable.end (); it ++)
    {
			//foundAt = name.find (it->first->GetRecordName ());
			foundAt = it->first->GetRecordName ().find (name);
			if (foundAt == std::string::npos)
			{
				continue;
			}
			else 
			{
    		SRVRecordEntry* newEntry = new SRVRecordEntry (it->first->GetRecordName (), 
																										it->first->GetTTL (),
																										it->first->GetClass (),
																										it->first->GetType (),
																										it->first->GetRData ());
																										
        instance.push_front (std::make_pair (newEntry, EventId ()));
        retValue = true;
			}
    }
		return retValue;
	}	

	SRVTable::SRVRecordI
	SRVTable::FindARecordHas (std::string name, bool &found)
	{
    NS_LOG_FUNCTION (this << name);
    SRVRecordI foundRecord;
    bool retValue = false;
		std::size_t foundAt = 0;

    // This method is implemented by assuming a name has a structure to store its RData feilds.
    for (SRVRecordI it = m_recordsTable.begin (); it != m_recordsTable.end (); it ++)
    {
			//foundAt = name.find (it->first->GetRecordName ());
    	NS_LOG_INFO(it->first->GetRecordName ());
			foundAt = it->first->GetRecordName ().find (name);
			if (foundAt == std::string::npos)
			{
				continue;
			}
			else 
			{
        foundRecord = it;
        retValue = true;
				break; // returns the first record in the table
			}
    }
    found  = retValue;
    return foundRecord;
	}
	
  // This method is implement to support Round Robin algorithm for server selection
  void
	SRVTable::SwitchServersRoundRobin (void)
	{
		NS_LOG_FUNCTION (this);
		if (!m_recordsTable.empty () || m_recordsTable.size () != 1)
		{
		  SRVRecordI it = m_recordsTable.begin ();

      SRVRecordEntry* newEntry = new SRVRecordEntry (it->first->GetRecordName (), 
																						      it->first->GetTTL (),
																						      it->first->GetClass (),
																						      it->first->GetType (),
																						      it->first->GetRData ());
																						
      m_recordsTable.push_back (std::make_pair (newEntry, it->second));				
      m_recordsTable.erase (it);		
    }
	}	
}

