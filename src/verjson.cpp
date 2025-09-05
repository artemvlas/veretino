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
{
    //if (QFile::exists(filePath))
    //    load();
}

const QString& VerJson::file_path() const
{
    return m_file_path;
}

void VerJson::setFilePath(const QString &filePath)
{
    m_file_path = filePath;
}

bool VerJson::setFile(const QString &filePath)
{
    if (!QFile::exists(filePath)) {
        qWarning() << "File not found:" + filePath;
        return false;
    }

    m_file_path = filePath;
    return load();
}

bool VerJson::load()
{
    if (!QFile::exists(m_file_path)) {
        qWarning() << "File not found:" + m_file_path;
        return false;
    }

    QFile jFile(m_file_path);
    if (!jFile.open(QFile::ReadOnly)) {
        qWarning() << "Failed to open file!";
        return false;
    }

    QByteArray ba = jFile.readAll();

    if (ba.startsWith("PK")) { // is compressed
        QMicroz qmz(ba);
        if (qmz)
            ba = qmz.extractData(0);
    }

    const QJsonDocument doc = QJsonDocument::fromJson(ba);

    // the Veretino json file is QJsonArray of QJsonObjects [{}, {}, ...]
    if (doc.isArray()) {
        QJsonArray main_array = doc.array();
        if (main_array.size() > 1) {
            QJsonValueRef header = main_array[0];
            QJsonValueRef main_list = main_array[1];

            if (header.isObject() && main_list.isObject()) {
                m_header = header.toObject();
                m_data = main_list.toObject();

                if (main_array.size() > 2) {
                    QJsonValueRef additional = main_array[2];
                    m_unreadable = additional.isObject() ? additional.toObject().value(a_key_Unreadable).toArray()
                                                         : QJsonArray();
                }

                return true;
            }
        }
    }

    qWarning() << "Corrupted or unreadable Json Database:" + m_file_path;
    return false;
}

bool VerJson::save()
{
    if (m_data.isEmpty()) {
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
    content.append(m_data);

    if (!m_unreadable.isEmpty()) {
        QJsonObject unr;
        unr[a_key_Unreadable] = m_unreadable;
        content.append(unr);
    }

    const QByteArray data = QJsonDocument(content).toJson();

    if (pathstr::hasExtension(m_file_path, Lit::sl_db_exts.at(1))) { // *.ver should be compressed
        return QMicroz::compress_buf(data,
                                     QStringLiteral("checksums.ver.json"),
                                     m_file_path);
    }

    QFile file(m_file_path);

    return (file.open(QFile::WriteOnly) && file.write(data));
}

void VerJson::addItem(const QString &file, const QString &checksum)
{
    m_data[file] = checksum;
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
    m_header[QStringLiteral(u"Total Checksums")] = m_data.size();

    if (!m_header.contains(h_key_Algo))
        m_header[h_key_Algo] = format::algoToStr(algorithm());
}

QString VerJson::findValueStr(const QJsonObject &object, const QString &approxKey, int sampleLength) const
{
    QJsonObject::const_iterator i;

    for (i = object.constBegin(); i != object.constEnd(); ++i) {
        if (i.key().contains(approxKey.left(sampleLength), Qt::CaseInsensitive)) {
            return i.value().toString();
        }
    }

    return QString();
}

QString VerJson::getInfo(const QString &header_key) const
{
    if (m_header.contains(header_key))
        return m_header.value(header_key).toString();

    return findValueStr(m_header, header_key);
}

QString VerJson::firstValueString(const QJsonObject &obj) const
{
    return !obj.isEmpty() ? obj.begin().value().toString() : QString();
}

QCryptographicHash::Algorithm VerJson::algorithm() const
{
    const QString first_value = firstValueString(m_data);

    // main list object
    if (tools::canBeChecksum(first_value))
        return tools::algoByStrLen(first_value.length());

    // header object
    const QString strAlgo = findValueStr(m_header, QStringLiteral(u"Algo"));

    if (strAlgo.isEmpty()) {
        qWarning() << "VerJson::algorithm >> Not found!";
        return static_cast<QCryptographicHash::Algorithm>(0);
    }

    return tools::strToAlgo(strAlgo);
}

const QJsonObject& VerJson::data() const
{
    return m_data;
}

const QJsonArray& VerJson::unreadableFiles() const
{
    return m_unreadable;
}
