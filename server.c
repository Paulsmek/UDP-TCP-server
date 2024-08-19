#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "utils.h"

#define MAX_CONNECTIONS 32
#define MAX_UDP_PAYLOAD_SIZE 1024
int ok = 0;

// Define a structure to help with subscribtions
struct Subscriber {
    char id[11];
    char topics[30][1500];
    int num_topics;
};


void run_chat_multi_server(int tcp_listenfd, int udp_sockfd) {
  struct pollfd poll_fds[MAX_CONNECTIONS];
  int num_sockets = 3; // Start with 3
  int rc;

  struct chat_packet received_packet;
  char udp_buffer[MAX_UDP_PAYLOAD_SIZE];
  struct Subscriber subscribers[MAX_CONNECTIONS];
  int num_clients = 0;

  rc = listen(tcp_listenfd, MAX_CONNECTIONS);
  DIE(rc < 0, "listen");
  // Set the socket for stdin
  poll_fds[0].fd = STDIN_FILENO;
  poll_fds[0].events = POLLIN;

  // Set the listenfd for accepting TCP connections
  poll_fds[1].fd = tcp_listenfd;
  poll_fds[1].events = POLLIN;

  // Set the UDP socket
  poll_fds[2].fd = udp_sockfd;
  poll_fds[2].events = POLLIN;

  // Initialize the rest of the poll_fds
  for (int i = 3; i < MAX_CONNECTIONS + 2; ++i) {
    poll_fds[i].fd = -1;
  }

  while (1) {
    // Wait for activity on any of the sockets
    rc = poll(poll_fds, num_sockets, -1);
    DIE(rc < 0, "poll");

    // Check for UDP activity
    if (poll_fds[2].revents & POLLIN) {
      // Receive UDP data
      struct sockaddr_in cli_addr;
      socklen_t addr_len = sizeof(cli_addr);
      ssize_t nbytes = recvfrom(udp_sockfd, udp_buffer, sizeof(udp_buffer), 0,
                                (struct sockaddr *)&cli_addr, &addr_len);
      if (nbytes < 0) {
        perror("recvfrom");
      } else {
        //send data
      }
    }
    // Check for activity on TCP client sockets
    for (int i = 0; i < num_sockets; ++i) {
      if (poll_fds[i].revents & POLLIN) {
        if (poll_fds[i].fd == tcp_listenfd) {
          struct sockaddr_in cli_addr;
            socklen_t cli_len = sizeof(cli_addr);
            const int newsockfd = accept(tcp_listenfd, (struct sockaddr *)&cli_addr, &cli_len);
            DIE(newsockfd < 0, "accept");
            struct chat_packet recv_packet;
            int rc = recv_all(newsockfd, &recv_packet, sizeof(recv_packet));
              if (rc < 0) {
                break;
              }
            int existing_index = -1;
            for (int j = 0; j < num_clients; ++j) {
                        if (strcmp(subscribers[j].id, received_packet.message) == 0) {
                            existing_index = j;
                            break;
                        }
                    }

            if (existing_index == -1) {
              struct Subscriber new_subscriber;
              strcpy(new_subscriber.id, received_packet.message);
              new_subscriber.num_topics = 0;
              subscribers[num_clients++] = new_subscriber;
              
              // Add the new TCP socket to the poll_fds array
              poll_fds[num_sockets].fd = newsockfd;
              poll_fds[num_sockets].events = POLLIN;
              num_sockets++;
              printf("New client %s connected from %s:%d.\n", recv_packet.message
                  , inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            } else {
              // handle later
              printf("Client %s already connected.\n", recv_packet.message);
            }
        }
        else if (poll_fds[i].fd == STDIN_FILENO) {
        char input[256];
        fgets(input, sizeof(input), stdin);
        if (strcmp(input, "exit\n") == 0) {
        // Close all client connections
        for (int i = 3; i < num_sockets; ++i) {
          close(poll_fds[i].fd);
        }
        // Close the TCP listening socket
        close(tcp_listenfd);
        // Close the UDP socket
        close(udp_sockfd);
        exit(EXIT_SUCCESS);
      }
        } else {
            // Receive data from TCP client
            int rc = recv_all(poll_fds[i].fd, &received_packet, sizeof(received_packet));
            if (rc < 0) {
            perror("recv");
            close(poll_fds[i].fd);
            // Remove the closed socket from poll_fds
            poll_fds[i].fd = -1;
            } else if (rc == 0) {
            //printf("TCP client socket %d closed connection\n", poll_fds[i].fd);
            close(poll_fds[i].fd);
            // Remove the closed socket from poll_fds
            poll_fds[i].fd = -1;
            } else {
              char *topic = strtok(received_packet.message, " ");
              if (topic != NULL) {
              // Find the subscriber index based on the socket fd
              int subscriber_index = -1;
              for (int j = 0; j < num_sockets; ++j) {
                  if (poll_fds[j].fd == poll_fds[i].fd) {
                      subscriber_index = j - 3; // Adjusting for the offset of 3
                      break;
                  }
              }
              if (subscriber_index >= 0 && subscriber_index < num_clients) {
                  // Add the topic to the subscribers list
                  struct Subscriber *subscriber = &subscribers[subscriber_index];
                  if (subscriber->num_topics < 30) {
                      strcpy(subscriber->topics[subscriber->num_topics++], topic);
                  }
              } 
              } else {
              }
            }
        }
      }
    }
  }
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);
  if (argc != 2) {
    return 1;
  }
  // Parse the port from the command-line arguments
  uint16_t port;
  if (sscanf(argv[1], "%hu", &port) != 1) {
    fprintf(stderr, "Invalid port number\n");
    return 1;
  }

  // Obtain a TCP socket for receiving connections
  const int tcp_listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (tcp_listenfd < 0) {
    perror("socket");
    return 1;
  }
  
  // Set socket options for TCP socket reusability
  const int enable = 1;
  if (setsockopt(tcp_listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
    perror("setsockopt(SO_REUSEADDR) failed");
    close(tcp_listenfd);
    return 1;
  }

  // Set up the TCP server address structure
  struct sockaddr_in tcp_serv_addr;
  memset(&tcp_serv_addr, 0, sizeof(struct sockaddr_in));
  tcp_serv_addr.sin_family = AF_INET;
  tcp_serv_addr.sin_port = htons(port);
  tcp_serv_addr.sin_addr.s_addr = INADDR_ANY;

  // Bind the TCP server address to the socket
  if (bind(tcp_listenfd, (const struct sockaddr *)&tcp_serv_addr, sizeof(tcp_serv_addr)) < 0) {
    perror("bind");
    close(tcp_listenfd);
    return 1;
  }
  // Obtain a UDP socket
  const int udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (udp_sockfd < 0) {
    perror("socket");
    close(tcp_listenfd);
    return 1;
  }

  // Set up the UDP server address structure
  struct sockaddr_in udp_serv_addr;
  memset(&udp_serv_addr, 0, sizeof(udp_serv_addr));
  udp_serv_addr.sin_family = AF_INET;
  udp_serv_addr.sin_port = htons(port); // Same port as TCP
  udp_serv_addr.sin_addr.s_addr = INADDR_ANY;

  // Bind the UDP server address to the socket
  if (bind(udp_sockfd, (const struct sockaddr *)&udp_serv_addr, sizeof(udp_serv_addr)) < 0) {
    perror("bind");
    close(tcp_listenfd);
    close(udp_sockfd);
    return 1;
  }

  // Start the server with the given port for TCP and UDP
  run_chat_multi_server(tcp_listenfd, udp_sockfd);

  // Close the TCP listening socket
  close(tcp_listenfd);
  // Close the UDP socket
  close(udp_sockfd);

  return 0;
}
