#include <QFile>
#include <KAuth>
#include <KSharedConfig>
#include <KConfigGroup>
#include <QDebug>


using namespace KAuth;

class WakeupHelper : public QObject
{
    Q_OBJECT
    public Q_SLOTS:
        ActionReply updateconfig(const QVariantMap& args);
        ActionReply toggleacpiwakeup(const QVariantMap& args);
};


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
