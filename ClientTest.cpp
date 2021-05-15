#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <strings.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include "Convert.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QLabel>

using namespace std;
#define STD_PORT 2048

double rubles = 0;
double dollars = 0;
double euros = 0;
bool auction = false;

int create_socket()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error opening socket!");
    return sockfd;
}

hostent* get_host_by_hostname(string name = "")
{
    if (name == "")
    {
        char buff[64];
        gethostname(buff, 63);
        return gethostbyname(buff);
    }
    else return gethostbyname(name.c_str());
}

hostent* get_host_by_ip(string ipstr = "127.0.0.1")
{
    in_addr ip;
    hostent *hp;
    if (!inet_aton(ipstr.c_str(), &ip)) error("Can't parse IP address!");
    if ((hp = gethostbyaddr((const void*)&ip, sizeof(ip), AF_INET)) == NULL) error("No server associated with " + ipstr);
    return hp;
}

sockaddr_in server_connect(in_port_t port, int sockfd, hostent* server)
{
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    bcopy((char*)server->h_addr_list[0], (char*)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_family = AF_INET; // TCP
    serv_addr.sin_port = htons(port);
    //if (bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) error ("Error on binding!");
    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) error("Error connecting");
    return serv_addr;
}

void* money_increase(void*)
{
    const double rubDelta = 1;
    const double dolDelta = 0.1;
    const double euroDelta = 0.01;
    while (!auction)
    {
        rubles += rubDelta;
        dollars += dolDelta;
        euros += euroDelta;
        sleep(1);
    }
}


void* auction_handle(void* arg)
{
    int fd = *(int*)arg;
    while (true)
    {
        char buf[128];
        if (recv(fd, buf, 127, 0) > 0)
        {
            if (buf == "Auction started")
            {
                recv(fd, buf, 127, 0);
                int decision_time = atoi(buf);
                int given_price = 100;
                send(fd, "R100", 127, 0);
                
                sleep(decision_time);

                recv(fd, buf, 127, 0);
                if (buf[0] == 'T')
                {
                    rubles -= given_price;
                }
                buf[0] = ' ';
                printf("%s", buf);
            }
        }
        
    }
}

int main(int argc, char *argv[])
{

    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "AuctionGame_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();

    int sockfd, portno = 2000;
    hostent *server;
    sockfd = create_socket();
    server = get_host_by_hostname();
    server_connect(portno, sockfd, server);

    pthread_t thread;
    pthread_create(&thread, 0, money_increase, 0);

    return a.exec();
}
