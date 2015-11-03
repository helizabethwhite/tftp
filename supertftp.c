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
	msg_len = recvfrom(main_listening_socket,buffer,buffer_length,0,(struct sockaddr *)&client,(socklen_t *)&fromlen);

	if(msg_len < 0){
        continue;
	}
      
      opcode = ntohs(*(unsigned short int*)&buffer); //opcode is in network byte order, convert to local byte order
      fprintf(stderr,"Main server port - opcode: %d\n",opcode,2);
      
      filename = (char*)&buffer+2;
      fprintf(stderr,"Main server port - filename: %s\n",filename,strlen(filename));
      
      mode = (char*)&buffer+2+strlen(filename)+1;
      fprintf(stderr,"Main server port - mode: %s\n",mode,strlen(mode));
      
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
          
          int block_num = 0;
          
          char packet[512];
          set_opcode(packet, 4);
          set_block_num(packet, block_num);
          
          if (opcode == 2)
          {
              // if you send to without binding first, it will bind to random available port
              sendto(new_listening_socket, packet, sizeof(packet), 0, &client, sizeof(client));
          }
          
          fprintf(stderr, "New client port number: %d\n", client.sin_port);
          fprintf(stderr, "Accepting requests on port %d\n", client.sin_port);
          
          
          // keep receiving requests and keeping track of block number
          while(1)
          {
              unsigned short op_code;
              unsigned short block;
              
              int new_msg_len = recvfrom(new_listening_socket,buffer,buffer_length,0,(struct sockaddr *)&client,(socklen_t *)&fromlen);
              
              if(new_msg_len < 0){
                  fprintf(stderr,"No msg");
                  continue;
              }
              
              op_code = ntohs(*(unsigned short int*)&buffer);
              fprintf(stderr,"Unique client port - opcode: %d\n",op_code,2);
              
              block = buffer[2] << 8 | buffer[3];
              fprintf(stderr,"Unique client port - block num: %d\n", block);
              
              
          }
         
          
          
      }
      
  }

    return 1;

}
