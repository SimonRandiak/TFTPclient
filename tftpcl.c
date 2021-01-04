#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#define DEFAULT_PORT 69
#define ACK_PACKET_LENGTH 4
#define PACKET_SIZE 516

void Get_File(char *filename);
int Build_RRQ_Packet(char *buffer, char *filename);
int Build_WRQ_Packet(char *buffer, char *filename);
void Build_ACK_Packet(char *buffer, uint16_t block_number);
void Build_DATA_Packet(char *buffer, uint16_t block_number);
void Get_TFTP_error(char *buffer);
int RRQ_loop(int socketfd, struct sockaddr_in serveraddr);
int WRQ_loop(int socketfd, struct sockaddr_in serveraddr);

typedef enum
{
  RRQ = 1,
  WRQ = 2,
  DATA = 3,
  ACK = 4,
  ERROR = 5

} opcode;

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("usage: %s ip_address\n", argv[0]);
    return -1;
  }

  int socketfd;
  struct sockaddr_in   serveraddr;
  memset(&serveraddr, 0, sizeof(serveraddr));

  socketfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (socketfd < 0)
  {
    puts("ERROR while creating Socket()");
    return socketfd;
  }

  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(DEFAULT_PORT);
  inet_pton(AF_INET, argv[1], &serveraddr.sin_addr);

  /*
  RRQ_loop(socketfd, serveraddr);
  */
  WRQ_loop(socketfd, serveraddr);

  shutdown(socketfd, SHUT_RDWR);

  return 0;
}

/*                                                            *
*                  FUNCTIONS                                  *
*                                                             */



void Get_File(char *filename)
{

  fgets(filename, 100, stdin);
  int size = strlen(filename);
  filename[size - 1] = '\0';


}

int Build_RRQ_Packet(char *buffer, char *filename)
{
  char *p = &buffer[2];

  buffer[0] = 0;
  buffer[1] = RRQ;

  strcpy(p, filename);
  p += strlen(filename);


  *p++ = 0;


  strcpy(p, "octet");
  p += strlen("octet");

  *p = 0;
  *p++ = '\0';
  int sizereturn = strlen(filename) + strlen("octet") + 4;

  return sizereturn;
}

int Build_WRQ_Packet(char *buffer, char *filename)
{
  char *p = &buffer[2];

  buffer[0] = 0;
  buffer[1] = WRQ;

  strcpy(p, filename);
  p += strlen(filename);

  *p++ = 0;

  strcpy(p, "octet");
  p += strlen("octet");

  *++p = '\0';

  int sizereturn = strlen(filename) + strlen("octet") + 4;

  return sizereturn;
}

void Build_ACK_Packet(char *buffer, uint16_t block_number)
{
  buffer[0] = 0;
  buffer[1] = ACK;
  uint16_t converted = htons(block_number); /* Convert intiger int o network byte order * Big Endian */
  memcpy(&buffer[2], &converted, sizeof(converted));

}

void Build_DATA_Packet(char *buffer, uint16_t block_number)
{
  buffer[0] = 0;
  buffer[1] = DATA;
  uint16_t converted = htons(block_number); /* Convert intiger int o network byte order * Big Endian */
  memcpy(&buffer[2], &converted, sizeof(converted));

}

void Get_TFTP_error(char *buffer)
{
  char *error_codes[] =
  {
  "Not defined, see error message (if any).",
  "File not found.",
  "Access violation.",
  "Disk full or allocation exceeded.",
  "Illegal TFTP operation.",
  "Unknown transfer ID.",
  "File already exists.",
  "No such user."
  };

  switch (buffer[3])
  {
    case 0:
      printf("%s\n", error_codes[0]); break;
    case 1:
      printf("%s\n", error_codes[1]); break;
    case 2:
      printf("%s\n", error_codes[2]); break;
    case 3:
      printf("%s\n", error_codes[3]); break;
    case 4:
      printf("%s\n", error_codes[4]); break;
    case 5:
      printf("%s\n", error_codes[5]); break;
    case 6:
      printf("%s\n", error_codes[6]); break;
    case 7:
      printf("%s\n", error_codes[7]); break;
    default:
      printf("Invalid error code\n"); break;
  }
}
int RRQ_loop(int socketfd, struct sockaddr_in serveraddr)
{
  char packet[PACKET_SIZE+1];
  char filename[100];
  int recvfd,   sendfd, size, addr_size;
  uint16_t ack_count = 1;
  uint16_t last_packet = 0;
  uint16_t recieved_ack = 0;
  int op_code;
  FILE *p = NULL;

  addr_size = sizeof(serveraddr);

  Get_File(filename);

  size = Build_RRQ_Packet(packet, filename);

  sendfd = sendto(socketfd, packet, size, 0, (struct sockaddr*) &serveraddr, addr_size);
  if (sendfd == -1)
  {
    printf("Error while sending datagram\n");
    return sendfd;
  }
  p = fopen(filename, "a");

  while (1 && last_packet != 1)
  {
    addr_size = sizeof(serveraddr);
    recvfd = recvfrom(socketfd, packet, PACKET_SIZE, 0, (struct sockaddr*) &serveraddr, &addr_size);
    if (recvfd - 4 < 0)
    {
      printf("Data corrupted\n");
      return -1;
    }

    op_code = packet[1];
    recieved_ack = *(uint16_t*)&packet[2];    /* Read in Big Endian */
    recieved_ack = ntohs(recieved_ack);       /* Convert to Little Endian * Intel processors */
    if (op_code == 5)
    {
      Get_TFTP_error(packet);
      return -1;
    }
    if (ack_count == recieved_ack)
    {
      ack_count++;
      fwrite(&packet[4], 1, (recvfd - 4), p);
    }
    else
    {
      Build_ACK_Packet(packet, ack_count);
      sendfd = sendto(socketfd, packet, ACK_PACKET_LENGTH, 0, (struct sockaddr*) &serveraddr, addr_size);
      printf("Re-sending ACK\n");
      continue;
    }


    Build_ACK_Packet(packet, recieved_ack);

    sendfd = sendto(socketfd, packet, ACK_PACKET_LENGTH, 0, (struct sockaddr*) &serveraddr, addr_size);

    if (recvfd -4 != 512)
    {
      last_packet = 1;
    }

  }
  fclose(p);
  return 0;
}

int WRQ_loop(int socketfd, struct sockaddr_in serveraddr)
{
  char packet[PACKET_SIZE+1];
  char filename[100];
  int recvfd,   sendfd, size, addr_size;
  uint16_t ack_count = 0;
  uint16_t last_packet = 0;
  uint16_t recieved_ack = 0;
  int op_code;
  FILE *p = NULL;

  addr_size = sizeof(serveraddr);

  Get_File(filename);

  size = Build_WRQ_Packet(packet, filename);

  sendfd = sendto(socketfd, packet, size, 0, (struct sockaddr*) &serveraddr, addr_size);
  if (sendfd == -1)
  {
    printf("Error while sending datagram\n");
    return sendfd;
  }
  p = fopen(filename, "r");

  while (1 && last_packet != 1)
  {
    addr_size = sizeof(serveraddr);
    recvfd = recvfrom(socketfd, packet, ACK_PACKET_LENGTH, 0, (struct sockaddr*) &serveraddr, &addr_size);
    if (recvfd - 4 < 0)
    {
      printf("Data corrupted\n");
      return -1;
    }

    op_code = packet[1];
    recieved_ack = *(uint16_t*)&packet[2];    /* Read in Big Endian */
    recieved_ack = ntohs(recieved_ack);       /* Convert to Little Endian * Intel processors */
    if (op_code == 5)
    {
      Get_TFTP_error(packet);
      return -1;
    }
    if (ack_count == recieved_ack)
    {
      ack_count++;
      size = fread(&packet[4], 1, 512, p);
    }
    else
    {
      sendfd = sendto(socketfd, packet, PACKET_SIZE, 0, (struct sockaddr*) &serveraddr, addr_size);
      printf("Re-sending DATA\n");
      continue;
    }


    Build_DATA_Packet(packet, ack_count);

    sendfd = sendto(socketfd, packet, size + 4, 0, (struct sockaddr*) &serveraddr, addr_size);

    if (size + 4 != 516)
    {
      recvfd = recvfrom(socketfd, packet, ACK_PACKET_LENGTH, 0, (struct sockaddr*) &serveraddr, &addr_size);
      last_packet = 1;
    }

  }
  fclose(p);
  return 0;
}
