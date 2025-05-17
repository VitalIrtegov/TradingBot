#include "taskmanager.h"
#include "datastreamer.h"
#include <QSaveFile>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QFileInfo>
#include <QFileInfoList>

TaskManager::TaskManager(DataStreamer* dataStreamer, const QString& tasksDir, QObject* parent)
    : QObject(parent),
    m_dataStreamer(dataStreamer),  // Используем переданный экземпляр
    m_tasksDir(tasksDir)
{
    QDir().mkpath(m_tasksDir);

    // Получаем сигналы от DataStreamer для проверки
    connect(m_dataStreamer, &DataStreamer::needCheckMissingData, this, &TaskManager::checkAndCreateTasks);
}

QString TaskManager::tasksDir() const {
    return m_tasksDir;
}

//  формирование новой задачи
bool TaskManager::createTask(qint64 eventTimestamp) {
    const qint64 interval = 5 * 60 * 1000; // 5-минутный интервал

    // Выравниваем время без секунд и миллисекунд
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
    QString fileName = QString("%1.json").arg(periodStart);
    QString filePath = m_tasksDir + "/" + fileName;

    // атомарная запись
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Ошибка открытия файла для записи:" << file.errorString();
        /*emit newLogMessage(QString("Ошибка создания файла для периода %1 - %2")
                .arg(formatTimestamp(periodStart))
                .arg(formatTimestamp(periodEnd)), "ERROR");*/
        return false;
    }

    qint64 bytesWritten = file.write(QJsonDocument(task).toJson());
    if (bytesWritten == -1) {
        qDebug() << "Ошибка записи в файл:" << file.errorString();
        //emit newLogMessage("Ошибка записи задачи", "ERROR");
        return false;
    }

    if (!file.commit()) {
        qDebug() << "Ошибка сохранения файла:" << file.errorString();
        //emit newLogMessage("Ошибка фиксации задачи", "ERROR");
        return false;
    }

    // логирование успешного создания
    //qDebug() << "Файл успешно создан:" << filePath;
    /*emit newLogMessage(QString("Создана задача: %1 (%2 - %3)")
            .arg(fileName)
            .arg(formatTimestamp(periodStart))
            .arg(formatTimestamp(periodEnd)), "WORK");*/

    return true;
}

// проверки для формирования новой задачи
void TaskManager::checkAndCreateTasks(const QString& dataDir) {
    //qDebug() << "проверка внутри TaskManager";

    const qint64 fiveMinutesMs = 5 * 60 * 1000;
    const qint64 timeZoneOffset = 3 * 60 * 60 * 1000;
    const qint64 currentTime = QDateTime::currentMSecsSinceEpoch() + timeZoneOffset;

    qDebug() << "Текущее время (UTC+3):" << QDateTime::fromMSecsSinceEpoch(currentTime).toString("hh:mm:ss");

    // Проверяем только папку market_data
    QDir dataDirectory(dataDir);
    QFileInfoList dataFiles = dataDirectory.entryInfoList(QStringList() << "*.bin", QDir::Files, QDir::Name);

    qDebug() << "Содержимое папки market_data:";
    foreach (const QFileInfo &file, dataFiles) {
        qint64 fileTime = file.baseName().toLongLong();
        qDebug() << "-" << file.fileName()
                 << "| Время:" << QDateTime::fromMSecsSinceEpoch(fileTime).toString("yyyy-MM-dd hh:mm:ss");
                 //<< "| Выровненное:" << QDateTime::fromMSecsSinceEpoch((fileTime / fiveMinutesMs) * fiveMinutesMs).toString("hh:mm");
    }

    if (dataFiles.isEmpty()) {
        qDebug() << "Нет файлов - пропускаем";
        return;
    }

    // Берем последний файл из market_data
    QFileInfo lastFile = dataFiles.last();
    qint64 lastFileTime = lastFile.baseName().toLongLong();
    qint64 alignedLastTime = (lastFileTime / fiveMinutesMs) * fiveMinutesMs; // выравниваем время по 5 минут
    qint64 nextExpectedTime = alignedLastTime + fiveMinutesMs;

    qDebug() << "Последний файл данных:" << lastFile.fileName()
             << "| Время:" << QDateTime::fromMSecsSinceEpoch(lastFileTime).toString("hh:mm:ss");
             //<< "| Выровненное время:" << QDateTime::fromMSecsSinceEpoch(alignedLastTime).toString("hh:mm:ss");

    qint64 alignedCurrentTime = (currentTime / fiveMinutesMs) * fiveMinutesMs; // выравниваем реальное время по 5 минут

    // Проверяем пропуск (>5 минут от ожидаемого времени)
    if (alignedCurrentTime >= nextExpectedTime + fiveMinutesMs) {
        //qDebug() << "Обнаружен пропуск! Требуется создать задачу для:"
                 //<< QDateTime::fromMSecsSinceEpoch(nextExpectedTime).toString("hh:mm:ss");

        // Формируем имя файла задачи и путь
        //QString expectedFileName = QString("%1.json").arg(nextExpectedTime);
        //QString taskFilePath = m_tasksDir + "/" + expectedFileName;
        //qDebug() << "Ожидаемое имя файла задачи:" << expectedFileName;
        //qDebug() << "Полный путь к файлу: " << taskFilePath;

        // Проверяем папку tasks
        QDir tasksDir(m_tasksDir);
        QFileInfoList taskFiles = tasksDir.entryInfoList(QStringList() << "*.json", QDir::Files);

        qDebug() << "Содержимое папки tasks:";
        foreach (const QFileInfo &file, taskFiles) {
            qint64 fileTime = file.baseName().remove(".json").toLongLong();
            qDebug() << "-" << file.fileName()
                     << "| Время:" << QDateTime::fromMSecsSinceEpoch(fileTime).toString("yyyy-MM-dd hh:mm:ss");
                     //<< "| Выровненное:" << QDateTime::fromMSecsSinceEpoch((fileTime / fiveMinutesMs) * fiveMinutesMs).toString("hh:mm")
                     //<< "| Modified:" << file.lastModified().toString("yyyy-MM-dd hh:mm:ss");
        }

        if (!taskFiles.isEmpty()) {
            // Берем последний файл из tasks
            QFileInfo lastTaskFile = taskFiles.last();
            qint64 lastTaskTime = lastTaskFile.baseName().remove(".json").toLongLong();

            // Вычисляем разницу между текущим выровненным временем и временем последней задачи
            qint64 timeDiff = alignedCurrentTime - lastTaskTime;

            qDebug() << "Последняя созданная задача:" << lastTaskFile.fileName()
                     << "| Время:" << QDateTime::fromMSecsSinceEpoch(lastTaskTime).toString("hh:mm:ss");
                     //<< "| Разница с текущим:" << timeDiff/1000 << "сек";

            if (timeDiff >= fiveMinutesMs) {
                // Вычисляем следующее время задачи, кратное 5 минутам
                qint64 newTaskTime = lastTaskTime + fiveMinutesMs;
                qDebug() << "Прошло больше 5 минут, создаем новую задачу.";
                createTask(newTaskTime);
            } else {
                qDebug() << "Прошло меньше 5 минут:"; //<< timeDiff/1000 << "сек";
            }
        } else {
            qDebug() << "В папке нет задач, создаем новую";
            createTask(nextExpectedTime);
        }
    } else {
        qDebug() << "Пропусков нет";
    }
}

// вывод в лог времени в понятном формате
QString TaskManager::formatTimestamp(qint64 timestamp) const {
    // Убедимся, что timestamp в миллисекундах
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp);
    return dt.toString("dd.MM.yyyy hh:mm:ss");
}
