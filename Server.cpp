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
#include <algorithm>
#include <vector>
#include <string>
#include "time.h"
#include "Convert.cpp"

#define MIN_USERS 2
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
    for (int clientFd : connected_clients) // оповещаем о старте игры
    {
        send(clientFd, "Game start", 127, 0);
    }

    while (true) // Цикл аукционов
    {
        int auctionDelay = 10 + rand() % 30; // время до старта аукциона (от 10 до 30)
        int clientDecisionTime = 5 + rand() % 15; // время на решение пользователя (от 5 до 15)
        //int productPrice = rand() % 1000 + 500; // всегда в рублях

        printf("Auction will start in %i seconds\n", auctionDelay);
        sleep(auctionDelay); // Ждём время до старта аукциона
        for (int clientFd : connected_clients) // оповещение всех подключённых пользователей
        {
            send(clientFd, "Auction started", 127, 0);  // оповещение об аукционе
            send(clientFd, to_string(clientDecisionTime).c_str(), 127, 0); // время на решение
        }
        printf("Auction started: %i seconds for users decision\n", clientDecisionTime);
        sleep(clientDecisionTime + 1); // ждём окончания аукциона

        int bestPrice = -1;
        int bestPriceFd = -1;
        vector<int> playingPlayersFd = connected_clients;
        while (playingPlayersFd.size() > 1) // Цикл пока не остался единственный играющий игрок
        {
            for (int clientFd : playingPlayersFd) 
            {
                char buf[128];
                recv(clientFd, &buf, 127, 0); // получаем ответы игроков
                printf("%i client decision: %s\n", clientFd, buf); // выводим в консоль
                if (strcmp(buf, "pas") != 0) // если не пас
                {
                    int price = atoi(buf); // переводим в число

                    if (price > bestPrice) // если цена пользователя больше предыдущей лучшей
                    {
                        bestPrice = price; // лучшая цена равна этой
                        bestPriceFd = clientFd; // ID клиента лучшей цены
                    }
                }
                else
                {
                    playingPlayersFd.erase(find(playingPlayersFd.begin(), playingPlayersFd.end(), clientFd)); // иначе если спасовал удаляем из играющих
                }
            }

            if (playingPlayersFd.size() == 1) break; // проверяем если остался 1 пользователь, то выходим из цикла

            for (int clientFd : playingPlayersFd) // оставшиеся
            {
                char message[128];
                int new_decision_time = 5 + rand() % 10; // Время на решение пользователя от 5 до 10
                if (clientFd == bestPriceFd) // Если его цена оказалась лучшей, то оповещаем об этом
                {
                    printf("%i client has the best price: %i\n", clientFd, bestPrice);
                    sprintf(message, "T");
                }
                else // Если нет, то отсылаем ему оповещение об этом и новую минимальную цену
                {
                    int new_min_price = bestPrice * 1.2; // минимальная цена равна лучше умноженной на 1.2
                    sprintf(message, "F%i", new_min_price);
                }
                send(clientFd, message, 127, 0); // отсылаем сообщение
                send(clientFd, to_string(new_decision_time).c_str(), 127, 0); // отсылаем время на решение
            }
        }
        
        send(bestPriceFd, "You win!", 127, 0); // Когда цикл закончился (остался 1 в аукционе), то присылаем ему сообщение о победе
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

    printf("Server started on %i port\n", portno);
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
