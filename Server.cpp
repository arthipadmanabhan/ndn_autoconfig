/*
 * Server.cpp
 *
 *  Created on: Apr 26, 2018
 *      Author: artpad
 */
#include <ndn-cxx/encoding/block.hpp>
#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cassert>
#include "RegistrationRequest_server.hpp"
#include "AutoconfigConstants.hpp"

int main() {
	// Initialize sockets and bind server info
	int socketFileDescriptor = socket(AF_INET,SOCK_DGRAM,0);
	struct sockaddr_in server,client;

	server.sin_family = AF_INET;
	server.sin_port = htons(AutoconfigConstants::portNumber);
	server.sin_addr.s_addr = INADDR_ANY;
	bind(socketFileDescriptor, (struct sockaddr *)&server, sizeof(server));

	// Always be ready to receive messages from clients
	unsigned char* receivingBuffer = new unsigned char[AutoconfigConstants::maxDatagramSize];
	socklen_t clientLength = sizeof(client);
	printf("Ready to receive \n");
	while (true) {
		int numberBytesReceived = recvfrom(socketFileDescriptor, receivingBuffer, AutoconfigConstants::maxDatagramSize, 0, (struct sockaddr *)&client, &clientLength);
		// recvfrom returns -1 on failure, number of bytes read otherwise
		if (numberBytesReceived < 0) {
			printf("Error reading from socket");
		} else {
			// Tuple contains bool (whether parsing the buffer was successful) and the resulting block if it was
			std::tuple<bool, ndn::Block> parsingResult = ndn::Block::fromBuffer(receivingBuffer, numberBytesReceived);
			if (std::get<0>(parsingResult)) {
				ndn::Block receivedBlock = std::get<1>(parsingResult);
				AutoconfigConstants::BlockType blockType = static_cast<AutoconfigConstants::BlockType>(receivedBlock.type());
				switch(blockType) {
					case AutoconfigConstants::PrefixListToRegister:
						Server::RegistrationRequest::registerClientInfoAndSendResponse(receivedBlock, socketFileDescriptor, client, clientLength);
						break;
					case AutoconfigConstants::UpdateRequest:
						Server::RegistrationRequest::sendMappingListToClient(socketFileDescriptor, client, clientLength);
						break;
					default:
						break;
				}
			}
			else {
				printf("Unable to parse received bytes \n");
			}
		}
		receivingBuffer[0] = '\0';
	}
}


