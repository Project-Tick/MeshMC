/* Copyright 2026 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "ElidedLabel.h"

#include <QPainter>
#include <QStyle>

QSize ElidedLabel::minimumSizeHint() const
{
    // room for the "…" alone; the height stays a label's
    const int margins = 2 * (frameWidth() + margin());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return QSize(fontMetrics().horizontalAdvance(QStringLiteral("…")) + margins, QLabel::minimumSizeHint().height());
#else
    return QSize(fontMetrics().width(QStringLiteral("…")) + margins, QLabel::minimumSizeHint().height());
#endif
}

void ElidedLabel::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    // the frame, if the label has one
    drawFrame(&painter);

    // the text, cut to the width there is
    const QRect area = contentsRect().adjusted(margin(), margin(), -margin(), -margin());
    const QString shown = fontMetrics().elidedText(text(), Qt::ElideRight, area.width());
    style()->drawItemText(&painter, area, QStyle::visualAlignment(layoutDirection(), alignment()), palette(), isEnabled(),
                          shown, foregroundRole());

    // the full text where it was cut
    const QString tip = shown == text() ? QString() : text();
    if (toolTip() != tip)
    {
        setToolTip(tip);
    }
}
