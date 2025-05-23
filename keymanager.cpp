#include "keymanager.h"
#include <windows.h>
#include <wincrypt.h>
#include "qaesencryption.h"
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QRandomGenerator>
#include <QCryptographicHash>

KeyManager& KeyManager::instance() {
    static KeyManager km;
    return km;
}

QByteArray KeyManager::generateSecureKey(int size) {
    QByteArray key(size, 0);
    QRandomGenerator::global()->fillRange(
        reinterpret_cast<quint32*>(key.data()),
        key.size()/sizeof(quint32)
        );
    return key;
}

QByteArray KeyManager::protectData(const QByteArray& data) {
    DATA_BLOB dataIn, dataOut;
    dataIn.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(data.constData()));
    dataIn.cbData = data.size();

    if (!CryptProtectData(&dataIn, L"TelegramBotKeys", NULL, NULL, NULL, 0, &dataOut)) {
        return QByteArray();
    }

    QByteArray result(reinterpret_cast<char*>(dataOut.pbData), dataOut.cbData);
    LocalFree(dataOut.pbData);
    return result;
}

QByteArray KeyManager::unprotectData(const QByteArray& protectedData) {
    DATA_BLOB dataIn, dataOut;
    dataIn.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(protectedData.constData()));
    dataIn.cbData = protectedData.size();

    if (!CryptUnprotectData(&dataIn, NULL, NULL, NULL, NULL, 0, &dataOut)) {
        return QByteArray();
    }

    QByteArray result(reinterpret_cast<char*>(dataOut.pbData), dataOut.cbData);
    LocalFree(dataOut.pbData);
    return result;
}

bool KeyManager::storeBotToken(const QString& token) {
    if (token.isEmpty()) {
        qDebug() << "Ошибка: Пустой токен не может быть сохранён";
        return false;
    }

    // 1. Преобразуем токен в бинарный формат
    QByteArray tokenData = token.toUtf8();
    qDebug() << "Токен преобразован в бинарные данные, размер:" << tokenData.size() << "байт";

    // 2. Шифруем данные
    QByteArray protectedData = protectData(tokenData);
    if (protectedData.isEmpty()) {
        qDebug() << "Ошибка шифрования: protectData вернул пустой массив";
        return false;
    }
    qDebug() << "Данные успешно зашифрованы, размер:" << protectedData.size() << "байт";

    // 3. Создаём HMAC для проверки целостности
    QByteArray hmac = QCryptographicHash::hash(protectedData, QCryptographicHash::Sha256);
    qDebug() << "HMAC создан:" << hmac.toHex();

    // 4. Открываем файл для записи
    QFile tokenFile("token.bin");
    if (!tokenFile.open(QIODevice::WriteOnly)) {
        qDebug() << "Ошибка открытия файла:" << tokenFile.errorString();
        return false;
    }

    // 5. Записываем данные (HMAC + зашифрованный токен)
    qint64 bytesWritten = tokenFile.write(hmac + protectedData);

    // 6. Проверяем корректность записи
    bool success = (bytesWritten == (hmac.size() + protectedData.size()));
    if (success) {
        qDebug() << "Токен успешно сохранён в token.bin, записано" << bytesWritten << "байт";
        //qDebug() << "Путь к файлу:" << QFileInfo(tokenFile).absoluteFilePath();
    } else {
        qDebug() << "Ошибка записи в файл. Ожидалось:"
                 << (hmac.size() + protectedData.size())
                 << "байт, записано:" << bytesWritten;
    }

    tokenFile.close();
    return success;
}

QString KeyManager::loadBotToken() {
    // 1. Открываем файл
    QFile tokenFile("token.bin");
    if (!tokenFile.exists()) {
        qDebug() << "Файл token.bin не найден";
        return QString();
    }

    if (!tokenFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Ошибка открытия файла:" << tokenFile.errorString();
        return QString();
    }

    // 2. Читаем данные
    QByteArray fileData = tokenFile.readAll();
    tokenFile.close();

    //qDebug() << "Прочитано из файла:" << fileData.size() << "байт";

    // 3. Проверяем минимальный размер (HMAC + хотя бы 1 байт данных)
    if (fileData.size() < 33) { // 32 байта HMAC + минимум 1 байт данных
        qDebug() << "Файл повреждён: слишком маленький размер";
        return QString();
    }

    // 4. Извлекаем HMAC и данные
    QByteArray storedHmac = fileData.left(32);
    QByteArray protectedData = fileData.mid(32);

    //qDebug() << "Извлечён HMAC:" << storedHmac.toHex();
    //qDebug() << "Размер защищённых данных:" << protectedData.size() << "байт";

    // 5. Проверяем целостность
    QByteArray calculatedHmac = QCryptographicHash::hash(protectedData, QCryptographicHash::Sha256);
    if (calculatedHmac != storedHmac) {
        qDebug() << "Ошибка целостности! Хранимый HMAC:" << storedHmac.toHex();
        qDebug() << "Вычисленный HMAC:" << calculatedHmac.toHex();
        return QString();
    }

    // 6. Дешифруем данные
    QByteArray tokenData = unprotectData(protectedData);
    if (tokenData.isEmpty()) {
        qDebug() << "Ошибка дешифрования: unprotectData вернул пустой массив";
        return QString();
    }

    QString token = QString::fromUtf8(tokenData);
    qDebug() << "Токен успешно загружен. Длина:" << token.length() << "символов";

    return token;
}

bool KeyManager::storeChatId(int64_t chatId) {
    // 1. Преобразуем chatId в бинарный формат
    QByteArray chatIdData(reinterpret_cast<const char*>(&chatId), sizeof(int64_t));
    qDebug() << "ChatId преобразован в бинарные данные, размер:" << chatIdData.size() << "байт";

    // 2. Шифруем данные
    QByteArray protectedData = protectData(chatIdData);
    if (protectedData.isEmpty()) {
        qDebug() << "Ошибка шифрования: protectData вернул пустой массив";
        return false;
    }
    qDebug() << "Данные успешно зашифрованы, размер:" << protectedData.size() << "байт";

    // 3. Создаём HMAC для проверки целостности
    QByteArray hmac = QCryptographicHash::hash(protectedData, QCryptographicHash::Sha256);
    qDebug() << "HMAC создан:" << hmac.toHex();

    // 4. Открываем файл для записи
    QFile chatIdFile("chatId.bin");
    if (!chatIdFile.open(QIODevice::WriteOnly)) {
        qDebug() << "Ошибка открытия файла:" << chatIdFile.errorString();
        return false;
    }

    // 5. Записываем данные (HMAC + зашифрованный chatId)
    qint64 bytesWritten = chatIdFile.write(hmac + protectedData);

    // 6. Проверяем корректность записи
    bool success = (bytesWritten == (hmac.size() + protectedData.size()));
    if (success) {
        qDebug() << "ChatId успешно сохранён в chatId.bin, записано" << bytesWritten << "байт";
    } else {
        qDebug() << "Ошибка записи в файл. Ожидалось:"
                 << (hmac.size() + protectedData.size())
                 << "байт, записано:" << bytesWritten;
    }

    chatIdFile.close();
    return success;
}

int64_t KeyManager::loadChatId() {
    // Открываем файл
    QFile chatIdFile("chatId.bin");
    if (!chatIdFile.exists()) {
        qDebug() << "Файл chatId.bin не найден";
        return 0;
    }

    if (!chatIdFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Ошибка открытия файла:" << chatIdFile.errorString();
        return 0;
    }

    // Читаем данные
    QByteArray fileData = chatIdFile.readAll();
    chatIdFile.close();

    //qDebug() << "Прочитано из файла:" << fileData.size() << "байт";

    // Проверяем минимальный размер (HMAC + 8 байт для int64_t)
    if (fileData.size() < 40) { // 32 байта HMAC + 8 байт данных
        qDebug() << "Файл повреждён: слишком маленький размер";
        return 0;
    }

    // Извлекаем HMAC и данные
    QByteArray storedHmac = fileData.left(32);
    QByteArray protectedData = fileData.mid(32);

    //qDebug() << "Размер защищённых данных:" << protectedData.size() << "байт";

    // Проверяем целостность
    QByteArray calculatedHmac = QCryptographicHash::hash(protectedData, QCryptographicHash::Sha256);
    if (calculatedHmac != storedHmac) {
        qDebug() << "Ошибка целостности! Хранимый HMAC:" << storedHmac.toHex();
        qDebug() << "Вычисленный HMAC:" << calculatedHmac.toHex();
        return 0;
    }

    // Дешифруем данные
    QByteArray chatIdData = unprotectData(protectedData);
    if (chatIdData.size() != sizeof(int64_t)) {
        qDebug() << "Ошибка дешифрования: неверный размер данных";
        return 0;
    }

    int64_t chatId = *reinterpret_cast<const int64_t*>(chatIdData.constData());
    qDebug() << "ChatId успешно загружен.";

    return chatId;
}

// Шифрование сообщения
QByteArray KeyManager::encryptMessage(const QString& text)
{
    QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::CBC);
    QByteArray iv = generateSecureKey(16); // Вектор инициализации
    QByteArray encrypted = encryption.encode(text.toUtf8(), m_aesKey, iv);
    return iv + encrypted; // IV (16 байт) + зашифрованные данные
}

// Дешифрование сообщения
QString KeyManager::decryptMessage(const QByteArray& encrypted)
{
    if (encrypted.size() <= 16) return QString(); // IV + минимум 1 блок данных

    QAESEncryption encryption(QAESEncryption::AES_256, QAESEncryption::CBC);
    QByteArray iv = encrypted.left(16);
    QByteArray ciphertext = encrypted.mid(16);
    QByteArray decrypted = encryption.decode(ciphertext, m_aesKey, iv);

    return QString::fromUtf8(decrypted);
}

// Безопасная очистка данных
void KeyManager::clearSensitiveData()
{
    // Затираем критические данные в памяти
    if (!m_aesKey.isEmpty()) {
        memset(m_aesKey.data(), 0, m_aesKey.size());
        m_aesKey.clear();
    }
    if (!m_hmacKey.isEmpty()) {
        memset(m_hmacKey.data(), 0, m_hmacKey.size());
        m_hmacKey.clear();
    }
}

// Вычисление HMAC
QByteArray KeyManager::calculateHmac(const QByteArray& data)
{
    if (m_hmacKey.isEmpty()) {
        qCritical() << "HMAC key not initialized!";
        return QByteArray();
    }

    // HMAC-SHA256 реализация
    QByteArray key = m_hmacKey;
    if (key.size() > 64) { // Блоковый размер для SHA-256
        key = QCryptographicHash::hash(key, QCryptographicHash::Sha256);
    }

    key.resize(64, '\0'); // Дополняем ключ до размера блока

    QByteArray innerPad(64, 0x36);
    QByteArray outerPad(64, 0x5C);

    for (int i = 0; i < 64; ++i) {
        innerPad[i] = innerPad[i] ^ key[i];
        outerPad[i] = outerPad[i] ^ key[i];
    }

    QByteArray innerData = innerPad + data;
    QByteArray innerHash = QCryptographicHash::hash(innerData, QCryptographicHash::Sha256);
    QByteArray outerData = outerPad + innerHash;

    return QCryptographicHash::hash(outerData, QCryptographicHash::Sha256);
}
