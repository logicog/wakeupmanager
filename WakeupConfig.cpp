/*
    Copyright 2018 by Birger Koblitz
 
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
   
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
   
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "WakeupConfig.h"

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <KSharedConfig>
#include <KConfigGroup>

#include <syslog.h>

QVariantMap WakeupConfig::readConfig()
{
    // If there is no configuration file, then this method will simply return empty
    // configuration data
    KSharedConfigPtr configPtr = KSharedConfig::openConfig(CONFIGFILE,KConfig::SimpleConfig);
    
    KConfigGroup group(configPtr->group("Wakeup"));
 
    QStringList acpiEnabled = group.readXdgListEntry("ACPIEnabled");
    QStringList acpiDisabled = group.readXdgListEntry("ACPIDisabled");
    QStringList usbEnabled = group.readXdgListEntry("USBEnabled");
    QStringList usbDisabled = group.readXdgListEntry("USBDisabled");
    QStringList wolEnabled = group.readXdgListEntry("WOLEnabled");
    
    QVariantMap m;
    m["ACPIEnabled"] = acpiEnabled;
    m["ACPIDisabled"] = acpiDisabled;
    m["USBEnabled"] = usbEnabled;
    m["USBDisabled"] = usbDisabled;
    m["WOLEnabled"] = wolEnabled;
    
    qDebug() << "Config read ACPI Enabled: " << acpiEnabled;
    qDebug() << "Config read ACPI Disabled: " << acpiDisabled;
    qDebug() << "Config read WOL Enabled: " << wolEnabled;
    
 //   configPtr->close();
    
    return m;
}


int WakeupConfig::enableWOL(const QVariantMap &args)
{
    QStringList wolEnabled(args["WOLEnabled"].toStringList());

    for (int i = 0; i < wolEnabled.size(); ++i) {
        qDebug() << "WOL Enabling " << wolEnabled.at(i);
	QString ethtool = "/usr/sbin/ethtool";
	QStringList args;
	args << "-s" << wolEnabled.at(i) << "wol" << "g";
	QProcess::execute(ethtool, args);
    }

    return 0;
}


int WakeupConfig::configureDevices(const QVariantMap &args)
{
    QStringList acpiEnabled(args["ACPIEnabled"].toStringList());
    QStringList acpiDisabled(args["ACPIDisabled"].toStringList());
    
    QStringList usbEnabled(args["USBEnabled"].toStringList());
    QStringList usbDisabled(args["USBDisabled"].toStringList());
    
    // First we read the acpi wakeup configuration under proc
    // to see which settings are already set, in order to only toggle the others
    
    qWarning() << "Reading ACPI file";
    QFile file("/proc/acpi/wakeup");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    
    if ( !file.isOpen() ){
        return 1;
    }
    
    qWarning() << "ACPI file open";
    QTextStream in(&file);
    do {
        QString line = in.readLine();
        if (line.isNull() )
            break;
        qDebug() << line;
        QRegExp rx("(\\w+)\\s+S(\\d)\\s+\\*(\\w+)\\s*([\\.\\:\\w]+)*");
        int pos;
        if ((pos = rx.indexIn(line, 0)) != -1) {
            qDebug() << "Entry: " << rx.cap(1) << ", State " << rx.cap(2) << " " << rx.cap(3)
                    << " " << rx.cap(4);
            if(acpiEnabled.contains(rx.cap(1)) && rx.cap(3) == "enabled")
                acpiEnabled.removeAll(rx.cap(1));
            if(acpiDisabled.contains(rx.cap(1)) && rx.cap(3) == "disabled")
                acpiDisabled.removeAll(rx.cap(1));
        } else {
            qDebug() << "No match!"; 
        }
        
    } while (1);
    
    file.close();
    
    // Now we open the /proc/acpi/wakeup file again for writing only those entries that need
    // to be toggled
    
    qDebug() << "Updating /proc/acpi/wakeup";

    for (int i = 0; i < acpiEnabled.size(); ++i) {
        qDebug() << "Enabling " << acpiEnabled.at(i);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        if(file.write(acpiEnabled.at(i).toLocal8Bit()) != acpiEnabled.at(i).size()) {
            file.close();
            return 2;
        }
        file.close();
    }

    for (int i = 0; i < acpiDisabled.size(); ++i) {
        qDebug() << "Disabling " << acpiDisabled.at(i);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        if(file.write(acpiDisabled.at(i).toLocal8Bit()) != acpiDisabled.at(i).size()) {
            file.close();
            return 2;
        }
        file.close();
    }


    // Now we set the entries for the USB devices
       
    for (int i = 0; i < usbEnabled.size(); ++i) {
        qDebug() << "Opening file " << usbEnabled.at(i).toLocal8Bit() << "/power/wakeup";
        file.setFileName(usbEnabled.at(i) + "/power/wakeup");
        if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Enabling";
            syslog(LOG_INFO, "Enablign USB Wakeup: %s", usbEnabled.at(i).toLocal8Bit().data());
            if(file.write("enabled") != 7) {
                file.close();
                return 3;
            }
            file.close();
        } else {
            return 4;
        }
    }
    
    for (int i = 0; i < usbDisabled.size(); ++i) {
        qDebug() << "Opening file " << usbDisabled.at(i) << "/power/wakeup";
        file.setFileName(usbDisabled.at(i).toLocal8Bit() + "/power/wakeup");
        if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Disabling";
            syslog(LOG_INFO, "Disablign USB Wakeup: %s", usbDisabled.at(i).toLocal8Bit().data());
            if(file.write("disabled") != 8) {
                file.close();
                return 3;
            }
            file.close();
        } else {
            return 4;
        }
    }
    
    return 0;
}

QString  WakeupConfig::readUSBDeviceInfo(QString &node)
{
    QFile file(node);
    
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    
    if ( !file.isOpen() ){
        qWarning() << "Unable to open file: " << node << file.errorString();
        return "";
    }
    
    qWarning() << "USB device file open";
    QTextStream in(&file);
    QString r = in.readLine();
    file.close();
    
    return r;
}

QString WakeupConfig::findUSBDevice(const QString &id)
{
    QString node = QString("/sys/bus/usb/devices/");
    qDebug() << "ACPIEntry: findUSBDevice: " << node << "  Searching" << id;
    
    QDir dir(node);
    QStringList filters;
    filters << "*-*";
    dir.setNameFilters(filters);
    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); i++) {
        QFileInfo fileInfo = list.at(i);
        QString idFileName(node);
        idFileName.append(fileInfo.fileName());
        idFileName.append("/idProduct");
        qDebug() << "WakeupConfig::findUSBDevice " << idFileName;
        QString idProduct = readUSBDeviceInfo(idFileName);
        if(idProduct == "")
            continue;
        
        idFileName = node;
        idFileName.append(fileInfo.fileName());
        idFileName.append("/idVendor");
        qDebug() << "WakeupConfig::findUSBDevice " << idFileName;
        QString idVendor = readUSBDeviceInfo(idFileName);
        
        if(id == idVendor +":"+ idProduct) {
            node += fileInfo.fileName() + "/";
            qDebug() << "WakeupConfig::findUSBDevice Found: " << id << ", node: " << node;
            syslog(LOG_INFO, "USB Device found: %s is %s ", id.toLocal8Bit().data(), node.toLocal8Bit().data());
            return node;
        }
    }
    syslog(LOG_INFO, "Not found: %s", id.toLocal8Bit().data());
    qDebug() << "WakeupConfig::findUSBDevice Found: " << id << " NOT FOUND";
    return "";
}


void WakeupConfig::getUSBDeviceNodes(QVariantMap &config)
{
    
    qDebug() << "getUSBDeviceNodes";
    QStringList usbEnabledIDs(config["USBEnabled"].toStringList());
    QStringList usbDisabledIDs(config["USBDisabled"].toStringList());
    
    QStringList usbEnabledNodes;
    QStringList usbDisabledNodes;
    
    for (int i = 0; i < usbEnabledIDs.size(); ++i) {
        qDebug() << "getUSBDeviceNodes, enabled: " << usbEnabledIDs.at(i);
        QString node = findUSBDevice(usbEnabledIDs.at(i));
        if(node!= "")
            usbEnabledNodes.push_back(node);
    }
     
    for (int i = 0; i < usbDisabledIDs.size(); ++i) {
        qDebug() << "getUSBDeviceNodes, disabled: " << usbDisabledIDs.at(i);
        QString node = findUSBDevice(usbDisabledIDs.at(i));
        if(node!= "")
            usbDisabledNodes.push_back(node);
    }
    
    // TODO
    // config.erase("USBEnabled");
    // config.erase("USBDisabled");
    
    config["USBEnabled"] = usbEnabledNodes;
    config["USBDisabled"] = usbDisabledNodes;
}

