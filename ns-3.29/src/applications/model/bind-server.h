#ifndef BIND_SERVER_H
#define BIND_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/random-variable-stream.h"

#include "ns3/dns.h"
#include "ns3/dns-header.h"
#include "ns3/root-server.h"
#include "ns3/tld-server.h"
#include "ns3/isp-server.h"
#include "ns3/auth-server.h"

#define DNS_PORT 53
namespace ns3 {

class Socket;
class Packet;

class BindServer : public Application                                              {
	public:
		/**
		 * /brief the serve type definition*/
		enum ServerType 
		{
			LOCAL_SERVER = 0x01, //!< Local name server
			ROOT_SERVER = 0x02, //!< Root name server for each domain
			TLD_SERVER = 0x03, //!< Top Level Domain server
			ISP_SERVER = 0x04, //!< ISP's DNS server
			AUTH_SERVER = 0x05, //!< Authoritative name server
		};
		
		/**
		 * /brief Availability of the recursive queering*/
		enum RAType
		{
			RA_AVAILABLE = 0x01,
			RA_UNAVAILABLE = 0x02,
		};
		
		static TypeId GetTypeId (void);
	 	BindServer (void);
	  virtual ~BindServer ();
		void DoDispose (void)
		{
			m_nsCache.DoDispose ();
			m_socket = 0;
		}

		void AddZone (std::string zone_name,
									uint16_t ns_class, 
									uint16_t type, 
									uint32_t TTL,
									std::string rData);

		///IDNS///
		void AddManifestRecord (std::string content_name,
											uint16_t ns_class,
											uint16_t type,
											uint32_t TTL,
											std::string manifestRecord);


	private:
		virtual void StartApplication (void);
		virtual void StopApplication (void);

		void HandleQuery (Ptr<Socket> socket);
		void SendQuery (Ptr<Packet> requestRecord, Address toAddress);
		void ReplyQuery (Ptr<Packet> replyPacket, Address toAddress);

		void LocalServerService (Ptr<Packet> nsQuery, Address toAddress);
		void HandleRootServerReply (Ptr<Packet> nsReply, Address toAddress);
		void HandleTLDServerReply (Ptr<Packet> nsReply, Address toAddress);
		void HandleISPServerReply (Ptr<Packet> nsReply, Address toAddress);
		void HandleAuthServerReply (Ptr<Packet> reply, Address toAddress);


		typedef std::map <std::string, Address> QueryList; // FIXME Add an expiration timer
		typedef std::map <std::string, Address>::iterator QueryListI;
		typedef std::map <std::string, Address>::const_iterator QueryListCI;

		///IDNS///
		void HandleManifestQuery(Ptr<Packet> nsQuery, Address toAddress);
		void HandleDNSQuery(Ptr<Packet> nsQuery, Address toAddress);
		typedef std::map <std::string, std::string> ContentsList; // FIXME Map Contents to Hostnames
		ContentsList m_contentsQueryList; //used by the local dns server to set the right type number

		QueryList m_recursiveQueryList; //!< This is only needed when the 
																		//   the server supports recursive quering
																		// FIXME Add an expiration timer																		
		SRVTable m_nsCache; //!< the Cache for nameserver records
		Ipv4Address m_localAddress;
		Ipv4Mask m_netMask;
		RAType m_raType;
		ServerType m_serverType;
		Ptr<Socket> m_socket;
		Ipv4Address m_rootAddress; //!< Root ns's address. Only needed for the local Name server

		/// For IDNS ///
		SRVTable m_mrCache;
		Ptr<RootServer> m_rootServer;
		Ptr<TldServer> m_tldServer;
		Ptr<IspServer> m_ispServer;
		Ptr<AuthServer> m_authServer;

};
}
#endif /* BIND_SERVER_H */
