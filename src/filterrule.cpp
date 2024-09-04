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
    extensionsList = extensions;
    mode_ = extensions.isEmpty() ? NotSet : filterMode;
}

void FilterRule::clearFilter()
{
    mode_ = NotSet;
    extensionsList.clear();
    ignoreShaFiles = true;
    ignoreDbFiles = true;
}

bool FilterRule::isFilter(const FilterMode filterMode) const
{
    return (filterMode == mode_);
}

bool FilterRule::isFilterEnabled() const
{
    return !isFilter(NotSet);
}

bool FilterRule::isFileAllowed(const QString &filePath) const
{
    if (!isFilter(Include)) {
        if (tools::isDatabaseFile(filePath))
            return !ignoreDbFiles;
        if (tools::isSummaryFile(filePath))
            return !ignoreShaFiles;
    }

    if (isFilter(NotSet))
        return true;

    // if 'isFilter(FilterRule::Include)': a file ('filePath') with any extension from 'extensionsList' is allowed
    // if 'isFilter(FilterRule::Ignore)': than all files except these types are allowed

    bool allowed = isFilter(Ignore);

    foreach (const QString &ext, extensionsList) {
        if (filePath.endsWith('.' + ext, Qt::CaseInsensitive)) {
            allowed = isFilter(Include);
            break;
        }
    }

    return allowed;
}

QString FilterRule::extensionString(const QString &sep) const
{
    return extensionsList.join(sep);
}
