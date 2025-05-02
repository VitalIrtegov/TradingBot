#ifndef DATASTREAMER_H
#define DATASTREAMER_H

#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <QElapsedTimer>

class DataStreamer : public QObject {
    Q_OBJECT
public:
    explicit DataStreamer(QObject *parent = nullptr);
    ~DataStreamer();

    void startStream();
    void stopStream();
    //void requestCurrentPrice(); // для запроса один раз

signals:
    void newLogMessage(const QString &message, const QString &type); // Для логов, без реализации! Так нужно для связи между классами

private slots:
    void handleResponse(const QString &message);
    void logPrice();
    //void onTextReceived(QString message); // получить ответ на запрос один раз

private:
    QWebSocket m_webSocket;
    QTimer m_timer;
    QString m_price;
};

#endif // DATASTREAMER_H
