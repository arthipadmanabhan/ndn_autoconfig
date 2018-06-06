/*
 * PrefixRegistration.cpp
 *
 *  Created on: Apr 28, 2018
 *      Author: artpad
 */

#include <iostream>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <thread>
#include <chrono>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/tlv.hpp>
#include <ndn-cxx/encoding/tlv-nfd.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/net/face-uri.hpp>
#include <ndn-cxx/mgmt/nfd/control-command.hpp>
#include <ndn-cxx/mgmt/control-response.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include "AutoconfigConstants.hpp"
#include "RegistrationRequest_client.hpp"

static char localIPAddress[64];

/* Helpers to get prefixes from user and local IP and package them to send to RV */

// Create a prefix block from a string and add it to the registration request
ndn::Block prefixFromInputString(std::string inputString) {
	ndn::Buffer inputStringBuffer = ndn::Buffer(inputString.c_str(), inputString.length() + 1);
	ndn::ConstBufferPtr inputStringBufferPointer = std::make_shared<const ndn::Buffer>(inputStringBuffer);
	ndn::Block prefix = ndn::Block(AutoconfigConstants::BlockType::Prefix, inputStringBufferPointer);

	// For debugging
	printf("Adding prefix block: \n");
	printf("Type: %d \n", prefix.type());
	printf("Length: %zu \n", prefix.value_size());
	printf("Value: %s \n\n", prefix.value());

	return prefix;
}

// TODO: Fix this method!!!!
// This is an ugly hack that only works for Mac and just takes the IPv4 ethernet address (en0). It's not portable to other platforms
// and we need a proper way of telling which address actually corresponds to local network
ndn::Block retreiveLocalIPAddress() {
    struct ifaddrs *myaddrs, *ifa;
    void *in_addr;
    char buffer[64];
    if(getifaddrs(&myaddrs) == 0) {
    	for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr == NULL) {
				continue;
			}
			if (!(ifa->ifa_flags & IFF_UP)) {
				continue;
			}
			switch (ifa->ifa_addr->sa_family) {
				case AF_INET: {
					struct sockaddr_in *s4 = (struct sockaddr_in *)ifa->ifa_addr;
					in_addr = &s4->sin_addr;
					break;
				}
				case AF_INET6: {
					struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)ifa->ifa_addr;
					in_addr = &s6->sin6_addr;
					break;
				}
				default:
					continue;
			}
			if (inet_ntop(ifa->ifa_addr->sa_family, in_addr, buffer, sizeof(buffer))) {
				if(ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, "en0")==0) {
					break;
				}
			}
		}
    }
    freeifaddrs(myaddrs);
    if(strlen(buffer) != 0) {
    	strcpy(localIPAddress, buffer);
    }

    ndn::Buffer IPBuffer = ndn::Buffer(buffer, strlen(buffer) + 1);
	ndn::ConstBufferPtr IPBufferPointer = std::make_shared<const ndn::Buffer>(IPBuffer);
	ndn::Block IPBlock = ndn::Block(AutoconfigConstants::BlockType::IPAddress, IPBufferPointer);

	return IPBlock;
}


// Ask user to input prefixes
// TODO: Figure out a better way of getting prefixes (secured)
// Node information must contain both a list of prefixes (provided by the application and the local IP address)
ndn::Block getNodeInformation() {
	ndn::Block prefixList = ndn::Block(AutoconfigConstants::BlockType::PrefixList);

	bool emptyInput = false;
	printf("Preparing for registration request. Enter prefixes one at a time, or press Enter be finished \n");
	while (!emptyInput) {
		std::string inputString;
		std::getline(std::cin, inputString);
		if (inputString.length() != 0) {
			ndn::Block prefix = prefixFromInputString(inputString);
			prefixList.push_back(prefix);
		} else {
			emptyInput = true;
		}
	}
	ndn::Block nodeInformation = ndn::Block(AutoconfigConstants::BlockType::IPPrefixMapping);
	nodeInformation.push_back(prefixList);
	ndn::Block IPAddress = retreiveLocalIPAddress();
	nodeInformation.push_back(IPAddress);

	// Encode the list of prefixes and IP address
	nodeInformation.encode();
	return nodeInformation;
}

/* Helper to determine address type of IP and return uri */
std::string uriFromIPAddress(const char *IPAddress) {
	struct addrinfo hints, *info;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;

	const char *addressString = IPAddress;
	std::string uriString;
	bool isIPv6 = false;
	if (getaddrinfo(IPAddress, 0, &hints, &info) == 0) {
		if (info->ai_family == AF_INET6) {
			struct sockaddr_in6 *addr = (struct sockaddr_in6*)(info->ai_addr);
			if (IN6_IS_ADDR_V4MAPPED(&addr->sin6_addr)) {
				struct in_addr IPv4address = { *(const in_addr_t *)(addr->sin6_addr.s6_addr+12)};
				addressString = inet_ntoa(IPv4address);
			} else {
				isIPv6 = true;
			}
		}

		ndn::FaceUri uri;
		if (isIPv6) {
			boost::asio::ip::udp::endpoint endpoint6(boost::asio::ip::address_v6::from_string(addressString), AutoconfigConstants::NDNPort);
			uri = ndn::FaceUri(endpoint6);
		} else {
			boost::asio::ip::udp::endpoint endpoint4(boost::asio::ip::address_v4::from_string(addressString), AutoconfigConstants::NDNPort);
			uri = ndn::FaceUri(endpoint4);
		}
		uriString = uri.toString();

		freeaddrinfo(info);
	}
	return uriString;
}

/* Callbacks for adding face and fib entry */

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
	if (statusCode == 409) {
		printf("Face already exists for this Uri: ");
		ndn::nfd::ControlParameters parameters;
		try {
		    parameters.wireDecode(response.getBody());
		    if (parameters.hasFaceId()) {
				unsigned long long faceId = parameters.getFaceId();
				printf("Id = %llu \n", faceId);
		    } else {
		    	// assert
		    }
		}
		catch (const ndn::tlv::Error& e) {
			printf("Error processing response \n");
		}
	} else {
		printf("Creating face failed with code %u \n", statusCode);
	}
}

/* Methods to receive response from RV and handle it */

// For each mapping (containing one IP address and a list of prefixes it serves), add a face for the IP,
// which if successful, triggers adding fib entries
void storeMapping(ndn::Block mappingListBlock) {
	mappingListBlock.parse();
	for (auto mappingIterator = mappingListBlock.elements_begin(); mappingIterator < mappingListBlock.elements_end(); mappingIterator++) {
		mappingIterator->parse();

		// Get the IP address and prefix list for this node
		ndn::Block IPBlock = mappingIterator->get(AutoconfigConstants::IPAddress);
		const char *IPAddress = reinterpret_cast<const char*>(IPBlock.value());
		if (strcmp(IPAddress, localIPAddress) != 0) {
			ndn::Block prefixList = mappingIterator->get(AutoconfigConstants::PrefixList);

			// Create a face for this IP address
			ndn::Face faceToNFD;
			ndn::KeyChain appKeyChain;
			ndn::nfd::Controller controller(faceToNFD, appKeyChain);

			ndn::nfd::ControlParameters parameters;
			parameters.setUri(uriFromIPAddress(IPAddress).c_str());

			controller.start<ndn::nfd::FaceCreateCommand>(parameters, std::bind(&OnFaceCreateSuccess, _1, prefixList), &OnFaceCreateFailure);
			faceToNFD.processEvents();
		}
	}
}

void receiveAndStoreServerResponse(int socketFileDescriptor, sockaddr *server) {
	unsigned char* receivingBuffer = new unsigned char[AutoconfigConstants::maxDatagramSize];
	socklen_t serverLength = sizeof(server);

	int numberBytesReceived = recvfrom(socketFileDescriptor, receivingBuffer, AutoconfigConstants::maxDatagramSize, 0, server, &serverLength);
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

/* Methods to send info to RV */

void sendUpdateRequestToServer(int socketFileDescriptor, sockaddr *server, socklen_t serverLength) {
	ndn::Block updateRequestBlock = ndn::Block(AutoconfigConstants::BlockType::UpdateRequest);
	updateRequestBlock.encode();
	if (sendto(socketFileDescriptor, updateRequestBlock.wire(), updateRequestBlock.size(), 0, server, serverLength) < 0) {
		printf("Error sending update request to RV \n");
	} else {
		printf("Sent update request to RV \n");
	}
}

void sendPrefixListToServer(int socketFileDescriptor, ndn::Block prefixListToRegister, sockaddr *server, socklen_t serverLength) {
	if (sendto(socketFileDescriptor, prefixListToRegister.wire(), prefixListToRegister.size(), 0, server, serverLength) < 0) {
		printf("Error sending to RV \n");
	} else {
		printf("Sent prefix list to RV \n");
	}
}

/* Manages registration, response retrieval, and retransmit/refresh */
void Client::RegistrationRequest::registerPrefixesAndReceiveResponse() {
	// Ask user for prefixes
	ndn::Block nodeInformation = getNodeInformation();

	// Set up socket to communicate with server
	int socketFileDescriptor, addressInfo;
	struct addrinfo hints, *serverInfo;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; /* we should be able to support both IPv4 and IPv6 */
	hints.ai_socktype = SOCK_DGRAM;
	if ((addressInfo = getaddrinfo(AutoconfigConstants::RV_IPAddress, AutoconfigConstants::portNumber, &hints, &serverInfo)) == 0) {
		socketFileDescriptor = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

		// Set up for retransmit timer for prefix registration
		timeval tv;
		tv.tv_sec = AutoconfigConstants::timeoutSeconds;
		tv.tv_usec = AutoconfigConstants::timeoutMilliseconds;

		fd_set fdRead;
		FD_ZERO(&fdRead);
		FD_SET(socketFileDescriptor, &fdRead);

		// Send and retransmit if no response before timeout
		sendPrefixListToServer(socketFileDescriptor, nodeInformation, serverInfo->ai_addr, serverInfo->ai_addrlen);
		int numberOfRegistrationRetries = 0;
		while (select(socketFileDescriptor + 1, &fdRead, NULL, NULL, &tv) <= 0 && numberOfRegistrationRetries < AutoconfigConstants::maxRetries) {
			printf("Received no response. Retransmitting prefix list \n");
			sendPrefixListToServer(socketFileDescriptor, nodeInformation, serverInfo->ai_addr, serverInfo->ai_addrlen);

			numberOfRegistrationRetries++;

			// Reset timeval because the call to select above can change it
			tv.tv_sec = AutoconfigConstants::timeoutSeconds;
			tv.tv_usec = AutoconfigConstants::timeoutMilliseconds;
		}
		if (numberOfRegistrationRetries < AutoconfigConstants::maxRetries) {
			// Now we know there is a response, so receive it
			receiveAndStoreServerResponse(socketFileDescriptor, serverInfo->ai_addr);
		} else {
			printf("Retransmitted max number of times and received no response \n");
		}

		// Start refresh timer
		int numberOfRequestUpdateRetries = 0;
		while (true) {
			std::this_thread::sleep_for(std::chrono::seconds(AutoconfigConstants::refreshPeriodSeconds));
			sendUpdateRequestToServer(socketFileDescriptor, serverInfo->ai_addr, serverInfo->ai_addrlen);
			while (select(socketFileDescriptor + 1, &fdRead, NULL, NULL, &tv) <= 0 && numberOfRequestUpdateRetries < AutoconfigConstants::maxRetries) {
				printf("Received no response. Retransmitting request update\n");
				sendUpdateRequestToServer(socketFileDescriptor, serverInfo->ai_addr, serverInfo->ai_addrlen);
				numberOfRequestUpdateRetries++;
				tv.tv_sec = AutoconfigConstants::timeoutSeconds;
				tv.tv_usec = AutoconfigConstants::timeoutMilliseconds;
			}
			if (numberOfRequestUpdateRetries < AutoconfigConstants::maxRetries) {
				receiveAndStoreServerResponse(socketFileDescriptor, serverInfo->ai_addr);
			} else {
				printf("Retransmitted update request max number of times and received no response \n");
			}
			numberOfRequestUpdateRetries = 0;
		}

		freeaddrinfo(serverInfo);
	}
}

