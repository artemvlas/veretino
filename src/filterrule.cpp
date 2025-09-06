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
    return mMode;
}

void FilterRule::setFilter(const FilterMode filterMode, const QString &extensions)
{
    static const QRegularExpression re("[, ]");
    setFilter(filterMode, extensions.split(re, Qt::SkipEmptyParts));
}

void FilterRule::setFilter(const FilterMode filterMode, const QStringList &extensions)
{
    mExtensions = extensions;
    mMode = extensions.isEmpty() ? NotSet : filterMode;
}

void FilterRule::clearFilter()
{
    mMode = NotSet;
    mExtensions.clear();
    setAttributes(IgnoreAllPointless);
}

bool FilterRule::isFilter(const FilterMode filterMode) const
{
    return (filterMode == mMode);
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

    // if 'isFilter(Include)': a file ('filePath') with any extension from 'mExtensions' is allowed
    // if 'isFilter(Ignore)': than all files except these types are allowed

    return pathstr::hasExtension(filePath, mExtensions) ? isFilter(Include) : isFilter(Ignore);
}

bool FilterRule::isAllowed(const QFileInfo &fi) const
{
    return hasAllowedAttributes(fi) && isAllowed(fi.fileName());
}

QString FilterRule::extensionString(const QString &sep) const
{
    return mExtensions.join(sep);
}

const QStringList& FilterRule::extensionList() const
{
    return mExtensions;
}

void FilterRule::setAttributes(quint8 attr)
{
    mAttributes = attr;
}

void FilterRule::addAttribute(FilterAttribute attr)
{
    mAttributes |= attr;
}

void FilterRule::removeAttribute(FilterAttribute attr)
{
    mAttributes &= ~attr;
}

bool FilterRule::hasAttribute(FilterAttribute attr) const
{
    return attr & mAttributes;
}

bool FilterRule::hasAllowedAttributes(const QFileInfo &fi) const
{
    return !((hasAttribute(IgnoreUnpermitted) && !fi.isReadable())
             || (hasAttribute(IgnoreSymlinks) && fi.isSymLink()));
}
