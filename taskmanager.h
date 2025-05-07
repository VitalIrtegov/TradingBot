#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QDateTime>

class TaskManager : public QObject {
    Q_OBJECT
public:
    explicit TaskManager(QObject *parent = nullptr);

public:
    // метод создания задач
    bool createTask(qint64 eventTimestamp);
private:
    QString m_tasksDir;
    QString formatTimestamp(qint64 timestamp) const;
};

#endif // TASKMANAGER_H
