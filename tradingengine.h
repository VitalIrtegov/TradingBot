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

signals:
    void newLogMessage(const QString &message);  // Для логов
    void marketDataUpdated(double price);       // Пример данных рынка

private slots:
    void processMarketData();  // Анализ данных

private:
    QTimer *m_timer;
    bool m_isRunning = false;
};
