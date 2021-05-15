#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

private slots:
    void get_ruble();
    void get_usd();
    void get_eur();
    void rub_to_usd();
    void rub_to_eur();
    void usd_to_rub();
    void usd_to_eur();
    void eur_to_rub();
    void eur_to_usd();
};
#endif // MAINWINDOW_H
