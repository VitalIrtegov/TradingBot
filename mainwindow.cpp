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
    m_engine(new TradingEngine(this)), // НЕ УДАЛЯТЬ!!!!  для класса TradingEngine
    m_network(new QNetworkAccessManager(this)),
    m_dataStreamer(new DataStreamer(this)),
    m_botToken(KeyManager::instance().loadBotToken()),
    m_chatId(KeyManager::instance().loadChatId()),
    m_logger(new Logger()),
    m_viewer(new BinViewer(this))
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
    logsButton = new QPushButton("Показать логи", this);
    viewBinButton = new QPushButton("Файлы bin", this);

    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addWidget(logsButton);
    buttonLayout->addWidget(viewBinButton);
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
    connect(logsButton, &QPushButton::clicked, this, &MainWindow::showLogsWindow);

    // Подключаем сигнал TradingEngine для логов
    connect(m_engine, &TradingEngine::newLogMessage, this, &MainWindow::logMessage);

    // Подключаем сигналы DataStreamer для логов
    connect(m_dataStreamer, &DataStreamer::newLogMessage, this, &MainWindow::logMessage);

    connect(viewBinButton, &QPushButton::clicked, this, &MainWindow::onBinViewerClicked);

    // Инициализация кнопок в Telegram
    //initTelegramKeyboard();

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

    m_logger->writeLog("Программа запущена", "SYSTEM"); // Записываем время запуска

    //m_engine->testLogOutput();  // НЕ УДАЛЯТЬ!!!! Тестовый вызов для проверки класса TradingEngine
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
    //logMessage("Бот запущен!", "INFO");
    //sendTelegramSignal("EUR/USD", "ВНИЗ", "4мин", 1.2650, 1.2600, 1.2700);
    m_dataStreamer->DataStreamer::startStream();
}

void MainWindow::onStopButtonClicked() {
    //logMessage("Бот остановлен!", "INFO");
    //sendTelegramResult("EUR/USD", "SL", 1.2650, 1.2450, 100, "4мин");
    m_dataStreamer->DataStreamer::stopStream();
}

void MainWindow::onBinViewerClicked() {
    m_viewer->BinViewer::showAndDisplay(); // просмотр бин файлов
}

void MainWindow::sendTelegramSignal(const QString &currencyPair, const QString &direction, const QString &duration,
                                    double price, double takeProfit, double stopLoss) {
    // Форматируем сообщение с MarkdownV2
    QString message = QString(
        "🚀 *Торговый сигнал* 🚀\n"
        "• Пара: `%1`\n"
        "• Сигнал: %2, %3\n"
        "• Цена: `%4`\n"
        "• TP: `%5`\n"
        "• SL: `%6`"
        ).arg(currencyPair)
         .arg(direction)
         .arg(duration)
         .arg(price, 0, 'f', 4)
         .arg(takeProfit, 0, 'f', 4)
         .arg(stopLoss, 0, 'f', 4);

    // Отправляем через универсальный метод
    sendToTelegram("sendMessage", {
        {"chat_id", m_chatId},
        {"text", message},
        {"parse_mode", "MarkdownV2"},
        {"disable_web_page_preview", true}
    });

    logMessage(QString("Отправлен сигнал: %1 %2, %3 | Цена: %4 | TP: %5 | SL: %6")
        .arg(currencyPair)
        .arg(direction)
        .arg(duration)
        .arg(price, 0, 'f', 4)
        .arg(takeProfit, 0, 'f', 4)
        .arg(stopLoss, 0, 'f', 4)
        ,"OPEN");
}

void MainWindow::sendTelegramResult(const QString &currencyPair,
                                 const QString &resultType, // "TP" / "SL" / "CLOSE"
                                 double entryPrice,
                                 double exitPrice,
                                 double profitPips,
                                 const QString &duration)
{
    // Определяем иконку и статус результата
    QString icon, status;
    if (resultType == "TP") {
        icon = "✅";
        status = "Тейк-профит";
    } else if (resultType == "SL") {
        icon = "❌";
        status = "Стоп-лосс";
    } else {
        icon = "🔹";
        status = "Закрыто вручную";
    }

    // Форматируем сообщение
    QString message = QString(
                          "%1 *Результат сделки* %1\n"
                          "• Пара: `%2`\n"
                          "• Отработка: `%3`\n"
                          "• Время удержания: %4\n"
                          "• Вход: `%5`\n"
                          "• Выход: `%6`\n"
                          "• Профит: `%7` пипс"
                          ).arg(icon)
                          .arg(currencyPair)
                          .arg(status)
                          .arg(duration)
                          .arg(entryPrice, 0, 'f', 4)
                          .arg(exitPrice, 0, 'f', 4)
                          .arg(profitPips, 0, 'f', 1);

    // Отправляем через универсальный метод
    sendToTelegram("sendMessage", {
        {"chat_id", m_chatId},
        {"text", message},
        {"parse_mode", "MarkdownV2"},
        {"disable_web_page_preview", true}
    });

    // Логируем с полными параметрами
    QString logMsg = QString("%1 %2 | Вход: %3 | Выход: %4 | Пипс: %5 | Время: %6")
                         .arg(currencyPair)
                         .arg(status)
                         .arg(entryPrice, 0, 'f', 4)
                         .arg(exitPrice, 0, 'f', 4)
                         .arg(profitPips, 0, 'f', 1)
                         .arg(duration);

    logMessage(logMsg, "RESULT");
}

void MainWindow::logMessage(const QString &message, const QString &type) {
    // вывод в интерфейс
    QString formattedMessage = QString("[%1] %2: %3")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(type)
        .arg(message);

    logTextEdit->append(formattedMessage);

    // запись в лог-файл через Logger
    /*if (m_logger) {
        m_logger->writeLog(message, type);
    }*/
}

void MainWindow::showLogsWindow() {
    if (m_logger) {
        m_logger->loadLogsForDate(QDate::currentDate());
        m_logger->show();
        m_logger->activateWindow();
    }
}

MainWindow::~MainWindow() {
    m_logger->writeLog("Программа остановлена", "SYSTEM"); // записываем время остановки
    delete ui;
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





