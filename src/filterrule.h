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
        Include, // only files with extensions from [m_extensions] are allowed, others are ignored
        Ignore   // files with extensions from [m_extensions] are ignored (not included in the database)
    };

    FilterRule(bool ignoreSummaries = true);
    FilterRule(const FilterMode filterMode, const QStringList &extensions);

    FilterMode mode() const;                                                            // returns m_mode
    void setFilter(const FilterMode filterMode, const QString &extensions);
    void setFilter(const FilterMode filterMode, const QStringList &extensions);
    void clearFilter();                                                                 // sets defaults
    bool isFilter(const FilterMode filterMode) const;                                   // checks whether the specified mode is setted (m_mode == filterMode)
    bool isEnabled() const;                                                             // != NotSet
    bool isFileAllowed(const QString &filePath) const;                                  // whether the file extension matches the filter rules
    QString extensionString(const QString &sep = QStringLiteral(u", ")) const;          // creates a string listing the elements of a list (m_extensions)
    const QStringList& extensionList() const;                                           // returns a ref to m_extensions

    explicit operator bool() const { return isEnabled(); }

    bool ignoreShaFiles = true;
    bool ignoreDbFiles = true;

private:
    FilterMode m_mode = NotSet;                                                          // current mode. if set, the extension list is assumed to be non-empty
    QStringList m_extensions;                                                            // list of file extensions (suffixes) for filtering
}; // class FilterRule

using FilterMode = FilterRule::FilterMode;

#endif // FILTERRULE_H
