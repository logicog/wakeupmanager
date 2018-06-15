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

#include <QFile>
#include <KAuth>
#include <KSharedConfig>
#include <KConfigGroup>
#include <QDebug>
#include <QDir>

#define CONFIGFILE "/etc/xdg/wakeupmanager.knsrc"

using namespace KAuth;

class WakeupHelper : public QObject
{
    Q_OBJECT
    public:
        int configureDevices(const QVariantMap &args);
        void getUSBDeviceNodes(QVariantMap &config);
        QString findUSBDevice(const QString &id);
        QString readUSBDeviceInfo(QString &node);

    private:
        QVariantMap readConfig();
        
    public Q_SLOTS:
        ActionReply updateconfig(const QVariantMap& args);
        ActionReply applyconfig(const QVariantMap& args);
};


int WakeupHelper::configureDevices(const QVariantMap &args)
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

QString  WakeupHelper::readUSBDeviceInfo(QString &node)
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

QString WakeupHelper::findUSBDevice(const QString &id)
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
        qDebug() << "WakeupHelper::findUSBDevice " << idFileName;
        QString idProduct = readUSBDeviceInfo(idFileName);
        if(idProduct == "")
            continue;
        
        idFileName = node;
        idFileName.append(fileInfo.fileName());
        idFileName.append("/idVendor");
        qDebug() << "WakeupHelper::findUSBDevice " << idFileName;
        QString idVendor = readUSBDeviceInfo(idFileName);
        
        if(id == idVendor +":"+ idProduct) {
            node += fileInfo.fileName() + "/";
            qDebug() << "WakeupHelper::findUSBDevice Found: " << id << ", node: " << node;
            return node;
        }
    }
    qDebug() << "WakeupHelper::findUSBDevice Found: " << id << " NOT FOUND";
    return "";
}


void WakeupHelper::getUSBDeviceNodes(QVariantMap &config)
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


ActionReply WakeupHelper::updateconfig(const QVariantMap &args)
{
    // Need to make a modifiable copy, as we have to add the old configuration
    QVariantMap vm(args);
    
    ActionReply reply;
    
    // Read the previous configuration so that we can add unchanged entries
    QVariantMap old = readConfig();

    for(auto e: old.keys() ) {
        for(int i=0; i<old.value(e).toStringList().size(); i++)  {
            QString s=old.value(e).toStringList().at(i);
            qDebug() << "Searching for origial entry" << e << ": " << s;
            bool found = false;
            for(auto en: vm.keys()) {
                if(vm.value(en).toStringList().indexOf(s)>=0) {
                    found = true;
                    break;
                }
            }
            if(!found) {
                qDebug() << "NOT Found: "<< e << ": " << s;
                QStringList sl = vm.value(e).toStringList();
                sl.push_back(s);
                vm[e] = sl;
            } else {
                qDebug() << "Found: "<< e << ": " << s;
            }
        }
    }
    
    KSharedConfigPtr configPtr = KSharedConfig::openConfig(CONFIGFILE,KConfig::SimpleConfig);
    
    KConfigGroup group(configPtr->group("Wakeup"));
    for(auto e : vm.keys()) { 
        qDebug() << e << "," << vm.value(e) << '\n';
        group.writeXdgListEntry(e, vm.value(e).toStringList());
    }
     
    configPtr->sync();
    getUSBDeviceNodes(vm);
    int ret = configureDevices(vm);
    switch(ret) {
        case 1:
            reply = ActionReply::HelperErrorReply();
            reply.setErrorDescription("Could not open acpi settings file: /proc/acpi/wakeup");
            return reply;
        case 2:
            reply = ActionReply::HelperErrorReply();
            reply.setErrorDescription("Could not write to /proc/acpi/wakeup.");
            return reply;
        case 3:
            reply = ActionReply::HelperErrorReply();
            reply.setErrorDescription("Could not write to usb sysfs file.");
            return reply;
        case 4:
            reply = ActionReply::HelperErrorReply();
            reply.setErrorDescription("Could not open usb sysfs file");
            return reply;
    }
     
    reply.addData("result", "OK");

    return reply;
}


QVariantMap WakeupHelper::readConfig()
{
    KSharedConfigPtr configPtr = KSharedConfig::openConfig(CONFIGFILE,KConfig::SimpleConfig);
    
    KConfigGroup group(configPtr->group("Wakeup"));
 
    QStringList acpiEnabled = group.readXdgListEntry("ACPIEnabled");
    QStringList acpiDisabled = group.readXdgListEntry("ACPIDisabled");
    QStringList usbEnabled = group.readXdgListEntry("USBEnabled");
    QStringList usbDisabled = group.readXdgListEntry("USBDisabled");
    
    QVariantMap m;
    m["ACPIEnabled"] = acpiEnabled;
    m["ACPIDisabled"] = acpiDisabled;
    m["USBEnabled"] = usbEnabled;
    m["USBDisabled"] = usbDisabled;
    
    qDebug() << "Config read E: " << acpiEnabled;
    qDebug() << "Config read D: " << acpiDisabled;
    
 //   configPtr->close();
    
    return m;
}


ActionReply WakeupHelper::applyconfig(const QVariantMap& args)
{
    Q_UNUSED( args )

    ActionReply reply;
    
    QVariantMap config = readConfig();
    getUSBDeviceNodes(config);
    int ret = configureDevices(config);
    switch(ret) {
        case 1:
            reply = ActionReply::HelperErrorReply();
            reply.setErrorDescription("Could not open acpi settings file: /proc/acpi/wakeup");
            return reply;
        case 2:
            reply = ActionReply::HelperErrorReply();
            reply.setErrorDescription("Could not write to /proc/acpi/wakeup.");
            return reply;
        case 3:
            reply = ActionReply::HelperErrorReply();
            reply.setErrorDescription("Could not write to usb sysfs file.");
            return reply;
        case 4:
            reply = ActionReply::HelperErrorReply();
            reply.setErrorDescription("Could not open usb sysfs file");
            return reply;
    }
     
    reply.addData("result", "OK");

    return reply;
}

KAUTH_HELPER_MAIN("org.kde.wakeupmanager", WakeupHelper)

#include "WakeupHelper.moc"
