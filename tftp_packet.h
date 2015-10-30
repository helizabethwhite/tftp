typedef unsigned char byte;
typedef unsigned short word;

#define OPCODE_READ     1   // Not gonna use this one...
#define OPCODE_WRITE    2
#define OPCODE_DATA     3
#define OPCODE_ACK      4
#define OPCODE_ERROR    5

#define TFTP_PACKET_MAX_SIZE 1024
#define TFTP_PACKET_DATA_SIZE 512

//"netascii", "octet", or "mail" are all
// possibilities, but for this spec we will use octet mode
#define TFTP_DEFAULT_TRANSFER_MODE "octet"

// This class defines a TFTP packet.
// A TFTP packet can be one of 5 types as dictated by the above opcodes.
class TFTP_Packet {

	protected:
		
		int current_packet_size;
		unsigned char data[TFTP_PACKET_MAX_SIZE]; // allocate data space

	public:

		TFTP_Packet();
		~TFTP_Packet();
    
		void clear();

		int getSize();
		bool setSize(int size);

		bool addByte(byte b);
		bool addWord(word w);
		bool addString(char* str);
		bool addMemory(char* buffer, int len);
		
		byte getByte(int offset);
		word getWord(int offset = 0);
		bool getString(int offset, char* buffer, int length);
		word getNumber();
		unsigned char* getData(int offset = 0);
		bool copyData(int offset, char* dest, int length);

		bool createRRQ(char* filename);
		bool createWRQ(char* filename);
		bool createACK(int packet_num);
		bool createData(int block, char* data, int data_size);
		bool createError(int error_code, char* message);

		bool sendPacket(TFTP_Packet*);
		bool isWRQ();
		bool isACK();
		bool isData();
		bool isError();

};