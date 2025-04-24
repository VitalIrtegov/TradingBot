#include "telegrambot.h"
#include "mainwindow.h"
#include <QUrlQuery>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QJsonArray>
//#include <QApplication>  // Для qApp

//TelegramBot::TelegramBot(QObject *parent) : QObject{parent} { }

TelegramBot::TelegramBot(QObject *parent)
    : QObject(parent),
    m_network(new QNetworkAccessManager(this)),
    m_token(KeyManager::instance().loadBotToken())
{
    connect(m_network, &QNetworkAccessManager::finished,
            this, &TelegramBot::onReplyReceived);
}

/*TelegramBot::~TelegramBot() {
    delete m_network;  // Освобождаем память
}*/

/*void TelegramBot::checkUpdates() {
    QUrl url("https://api.telegram.org/bot" + m_token + "/getUpdates");
    QUrlQuery params;
    params.addQueryItem("offset", QString::number(m_lastUpdateId + 1));
    params.addQueryItem("timeout", "1");  // Короткий таймаут
    url.setQuery(params);

    m_network->get(QNetworkRequest(url));
}*/

void TelegramBot::sendMessage(int64_t chatId, const QString& text) {
    QUrl url("https://api.telegram.org/bot" + m_token + "/sendMessage");
    QUrlQuery params;
    params.addQueryItem("chat_id", QString::number(chatId));
    params.addQueryItem("text", text);
    url.setQuery(params);

    m_network->get(QNetworkRequest(url));

    //emit logMessage("Отправлено: " + text); // вместо этого работает код ниже
    MainWindow::logMessage("Отправлено: " + text);
}

// Получение обновлений (раз в 1-5 секунд)
void TelegramBot::getUpdates() {
    QUrl url("https://api.telegram.org/bot" + m_token + "/getUpdates");
    QUrlQuery query;
    query.addQueryItem("offset", QString::number(m_lastUpdateId + 1));
    query.addQueryItem("timeout", "5");  // Долгий опрос
    url.setQuery(query);

    m_network->get(QNetworkRequest(url));
}

// Обработка ответов от Telegram API
void TelegramBot::onReplyReceived(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        //emit logMessage("Ошибка: " + reply->errorString()); // вместо этого работает код ниже
        MainWindow::logMessage("Ошибка: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject json = doc.object();

    // Обработка входящих сообщений
    if (json.contains("result")) {
        for (const QJsonValue& update : json["result"].toArray()) {
            QJsonObject obj = update.toObject();
            m_lastUpdateId = obj["update_id"].toInt();

            if (obj.contains("message")) {
                QJsonObject msg = obj["message"].toObject();
                int64_t chatId = msg["chat"].toObject()["id"].toVariant().toLongLong();
                QString text = msg["text"].toString();

                emit messageReceived(chatId, text);
                //emit logMessage("Получено: " + text); // вместо этого работает код ниже
                MainWindow::logMessage("Получено: " + text);
            }
        }
    }

    // Запрашиваем новые обновления
    getUpdates();
}

/*void TelegramBot::onReplyReceived(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Ошибка:" << reply->errorString();
    } else {
        qDebug() << "Ответ API:" << reply->readAll();
    }
    reply->deleteLater();
}*/
