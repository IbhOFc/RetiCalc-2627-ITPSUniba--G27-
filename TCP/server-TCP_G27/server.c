#if defined WIN32
#include <winsock.h>
typedef int socklen_t;
#else
#define closesocket close
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define PROTOPORT 5193 
#define QLEN 5          
#define BUFFER_SIZE 512

void clearwinsock() {
#if defined WIN32
    WSACleanup();
#endif
}


int recv_all(int sock, void *buffer, size_t len) {
    size_t total_received = 0;
    char *ptr = (char *)buffer;
    while (total_received < len) {
        int bytes = recv(sock, ptr + total_received, len - total_received, 0);
        if (bytes <= 0) {
            return bytes; 
        }
        total_received += bytes;
    }
    return (int)total_received;
}

int main(void) {
#if defined WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("Errore in WSAStartup()\n");
        return 1;
    }
#endif

    int my_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (my_socket < 0) {
        perror("Creazione socket fallita");
        clearwinsock();
        return 1;
    }

    int opt = 1;
    setsockopt(my_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in sad;
    memset(&sad, 0, sizeof(sad));
    sad.sin_family = AF_INET;
    sad.sin_addr.s_addr = INADDR_ANY;
    sad.sin_port = htons(PROTOPORT);

    if (bind(my_socket, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
        perror("bind() fallita");
        closesocket(my_socket);
        clearwinsock();
        return 1;
    }

    if (listen(my_socket, QLEN) < 0) {
        perror("listen() fallita");
        closesocket(my_socket);
        clearwinsock();
        return 1;
    }

    printf("Server avviato e in ascolto sulla porta %d...\n", PROTOPORT);

    struct sockaddr_in cad;
    socklen_t client_len;

    while (1) {
        client_len = sizeof(cad);
        int client_socket = accept(my_socket, (struct sockaddr *)&cad, &client_len);
        if (client_socket < 0) {
            perror("accept() fallita");
            continue;
        }

        printf("connesso con il client il cui nome IP: %s\n", inet_ntoa(cad.sin_addr));

        const char *welcome_msg = "connessione avvenuta";
        if (send(client_socket, welcome_msg, strlen(welcome_msg) + 1, 0) < 0) {
            perror("send() messaggio di benvenuto fallita");
            closesocket(client_socket);
            continue;
        }

        while (1) {
            int32_t net_nums[3];
            int status = recv_all(client_socket, net_nums, sizeof(net_nums));
            if (status <= 0) {
                printf("Client disconnesso.\n");
                break;
            }

            int32_t n1 = ntohl(net_nums[0]);
            int32_t n2 = ntohl(net_nums[1]);
            int32_t n3 = ntohl(net_nums[2]);

            int32_t res = n1 + n2 - n3;
            printf("la somma di %d+%d-%d=%d\n", n1, n2, n3, res);

            char out_buf[BUFFER_SIZE];
            if (res == 100) {
                strcpy(out_buf, "QUIT");
            } else {
                snprintf(out_buf, sizeof(out_buf), "%d", res);
            }

            if (send(client_socket, out_buf, strlen(out_buf) + 1, 0) < 0) {
                perror("send() risposta fallita");
                break;
            }

            if (res == 100) {
                break;
            }
        }

        closesocket(client_socket);
    }

    closesocket(my_socket);
    clearwinsock();
    return 0;
}