#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "tradingengine.h"
#include "Logger.h"
#include "datastreamer.h"
#include "binviewer.h"

#include <QPushButton>
#include <QTextEdit>
#include <QNetworkAccessManager>

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

    // Константы для типов сообщений
    /*static constexpr auto INFO = "INFO";
    static constexpr auto WARNING = "WARNING";
    static constexpr auto ERROR = "ERROR";
    static constexpr auto WORK = "WORK";
    static constexpr auto TEST = "TEST";
    static constexpr auto SYSTEM = "SYSTEM";
    static constexpr auto OPEN = "OPEN";
    static constexpr auto RESULT = "RESULT";
    static constexpr auto PRICE = "PRICE";*/

    void sendTelegramSignal(const QString &currencyPair, const QString &direction, const QString &duration,
                            double price, double takeProfit, double stopLoss);
    void sendTelegramResult(const QString &currencyPair, const QString &resultType, double entryPrice,
                         double exitPrice, double profitPips, const QString &duration);

private slots:
    void onStartButtonClicked();  // Обработчик кнопки "Старт"
    void onStopButtonClicked();  // Обработчик кнопки "Стоп"
    void logMessage(const QString &message, const QString &type); // Логирование в окно
    void showLogsWindow(); // Логирование в файл
    void onBinViewerClicked();
    void testSignalClicked();

private:
    //void setupBotToken(); // запись токена нужна один раз
    //void setupBotChatId(); // запись ChatId нужна один раз
    Ui::MainWindow *ui;
    TradingEngine *m_engine;  // Добавляем движок
    QTextEdit *logTextEdit;
    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *testButton;

    // Инициализация клавиатуры в Telegram канале
    void initTelegramKeyboard();

    // Обработка нажатий inline-кнопок
    void processCallback(const QJsonObject &callbackData);

    // метод для запросов к Telegram API
    void sendToTelegram(const QString &method, const QJsonObject &params);

    QNetworkAccessManager *m_network;
    QString m_botToken;
    int64_t m_chatId; // Ваш chat_id (узнать через @userinfobot)
    qint64 m_lastUpdateId = 0;  // ID последнего обработанного обновления

    Logger *m_logger; // Логирование в файл
    QPushButton *logsButton;

    DataStreamer *m_dataStreamer;

    BinViewer *m_viewer;
    QPushButton *viewBinButton;
};

#endif // MAINWINDOW_H
