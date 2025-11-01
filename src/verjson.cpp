/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "verjson.h"
#include "tools.h"
#include "pathstr.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QDirIterator>
#include "qmicroz.h"

const QString VerJson::h_key_Algo = QStringLiteral(u"Hash Algorithm");
const QString VerJson::h_key_Comment = QStringLiteral(u"Comment");
const QString VerJson::h_key_DateTime = QStringLiteral(u"DateTime");
const QString VerJson::h_key_Ignored = QStringLiteral(u"Ignored");
const QString VerJson::h_key_Included = QStringLiteral(u"Included");
const QString VerJson::h_key_WorkDir = QStringLiteral(u"WorkDir");
const QString VerJson::h_key_Flags = QStringLiteral(u"Flags");

const QString VerJson::h_key_Updated = QStringLiteral(u"Updated");
const QString VerJson::h_key_Verified = QStringLiteral(u"Verified");

const QString VerJson::a_key_Unreadable = QStringLiteral(u"Unreadable files");

VerJson::VerJson(QObject *parent)
    : QObject(parent)
{}

VerJson::VerJson(const QString &filePath, QObject *parent)
    : QObject(parent), m_file_path(filePath)
{}

const QString& VerJson::file_path() const
{
    return m_file_path;
}

void VerJson::setFilePath(const QString &filePath)
{
    m_file_path = filePath;
}

void VerJson::load()
{
    QFile jFile(m_file_path);

    if (!jFile.open(QFile::ReadOnly))
        throw QFile::exists(m_file_path) ? ERR_READ : ERR_NOTEXIST;

    QByteArray ba = jFile.readAll();

    if (QMicroz::isArchive(ba)) { // is compressed
        QMicroz qmz(ba);
        if (qmz)
            ba = qmz.extractData(0);
    }

    const QJsonDocument doc = QJsonDocument::fromJson(ba);

    // the Veretino json file is QJsonArray of QJsonObjects [{}, {}, ...]
    if (!doc.isArray())
        throw ERR_ERROR;

    QJsonArray main_array = doc.array();

    if (main_array.size() < 2)
        throw ERR_ERROR;

    QJsonValueRef header = main_array[0];
    QJsonValueRef items = main_array[1];

    if (!header.isObject() || !items.isObject())
        throw ERR_ERROR;

    m_header = header.toObject();
    m_items = items.toObject();

    if (m_items.isEmpty())
        throw ERR_NOTFOUND;

    if (main_array.size() > 2) {
        QJsonValueRef additional = main_array[2];
        m_unreadable = additional.isObject() ? additional.toObject().value(a_key_Unreadable).toArray()
                                             : QJsonArray();
    }
}

bool VerJson::save()
{
    if (m_items.isEmpty()) {
        qWarning() << "No data to save";
        return false;
    }

    if (m_file_path.isEmpty()) {
        qWarning() << "No file path provided";
        return false;
    }

    fillHeader();

    QJsonArray content;
    content.append(m_header);
    content.append(m_items);

    if (!m_unreadable.isEmpty()) {
        QJsonObject unr;
        unr[a_key_Unreadable] = m_unreadable;
        content.append(unr);
    }

    const QByteArray data = QJsonDocument(content).toJson();

    if (pathstr::hasExtension(m_file_path, Lit::sl_db_exts.at(1))) { // *.ver should be compressed
        return QMicroz::compress(QStringLiteral("checksums.ver.json"),
                                 data,
                                 m_file_path);
    }

    QFile file(m_file_path);

    return (file.open(QFile::WriteOnly) && file.write(data));
}

void VerJson::addItem(const QString &file, const QString &checksum)
{
    m_items[file] = checksum;
}

void VerJson::addItemUnr(const QString &file)
{
    m_unreadable.append(file);
}

void VerJson::addInfo(const QString &header_key, const QString &value)
{
    m_header[header_key] = value;
}

void VerJson::fillHeader()
{
    static const QString app_origin = tools::joinStrings(Lit::s_appNameVersion, Lit::s_webpage, QStringLiteral(u" >> "));
    m_header[QStringLiteral(u"App Version")] = app_origin;
    //m_header[QStringLiteral(u"Folder")] = pathstr::basicName(workDir());
    m_header[QStringLiteral(u"Total Checksums")] = m_items.size();

    if (!m_header.contains(h_key_Algo))
        m_header[h_key_Algo] = format::algoToStr(algorithm());
}

QString VerJson::findValue(const QJsonObject &object, const QString &key) const
{
    if (object.contains(key))
        return object.value(key).toString();

    const QString search_pattern = key.left(4);
    QJsonObject::const_iterator i;

    for (i = object.constBegin(); i != object.constEnd(); ++i) {
        if (i.key().contains(search_pattern, Qt::CaseInsensitive)) {
            return i.value().toString();
        }
    }

    return QString();
}

QString VerJson::getInfo(const QString &header_key) const
{
    return findValue(m_header, header_key);
}

QCryptographicHash::Algorithm VerJson::algorithm() const
{
    const QString first_value = !m_items.isEmpty() ? m_items.begin().value().toString()
                                                   : QString();

    // main list object
    if (tools::canBeChecksum(first_value))
        return tools::algoByStrLen(first_value.length());

    // header object
    const QString strAlgo = findValue(m_header, QStringLiteral(u"Algo"));

    if (strAlgo.isEmpty()) {
        qWarning() << "VerJson::algorithm >> Not found!";
        return static_cast<QCryptographicHash::Algorithm>(0);
    }

    return tools::strToAlgo(strAlgo);
}

const QJsonObject& VerJson::items() const
{
    return m_items;
}

const QJsonArray& VerJson::unreadableFiles() const
{
    return m_unreadable;
}
