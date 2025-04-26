#ifndef KEYMANAGER_H
#define KEYMANAGER_H

#include <QByteArray>
#include <QString>

class KeyManager {
public:
    static KeyManager& instance();

    // Основной API запись/чтение токена
    bool storeBotToken(const QString& token);
    QString loadBotToken();

    // Основной API запись/чтение chatId
    bool storeChatId(int64_t chatId);
    int64_t loadChatId();

    // Шифрование/дешифрование
    QByteArray encryptMessage(const QString& text);
    QString decryptMessage(const QByteArray& encrypted);

    // Безопасная очистка
    void clearSensitiveData();

    // Запрещаем копирование
    KeyManager(const KeyManager&) = delete;
    KeyManager& operator=(const KeyManager&) = delete;

private:
    KeyManager() = default;
    QByteArray m_aesKey;
    QByteArray m_hmacKey;

    QByteArray generateSecureKey(int size);
    QByteArray protectData(const QByteArray& data);
    QByteArray unprotectData(const QByteArray& protectedData);
    QByteArray calculateHmac(const QByteArray& data);
};

#endif // KEYMANAGER_H
