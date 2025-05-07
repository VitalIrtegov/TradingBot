#ifndef DATASTREAMER_H
#define DATASTREAMER_H

#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <QVector>

struct TickData {
    qint64 timestamp;
    double price;
    double volume;
};

class DataStreamer : public QObject {
    Q_OBJECT
public:
    explicit DataStreamer(QObject *parent = nullptr);
    ~DataStreamer();

    void startStream();
    void stopStream();
    void testStream();

signals:
    void newLogMessage(const QString &message, const QString &type); // Для логов, без реализации! Так нужно для связи между классами

private slots:
    void handleWebSocketMessage(const QString& message);

private:
    void saveFiveMinuteData();
    void checkDataGap(qint64 currentTimestamp); // Нет данных более 15 секунд

    QWebSocket m_webSocket;
    QTimer m_minuteTimer;
    QVector<TickData> m_minuteBuffer;
    QVector<QVector<TickData>> m_fiveMinuteBuffer;
    double m_lastPrice = 0;

    QString m_dataDir = "market_data"; // Папка для сохранения бин
    QString m_tasksDir = "tasks"; // Папка для задач
    QString m_restDataDir = "rest_data"; // Папка для скачанные 5-минутки
    QString m_archiveDir = "archive"; // Папка для выполненные задачи

    bool m_waitForNewPeriod = true;
    qint64 m_currentPeriodStart = 0;
    int m_lastProcessedMinute = -1;

    qint64 m_lastTickTime = 0; // Нет данных более 15 секунд
    bool m_dataGapDetected = false; // Нет данных более 15 секунд
    const qint64 MAX_ALLOWED_GAP = 15000; // Нет данных более 15 секунд

    void createGapTask(qint64 eventTimestamp);
    void testCreateGapTask(qint64 serverTime);
    QString formatTimestamp(qint64 timestamp) const;
};

#endif // DATASTREAMER_H
