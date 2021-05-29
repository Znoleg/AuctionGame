#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <threads.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <vector>
#include <string>
#include "time.h"
#include "Convert.cpp"

#define MIN_USERS 1
using namespace std;
std::vector<int> connected_clients;

int create_socket_serv()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error opening socket!");
    return sockfd;
}

sockaddr_in create_connection_server(in_port_t port, int sockfd)
{
    sockaddr_in serv_addr;
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET; // TCP
    serv_addr.sin_port = htons(port);
    if (bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) error ("Error on binding!");
}

void start_listening(int sockfd)
{
    if (listen(sockfd, 5) < 0) error("Error on listening start");
}

// Поток игры
void* auction_thread(void*)
{
    while (true)
    {
        int auctionDelay = 10; // время до старта аукциона
        int clientDecisionTime = rand() % 30 + 5; // время на решение пользователя
        //int productPrice = rand() % 1000 + 500; // всегда в рублях

        printf("Auction will start in %i seconds\n", auctionDelay);
        sleep(auctionDelay); // Ждём время до старта аукциона
        for (int clientId : connected_clients) // оповещение всех подключённых пользователей
        {
            send(clientId, "Auction started", 127, 0);  // оповещение об аукционе
            send(clientId, to_string(clientDecisionTime).c_str(), 127, 0); // время на решение
        }
        printf("Auction started: %i for users decision", clientDecisionTime);
        sleep(clientDecisionTime + 1); // ждём окончания аукциона

        int bestPrice = -1;
        int bestPriceId = -1;
        for (int clientId : connected_clients)
        {
            char buf[128];
            recv(clientId, &buf, 127, 0); // получаем решение пользователя
            printf("%i client decision: %s", clientId, buf);
            if (strcmp(buf, "pas") != 0) // если не пас
            {
                int price = atoi(buf + 1); // получаем цену
                if (buf[0] == 'R') // валюта рубль
                {

                }
                else if (buf[0] == 'U') // доллар
                {
                    price = UsdToRub(price); // конвертируем в рубли (чтобы сравнить лучшую цену)
                }
                else if (buf[0] == 'E') // евро
                {
                    price = EurToRub(price); // конвертируем в рубли
                }

                if (price > bestPrice) // если цена пользователя больше предыдущей лучшей
                {
                    bestPrice = price; // лучшая цена равна этой
                    bestPriceId = clientId; // ID клиента лучшей цены
                }
            }
        }
        for (int clientId : connected_clients) // Отсылаем результаты всем клиентам
        {
            char message[128];
            if (clientId != bestPriceId) sprintf(message, "FYou didn't bought goods :("); // не купил
            else sprintf(message, "TYou bought goods for %i rubles!", bestPrice); // купил
            printf("%i client will get the information: %s", clientId, message); // вывод в консоль
            send(clientId, message, 127, 0); // отправляем результат
        }
    }
}

int main(int argc, char* argv[])
{
    srand(time(NULL)); // ставим сид рандома
    in_port_t portno = 2400; // стандартный порт
    int sockfd, newclient;
    socklen_t clilen;
    sockaddr_in cli_addr;
    if (argc == 2)
    {
        portno = atoi(argv[1]); // если предоставлен порт из командной строки то переводим его в число и используем
    }
    
    sockfd = create_socket_serv(); // создаём сокет
    create_connection_server(portno, sockfd); // создаём соединение
    start_listening(sockfd); // позволяем подключиться клиентам

    clilen = sizeof(cli_addr);

    printf("Server started\n");
    printf("Waiting for %i users to connect\n", MIN_USERS); // MIN_USERS - минимальное кол-во пользователей для игры
    
    int clientNum = 0;
    pthread_t auction_thread_id;
    while (true)
    {
        newclient = accept(sockfd, (sockaddr*)&cli_addr, &clilen); // Принимаем клиента
        if (newclient < 0) error("Error on accept\n"); // Если ошибка
        
        connected_clients.push_back(newclient); // Заносим клиента в вектор connecter_clients
        clientNum++; // Номер клиента
        if (MIN_USERS - clientNum > 0) // Если достаточное число не подключилось
        {
            printf("Waiting for %i users to connect\n", MIN_USERS - clientNum);
        }
        else // Если набралось,то начинаем игру
        {
            printf("Minimal %i users connected! Starting a game.\n", MIN_USERS);
            pthread_create(&auction_thread_id, NULL, auction_thread, NULL); // создаём поток игры
        }
    }
}
