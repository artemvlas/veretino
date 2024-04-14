/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "filterrule.h"
#include "tools.h"

FilterRule::FilterRule(bool ignoreSummaries)
    : ignoreShaFiles(ignoreSummaries)
    , ignoreDbFiles(ignoreSummaries)
{}

FilterRule::FilterRule(const FilterMode filterMode, const QStringList &extensions)
{
    setFilter(filterMode, extensions);
}

void FilterRule::setFilter(const FilterMode filterMode, const QStringList &extensions)
{
    extensionsList = extensions;
    extensionsList.isEmpty() ? mode_ = NotSet : mode_ = filterMode;
    cleanUpExtList();
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

void FilterRule::cleanUpExtList()
{
    if (!extensionsList.isEmpty()) {
        extensionsList.removeDuplicates();

        QStringList list;
        if (ignoreShaFiles)
            list.append({"sha1", "sha256", "sha512"});
        if (ignoreDbFiles)
            list.append({"ver", "ver.json"});

        if (!list.isEmpty()) {
            foreach (const QString &str, list) {
                extensionsList.removeOne(str);
            }
        }
    }
}
