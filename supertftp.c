#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#define OPCODE_READ     1   // Not gonna use this one...
#define OPCODE_WRITE    2
#define OPCODE_DATA     3
#define OPCODE_ACK      4
#define OPCODE_ERROR    5
#define HEADER_SIZE     4
#include <unistd.h>

void set_opcode(char *packet, int op)
{
	// opcode should fit in a byte
	packet[0] = 0;
	packet[1] = (unsigned char)op;
}

void set_block_num(char *packet, int block)
{
	unsigned short *p = (unsigned short*)packet;
	p[1] = htons(block);
}

void complete_write(char* dest, char* src)
{
    FILE* result = fopen(dest, "wb");
    
    FILE* temp = fopen(src, "rb");
    
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
    remove(src);
}

char * filename;

int main(int argc, char *argv[]){

  int main_listening_socket, new_listening_socket;
  int listening_port;
  int buffer_length = 516;
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
    
    char temp_filename[80]; // arbitrarily large filename to specify filename+directory

    unsigned short opcode;
    
    struct stat st = {0};
    
    while(1){
      
        // create working directory for server temp storage
      if (stat("../tftp_temp", &st) == -1) {
          mkdir("../tftp_temp", 0700);
      }
      
      msg_len = recvfrom(main_listening_socket,buffer,buffer_length,0,(struct sockaddr *)&client,(socklen_t *)&fromlen);

      if(msg_len < 0)
      {
          continue;
      }
      
      opcode = ntohs(*(unsigned short int*)&buffer); //opcode is in network byte order, convert to local byte order
      
      /*mode = (char*)&buffer+2+strlen(filename)+1;
      fprintf(stderr,"Main server port - mode: %s\n",mode,strlen(mode));*/
      
      if (opcode == OPCODE_READ)
      {
          fprintf(stderr, "RRQ received, ignoring.\n");
          continue;
          
      }
      else if (opcode == OPCODE_WRITE)
      {
          filename = (char*)&buffer+2;
          //filename = strcat("../temp/",filename); // make it in our working directory
          //fprintf(stderr,"Main server port - mode: %s\n",filename,strlen(filename));
          
          
          // Check to see if the file already exists
          if (access(filename, F_OK) != -1)
          {
              fprintf(stderr, "Error: File already exists.\n");
              char packet[512];
              set_opcode(packet, 5);
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
          strcpy(file, filename); // preserve filename
          
          strcpy(temp_filename, "../tftp_temp/");
          strcat(temp_filename, file);
          
          
          char packet[512];
          set_opcode(packet, 4);
          set_block_num(packet, block_num);
          
          // This is a temporary file for writing to while receiving packets, so as to
          // only display the final file when all data has been written
          FILE *fp;
          fp = fopen(temp_filename, "w");
         
          
          // We've already confirmed we just received a WRQ and that the file doesn't exist already
          // Send an ACK packet
          // sendto without binding first will bind to random port, allowing for concurrency
          sendto(new_listening_socket, packet, sizeof(packet), 0, &client, sizeof(client));
          block_num++;
          
          // Keep receiving requests while keeping track of block number
          while(1)
          {
              unsigned short op_code;
              unsigned short block;
              
              bzero(buffer, buffer_length);
              
              int new_msg_len = recvfrom(new_listening_socket,buffer,buffer_length,0,(struct sockaddr *)&client,(socklen_t *)&fromlen);
              
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
              else if (op_code == OPCODE_DATA)
              {
                  set_opcode(packet, 4);
                  set_block_num(packet, block_num);
                  // Source, size per element in bytes, # of elements, filestream
                  char payload[new_msg_len-HEADER_SIZE];
                  memcpy(payload, buffer+HEADER_SIZE, sizeof(payload));
                  fwrite(payload, 1, sizeof(payload), fp);
                  sendto(new_listening_socket, packet, sizeof(packet), 0, &client, sizeof(client));
                  block_num++;
              }
              
              
              // Last packet received, child process should exit
              if (new_msg_len < 512)
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
