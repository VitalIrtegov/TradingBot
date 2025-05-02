#include "datastreamer.h"
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>

DataStreamer::DataStreamer(QObject *parent) : QObject(parent) {

    // получение данных через WebSocket
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &DataStreamer::handleResponse);

    // Настройка таймера
    m_timer.setInterval(1000);
    // логи по истечению интервала
    connect(&m_timer, &QTimer::timeout, this, &DataStreamer::logPrice);

    // для единичного запроса по нажатию кнопки
    //connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &DataStreamer::onTextReceived);
}

void DataStreamer::startStream() {
    /*if(m_webSocket.state() != QAbstractSocket::UnconnectedState) {
        qDebug() << "Соединение уже активно";
        return;
    }

    // Сбрасываем предыдущие подключения
    disconnect(&m_webSocket, &QWebSocket::connected, nullptr, nullptr);

    connect(&m_webSocket, &QWebSocket::connected, this, [this]() {
        emit newLogMessage("Старт потока", "WORK");
        qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss ") << "Старт потока";
        requestCurrentPrice(); // автозапрос после подключения по webSocket
    });

    m_webSocket.open(QUrl("wss://stream.binance.com:9443/ws"));*/

    m_webSocket.open(QUrl("wss://stream.binance.com:9443/ws/btcusdt@ticker"));
    m_timer.start();
    qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss ") << "Старт потока";
    emit newLogMessage("Старт потока", "WORK");
}

void DataStreamer::stopStream() {    
    if(m_webSocket.state() != QAbstractSocket::UnconnectedState) {
        m_webSocket.close();
        m_timer.stop();
        qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "Соединение закрыто";
        emit newLogMessage("Соединение закрыто", "WORK");
    }
}

void DataStreamer::handleResponse(const QString &message) {
    QJsonObject data = QJsonDocument::fromJson(message.toUtf8()).object();
    if(data.contains("c")) {
        m_price = data["c"].toString();
    }
}

void DataStreamer::logPrice() {
    //qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss ") << "EUR/USD: " << m_price;
    emit newLogMessage(QString("BTC/USD: %1").arg(m_price), "PRICE");
}

/*void DataStreamer::requestCurrentPrice() {
    if(m_webSocket.state() == QAbstractSocket::ConnectedState) {
        // запрос для Binance WebSocket API
        QString request = R"({
            "method": "SUBSCRIBE",
            "params": ["eurusdt@ticker"],
            "id": )" + QString::number(QDateTime::currentSecsSinceEpoch()) + "}";

        m_webSocket.sendTextMessage(request);
        emit newLogMessage("Подписка отправлена", "PRICE");
    } else {
        emit newLogMessage("Ошибка: нет соединения", "ERROR");
    }
}

void DataStreamer::onTextReceived(QString message) {
    // Парсинг JSON
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject data = doc.object();

    // Игнорируем служебные сообщения
    if(data.contains("result") && data["result"].isNull()) { return; }

    // 1. Проверяем, что это данные по цене EUR/USDT
    if(data["e"].toString() == "24hrTicker" && data["s"].toString() == "EURUSDT") {
        QString price = data["c"].toString(); // Последняя цена
        qDebug() << "Получена цена:" << price;
        emit newLogMessage("EUR/USDT: " + price, "PRICE");

        // 2. Отписываемся сразу после получения цены
        QString unsubscribeMsg = R"({
            "method": "UNSUBSCRIBE",
            "params": ["eurusdt@ticker"],
            "id": )" + QString::number(QDateTime::currentSecsSinceEpoch()) + "}";

        m_webSocket.sendTextMessage(unsubscribeMsg);
        emit newLogMessage("Отписка отправлена", "PRICE");
    } else {
        qDebug() << "Другое сообщение:" << message;
    }
}*/

DataStreamer::~DataStreamer() {
    stopStream();
}
