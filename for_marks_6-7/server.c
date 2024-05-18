#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define MAXPENDING 5
#define RCVBUFSIZE 256

void DieWithError(char *errorMessage);
void HandleTCPClient(int clntSocket);
int** playGame(int numPlayers);
void HandleSigint(int sig);
int* getPointResults(int** results, int numPlayers);


int servSock;
int numPlayers;
int* clientSockets;
int observerSocket = -1; // Сокет для клиента-наблюдателя

int main(int argc, char *argv[])
{
    struct sockaddr_in echoServAddr;
    struct sockaddr_in echoClntAddr;
    unsigned short echoServPort;
    unsigned int clntLen;

    if (argc != 3)
    {
        fprintf(stderr, "Использование: %s <Порт сервера> <Количество игроков>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);
    numPlayers = atoi(argv[2]);

    int playerNumbers[numPlayers];

    signal(SIGINT, HandleSigint);

    clientSockets = (int*)malloc(numPlayers * sizeof(int));

    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() не удалось");

    memset(&echoServAddr, 0, sizeof(echoServAddr));
    echoServAddr.sin_family = AF_INET;
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    echoServAddr.sin_port = htons(echoServPort);

    if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() не удалось");

    if (listen(servSock, MAXPENDING) < 0)
        DieWithError("listen() не удалось");

    printf("Ожидание подключения %d игроков...\n", numPlayers);

    for (int i = 0; i < numPlayers; i++)
    {
        clntLen = sizeof(echoClntAddr);
        if ((clientSockets[i] = accept(servSock, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0)
            DieWithError("accept() не удалось");

        char playerNumberStr[RCVBUFSIZE];
        int recvMsgSize;
        if ((recvMsgSize = recv(clientSockets[i], playerNumberStr, RCVBUFSIZE - 1, 0)) < 0)
            DieWithError("recv() не удалось");
        playerNumberStr[recvMsgSize] = '\0';
        playerNumbers[i] = atoi(playerNumberStr);

        printf("Игрок с номером %d подключился\n", playerNumbers[i]);
    }

    printf("Все игроки подключены. Ожидание подключения наблюдателя...\n");

    // Ожидание подключения наблюдателя
    clntLen = sizeof(echoClntAddr);
    if ((observerSocket = accept(servSock, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0)
        DieWithError("accept() для наблюдателя не удалось");

    printf("Наблюдатель подключился.\n");

    printf("Начинается турнир...\n");

    int** results = playGame(numPlayers);

    // Отправка результатов игры каждому клиенту
    for (int i = 0; i < numPlayers; i++) {
        int first_id = playerNumbers[i];
        for (int j = i + 1; j < numPlayers; j++) {
            int second_id = playerNumbers[j];

            char result_i[RCVBUFSIZE];
            char result_j[RCVBUFSIZE];
            char observer_result[RCVBUFSIZE];

            if (results[i][j] == 1) {
                snprintf(result_i, RCVBUFSIZE, "Ваш результат с игроком %d: %s %d\n", second_id, "победа, количество заработанных очков:", 2);
                snprintf(result_j, RCVBUFSIZE, "Ваш результат с игроком %d: %s %d\n", first_id, "проигрыш, количество заработанных очков:", 0);
                snprintf(observer_result, RCVBUFSIZE, "Играет игрок %d против игрока %d: По итогам поединка игрок %d выиграл у игрока %d\n", first_id, second_id, first_id, second_id);
            } else if (results[i][j] == 2) {
                snprintf(result_i, RCVBUFSIZE, "Ваш результат с игроком %d: %s %d\n", second_id, "проигрыш, количество заработанных очков:", 0);
                snprintf(result_j, RCVBUFSIZE, "Ваш результат с игроком %d: %s %d\n", first_id, "победа, количество заработанных очков:", 2);
                snprintf(observer_result, RCVBUFSIZE, "Играет игрок %d против игрока %d: По итогам поединка игрок %d выиграл у игрока %d\n", first_id, second_id, second_id, first_id);
            } else {
                snprintf(result_i, RCVBUFSIZE, "Ваш результат с игроком %d: %s %d\n", second_id, "ничья, количество заработанных очков:", 1);
                snprintf(result_j, RCVBUFSIZE, "Ваш результат с игроком %d: %s %d\n", first_id, "ничья, количество заработанных очков:", 1);
                snprintf(observer_result, RCVBUFSIZE, "Играет игрок %d против игрока %d: По итогам поединка у игроков ничья\n", first_id, second_id);
            }

            if (send(clientSockets[i], result_i, strlen(result_i), 0) != strlen(result_i))
                DieWithError("send() не удалось");

            if (send(clientSockets[j], result_j, strlen(result_j), 0) != strlen(result_j))
                DieWithError("send() не удалось");

            // Отправка результатов наблюдателю
            
            if (send(observerSocket, observer_result, strlen(observer_result), 0) != strlen(observer_result))
                DieWithError("send() наблюдателю не удалось");
        }
    }
    
    int* points_results = getPointResults(results, numPlayers);

    for (int i = 0; i < numPlayers; i++) {
        int player_id = playerNumbers[i];
        char points_msg[RCVBUFSIZE];
        snprintf(points_msg, RCVBUFSIZE, "Игрок %d получает в итоге %d очков\n", player_id, points_results[i]);
        
        if (send(observerSocket, points_msg, strlen(points_msg), 0) != strlen(points_msg))
            DieWithError("send() наблюдателю не удалось");
    }

    printf("Турнир завершен, все игроки получили свои результаты");

    for (int i = 0; i < numPlayers; i++) {
        close(clientSockets[i]);
    }
    printf("\nЗавершение работы сервера\n");
    close(servSock);
    if (observerSocket != -1) {
        close(observerSocket);
    }
    free(clientSockets);
    exit(0);
}

int** playGame(int numPlayers) {
    int** results = (int**)malloc(numPlayers * sizeof(int*));
    if (results == NULL) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < numPlayers; i++) {
        results[i] = (int*)malloc(numPlayers * sizeof(int));
        if (results[i] == NULL) {
            fprintf(stderr, "Ошибка выделения памяти\n");
            for (int j = 0; j < i; j++) {
                free(results[j]);
            }
            free(results);
            exit(EXIT_FAILURE);
        }
    }

    srand(time(0));
    for (int i = 0; i < numPlayers; i++) {
        for (int j = i; j < numPlayers; j++) {
            int result = rand() % 3;
            if (i == j) {
                results[i][j] = 3;
            }
            if (result == 0) {
                results[i][j] = 0;
                results[j][i] = 0;
            }
            else if (result == 1) {
                results[i][j] = 1;
                results[j][i] = 2;
            } else {
                results[i][j] = 2;
                results[j][i] = 1;
            }
        }
    }

    return results;
}

int* getPointResults(int** results, int numPlayers) {
    int* points_results = (int*)malloc(numPlayers * sizeof(int));
    if (points_results == NULL) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < numPlayers; i++) {
        points_results[i] = 0;
        for (int j = 0; j < numPlayers; j++) {
            if (i != j) {
                if (results[i][j] == 1) {
                    points_results[i] += 2;
                }
                else if (results[i][j] == 0) {
                    points_results[i] += 1;
                }
            }
        }
    }
    return points_results;
}


void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

void HandleSigint(int sig)
{
    for (int i = 0; i < numPlayers; i++) {
        close(clientSockets[i]);
    }
    printf("\nЗавершение работы сервера\n");
    close(servSock);
    if (observerSocket != -1) {
        close(observerSocket);
    }
    free(clientSockets);
    exit(0);
}
