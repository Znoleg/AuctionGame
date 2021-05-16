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

    auto getRubleBtn = findChild<QPushButton*>("GetRubleBtn");
    auto getUsdBtn = findChild<QPushButton*>("GetUsdBtn");
    auto getEurBtn = findChild<QPushButton*>("GetEurBtn");
    connect(getRubleBtn, SIGNAL(released()), this, SLOT(get_ruble()));
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

    start_update_money_label_thr();
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

void* auction_thread(void* arg)
{
    int auction_time = *(int*)arg;
    while (auction_time > 0)
    {
        aucTimeLeft->display(auction_time);
        auction_time--;
        sleep(1);
    }

    char* result = new char[30];
    std::string intermediate;
    if (aucBetValue->text() == "")
    {
        strcpy(result, "pas");
        pthread_exit(result);
    }
    double betValue = aucBetValue->text().toDouble();
    switch (aucCurrency->currentIndex())
    {
        case 0:
        {
            if (betValue > rubles)
            {
                strcpy(result, "pas");
                pthread_exit(result);
            }
            intermediate += "R"; break;
        }
        case 1:
        {
            if (betValue > dollars)
            {
                strcpy(result, "pas");
                pthread_exit(result);
            }
            intermediate += "U"; break;
        }
        case 2:
        {
            if (betValue > euros)
            {
                strcpy(result, "pas");
                pthread_exit(result);
            }
            intermediate += "E"; break;
        }
    }
    intermediate += QString::number(betValue).toStdString();
    strcpy(result, intermediate.c_str());
    pthread_exit(result);
}

void MainWindow::start_auction(int auction_time, std::string& result, double& price)
{
    enable_auc_form();
    pthread_t thread;
    char* res = NULL;
    pthread_create(&thread, 0, auction_thread, (void**)&auction_time);
    pthread_join(thread, (void**)&res);
    disable_auc_form();
    result = std::string(res);
    if (result != "pas")
    {
        std::string number_str(result.begin() + 1, result.end());
        price = std::stod(number_str);
    }
}

void MainWindow::print_warning_message(std::string message)
{
    warningLabel->setText(QString::fromStdString(message));
}

void MainWindow::get_ruble()
{
    rubles += 10;
    rubLabel->setText(QString::number(rubles));
}

void MainWindow::get_usd()
{
    dollars += 0.25;
    usdLabel->setText(QString::number(dollars));
}

void MainWindow::get_eur()
{
    euros += 0.1;
    eurLabel->setText(QString::number(euros));
}

void MainWindow::rub_to_usd()
{
    double rubToConv = rubConvert->text().toDouble();
    if (!check_rubles(rubToConv))
    {
        warningLabel->setText("Недостаточно средств!");
        return;
    }

    rubles -= rubToConv;
    dollars += RubToUsd(rubToConv);
    update_money_label();
}

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

void MainWindow::update_money_label()
{
    rubLabel->setText(QString::number(rubles));
    usdLabel->setText(QString::number(dollars));
    eurLabel->setText(QString::number(euros));
}

void MainWindow::update_goods_label()
{
    goodsCntLabel->setText(QString::number(goods));
}

void* update_money_label_thr(void* arg)
{
    MainWindow* window = (MainWindow*)arg;
    while (true)
    {
        window->update_money_label();
        sleep(5);
    }
}

void MainWindow::start_update_money_label_thr()
{
    pthread_t thread;
    pthread_create(&thread, 0, update_money_label_thr, this);
}

void disable_auc_form()
{
    aucLabel->setVisible(false);
    aucTimeLeft->setVisible(false);
    aucBetValue->setVisible(false);
    aucCurrency->setVisible(false);
}

void enable_auc_form()
{
    aucLabel->setVisible(true);
    aucTimeLeft->setVisible(true);
    aucBetValue->setVisible(true);
    aucCurrency->setVisible(true);
}
