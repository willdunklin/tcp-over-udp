#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT      8080
#define DATA_SIZE 4096

typedef struct tcp_datagram {
    uint32_t sequence;
    uint32_t acknowledgement;
    uint8_t syn;
    uint8_t ack;
    uint8_t data[DATA_SIZE];
} tcp_datagram;

int sockfd;
char buffer[sizeof(tcp_datagram)];
struct sockaddr_in servaddr, cliaddr;

uint32_t last_ack, last_seq;

void print_datagram(tcp_datagram* data) {
    printf("{%d, %d, %d, %d, %s}\n", data->sequence, data->acknowledgement, data->syn, data->ack, data->data);
}

void print_buffer(char* label) {
    printf(label);
    tcp_datagram data;
    memcpy(&data, buffer, sizeof(tcp_datagram));
    print_datagram(&data);
}

int udp_recv() {
    int len = sizeof(servaddr);
    recvfrom(sockfd, (char *)buffer, sizeof(tcp_datagram), MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
    print_buffer("received: ");
}

int udp_send() {
    print_buffer("sending: ");
    sendto(sockfd, (char *)buffer, sizeof(tcp_datagram), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
}

int establish() {
    // Receive
    udp_recv();
    // Unpack msg
    tcp_datagram msg, new_msg;
    memcpy(&msg, buffer, sizeof(tcp_datagram));

    // Set datagram
    last_ack = msg.acknowledgement = msg.sequence + 1;
    last_seq = msg.sequence = 10391; // Should be random
    msg.syn = 1;
    msg.ack = 1;

    do {
        memcpy(buffer, &msg, sizeof(tcp_datagram));
        udp_send();

        // Wait for ack
        udp_recv();
        memcpy(&new_msg, buffer, sizeof(tcp_datagram));
    }while (new_msg.acknowledgement != last_seq + 1 && new_msg.sequence != last_ack && new_msg.ack != 1);
}

void init() {
    // Close if socket failure
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port        = htons(PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    struct timeval tv;
    tv.tv_sec = 10; // Timeout in seconds
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
}

int main() {
    // Initialize UDP connection (source: https://www.geeksforgeeks.org/udp-server-client-implementation-c/)
    init();
     
    establish();
    // sleep(10);
    return 0;
}