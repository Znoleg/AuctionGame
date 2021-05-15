#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "unistd.h"
#include "Convert.h"
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>

void update_money_label();
void* update_money_label_thr(void*);
void start_update_money_label_thr();

QLabel* rubLabel;
QLabel* usdLabel;
QLabel* eurLabel;
QLineEdit* rubConvert, *usdConvert, *eurConvert;

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
    connect(eurToRubBtn, SIGNAL(released()), this, SLOT(eur_to_usd()));
    connect(eurToUsdBtn, SIGNAL(released()), this, SLOT(eur_to_rub()));

    start_update_money_label_thr();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::get_ruble()
{
    rubles += 5;
    rubLabel->setText(QString::number(rubles));
}

void MainWindow::get_usd()
{
    dollars += 1;
    usdLabel->setText(QString::number(dollars));
}

void MainWindow::get_eur()
{
    euros += 0.5;
    eurLabel->setText(QString::number(euros));
}

void MainWindow::rub_to_usd()
{
    double rubToConv = rubConvert->text().toDouble();
    rubles -= rubToConv;
    dollars += RubToUsd(rubToConv);
    update_money_label();
}

void MainWindow::rub_to_eur()
{
    double rubToConv = rubConvert->text().toDouble();
    rubles -= rubToConv;
    euros += RubToEur(rubToConv);
    update_money_label();
}

void MainWindow::usd_to_rub()
{
    double usdToConv = usdConvert->text().toDouble();
    dollars -= usdToConv;
    rubles += UsdToEur(usdToConv);
    update_money_label();
}

void MainWindow::usd_to_eur()
{
    double usdToConv = usdConvert->text().toDouble();
    dollars -= usdToConv;
    euros += UsdToEur(usdToConv);
    update_money_label();
}

void MainWindow::eur_to_rub()
{
    double eurToConv = eurConvert->text().toDouble();
    euros -= eurToConv;
    rubles += EurToRub(eurToConv);
    update_money_label();
}

void MainWindow::eur_to_usd()
{
    double eurToConv = eurConvert->text().toDouble();
    euros -= eurToConv;
    dollars += EurToRub(eurToConv);
    update_money_label();
}

void update_money_label()
{
    rubLabel->setText(QString::number(rubles));
    usdLabel->setText(QString::number(dollars));
    eurLabel->setText(QString::number(euros));
}

void* update_money_label_thr(void*)
{
    while (true)
    {
        update_money_label();
        sleep(5);
    }
}

void start_update_money_label_thr()
{
    pthread_t thread;
    pthread_create(&thread, 0, update_money_label_thr, 0);
}

