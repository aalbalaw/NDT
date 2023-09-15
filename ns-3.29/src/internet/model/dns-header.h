/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Hiroaki Nishi Laboratory, Keio University, Japan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Author: Janaka Wijekoon <janaka@west.sd.ekio.ac.jp>, Hiroaki Nishi <west@sd.keio.ac.jp>
 */

#ifndef DNS_HEADER_H
#define DNS_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"

namespace ns3 {
//  4.1.2. Question section format

//  The question section is used to carry the "question" in most queries,
//  i.e., the parameters that define what is being asked.  The section
//  contains QDCOUNT (usually 1) entries, each of the following format:

//                                      1  1  1  1  1  1
//        0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |                                               |
//      /                     QNAME                     /
//      /                                               /
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |                     QTYPE                     |
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |                     QCLASS                    |
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

//  where:

//  QNAME           a domain name represented as a sequence of labels, where
//                  each label consists of a length octet followed by that
//                  number of octets.  The domain name terminates with the
//                  zero length octet for the null label of the root.  Note
//                  that this field may be an odd number of octets; no
//                  padding is used.

//  QTYPE           a two octet code which specifies the type of the query.
//                  The values for this field include all codes valid for a
//                  TYPE field, together with some more general codes which
//                  can match more than one type of RR.

//  QCLASS          a two octet code that specifies the class of the query.
//                  For example, the QCLASS field is IN for the Internet.

class QuestionSectionHeader : public Header
{
public:
  QuestionSectionHeader ();
  virtual ~ QuestionSectionHeader ();
  
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Return the instance type identifier.
   * \return instance type ID
   */
  virtual TypeId GetInstanceTypeId (void) const;

  virtual void Print (std::ostream& os) const;

  /**
   * \brief Get the serialized size of the packet.
   * \return size
   */
  virtual uint32_t GetSerializedSize (void) const;

  /**
   * \brief Serialize the packet.
   * \param start Buffer iterator
   */
  virtual void Serialize (Buffer::Iterator start) const;

  /**
   * \brief Deserialize the packet.
   * \param start Buffer iterator
   * \return size of the packet
   */
  virtual uint32_t Deserialize (Buffer::Iterator start);
  
private:
  std::string m_qName;
  uint16_t m_qType;
  uint16_t m_qClass;
  
public:
  /*
  * /brief Get and Set the qname
  */  
  std::string GetqName () const
  {
    return m_qName;
  }
  void SetqName (std::string qname)
  {
    m_qName = qname;    
  }   
   
  /*
  * /brief Get and Set the qtype
  */  
  uint16_t GetqType () const
  {
    return m_qType;
  }
  void SetqType (uint16_t qtype)
  {
    m_qType = qtype;    
  }     
  
  /*
  * /brief Get and Set the qclass
  */  
  uint16_t GetqClass () const
  {
    return m_qClass;
  }
  void SetqClass (uint16_t qclass)
  {
    m_qClass = qclass;    
  }     
      
};// end of QuestionSectionHeader

static inline std::ostream& operator<< (std::ostream& os, const QuestionSectionHeader & header)
{
  header.Print (os);
  return os;
}
//4.1.3. Resource record format / Answer

//The answer, authority, and additional sections all share the same
//format: a variable number of resource records, where the number of
//records is specified in the corresponding count field in the header.
//Each resource record has the following format:
//                                    1  1  1  1  1  1
//      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//    |                                               |
//    /                                               /
//    /                      NAME                     /
//    |                                               |
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//    |                      TYPE                     |
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//    |                     CLASS                     |
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//    |                      TTL                      |
//    |                                               |
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//    |                   RDLENGTH                    |
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
//    /                     RDATA                     /
//    /                                               /
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//where:

//NAME            a domain name to which this resource record pertains.

//TYPE            two octets containing one of the RR type codes.  This
//                field specifies the meaning of the data in the RDATA
//                field.



//CLASS           two octets which specify the class of the data in the
//                RDATA field.

//TTL             a 32 bit unsigned integer that specifies the time
//                interval (in seconds) that the resource record may be
//                cached before it should be discarded.  Zero values are
//                interpreted to mean that the RR can only be used for the
//                transaction in progress, and should not be cached.

//RDLENGTH        an unsigned 16 bit integer that specifies the length in
//                octets of the RDATA field.

//RDATA           a variable length string of octets that describes the
//                resource.  The format of this information varies
//                according to the TYPE and CLASS of the resource record.
//                For example, the if the TYPE is A and the CLASS is IN,
//                the RDATA field is a 4 octet ARPA Internet address.

class ResourceRecordHeader : public Header
{
public:
  ResourceRecordHeader ();
  virtual ~ ResourceRecordHeader ();
  
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Return the instance type identifier.
   * \return instance type ID
   */
  virtual TypeId GetInstanceTypeId (void) const;

  virtual void Print (std::ostream& os) const;

  /**
   * \brief Get the serialized size of the packet.
   * \return size
   */
  virtual uint32_t GetSerializedSize (void) const;

  /**
   * \brief Serialize the packet.
   * \param start Buffer iterator
   */
  virtual void Serialize (Buffer::Iterator start) const;

  /**
   * \brief Deserialize the packet.
   * \param start Buffer iterator
   * \return size of the packet
   */
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  std::string m_name;
  uint16_t m_type;
  uint16_t m_class;
  uint32_t m_timeToLive;
  std::string m_rData; // !< This contains the resource data (IP, CNAME or NS records)
  uint16_t m_rDataLength; // !< the length of the resource data

public:
  /*
  * /brief Get and Set thename
  */  
  std::string GetName () const
  {
    return m_name;
  }
  void SetName (std::string name)
  {
    m_name = name; // Added the "." for testing    
  }   
   
  /*
  * /brief Get and Set the qtype
  */  
  uint16_t GetType () const
  {
    return m_type;
  }
  void SetType (uint16_t type)
  {
    m_type = type;    
  }     
  
  /*
  * /brief Get and Set the qclass
  */  
  uint16_t GetClass () const
  {
    return m_class;
  }
  void SetClass (uint16_t cclass)
  {
    m_class = cclass;
  } 
  
  /*
  * /brief Get and Set the qtype
  */  
  uint32_t GetTimeToLive () const
  {
    return m_timeToLive;
  }
  void SetTimeToLive (uint32_t timeToLive)
  {
    m_timeToLive = timeToLive;    
  }    

  /*
  * /brief Get and Set the Resource data
  */  
  std::string GetRData () const
  {
    return m_rData;
  }
  void SetRData (std::string rData)
  {
    m_rData = rData;
    SetRdLength (); // Set the length of the resource data
  } 

  /*
  * /brief Get and Set the resource data length
  */  
  uint16_t GetRdLength () const
  {
    return m_rDataLength;
  }
  void SetRdLength (void)
  {
    m_rDataLength = m_rData.size ();    
  }     
};// end of ResourceRecordHeader


// DNS Header (RFC 1035)
//  4.1.1. Header section format

//  The header contains the following fields:

//                                      1  1  1  1  1  1
//        0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |                      ID                       |
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |                    QDCOUNT                    |
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |                    ANCOUNT                    |
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |                    NSCOUNT                    |
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//      |                    ARCOUNT                    |
//      +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

//  where:

//  ID              A 16 bit identifier assigned by the program that
//                  generates any kind of query.  This identifier is copied
//                  the corresponding reply and can be used by the requester
//                  to match up replies to outstanding queries.

//  QR              A one bit field that specifies whether this message is a
//                  query (0), or a response (1).

//  OPCODE          A four bit field that specifies kind of query in this
//                  message.  This value is set by the originator of a query
//                  and copied into the response.  The values are:

//                  0               a standard query (QUERY)

//                  1               an inverse query (IQUERY)

//                  2               a server status request (STATUS)

//                  3-15            reserved for future use

//  AA              Authoritative Answer - this bit is valid in responses,
//                  and specifies that the responding name server is an
//                  authority for the domain name in question section.

//                  Note that the contents of the answer section may have
//                  multiple owner names because of aliases.  The AA bit
//                  corresponds to the name which matches the query name, or
//                  the first owner name in the answer section.

//  TC              TrunCation - specifies that this message was truncated
//                  due to length greater than that permitted on the
//                  transmission channel.

//  RD              Recursion Desired - this bit may be set in a query and
//                  is copied into the response.  If RD is set, it directs
//                  the name server to pursue the query recursively.
//                  Recursive query support is optional.

//  RA              Recursion Available - this be is set or cleared in a
//                  response, and denotes whether recursive query support is
//                  available in the name server.

//  Z               Reserved for future use.  Must be zero in all queries
//                  and responses.

//  RCODE           Response code - this 4 bit field is set as part of
//                  responses.  The values have the following
//                  interpretation:

//                  0               No error condition

//                  1               Format error - The name server was
//                                  unable to interpret the query.

//                  2               Server failure - The name server was
//                                  unable to process this query due to a
//                                  problem with the name server.

//                  3               Name Error - Meaningful only for
//                                  responses from an authoritative name
//                                  server, this code signifies that the
//                                  domain name referenced in the query does
//                                  not exist.

//                  4               Not Implemented - The name server does
//                                  not support the requested kind of query.

//                  5               Refused - The name server refuses to
//                                  perform the specified operation for
//                                  policy reasons.  For example, a name
//                                  server may not wish to provide the
//                                  information to the particular requester,
//                                  or a name server may not wish to perform
//                                  a particular operation (e.g., zonetransfer) 
//                                  for particular data.

//                  6-15            Reserved for future use.

//  QDCOUNT         an unsigned 16 bit integer specifying the number of
//                  entries in the question section.

//  ANCOUNT         an unsigned 16 bit integer specifying the number of
//                  resource records in the answer section.

//  NSCOUNT         an unsigned 16 bit integer specifying the number of name
//                  server resource records in the authority records
//                  section.

//  ARCOUNT         an unsigned 16 bit integer specifying the number of
//                  resource records in the additional records section.

#define DNS_HEADER_SIZE 12

class DNSHeader : public Header
{
public:
  /*cstrctr*/
  DNSHeader ();
  /*destrctr*/
  virtual ~DNSHeader ();
    
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Return the instance type identifier.
   * \return instance type ID
   */
  virtual TypeId GetInstanceTypeId (void) const;

  virtual void Print (std::ostream& os) const;

  /**
   * \brief Get the serialized size of the packet.
   * \return size
   */
  virtual uint32_t GetSerializedSize (void) const;

  /**
   * \brief Serialize the packet.
   * \param start Buffer iterator
   */
  virtual void Serialize (Buffer::Iterator start) const;

  /**
   * \brief Deserialize the packet.
   * \param start Buffer iterator
   * \return size of the packet
   */
  virtual uint32_t Deserialize (Buffer::Iterator start);
  
private:
  uint16_t m_id;
  uint16_t m_flagSet; //!< QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
  uint16_t m_qdCount;
  uint16_t m_anCount;
  uint16_t m_nsCount;
  uint16_t m_arCount;
  
  uint16_t m_totalRecordsCount; // !< the Total records added to the DNS header
  
public:
  /*
   * /brief Get and Set the ID
  */
  uint16_t GetId () const
  {
    return m_id;
  }
  void SetId ( uint16_t id)
  {
    m_id = id;
  }
  
  /*
   * /brief Get and Set the flags which are given in the second octet 
   */  
  bool GetQRbit () const
  {
    return (m_flagSet & (1 << 15));
  }
  void SetQRbit (bool flag)
  {
    (flag) ? m_flagSet |= (1 << 15) :  m_flagSet &= ~(1 << 15);
  }
  
  uint8_t GetOpcode () const
  {
    return ((m_flagSet >> 11) & 0x000F);
  }
  void SetOpcode (uint8_t opcode)
  {
    m_flagSet |= (opcode << 11);    
  }
  void ResetOpcode (void)
  {
    m_flagSet &= (0 << 11);    
  }  
  
  bool GetAAbit () const
  {
    return (m_flagSet & (1 << 10));  
  }
  void SetAAbit (bool flag)
  {
    (flag) ? m_flagSet |= (1 << 10) :  m_flagSet &= ~(1 << 10);   
  }
  
  bool GetTCbit () const
  {
    return (m_flagSet & (1 << 9));  
  }
  void SetTCbit (bool flag)
  {
    (flag) ? m_flagSet |= (1 << 9) :  m_flagSet &= ~(1 << 9);    
  }  
  
  bool GetRDbit () const
  {
    return (m_flagSet & (1 << 8));  
  }
  void SetRDbit (bool flag)
  {
    (flag) ? m_flagSet |= (1 << 8) :  m_flagSet &= ~(1 << 8);    
  }  
  
  bool GetRAbit () const
  {
    return (m_flagSet & (1 << 7));  
  }
  void SetRAbit (bool flag)
  {
    (flag) ? m_flagSet |= (1 << 7) :  m_flagSet &= ~(1 << 7);    
  }

	uint8_t GetZcode () const
	{
		return ((m_flagSet >> 4) & 0x000F);
	}
  void SetZcode (void)
	{
		m_flagSet |= (0x00 << 4);
	}

  uint8_t GetRcode () const
  {
    return ((m_flagSet >> 0) & 0x000F);
  }
  void SetRcode (uint8_t opcode)
  {
    m_flagSet |= (opcode << 0);    
  }  
  
  /*
  * /brief Get and Set the number of entries in the question section
  */  
  uint16_t GetQdCount () const
  {
    return m_qdCount;
  }
  void SetQdCount (/*uint8_t qdCount*/)
  {
    m_qdCount ++;// = qdCount;  
    m_totalRecordsCount ++;  
  }
  void ResetQdCount (/*uint8_t qdCount*/)
  {
    m_totalRecordsCount = m_totalRecordsCount - m_qdCount;
    m_qdCount = 0; 
  }    
  
  /*
  * /brief Get and Set the number of entries in the answer section
  */  
  uint16_t GetAnCount () const
  {
    return m_anCount;
  }
  void SetAnCount (/*uint8_t anCount*/)
  {
    m_anCount ++;//= anCount; 
    m_totalRecordsCount ++;   
  }  
  void ResetAnCount (/*uint8_t anCount*/)
  {
    m_totalRecordsCount = m_totalRecordsCount - m_anCount;  
    m_anCount = 0;   
  }    
  /*
  * /brief Get and Set the number of name server records in the authority section
  */  
  uint16_t GetNsCount () const
  {
    return m_nsCount;
  }
  void SetNsCount (/*uint8_t nsCount*/)
  {
    m_nsCount ++;//= nsCount; 
    m_totalRecordsCount ++;   
  }   
  void ResetNsCount (/*uint8_t nsCount*/)
  {
    m_totalRecordsCount = m_totalRecordsCount - m_nsCount;    
    m_nsCount  = 0;   
  }     
  /*
  * /brief Get and Set the number of entries in the additional section
  */  
  uint16_t GetArCount () const
  {
    return m_arCount;
  }
  void SetArCount (/*uint8_t arCount*/)
  {
    m_arCount ++;//= arCount;
    m_totalRecordsCount ++;    
  }
  void ResetArCount (/*uint8_t arCount*/)
  {
    m_totalRecordsCount = m_totalRecordsCount - m_arCount;      
    m_arCount = 0;    
  }  
  
private:
  std::list <QuestionSectionHeader> m_qdList;
  std::list <ResourceRecordHeader> m_rrList;
  std::list <ResourceRecordHeader> m_nsList;
  std::list <ResourceRecordHeader> m_arList;
  
  //TODO
  // Calculate maximum records can be added to the DNS header
  // For this implementation we assumed that number of records
  // will not create any fragmentation.
  
public:

// name The question section header manipulation
//\{
  /**
   * \brief Add a Question to the DNS message
   * \param question the question that should add to DNS message
   */
  void AddQuestion (QuestionSectionHeader question)
  {
    m_qdList.push_front (question);
    SetQdCount ();
  }

  /**
   * \brief Delete a Question from the DNS message
   * \param question the question that should delete from DNS message
   */
  void DeleteQuestion (QuestionSectionHeader question)
  {
    for (std::list <QuestionSectionHeader>::iterator it = m_qdList.begin (); it!= m_qdList.end (); it++)
    {
      if ((question.GetqName () == it->GetqName ()) &&
          (question.GetqType () == it->GetqType ()))
      { 
        m_qdList.erase (it);
        m_qdCount --;
        m_totalRecordsCount --;
      }
    }   
  }

  /**
   * \brief clear all the questions in the DNS message
   */
  void ClearQuestions ()
  {
    m_qdList.clear ();  
    ResetQdCount ();   
  }

  /**
   * \brief Get the list of the questions added to the DNS message
   * \returns the list of the questions added to the DNS message
   */
  std::list<QuestionSectionHeader> GetQuestionList (void) const
  {
    return m_qdList;
  } 
//\}

// name The answer section header manipulation
//\{
  /**
   * \brief Add a Answer to the DNS message
   * \param answer the answer that should add to DNS message
   */
  void AddAnswer (ResourceRecordHeader answer)
  {
    m_rrList.push_front (answer);
    SetAnCount ();
  }

  /**
   * \brief Delete a answer from the DNS message
   * \param answeer the answer that should delete from DNS message
   */
  void DeleteAnswer (ResourceRecordHeader answer)
  {
    for (std::list <ResourceRecordHeader>::iterator it = m_rrList.begin (); it!= m_rrList.end (); it++)
    {
      if ((answer.GetName () == it->GetName ()) &&
          (answer.GetType () == it->GetType ()))
      { 
        m_rrList.erase (it);
        m_anCount --;  
        m_totalRecordsCount --;              
      }
    }   
  }

  /**
   * \brief clear all the answer in the DNS message
   */
  void ClearAnswers ()
  {
    m_rrList.clear ();  
    ResetAnCount ();       
  }

  /**
   * \brief Get the list of the answer added to the DNS message
   * \returns the list of the answer added to the DNS message
   */
  std::list<ResourceRecordHeader> GetAnswerList (void) const
  {
    return m_rrList;
  } 
//\}

// name The authority record section header manipulation
//\{
  /**
   * \brief Add a ns record to the DNS message
   * \param nsRecord the ns record that should add to DNS message
   */
  void AddNsRecord (ResourceRecordHeader nsRecord)
  {
    m_nsList.push_front (nsRecord);
    SetNsCount ();
  }

  /**
   * \brief Delete a ns record from the DNS message
   * \param nsRecord the ns record that should delete from DNS message
   */
  void DeleteNsRecord (ResourceRecordHeader nsRecord)
  {
    for (std::list <ResourceRecordHeader>::iterator it = m_nsList.begin (); it!= m_nsList.end (); it++)
    {
      if ((nsRecord.GetName () == it->GetName ()) &&
          (nsRecord.GetType () == it->GetType ()))
      { 
        m_nsList.erase (it);
        m_nsCount --;
        m_totalRecordsCount --;        
      }
    }   
  }

  /**
   * \brief clear all the ns records in the DNS message
   */
  void ClearNsRecords ()
  {
    m_nsList.clear ();
    ResetNsCount ();         
  }

  /**
   * \brief Get the list of the ns records added to the DNS message
   * \returns the list of the ns records added to the DNS message
   */
  std::list<ResourceRecordHeader> GetNsRecordList (void) const
  {
    return m_nsList;
  } 
//\}

// name The additional record section header manipulation
//\{
  /**
   * \brief Add a additional record to the DNS message
   * \param aRecord the additional record that should add to DNS message
   */
  void AddARecord (ResourceRecordHeader aRecord)
  {
    m_arList.push_front (aRecord);
    SetArCount ();
  }

  /**
   * \brief Delete a additional record from the DNS message
   * \param aRecord the additional record that should delete from DNS message
   */
  void DeleteARecord (ResourceRecordHeader aRecord)
  {
    for (std::list <ResourceRecordHeader>::iterator it = m_arList.begin (); it!= m_arList.end (); it++)
    {
      if ((aRecord.GetName () == it->GetName ()) &&
          (aRecord.GetType () == it->GetType ()))
      { 
        m_arList.erase (it);
        m_arCount --;
        m_totalRecordsCount --;        
      }
    }   
  }

  /**
   * \brief clear all the additional records in the DNS message
   */
  void ClearArList ()
  {
    m_arList.clear (); 
    ResetArCount ();     
  }

  /**
   * \brief Get the list of the additional records added to the DNS message
   * \returns the list of the additional records added to the DNS message
   */
  std::list<ResourceRecordHeader> GetArList (void) const
  {
    return m_arList;
  } 
//\}
     
};// end DNSHeader

static inline std::ostream& operator<< (std::ostream& os, const DNSHeader & header)
{
  header.Print (os);
  return os;
}
} // end of ns-3 namespace
#endif /* HEADERS */

