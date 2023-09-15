#ifndef PRODUCER_H
#define PRODUCER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/udp-socket.h"
#include "ns3/address-utils.h"

namespace ns3 {

class producer: public Object {

public:
	/**
	* \brief Get the type ID.
	* \return the object TypeId
	*/
	static TypeId GetTypeId (void);

	producer ();

	~producer();


	/*
	 * Producer received content from application layer.
	 * This will cause the Producer to create a Manifest and send it to the right consumers.
	 */
	void ReceiveContent(int size, Address to);


	/*
	 * Used by Producer to notify ITP of a packet that need to be transmitted
	 */
	void SetSendCallback (Callback<void, Ptr<Packet>, Address  >);
	Callback<void, Ptr<Packet>, Address> m_sendData;


	/*
	 * Producer sends a Manifest that describes:
	 * 1. How may Interest need to used to retrieve this CO
	 * 2. Name of the CO
	 * 3. Size of the Data packet for each Interest
	 * TODO: Support multiple COs and reliable of manifest
	 */
	void SendManifest(int size, Address to);


	/*
	 * Respond to an Interests based on the CO's name and size of the data packet
	 * TODO: Support multiple COs
	 */
	void OnInterest(Ptr<Packet> pkt, Address from);

	/*
	 * Set the payload size for data packets.
	 * We assuming we know the right MTU value in our network, which means we don't need fragmentation
	 */
	void SetPayloadSize(uint32_t size);

	/*
	 * Return payload size of data packets
	 */
	uint32_t GetPayloadSize();


private:
	uint32_t m_payloadSize;
	uint32_t m_sum;

};

}

#endif /* IPRODUCER_H */
