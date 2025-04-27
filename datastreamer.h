#ifndef DATASTREAMER_H
#define DATASTREAMER_H

#include <QObject>
#include <QWebSocket>
#include <QTimer>

class DataStreamer : public QObject {
    Q_OBJECT
public:
    explicit DataStreamer(QObject *parent = nullptr);
    ~DataStreamer();

    void startStream();
    void stopStream();

signals:
    void newLogMessage(const QString &message, const QString &type);

private:
    QWebSocket m_webSocket;
};

#endif // DATASTREAMER_H
