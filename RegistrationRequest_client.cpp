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
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/tlv.hpp>
#include <ndn-cxx/encoding/tlv-nfd.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/mgmt/nfd/control-command.hpp>
#include <ndn-cxx/mgmt/control-response.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include "AutoconfigConstants.hpp"
#include "RegistrationRequest_client.hpp"

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

// TODO: This needs to support IPv6 addresses as well
const char* uriFromIPAddress(const char *IPAddress) {
	const char *scheme = "udp4://";
	const char *port = ":6363";
	int length = strlen(IPAddress) + strlen(scheme) + strlen(port);
	char uriArray[length];
	strcpy(uriArray, scheme);
	strcat(uriArray, IPAddress);
	strcat(uriArray, port);
	const char *Uri = uriArray;
	return Uri;
}

void OnFibAddNextHopSuccess (const ndn::nfd::ControlParameters parameters) {
	printf("Adding next hop in fib succeeded \n");
}

void OnFibAddNextHopFailure (const ndn::mgmt::ControlResponse response) {
	printf("Adding next hop in fib failed \n");
}

// Get faceId from response and add fib entries for each prefix this IP serves
void OnFaceCreateSuccess (const ndn::nfd::ControlParameters parameters, ndn::Block prefixList) {
	if (parameters.hasFaceId()) {
		unsigned long long faceId = parameters.getFaceId();
		printf("New face created. Id: %llu \n", faceId);

		prefixList.parse();
		for (auto prefixIterator = prefixList.elements_begin(); prefixIterator < prefixList.elements_end(); prefixIterator++) {
			// Control parameters include name and FaceId
			ndn::nfd::ControlParameters parameters;
			parameters.setFaceId(faceId);
			ndn::Name prefixName = ndn::Name(reinterpret_cast<const char*>(prefixIterator->value()));
			parameters.setName(prefixName);

			ndn::Face faceToNFD;
			ndn::KeyChain appKeyChain;
			ndn::nfd::Controller controller(faceToNFD, appKeyChain);
			controller.start<ndn::nfd::FibAddNextHopCommand>(parameters, &OnFibAddNextHopSuccess, &OnFibAddNextHopFailure);
			faceToNFD.processEvents();
		}
	}
	else {
		// Assert
	}
}

void OnFaceCreateFailure (const ndn::mgmt::ControlResponse response) {
	unsigned int statusCode = response.getCode();
	printf("Creating face failed with code %u \n", statusCode);
}

// For each mapping (containing one IP address and a list of prefixes it serves), add a face for the IP,
// which if successful, triggers adding fib entries
void storeMapping(ndn::Block mappingListBlock) {
	mappingListBlock.parse();
	for (auto mappingIterator = mappingListBlock.elements_begin(); mappingIterator < mappingListBlock.elements_end(); mappingIterator++) {
		mappingIterator->parse();

		// Get the IP address and prefix list for this node
		ndn::Block IPBlock = mappingIterator->get(AutoconfigConstants::IPAddress);
		const char *IPAddress = reinterpret_cast<const char*>(IPBlock.value());
		ndn::Block prefixList = mappingIterator->get(AutoconfigConstants::PrefixListToRegister);

		// Create a face for this IP address
		ndn::Face faceToNFD;
		ndn::KeyChain appKeyChain;
		ndn::nfd::Controller controller(faceToNFD, appKeyChain);

		ndn::nfd::ControlParameters parameters;
		parameters.setUri(uriFromIPAddress(IPAddress));

		controller.start<ndn::nfd::FaceCreateCommand>(parameters, std::bind(&OnFaceCreateSuccess, _1, prefixList), &OnFaceCreateFailure);
		faceToNFD.processEvents();
	}
}

// Sends prefixes, receives response, and sends response to be stored in local NFD
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
					storeMapping(receivedBlock);
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

