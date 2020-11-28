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
#define MAX_TRIES 5

typedef struct tcp_datagram {
    uint32_t sequence;
    uint32_t acknowledgement;
    uint8_t syn;
    uint8_t ack;
    uint8_t data[DATA_SIZE];
} tcp_datagram;

tcp_datagram* create_datagram() {
    tcp_datagram* new = (tcp_datagram*)malloc(sizeof(tcp_datagram));
    memset(new, 0, sizeof(tcp_datagram));
    return new;
}

tcp_datagram *msg, *last_sent_msg, *last_recv_msg;

int sockfd;
char buffer[sizeof(tcp_datagram)];
struct sockaddr_in servaddr;

uint32_t last_ack, last_seq;

void print_datagram(tcp_datagram* data) {
    printf("{%d, %d, %d, %d, %s}\n", data->sequence, data->acknowledgement, data->syn, data->ack, data->data);
}

void print_buffer(char* label) {
    printf("%s", label);
    tcp_datagram data;
    memcpy(&data, buffer, sizeof(tcp_datagram));
    print_datagram(&data);
}

int udp_recv(tcp_datagram* datagram) {
    int len = sizeof(servaddr);
    int result = recvfrom(sockfd, (tcp_datagram *)datagram, sizeof(tcp_datagram), MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
    print_buffer("receiving: ");
    return result;
}

int udp_send(tcp_datagram* datagram) {
    print_buffer("sending: ");
    return sendto(sockfd, (tcp_datagram *)datagram, sizeof(tcp_datagram), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
}

void tcp_recv() {
    // Receive, if timeout happens exit
    if(udp_recv(msg) == -1)
        return;

    // Log the message as last received
    memcpy(last_recv_msg, msg, sizeof(tcp_datagram));

    // Handle 3 way handshake
    if(msg->syn == 1) {
        //etc
    }

    // If something is off, exit and wait for retransmission
    if(last_sent_msg->acknowledgement != msg->sequence || last_sent_msg->sequence + 1 != msg->acknowledgement || msg->ack == 1)
        return;

    // We know we've got a valid message we're acknowledging
    memset(msg, 0, sizeof(tcp_datagram));
    msg->ack = 1;
    msg->acknowledgement = last_recv_msg->sequence + 1;
    msg->sequence = last_recv_msg->acknowledgement;

    memcpy(last_sent_msg, msg, sizeof(tcp_datagram));
    udp_send(msg);
}

void tcp_send(char* data) {
    // While there is no acknowledgement retransmit
    int tries = 0;
    do {
        memset(msg, 0, sizeof(tcp_datagram));
        msg->acknowledgement = last_recv_msg->sequence + 1;
        msg->sequence = last_recv_msg->acknowledgement;
        memcpy(msg->data, data, DATA_SIZE);

        memcpy(last_sent_msg, msg, sizeof(tcp_datagram));
        udp_send(msg);
    // Loop back if the ack is invalid unless we've exhausted the number of tries
    } while (tries++ < MAX_TRIES
             && (udp_recv(msg) == -1
             || last_sent_msg->acknowledgement != msg->sequence
             || last_sent_msg->sequence + 1 != msg->acknowledgement
             || msg->ack != 1));
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

    // Set the timeouts for send/recv
    struct timeval tv;
    tv.tv_sec = 10; // Timeout in seconds
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));

    last_sent_msg = create_datagram();
    last_sent_msg->acknowledgement = 42492; // Corresponds to initial sequence number

    last_recv_msg = create_datagram();
    last_recv_msg->acknowledgement = last_sent_msg->acknowledgement;

    msg = create_datagram();
}

int main() {
    // Initialize UDP connection (source: https://www.geeksforgeeks.org/udp-server-client-implementation-c/)
    init();
    
    establish();

    close(sockfd);
    return 0;
}