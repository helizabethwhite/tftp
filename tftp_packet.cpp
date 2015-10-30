#include "tftp_packet.h"

using namespace std;

TFTP_Packet::TFTP_Packet()	{
	// initialize the packet
    clear();
}

TFTP_Packet::~TFTP_Packet() {
}

void TFTP_Packet::clear()	{
	
    current_packet_size = 0;
    // Put "current_packet_size" at the beginning of the block of memory
    // occupied by data
	memset(data, current_packet_size, TFTP_PACKET_SIZE);
}

unsigned char* TFTP_Packet::getData(int offset) {
	return &(data[offset]);
}

/*
 * Return what type of packet we are handling
 */
word TFTP_Packet::getOPCode() {
	return (this->GetWord(0));
}

/*
 * Copy data from one area of memory to another
 */
bool TFTP_Packet::copyData(int offset, char* dest, int length) {
    // Make sure we are not attempting to place data
    // outside of the memory we have allocated
	if (offset > this->getSize())
    {
        return false;
    }
	else if (length < (this->getSize() - offset))
    {
        return false;
    }
	memcpy(dest, &(data[offset]), (this->getSize()-offset));
	return true;
}


/*
 * This function allows for adding data to the packet, up to
 * the maximum allowed packet size (1024 bytes).
 *
 * Why 1024 and 512? UDP is connectionless and error-checking
 * is minimal, so sending smaller packet sizes ensures they
 * are more likely to get to their destination.
 */
bool TFTP_Packet::addByte(BYTE b) {

	if (current_packet_size >= TFTP_PACKET_SIZE) {
		return false;
	}
    // Add new byte to the packet we are assembling
	data[current_packet_size] = (unsigned char)b;
	current_packet_size++;
	return true;
}

/* Add data to the packet in larger amounts
 * 1 word = 2 bytes
 */
bool TFTP_Packet::addWord(word w) {
    // add first half
	if (!addByte(*(((byte*)&w)+1)))
    {
        return false;
    }
    // add second
	return (!addByte(*((byte*)&w)));
}

/*
 * Build the string component of the packet
 *
 * This is only relevant for read/write and error
 * packets.
 */
bool TFTP_Packet::addString(char* str) {
	int n = strlen(str);
	for (int i=0; i < n; i++) {
		if (!addByte(*(str + i))) {
			return false;
		}
	}
	return true;
}

/*
 * Increase the size of our packet (for data transmission)
 */
bool TFTP_Packet::addMemory(char* buffer, int len) {

	if (current_packet_size + len >= TFTP_PACKET_SIZE)	{
		cout << "Error: maximum size already reached.\n";
		return false;
	}
	memcpy(&(data[current_packet_size]), buffer, len);
	current_packet_size += len;
	return true;
}

/*
 * Return the byte located at the specified offset
 */
byte TFTP_Packet::getByte(int offset) {
	return (byte)data[offset];
}

/*
 * Return the word located at the specified offset
 */
word TFTP_Packet::getWord(int offset) {

    // Get first half of the word
	word hi = getByte(offset);
    // Get second half of the word
	word lo = getByte(offset + 1);
    // Concatenate the two bytes to form the resulting word
	return ((hi<<8)|lo);
}


word TFTP_Packet::getNumber() {
	if (this->isData() || this->isACK())
    {
		return this->getWord(2);
	}
    else {
		return 0;
	}
}

bool TFTP_Packet::getString(int offset, char* buffer, int len) {
	if (offset > current_packet_size)
    {
        return false;
    }
	else if (len < current_packet_size - offset)
    {
        return false;
    }
	memcpy(buffer, &(data[offset]), current_packet_size - offset);
	return true;
}

/*
 * Assemble the WRQ packet, which doesn't get 
 * the file transfer going until ACK is received
 *
 * This will get called after receiving a Put command, 
 * followed by the filename
 */
bool TFTP_Packet::createWRQ(char* filename) {
	clear();
	addWord(OPCODE_WRITE);
	addString(filename);
	addByte(0);
	addString(TFTP_DEFAULT_TRANSFER_MODE);
	addByte(0);
	return true;
}

/*
 * Assemble the ACK packet
 */
bool TFTP_Packet::createACK(int packet_num) {
	clear();
	addWord(OPCODE_ACK);
	addWord(packet_num);
	return true;
}

/*
 * Assemble a DATA packet.
 *
 * This will ideally be used enough times to create
 * the entirety of a file's data when writing to
 * the server.
 */
bool TFTP_Packet::createData(int block, char* data, int data_size) {
	clear();
	addWord(OPCODE_DATA);
	addWord(block);
	addMemory(data, data_size);
	return true;
}

/*
 * Create an error packet. For example, maybe the file
 * being Put to the server already exists. Somewhere, I
 * will need to define error codes to correspond with
 * common errors that might arise.
 */
bool TFTP_Packet::createError(int error_code, char* message) {
	clear();
	addWord(OPCODE_ERROR);
	addWord(error_code);
	addString(message);
	addByte(0);
	return true;

}

/*
 * Get the current size of the packet
 */
int TFTP_Packet::getSize() {
	return current_packet_size;
}

/*
 * ~Helper functions~
 */
bool TFTP_Packet::isWRQ() {
	return (this->getOPCode == OPCODE_WRITE);
}

bool TFTP_Packet::isACK() {
	return (this->getOPCode == OPCODE_ACK);
}

bool TFTP_Packet::isData() {
	return (this->getOPCode == OPCODE_DATA);
}

bool TFTP_Packet::isError() {
	return (this->getOPCode == OPCODE_ERROR);
}