/*
 * AutoconfigConstants.hpp
 *
 *  Created on: Apr 30, 2018
 *      Author: artpad
 */

#ifndef AUTOCONFIGCONSTANTS_HPP_
#define AUTOCONFIGCONSTANTS_HPP_

namespace AutoconfigConstants {

enum BlockType {
	PrefixListToRegister = 128,
	Prefix = 129,
	IPAddress = 130,
	IPPrefixMapping = 131,
	IPPrefixMappingList = 132
};

// TODO: Save only RV name and get IP with DNS
static const char* RV_IPAddress = "192.168.0.103";

static const uint16_t portNumber = 53000;

static const int maxDatagramSize = 65507; // check this

}

#endif /* AUTOCONFIGCONSTANTS_HPP_ */