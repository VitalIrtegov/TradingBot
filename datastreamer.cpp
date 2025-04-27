#include "datastreamer.h"
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>

DataStreamer::DataStreamer(QObject *parent) : QObject(parent) {
    connect(&m_webSocket, &QWebSocket::connected, [this]() {
        emit newLogMessage("WebSocket подключен", "SYSTEM");
    });

    connect(&m_webSocket, &QWebSocket::disconnected, [this]() {
        emit newLogMessage("WebSocket отключен", "SYSTEM");
    });

    connect(&m_webSocket, &QWebSocket::textMessageReceived, [this](const QString &msg) {
        QJsonObject data = QJsonDocument::fromJson(msg.toUtf8()).object();
        if(data.contains("c")) {
            emit newLogMessage(QString("Цена: %1").arg(data["c"].toString()), "PRICE");
        }
    });
}

void DataStreamer::startStream() {
    //m_webSocket.open(QUrl("wss://stream.binance.com:9443/ws/eurusdt@ticker"));
    qDebug() << "Старт потока";
}

void DataStreamer::stopStream() {
    //m_webSocket.close();
    qDebug() << "Стоп потока";
}

DataStreamer::~DataStreamer() { }
