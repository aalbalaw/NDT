/* it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef ITP_H
#define ITP_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/udp-socket.h"
#include "ns3/address-utils.h"
#include "ns3/producer.h"
#include "ns3/consumer.h"

namespace ns3 {

//class Socket;
class Packet;

/**
 * \ingroup itp
 * \brief A Itp  client
 *
 * Every packet sent should be returned by the server and received here.
 */
class itp: public Object {
public:
	/**
	 * \brief Get the type ID.
	 * \return the object TypeId
	 */
	static TypeId GetTypeId(void);

	itp();

	~itp();

	void CreateSocket(uint16_t port, Ptr<Node> p);

	/**
	 * send over created udp socket
	 *
	 */

	/*
	 * API call used by the application to pass CO to the ITP layer
	 */
	void SendContent(Ptr<Packet> p, InetSocketAddress to);

	void SendContent(Ptr<Packet> p, Address to);

	/*
	 * Used by the producer and the consumer to send data over UDP
	 */
	void SendTo(Ptr<Packet> p, Address to);

	/*
	 * Used by ITP to receive packets from UDP.
	 * ITP checks the packet's type to know where to forward the packet
	 */
	void ReceveFrom(Ptr<Socket> socket);

	/*
	 * Used by application to set their receive function callback
	 * Their function's signature must be void receive(unit8_t* data, Address from)
	 * Ex: itpInstance->SetRecvCallback(MakeCallback(&HttpItpClient::HandleRead, this));
	 */
	void SetRecvCallback(Callback<void, Address>);

	Callback<void, Address> m_receiveData;

	/*
	 * Used by consumer to notify ITP layer when a whole content is retrieved
	 * TODO: Deliver the content size to the application. Maybe include real data
	 */
	void ReceivedContent(Address from);

	/*
	 * Used by the application to send an atomic packet that doesn't need
	 * a manifest to be constructed in the first place.
	 * This function will carry type number 5.
	 */
	void ForceSend(Ptr<Packet> p, InetSocketAddress to);

	/*
	 * Notify and deliver a content object to the application layer along with packet and the sender IP.
	 */
	void
	SetRecvContentCallback(Callback<void, Ptr<Packet>, Address>);

	Callback<void, Ptr<Packet>, Address> m_receivedContent;

	void CloseSocket();

	Ptr<Socket> m_socket; //!< Socket
	Ptr<Socket> m_Serversocket; //!< Socket
	Ptr<Node> m_node;
	Ptr<producer> m_producer;
	Ptr<consumer> m_consumer;
	TracedCallback<uint32_t> m_cWndTrace;

};
}

#endif /* ITP_CLIENT_H */
