# ndn_autoconfig
Allows NDN nodes to interconnect in networks with communication restrictions

This set of client-server applications is designed to allow NDN nodes to discover each other when the local network doesn't allow multicast. This is done with a rendezvous server (RV) that stores information about all nodes that have registered with it and distributes this information to all other nodes upon request.

Current status: The client can currently send prefixes under which it publishes to the server, and server will store the IP address – prefix list mapping and send back mappings for all nodes it knows about. When the client receives this mapping (it will retransmit up to 3 times if it doesn't), it will create a face for each IP address received and add each prefix to its fib with the corresponding IP as next hop. The client will also ask for an updated list from RV periodically. 
 
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
