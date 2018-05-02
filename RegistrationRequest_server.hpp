/*
 * RegistrationRequest_server.hpp
 *
 *  Created on: Apr 28, 2018
 *      Author: artpad
 */

#ifndef REGISTRATIONREQUEST_SERVER_HPP_
#define REGISTRATIONREQUEST_SERVER_HPP_

#include <ndn-cxx/encoding/block.hpp>

namespace Server {

class RegistrationRequest {
public:
	static void handleRegistrationRequest(ndn::Block prefixListBlock, const char *clientAddress);

private:
	static ndn::Block m_prefixIPMappingList;
};

}

#endif /* REGISTRATIONREQUEST_SERVER_HPP_ */
