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
ndn::Block addPrefixBlock(std::string inputString, ndn::Block registrationRequestBlock) {
	ndn::Buffer inputStringBuffer = ndn::Buffer(inputString.c_str(), inputString.length() + 1);
	ndn::ConstBufferPtr inputStringBufferPointer = std::make_shared<const ndn::Buffer>(inputStringBuffer);
	ndn::Block prefixBlock = ndn::Block(AutoconfigConstants::BlockType::Prefix, inputStringBufferPointer);

	registrationRequestBlock.push_back(prefixBlock);

	// For debugging
	printf("Adding prefix block: \n");
	printf("Type: %d \n", prefixBlock.type());
	printf("Length: %zu \n", prefixBlock.value_size());
	printf("Value: %s \n", prefixBlock.value());

	return registrationRequestBlock;
}

void Client::RegistrationRequest::registerPrefixes()
{
	ndn::Block prefixListBlock = ndn::Block(AutoconfigConstants::BlockType::PrefixListToRegister);

	// Ask user to input prefixes
	// TODO: Figure out a better way of getting prefixes (secured)
	bool emptyInput = false;
	printf("Preparing for registration request. Enter prefixes one at a time, or press Enter be finished \n");
	while (!emptyInput) {
		std::string inputString;
		std::getline(std::cin, inputString);
		if (inputString.length() != 0) {
			prefixListBlock = addPrefixBlock(inputString, prefixListBlock);
		}
		else {
			emptyInput = true;
		}
	}

	// Encode the list of prefixes and send to RV
	prefixListBlock.encode();
	int sockfd = socket(AF_INET,SOCK_DGRAM,0);
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(AutoconfigConstants::portNumber);
	server.sin_addr.s_addr = inet_addr(AutoconfigConstants::RV_IPAddress);
	if (sendto(sockfd, prefixListBlock.wire(), prefixListBlock.size(), 0, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		printf("Error sending to RV \n");
	}
	else {
		printf("Sent prefix list to RV \n");
	}
}


