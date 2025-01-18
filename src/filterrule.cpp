/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "filterrule.h"
#include "tools.h"
#include "pathstr.h"
#include <QRegularExpression>

FilterRule::FilterRule(bool ignoreSummaries)
    : ignoreShaFiles(ignoreSummaries)
    , ignoreDbFiles(ignoreSummaries)
{}

FilterRule::FilterRule(const FilterMode filterMode, const QStringList &extensions)
{
    setFilter(filterMode, extensions);
}

FilterMode FilterRule::mode() const
{
    return m_mode;
}

void FilterRule::setFilter(const FilterMode filterMode, const QString &extensions)
{
    static const QRegularExpression re("[, ]");
    setFilter(filterMode, extensions.split(re, Qt::SkipEmptyParts));
}

void FilterRule::setFilter(const FilterMode filterMode, const QStringList &extensions)
{
    m_extensions = extensions;
    m_mode = extensions.isEmpty() ? NotSet : filterMode;
}

void FilterRule::clearFilter()
{
    m_mode = NotSet;
    m_extensions.clear();
    ignoreShaFiles = true;
    ignoreDbFiles = true;
}

bool FilterRule::isFilter(const FilterMode filterMode) const
{
    return (filterMode == m_mode);
}

bool FilterRule::isEnabled() const
{
    return !isFilter(NotSet);
}

bool FilterRule::isFileAllowed(const QString &filePath) const
{
    if (!isFilter(Include)) {
        if (paths::isDbFile(filePath))
            return !ignoreDbFiles;
        if (paths::isDigestFile(filePath))
            return !ignoreShaFiles;
    }

    if (isFilter(NotSet))
        return true;

    // if 'isFilter(Include)': a file ('filePath') with any extension from 'm_extensions' is allowed
    // if 'isFilter(Ignore)': than all files except these types are allowed

    return pathstr::hasExtension(filePath, m_extensions) ? isFilter(Include): isFilter(Ignore);
}

QString FilterRule::extensionString(const QString &sep) const
{
    return m_extensions.join(sep);
}

const QStringList& FilterRule::extensionList() const
{
    return m_extensions;
}
