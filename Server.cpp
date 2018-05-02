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

const int maxDatagramSize = 65507; // check this

int main() {
	// Initialize sockets and bind server info
	int sockfd = socket(AF_INET,SOCK_DGRAM,0);
	struct sockaddr_in server,client;

	server.sin_family = AF_INET;
	server.sin_port = htons(53000);
	server.sin_addr.s_addr = INADDR_ANY;
	bind(sockfd, (struct sockaddr *)&server, sizeof(server));

	// Always be ready to receive messages from clients
	unsigned char* receivingBuffer = new unsigned char[maxDatagramSize];
	socklen_t clientLength = sizeof(client);
	printf("Ready to receive \n");
	while (true) {
		int numberBytesReceived = recvfrom(sockfd, receivingBuffer, maxDatagramSize, 0, (struct sockaddr *)&client, &clientLength);
		// recvfrom returns -1 on failure, number of bytes read otherwise
		if (numberBytesReceived < 0) {
			printf("Error reading from socket");
		} else {
			const char *clientIPAddress = inet_ntoa(client.sin_addr);

			// Tuple contains bool (whether parsing the buffer was successful) and the resulting block if it was
			std::tuple<bool, ndn::Block> parsingResult = ndn::Block::fromBuffer(receivingBuffer, numberBytesReceived);
			if (std::get<0>(parsingResult)) {
				ndn::Block receivedBlock = std::get<1>(parsingResult);
				AutoconfigConstants::BlockType blockType = static_cast<AutoconfigConstants::BlockType>(receivedBlock.type());
				switch(blockType) {
					case AutoconfigConstants::PrefixListToRegister:
						Server::RegistrationRequest::handleRegistrationRequest(receivedBlock, clientIPAddress);
						break;
					default:
						break;
				}
			}
		}
		receivingBuffer[0] = '\0';
	}
}


