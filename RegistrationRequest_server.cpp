/*
 * RegistrationRequest_server.cpp
 *
 *  Created on: Apr 28, 2018
 *      Author: artpad
 */

#include "RegistrationRequest_server.hpp"
#include "AutoconfigConstants.hpp"

ndn::Block Server::RegistrationRequest::m_prefixIPMappingList = ndn::Block(AutoconfigConstants::BlockType::IPPrefixMappingList);

void Server::RegistrationRequest::handleRegistrationRequest(ndn::Block prefixListBlock, const char *clientIPAddress) {
	// Create the block for this IP-prefix mapping
	ndn::Block mapping = ndn::Block(AutoconfigConstants::BlockType::IPPrefixMapping);

	// Create the block for the IP address
	ndn::Buffer myBuffer = ndn::Buffer(clientIPAddress, strlen(clientIPAddress));
	ndn::ConstBufferPtr cbp = std::make_shared<const ndn::Buffer>(myBuffer);
	ndn::Block IPAddressToAdd = ndn::Block(AutoconfigConstants::BlockType::IPAddress, cbp);

	// Add the IP and registration block, which is a list of prefixes, into the mapping
	mapping.push_back(IPAddressToAdd);
	mapping.push_back(prefixListBlock);

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
