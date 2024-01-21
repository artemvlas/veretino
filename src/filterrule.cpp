#include "filterrule.h"
#include "tools.h"

FilterRule::FilterRule(bool ignoreSummaries)
    : ignoreShaFiles(ignoreSummaries)
    , ignoreDbFiles(ignoreSummaries)
{}

FilterRule::FilterRule(const ExtensionsFilter filterType, const QStringList &extensions)
    : extensionsFilter_(filterType)
    , extensionsList(extensions)
{}

void FilterRule::setFilter(const ExtensionsFilter filterType, const QStringList &extensions)
{
    extensionsList = extensions;
    extensionsList.isEmpty() ? extensionsFilter_ = NotSet : extensionsFilter_ = filterType;
}

void FilterRule::clearFilter()
{
    extensionsFilter_ = NotSet;
    extensionsList.clear();
    ignoreShaFiles = true;
    ignoreDbFiles = true;
}

bool FilterRule::isFilter(const ExtensionsFilter filterType) const
{
    return (filterType == extensionsFilter_);
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
