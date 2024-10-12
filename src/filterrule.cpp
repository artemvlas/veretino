/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "filterrule.h"
#include "tools.h"
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
    return mode_;
}

void FilterRule::setFilter(const FilterMode filterMode, const QString &extensions)
{
    static const QRegularExpression re("[, ]");
    setFilter(filterMode, extensions.split(re, Qt::SkipEmptyParts));
}

void FilterRule::setFilter(const FilterMode filterMode, const QStringList &extensions)
{
    extensions_ = extensions;
    mode_ = extensions.isEmpty() ? NotSet : filterMode;
}

void FilterRule::clearFilter()
{
    mode_ = NotSet;
    extensions_.clear();
    ignoreShaFiles = true;
    ignoreDbFiles = true;
}

bool FilterRule::isFilter(const FilterMode filterMode) const
{
    return (filterMode == mode_);
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

    // if 'isFilter(Include)': a file ('filePath') with any extension from 'extensions_' is allowed
    // if 'isFilter(Ignore)': than all files except these types are allowed

    return paths::hasExtension(filePath, extensions_) ? isFilter(Include): isFilter(Ignore);
}

QString FilterRule::extensionString(const QString &sep) const
{
    return extensions_.join(sep);
}

QStringList FilterRule::extensionList() const
{
    return extensions_;
}
