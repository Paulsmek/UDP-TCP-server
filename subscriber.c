#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "utils.h"

#define MAX_PFDS 32
struct chat_packet sent_packet;
struct pollfd pfds[MAX_PFDS];
int nfds = 0;
int sockfd;  /* client socket */
int ok = 0;

void run_client(int sockfd) {
  struct pollfd stdin_pfd;
  stdin_pfd.fd = STDIN_FILENO;
  stdin_pfd.events = POLLIN;
  pfds[nfds++] = stdin_pfd;

  pfds[nfds].fd = sockfd;
  pfds[nfds].events = POLLIN;
  nfds++;

  char buf[MSG_MAXSIZE + 1];
  memset(buf, 0, MSG_MAXSIZE + 1);

  while (1) {
    poll(pfds, nfds, -1);

    if (pfds[0].revents & POLLIN) {
      // Read user input from stdin
      if (fgets(buf, sizeof(buf), stdin) == NULL) {
        break;
      }

      buf[strcspn(buf, "\n")] = '\0'; // Remove newline character

      // Parse user command
      char *command = strtok(buf, " ");
      if (command == NULL) {
        continue;
      }

      if (strcmp(command, "subscribe") == 0) {
        char *topic = strtok(NULL, " ");
        if (topic != NULL) {
          printf("Subscribed to topic %s\n", topic);
        }
      } else if (strcmp(command, "unsubscribe") == 0) {
        char *topic = strtok(NULL, " ");
        if (topic != NULL) {
          printf("Unsubscribed from topic %s\n", topic);
        }
      } else if (strcmp(command, "exit") == 0) {
        break;
      }
    }

    if (pfds[1].revents & POLLIN) {
      // Receive message from server
      int bytes_received = recv_all(sockfd, buf, sizeof(buf));
      if (bytes_received <= 0) {
        break;
      }
    }
  }

  close(sockfd);
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);
  if (argc != 4) {
    return 1;
  }

  uint16_t port;
  int rc = sscanf(argv[3], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  //  Obtain a TCP socket for server connection
  const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(sockfd < 0, "socket");

  // Write in serv_addr server address, address family and port
  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
  DIE(rc <= 0, "inet_pton");

  // Connect to server
  rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "connect");
  
  sent_packet.len = strlen(argv[1]) + 1;
  strcpy(sent_packet.message, argv[1]);
  send_all(sockfd, &sent_packet, sizeof(sent_packet));
  run_client(sockfd);

  // Close connection
  close(sockfd);
  return 0;
}
