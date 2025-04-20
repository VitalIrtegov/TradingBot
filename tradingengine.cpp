#include "tradingengine.h"
#include <QDebug>
#include <QDateTime>
//#include <cstdlib>  // Для srand()
//#include <ctime>    // Для time()

TradingEngine::TradingEngine(QObject *parent) : QObject(parent)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &TradingEngine::processMarketData);
}

void TradingEngine::startTrading()
{
    if (m_isRunning) return;

    m_isRunning = true;
    m_timer->start(1000);  // 1 сек интервал
    emit newLogMessage("TradingEngine: Запущен");
}

void TradingEngine::stopTrading()
{
    m_timer->stop();
    m_isRunning = false;
    emit newLogMessage("TradingEngine: Остановлен");
}

void TradingEngine::processMarketData()
{
    // Здесь ваша логика: анализ, сделки и т.д.
    /*double fakePrice = 100 + (srand(QDateTime::currentMSecsSinceEpoch() / 1000) % 10);  // Пример данных
    emit marketDataUpdated(fakePrice);
    emit newLogMessage(QString("Цена обновлена: %1").arg(fakePrice));*/
}
