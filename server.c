/*
 * server.c - UDP external-server simulator for the NAT/PAT demonstration.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP       "127.0.0.1"   /* Actual local socket */
#define SERVER_PORT     8080
#define SERVER_SIM_IP  "198.51.100.20"
#define BUFFER_SIZE    2048
#define DEMO_DELAY_SEC 2

int main(void)
{
    int server_socket;
    struct sockaddr_in server_address, nat_address;
    socklen_t nat_length;
    char buffer[BUFFER_SIZE];
    char response[] = "Hello from the Server! Your message was received successfully.";

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("bind");
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf("\n========== EXTERNAL SERVER ==========\n");
    printf("Simulated server IP : %s\n", SERVER_SIM_IP);
    printf("Actual socket       : %s:%d\n", SERVER_IP, SERVER_PORT);
    printf("Waiting for NAT-forwarded messages...\n");

    for (;;) {
        nat_length = sizeof(nat_address);
        if (recvfrom(server_socket, buffer, sizeof(buffer), 0,
                    (struct sockaddr *)&nat_address, &nat_length) < 0) {
            perror("recvfrom");
            continue;
        }
        buffer[sizeof(buffer) - 1] = '\0';

        printf("\n\n========== REQUEST RECEIVED ==========\n");

        /* Parse simulated NAT identity and message from payload metadata. */
        char *public_line = strstr(buffer, "NAT_PUBLIC=");
        char *message_line = strstr(buffer, "\nMESSAGE=");

        if (public_line != NULL && message_line != NULL) {
            *message_line = '\0';
            public_line += strlen("NAT_PUBLIC=");
            message_line += strlen("\nMESSAGE=");
            printf("[SERVER] Request received from simulated NAT public address: %s\n",
                   public_line);
            printf("[SERVER] Message: %s\n", message_line);
        } else {
            printf("[SERVER] Received data: %s\n", buffer);
            printf("[SERVER] Sender is the NAT simulator's local socket.\n");
        }

        printf("[SERVER] Actual sender socket: %s:%u (loopback simulation)\n",
               inet_ntoa(nat_address.sin_addr),
               (unsigned int)ntohs(nat_address.sin_port));

        printf("[SERVER] Processing request...\n");
        sleep(DEMO_DELAY_SEC); /* Intentional pause for live explanation */

        printf("[SERVER] Sending response back to NAT router...\n");
        if (sendto(server_socket, response, strlen(response) + 1, 0,
                   (struct sockaddr *)&nat_address, nat_length) < 0) {
            perror("sendto");
            continue;
        }

        printf("[SERVER] Response sent.\n");
    }

    close(server_socket);
    return EXIT_SUCCESS;
}
