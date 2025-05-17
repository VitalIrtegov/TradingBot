#include "datastreamer.h"
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QCoreApplication>
#include <QSaveFile>


DataStreamer::DataStreamer(QObject* parent) : QObject(parent) {

    QString basePath = QCoreApplication::applicationDirPath();

    // Инициализация путей
    m_dataDir = basePath + "/market_data";
    m_tasksDir = basePath + "/tasks";
    m_restDataDir = basePath + "/rest_data";
    m_archiveDir = basePath + "/archive";

    // Лямбда для создания папки
    auto ensureDirExists = [](const QString &path) -> bool {
        QDir dir(path);
        if(!dir.exists()) {
            if(!dir.mkpath(".")) {
                qCritical() << "Не удалось создать папку market_data";
                return false;
            }
            qDebug() << "Создана папка:" << path;
            return true;
        }
        return true; // Папка уже существует
    };

    // Создаём все папки
    bool allOk = true;
    allOk &= ensureDirExists(m_dataDir);
    allOk &= ensureDirExists(m_tasksDir);
    allOk &= ensureDirExists(m_restDataDir);
    allOk &= ensureDirExists(m_archiveDir);

    if (!allOk) {
        emit newLogMessage("Ошибка инициализации папок", "FATAL");
    }

    // поток данных
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &DataStreamer::handleWebSocketMessage);

    //m_reconnectTimer.setInterval(120000); // таймер для переподключения каждые 2 минуты
    m_reconnectTimer.setInterval(55000); // 55 секунд, длинная проверка связи
    connect(&m_reconnectTimer, &QTimer::timeout, this, &DataStreamer::attemptReconnect);

    m_quickCheckTimer.setInterval(5000); // 5 секунд, короткая проверка связи
    connect(&m_quickCheckTimer, &QTimer::timeout, this, &DataStreamer::checkConnectionNow);

    /*connect(this, &DataStreamer::startStream, [this](){
        m_reconnectTimer.start();
        m_lastDataTimestamp = QDateTime::currentMSecsSinceEpoch();
    });*/
    // Отслеживание состояние WebSocket
    //connect(&m_webSocket, &QWebSocket::connected, this, &DataStreamer::onConnected);
    //connect(&m_webSocket, &QWebSocket::disconnected, this, &DataStreamer::onDisconnected);

    m_reconnectTimer.start();
}

DataStreamer::~DataStreamer() {
    stopStream();
}

void DataStreamer::startStream() {
    m_webSocket.open(QUrl("wss://stream.binance.com:9443/ws/btcusdt@ticker"));
    m_minuteTimer.start();
    m_waitForNewPeriod = true;
    m_lastTickTime = 0; // Нет данных более 15 секунд
    m_dataGapDetected = false; // Нет данных более 15 секунд

    //m_lastValidDataTime = QDateTime::currentMSecsSinceEpoch();
    //m_reconnectTimer.start();
    //m_webSocket.open(QUrl("wss://stream.binance.com:9443/ws/btcusdt@ticker"));
    //m_timer.start();
    qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "Попытка соединения";
    emit newLogMessage("Попытка соединения", "WORK");
}

void DataStreamer::stopStream() {    
    if(m_webSocket.state() != QAbstractSocket::UnconnectedState) {
        m_webSocket.close();
        m_minuteTimer.stop();
        m_waitForNewPeriod = true;  // Сбрасываем флаг ожидания

        //m_reconnectTimer.stop();

        qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "Соединение закрыто";
        emit newLogMessage("Соединение закрыто", "WORK");
    }
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

            qDebug() << "Отсутствовали данные 15 сек!";
            emit newLogMessage("Отсутствовали данные 15 сек!", "WORK");

            // Сбрасываем все буферы
            m_minuteBuffer.clear(); // очищаем минутный буфер
            m_fiveMinuteBuffer.clear(); // очищаем пятиминутный буфер
            //m_waitForNewPeriod = true; // сбрасываем флаг ожидания периода
            //m_lastProcessedMinute = -1; // сбрасываем счетчик минут

            //m_reconnectTimer.start(); // старт таймера переподключения
        }
    } else {
        m_dataGapDetected = false;
    }

    m_lastTickTime = currentTimestamp;
}

void DataStreamer::handleWebSocketMessage(const QString &message) {
    m_lastValidDataTime = QDateTime::currentMSecsSinceEpoch(); // валидное время для контроля свзи

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
            qDebug() << "Начало нового интервала 5 минут:"
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
            //qDebug() << "Начало минуты:" << tickTime.toString("hh:mm:ss");
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

    // Берем время первого тика и выравниваем по 5 минутам
    qint64 firstTickTime = m_fiveMinuteBuffer.first().first().timestamp;
    const qint64 fiveMinutesMs = 5 * 60 * 1000;
    qint64 alignedTime = (firstTickTime / fiveMinutesMs) * fiveMinutesMs;

    // формирование имени для сохранения
    QString fileName = QString("%1/%2.bin").arg(m_dataDir).arg(alignedTime);
    // Используем текущее время для имени файла вместо времени первого тика
    //qint64 fileTimestamp = (QDateTime::currentMSecsSinceEpoch() / 300000) * 300000;
    //QString fileName = QString("%1/%2.bin").arg(m_dataDir).arg(fileTimestamp);

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

// Попытка переподключения
void DataStreamer::attemptReconnect() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    //if(m_webSocket.state() == QAbstractSocket::ConnectedState) {
        if(now - m_lastValidDataTime > 15000) {
            //qDebug() << "отключено";
            startStream();
            m_quickCheckTimer.start();
        }/* else {
            qDebug() << "подключено";
        }*/
    //}
       emit needCheckMissingData(m_dataDir); // вызываем проверку задач
}

// короткая проверка статуса
void DataStreamer::checkConnectionNow() {
    bool isConnected = (m_webSocket.state() == QAbstractSocket::ConnectedState);
    qDebug() << "статус:" << (isConnected ? "подключено" : "отключено");
    if(isConnected) {
        emit newLogMessage("подключено", "WORK");
    } else {
        emit newLogMessage("отключено", "WORK");
        // Сбрасываем все буферы
        // Сбрасываем все буферы
        m_minuteBuffer.clear(); // очищаем минутный буфер
        m_fiveMinuteBuffer.clear(); // очищаем пятиминутный буфер
    }

    m_quickCheckTimer.stop();
}

void DataStreamer::testStream() {
    qDebug() << "Тест: Имитация пропуска данных 20 секунд";
    checkDataGap(QDateTime::currentMSecsSinceEpoch() - 20000);
}

/*void DataStreamer::createGapTask(qint64 eventTimestamp) {
    // 1. Вычисляем 5-минутный интервал
    const qint64 interval = 5 * 60 * 1000; // 5 минут в мс
    qint64 periodStart = (eventTimestamp / interval) * interval;
    qint64 periodEnd = periodStart + interval;

    // 2. Формируем задачу
    QJsonObject task {
                     {"symbol", "BTCUSDT"},
                     {"created_at", QDateTime::currentMSecsSinceEpoch()},
                     {"start", periodStart},
                     {"end", periodEnd},
                     {"status", "pending"},
                     {"timeframe", "1m"},
                     {"candles_required", "5"}
    };

    // 3. Генерируем имя файла
    QString fileName = QString("%1_%2.json").arg(periodStart).arg(periodEnd);
    QString filePath = m_tasksDir + "/" + fileName;

    // 4. Атомарное сохранение
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit newLogMessage(
            QString("Ошибка создания файла для периода %1 - %2")
                .arg(formatTimestamp(periodStart))
                .arg(formatTimestamp(periodEnd)),
            "ERROR"
            );
        return;
    }

    file.write(QJsonDocument(task).toJson());
    if (!file.commit()) {
        emit newLogMessage("Ошибка сохранения задачи", "ERROR");
        return;
    }

    // 5. Логирование
    emit newLogMessage(
        QString("Создана задача: %1 (%2 - %3)")
            .arg(fileName)
            .arg(formatTimestamp(periodStart))
            .arg(formatTimestamp(periodEnd)),
        "WORK"
        );
}*/

// Тестовый метод
/*void DataStreamer::testCreateGapTask(qint64 serverTime) {
    // 1. Вызываем основной метод
    createGapTask(serverTime);

    // 2. Проверяем результат
    qint64 interval = 5 * 60 * 1000;
    qint64 expectedStart = (serverTime / interval) * interval;
    qint64 expectedEnd = expectedStart + interval;

    QString expectedFileName = QString("%1_%2.json")
                                   .arg(expectedStart)
                                   .arg(expectedEnd);

    QString filePath = m_tasksDir + "/" + expectedFileName;

    if (QFile::exists(filePath)) {
        emit newLogMessage(
            QString("Тест пройден: %1 (%2 - %3)")
                .arg(expectedFileName)
                .arg(formatTimestamp(expectedStart))
                .arg(formatTimestamp(expectedEnd)),
            "TEST_SUCCESS"
            );
    } else {
        emit newLogMessage(
            QString("Тест не пройден: файл для периода %1 - %2 не создан")
                .arg(formatTimestamp(expectedStart))
                .arg(formatTimestamp(expectedEnd)),
            "TEST_FAIL"
            );
    }
}*/

// вывод в лог времени в понятном формате
/*QString DataStreamer::formatTimestamp(qint64 timestamp) const {
    // Убедимся, что timestamp в миллисекундах
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp);
    return dt.toString("dd.MM.yyyy hh:mm:ss");
}*/
