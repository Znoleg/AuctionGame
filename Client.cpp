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
bool money_get_flag = true;

int create_socket()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error opening socket!");
    return sockfd;
}

hostent* get_host_by_ip(string ipstr)
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
    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) error("Error connecting");
    return serv_addr;
}

const double rubDelta = 1;
const double dolDelta = 0.1;
const double euroDelta = 0.01;
// Поток увеличения денег игрока. Через секунду прибавляем каждой валюты.
void* money_increase(void*)
{
    while (true)
    {
        if (money_get_flag)
        { 
            rubles += rubDelta;
            dollars += dolDelta;
            euros += euroDelta;
            sleep(1);
        }
    }
}

// Поток игры
void* auction_handle(void* arg)
{
    MainWindow* window = (MainWindow*)arg; // Переводим аргумент типа void* в QT окно MainWindow
    int fd = sockfd; // сокет сервера

    char buf[128]; // Буфер сообщений
    recv(fd, buf, 127, 0); // Ожидаем старта игры
    pthread_t thread;
    pthread_create(&thread, 0, money_increase, 0); // Поток увеличения денег игрока
    memset(buf, 0, 127);

    while (true) // Бесконечный цикл
    {
        if (recv(fd, buf, 127, 0) > 0)
        {
            if (strcmp(buf, "Auction started") == 0) // Если получили сообщение о старту акциона
            {
                memset(buf, 0, 127); // сброс буфера
                recv(fd, buf, 127, 0); // получаем время на решение
                int decision_time = atoi(buf); // переводим в инт
                string result = "";
                double given_price_rub; // переменна для отображения цены
                money_get_flag = false; // отключаем получение денег
                window->TurnMoneyButtons(false); // отключаем кнопки получения денег
                int min_price = 0; // минимальная цена покупки товара (в начале 0)
                int valuta; //тип валюты пользователя

                while (true)
                {
                    if (result == "") // Если ответа нет (сначала нету его)
                    {
                        window->start_auction(decision_time, min_price, result, given_price_rub, valuta); // вызываем функцию окна которая получает введённые данные и возвращает результаты
                         // Если спасовали, то выходим
                        memset(buf, 0, 127); // сбрасываем буфер
                        strcpy(buf, result.c_str()); // записываем результирующую строку в буфер
                    }
                    else // Если ответ есть
                    {
                        sleep(decision_time); // То ждём других пользователей
                    }
                    send(fd, buf, 127, 0); // отправляем результат (валюта и цена или пасс)
                    if (result == "pas") break; // Если пас, то выходим из цикла игры

                    char auc_res[127];
                    recv(fd, auc_res, 127, 0); // получаем результат аукциона

                    if (auc_res[0] == 'T') // если наша цена лучшая
                    {
                        char dec_buf[127];
                        recv(fd, dec_buf, 127, 0); // получаем время на решение
                        decision_time = atoi(dec_buf); // перевод в число
                        window->print_warning_message("Лучшая цена! Ждём ответов!"); // Пишем сообщение
                    }
                    else if (auc_res[0] == 'F') // если цена не лучшая
                    {
                        min_price = atoi(auc_res + 1); // переводим минимальную цену в число
                        char dec_buf[127];
                        recv(fd, dec_buf, 127, 0); // получаем время на решение
                        decision_time = atoi(dec_buf); // перевод в число
                        result = ""; // сбрасываем ответ чтобы попросить новый ответ на 106 строчке
                        given_price_rub = 0;
                        window->print_warning_message("Новая цена лота " + to_string(min_price) + " рублей!"); // выводим сообщение
                    }
                    else if (strcmp(auc_res, "You win!") == 0) // если мы победили
                    {
                        goods++;
                        if (valuta == 0) rubles -= given_price_rub; // определяем валюту из строки и уменьшаем нужную валюту
                        else if (valuta == 1) dollars -= RubToUsd(given_price_rub);
                        else if (valuta == 2) euros -= RubToEur(given_price_rub);
                        window->update_goods_label(); // Обновляем строку кол-ва товаров в окне
                        window->update_money_label(); // Обновляем строки денег в окне
                        window->print_warning_message("Вы выиграли лот! Поздравляем!");
                        break;
                    }
                }

                window->TurnMoneyButtons(true); // Включаем кнопки получаения денег
                money_get_flag = true; // Включаем поток получения денег
                sleep(3); // Ждём чтобы игрок прочитал сообщение
                //window->print_warning_message(""); // Сбрасываем сообщение
            }
        }
        
    }
}

int main(int argc, char *argv[])
{
    /* Стандартная процедура создания окна QT */
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

    /* */

    in_port_t portno = 2400; // стандартный порт
    string ip = "127.0.0.1"; // стандартный ip

    if (argc >= 3) // если кол-во аргументов 3 и больше
    {
        ip = argv[1]; // ip равен аргументу 1
        portno = atoi(argv[2]); // port равен аргументу 2
    }

    hostent *server;
    sockfd = create_socket(); // создаём сокет
    server = get_host_by_ip(ip); // получаем хостинг по ip
    server_connect(portno, sockfd, server); // подключаемся к серверу по полученному хостингу

    pthread_t thread, auction_thr;
    pthread_create(&auction_thr, NULL, auction_handle, (void**)&w);

    return a.exec(); // Запускаем QT окно
}
