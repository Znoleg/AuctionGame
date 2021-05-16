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
int goods = 0;
int sockfd;

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
    while (true)
    {
        rubles += rubDelta;
        dollars += dolDelta;
        euros += euroDelta;
        sleep(1);
    }
}


void* auction_handle(void* arg)
{
    MainWindow* window = (MainWindow*)arg;
    int fd = sockfd;
    while (true)
    {
        char buf[128];
        if (recv(fd, buf, 127, 0) > 0)
        {
            if (strcmp(buf, "Auction started") == 0)
            {
                memset(buf, 0, 127); // сброс буфера
                recv(fd, buf, 127, 0); // получаем время на решение
                int decision_time = atoi(buf); // переводим в инт
                string result;
                double given_price;
                window->start_auction(decision_time, result, given_price); // вызываем функцию окна которая получает введённые данные и возвращает результаты
                //sleep(decision_time + 1); // ждём время на размышление чтобы предыдущая ф-ция закончилась
                memset(buf, 0, 127); // сбрасываем буфер
                strcpy(buf, result.c_str()); // записываем результирующую строку в буфер
                send(fd, buf, 127, 0); // отправляем результат (валюта и цена или пасс)
                memset(buf, 0, 127); // сбрасываем буфер
                recv(fd, buf, 127, 0); // получаем результат аукциона

                if (buf[0] == 'T') // если мы выиграли
                {
                    goods++;
                    if (result[0] == 'R') rubles -= given_price; // определяем валюту из строки и уменьшаем нужную валюту
                    else if (result[0] == 'U') dollars -= given_price;
                    else if (result[0] == 'E') euros -= given_price;
                }
                buf[0] = ' '; // Убираем T/F из строки
                window->update_goods_label();
                window->update_money_label();
                window->print_warning_message(buf); // выписываем результирующее сообщение в окно
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

    int portno = 2080;
    hostent *server;
    sockfd = create_socket();
    server = get_host_by_hostname();
    server_connect(portno, sockfd, server);

    pthread_t thread, auction_thr;
    pthread_create(&thread, 0, money_increase, 0);
    pthread_create(&auction_thr, NULL, auction_handle, (void**)&w);

    return a.exec();
}
