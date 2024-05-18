#include <stdio.h>      // Для printf() и fprintf()
#include <sys/socket.h> // Для socket(), connect(), и т.д.
#include <arpa/inet.h>  // Для sockaddr_in и inet_addr()
#include <stdlib.h>     // Для atoi() и exit()
#include <string.h>     // Для memset() и strlen()
#include <unistd.h>     // Для close()
#include <signal.h>     // Для signal()

#define RCVBUFSIZE 128  // Размер буфера для получения данных

void DieWithError(char *errorMessage); // Функция обработки ошибок
void HandleSigint(int sig);            // Функция обработки сигнала

int sock; // Дескриптор сокета клиента

int main(int argc, char *argv[])
{
    struct sockaddr_in echoServAddr; // Адрес сервера
    unsigned short echoServPort;     // Порт сервера
    char *servIP;                    // IP-адрес сервера
    char *playerNumber;              // Номер игрока
    char echoBuffer[RCVBUFSIZE];     // Буфер для получения данных
    int bytesRcvd, totalBytesRcvd;   // Переменные для подсчета полученных байт
    int sumPoints = 0;               // Переменная для суммы очков

    // Проверка правильного количества аргументов
    if (argc != 4)
    {
        fprintf(stderr, "Использование: %s <IP сервера> <Порт сервера> <Номер игрока>\n", argv[0]);
        exit(1);
    }

    servIP = argv[1];              // Первый аргумент: IP сервера
    echoServPort = atoi(argv[2]);  // Второй аргумент: порт сервера
    playerNumber = argv[3];        // Третий аргумент: номер игрока

    signal(SIGINT, HandleSigint);  // Установка обработчика сигнала SIGINT

    // Создание TCP сокета
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() не удалось");

    // Заполнение структуры адреса сервера
    memset(&echoServAddr, 0, sizeof(echoServAddr));   // Обнуление структуры
    echoServAddr.sin_family = AF_INET;                // Семейство адресов Интернет
    echoServAddr.sin_addr.s_addr = inet_addr(servIP); // IP адрес сервера
    echoServAddr.sin_port = htons(echoServPort);      // Порт сервера

    // Подключение к серверу
    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("connect() не удалось");

    // Отправка номера игрока серверу
    if (send(sock, playerNumber, strlen(playerNumber), 0) != strlen(playerNumber))
        DieWithError("send() отправил другое количество байт, чем ожидалось");

    totalBytesRcvd = 0;
    printf("Получено: \n");
    // Получение данных от сервера
    while ((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE - 1, 0)) > 0) {
        totalBytesRcvd += bytesRcvd;
        echoBuffer[bytesRcvd] = '\0'; // Обнуление конца строки

        // Извлечение последнего символа и преобразование его в целое число
        char lastChar = echoBuffer[strlen(echoBuffer) - 2];
        int result = lastChar - '0';
        sumPoints += result;

        printf("%s", echoBuffer);
    }

    // Вывод общего количества очков
    printf("Ваш результат в количестве очков: %d\n", sumPoints);

    close(sock); // Закрытие сокета
    exit(0);
}

void DieWithError(char *errorMessage)
{
    perror(errorMessage); // Вывод сообщения об ошибке
    exit(1);
}

void HandleSigint(int sig)
{
    printf("\nЗавершение работы клиента\n");
    close(sock); // Закрытие сокета
    exit(0);
}
