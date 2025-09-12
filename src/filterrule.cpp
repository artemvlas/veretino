/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "filterrule.h"
#include "tools.h"
#include "pathstr.h"
#include <QRegularExpression>

FilterRule::FilterRule(FilterAttribute attr)
{
    setAttributes(attr);
}

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
    setAttributes(IgnoreAllPointless);
}

bool FilterRule::isFilter(const FilterMode filterMode) const
{
    return (filterMode == m_mode);
}

bool FilterRule::isEnabled() const
{
    return !isFilter(NotSet);
}

bool FilterRule::isAllowed(const QString &filePath) const
{
    if (!isFilter(Include)) {
        if (paths::isDbFile(filePath))
            return !hasAttribute(IgnoreDbFiles);
        if (paths::isDigestFile(filePath))
            return !hasAttribute(IgnoreDigestFiles);
    }

    if (isFilter(NotSet))
        return true;

    // if 'isFilter(Include)': a file ('filePath') with any extension from 'm_extensions' is allowed
    // if 'isFilter(Ignore)': than all files except these types are allowed

    return pathstr::hasExtension(filePath, m_extensions) ? isFilter(Include) : isFilter(Ignore);
}

bool FilterRule::isAllowed(const QFileInfo &fi) const
{
    return passAttributes(fi) && isAllowed(fi.fileName());
}

QString FilterRule::extensionString(const QString &sep) const
{
    return m_extensions.join(sep);
}

const QStringList& FilterRule::extensionList() const
{
    return m_extensions;
}

void FilterRule::setAttributes(quint8 attr)
{
    m_attributes = attr;
}

void FilterRule::addAttribute(FilterAttribute attr)
{
    m_attributes |= attr;
}

void FilterRule::removeAttribute(FilterAttribute attr)
{
    m_attributes &= ~attr;
}

bool FilterRule::hasAttribute(FilterAttribute attr) const
{
    return attr & m_attributes;
}

bool FilterRule::passAttributes(const QFileInfo &fi) const
{
    if (hasAttribute(IgnoreUnpermitted) && !fi.isReadable())
        return false;

    if (hasAttribute(IgnoreSymlinks) && fi.isSymLink())
        return false;

    return true;
}
