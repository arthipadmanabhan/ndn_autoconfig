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
	IPPrefixMappingList = 132,
	UpdateRequest = 133
};

// RV constants
static const char* RV_IPAddress = "192.168.0.103"; /* This can be changed to RV name once we have that */
static const char* portNumber = "53000";

// max UDP datagram
static const int maxDatagramSize = 65507; // check this

// Registration/refresh retransmit constants
static const int timeoutSeconds = 2;
static const int timeoutMilliseconds = 0;
static const int maxRetries = 3;

// Refresh timer
static const int refreshPeriodSeconds = 30;

// NDN constant
static const unsigned short NDNPort = 6363;
}

#endif /* AUTOCONFIGCONSTANTS_HPP_ */
