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
	static void registerClientInfoAndSendResponse(ndn::Block nodeInformation, int socketFileDescriptor, sockaddr_storage client, socklen_t clientLength);
	static void sendMappingListToClient(int socketFileDescriptor, sockaddr_storage client, socklen_t clientLength);

private:
	static void addNodeInformationToList(ndn::Block nodeInformation);
	static ndn::Block m_prefixIPMappingList;
};

}

#endif /* REGISTRATIONREQUEST_SERVER_HPP_ */
