#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QDateTime>

class TaskManager : public QObject {
    Q_OBJECT
public:
    explicit TaskManager(const QString& tasksDir, QObject* parent = nullptr);

public:
    bool createTask(qint64 eventTimestamp);
    QString tasksDir() const;

signals:
    void newLogMessage(const QString &message, const QString &type);

private:
    QString m_tasksDir;
    QString formatTimestamp(qint64 timestamp) const;
};

#endif // TASKMANAGER_H
