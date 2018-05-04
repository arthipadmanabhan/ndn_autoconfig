/*
 * RegistrationRequest_server.hpp
 *
 *  Created on: Apr 28, 2018
 *      Author: artpad
 */

#ifndef REGISTRATIONREQUEST_SERVER_HPP_
#define REGISTRATIONREQUEST_SERVER_HPP_

#include <ndn-cxx/encoding/block.hpp>
#include <arpa/inet.h>

namespace Server {

class RegistrationRequest {
public:
	static void handleRegistrationRequest(ndn::Block prefixListBlock, int socketFileDescriptor, sockaddr_in client, socklen_t clientLength);

private:
	static void addPrefixListToMappingList(ndn::Block prefixListBlock, sockaddr_in client);
	static ndn::Block m_prefixIPMappingList;
};

}

#endif /* REGISTRATIONREQUEST_SERVER_HPP_ */
