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

    int msg_len =-1;                            // length of the packets received by client
    int fromlen;
    char buffer[BUFFER_LENGTH];                 // temp storage of packets received
    
    char temp_filename[80];                     // arbitrarily large filename to specify filename+directory

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
      
      opcode = ntohs(*(unsigned short int*)&buffer);    //opcode is in network byte order, convert to local byte order
      filename = (char*)&buffer+2;
      
        if (opcode == OPCODE_READ)
      {
          struct stat st;
          int result = stat(filename, &st);
          // Check to see if the file doesn't exist
          if (result != 0)
          {
              fprintf(stderr, "Error: File doesn't exist.\n");
              char packet[MAX_PAYLOAD_LENGTH];
              set_opcode(packet, OPCODE_ERROR);
              set_block_num(packet, 1); // Error Code 1: File not found
              sendto(main_listening_socket, packet, sizeof(packet), 0, &client, sizeof(client));
              continue;
          }
          
      }
      else if (opcode == OPCODE_WRITE)
      {
          
          // Check to see if the file already exists
          if (access(filename, F_OK) != -1)
          {
              fprintf(stderr, "Error: File already exists.\n");
              char packet[MAX_PAYLOAD_LENGTH];
              set_opcode(packet, OPCODE_ERROR);
              set_block_num(packet, 6); // Error Code 6: File already exists
              sendto(main_listening_socket, packet, sizeof(packet), 0, &client, sizeof(client));
              continue;
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
      // Parent Process, doesn't need to know about new socket
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
          
          FILE *fp;
          char file[40];
          strcpy(file, filename);                       // preserve filename gathered by first processed req
          
          
          if (opcode == OPCODE_READ)
          {
              block_num++;                              // In contrast to WRQ, block number begins at 1 and not 0
              
              fp = fopen(filename, "rb");               // open file for reading
              
              char *file_contents_buffer;               // Buffer used to store contents of file
              size_t file_size;                         // Size of the file
    
              fseek(fp, 0, SEEK_END);
              file_size = ftell(fp);                                            // get the size of the file
              rewind(fp);                                                       // set our filestream back to the top of the file instead of the bottom
              file_contents_buffer = malloc(file_size * (sizeof(char)));        // allocate buffer size to hold contents of file
              fread(file_contents_buffer, sizeof(char), file_size, fp);         // read the contents of the file into the buffer
              fclose(fp);
              
              int payload_length = MAX_PAYLOAD_LENGTH;
              if (file_size < MAX_PAYLOAD_LENGTH)       // Always check the size of the file in case the file is small enough to transfer just one packet with reduced size
              {
                  payload_length = file_size;
              }
              
              char packet[4+payload_length];            // 516 bytes (4 bytes for opcode and blocknum + payload) is
              bzero(packet, 4+payload_length);
              set_opcode(packet, OPCODE_DATA);          // default for data packets, but our file size might be small enough for less
              set_block_num(packet, block_num);
              
              
              memcpy(packet+4, file_contents_buffer, payload_length); // Create our first packet of data
              
              sendto(new_listening_socket, packet, sizeof(packet), 0, &client, sizeof(client));     // Send our first data packet
              
              // Verify that the new port does not equal the main server port
              assert(client.sin_port != listening_port);
              
              // Keep receiving requests while keeping track of block number
              int new_msg_len;
              int offset = 1;
              int retransmit_attempts = 0;
              int sent_last_packet = 0;
              while(1)
              {
                  unsigned short op_code;
                  unsigned short block;
                  
                  bzero(buffer, BUFFER_LENGTH);                         // Set all contents of the buffer to contain 0s
                  
                  new_msg_len = recvfrom(new_listening_socket,buffer,BUFFER_LENGTH,0,(struct sockaddr *)&client,(socklen_t *)&fromlen);
                  
                  if (new_msg_len < 0)
                  {
                      continue;
                  }
                  
                  op_code = ntohs(*(unsigned short int*)&buffer);
                  
                  block = buffer[2] << 8 | buffer[3];
                  
                  if (op_code == OPCODE_ERROR)
                  {
                      fprintf(stderr,"Error received, exiting.\n");
                      close(new_listening_socket);
                      fclose(fp);
                      exit(1);
                  }
                  else if (op_code == OPCODE_ACK)
                  {
                      
                      // we are receiving the proper ack packet, proceed transmitting data normally
                      if (block == block_num && !sent_last_packet)
                      {
                          retransmit_attempts = 0;
                          block_num++;
                          if (file_size-offset*payload_length < payload_length)
                          {
                              payload_length = file_size-offset*payload_length;
                              
                          }
                          char data_packet[4+payload_length];               // Only create a data packet as large as needed to contain the remaining file data
                          set_opcode(data_packet, OPCODE_DATA);
                          set_block_num(data_packet, block_num);
                          
                          memcpy(data_packet+4, file_contents_buffer+offset*MAX_PAYLOAD_LENGTH, payload_length);
                          sendto(new_listening_socket, data_packet, sizeof(data_packet), 0, &client, sizeof(client));
            
                          // We have sent our last packet
                          if (sizeof(data_packet) < 516)
                          {
                              sent_last_packet = 1;
                          }
                          offset++;
                          
                      }
                      
                      // a packet was lost somewhere, retransmit last packet
                      else if ((block != block_num) && (retransmit_attempts < 5))
                      {
                          offset--;
                          if (file_size-offset*payload_length < payload_length)
                          {
                              payload_length = file_size-offset*payload_length;
                          }
                          char data_packet[4+payload_length];               // Only create a data packet as large as needed to contain the remaining file data
                          set_opcode(data_packet, OPCODE_DATA);
                          set_block_num(data_packet, block_num);
                          
                          memcpy(data_packet+4, file_contents_buffer+offset*MAX_PAYLOAD_LENGTH, payload_length);
                          sendto(new_listening_socket, data_packet, sizeof(data_packet), 0, &client, sizeof(client));
                          offset++;
                          retransmit_attempts++;
                      }
                      
                      // We've transmitted the same packet 5 times and are timing out, or we've sent our last packet
                      else
                      {
                          fprintf(stderr,"Exiting.\n");
                          close(new_listening_socket);
                          fclose(fp);
                          exit(1);
                      }
                     
                      
                  }
              }
 
          }
          else if (opcode == OPCODE_WRITE)
          {
              strcpy(temp_filename, "temp");                // add temp directory to empty string
              strcat(temp_filename, file);                  // concatenate temp directory with unique filename
              
              
              
              // This is a temporary file for writing to while receiving packets, so as to
              // only display the final file when all data has been written
              fp = fopen(temp_filename, "wb");
              
              char packet[MAX_PAYLOAD_LENGTH];
              set_opcode(packet, OPCODE_ACK);
              set_block_num(packet, block_num);
              
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
                  
                  bzero(buffer, BUFFER_LENGTH);                         // Set all contents of the buffer to contain 0s
                  
                  new_msg_len = recvfrom(new_listening_socket,buffer,BUFFER_LENGTH,0,(struct sockaddr *)&client,(socklen_t *)&fromlen);
                  
                  if (new_msg_len < 0)
                  {
                      continue;
                  }
                  
                  op_code = ntohs(*(unsigned short int*)&buffer);
                  
                  block = buffer[2] << 8 | buffer[3];                   // Concatenate upper and lower halves of the block into one short
                  
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
      
  }

    return 1;

}
