#include "datastreamer.h"
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QCoreApplication>

DataStreamer::DataStreamer(QObject *parent) : QObject(parent) {
    // Инициализация папки данных
    m_dataDir = QCoreApplication::applicationDirPath() + "/market_data";
    QDir dir(m_dataDir);
    if(!dir.exists()) {
        if(!dir.mkpath(".")) {
            qCritical() << "Не удалось создать папку market_data";
        }
    }

    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &DataStreamer::handleWebSocketMessage);

    // Настройка таймера
    //m_timer.setInterval(1000);
    // логи по истечению интервала
    //connect(&m_timer, &QTimer::timeout, this, &DataStreamer::logPrice);
}

void DataStreamer::startStream() {
    m_webSocket.open(QUrl("wss://stream.binance.com:9443/ws/btcusdt@ticker"));
    m_minuteTimer.start();
    m_waitForNewPeriod = true;
    m_lastTickTime = 0; // Нет данных более 15 секунд
    m_dataGapDetected = false; // Нет данных более 15 секунд

    //m_webSocket.open(QUrl("wss://stream.binance.com:9443/ws/btcusdt@ticker"));
    //m_timer.start();
    qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss ") << "Старт потока";
    emit newLogMessage("Старт потока", "WORK");
}

void DataStreamer::stopStream() {    
    if(m_webSocket.state() != QAbstractSocket::UnconnectedState) {
        m_webSocket.close();
        m_minuteTimer.stop();
        m_waitForNewPeriod = true;  // Сбрасываем флаг ожидания
        //m_timer.stop();
        qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "Соединение закрыто";
        emit newLogMessage("Соединение закрыто", "WORK");
    }
}

void DataStreamer::testStream() {
    qDebug() << "Тест: Имитация пропуска данных 20 секунд";
    checkDataGap(QDateTime::currentMSecsSinceEpoch() - 20000);
}

void DataStreamer::checkDataGap(qint64 currentTimestamp) {
    if(m_lastTickTime == 0) {
        m_lastTickTime = currentTimestamp;
        return;
    }

    const qint64 gap = currentTimestamp - m_lastTickTime;

    if(gap > MAX_ALLOWED_GAP) {
        if(!m_dataGapDetected) {
            qCritical() << "Пропуск данных:"
                        << gap/1000 << "секунд";
            m_dataGapDetected = true;

            stopStream();

            // Экстренное сохранение
            /*if(!m_fiveMinuteBuffer.isEmpty()) {
                saveFiveMinuteData();
                m_fiveMinuteBuffer.clear();
            }
            emit dataGapDetected(gap);*/ // Сигнал для внешних обработчиков
        }
    } else {
        m_dataGapDetected = false;
    }/*else if(m_dataGapDetected) {
        qInfo() << "[GAP RESOLVED] Данные восстановлены";
        m_dataGapDetected = false;
        emit dataRestored();
    }*/

    m_lastTickTime = currentTimestamp;
}

void DataStreamer::handleWebSocketMessage(const QString &message) {
    const qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    // Проверка пропуска данных более 15 сек
    checkDataGap(currentTime);

    // парсинг входящего сообщения
    QJsonObject json = QJsonDocument::fromJson(message.toUtf8()).object();
    if(!json.contains("E") || !json.contains("c") || !json.contains("v")) return;

    // извлечение данных тика
    const qint64 timestamp = json["E"].toVariant().toLongLong();
    const QDateTime tickTime = QDateTime::fromMSecsSinceEpoch(timestamp);
    const int currentMinute = tickTime.time().minute();
    const int currentSecond = tickTime.time().second();

    // проверка на начало новой пятиминутки
    if(m_waitForNewPeriod) {
        if(currentMinute % 5 == 0) { // Кратно 5 минутам (00, 05, 10...)
            m_waitForNewPeriod = false;
            m_currentPeriodStart = timestamp;
            emit newLogMessage("Начало нового интервала 5 минут", "WORK");
            qDebug() << "Начало нового 5-минутного периода:"
                     << tickTime.toString("hh:mm:ss");
        } else {
            return; // Пропускаем данные до начала периода
        }
    }

    // проверка перехода на следующую минуту
    if(!m_waitForNewPeriod) {
        // Если это первые секунды новой минуты (00-03 сек)
        if(currentSecond <= 3 && currentMinute != m_lastProcessedMinute) {
            // Переносим данные в 5-минутный буфер
            if(!m_minuteBuffer.isEmpty()) {
                m_fiveMinuteBuffer.append(m_minuteBuffer);
                m_minuteBuffer.clear();
            }
            m_lastProcessedMinute = currentMinute;
            qDebug() << "Начало минуты:" << tickTime.toString("hh:mm:ss");
        }

        // проверка завершения 5-минутного периода
        if((timestamp - m_currentPeriodStart) >= 300000) { // Ровно 5 минут
            if(!m_fiveMinuteBuffer.isEmpty()) {
                saveFiveMinuteData();
                m_fiveMinuteBuffer.clear();
                qDebug() << "Прошло 5 минут:" << tickTime.toString("hh:mm:ss") << "сохранение.";
            }
            m_waitForNewPeriod = true; // сброс флага для нового периода
            return;
        }
    }

    // сохранение тика в буфер
    m_minuteBuffer.append({
        .timestamp = timestamp,
        .price = json["c"].toString().toDouble(),
        .volume = json["v"].toString().toDouble()
    });
}

    //QDateTime lastDt = QDateTime::fromMSecsSinceEpoch(m_minuteBuffer.last().timestamp);
    //qDebug() << "Текущая секунда:" << lastDt.toString("hh:mm:ss");
    //qDebug() << "Текущее время буфера:" << lastDt.toString("hh:mm:ss");
    //qDebug() << "Размер буфера:" << m_minuteBuffer.size();
    //qDebug() << "Последняя валидная цена:" << m_lastPrice;

void DataStreamer::saveFiveMinuteData() {
    if(m_fiveMinuteBuffer.isEmpty()) {
        emit newLogMessage("Нет данных для сохранения", "WARNING");
        return;
    }

    QString fileName = QString("%1/%2.bin")
                           .arg(m_dataDir)
                           .arg(m_fiveMinuteBuffer.first().first().timestamp);

    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);
        for(const auto& minute : m_fiveMinuteBuffer) {
            for(const auto& tick : minute) {
                out << tick.timestamp << tick.price << tick.volume;
            }
        }
        file.close();
        emit newLogMessage(QString("Сохранен блок: %1 записей")
                               .arg(m_fiveMinuteBuffer.size() * 60), "SUCCESS");
    } else {
        emit newLogMessage(QString("Ошибка сохранения: %1").arg(file.errorString()), "ERROR");
    }
}

DataStreamer::~DataStreamer() {
    stopStream();
}
