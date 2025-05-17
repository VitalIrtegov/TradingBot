#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include "datastreamer.h"

#include <QObject>
#include <QDateTime>

class TaskManager : public QObject {
    Q_OBJECT
public:
    //xplicit TaskManager(const QString& tasksDir, QObject* parent = nullptr);
    explicit TaskManager(DataStreamer* dataStreamer, const QString& tasksDir, QObject* parent = nullptr);

    bool createTask(qint64 eventTimestamp);
    QString tasksDir() const;

private slots:
    void checkAndCreateTasks(const QString& dataDir); // проверки для формирования новой задачи

signals:
    void newLogMessage(const QString &message, const QString &type);

private:
    DataStreamer *m_dataStreamer;

    QString m_tasksDir;
    QString formatTimestamp(qint64 timestamp) const;
};

#endif // TASKMANAGER_H
