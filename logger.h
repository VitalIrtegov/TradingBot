#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QWidget>
#include <QFile>
#include <QJsonArray>
#include <QDateTime>
#include <QMap>
#include <QTextEdit>
#include <QPushButton>
#include <QDateEdit>
#include <QVBoxLayout>

class Logger : public QWidget {
    Q_OBJECT

public:
    explicit Logger(QWidget *parent = nullptr);
    void writeLog(const QString &message, const QString &type = "INFO");
    void loadLogsForDate(const QDate &date);

private:
    QTextEdit *m_logDisplay;
    QDateEdit *m_dateSelector;
    QJsonArray m_currentLogs;
    QString m_currentLogFile;

    void setupUI();
    void loadLogsFromFile(const QString &filename);
    bool isValidType(const QString &type) const;
};

#endif // LOGGER_H
