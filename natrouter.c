/*
 * natrouter.c - UDP application-level NAT/PAT simulator.
 *
 * This is a proxy-style simulation, not kernel packet NAT. The client and
 * server use loopback sockets; simulated IP/port identities are displayed.
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define NAT_LISTEN_IP       "127.0.0.1"  /* Actual local socket */
#define NAT_LISTEN_PORT     5000
#define SERVER_SOCKET_IP    "127.0.0.1"  /* Actual local server socket */
#define SERVER_SOCKET_PORT  8080
#define NAT_LAN_SIM_IP     "192.168.1.1"
#define NAT_WAN_SIM_IP     "203.0.113.1"
#define CLIENT_SIM_IP      "192.168.1.10"
#define SERVER_SIM_IP      "198.51.100.20"
#define SERVER_SIM_PORT 8080
#define FIRST_TRANSLATED_PORT 40001
#define BUFFER_SIZE        1024
#define DEMO_DELAY_SEC    2

static void pause_for_demo(void)
{
    sleep(DEMO_DELAY_SEC); /* Intentional pause for live explanation */
}

int main(void)
{
    int nat_socket;
    struct sockaddr_in nat_address, server_address, client_address, sender_address;
    socklen_t client_length, sender_length;
    char client_message[BUFFER_SIZE];
    char outgoing_message[BUFFER_SIZE + 128];
    char server_response[BUFFER_SIZE];
    unsigned int translated_port = FIRST_TRANSLATED_PORT;

    nat_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (nat_socket < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&nat_address, 0, sizeof(nat_address));
    nat_address.sin_family = AF_INET;
    nat_address.sin_port = htons(NAT_LISTEN_PORT);
    inet_pton(AF_INET, NAT_LISTEN_IP, &nat_address.sin_addr);

    if (bind(nat_socket, (struct sockaddr *)&nat_address, sizeof(nat_address)) < 0) {
        perror("bind");
        close(nat_socket);
        return EXIT_FAILURE;
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_SOCKET_PORT);
    inet_pton(AF_INET, SERVER_SOCKET_IP, &server_address.sin_addr);

    printf("\n========== NAT ROUTER SIMULATOR ==========\n");
    printf("Simulated LAN IP : %s\n", NAT_LAN_SIM_IP);
    printf("Simulated WAN IP : %s\n", NAT_WAN_SIM_IP);
    printf("Actual listener  : %s:%d\n", NAT_LISTEN_IP, NAT_LISTEN_PORT);
    printf("Waiting for client datagrams...\n");

    for (;;) {
        client_length = sizeof(client_address);
        memset(&client_address, 0, sizeof(client_address));

        if (recvfrom(nat_socket, client_message, sizeof(client_message), 0,
                     (struct sockaddr *)&client_address, &client_length) < 0) {
            perror("recvfrom (client)");
            continue;
        }
        client_message[sizeof(client_message) - 1] = '\0';

        printf("\n\n========== CLIENT TRAFFIC RECEIVED ==========\n");
        printf("[NAT] Message: %s\n", client_message);
        printf("[NAT] Simulated private source: %s\n", CLIENT_SIM_IP);
        printf("[NAT] Simulated destination  : %s:%d\n",
               SERVER_SIM_IP, SERVER_SIM_PORT);
        printf("[NAT] Actual sender socket  : %s:%u (loopback simulation)\n",
               inet_ntoa(client_address.sin_addr),
               (unsigned int)ntohs(client_address.sin_port));

        printf("\n[NAT] Creating translation entry...\n");
        pause_for_demo();

        printf("\n-------------- NAT TABLE --------------\n");
        printf("Private IP:Port        Public IP:Port\n");
        printf("%s:%u  -->  %s:%u\n",
               CLIENT_SIM_IP, (unsigned int)ntohs(client_address.sin_port),
               NAT_WAN_SIM_IP, translated_port);
        printf("-------------------------------------\n");

        /* Encapsulate demo metadata in the UDP payload for server display.
           These are simulation labels, not rewritten IP packet headers. */
        (void)snprintf(outgoing_message, sizeof(outgoing_message),
                       "NAT_PUBLIC=%s:%u\nMESSAGE=%s",
                       NAT_WAN_SIM_IP, translated_port, client_message);

        printf("[NAT] Translating source identity to %s:%u\n",
               NAT_WAN_SIM_IP, translated_port);
        printf("[NAT] Forwarding datagram to external server...\n");
        pause_for_demo();

        if (sendto(nat_socket, outgoing_message, strlen(outgoing_message) + 1, 0,
                   (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
            perror("sendto (server)");
            continue;
        }

        printf("[NAT] Request forwarded. Waiting for server reply...\n");

        sender_length = sizeof(sender_address);
        if (recvfrom(nat_socket, server_response, sizeof(server_response), 0,
                    (struct sockaddr *)&sender_address, &sender_length) < 0) {
            perror("recvfrom (server)");
            continue;
        }

        /* Accept only responses from the configured local server socket. */
        if (sender_address.sin_port != htons(SERVER_SOCKET_PORT) ||
            sender_address.sin_addr.s_addr != server_address.sin_addr.s_addr) {
            printf("[NAT] Ignored datagram from an unexpected sender.\n");
            continue;
        }

        printf("\n[NAT] Server response received.\n");
        printf("[NAT] Mapping lookup: public port %u -> private client %s\n",
               translated_port, CLIENT_SIM_IP);
        printf("[NAT] Forwarding response to original client...\n");
        pause_for_demo();

        if (sendto(nat_socket, server_response, strlen(server_response) + 1, 0,
                   (struct sockaddr *)&client_address, client_length) < 0) {
            perror("sendto (client reply)");
            continue;
        }

        printf("[NAT] Response delivered to client.\n");
        translated_port++;
        if (translated_port > 49999U) {
            translated_port = FIRST_TRANSLATED_PORT;
        }
    }

    close(nat_socket);
    return EXIT_SUCCESS;
}
