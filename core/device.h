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

#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QSet>
#include <QSslKey>
#include <QTimer>
#include <QtCrypto>

#include "networkpackage.h"

class DeviceLink;
class KdeConnectPlugin;

class KDECONNECTCORE_EXPORT Device
    : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device")
    Q_PROPERTY(QString type READ type CONSTANT)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString iconName READ iconName CONSTANT)
    Q_PROPERTY(QString statusIconName READ statusIconName)
    Q_PROPERTY(bool isReachable READ isReachable NOTIFY reachableStatusChanged)
    Q_PROPERTY(bool isPaired READ isPaired NOTIFY pairingChanged)

    enum PairStatus {
        NotPaired,
        Requested,
        RequestedByPeer,
        Paired,
    };

    enum DeviceType {
        Unknown,
        Desktop,
        Laptop,
        Phone,
        Tablet,
    };
    static DeviceType str2type(QString deviceType);
    static QString type2str(DeviceType deviceType);

public:
    /**
     * Restores the @p device from the saved configuration
     *
     * We already know it but we need to wait for an incoming DeviceLink to communicate
     */
    Device(QObject* parent, const QString& id);

    /**
     * Device known via an incoming connection sent to us via a devicelink.
     *
     * We know everything but we don't trust it yet
     */
    Device(QObject* parent, const NetworkPackage& np, DeviceLink* dl);

    virtual ~Device();

    QString id() const { return m_deviceId; }
    QString name() const { return m_deviceName; }
    QString dbusPath() const { return "/modules/kdeconnect/devices/"+id(); }
    QString type() const { return type2str(m_deviceType); };
    QString iconName() const;
    QString statusIconName() const;

    //Add and remove links
    void addLink(const NetworkPackage& identityPackage, DeviceLink*);
    void removeLink(DeviceLink*);

    Q_SCRIPTABLE bool isPaired() const { return m_pairStatus==Device::Paired; }
    Q_SCRIPTABLE bool pairRequested() const { return m_pairStatus==Device::Requested; }

    Q_SCRIPTABLE QStringList availableLinks() const;
    bool isReachable() const { return !m_deviceLinks.isEmpty(); }

    Q_SCRIPTABLE QStringList loadedPlugins() const;
    Q_SCRIPTABLE bool hasPlugin(const QString& name) const;

    Q_SCRIPTABLE QString pluginsConfigFile() const;

public Q_SLOTS:
    ///sends a @p np package to the device
    virtual bool sendPackage(NetworkPackage& np);

    //Dbus operations
public Q_SLOTS:
    Q_SCRIPTABLE void requestPair();
    Q_SCRIPTABLE void unpair();
    Q_SCRIPTABLE void reloadPlugins(); //From kconf
    void acceptPairing();
    void rejectPairing();

private Q_SLOTS:
    void privateReceivedPackage(const NetworkPackage& np);
    void linkDestroyed(QObject* o);
    void pairingTimeout();

Q_SIGNALS:
    Q_SCRIPTABLE void pluginsChanged();
    Q_SCRIPTABLE void reachableStatusChanged();
    Q_SCRIPTABLE void pairingChanged(bool paired);
    Q_SCRIPTABLE void pairingFailed(const QString& error);
    Q_SCRIPTABLE void nameChanged(const QString& name);

    QT_DEPRECATED Q_SCRIPTABLE void pairingSuccesful();
    QT_DEPRECATED Q_SCRIPTABLE void unpaired();

private: //Methods
    void setName(const QString &name);
    QString iconForStatus(bool reachable, bool paired) const;
    void unpairInternal();
    void setAsPaired();
    bool sendOwnPublicKey();

private: //Fields (TODO: dPointer!)
    const QString m_deviceId;
    QString m_deviceName;
    DeviceType m_deviceType;
    QCA::PublicKey m_publicKey;
    PairStatus m_pairStatus;
    int m_protocolVersion;

    QVector<DeviceLink*> m_deviceLinks;
    QHash<QString, KdeConnectPlugin*> m_plugins;
    QMultiMap<QString, KdeConnectPlugin*> m_pluginsByIncomingInterface;
    QMultiMap<QString, KdeConnectPlugin*> m_pluginsByOutgoingInterface;

    QTimer m_pairingTimeut;
    const QSet<QString> m_incomingCapabilities;
    const QSet<QString> m_outgoingCapabilities;

};

Q_DECLARE_METATYPE(Device*)

#endif // DEVICE_H
