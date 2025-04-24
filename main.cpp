#include "mainwindow.h"
#include <QApplication>
#include "keymanager.h"
#include <QDebug>
#include <QByteArray>
#include "qaesencryption.h"
#include "telegrambot.h"
#include <QFile>
//#include <QRandomGenerator>
//#include <QCryptographicHash>
#include <QUrlQuery>
#include <QNetworkAccessManager>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    TelegramBot bot;

    // Логирование
    QObject::connect(&bot, &TelegramBot::logMessage, [](const QString& msg) {
        qDebug() << msg;
    });

    // Обработка входящих сообщений
    QObject::connect(&bot, &TelegramBot::messageReceived, [&bot](int64_t chatId, const QString& text) {
        qDebug() << "Chat" << chatId << ":" << text;

        // Автоответ на /start
        if (text == "/start") {
            bot.sendMessage(chatId, "Бот работает без tgbot-cpp!");
        }
    });

    // Старт получения сообщений
    bot.getUpdates();

    return a.exec();
}
