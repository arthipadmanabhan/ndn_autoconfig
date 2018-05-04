# ndn_autoconfig
Allows NDN nodes to interconnect in networks with communication restrictions

This set of client-server applications is designed to allow NDN nodes to discover each other when the local network either doesn’t allow multicast or allows neither multicast nor unicast. 

Current status: In the case where unicast is allowed but multicast is not, RV stores IP address - prefix list mappings and can distribute this list to any other nodes in the network so they can communicate directly going forward. We started by focusing on this case: the client can currently send prefixes under which it publishes to the server, and server will store the IP address – prefix list mapping and send back mappings for all nodes it knows about.  

The server being used for testing this project is icear, and its IP address is reachable if you are connected to the icear LAN. We are currently assuming that client and server are on the same LAN.

To test:
Prerequisite: ndn-cxx must be installed.
To compile and run client code:

g++ Client.cpp RegistrationRequest_client.cpp -std=c++11 `pkg-config --cflags libndn-cxx` `pkg-config --libs libndn-cxx` -o Client

./Client

To compile and run server code:

g++ Server.cpp RegistrationRequest_server.cpp -std=c++11 `pkg-config --cflags libndn-cxx` `pkg-config --libs libndn-cxx` -o Server

./Server

Note: server code is run on icear with permissions. The app can be tested on other networks by changing RV_IPAddress in AutoconfigConstants.hpp to the IP of the machine where the server code is being compiled. 
