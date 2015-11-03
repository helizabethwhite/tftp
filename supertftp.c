#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define OPCODE_READ     1   // Not gonna use this one...
#define OPCODE_WRITE    2
#define OPCODE_DATA     3
#define OPCODE_ACK      4
#define OPCODE_ERROR    5

struct Packet {
    int packet_num;
    u_char data[512];
    int current_packet_size;
};

struct Packet handle_packet()
{
    struct Packet p;
    p.current_packet_size = 0;
    unsigned char ack_short = 4;
    memset(p.data, 0, 512);
    p.data[1] = ack_short;
    //p.data[1] = 0;
    fprintf(stderr, "0: %d\n", p.data[0]);
    fprintf(stderr, "1: %d\n", p.data[1]);
    fprintf(stderr, "2: %d\n", p.data[2]);
    fprintf(stderr, "3: %d\n", p.data[3]);
    /*p.data[p.current_packet_size] = ack_short & 0xff;
    fprintf(stderr, "Inside p.data[current_packet_size]: %d\n", p.data[p.current_packet_size]);
    p.current_packet_size = 1;
    p.data[p.current_packet_size] = (ack_short >> 8) & 0xff;
    p.data[2] = 0;
    p.data[3] = 0;*/
    
    /*if (send(socket_num, &p, 0, 0) == -1)
    {
        fprintf(stderr, "Error sending ack.\n");
        exit(1);
    }
    else
    {
        fprintf(stderr, "Ack sent successfully.\n");
    }
    return p;*/
}

void set_opcode(char *buf, int op)
{
	/* we assume that op fits in a byte */
	buf[0] = 0;
	buf[1] = (unsigned char)op;
}

void set_block_num(char *packet, int block)
{
	unsigned short *p = (unsigned short*)packet;
	p[1] = htons(block);
}

int main(int argc, char *argv[]){

  int main_listening_socket, new_listening_socket;
  int listening_port;
  int buffer_length = 512;
    int pid;


  if (argc < 2) {
	fprintf(stderr,"Please provide a port number and try again.\n");
	exit(1);

  }

  main_listening_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if(main_listening_socket < 0){
	fprintf (stderr,"Cannot create socket.\n");
	exit(-1);
  }

  struct sockaddr_in server;
  struct sockaddr_in client;
  struct sockaddr_in newAddress;
  
  int addr_length = sizeof(server);
  bzero(&server,addr_length);

  listening_port = atoi(argv[1]);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(listening_port);

  if(bind(main_listening_socket,(struct sockaddr *)&server,addr_length)<0){
	fprintf(stderr,"Cannot listen on the port.\n");
	exit(-1);
  }

  int msg_len =-1;
  int fromlen;
  char buffer[buffer_length];
  char resbuf[512];

  unsigned short opcode;
  char * filename;
  char * mode;

  while(1){
	 fprintf(stderr,"At top of loop\n");
	msg_len = recvfrom(main_listening_socket,buffer,buffer_length,0,(struct sockaddr *)&client,(socklen_t *)&fromlen);

	if(msg_len < 0){
		fprintf(stderr,"No msg");
        continue;
	}
      
      opcode = ntohs(*(unsigned short int*)&buffer); //opcode is in network byte order, convert to local byte order
      fprintf(stderr,"Opcode: %d\n",opcode,2);
      
      filename = (char*)&buffer+2;
      fprintf(stderr,"Filename: %s\n",filename,strlen(filename));
      
      mode = (char*)&buffer+2+strlen(filename)+1;
      fprintf(stderr,"Mode: %s\n",mode,strlen(mode));
      
      fprintf(stderr,"About to fork...\n");
      pid = fork();
      
      if (pid == -1)
      {
          exit(-1);
      }
      // parent process
      else if (pid > 0)
      {
          close(new_listening_socket);
      }
      // child process
      else
      {
          // SEND ACKNOWLEDGEMENT HERE?? (block num is 0), then send your first data
          close(main_listening_socket);
          new_listening_socket = socket(AF_INET, SOCK_DGRAM, 0);
          
          char packet[512];
          set_opcode(packet, 4);
          set_block_num(packet, 0);
          
          if (opcode == 2)
          {
              
              // if you send to without binding first, it will bind to random available port
              sendto(new_listening_socket, packet, sizeof(packet), 0, &client, sizeof(client));
              
          }
          
          fprintf(stderr, "New client port number: %d\n", client.sin_port);
          int new_msg_len = recvfrom(new_listening_socket,buffer,buffer_length,0,(struct sockaddr *)&client,(socklen_t *)&fromlen);
          
          if(new_msg_len < 0){
              fprintf(stderr,"No msg");
              continue;
          }
          fprintf(stderr, "Accepting requests on port %d\n", client.sin_port);
          
          // write request
          
          
      }

	//memcpy(opcode,buffer,2);
	//opcode = (unsigned short int* )&buffer;
		//fprintf(stderr,"%d",ntohs((unsigned short int)*opcode),2);


      /*fprintf(stderr, "%s request for %s from %s using port %d\n", opcode, filename, inet_ntoa(client.sin_addr), ntohs(client.sin_port));*/
      
      fprintf(stderr,"My pid: %d\n", pid);
	//sendto (listening_socket,resbuf,512,0,(struct sockaddr*)&client,fromlen)


	//write(1,buffer,msg_len);
      
  }

    return 1;

}
