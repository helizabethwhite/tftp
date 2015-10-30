// to do: 
//	fxns for reading/writing mut be declared first (they have new types of writeFunc etc)
//	enum for opcodes
// diff ports

package main


import (
	"errors"
	"fmt"
	"io"
	"net"
	"time"
	"bytes"
	"encoding/binary"
)

/*
 *
 * CODE FOR PACKET
 *
 */


const (
	RRQ   = uint16(1) // Read request (RRQ)
	WRQ   = uint16(2) // Write request (WRQ)
	DATA  = uint16(3) // Data
	ACK   = uint16(4) // Acknowledgement
	ERROR = uint16(5) // Error
	BLOCK_SIZE = 512; // Typical block size for UDP (DNS standard)
}

// Packet represents any TFTP packet
type Packet interface {
	// Returns the packet type
	getType()
	// 
	Bytes() []byte
}

type ReqPacket struct {
	Filename  string
	Mode      string
	Type      uint16
	BlockSize int
}

func (p *ReqPacket) GetType() uint16 {
	return p.Type
}

func ParsePacket(buf []byte) (Packet, error) {
	// TO DO: parse the packet and determine the type of packet,
	// take appropriate action
}

/*
 *
 * CODE FOR CLIENT
 *
 */

 type TftpClient struct {
	address *net.UDPAddr
	UDPConn *net.UDPConn
}

func (client *TftpClient) Put(filename string, handler func (w *io.PipeWriter)) (error) {

	// returns an error if address cannot be resolved
	address, err := net.ResolveUDPAddr("udp", "localhost:69")

	// error with connection, abort
	if err != nil {
		return err
	}

	// listen for UDP packets
	connection, e := net.ListenUDP("udp", address)

	// again, error with connection, abort further processing
	if err != nil {
		return err
	}

	reader, writer := io.Pipe()

	// TO DO: code for sender goes here

	return nil

}

// format is variable_name Type
type TftpServer struct {
	mDir	string	// working directory
}

// returns new instance of tftp server that works in the named directory
func newTftpServer(directory string) *TftpServer {
	return &TftpServer {
		mDir: directory,
	}
}

func main() {
	fmt.Printf("HI\n")
}