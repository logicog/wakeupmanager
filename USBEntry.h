#ifndef USBENTRY_H
#define USBENTRY_H
 
#include <QString>
#include <QObject>
#include <QCheckBox>

class USBEntry : public QObject {

public:
    USBEntry(QString deviceNode);
    void        debug();
    bool        canWake();
    QCheckBox   *getCheckBox();
    QString     getUSBDevNumber();
    bool        isEnabled() {return enabled;};
    
private:
    QString     readUSBDeviceInfo(QString info);
    QString     product;
    QString     manufacturer;
    QString     sysfsNode;
    QString     devClass;
    QCheckBox   *checkBox;
    QString     devNumber;
    bool        enabled;
};

#endif
