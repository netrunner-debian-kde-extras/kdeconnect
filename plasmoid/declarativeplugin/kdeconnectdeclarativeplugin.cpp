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

#include "kdeconnectdeclarativeplugin.h"

#include <QQmlEngine>
#include <QQmlContext>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QtQml>

#include "objectfactory.h"
#include "responsewaiter.h"
#include "processrunner.h"

#include "interfaces/devicessortproxymodel.h"
#include "interfaces/devicesmodel.h"
#include "interfaces/notificationsmodel.h"

QObject* createDeviceDbusInterface(QVariant deviceId)
{
    return new DeviceDbusInterface(deviceId.toString());
}

QObject* createDeviceBatteryDbusInterface(QVariant deviceId)
{
    return new DeviceBatteryDbusInterface(deviceId.toString());
}

QObject* createSftpInterface(QVariant deviceId)
{
    return new SftpDbusInterface(deviceId.toString());
}

QObject* createRemoteControlInterface(QVariant deviceId)
{
    return new RemoteControlDbusInterface(deviceId.toString());
}

QObject* createMprisInterface(QVariant deviceId)
{
    return new MprisDbusInterface(deviceId.toString());
}

QObject* createDeviceLockInterface(QVariant deviceId)
{
    Q_ASSERT(!deviceId.toString().isEmpty());
    return new LockDeviceDbusInterface(deviceId.toString());
}

QObject* createDBusResponse()
{
    return new DBusAsyncResponse();
}

void KdeConnectDeclarativePlugin::registerTypes(const char* uri)
{
    qmlRegisterType<DevicesModel>(uri, 1, 0, "DevicesModel");
    qmlRegisterType<NotificationsModel>(uri, 1, 0, "NotificationsModel");
    qmlRegisterType<DBusAsyncResponse>(uri, 1, 0, "DBusAsyncResponse");
    qmlRegisterType<ProcessRunner>(uri, 1, 0, "ProcessRunner");
    qmlRegisterType<DevicesSortProxyModel>(uri, 1, 0, "DevicesSortProxyModel");
    qmlRegisterUncreatableType<MprisDbusInterface>(uri, 1, 0, "MprisDbusInterface", QStringLiteral("You're not supposed to instantiate interfacess"));
    qmlRegisterUncreatableType<LockDeviceDbusInterface>(uri, 1, 0, "LockDeviceDbusInterface", QStringLiteral("You're not supposed to instantiate interfacess"));
    qmlRegisterUncreatableType<DeviceDbusInterface>(uri, 1, 0, "DeviceDbusInterface", QStringLiteral("You're not supposed to instantiate interfacess"));
}

void KdeConnectDeclarativePlugin::initializeEngine(QQmlEngine* engine, const char* uri)
{
    QQmlExtensionPlugin::initializeEngine(engine, uri);
 
    engine->rootContext()->setContextProperty("DeviceDbusInterfaceFactory"
      , new ObjectFactory(engine, createDeviceDbusInterface));
    
    engine->rootContext()->setContextProperty("DeviceBatteryDbusInterfaceFactory"
      , new ObjectFactory(engine, createDeviceBatteryDbusInterface));
    
    engine->rootContext()->setContextProperty("SftpDbusInterfaceFactory"
      , new ObjectFactory(engine, createSftpInterface));

    engine->rootContext()->setContextProperty("MprisDbusInterfaceFactory"
      , new ObjectFactory(engine, createMprisInterface));

    engine->rootContext()->setContextProperty("RemoteControlDbusInterfaceFactory"
      , new ObjectFactory(engine, createRemoteControlInterface));

    engine->rootContext()->setContextProperty("LockDeviceDbusInterfaceFactory"
      , new ObjectFactory(engine, createDeviceLockInterface));
    
    engine->rootContext()->setContextProperty("DBusResponseFactory"
      , new ObjectFactory(engine, createDBusResponse));    
    
    engine->rootContext()->setContextProperty("DBusResponseWaiter"
      , DBusResponseWaiter::instance());
}
