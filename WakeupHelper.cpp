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

#include <syslog.h>
#include "WakeupConfig.h"

using namespace KAuth;

class WakeupHelper : public QObject
{
    Q_OBJECT
    
    public Q_SLOTS:
        ActionReply updateconfig(const QVariantMap& args);
        ActionReply applyconfig(const QVariantMap& args);
};


ActionReply WakeupHelper::updateconfig(const QVariantMap &args)
{
    // Need to make a modifiable copy, as we have to add the old configuration
    QVariantMap vm(args);
    
    ActionReply reply;
    
    // Read the previous configuration so that we can add unchanged entries
    QVariantMap old = WakeupConfig::readConfig();

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
    WakeupConfig::getUSBDeviceNodes(vm);
    int ret = WakeupConfig::configureDevices(vm);
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


ActionReply WakeupHelper::applyconfig(const QVariantMap& args)
{
    Q_UNUSED( args )
    
    // Log to syslog
    syslog(LOG_INFO, "kcm_wakeup applyconfig called");
    
    ActionReply reply;
    
    QVariantMap config = WakeupConfig::readConfig();
    WakeupConfig::getUSBDeviceNodes(config);
    int ret = WakeupConfig::configureDevices(config);
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
