/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef FILTERRULE_H
#define FILTERRULE_H
#include <QStringList>
#include <QFileInfo>

class FilterRule
{
public:
    enum FilterMode : quint8 {
        NotSet,
        Include, // only files with extensions from [m_extensions] are allowed, others are ignored
        Ignore   // files with extensions from [m_extensions] are ignored (not included in the database)
    };

    enum FilterAttribute : quint8 {
        NoAttributes = 0,
        IgnoreDigestFiles = 1,
        IgnoreDbFiles = 2,
        IgnoreUnpermitted = 4,
        IgnoreSymlinks = 8,
        IgnoreAllPointless = (IgnoreDigestFiles | IgnoreDbFiles | IgnoreUnpermitted | IgnoreSymlinks)
    };

    FilterRule(FilterAttribute attr = IgnoreAllPointless);
    FilterRule(const FilterMode filterMode, const QStringList &extensions);

    // returns m_mode
    FilterMode mode() const;

    // splits the str (e.g. "txt, pdf, mkv") and sets the list
    void setFilter(const FilterMode filterMode, const QString &extensions);
    void setFilter(const FilterMode filterMode, const QStringList &extensions);

    // sets defaults
    void clearFilter();

    // checks whether the specified mode is setted (m_mode == filterMode)
    bool isFilter(const FilterMode filterMode) const;

    // != NotSet
    bool isEnabled() const;

    // whether the file extension matches the filter rules
    bool isAllowed(const QString &filePath) const;

    // whether the file attributes (e.g. read permissions, symlink...)
    // as well as the file extension matches the filter rules
    bool isAllowed(const QFileInfo &fi) const;

    // creates a string listing the elements of a list (m_extensions)
    QString extensionString(const QString &sep = QStringLiteral(u", ")) const;

    // returns a ref to m_extensions
    const QStringList& extensionList() const;

    // -- Attributes --
    // replace current ones (m_attributes = attr)
    void setAttributes(quint8 attr);

    // add flag to m_attributes
    void addAttribute(FilterAttribute attr);

    // remove flag
    void removeAttribute(FilterAttribute attr);

    // checks if the flag is set
    bool hasAttribute(FilterAttribute attr) const;

    // whether the file (fi) attributes (e.g. symlink, read permissions...) are allowed
    bool passAttributes(const QFileInfo &fi) const;

    explicit operator bool() const { return isEnabled(); }

private:
    // current mode. if set, the extension list is assumed to be non-empty
    FilterMode m_mode = NotSet;

    // list of file extensions (suffixes) for filtering
    QStringList m_extensions;
    quint8 m_attributes = IgnoreAllPointless;
}; // class FilterRule

using FilterMode = FilterRule::FilterMode;
using FilterAttribute = FilterRule::FilterAttribute;

#endif // FILTERRULE_H
