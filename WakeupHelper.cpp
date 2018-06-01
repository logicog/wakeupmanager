#include <QFile>
#include <KAuth>
#include <KSharedConfig>
#include <KConfigGroup>
#include <QDebug>


using namespace KAuth;

class WakeupHelper : public QObject
{
    Q_OBJECT
    public:
        int configureDevices(QVariantMap &args);
    
    public Q_SLOTS:
        ActionReply updateconfig(const QVariantMap& args);
        ActionReply toggleacpiwakeup(const QVariantMap& args);
};


int WakeupHelper::configureDevices(QVariantMap &args)
{
//     QStringList acpiEnabled = configPtr;
//     QString acpiDisabled;
//     QString usbEnabled;
//     QString usbDisabled;
//     
    
    QStringList acpiEnabled(args["ACPIEnabled"]);
    QStringList acpiDisabled(args["ACPIDisabled"]);
    
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
    
    qDebug() << "Updating /proc/acpi/wakeup";
    
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    
    for (int i = 0; i < acpiEnabled.size(); ++i) {
        file.write(acpiEnabled.at(i).toLocal8Bit());
    }
    
    for (int i = 0; i < acpiDisabled.size(); ++i) {
        file.write(acpiDisabled.at(i).toLocal8Bit());
    }
    
    file.close();
    
    return 0;
}


ActionReply WakeupHelper::updateconfig(const QVariantMap &args)
{
    ActionReply reply;
    
    
    KSharedConfigPtr configPtr = KSharedConfig::openConfig("/tmp/wakeupmanager.rc",KConfig::SimpleConfig);
    
    
    KConfigGroup group(configPtr->group("Wakeup"));
    for(auto e : args.keys()) {
        qDebug() << e << "," << args.value(e) << '\n';
        group.writeEntry(e, args.value(e));
    }
    
    configPtr->sync();
    
    configureDevices(configPtr);
        
    reply.addData("result", "OK");

    return reply;
}


ActionReply WakeupHelper::toggleacpiwakeup(const QVariantMap& args)
{
    ActionReply reply;
    QString entry = args["entry"].toString();
    
    QFile file("/proc/acpi/wakeup");
    
    if (!file.open(QIODevice::WriteOnly)) {
       reply = ActionReply::HelperErrorReply();
       reply.setErrorDescription(file.errorString());
       return reply;
    }
    
    if(file.write(entry.toUtf8()) != entry.size()) {
       reply = ActionReply::HelperErrorReply();
       reply.setErrorDescription(file.errorString());
       return reply;
    }

    file.close();
    reply = ActionReply::SuccessReply();

    return reply;
}

KAUTH_HELPER_MAIN("org.kde.wakeupmanager", WakeupHelper)

#include "WakeupHelper.moc"
