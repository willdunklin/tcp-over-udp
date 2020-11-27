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
struct sockaddr_in servaddr;

uint32_t last_ack, last_seq;

void print_datagram(tcp_datagram* data) {
    printf("{%d, %d, %d, %d, %s}\n", data->sequence, data->acknowledgement, data->syn, data->ack, data->data);
}

void print_buffer() {
    tcp_datagram data;
    memcpy(&data, buffer, sizeof(tcp_datagram));
    print_datagram(&data);
}

int tcp_recv() {
    int len = sizeof(servaddr);
    recvfrom(sockfd, (char *)buffer, sizeof(tcp_datagram), MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
    printf("receiving: ");
    print_buffer();
}

int tcp_send() {
    printf("sending: ");
    print_buffer();
    sendto(sockfd, (char *)buffer, sizeof(tcp_datagram), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
}

int establish() {
    tcp_datagram msg, new_msg;

    last_seq = msg.sequence = 953837; // Should be random
    msg.syn = 1;

    do {
        memcpy(buffer, &msg, sizeof(tcp_datagram));
        tcp_send();
    
        tcp_recv();
        memcpy(&new_msg, buffer, sizeof(tcp_datagram));
        last_ack = new_msg.sequence;
    }while (new_msg.acknowledgement != last_seq + 1 && new_msg.ack != 1 && new_msg.syn != 1);
    
    msg.acknowledgement = last_ack + 1;
    msg.sequence = new_msg.acknowledgement;
    msg.ack = 1;
    msg.syn = 0;
    memcpy(buffer, &msg, sizeof(tcp_datagram));
    tcp_send();
}

void init() {
    // Close if socket failure
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port        = htons(PORT);

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

    close(sockfd);
    return 0;
}