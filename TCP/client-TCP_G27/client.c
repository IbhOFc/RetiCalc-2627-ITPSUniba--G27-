#if defined WIN32
#include <winsock.h>
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

#define BUFFER_SIZE 512

void clearwinsock() {
#if defined WIN32
    WSACleanup();
#endif
}

int main(void) {
#if defined WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("Errore in WSAStartup()\n");
        return 1;
    }
#endif

    char host_input[256];
    int port;

    // 2. Lettura da tastiera di IP dell'host e porta del server
    printf("Inserisci l'indirizzo IP del server: ");
    if (scanf("%255s", host_input) != 1) {
        fprintf(stderr, "Input IP non valido.\n");
        clearwinsock();
        return 1;
    }

    printf("Inserisci il numero di porta del server: ");
    if (scanf("%d", &port) != 1 || port <= 0 || port > 65535) {
        fprintf(stderr, "Numero di porta non valido.\n");
        clearwinsock();
        return 1;
    }


    int c_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (c_socket < 0) {
        perror("Creazione socket fallita");
        clearwinsock();
        return 1;
    }

    struct sockaddr_in sad;
    memset(&sad, 0, sizeof(sad));
    sad.sin_family = AF_INET;
    sad.sin_port = htons((uint16_t)port);

    unsigned long ip_addr = inet_addr(host_input);
    if (ip_addr != INADDR_NONE) {
        sad.sin_addr.s_addr = ip_addr;
    } else {
        struct hostent *host = gethostbyname(host_input);
        if (host == NULL) {
            fprintf(stderr, "Risoluzione host fallita per: %s\n", host_input);
            closesocket(c_socket);
            clearwinsock();
            return 1;
        }
        memcpy(&sad.sin_addr, host->h_addr_list[0], host->h_length);
    }

    if (connect(c_socket, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
        perror("Connessione al server fallita");
        closesocket(c_socket);
        clearwinsock();
        return 1;
    }

    char in_buf[BUFFER_SIZE];
    int bytes_rcvd = recv(c_socket, in_buf, sizeof(in_buf) - 1, 0);
    if (bytes_rcvd <= 0) {
        perror("Errore nella ricezione del messaggio dal server");
        closesocket(c_socket);
        clearwinsock();
        return 1;
    }
    in_buf[bytes_rcvd] = '\0';
    printf("%s\n", in_buf);

    while (1) {
        int n1, n2, n3;

        printf("Inserisci tre interi (n1 n2 n3): ");
        if (scanf("%d %d %d", &n1, &n2, &n3) != 3) {
            fprintf(stderr, "Input non valido. Inserire tre numeri interi.\n");
            int c;
            while ((c = getchar()) != '\n' && c != EOF); 
            continue;
        }

        int32_t net_nums[3];
        net_nums[0] = htonl((int32_t)n1);
        net_nums[1] = htonl((int32_t)n2);
        net_nums[2] = htonl((int32_t)n3);

        if (send(c_socket, (const char *)net_nums, sizeof(net_nums), 0) < 0) {
            perror("Invio dei dati fallito");
            break;
        }

        bytes_rcvd = recv(c_socket, in_buf, sizeof(in_buf) - 1, 0);
        if (bytes_rcvd <= 0) {
            printf("Connessione chiusa dal server.\n");
            break;
        }
        in_buf[bytes_rcvd] = '\0';
        printf("%s\n", in_buf);

        if (strcmp(in_buf, "QUIT") == 0) {
            break;
        }
    }

    closesocket(c_socket);
    clearwinsock();
   return 0;
}