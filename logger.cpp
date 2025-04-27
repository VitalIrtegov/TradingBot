#include "logger.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QScrollBar>

Logger::Logger(QWidget *parent) : QWidget(parent) {
    QDir().mkdir("logs"); // Создаем папку для логов, если ее нет
    setupUI();
    loadLogsForDate(QDate::currentDate());
}

void Logger::setupUI() {
    // Фиксируем размер окна (ширина, высота)
    //setFixedSize(800, 600);

    // Или задаём минимальный/максимальный размер
    setMinimumSize(640, 480);
    setMaximumSize(1024, 768);

    QVBoxLayout *layout = new QVBoxLayout(this);

    m_dateSelector = new QDateEdit(this);
    m_dateSelector->setDisplayFormat("dd.MM.yyyy");
    m_dateSelector->setDate(QDate::currentDate());
    m_dateSelector->setCalendarPopup(true);

    // Прямое подключение без промежуточного слота
    connect(m_dateSelector, &QDateEdit::dateChanged, [this](const QDate &date){
        loadLogsForDate(date);
    });

    m_logDisplay = new QTextEdit(this);
    m_logDisplay->setReadOnly(true);

    layout->addWidget(m_dateSelector);
    layout->addWidget(m_logDisplay);
    setLayout(layout);
}

void Logger::writeLog(const QString &message, const QString &type) {
    // Создаем папку logs, если ее нет
    QDir logsDir("logs");
    if (!logsDir.exists()) {
        if (!logsDir.mkpath(".")) {
            qWarning() << "Не удалось создать папку logs!";
            return;
        }
    }

    QString normalizedType = type.toUpper();
    QString today = QDate::currentDate().toString("dd.MM.yyyy");

    // Полный путь к файлу
    QString logFilePath = "logs/logs_" + today + ".json";

    // Загружаем текущие логи (если файл существует)
    if (m_currentLogFile != logFilePath) {
        m_currentLogFile = logFilePath;
        m_currentLogs = QJsonArray();

        QFile checkFile(logFilePath);
        if (checkFile.exists() && checkFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray jsonData = checkFile.readAll();
            checkFile.close();

            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

            if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
                m_currentLogs = doc.array();
            }
        }
    }

    // Создаем запись лога
    QJsonObject logEntry;
    logEntry["timestamp"] = QDateTime::currentDateTime().toString("hh:mm:ss");
    logEntry["type"] = normalizedType;
    logEntry["message"] = message;

    // Добавляем в массив
    m_currentLogs.append(logEntry);

    // Сохраняем в файл
    QFile logFile(logFilePath);
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonDocument doc(m_currentLogs);
        logFile.write(doc.toJson(QJsonDocument::Indented));
        logFile.close();
    } else {
        qWarning() << "Не удалось открыть файл лога для записи:" << logFile.errorString();
    }
}

void Logger::loadLogsForDate(const QDate &date) {
    QString dateStr = date.toString("dd.MM.yyyy");
    QString filename = "logs/logs_" + dateStr + ".json";

    // Проверяем существование папки
    QDir logsDir("logs");
    if (!logsDir.exists()) {
        m_logDisplay->setText("Папка logs не существует");
        return;
    }

    // Проверяем существование файла
    QFileInfo fileInfo(filename);
    if (!fileInfo.exists()) {
        m_logDisplay->setText(QString("Файл логов за %1 не найден").arg(dateStr));
        return;
    }

    loadLogsFromFile(filename);
}

void Logger::loadLogsFromFile(const QString &filename) {
    m_logDisplay->clear();

    QFile logFile(filename);
    if (!logFile.exists()) {
        m_logDisplay->append("Логи за выбранную дату отсутствуют");
        return;
    }

    if (logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray jsonData = logFile.readAll();
        logFile.close();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

        if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
            QJsonArray logs = doc.array();
            foreach (const QJsonValue &value, logs) {
                QJsonObject obj = value.toObject();
                QString logLine = QString("[%1] %2: %3")
                                      .arg(obj["timestamp"].toString())
                                      .arg(obj["type"].toString())
                                      .arg(obj["message"].toString());
                m_logDisplay->append(logLine);
            }
        } else {
            m_logDisplay->append("Ошибка чтения лог-файла");
        }
    }
}
