/* Included headers */
#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>

/* Definitions */
#define ACK_PACKET_LENGTH (4)

/* Global Variables */
char wrq[516];
static int status = 0;
int packet_size = 0;
int recv_packets = 0;
int ack_block = 0;
int ack_current = 0;

/* Functions prototypes */
int Build_RRQ_Packet(char *buffer, char *filename);
int Build_WRQ_Packet(char *buffer, char *filename);
int Get_Error(char *packet);
int Get_size(char *buffer);
int Build_ACK_Packet(char *buffer);
int readchar(char *buffer);
void Get_File(char *filename);



int main(int argc, char *argv[])
{
  char filename[100];

  struct sockaddr_in v4address = { AF_INET, htons(69) };


  if((status = inet_pton(AF_INET, argv[1], &v4address.sin_addr)) < 0)
  {
    printf("Inet_pton error occured\n");
    return 1;
  }

  /* Create socket */
  int socketid = socket(PF_INET, SOCK_DGRAM, 0);
  if (socketid < 0)
  {
    printf("Couldn't create socket\n");
    return socketid;
  }
  
  /*
  char option[5];

  readchar(option);
  fflush(stdin);
  */

  printf("Tftp>");
  Get_File(filename);

  /* Builds Read request packet */
  Build_RRQ_Packet(wrq, filename);

  /* Send write request packet */
  sendto(socketid, wrq, Get_size(wrq), 0, (struct sockaddr*) &v4address, sizeof(v4address));

  /* set ack_current to 1 */
  ack_current = 1;


  FILE *p = fopen(filename, "w");

  while(1)
  {

    /* Get address size */
    int addr_size =  sizeof(v4address);

    /* Recieve packet from server */
    packet_size = recvfrom(socketid, wrq, 516, 0, (struct sockaddr*) &v4address, &addr_size);

    /*
    ack_block = ((unsigned short)wrq[2] << 8) | (unsigned short)wrq[3];
    */

    uint16_t ackt = wrq[2] << 8 | wrq[3];
    recv_packets += packet_size;

    /* Print some information on screen */
    printf("%d bytes recieved from %s\n", packet_size, inet_ntoa(v4address.sin_addr));
    printf("%d acknowledgement recieved from %s\n", ack_current - 1, inet_ntoa(v4address.sin_addr));

    if (packet_size - 4 < 0)
    {
      printf("Error\n");
      return -1;
    }
    /* Check if opcode is 5 (Error) */
    if (wrq[1] == 5)
    {
        Get_Error(wrq);
        return -1;  /* Error */
    }

    /* If there is some data and not duplicated write to file */
    if ((packet_size - 4 > 0) && (ackt > ack_current - 1))
    {
      /* Write to file */
      fwrite(&wrq[4], 1, (packet_size - 4), p);
      ack_current++;
      if (ferror(p))
      {
        printf("Failed while writing to file\n");
        return -1;
      }
    }
    else
    {
      wrq[2] = ack_current >> 8;
      wrq[3] = ack_current;
    }

    /* build acknowledgement packet */
    Build_ACK_Packet(wrq);

    /* Send acknowledgement packet */
    status = sendto(socketid, wrq, ACK_PACKET_LENGTH, 0, (struct sockaddr*) &v4address, sizeof(v4address));

    if (status == -1)
    {
      printf("Error while sending datagram\n");
      return status;
    }


    /* If this is the last packet break the loop */
    if(packet_size - 4 != 512)
      break;

  }
  printf("\nDone\n");

  printf("%d Total bytes recieved from %s\n", recv_packets, inet_ntoa(v4address.sin_addr));
  printf("%d Total acknowledgement recieved from %s\n", ack_current, inet_ntoa(v4address.sin_addr));

  if((status = close(socketid)) < 0)
  {
    printf("Error whiile closing socket\n");
    return status;
  }
  printf("\n");

  return 0;
}

int Build_RRQ_Packet(char *buffer, char *filename)
{

  buffer[0] = 0;
  buffer[1] = 1;
  char *p = buffer + 2;

  strcat(p, filename);
  p += strlen(filename) +1;

  strcat(p, "octet");
  p += strlen("octet");

  *p = 0;
  *p++ = '\0';
  return 0;

}

int Build_WRQ_Packet(char *buffer, char *filename)
{

  buffer[0] = 0;
  buffer[1] = 2;
  char *p = buffer + 2;

  strcat(p, filename);
  p += strlen(filename) +1;

  strcat(p, "netascii");
  p += strlen("netascii");

  *p = 0;
  *p++ = '\0';
  return 0;

}

int Get_Error(char *packet)
{

  char *errors[] =
  {
   "Not defined, see error message (if any)",
   "File not found",
   "Access violation",
   "Disk full or allocation exceeded",
   "Illegal TFTP operation",
   "Unknown transfer ID",
   "File already exists",
   "No such user"
 };
 char *p = &packet[3];
   switch (*p)
   {
     case 0:
      printf("%s\n",errors[0]);
     case 1:
      printf("%s\n",errors[1]);
      break;
     case 2:
      printf("%s\n",errors[2]);
      break;
     case 3:
      printf("%s\n",errors[3]);
      break;
     case 4:
      printf("%s\n",errors[4]);
      break;
     case 5:
      printf("%s\n",errors[5]);
      break;
     case 6:
      printf("%s\n",errors[6]);
      break;
     case 7:
      printf("%s\n",errors[7]);
      break;
     default:
      printf("Invalid error code\n");
   };
   return 0;

}

int Build_ACK_Packet(char *buffer)
{

  buffer[0] = 0;
  buffer[1] = 4;
  return 0;

}

int Get_size(char *buffer)
{

  int count = 0;
  int flag = 3;
  char *p = buffer;
  while (flag != 0)
  {
    if(*p == 0)
    {
      count++;
      p++;
      flag--;
      continue;
    }
    count++;
    p++;
  }
  return count;

}

void Get_File(char *filename)
{

  char c;
  int cn = 0;
  while((c = getchar()) != '\n')
  {
    filename[cn] = c;
    cn++;
  }
  filename[cn+1] = 0;

}
int readchar(char *buffer)
{
  char character;
  int character_count;
  char *p = buffer;
  while((character = getchar()) != '\n')
  {
    if (character != ' ')
    {
      *p = character;
      character_count++;
      p++;
    }
    else
      continue;
  }
  *p = '\0';

  return character_count;
}
