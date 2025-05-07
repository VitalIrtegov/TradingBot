#include "taskmanager.h"
#include <QSaveFile>
#include <QDir>

TaskManager::TaskManager(const QString& tasksDir, QObject* parent)
    : QObject(parent), m_tasksDir(tasksDir) {
    QDir().mkpath(m_tasksDir);
}

bool TaskManager::createTask(qint64 eventTimestamp) {
    // вычисляем 5-минутный интервал
    const qint64 interval = 5 * 60 * 1000;
    qint64 periodStart = (eventTimestamp / interval) * interval;
    qint64 periodEnd = periodStart + interval;

    // формируем задачу
    QJsonObject task {
        {"symbol", "BTCUSDT"},
        {"created_at", QDateTime::currentMSecsSinceEpoch()},
        {"start", periodStart},
        {"end", periodEnd},
        {"status", "pending"},
        {"timeframe", "1m"},
        {"candles_required", "5"}
    };

    // имя файла
    QString fileName = QString("%1_%2.json").arg(periodStart).arg(periodEnd);
    QString filePath = m_tasksDir + "/" + fileName;



    // атомарная запись
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit newLogMessage(
            QString("Ошибка создания файла для периода %1 - %2")
                .arg(formatTimestamp(periodStart))
                .arg(formatTimestamp(periodEnd)),
            "ERROR"
            );
        return false;
    }

    file.write(QJsonDocument(task).toJson());
    if (!file.commit()) {
        emit newLogMessage("Ошибка сохранения задачи", "ERROR");
        return;
    }

    // логирование
    emit newLogMessage(
        QString("Создана задача: %1 (%2 - %3)")
            .arg(fileName)
            .arg(formatTimestamp(periodStart))
            .arg(formatTimestamp(periodEnd)),
        "WORK"
        );

    return file.commit();
}

// вывод в лог времени в понятном формате
QString DataStreamer::formatTimestamp(qint64 timestamp) const {
    // Убедимся, что timestamp в миллисекундах
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp);
    return dt.toString("dd.MM.yyyy hh:mm:ss");
}
