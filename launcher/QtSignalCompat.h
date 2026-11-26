#pragma once

#include <QNetworkReply>
#include <QProcess>

// Resolve the legacy signal through Qt's meta-object by name. Some Qt 5.6
// builds fail to resolve the overloaded signal through its member pointer.
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#define MESHMC_CONNECT_NETWORK_ERROR(sender, receiver, receiverType, slot) \
    QObject::connect(sender, &QNetworkReply::errorOccurred, receiver, &receiverType::slot)
#else
#define MESHMC_CONNECT_NETWORK_ERROR(sender, receiver, receiverType, slot) \
    QObject::connect(sender, SIGNAL(error(QNetworkReply::NetworkError)), receiver, SLOT(slot(QNetworkReply::NetworkError)))
#endif

namespace QtCompat
{
// QProcess has provided errorOccurred since Qt 5.6.
constexpr auto ProcessError = &QProcess::errorOccurred;
}
