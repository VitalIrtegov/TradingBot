#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "keymanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>
#include <QDateTime>
#include <QInputDialog>
//#include <QMessageBox> // для записи токена
//#include <QFileInfo> // для записи токена

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_engine(new TradingEngine(this))
{
    //setupBotToken(); // запись токена нужна один раз

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

MainWindow::~MainWindow() {
    delete ui;
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

/*void MainWindow::setupBotToken() { // НЕ УДАЛЯТЬ!!!! Нужно для записи токена
    KeyManager& km = KeyManager::instance();

    bool ok;
    QString token = QInputDialog::getText(
        this,
        "Ввод токена бота",
        "Введите токен Telegram бота:",
        QLineEdit::Normal,
        "",
        &ok
        );
    if (!ok || token.isEmpty()) {
        qDebug() << "Пустая строка или отмена";
        return;
    }

    // Простая валидация формата токена
    if (!token.contains(':') || token.length() < 30) {
        QMessageBox::warning(this,
                             "Неверный формат",
                             "Токен должен быть в формате 123456789:ABC-DEF1234ghIkl...");
        return;
    }

    // Сохраняем токен
    if (km.storeBotToken(token)) {
        qDebug() << "Токен успешно сохранён!";

        // Проверяем, что файл создан
        QFileInfo tokenFile("token.bin");
        if (tokenFile.exists()) {
            qDebug() << "Файл token.bin создан, размер:"
                     << tokenFile.size() << "байт";
        } else {
            qCritical() << "Ошибка: файл токена не создан!";
        }
    } else {
        qCritical() << "Не удалось сохранить токен!";
    }

    return;
}*/

