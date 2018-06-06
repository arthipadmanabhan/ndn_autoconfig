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

void Server::RegistrationRequest::addNodeInformationToList(ndn::Block nodeInformation) {
	m_prefixIPMappingList.push_back(nodeInformation);

	// For debugging and verifying that the block has the necessary parts (IP + prefix list)
	nodeInformation.parse();
	printf("Registered prefix list: \n");
	ndn::Block prefixList = nodeInformation.get(AutoconfigConstants::PrefixList);
	prefixList.parse();
	for (auto iterator = prefixList.elements_begin(); iterator < prefixList.elements_end(); iterator++) {
		printf("%s \n", iterator->value());
	}
	ndn::Block IP = nodeInformation.get(AutoconfigConstants::IPAddress);
	const char *IPAddress = reinterpret_cast<const char*>(IP.value());
	printf("for IP address: %s \n", IPAddress);
	printf("Number of prefix registration calls made to RV: %zu \n", m_prefixIPMappingList.elements_size());
	// End debug
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
void Server::RegistrationRequest::registerClientInfoAndSendResponse(ndn::Block nodeInformation, int socketFileDescriptor, sockaddr_storage client, socklen_t clientLength) {
	addNodeInformationToList(nodeInformation);
	sendMappingListToClient(socketFileDescriptor, client, clientLength);
}
