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
    enum FilterMode : quint8 {
        NotSet,
        Include, // only files with extensions from [extensionsList] are allowed, others are ignored
        Ignore   // files with extensions from [extensionsList] are ignored (not included in the database)
    };

    FilterRule(bool ignoreSummaries = true);
    FilterRule(const FilterMode filterMode, const QStringList &extensions);

    FilterMode mode() const;
    void setFilter(const FilterMode filterMode, const QString &extensions);
    void setFilter(const FilterMode filterMode, const QStringList &extensions);
    void clearFilter(); // set defaults
    bool isFilter(const FilterMode filterMode) const;
    bool isFilterEnabled() const;
    bool isFileAllowed(const QString &filePath) const; // whether the file extension matches the filter rules
    QString extensionString(const QString &sep = ", ") const;

    QStringList extensionsList;
    bool ignoreShaFiles = true;
    bool ignoreDbFiles = true;

private:
    FilterMode mode_ = NotSet;
}; // class FilterRule

using FilterMode = FilterRule::FilterMode;

#endif // FILTERRULE_H
