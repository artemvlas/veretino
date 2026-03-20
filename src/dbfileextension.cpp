#include "dbfileextension.h"
#include "pathstr.h"

const QString DbFileExtension::s_extLong = QStringLiteral(u"ver.json");
const QString DbFileExtension::s_extShort = QStringLiteral(u"ver");

DbFileExtension::DbFileExtension(const QString &dbFile)
    : m_dbFile(&dbFile) {}

QString DbFileExtension::extension() const
{
    return hasShort() ? s_extShort : s_extLong;
}

bool DbFileExtension::hasLong() const
{
    return hasLong(*m_dbFile);
}

bool DbFileExtension::hasShort() const
{
    return hasShort(*m_dbFile);
}

QString DbFileExtension::extension(bool isLong)
{
    return isLong ? s_extLong : s_extShort;
}

bool DbFileExtension::hasLong(const QString &dbFile)
{
    return pathstr::hasExtension(dbFile, s_extLong);
}

bool DbFileExtension::hasShort(const QString &dbFile)
{
    return pathstr::hasExtension(dbFile, s_extShort);
}

bool DbFileExtension::isDbFile(const QString &filePath)
{
    // or just hasLong(...) || hasShort(...)
    return pathstr::hasExtension(filePath, s_extLong)
           || pathstr::hasExtension(filePath, s_extShort);
}
