#include <iostream>

typedef unsigned char byte;
typedef unsigned short word;

#define OPCODE_READ     1   // Not gonna use this one...
#define OPCODE_WRITE    2
#define OPCODE_DATA     3
#define OPCODE_ACK      4
#define OPCODE_ERROR    5

#define TFTP_PACKET_SIZE 512

// a couple of different possibilities,
// but for this spec we will use octet mode
#define TFTP_TRANSFER_MODE "octet"

// This class defines a TFTP packet.
// A TFTP packet can be one of 5 types as dictated by the above opcodes.
class TFTP_Packet {

	protected:
		
		int current_packet_size;
		unsigned char data[TFTP_PACKET_SIZE]; // allocate data space

	public:

		TFTP_Packet();
		~TFTP_Packet();
    
		void clear();
		int getSize();
    
        word getOPCode();
        word getPacketNumber();
    
        byte getByte(int offset);
        word getWord(int offset);
        bool getString(int offset, char* buffer, int length);       // can be used for getting filename, mode, or error msg
    
		bool addByte(byte b);                       // function used to expand packet (most likely data)
		bool addWord(word w);                       // function used to expand packet (most likely data)
		bool addString(char* str);
		
		unsigned char* getData(int offset);
    
		bool copyData(int offset, char* dest, int length);

		bool createWRQ(char* filename);
		bool createACK(int packet_num);
		bool createData(int block, char* data, int data_size);
		bool createError(int error_code, char* message);
};