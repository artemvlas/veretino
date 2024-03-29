/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef FILTERRULE_H
#define FILTERRULE_H
#include <QStringList>

class FilterRule
{
public:
    enum ExtensionsFilter {NotSet, Include, Ignore};
    FilterRule(bool ignoreSummaries = true);
    FilterRule(const ExtensionsFilter filterType, const QStringList &extensions);

    void setFilter(const ExtensionsFilter filterType, const QStringList &extensions);
    void clearFilter(); // set defaults
    bool isFilter(const ExtensionsFilter filterType) const;
    bool isFileAllowed(const QString &filePath) const; // whether the file extension matches the filter rules

    ExtensionsFilter extensionsFilter_ = NotSet;
    QStringList extensionsList;
    bool ignoreShaFiles = true;
    bool ignoreDbFiles = true;
}; // class FilterRule

#endif // FILTERRULE_H
