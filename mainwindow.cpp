#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "unistd.h"
#include "Convert.h"
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QLCDNumber>
#include <QComboBox>

void enable_auc_form();
void disable_auc_form();

QLabel* rubLabel;
QLabel* usdLabel;
QLabel* eurLabel;
QLabel* warningLabel;
QLineEdit* rubConvert, *usdConvert, *eurConvert;

QGridLayout* aucForm;
QLCDNumber* aucTimeLeft;
QLineEdit* aucBetValue;
QComboBox* aucCurrency;
QLabel* aucLabel;

QLabel* goodsCntLabel;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* Здесь получаем все элементы окна с помощью поиска по имени */
    rubLabel = findChild<QLabel*>("RubleValue");
    usdLabel = findChild<QLabel*>("UsdValue");
    eurLabel = findChild<QLabel*>("EurValue");

    rubConvert = findChild<QLineEdit*>("RubConvertCnt");
    usdConvert = findChild<QLineEdit*>("UsdConvertCnt");
    eurConvert = findChild<QLineEdit*>("EurConvertCnt");

    warningLabel = findChild<QLabel*>("WarningLabel");

    aucForm = findChild<QGridLayout*>("AucForm");
    aucTimeLeft = findChild<QLCDNumber*>("AucTimeleft");
    aucBetValue = findChild<QLineEdit*>("AucBetValue");
    aucCurrency = findChild<QComboBox*>("AucCurrency");
    aucLabel = findChild<QLabel*>("AucLabel");
    disable_auc_form();

    goodsCntLabel = findChild<QLabel*>("GoodsCount");

    /* Здесь ставим конпкам функционал */
    auto getRubleBtn = findChild<QPushButton*>("GetRubleBtn");
    auto getUsdBtn = findChild<QPushButton*>("GetUsdBtn");
    auto getEurBtn = findChild<QPushButton*>("GetEurBtn");
    connect(getRubleBtn, SIGNAL(released()), this, SLOT(get_ruble())); // Например кнопка получить рубиль по нажатию вызовет функцию get_ruble()
    connect(getUsdBtn, SIGNAL(released()), this, SLOT(get_usd()));
    connect(getEurBtn, SIGNAL(released()), this, SLOT(get_eur()));

    auto rubToUsdBtn = findChild<QPushButton*>("RubToUsdBtn");
    auto rubToEurBtn = findChild<QPushButton*>("RubToEurBtn");
    connect(rubToUsdBtn, SIGNAL(released()), this, SLOT(rub_to_usd()));
    connect(rubToEurBtn, SIGNAL(released()), this, SLOT(rub_to_eur()));

    auto usdToRubBtn = findChild<QPushButton*>("UsdToRubBtn");
    auto usdToEurBtn = findChild<QPushButton*>("UsdToEurBtn");
    connect(usdToRubBtn, SIGNAL(released()), this, SLOT(usd_to_rub()));
    connect(usdToEurBtn, SIGNAL(released()), this, SLOT(usd_to_eur()));

    auto eurToRubBtn = findChild<QPushButton*>("EurToRubBtn");
    auto eurToUsdBtn = findChild<QPushButton*>("EurToUsdBtn");
    connect(eurToRubBtn, SIGNAL(released()), this, SLOT(eur_to_rub()));
    connect(eurToUsdBtn, SIGNAL(released()), this, SLOT(eur_to_usd()));
    /* */

    start_update_money_label_thr(); // Запускаем поток обновления строк денег
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool check_rubles(double ruble)
{
    if (ruble > rubles) return false;
    else return true;
}

bool check_usd(double usd)
{
    if (usd > dollars) return false;
    else return true;
}

bool check_eur(double eur)
{
    if (eur > euros) return false;
    else return true;
}

// Поток одного аукциона.
void* auction_thread(void* arg)
{
    int auction_time = *(int*)arg; // Преобразуем в int время аукциона
    while (auction_time > 0) // Пока время аукциона больше 0
    {
        aucTimeLeft->display(auction_time); // Записываем секунды в поле окна
        auction_time--; // уменьшаем
        sleep(1); // ждём секунду
    }

    char* result = new char[30];
    std::string intermediate; // Промежуточная строка для формирования результата
    if (aucBetValue->text() == "") // Если в поле ставки пользователь ничего не написал
    {
        strcpy(result, "pas"); // то он спасовал. Копируем строку "pas" в результат
        pthread_exit(result); // выходим из потока
    }
    double betValue = aucBetValue->text().toDouble(); // Если пред. условие не прошло то переводим текст из поля в число Double
    switch (aucCurrency->currentIndex()) // Смотрим какая валюта выбрана в поле валюты
    {
        case 0: // Если 0 то рубли
        {
            if (betValue > rubles) // Тут идут проверки что если написанная ставка больше доступных монет (например рублей 15 а игрок поставил 30 на ставку)
            {
                strcpy(result, "pas"); // то он пасует
                pthread_exit(result);
            }
            intermediate += "R"; break; // Если всё ок то добавляем в промежуточную строку символ R (рубли)
        }
        case 1: // 1 - доллары
        {
            if (betValue > dollars) // то же самое что в прошлом
            {
                strcpy(result, "pas");
                pthread_exit(result);
            }
            intermediate += "U"; break; // доллары (usd)
        }
        case 2: // 2 - евро
        {
            if (betValue > euros)
            {
                strcpy(result, "pas");
                pthread_exit(result);
            }
            intermediate += "E"; break; // евро (euro)
        }
    }
    intermediate += QString::number(betValue).toStdString(); // добавляем к промежуточному результату число ставки, конвертируя его в строку
    strcpy(result, intermediate.c_str()); // копируем промежуточный результат в C строку
    pthread_exit(result); // выходим из поток с полученным результатом
}

// Функция запуска обработки аукциона.
void MainWindow::start_auction(int auction_time, std::string& result, double& price) // Вызывается из Client.cpp
{
    enable_auc_form(); // Включаем в окне форму для ставки (поле ставки, выбор валюты и тд)
    pthread_t thread;
    char* res = NULL;
    pthread_create(&thread, 0, auction_thread, (void**)&auction_time); // создаём поток обработки аукциона
    pthread_join(thread, (void**)&res); // присоединяемся к нему
    disable_auc_form(); // По завершению выключаем форму для ставки
    result = std::string(res); // Переводим результат из потока в string
    if (result != "pas") // Если не спасовал то переводим полученное строку, в которой написана ставка в число
    {
        std::string number_str(result.begin() + 1, result.end()); // Здесь отбрасываем 1 букву строки обазаначающую валюту
        price = std::stod(number_str); // и число равно оставшейся строке
    }
}

// Выводит сообщение в поле окна waringLabel
void MainWindow::print_warning_message(std::string message)
{
    warningLabel->setText(QString::fromStdString(message));
}

// Функция кнопки получить рубль. Даёт 10 рублей.
void MainWindow::get_ruble()
{
    rubles += 10;
    rubLabel->setText(QString::number(rubles)); //Обновляем текст
}

// Функция кнопки получить доллары
void MainWindow::get_usd()
{
    dollars += 0.25;
    usdLabel->setText(QString::number(dollars));
}

// Функция кнопки получить евро
void MainWindow::get_eur()
{
    euros += 0.1;
    eurLabel->setText(QString::number(euros));
}

//// Здесь идут функции для кнопок конвертации валют
// Рубль в доллар
void MainWindow::rub_to_usd()
{
    double rubToConv = rubConvert->text().toDouble(); // Переводим текст из поля в число
    if (!check_rubles(rubToConv)) // Вызываем функцию проверки введённых рублей
    {
        warningLabel->setText("Недостаточно средств!"); // Если человек написал больше чем у него есть то выводим сообщение
        return;
    }

    rubles -= rubToConv; // Если всё ок то уменьшаем рубли
    dollars += RubToUsd(rubToConv); // Вызываем функцию конвертации рублей в доллары из файла Convert.cpp
    update_money_label(); // Обновляем значения валют
}

// Следующие функции по аналогии для каждой валюты

void MainWindow::rub_to_eur()
{
    double rubToConv = rubConvert->text().toDouble();
    if (!check_rubles(rubToConv))
    {
        warningLabel->setText("Недостаточно средств!");
        return;
    }

    rubles -= rubToConv;
    euros += RubToEur(rubToConv);
    update_money_label();
}

void MainWindow::usd_to_rub()
{
    double usdToConv = usdConvert->text().toDouble();
    if (!check_usd(usdToConv))
    {
        warningLabel->setText("Недостаточно средств!");
        return;
    }

    dollars -= usdToConv;
    rubles += UsdToEur(usdToConv);
    update_money_label();
}

void MainWindow::usd_to_eur()
{
    double usdToConv = usdConvert->text().toDouble();
    if (!check_usd(usdToConv))
    {
        warningLabel->setText("Недостаточно средств!");
        return;
    }

    dollars -= usdToConv;
    euros += UsdToEur(usdToConv);
    update_money_label();
}

void MainWindow::eur_to_rub()
{
    double eurToConv = eurConvert->text().toDouble();
    if (!check_eur(eurToConv))
    {
        warningLabel->setText("Недостаточно средств!");
        return;
    }

    euros -= eurToConv;
    rubles += EurToRub(eurToConv);
    update_money_label();
}

void MainWindow::eur_to_usd()
{
    double eurToConv = eurConvert->text().toDouble();
    if (!check_eur(eurToConv))
    {
        warningLabel->setText("Недостаточно средств!");
        return;
    }

    euros -= eurToConv;
    dollars += EurToRub(eurToConv);
    update_money_label();
}
////

// Обновляет в окне строки валют по текущим валютам
void MainWindow::update_money_label()
{
    rubLabel->setText(QString::number(rubles));
    usdLabel->setText(QString::number(dollars));
    eurLabel->setText(QString::number(euros));
}

// Обновляет в окне строку количества товара
void MainWindow::update_goods_label()
{
    goodsCntLabel->setText(QString::number(goods));
}

// Поток обновления валют. Работает всегда. Таким образом Client.cpp обновляет валюты в своём файле а QT их тут прописывает.
void* update_money_label_thr(void* arg)
{
    MainWindow* window = (MainWindow*)arg;
    while (true)
    {
        window->update_money_label(); // Просто вызываем функцию обновить валюты
        sleep(5); // Каждые 5 секунд
    }
}

// Начинает поток обновления валют
void MainWindow::start_update_money_label_thr()
{
    pthread_t thread;
    pthread_create(&thread, 0, update_money_label_thr, this);
}

// Выключает форму ставки
void disable_auc_form()
{
    aucLabel->setVisible(false);
    aucTimeLeft->setVisible(false);
    aucBetValue->setVisible(false);
    aucCurrency->setVisible(false);
}

// Включает форму ставки
void enable_auc_form()
{
    aucLabel->setVisible(true);
    aucTimeLeft->setVisible(true);
    aucBetValue->setVisible(true);
    aucCurrency->setVisible(true);
}
