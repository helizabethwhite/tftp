#include <iostream>

// This class defines a TFTP server client, which
// defaults to operate on port 69
class TFTP_Client {
    protected:
    
        char* server_ip = "127.0.0.1";      // default is localhost
        int	server_port = "69";             // default is 69
    
    public:
    
        TFTP_Client(char* ip, int port);    // will assign to default if none provided
        ~TFTP_Client();
    
        int connectToServer();              // connect to server using port
    
        bool Put(char* filename);
    
};