#ifndef ACPIENTRY_H
#define ACPIENTRY_H
 
#include <QString>
#include <QStringList>
#include "USBEntry.h"

class KCModule;
class QCkeckBox;

class ACPIEntry : public QObject {
    Q_OBJECT
  public:
    ACPIEntry(QString e, QString s, QString isEn, QString sys="");
    ~ACPIEntry();
    
    void            debug();
    bool            canWake();
    QCheckBox       *getCheckBox() { return checkBox; };
    QCheckBox       *createCheckBox();
    QWidget         *getUSBNodes(KCModule *);
    bool            isEnabled() { return enabled; };
    QString         getName() { return entry; }; 
    QString         getNet() { return netDevice; };
    QStringList     changedUSBEntries(bool enabled);
    void            resetUSBEntries();

  private:
    int                 readPCIDeviceClass();
    int                 readPCINetDevice();
    int                 scanPCIUSBHub();
    int                 readUSBBus(QString &node, QList<USBEntry *> &entries);
    QString             entry;
    int                 sleepState;
    bool                enabled;
    QString             sysfsNode;
    QString             devClass;
    QString		trivialName;
    QString		netDevice;
    QList<USBEntry *>   usbEntries;
    QCheckBox           *checkBox;
    
  public Q_SLOTS:
    void handleStateChange(int state);
    
};

#endif
