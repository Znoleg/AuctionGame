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

void* new_thread(void* clientid)
{
    int id = *static_cast<int*>(clientid);
    printf("%i%s\n", id, " client connected");
    for (int i = 0; i < 10; i++)
    {
        printf("%i ", i);
        usleep(1);
    }
    printf("%i%s\n", id, " client work ends");
    return NULL;
}

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

void* auction_thread(void*)
{
    while (true)
    {
        int auctionDelay = 10; // время до старта аукциона
        int clientDecisionTime = rand() % 30 + 5; // время на решение пользователя
        //int productPrice = rand() % 1000 + 500; // всегда в рублях

        printf("Auction will start in %i seconds\n", auctionDelay);
        sleep(auctionDelay);
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
                int price = atoi(buf + 1); // получаем валюту+цену
                if (buf[0] == 'R') // рубль
                {

                }
                else if (buf[0] == 'U') // доллар
                {
                    price = UsdToRub(price);
                }
                else if (buf[0] == 'E') // евро
                {
                    price = EurToRub(price);
                }

                if (price > bestPrice) // если цена пользователя больше предыдущей лучшей
                {
                    bestPrice = price;
                    bestPriceId = clientId;
                }
            }
        }
        for (int clientId : connected_clients)
        {
            char message[128];
            if (clientId != bestPriceId) sprintf(message, "FYou didn't bought goods :("); // не купил
            else sprintf(message, "TYou bought goods for %i rubles!", bestPrice); // купил
            printf("%i client will get the information: %s", clientId, message);
            send(clientId, message, 127, 0); // отправляем результат
        }
    }
}

int main(int argc, char* argv[])
{
    srand(time(NULL));
    in_port_t portno = 2080;
    int sockfd, newclient;
    socklen_t clilen;
    sockaddr_in cli_addr;
    if (argc == 2)
    {
        portno = atoi(argv[1]);
    }
    sockfd = create_socket_serv();
    create_connection_server(portno, sockfd);
    start_listening(sockfd);

    clilen = sizeof(cli_addr);

    printf("Server started\n");
    printf("Waiting for %i users to connect\n", MIN_USERS);
    int clientNum = 0;
    pthread_t auction_thread_id;
    while (true)
    {
        newclient = accept(sockfd, (sockaddr*)&cli_addr, &clilen);
        if (newclient < 0) error("Error on accept\n");
        connected_clients.push_back(newclient);
        clientNum++;
        if (MIN_USERS - clientNum > 0)
        {
            printf("Waiting for %i users to connect\n", MIN_USERS - clientNum);
        }
        else
        {
            printf("Minimal %i users connected! Starting a game.\n", MIN_USERS);
            pthread_create(&auction_thread_id, NULL, auction_thread, NULL);
        }
    }
}
