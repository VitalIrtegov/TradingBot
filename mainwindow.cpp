#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "keymanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>
#include <QDateTime>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkAccessManager>
//#include <QMessageBox> // для записи токена и ChatId
//#include <QFileInfo> // для записи токена и ChatId
//#include <QInputDialog> // для записи токена и ChatId

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_engine(new TradingEngine(this)),
    m_network(new QNetworkAccessManager(this)),
    m_botToken(KeyManager::instance().loadBotToken()),
    m_chatId(KeyManager::instance().loadChatId())
{
    //setupBotToken(); // запись токена нужна один раз
    //setupBotChatId(); // запись ChatId нужна один раз

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

    // Инициализация кнопок в Telegram
    initTelegramKeyboard();

    // Проверка обновлений каждые 3 секунды
    QTimer *updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, [this]() {
        sendToTelegram("getUpdates", {
            {"offset", m_lastUpdateId + 1},
            {"timeout", 10},
            {"allowed_updates", QJsonArray{"callback_query"}}
        });
    });
    updateTimer->start(3000);

    //m_engine->testLogOutput();  // НЕ УДАЛЯТЬ!!!! Тестовый вызов для проверки класса TradingEngine
}

MainWindow::~MainWindow() {
    delete ui;
}

// Создает сообщение с кнопками управления в канале
void MainWindow::initTelegramKeyboard() {
    sendToTelegram("sendMessage", {
        {"chat_id", m_chatId},
        {"text", "Управление ботом:"},
        {"reply_markup", QJsonObject{
            {"inline_keyboard", QJsonArray{
                QJsonArray{
                    QJsonObject{{"text", "▶️ Старт"}, {"callback_data", "cmd_start"}},
                    QJsonObject{{"text", "⏹️ Стоп"}, {"callback_data", "cmd_stop"}}
                }
            }}
        }}
    });
}

// Обрабатывает действия пользователя из Telegram
void MainWindow::processCallback(const QJsonObject &callback) {
    const QString data = callback["data"].toString();
    //logMessage("Telegram: " + data); // Логируем нажатие

    if (data == "cmd_start") onStartButtonClicked();
    else if (data == "cmd_stop") onStopButtonClicked();

    // Подтверждаем обработку
    sendToTelegram("answerCallbackQuery", {
        {"callback_query_id", callback["id"].toString()},
        {"text", "Команда принята"}
    });
}

// Универсальный метод для API запросов
void MainWindow::sendToTelegram(const QString &method, const QJsonObject &params) {
    QUrl url(QString("https://api.telegram.org/bot%1/%2").arg(m_botToken).arg(method));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_network->post(request, QJsonDocument(params).toJson());
    connect(reply, &QNetworkReply::finished, [this, reply, method]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (method == "getUpdates") {
                for (const QJsonValue &update : doc.object()["result"].toArray()) {
                    qint64 updateId = update["update_id"].toInt();
                    if (updateId > m_lastUpdateId) {
                        m_lastUpdateId = updateId;
                        if (update.toObject().contains("callback_query")) {
                            processCallback(update["callback_query"].toObject());
                        }
                    }
                }
            }
        }
        reply->deleteLater();
    });
}

void MainWindow::onStartButtonClicked() {
    logMessage("Бот запущен!");
}

void MainWindow::onStopButtonClicked() {
    logMessage("Бот остановлен!");
}

void MainWindow::logMessage(const QString &message) {
    logTextEdit->append(QDateTime::currentDateTime().toString("[hh:mm:ss] ") + message);
}

/*void MainWindow::setupBotToken() { // НЕ УДАЛЯТЬ!!!! Нужно для записи токена
    KeyManager& km = KeyManager::instance();

    bool ok;
    QString token = QInputDialog::getText(this, "Ввод токена бота", "Введите токен:", QLineEdit::Normal, "", &ok);

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

/*void MainWindow::setupBotChatId() { // НЕ УДАЛЯТЬ!!!! Нужно для записи Chat ID
    KeyManager& km = KeyManager::instance();

    bool ok;
    QString text = QInputDialog::getText(this, "Ввод Chat ID", "Введите ID чата:", QLineEdit::Normal, "", &ok);

    if (!ok || text.isEmpty()) {
        qDebug() << "Пустая строка или отмена";
        return;
    }

    // Преобразуем строку в int64_t
    bool conversionOk;
    int64_t chatId = text.toLongLong(&conversionOk);

    if (!conversionOk) {
        QMessageBox::warning(this, "Ошибка", "Некорректный формат Chat ID");
        return;
    }

    // Сохраняем Chat ID
    if (km.storeChatId(chatId)) {
        qDebug() << "Chat ID успешно сохранён!";

        // Проверяем, что файл создан
        QFileInfo tokenFile("chatId.bin");
        if (tokenFile.exists()) {
            qDebug() << "Файл chatId.bin создан, размер:"
                     << tokenFile.size() << "байт";
        } else {
            qCritical() << "Ошибка: файл chatId не создан!";
        }
    } else {
        qCritical() << "Не удалось сохранить chatId!";
    }

    return;
}*/





