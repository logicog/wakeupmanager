#include <QFile>
#include <KAuth>
#include <KSharedConfig>
#include <KConfigGroup>
#include <QDebug>

#define CONFIGFILE "/tmp/wakeupmanager.rc"

using namespace KAuth;

class WakeupHelper : public QObject
{
    Q_OBJECT
    public:
        int configureDevices(const QVariantMap &args);
    
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
    
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    
    for (int i = 0; i < acpiEnabled.size(); ++i) {
        if(file.write(acpiEnabled.at(i).toLocal8Bit()) != acpiEnabled.at(i).size()) {
            file.close();
            return 2;
        }
    }

    for (int i = 0; i < acpiDisabled.size(); ++i) {
        if(file.write(acpiDisabled.at(i).toLocal8Bit()) != acpiDisabled.at(i).size()) {
            file.close();
            return 2;
        }
    }
    
    file.close();
    
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


ActionReply WakeupHelper::updateconfig(const QVariantMap &args)
{
    ActionReply reply;
    
    
    KSharedConfigPtr configPtr = KSharedConfig::openConfig(CONFIGFILE,KConfig::SimpleConfig);
    
    
    KConfigGroup group(configPtr->group("Wakeup"));
    for(auto e : args.keys()) {
        qDebug() << e << "," << args.value(e) << '\n';
        group.writeEntry(e, args.value(e));
    }
    
    configPtr->sync();
    
    int ret = configureDevices(args);
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
    
 //   configPtr->close();
    
    return m;
}


ActionReply WakeupHelper::applyconfig(const QVariantMap& args)
{
    Q_UNUSED( args )

    ActionReply reply;
    
    QVariantMap config = readConfig();
    
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
