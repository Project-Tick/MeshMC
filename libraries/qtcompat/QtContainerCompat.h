#pragma once

#include <QString>
#include <QSet>

namespace QtCompat
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
constexpr auto SkipEmptyParts = Qt::SkipEmptyParts;
#else
constexpr auto SkipEmptyParts = QString::SkipEmptyParts;
#endif

template<class T>
QSet<T> toSet(const QList<T> &items)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return QSet<T>(items.cbegin(), items.cend());
#else
    return QSet<T>::fromList(items);
#endif
}
}
