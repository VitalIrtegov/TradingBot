#ifndef TELEGRAMBOT_H
#define TELEGRAMBOT_H

#include "keymanager.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

class TelegramBot : public QObject {
    Q_OBJECT

public:
    explicit TelegramBot(QObject *parent = nullptr);
    void sendMessage(int64_t chatId, const QString& text); // Отправка сообщения
    void getUpdates();  // Получение новых сообщений

signals:
    void messageReceived(int64_t chatId, const QString& text);  // Сигнал о новом сообщении
    void logMessage(const QString& text);  // Для логов

private slots:
    //void checkUpdates();
    void onReplyReceived(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_network;
    //QTimer m_updateTimer;
    QString m_token;
    int m_lastUpdateId = 0;  // Для отслеживания новых сообщений
};

#endif // TELEGRAMBOT_H
