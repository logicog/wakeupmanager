#ifndef WAKEUPCONFIG_H
#define WAKEUPCONFIG_H
 
#include <QString>
#include <QVariantMap>

#define CONFIGFILE "/etc/xdg/wakeupmanager.knsrc"

class WakeupConfig : public QObject
{
    Q_OBJECT
    public:
        static int configureDevices(const QVariantMap &args);
        static int enableWOL(const QVariantMap &args);
        static void getUSBDeviceNodes(QVariantMap &config);
        static QString findUSBDevice(const QString &id);
        static QString readUSBDeviceInfo(QString &node);
        static QVariantMap readConfig();
};


#endif
