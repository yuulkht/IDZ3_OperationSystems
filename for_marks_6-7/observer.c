#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define RCVBUFSIZE 256

void DieWithError(char *errorMessage);
void HandleSigint(int sig);

int sock;

int main(int argc, char *argv[])
{
    struct sockaddr_in echoServAddr;
    unsigned short echoServPort;
    char *servIP;
    char observerID[] = "observer";
    char echoBuffer[RCVBUFSIZE];
    int bytesRcvd, totalBytesRcvd;

    if (argc != 3)
    {
        fprintf(stderr, "Использование: %s <IP сервера> <Порт сервера>\n", argv[0]);
        exit(1);
    }

    servIP = argv[1];
    echoServPort = atoi(argv[2]);

    signal(SIGINT, HandleSigint);

    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() не удалось");

    memset(&echoServAddr, 0, sizeof(echoServAddr));
    echoServAddr.sin_family = AF_INET;
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);
    echoServAddr.sin_port = htons(echoServPort);

    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("connect() не удалось");

    if (send(sock, observerID, strlen(observerID), 0) != strlen(observerID))
        DieWithError("send() не удалось");

    totalBytesRcvd = 0;
    printf("Получено: \n");

    while ((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE - 1, 0)) > 0) {
        totalBytesRcvd += bytesRcvd;
        echoBuffer[bytesRcvd] = '\0';
        printf("%s", echoBuffer);
    }

    printf("Получение данных завершено.\n");

    close(sock);
    exit(0);
}

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

void HandleSigint(int sig)
{
    printf("\nЗавершение работы клиента-наблюдателя\n");
    close(sock);
    exit(0);
}
