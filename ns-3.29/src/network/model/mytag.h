#ifndef MYTAG_H
#define MYTAG_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"

namespace ns3 {

// define this class in a public header
class MyTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  // these are our accessors to our tag structure
  void SetFlag();

  void SetDestinationIP(Ipv4Address dest);
  Ipv4Address GetDestinationIP();

  void SetSourceIP(Ipv4Address src);
  Ipv4Address GetSourceIP();

  void SetDestinationPort(uint16_t dest);
  uint16_t GetDestinationPort();

  void SetSourcePort(uint16_t src);
  uint16_t GetSourcePort();

  bool GetFlag (void) const;
private:
  bool m_isPW=false;
  Ipv4Address m_dstIP;
  Ipv4Address m_srcIP;

  uint16_t m_srcPort;
  uint16_t m_dstPort;
};
}
#endif /* MYTAG_H */
