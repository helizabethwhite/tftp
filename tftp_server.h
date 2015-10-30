#include <sys/socket.h>
#include <iostream>

// This class defines a TFTP server!
class TFTP_Server {

    TFTP_Server(int port);
    ~TFTP_Server();
    
    SOCKET server_socket;
    struct sockaddr_in server_address;
    int listener;
    int serverPort;
    
	bool sendPacket(TFTP_Packet* packet, TFTP_Client* client);
	bool receivePacket(TFTP_Packet* packet, ServerClient* client);
};