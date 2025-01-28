/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef VERJSON_H
#define VERJSON_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>

class VerJson : public QObject
{
    Q_OBJECT
public:
    explicit VerJson(QObject *parent = nullptr);
    explicit VerJson(const QString &filePath, QObject *parent = nullptr);
    explicit operator bool() const { return !m_data.isEmpty(); }

    const QString& file_path() const;
    void setFilePath(const QString &filePath);
    bool setFile(const QString &filePath);
    bool load();
    bool save();

    void addItem(const QString &file, const QString &checksum);
    void addItemUnr(const QString &file);
    void addInfo(const QString &header_key, const QString &value);

    const QJsonObject& data() const;
    const QJsonArray& unreadableFiles() const;
    QString getInfo(const QString &header_key) const;
    QCryptographicHash::Algorithm algorithm() const;

    // static keys
    static const QString h_key_DateTime;
    static const QString h_key_Ignored;
    static const QString h_key_Included;
    static const QString h_key_Algo;
    static const QString h_key_WorkDir;
    static const QString h_key_Flags;

    static const QString h_key_Updated;
    static const QString h_key_Verified;

private:
    void fillHeader();
    QString findValueStr(const QJsonObject &object,
                         const QString &approxKey,
                         int sampleLength = 4) const;
    QString firstValueString(const QJsonObject &obj) const;

    QString m_file_path;
    QString m_workdir;
    QJsonObject m_header;
    QJsonObject m_data;
    QJsonArray m_unreadable;

    static const QString a_key_Unreadable;
}; // class VerJson

#endif // VERJSON_H
