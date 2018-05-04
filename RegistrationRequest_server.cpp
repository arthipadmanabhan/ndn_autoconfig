/*
 * RegistrationRequest_server.cpp
 *
 *  Created on: Apr 28, 2018
 *      Author: artpad
 */

#include "RegistrationRequest_server.hpp"
#include "AutoconfigConstants.hpp"

ndn::Block Server::RegistrationRequest::m_prefixIPMappingList = ndn::Block(AutoconfigConstants::BlockType::IPPrefixMappingList);

void Server::RegistrationRequest::addPrefixListToMappingList(ndn::Block prefixListBlock, sockaddr_in client) {
	// Create the block for this IP-prefix mapping
	ndn::Block mapping = ndn::Block(AutoconfigConstants::BlockType::IPPrefixMapping);

	// Create the block for the IP address
	const char *clientIPAddress = inet_ntoa(client.sin_addr);
	ndn::Buffer clientIPBuffer = ndn::Buffer(clientIPAddress, strlen(clientIPAddress) + 1);
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
	printf("for IP address: %s \n", clientIPAddress);
	printf("Number of entries in mapping list: %zu \n", m_prefixIPMappingList.elements_size());
}

void Server::RegistrationRequest::handleRegistrationRequest(ndn::Block prefixListBlock, int socketFileDescriptor, sockaddr_in client, socklen_t clientLength) {
	// Add the received block containing a list of prefixes and the IP it came from to the mapping
	addPrefixListToMappingList(prefixListBlock, client);

	// Encode and send the current IP-prefix mappings we know about to client
	m_prefixIPMappingList.encode();
	if (sendto(socketFileDescriptor, m_prefixIPMappingList.wire(), m_prefixIPMappingList.size(), 0, (struct sockaddr *)&client, clientLength) < 0) {
		printf("Error sending response to client \n");
	} else {
		printf("Sent response to client \n");
	}
}
