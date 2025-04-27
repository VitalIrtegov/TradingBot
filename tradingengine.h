#pragma once
#include <QObject>
#include <QTimer>

class TradingEngine : public QObject
{
    Q_OBJECT

public:
    explicit TradingEngine(QObject *parent = nullptr);
    void startTrading();
    void stopTrading();
    void testLogOutput(); // НЕ УДАЛЯТЬ!!!! Тестовый вызов для проверки класса TradingEngine

signals:
    void newLogMessage(const QString &message, const QString &type);  // Для логов  // Без реализации! Так нужно для связимежду классами
    void marketDataUpdated(double price);       // Пример данных рынка

private slots:
    void processMarketData();  // Анализ данных

private:
    QTimer *m_timer;
    bool m_isRunning = false;

};
