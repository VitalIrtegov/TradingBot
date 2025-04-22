#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),  // Правильное имя указателя
    m_engine(new TradingEngine(this))  // Отдельная инициализация
{
    setWindowTitle("Trading Bot Controller");
    //resize(800, 600); // по умолчанию

    // Фиксируем размер окна (ширина, высота)
    setFixedSize(800, 600);

    // Или задаём минимальный/максимальный размер
    setMinimumSize(640, 480);
    setMaximumSize(1024, 768);

    // Создаём главный виджет и основной layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Создаём панель кнопок
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    startButton = new QPushButton("Старт", this);
    stopButton = new QPushButton("Стоп", this);

    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->setAlignment(Qt::AlignLeft);

    // Создаём поле для логов
    logTextEdit = new QTextEdit(this);
    logTextEdit->setReadOnly(true);

    // Добавляем всё в главный layout
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(logTextEdit);

    // Устанавливаем центральный виджет
    setCentralWidget(centralWidget);

    // Подключаем сигналы и слоты
    connect(startButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);
    connect(m_engine, &TradingEngine::newLogMessage, this, &MainWindow::logMessage);

    //m_engine->testLogOutput();  // НЕ УДАЛЯТЬ!!!! Тестовый вызов для проверки класса TradingEngine
}

void MainWindow::onStartButtonClicked() {
    logMessage("Бот запущен!");
    // Здесь будет основная логика
}

void MainWindow::onStopButtonClicked() {
    logMessage("Бот остановлен!");
    // Здесь будет основная логика
}

void MainWindow::logMessage(const QString &message) {
    //qDebug() << "Получено сообщение:" << message;  // Отладка в консоль
    logTextEdit->append(QDateTime::currentDateTime().toString("[hh:mm:ss] ") + message);
    //logTextEdit->append(message);
}

MainWindow::~MainWindow() {
    delete ui;
}
