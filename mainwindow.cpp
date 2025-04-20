#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {

    ui->setupUi(this);
    setupUI();

    // Пример: кнопка "Старт"
    QPushButton *startButton = new QPushButton("Старт", this);
    connect(startButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    ui->statusBar->addWidget(startButton);

    // Поле для логов
    QTextEdit *logWidget = new QTextEdit(this);
    logWidget->setReadOnly(true);
    setCentralWidget(logWidget);
}

void MainWindow::setupUI()
{
    setWindowTitle("Trading Bot Controller");
    resize(800, 600);
}

void MainWindow::onStartButtonClicked()
{
    logMessage("Бот запущен!");
    // Здесь будет основная логика
}

void MainWindow::logMessage(const QString &message)
{
    ui->centralWidget()->append(message);  // Вывод в QTextEdit
}



MainWindow::~MainWindow()
{
    delete ui;
}
