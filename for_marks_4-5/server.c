#include <stdio.h>      // для printf() и fprintf()
#include <sys/socket.h> // для socket(), bind(), и connect()
#include <arpa/inet.h>  // для sockaddr_in и inet_ntoa()
#include <stdlib.h>     // для atoi() и exit()
#include <string.h>     // для memset()
#include <unistd.h>     // для close()
#include <signal.h>     // для signal()
#include <time.h>       // для srand() и rand()

#define MAXPENDING 5    // Максимальное количество ожидающих подключений
#define RCVBUFSIZE 128   // Размер буфера для получения данных

void DieWithError(char *errorMessage);  // Функция обработки ошибок
void HandleTCPClient(int clntSocket);   // Функция обработки TCP клиента
int** play_game(int numPlayers);              // Функция для получения исхода поединка
void HandleSigint(int sig);             // Функция обработки сигнала

int servSock; // Дескриптор сокета сервера
int numPlayers; // Количество игроков
int* clientSockets; // Массив сокетов клиентов



int main(int argc, char *argv[])
{
    struct sockaddr_in echoServAddr; // Локальный адрес
    struct sockaddr_in echoClntAddr; // Адрес клиента
    unsigned short echoServPort;     // Порт сервера
    unsigned int clntLen;            // Длина структуры адреса клиента

    if (argc != 3)     // Проверка правильного количества аргументов
    {
        fprintf(stderr, "Использование: %s <Порт сервера> <Количество игроков>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);  // Первый аргумент: локальный порт
    numPlayers = atoi(argv[2]);    // Второй аргумент: количество игроков

    int playerNumbers[numPlayers];

    // Установка обработчика сигнала SIGINT
    signal(SIGINT, HandleSigint);

    clientSockets = (int*)malloc(numPlayers * sizeof(int)); // Выделение памяти для сокетов клиентов

    // Создание сокета для входящих подключений
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() не удалось");

    // Создание структуры локального адреса
    memset(&echoServAddr, 0, sizeof(echoServAddr));   // Обнуление структуры
    echoServAddr.sin_family = AF_INET;                // Семейство адресов Интернет
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Любой входящий интерфейс
    echoServAddr.sin_port = htons(echoServPort);      // Локальный порт

    // Привязка к локальному адресу
    if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() не удалось");

    // Установка сокета в режим прослушивания входящих подключений
    if (listen(servSock, MAXPENDING) < 0)
        DieWithError("listen() не удалось");

    printf("Ожидание подключения %d игроков...\n", numPlayers);

    for (int i = 0; i < numPlayers; i++) // Ожидание подключения всех игроков
    {
        clntLen = sizeof(echoClntAddr);
        if ((clientSockets[i] = accept(servSock, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0)
            DieWithError("accept() не удалось");

        // Получение номера игрока от клиента
        char playerNumberStr[RCVBUFSIZE];
        int recvMsgSize;
        if ((recvMsgSize = recv(clientSockets[i], playerNumberStr, RCVBUFSIZE - 1, 0)) < 0)
            DieWithError("recv() не удалось");
        playerNumberStr[recvMsgSize] = '\0';
        playerNumbers[i] = atoi(playerNumberStr);

        printf("Игрок с номером %d подключился\n", playerNumbers[i]);
    }

    printf("Все игроки подключены. Начинается турнир...\n");

    // Моделирование турнира
    int** results = play_game(numPlayers);

    // Отправка результатов игры каждому клиенту
    for (int i = 0; i < numPlayers; i++) {
        int first_id = playerNumbers[i];
        for (int j = i + 1; j < numPlayers; j++) {
            int second_id = playerNumbers[j];

            char result_i[RCVBUFSIZE];
            char result_j[RCVBUFSIZE];

            if (results[i][j] == 1) {
                sprintf(result_i, "Ваш результат с игроком %d: %s %d\n", second_id, "победа, количество заработанных очков:", 2);
                sprintf(result_j, "Ваш результат с игроком %d: %s %d\n", first_id, "проигрыш, количество заработанных очков:", 0);
            } else if (results[i][j] == 2) {
                sprintf(result_i, "Ваш результат с игроком %d: %s %d\n", second_id, "проигрыш, количество заработанных очков:", 0);
                sprintf(result_j, "Ваш результат с игроком %d: %s %d\n", first_id, "победа, количество заработанных очков:", 2);
            } else {
                sprintf(result_i, "Ваш результат с игроком %d: %s %d\n", second_id, "ничья, количество заработанных очков:", 1);
                sprintf(result_j, "Ваш результат с игроком %d: %s %d\n", first_id, "ничья, количество заработанных очков:", 1);
            }

            if (send(clientSockets[i], result_i, strlen(result_i), 0) != strlen(result_i))
                DieWithError("send() не удалось");

            if (send(clientSockets[j], result_j, strlen(result_j), 0) != strlen(result_j))
                DieWithError("send() не удалось");
        }
    }

    printf("Турнир завершен, все игроки получили свои результаты");
    // Закрытие всех клиентских сокетов
    for (int i = 0; i < numPlayers; i++) {
        close(clientSockets[i]);
    }

    free(clientSockets); // Освобождение памяти

    close(servSock); // Закрытие серверного сокета

    return 0;
}

// Моделирование игры: 0 - ничья, 1 - выиграл первый игрок, 2 - выиграл 2 игрок
int** play_game(int numPlayers) {
    // Выделение памяти под двумерный массив результатов
    int** results = (int**)malloc(numPlayers * sizeof(int*));
    if (results == NULL) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < numPlayers; i++) {
        results[i] = (int*)malloc(numPlayers * sizeof(int));
        if (results[i] == NULL) {
            fprintf(stderr, "Ошибка выделения памяти\n");
            // Освобождение ранее выделенной памяти
            for (int j = 0; j < i; j++) {
                free(results[j]);
            }
            free(results);
            exit(EXIT_FAILURE);
        }
    }

    // Заполнение массива результатов исходами поединков
    srand(time(0));
    for (int i = 0; i < numPlayers; i++) {
        for (int j = i; j < numPlayers; j++) {
            int result = rand() % 3;
            if (i == j) {
                results[i][j] = 3;
            }
            if (result == 0) {
                results[i][j] = 0; // 0 - ничья, 1 - выигрыш, 2 - проигрыш, 3 - маркер отсутствия игры с самим собой
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
    free(clientSockets);
    exit(0);
}
