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

#include <string>
#include "dns-header.h"

#include "ns3/assert.h"
#include "ns3/log.h"


namespace ns3 {
	/**
	 * color definitions*/
	#define RESET   "\033[0m"
	#define BLACK   "\033[30m"      /* Black */
	#define RED     "\033[31m"      /* Red */
	#define GREEN   "\033[32m"      /* Green */
	#define YELLOW  "\033[33m"      /* Yellow */
	#define BLUE    "\033[34m"      /* Blue */
	#define MAGENTA "\033[35m"      /* Magenta */
	#define CYAN    "\033[36m"      /* Cyan */
	#define WHITE   "\033[37m"      /* White */
	#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
	#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
	#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
	#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
	#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
	#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
	#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
	#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */
// Question Header
NS_OBJECT_ENSURE_REGISTERED (QuestionSectionHeader);

  QuestionSectionHeader::QuestionSectionHeader ()
  {}
  
  QuestionSectionHeader::~QuestionSectionHeader ()  
  {}        
  
  TypeId
  QuestionSectionHeader::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::QuestionSectionHeader")
                        .SetParent<Header> ()
                        .AddConstructor<QuestionSectionHeader> ();
    return tid;
  }
  TypeId
  QuestionSectionHeader::GetInstanceTypeId (void) const
  {
    return GetTypeId ();
  }

  void 
  QuestionSectionHeader::Print (std::ostream &os) const
  {
    os << " " << RED << m_qName <<
       ": type " << m_qType <<
       ", class " << m_qClass << RESET << std::endl;
  }
  
  uint32_t 
  QuestionSectionHeader::GetSerializedSize (void) const
  {
    return (2 /*2B to send the size of the query*/ + 
            m_qName.size () + 1 /* size of the query and additional 1 byte for end*/ + 
            sizeof (m_qType) /* size of the type*/ + 
            sizeof (m_qClass) /*size of the class*/
            ); 
  }
  
  void 
  QuestionSectionHeader::Serialize (Buffer::Iterator start) const
  {
    Buffer::Iterator i = start;

	  i.WriteU16 (m_qName.size () + 1);
		i.Write ((uint8_t*) m_qName.c_str (), m_qName.size () + 1);
    i.WriteHtonU16 (m_qType);
    i.WriteHtonU16 (m_qClass);     
  }
  
  uint32_t 
  QuestionSectionHeader::Deserialize (Buffer::Iterator start)
  {
    Buffer::Iterator i = start;
    uint8_t receivedSize = 0;
    m_qName = std::string ("");

		receivedSize = i.ReadU16 ();
		char data[receivedSize];
		i.Read ((uint8_t*) data, receivedSize);
		m_qName = data;
    m_qType = i.ReadNtohU16 ();
    m_qClass = i.ReadNtohU16 ();
    
    return QuestionSectionHeader::GetSerializedSize ();
  }

// RR Header
NS_OBJECT_ENSURE_REGISTERED (ResourceRecordHeader);

  ResourceRecordHeader::ResourceRecordHeader ()
  {}
  
  ResourceRecordHeader::~ResourceRecordHeader ()  
  {}        
  
  TypeId
  ResourceRecordHeader::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::ResourceRecordHeader")
                        .SetParent<Header> ()
                        .AddConstructor<ResourceRecordHeader> ();
    return tid;
  }
  TypeId
  ResourceRecordHeader::GetInstanceTypeId (void) const
  {
    return GetTypeId ();
  }

  void 
  ResourceRecordHeader::Print (std::ostream &os) const
  {
    if (m_type == 1)
    { 
      os << " " << m_name <<
        ": type A" <<
         ", class IN" << 
         ", addr " << m_rData << std::endl;
      os << "   Name: " << m_name << std::endl;
      os << "   Type: " << m_type << std::endl;
      os << "   Class: " << m_class << std::endl;
      os << "   Time to Live: " << m_timeToLive << std::endl;
      os << "   Data length: " << m_rDataLength << std::endl; 
      os << "   Address: " << m_rData << RESET << std::endl;
    } 
    if (m_type == 2)
    { 
      os << " " << m_name <<
        ": type NS" <<
         ", class IN" << 
         ", ns record " << m_rData << std::endl;
      os << "   Name: " << m_name << std::endl;
      os << "   Type: " << m_type << std::endl;
      os << "   Class: " << m_class << std::endl;
      os << "   Time to Live: " << m_timeToLive << std::endl;
      os << "   Data length: " << m_rDataLength << std::endl; 
      os << "   NS: " << m_rData << RESET << std::endl;
    }     
    if(m_type == 5)
    { 
      os << " " << m_name <<
        ": type CNAME" <<
         ", class IN" << 
         ", cname " << m_rData << std::endl;
      os << "   Name: " << m_name << std::endl;
      os << "   Type: " << m_type << std::endl;
      os << "   Class: " << m_class << std::endl;
      os << "   Time to Live: " << m_timeToLive << std::endl;
      os << "   Data length: " << m_rDataLength << std::endl; 
      os << "   CNAME: " << m_rData << RESET << std::endl;
    }         
  }
  
  uint32_t 
  ResourceRecordHeader::GetSerializedSize (void) const
  {
    return (2 /* 2B to send size of the name*/ +
            m_name.size () + 1/* size of the name and additional 1 byte for end*/ +
            sizeof (m_type) /* size of the type*/ + 
            sizeof (m_class) /* size of the class*/ + 
            sizeof (m_timeToLive) /* size of the TTL*/ + 
            sizeof (m_rDataLength) /* size of the resource record*/ +
            2 /* 2B to send the size of the resource record*/ +
            m_rData.size () + 1/* size of the resource record and additional 1 byte for end*/
            );
  }
  
  void 
  ResourceRecordHeader::Serialize (Buffer::Iterator start) const
  {
    Buffer::Iterator i = start;

		i.WriteU16 ((m_name.size ()+1)); // size of the string 
		i.Write ((uint8_t*) m_name.c_str (), (m_name.size ()+1));
  
    i.WriteHtonU16 (m_type);
    i.WriteHtonU16 (m_class);      
    i.WriteHtonU32 (m_timeToLive);      
    i.WriteHtonU16 (m_rDataLength);

    i.WriteU16 ((m_rData.size ()+1));
	  i.Write ((uint8_t*) m_rData.c_str (), (m_rData.size ()+1));     
  }
  
  uint32_t 
  ResourceRecordHeader::Deserialize (Buffer::Iterator start)
  {
    Buffer::Iterator i = start;
    uint8_t receivedSize;
    m_name = std::string ("");

		receivedSize = i.ReadU16 ();
		char data[receivedSize];
		i.Read ((uint8_t*) data, receivedSize);
		m_name = data;

    m_type = i.ReadNtohU16 ();
    m_class = i.ReadNtohU16 ();
    m_timeToLive = i.ReadNtohU32 ();
    m_rDataLength = i.ReadNtohU16 ();     

    receivedSize = 0;
    m_rData = std::string ("");
		receivedSize = i.ReadU16 ();
  	
  	char data1[receivedSize];
		i.Read ((uint8_t*) data1, receivedSize);
		m_rData = data1;

    return ResourceRecordHeader::GetSerializedSize ();
  }

//DNS Header
NS_OBJECT_ENSURE_REGISTERED (DNSHeader);
NS_LOG_COMPONENT_DEFINE ("DNSHeader");

  DNSHeader::DNSHeader ()
    : m_id (0x01A9),
      m_flagSet (0x0000),
      m_qdCount (0),
      m_anCount (0),
      m_nsCount (0),
      m_arCount (0),
      m_totalRecordsCount (0)  
  {}
  
  DNSHeader::~DNSHeader ()  
  {}        
  
  TypeId
  DNSHeader::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::DNSHeader").SetParent<Header> ().AddConstructor<DNSHeader> ();
    return tid;
  }
  TypeId
  DNSHeader::GetInstanceTypeId (void) const
  {
    return GetTypeId ();
  }

  void 
  DNSHeader::Print (std::ostream &os) const
  {
    os << YELLOW << "DNS header" << " ID: " << m_id << std::endl;
    os << " Flags: " << m_flagSet << std::endl;
    os <<  " Questions: " << m_qdCount << std::endl;
    os <<  " Answers: " << m_anCount << std::endl;
    os <<  " Authority RRs: " << m_nsCount << std::endl;
    os <<  " Additional RRs: " << m_arCount << RESET << std::endl; 
    if (m_qdCount != 0)
    {
      // Since queries has different sizes
      for (std::list<QuestionSectionHeader>::const_iterator iter = m_qdList.begin (); 
				   iter != m_qdList.end (); 
				   iter ++)
      {
        os << "Queries " << std::endl;
        iter->Print (os);
      }      
    }
    if (m_anCount != 0)
    {
      for (std::list<ResourceRecordHeader>::const_iterator iter = m_rrList.begin (); 
				   iter != m_rrList.end (); 
				   iter ++)
      {
        os << "Answers " << GREEN << std::endl;
        iter->Print (os);
      }      
    }
    if (m_nsCount != 0)
    {
      for (std::list<ResourceRecordHeader>::const_iterator iter = m_nsList.begin (); 
				   iter != m_nsList.end (); 
				   iter ++)
      {
        os << "Authority RRs " << YELLOW << std::endl;
        iter->Print (os);
      }
    }
    if (m_arCount != 0)
    {
      for (std::list<ResourceRecordHeader>::const_iterator iter = m_arList.begin (); 
				   iter != m_arList.end (); 
				   iter ++)
      {
        os << "Additional RRs " << BLUE << std::endl;
        iter->Print (os);
      }
    } 
  }
  
  uint32_t 
  DNSHeader::GetSerializedSize (void) const
  {
    QuestionSectionHeader question;
    ResourceRecordHeader rRecords;
    
    uint32_t totHeaderSize = DNS_HEADER_SIZE;
    
    if (m_qdCount != 0)
    {
      // Since queries has different sizes
      for (std::list<QuestionSectionHeader>::const_iterator iter = m_qdList.begin (); 
				   iter != m_qdList.end (); 
				   iter ++)
      {
        totHeaderSize = totHeaderSize + iter->GetSerializedSize ();
      }      
    }
    if (m_anCount != 0)
    {
      for (std::list<ResourceRecordHeader>::const_iterator iter = m_rrList.begin (); 
				   iter != m_rrList.end (); 
				   iter ++)
      {
        totHeaderSize = totHeaderSize + iter->GetSerializedSize ();
      }      
    }
    if (m_nsCount != 0)
    {
      for (std::list<ResourceRecordHeader>::const_iterator iter = m_nsList.begin (); 
				   iter != m_nsList.end (); 
				   iter ++)
      {
        totHeaderSize = totHeaderSize + iter->GetSerializedSize ();
      }
    }
    if (m_arCount != 0)
    {
      for (std::list<ResourceRecordHeader>::const_iterator iter = m_arList.begin (); 
				   iter != m_arList.end (); 
				   iter ++)
      {
        totHeaderSize = totHeaderSize + iter->GetSerializedSize ();
      }
    }        
    
    return totHeaderSize;
  }
  
  void 
  DNSHeader::Serialize (Buffer::Iterator start) const
  {
    Buffer::Iterator i = start;
    i.WriteHtonU16 (m_id);
    i.WriteHtonU16 (m_flagSet);
    i.WriteHtonU16 (m_qdCount);
    i.WriteHtonU16 (m_anCount);
    i.WriteHtonU16 (m_nsCount);
    i.WriteHtonU16 (m_arCount);    
    
    // Serializing records added to the DNS header
    if (m_qdCount != 0) // Check any queries are added to the DNS header
    {
      for (std::list<QuestionSectionHeader>::const_iterator iter = m_qdList.begin (); 
				   iter != m_qdList.end (); 
				   iter ++)
      {
        iter->Serialize (i);
        i.Next(iter->GetSerializedSize ());
      }
    }   
    if (m_anCount != 0) // Check any answers are added to the DNS header
    {
      for (std::list<ResourceRecordHeader>::const_iterator iter = m_rrList.begin (); 
				   iter != m_rrList.end (); 
				   iter ++)
      {
        iter->Serialize (i);
        i.Next(iter->GetSerializedSize ());
      }
    }
    if (m_nsCount != 0) // Check any ns records are added to the DNS header
    {
      for (std::list<ResourceRecordHeader>::const_iterator iter = m_nsList.begin (); 
				   iter != m_nsList.end (); 
				   iter ++)
      {
        iter->Serialize (i);
        i.Next(iter->GetSerializedSize ());
      }
    }
    if (m_arCount != 0) // Check any additional records are added to the DNS header
    {
      for (std::list<ResourceRecordHeader>::const_iterator iter = m_arList.begin (); 
				   iter != m_arList.end (); 
				   iter ++)
      {
        iter->Serialize (i);
        i.Next(iter->GetSerializedSize ());
      }
    }            
  }
  
  uint32_t 
  DNSHeader::Deserialize (Buffer::Iterator start)
  {
    Buffer::Iterator i = start;
    m_id  = i.ReadNtohU16 ();
    m_flagSet  = i.ReadNtohU16 ();
    m_qdCount  = i.ReadNtohU16 ();
    m_anCount  = i.ReadNtohU16 ();
    m_nsCount  = i.ReadNtohU16 ();
    m_arCount  = i.ReadNtohU16 ();
    
    if (m_qdCount != 0) 
    {
		  for (uint8_t n = 0; n < m_qdCount; n++)
      {
        QuestionSectionHeader question;
        i.Next (question.Deserialize (i));
        m_qdList.push_back (question);
      }
    }
    if (m_anCount != 0) 
    {
		  for (uint8_t n = 0; n < m_anCount; n++)
      {
        ResourceRecordHeader answer;
        i.Next (answer.Deserialize (i));
        m_rrList.push_back (answer);
      }
    }
    if (m_nsCount != 0) 
    {
		  for (uint8_t n = 0; n < m_nsCount; n++)
      {
        ResourceRecordHeader nsRecord;
        i.Next (nsRecord.Deserialize (i));
        m_nsList.push_back (nsRecord);
      }
    } 
    if (m_arCount != 0) 
    {
		  for (uint8_t n = 0; n < m_arCount; n++)
      {
        ResourceRecordHeader aRecord;
        i.Next (aRecord.Deserialize (i));
        m_arList.push_back (aRecord);
      }
    }             
    return DNSHeader::GetSerializedSize ();  
  }  
}

