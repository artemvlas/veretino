/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef FILTERRULE_H
#define FILTERRULE_H
#include <QStringList>

class FilterRule
{
public:
    enum FilterMode {NotSet, Include, Ignore};
    FilterRule(bool ignoreSummaries = true);
    FilterRule(const FilterMode filterMode, const QStringList &extensions);

    void setFilter(const FilterMode filterMode, const QStringList &extensions);
    void clearFilter(); // set defaults
    bool isFilter(const FilterMode filterMode) const;
    bool isFilterEnabled() const;
    bool isFileAllowed(const QString &filePath) const; // whether the file extension matches the filter rules

    FilterMode mode_ = NotSet;
    QStringList extensionsList;
    bool ignoreShaFiles = true;
    bool ignoreDbFiles = true;

}; // class FilterRule

#endif // FILTERRULE_H
