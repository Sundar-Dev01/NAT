/*
 * client.c - UDP client for the NAT/PAT demonstration.
 *
 * Real sockets use loopback; the private/public addresses shown are
 * simulated network identities for explaining NAT.
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define NAT_IP       "127.0.0.1"   /* Actual local socket address */
#define NAT_PORT     5000
#define CLIENT_SIM_IP "192.168.1.10"
#define SERVER_SIM_IP "198.51.100.20"
#define SERVER_SIM_PORT 8080
#define BUFFER_SIZE 1024

int main(void)
{
    int client_socket;
    struct sockaddr_in nat_address;
    char message[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    socklen_t address_length = sizeof(nat_address);

    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&nat_address, 0, sizeof(nat_address));
    nat_address.sin_family = AF_INET;
    nat_address.sin_port = htons(NAT_PORT);
    if (inet_pton(AF_INET, NAT_IP, &nat_address.sin_addr) != 1) {
        fprintf(stderr, "Invalid NAT address\n");
        close(client_socket);
        return EXIT_FAILURE;
    }

    printf("\n========== UDP NAT CLIENT ==========\n");
    printf("Simulated private IP : %s\n", CLIENT_SIM_IP);
    printf("Default gateway      : 192.168.1.1\n");
    printf("Destination server   : %s:%d\n", SERVER_SIM_IP, SERVER_SIM_PORT);
    printf("----------------------------------\n");

    printf("Enter a message (or 'quit' to exit): ");
    if (fgets(message, sizeof(message), stdin) == NULL) {
        close(client_socket);
        return EXIT_FAILURE;
    }
    message[strcspn(message, "\n")] = '\0';

    if (strcmp(message, "quit") == 0) {
        close(client_socket);
        return EXIT_SUCCESS;
    }

    printf("\n[CLIENT] Sending message to gateway 192.168.1.1...\n");
    printf("[CLIENT] Destination remains %s:%d in our simulated packet.\n",
           SERVER_SIM_IP, SERVER_SIM_PORT);

    if (sendto(client_socket, message, strlen(message) + 1, 0,
                (struct sockaddr *)&nat_address, sizeof(nat_address)) < 0) {
        perror("sendto");
        close(client_socket);
        return EXIT_FAILURE;
    }

    printf("[CLIENT] Message sent. Waiting for NAT/server response...\n");

    if (recvfrom(client_socket, response, sizeof(response), 0,
                 (struct sockaddr *)&nat_address, &address_length) < 0) {
        perror("recvfrom");
        close(client_socket);
        return EXIT_FAILURE;
    }

    printf("\n========== RESPONSE RECEIVED ==========\n");
    printf("[CLIENT] Server reply: %s\n", response);
    printf("====================================\n");

    close(client_socket);
    return EXIT_SUCCESS;
}
