/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mpriscontrolplugin.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <qdbusconnectioninterface.h>
#include <QDBusReply>
#include <QDBusMessage>
#include <QDBusServiceWatcher>

#include <KPluginFactory>

#include <core/device.h>
#include "mprisdbusinterface.h"
#include "propertiesdbusinterface.h"

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_mpriscontrol.json", registerPlugin< MprisControlPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_MPRIS, "kdeconnect.plugin.mpris")

MprisControlPlugin::MprisControlPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , prevVolume(-1)
{
    m_watcher = new QDBusServiceWatcher(QString(), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);

    connect(m_watcher, &QDBusServiceWatcher::serviceRegistered, this, &MprisControlPlugin::addService);
    connect(m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, &MprisControlPlugin::removeService);

    //Add existing interfaces
    QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
    Q_FOREACH (const QString& service, services) {
        addService(service);
    }
}

void MprisControlPlugin::addService(const QString& service)
{
    if (service.startsWith(QLatin1String("org.mpris.MediaPlayer2"))) {
        addPlayer(service);
    }
}

void MprisControlPlugin::removeService(const QString& service)
{
    if (service.startsWith(QLatin1String("org.mpris.MediaPlayer2"))) {
        removePlayer(service);
    }
}


void MprisControlPlugin::addPlayer(const QString& service)
{
    QDBusInterface mprisInterface(service, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2");
    //FIXME: This call hangs and returns an empty string if KDED is still starting!
    QString identity = mprisInterface.property("Identity").toString();
    if (identity.isEmpty()) {
        identity = service.mid(sizeof("org.mpris.MediaPlayer2"));
    }
    playerList[identity] = service;
    qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Mpris addPlayer" << service << "->" << identity;
    sendPlayerList();

    OrgFreedesktopDBusPropertiesInterface* freedesktopInterface = new OrgFreedesktopDBusPropertiesInterface(service, "/org/mpris/MediaPlayer2", QDBusConnection::sessionBus(), this);
    connect(freedesktopInterface, SIGNAL(PropertiesChanged(QString,QVariantMap,QStringList)), this, SLOT(propertiesChanged(QString,QVariantMap)));

    OrgMprisMediaPlayer2PlayerInterface* mprisInterface0  = new OrgMprisMediaPlayer2PlayerInterface(service, "/org/mpris/MediaPlayer2", QDBusConnection::sessionBus());
    connect(mprisInterface0, SIGNAL(Seeked(qlonglong)), this, SLOT(seeked(qlonglong)));
}

void MprisControlPlugin::seeked(qlonglong position){
    //qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Seeked in player";
    NetworkPackage np(PACKAGE_TYPE_MPRIS);
    np.set("pos", position/1000); //Send milis instead of nanos
    OrgFreedesktopDBusPropertiesInterface* interface = (OrgFreedesktopDBusPropertiesInterface*)sender();
    const QString& service = interface->service();
    const QString& player = playerList.key(service);
    np.set("player", player);
    sendPackage(np);
}

void MprisControlPlugin::propertiesChanged(const QString& propertyInterface, const QVariantMap& properties)
{
    Q_UNUSED(propertyInterface);

    NetworkPackage np(PACKAGE_TYPE_MPRIS);
    bool somethingToSend = false;
    if (properties.contains("Volume")) {
        int volume = (int) (properties["Volume"].toDouble()*100);
        if (volume != prevVolume) {
            np.set("volume",volume);
            prevVolume = volume;
            somethingToSend = true;
        }
    }
    if (properties.contains("Metadata")) {
        QDBusArgument bullshit = qvariant_cast<QDBusArgument>(properties["Metadata"]);
        QVariantMap nowPlayingMap;
        bullshit >> nowPlayingMap;
        if (nowPlayingMap.contains("xesam:title")) {
            QString nowPlaying = nowPlayingMap["xesam:title"].toString();
            if (nowPlayingMap.contains("xesam:artist")) {
                nowPlaying = nowPlayingMap["xesam:artist"].toString() + " - " + nowPlaying;
            }
            np.set("nowPlaying",nowPlaying);
            somethingToSend = true;
        }
        if (nowPlayingMap.contains("mpris:length")) {
            if (nowPlayingMap.contains("mpris:length")) {
                long long length = nowPlayingMap["mpris:length"].toLongLong();
                np.set("length",length/1000); //milis to nanos
            }
            somethingToSend = true;
        }

    }
    if (properties.contains("PlaybackStatus")) {
        bool playing = (properties["PlaybackStatus"].toString() == "Playing");
        np.set("isPlaying", playing);
        somethingToSend = true;
    }

    if (somethingToSend) {
        OrgFreedesktopDBusPropertiesInterface* interface = (OrgFreedesktopDBusPropertiesInterface*)sender();
        const QString& service = interface->service();
        const QString& player = playerList.key(service);
        np.set("player", player);
        // Always also update the position
        OrgMprisMediaPlayer2PlayerInterface mprisInterface(service, "/org/mpris/MediaPlayer2", QDBusConnection::sessionBus());
        if (mprisInterface.canSeek()) {
            long long pos = mprisInterface.position();
            np.set("pos", pos/1000); //Send milis instead of nanos
        }
        sendPackage(np);
    }
}

void MprisControlPlugin::removePlayer(const QString& ifaceName)
{
    const QString identity = playerList.key(ifaceName);
    qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Mpris removePlayer" << ifaceName << "->" << identity;
    playerList.remove(identity);
    sendPlayerList();
}

bool MprisControlPlugin::receivePackage (const NetworkPackage& np)
{
    if (np.has("playerList")) {
        return false; //Whoever sent this is an mpris client and not an mpris control!
    }

    //Send the player list
    const QString player = np.get<QString>("player");
    bool valid_player = playerList.contains(player);
    if (!valid_player || np.get<bool>("requestPlayerList")) {
        sendPlayerList();
        if (!valid_player) {
            return true;
        }
    }

    //Do something to the mpris interface
    OrgMprisMediaPlayer2PlayerInterface mprisInterface(playerList[player], "/org/mpris/MediaPlayer2", QDBusConnection::sessionBus());
    mprisInterface.setTimeout(500);
    if (np.has("action")) {
        const QString& action = np.get<QString>("action");
        //qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Calling action" << action << "in" << playerList[player];
        //TODO: Check for valid actions, currently we trust anything the other end sends us
        mprisInterface.call(action);
    }
    if (np.has("setVolume")) {
        double volume = np.get<int>("setVolume")/100.f;
        qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Setting volume" << volume << "to" << playerList[player];
        mprisInterface.setVolume(volume);
    }
    if (np.has("Seek")) {
        int offset = np.get<int>("Seek");
        //qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Seeking" << offset << "to" << playerList[player];
        mprisInterface.Seek(offset);
    }

    if (np.has("SetPosition")){
        qlonglong position = np.get<qlonglong>("SetPosition",0)*1000;
        qlonglong seek = position - mprisInterface.position();
        //qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Setting position by seeking" << seek << "to" << playerList[player];
        mprisInterface.Seek(seek);
    }

    //Send something read from the mpris interface
    NetworkPackage answer(PACKAGE_TYPE_MPRIS);
    bool somethingToSend = false;
    if (np.get<bool>("requestNowPlaying")) {

        QVariantMap nowPlayingMap = mprisInterface.metadata();
        QString nowPlaying = nowPlayingMap["xesam:title"].toString();
        if (nowPlayingMap.contains("xesam:artist")) {
            nowPlaying = nowPlayingMap["xesam:artist"].toString() + " - " + nowPlaying;
        }
        answer.set("nowPlaying",nowPlaying);

        if (nowPlayingMap.contains("mpris:length")) {
            qlonglong length = nowPlayingMap["mpris:length"].toLongLong();
            answer.set("length",length/1000);
        }
        qlonglong pos = mprisInterface.position();
        answer.set("pos", pos/1000);

        bool playing = (mprisInterface.playbackStatus() == QLatin1String("Playing"));
        answer.set("isPlaying", playing);

        somethingToSend = true;
    }
    if (np.get<bool>("requestVolume")) {
        int volume = (int)(mprisInterface.volume() * 100);
        answer.set("volume",volume);
        somethingToSend = true;
    }
    if (somethingToSend) {
        answer.set("player", player);
        sendPackage(answer);
    }

    return true;
}

void MprisControlPlugin::sendPlayerList()
{
    NetworkPackage np(PACKAGE_TYPE_MPRIS);
    np.set("playerList",playerList.keys());
    sendPackage(np);
}

#include "mpriscontrolplugin.moc"
