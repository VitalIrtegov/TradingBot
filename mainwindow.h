#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "tradingengine.h"

#include <QPushButton>
#include <QTextEdit>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartButtonClicked();  // Обработчик кнопки "Старт"
    void onStopButtonClicked();  // Обработчик кнопки "Стоп"
    void logMessage(const QString &message);  // Логирование

private:
    //void setupBotToken(); // запись токена нужна один раз
    Ui::MainWindow *ui;
    void setupUI();  // Настройка интерфейса
    TradingEngine *m_engine;  // Добавляем движок
    QTextEdit *logTextEdit;
    QPushButton *startButton;
    QPushButton *stopButton;

};
#endif // MAINWINDOW_H
