/*
 * PrefixRegistration.cpp
 *
 *  Created on: Apr 28, 2018
 *      Author: artpad
 */

#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "RegistrationRequest_client.hpp"
#include <ndn-cxx/encoding/block.hpp>
#include "AutoconfigConstants.hpp"

// Create a prefix block out of the input string and include it in the value of the registration request block
ndn::Block addPrefixBlock(std::string inputString, ndn::Block prefixListBlock) {
	ndn::Buffer inputStringBuffer = ndn::Buffer(inputString.c_str(), inputString.length() + 1);
	ndn::ConstBufferPtr inputStringBufferPointer = std::make_shared<const ndn::Buffer>(inputStringBuffer);
	ndn::Block prefixBlock = ndn::Block(AutoconfigConstants::BlockType::Prefix, inputStringBufferPointer);

	prefixListBlock.push_back(prefixBlock);

	// For debugging
	printf("Adding prefix block: \n");
	printf("Type: %d \n", prefixBlock.type());
	printf("Length: %zu \n", prefixBlock.value_size());
	printf("Value: %s \n\n", prefixBlock.value());

	return prefixListBlock;
}

// Ask user to input prefixes
// TODO: Figure out a better way of getting prefixes (secured)
ndn::Block getPrefixesToRegister() {
	ndn::Block prefixListBlock = ndn::Block(AutoconfigConstants::BlockType::PrefixListToRegister);

	bool emptyInput = false;
	printf("Preparing for registration request. Enter prefixes one at a time, or press Enter be finished \n");
	while (!emptyInput) {
		std::string inputString;
		std::getline(std::cin, inputString);
		if (inputString.length() != 0) {
			prefixListBlock = addPrefixBlock(inputString, prefixListBlock);
		} else {
			emptyInput = true;
		}
	}
	// Encode the list of prefixes
	prefixListBlock.encode();
	return prefixListBlock;
}

// Just prints IP addresses and prefixes they each serve
// TODO: figure out how to store these directly in FIB
void parseMappingListBlock(ndn::Block receivedBlock) {
	receivedBlock.parse();
	for (auto mappingIterator = receivedBlock.elements_begin(); mappingIterator < receivedBlock.elements_end(); mappingIterator++) {
		mappingIterator->parse();
		ndn::Block IPBlock = mappingIterator->get(AutoconfigConstants::IPAddress);
		printf("For IP address: %s, registered prefixes are: \n", IPBlock.value());
		ndn::Block prefixListBlock = mappingIterator->get(AutoconfigConstants::PrefixListToRegister);
		prefixListBlock.parse();
		for (auto iterator = prefixListBlock.elements_begin(); iterator < prefixListBlock.elements_end(); iterator++) {
			printf("%s \n", iterator->value());
		}
	}
}

void Client::RegistrationRequest::registerPrefixesAndReceiveResponse() {
	// Ask user for prefixes
	ndn::Block prefixListToRegister = getPrefixesToRegister();

	// Send to server
	int socketFileDescriptor = socket(AF_INET,SOCK_DGRAM,0);
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(AutoconfigConstants::portNumber);
	server.sin_addr.s_addr = inet_addr(AutoconfigConstants::RV_IPAddress);

	// TODO: Add retransmit timer logic to send/receive
	if (sendto(socketFileDescriptor, prefixListToRegister.wire(), prefixListToRegister.size(), 0, (struct sockaddr *)&server, sizeof(server)) < 0) {
		printf("Error sending to RV \n");
	} else {
		printf("Sent prefix list to RV \n");
	}

	// Receive response from server
	unsigned char* receivingBuffer = new unsigned char[AutoconfigConstants::maxDatagramSize];
	socklen_t serverLength = sizeof(server);
	int numberBytesReceived = recvfrom(socketFileDescriptor, receivingBuffer, AutoconfigConstants::maxDatagramSize, 0, (struct sockaddr *)&server, &serverLength);
	// recvfrom returns -1 on failure, number of bytes read otherwise
	if (numberBytesReceived < 0) {
		printf("Error receiving response from server");
	} else {
		printf("Received acknowledgement and information about other nodes \n");

		// Tuple contains bool (whether parsing the buffer was successful) and the resulting block if it was
		std::tuple<bool, ndn::Block> parsingResult = ndn::Block::fromBuffer(receivingBuffer, numberBytesReceived);
		if (std::get<0>(parsingResult)) {
			ndn::Block receivedBlock = std::get<1>(parsingResult);
			AutoconfigConstants::BlockType blockType = static_cast<AutoconfigConstants::BlockType>(receivedBlock.type());
			switch(blockType) {
				case AutoconfigConstants::IPPrefixMappingList:
					parseMappingListBlock(receivedBlock);
					break;
				default:
					break;
			}
		}
		else {
			printf("Unable to parse received bytes \n");
		}
	}
}


