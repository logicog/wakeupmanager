#ifndef USBENTRY_H
#define USBENTRY_H
 
#include <QString>
#include <QStringList>
#include <QObject>
#include <QCheckBox>

class KCModule;

class USBEntry : public QObject {

public:
    USBEntry(QString deviceNode);
    ~USBEntry();
    void        debug();
    bool        canWake();
    QCheckBox   *getCheckBox();
    QString     getUSBDevNumber();
    bool        isEnabled() {return enabled;};
    void        setSubEntries(QList<USBEntry *> *subEntries) { usbEntries = subEntries; };
    bool        isHub() { return devClass == "09"? true : false; };
    QWidget     *getUSBNodes(KCModule *parent);
    QStringList changedUSBEntries(bool enabled);
    QString     getUSBDevID() { return devID; }
private:
    QString     readUSBDeviceInfo(QString info);
    QString     product;
    QString     manufacturer;
    QString     sysfsNode;
    QString     devClass;
    QCheckBox   *checkBox;
    QString     devNumber;
    QString     devID;
    bool        enabled;
    QList<USBEntry *>   *usbEntries;
};

#endif
