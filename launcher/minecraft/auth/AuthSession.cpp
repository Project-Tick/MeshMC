#include "AuthSession.h"
#include <QCryptographicHash>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStringList>

QString AuthSession::serializeUserProperties()
{
    QJsonObject userAttrs;
    /*
    for (auto key : u.properties.keys())
    {
        auto array = QJsonArray::fromStringList(u.properties.values(key));
        userAttrs.insert(key, array);
    }
    */
    QJsonDocument value(userAttrs);
    return value.toJson(QJsonDocument::Compact);

}

bool AuthSession::MakeOffline(QString offline_playername)
{
    if (status != PlayableOffline && status != PlayableOnline)
    {
        return false;
    }
    session = "-";
    player_name = offline_playername;
    status = PlayableOffline;
    return true;
}

void AuthSession::MakeDemo(const QString& name)
{
    auto playerId = QCryptographicHash::hash("OfflinePlayer:" + name.toUtf8(), QCryptographicHash::Md5);
    // Java's name-based UUID uses the version 3 and RFC 4122 variant bits.
    playerId[6] = char((playerId.at(6) & 0x0f) | 0x30);
    playerId[8] = char((playerId.at(8) & 0x3f) | 0x80);
    uuid = QString::fromLatin1(playerId.toHex());
    player_name = name;
    session = "-";
    access_token = "0";
    user_type = "legacy";
    wants_online = false;
    // Use the regular online asset update without claiming a Minecraft account.
    status = PlayableOnline;
    demo = true;
}
