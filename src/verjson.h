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
    explicit operator bool() const { return !m_items.isEmpty(); }

    const QString& file_path() const;
    void setFilePath(const QString &filePath);

    bool load();
    bool save();

    void addItem(const QString &file, const QString &checksum);
    void addItemUnr(const QString &file);
    void addInfo(const QString &header_key, const QString &value);

    const QJsonObject& items() const;
    const QJsonArray& unreadableFiles() const;
    QString getInfo(const QString &header_key) const;
    QCryptographicHash::Algorithm algorithm() const;

    // static keys
    static const QString h_key_Algo;
    static const QString h_key_Comment;
    static const QString h_key_DateTime;
    static const QString h_key_Ignored;
    static const QString h_key_Included;
    static const QString h_key_WorkDir;
    static const QString h_key_Flags;

    static const QString h_key_Updated;
    static const QString h_key_Verified;

private:
    void fillHeader();

    // returns the value corresponding to the key
    // if there is no such key, searches for a match of the first characters
    QString findValue(const QJsonObject &object, const QString &key) const;

    QString m_file_path;
    QString m_workdir;
    QJsonObject m_header;
    QJsonObject m_items;
    QJsonArray m_unreadable;

    static const QString a_key_Unreadable;
}; // class VerJson

#endif // VERJSON_H
