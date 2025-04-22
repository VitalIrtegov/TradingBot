#ifndef KEYMANAGER_H
#define KEYMANAGER_H

#include <QByteArray>
#include <QString>

class KeyManager {
public:
    static KeyManager& instance();

    // Основной API
    bool storeBotToken(const QString& token);
    QString loadBotToken();
    bool hasStoredBotToken() const;

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

    bool initKeys();
    bool saveKeys();
    bool loadKeys();

    QByteArray generateSecureKey(int size);
    QByteArray protectData(const QByteArray& data);
    QByteArray unprotectData(const QByteArray& protectedData);
    QByteArray calculateHmac(const QByteArray& data);
};

#endif // KEYMANAGER_H
