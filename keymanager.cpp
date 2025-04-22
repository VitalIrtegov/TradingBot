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

bool KeyManager::initKeys() {
    m_aesKey = generateSecureKey(32);
    m_hmacKey = generateSecureKey(32);
    return saveKeys();
}

bool KeyManager::saveKeys() {
    QByteArray combined = m_aesKey + m_hmacKey;
    QByteArray protectedData = protectData(combined);

    QFile keyFile("keys.bin");
    if (!keyFile.open(QIODevice::WriteOnly)) return false;

    // Добавляем HMAC для проверки целостности
    QByteArray hmac = QCryptographicHash::hash(protectedData, QCryptographicHash::Sha256);
    return keyFile.write(hmac + protectedData) == (hmac.size() + protectedData.size());
}

bool KeyManager::loadKeys() {
    QFile keyFile("keys.bin");
    if (!keyFile.open(QIODevice::ReadOnly)) return false;

    QByteArray data = keyFile.readAll();
    if (data.size() < 32) return false; // 32 байта HMAC

    QByteArray storedHmac = data.left(32);
    QByteArray protectedData = data.mid(32);

    // Проверка целостности
    if (QCryptographicHash::hash(protectedData, QCryptographicHash::Sha256) != storedHmac) {
        return false;
    }

    QByteArray decrypted = unprotectData(protectedData);
    if (decrypted.size() != 64) return false; // 32+32

    m_aesKey = decrypted.left(32);
    m_hmacKey = decrypted.mid(32);
    return true;
}

bool KeyManager::storeBotToken(const QString& token) {
    if (token.isEmpty()) return false;

    QByteArray tokenData = token.toUtf8();
    QByteArray protectedData = protectData(tokenData);
    if (protectedData.isEmpty()) return false;

    QFile tokenFile("bot_token.bin");
    if (!tokenFile.open(QIODevice::WriteOnly)) return false;

    // Добавляем HMAC для проверки целостности
    QByteArray hmac = QCryptographicHash::hash(protectedData, QCryptographicHash::Sha256);
    return tokenFile.write(hmac + protectedData) == (hmac.size() + protectedData.size());
}

QString KeyManager::loadBotToken() {
    QFile tokenFile("bot_token.bin");
    if (!tokenFile.open(QIODevice::ReadOnly)) return QString();

    QByteArray fileData = tokenFile.readAll();
    if (fileData.size() < 32) return QString(); // HMAC-SHA256 занимает 32 байта

    QByteArray storedHmac = fileData.left(32);
    QByteArray protectedData = fileData.mid(32);

    // Проверка целостности
    if (QCryptographicHash::hash(protectedData, QCryptographicHash::Sha256) != storedHmac) {
        return QString();
    }

    QByteArray tokenData = unprotectData(protectedData);
    return QString::fromUtf8(tokenData);
}

bool KeyManager::hasStoredBotToken() const {
    QFile tokenFile("bot_token.bin");
    return tokenFile.exists();
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
