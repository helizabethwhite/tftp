/*
 * created by helizabethwhite, 2015
 *
 * This file implements a simple TFTP server that runs on a specified port.
 * The server only processes WRQ (write request) packets, and ignores all others
 * aside from ERROR.
 *
 * TO DO: Add timeouts on server end, block number exceeds the amount that 2 bytes can hold, and perhaps 
 * create instance of server for better unit testing?
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <assert.h>

#define OPCODE_READ             1
#define OPCODE_WRITE            2
#define OPCODE_DATA             3
#define OPCODE_ACK              4
#define OPCODE_ERROR            5
#define HEADER_SIZE             4
#define BUFFER_LENGTH           516
#define MAX_PAYLOAD_LENGTH      512

/*
 * Used to set the first 2 bytes of a packet
 */
void set_opcode(char *packet, int op)
{
	// opcode should fit in a byte
	packet[0] = 0;
	packet[1] = (unsigned char)op;
}

/* 
 * Used to set the second 2 bytes of a packet
 */
void set_block_num(char *packet, int block)
{
	unsigned short *p = (unsigned short*)packet;
	p[1] = htons(block);
}

/*
 * Used to copy contents of temp file to 
 * destination file so that the result only
 * appears on server after all packets have
 * been successfully received
 */
void complete_write(char* dest, char* src)
{
    FILE* result = fopen(dest, "wb");
    
    FILE* temp = fopen(src, "r");
    
    if (temp == NULL)
    {
        perror("error");
    }
    
    int c = fgetc(temp);
    while (c != EOF)
    {
        fputc(c, result);
        c = fgetc(temp);
    }
    fclose(result);
    fclose(temp);
    rename(src, dest);
}

// filename taken from WRQ packets
char * filename;

int main(int argc, char *argv[]){
    
    int main_listening_socket, new_listening_socket;
    int listening_port;
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

    int msg_len =-1;    // length of the packets received by client
    int fromlen;
    char buffer[BUFFER_LENGTH]; // temp storage of packets received
    
    char temp_filename[80]; // arbitrarily large filename to specify filename+directory

    unsigned short opcode;
    
    struct stat st = {0};
    
    while(1){
      
        // create working directory for server temp storage
        /*if (stat("../tftp_temp/", &st) == -1)
        {
            mkdir("../tftp_temp/", 0700);
        }*/
      
        msg_len = recvfrom(main_listening_socket,buffer,BUFFER_LENGTH,0,(struct sockaddr *)&client,(socklen_t *)&fromlen);

      if (msg_len < 0)
      {
          continue;
      }
      
      opcode = ntohs(*(unsigned short int*)&buffer); //opcode is in network byte order, convert to local byte order
      
      if (opcode == OPCODE_READ)
      {
          fprintf(stderr, "RRQ received, ignoring.\n");
          continue;
          
      }
      else if (opcode == OPCODE_WRITE)
      {
          filename = (char*)&buffer+2;
          
          // Check to see if the file already exists
          if (access(filename, F_OK) != -1)
          {
              fprintf(stderr, "Error: File already exists.\n");
              char packet[MAX_PAYLOAD_LENGTH];
              set_opcode(packet, OPCODE_ERROR);
              set_block_num(packet, 6); // Error Code 6: File already exists
              sendto(main_listening_socket, packet, sizeof(packet), 0, &client, sizeof(client));
          }
          
      }
      
      pid = fork();
      
      // Error during fork, abort
      if (pid == -1)
      {
          close(main_listening_socket);
          close(new_listening_socket);
          exit(-1);
      }
      // Parent Process
      else if (pid > 0)
      {
          close(new_listening_socket);
      }
      // Child Process
      else
      {
          close(main_listening_socket);
          
          new_listening_socket = socket(AF_INET, SOCK_DGRAM, 0);
          
          int block_num = 0;
          
          char file[40];
          strcpy(file, filename); // preserve filename gathered by first WRQ
          
          strcpy(temp_filename, "temp");       // add temp directory to empty string
          strcat(temp_filename, file);                  // concatenate temp directory with unique filename
          
          
          char packet[MAX_PAYLOAD_LENGTH];
          set_opcode(packet, OPCODE_ACK);
          set_block_num(packet, block_num);
          
          // This is a temporary file for writing to while receiving packets, so as to
          // only display the final file when all data has been written
          FILE *fp;
          fp = fopen(temp_filename, "wb");
         
          
          // We've already confirmed we just received a WRQ and that the file doesn't exist already
          // Send an ACK packet
          // sendto without binding first will bind to random port, allowing for concurrency
          sendto(new_listening_socket, packet, sizeof(packet), 0, &client, sizeof(client));
          
          // Verify that the new port does not equal the main server port
          assert(client.sin_port != listening_port);
          
          block_num++;
          
          // Keep receiving requests while keeping track of block number
          int new_msg_len;
          while(1)
          {
              unsigned short op_code;
              unsigned short block;
              
              bzero(buffer, BUFFER_LENGTH); // Set all contents of the buffer to contain 0s
              
              new_msg_len = recvfrom(new_listening_socket,buffer,BUFFER_LENGTH,0,(struct sockaddr *)&client,(socklen_t *)&fromlen);
              
              if (new_msg_len < 0)
              {
                  continue;
              }
              
              op_code = ntohs(*(unsigned short int*)&buffer);
              
              block = buffer[2] << 8 | buffer[3]; // Concatenate upper and lower halves of the block into one short
              
              if (op_code == OPCODE_ERROR)
              {
                  fprintf(stderr,"Error received, exiting.\n");
                  close(new_listening_socket);
                  fclose(fp);
                  exit(1);
              }
              else if (op_code == OPCODE_DATA)
              {
                  set_opcode(packet, OPCODE_ACK);
                  set_block_num(packet, block_num);
                  char payload[new_msg_len-HEADER_SIZE];
                  
                  // Source, size per element in bytes, # of elements, filestream
                  memcpy(payload, buffer+HEADER_SIZE, sizeof(payload));
                  fwrite(payload, 1, sizeof(payload), fp);
                  sendto(new_listening_socket, packet, sizeof(packet), 0, &client, sizeof(client));
                  block_num++;
              }
              
              
              // Last packet received, child process should exit
              if (new_msg_len < MAX_PAYLOAD_LENGTH)
              {
                  close(fp);
                  
                  // Transfer contents of temp file into correct file
                  // Do this by looping through the temp file and copying into destination file
                  complete_write(file, temp_filename);
                  close(new_listening_socket);
                  exit(1);
              }
              
          }
          
      }
      
  }

    return 1;

}
