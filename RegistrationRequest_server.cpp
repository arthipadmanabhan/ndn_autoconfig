/*
 * RegistrationRequest_server.cpp
 *
 *  Created on: Apr 28, 2018
 *      Author: artpad
 */

#include "RegistrationRequest_server.hpp"
#include "AutoconfigConstants.hpp"
#include <netdb.h>

ndn::Block Server::RegistrationRequest::m_prefixIPMappingList = ndn::Block(AutoconfigConstants::BlockType::IPPrefixMappingList);

void Server::RegistrationRequest::addPrefixListToMappingList(ndn::Block prefixListBlock, sockaddr_storage client) {
	// Create the block for this IP-prefix mapping
	ndn::Block mapping = ndn::Block(AutoconfigConstants::BlockType::IPPrefixMapping);

	// Create the block for the IP address
	char hoststr[NI_MAXHOST];
	if (getnameinfo((struct sockaddr *)&client, client.ss_len, hoststr, sizeof(hoststr), NULL, 0, NI_NUMERICHOST | NI_NUMERICSERV | NI_DGRAM) == 0) {
		ndn::Buffer clientIPBuffer = ndn::Buffer(hoststr, strlen(hoststr) + 1);
		ndn::ConstBufferPtr clientIPBufferPtr = std::make_shared<const ndn::Buffer>(clientIPBuffer);
		ndn::Block IPAddressToAdd = ndn::Block(AutoconfigConstants::BlockType::IPAddress, clientIPBufferPtr);

		// Add the IP and registration block, which is a list of prefixes, into the mapping
		mapping.push_back(prefixListBlock);
		mapping.push_back(IPAddressToAdd);

		// Add this mapping to the block containing all mappings
		m_prefixIPMappingList.push_back(mapping);

		// For debugging (and maybe asserting for proper block type in the future?)
		prefixListBlock.parse();
		printf("Registered prefix list: \n");
		for (auto iterator = prefixListBlock.elements_begin(); iterator < prefixListBlock.elements_end(); iterator++) {
			printf("%s \n", iterator->value());
		}
		printf("for IP address: %s \n", hoststr);
		printf("Number of prefix registration calls made to RV: %zu \n", m_prefixIPMappingList.elements_size());
	}
}

// Encode and send the current IP-prefix mappings we know about to client
void Server::RegistrationRequest::sendMappingListToClient(int socketFileDescriptor, sockaddr_storage client, socklen_t clientLength) {
	m_prefixIPMappingList.encode();
	if (sendto(socketFileDescriptor, m_prefixIPMappingList.wire(), m_prefixIPMappingList.size(), 0, (struct sockaddr *)&client, clientLength) < 0) {
		printf("Error sending response to client \n");
	} else {
		printf("Sent response to client \n");
	}
}

// Add the received block containing a list of prefixes and the IP it came from to the mapping
// and send back info about all clients we know about
void Server::RegistrationRequest::registerClientInfoAndSendResponse(ndn::Block prefixListBlock, int socketFileDescriptor, sockaddr_storage client, socklen_t clientLength) {
	addPrefixListToMappingList(prefixListBlock, client);
	sendMappingListToClient(socketFileDescriptor, client, clientLength);
}
