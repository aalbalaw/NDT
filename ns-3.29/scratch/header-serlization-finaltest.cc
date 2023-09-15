/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
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

// Network topology
//
//       n0    n1   n2   n3
//       |     |    |    |
//       =================
//              LAN
//
// - UDP flows from n0 to n1 and back
// - DropTail queues
// - Tracing of queues and packet receptions to file "request-response.tr"
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4.h"
#include "ns3/manifest-header.h"
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RequestResponseExample");

std::string BufferToString1(uint8_t *buffer, uint32_t len) {
	std::ostringstream oss;
	//
	// Tell the stream to make hex characters, zero-filled
	//
	oss.setf(std::ios::hex, std::ios::basefield);
	oss.fill('0');

	//
	// Loop through the buffer, separating the two-digit-wide hex bytes
	// with a colon.
	//
	for (uint8_t i = 0; i < len; i++) {
		oss << ":" << std::setw(2) << (uint32_t) buffer[i];
	}
	return oss.str();
}

bool StringToBuffer1(std::string s, uint8_t *buffer) {
	//
	// If the string was made by our inverse function, the string length must
	// be a multiple of three characters in length.  Use this fact to do a
	// quick reasonableness test.
	//
	if ((s.length() % 3) != 0) {
		return false;
	}

	std::istringstream iss;
	iss.str(s);

	uint8_t n = 0;

	while (iss.good()) {
		//
		// The first character in the "triplet" we're working on is always the
		// the ':' separator.  Read that into a char and make sure we're skipping
		// what we think we're skipping.
		//
		char c;
		iss.read(&c, 1);
		if (c != ':') {
			return false;
		}

		//
		// And then read in the real bits and convert them.
		//
		uint32_t tmp;
		iss >> std::hex >> tmp;
		buffer[n] = tmp;
		n++;
	}
	return true;
}

std::string ManifestToString1(uint32_t numberOfChunk,
		Ipv4Address serverAddress) {

	ManifestHeader mHdr;
	mHdr.SetServerAddress(serverAddress);
	mHdr.SetNumberOfChunks(numberOfChunk);

	uint32_t size = mHdr.GetSerializedSize();
	Buffer buffer;
	buffer.AddAtStart(size);
	mHdr.Serialize(buffer.Begin());

	uint8_t *byteBuffer;
	byteBuffer = new uint8_t[size];
	buffer.CopyData(byteBuffer, size);

	std::string manifest = BufferToString1(byteBuffer, size);

	delete byteBuffer;
	return manifest;
}

bool StringToManifest1(ManifestHeader &mHdr, std::string ManifestAsString) {

	//Step7: Copy string to a byte array to construct
	uint8_t *byteBuffer;
	byteBuffer = new uint8_t[mHdr.GetSerializedSize()];

	bool status = StringToBuffer1(ManifestAsString, byteBuffer);

	//Step 8: Copy byte array to Buffer
	Buffer buffer;
	buffer.AddAtStart(mHdr.GetSerializedSize());
	Buffer::Iterator i = buffer.Begin();
	i.Write(byteBuffer, mHdr.GetSerializedSize());

	uint32_t deserialized = mHdr.Deserialize(buffer.Begin());

	return status;
}

int main(int argc, char *argv[]) {

	//uint8_t *buffer = new uint8_t () [packet->GetSize ()];
	//packet->CopyData (buffer, packet->GetSize ());

//	// Step 1: Create Manifest Header
//	ManifestHeader manifestHeader;
//	manifestHeader.SetServerAddress("10.0.0.1");
//	manifestHeader.SetNumberOfChunks(1500);
//
//	// Step 2: Serialize Manifest Header to a Buffer
//	uint32_t size = manifestHeader.GetSerializedSize();
//	Buffer m_buffer;
//	m_buffer.AddAtStart(size);
//	manifestHeader.Serialize(m_buffer.Begin());
//
//	// Step 3: Convert Buffer to a byte array
//	uint8_t *buffer;
//	buffer = new uint8_t[size];
//	m_buffer.CopyData(buffer, size);
//
//	// Step 4: Convert byte array to hexa string
//	std::string manifest = BufferToString1 (buffer, size);

	// Step 5: Add string as RData to resource record
	DNSHeader dnsHeader;
	dnsHeader.SetId(1);
	dnsHeader.SetQRbit(1);
	dnsHeader.SetOpcode(0);
	dnsHeader.SetAAbit(0);
	dnsHeader.SetTCbit(0);
	dnsHeader.SetRDbit(0);
	dnsHeader.SetRAbit(0);
	dnsHeader.SetRcode(0);
	dnsHeader.SetZcode();
	ResourceRecordHeader answer;
	answer.SetRData(ManifestToString1(2000, "10.1.1.1"));
	answer.SetType(202);
	dnsHeader.AddAnswer(answer);

	//Step 6: Get Resource Record from DNS header and check type
	std::list<ResourceRecordHeader> answerList;
	answerList = dnsHeader.GetAnswerList();
	std::string str = answerList.begin()->GetRData();

//	//Step7: Copy string to a byte array to construct
//	uint8_t *buffer2;
//	buffer2 = new uint8_t[12];
//	uint32_t *len;
//	StringToBuffer1(str, buffer2);
//
//	//Step 8: Copy byte array to Buffer
//	Buffer m_buffer2;
//	m_buffer2.AddAtStart(12);
//	Buffer::Iterator i = m_buffer2.Begin();
//	i.Write (buffer2, size);

	//Step 6: Deserlise the Buffer inside a new header and check source port number.
	ManifestHeader manifestHeader2;
	//uint32_t deserialized = manifestHeader2.Deserialize(m_buffer2.Begin());
	StringToManifest1(manifestHeader2, str);
	std::cout << "Server Address" << manifestHeader2.GetServerAddress()
			<< std::endl;
	std::cout << "number of Chunks " << manifestHeader2.GetNumberOfChunks()
			<< std::endl;

	std::string contentName;
	std::string tld;
	std::string hostName;
	std::string::size_type found = 0;
	bool foundTLDinCache = false;

	std::string qName = "http://www.example.com/index.html";

	// find the TLD of the query
	found = qName.find_last_of('/');
	contentName = qName.substr(found);
	std::cout<<"ContentName: "<<contentName<<std::endl;
	hostName = qName.substr(0, found);
	std::cout<<"HostName: "<<hostName<<std::endl;
	found =  hostName.find_last_of('.');
	tld = hostName.substr(found);
	std::cout<< " TLD: "<<tld<<std::endl;
	NS_LOG_INFO("Run Simulation.");
	Simulator::Run();
	Simulator::Stop(Seconds(10.0));
	NS_LOG_INFO("Done.");
}
