#include "ACPIEntry.h"

#include <QDebug>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QVBoxLayout>

ACPIEntry::ACPIEntry(QString e, QString s, QString isEn, QString sys)
: checkBox(NULL)
{
    entry = e;
    sleepState = s.toLong();
    if( (sleepState <3) || (sleepState>4) )
        sleepState = 0;
    if(isEn == "enabled" ) {
        enabled = true;
    } else {
        enabled = false;
    }
    sysfsNode =sys;
    
    if( sysfsNode.startsWith("pci:") ) {
        readPCIDeviceClass();
        qDebug() << "ACPIEntry pci device class: " << devClass;
        if(devClass.startsWith("0x0c03")) {
            devClass = "USB Hub";
            readUSBEntries();
        }
    }
    if ( entry == "PS2K" )
        devClass = "PS2 Keyboard";
    if ( entry == "PS2M" )
        devClass = "PS2 Mouse";
    if ( entry.startsWith("LID") )
        devClass = "Laptop Lid";
}


ACPIEntry::~ACPIEntry()
{
    qDeleteAll(usbEntries.begin(), usbEntries.end());
}


int ACPIEntry::readPCIDeviceClass()
{
    QString node = QString("/sys/bus/pci/devices/");
    node.append(sysfsNode.mid(4));
    node.append("/class");
    QFile file(node);
    
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    
    if ( !file.isOpen() ){
        qWarning() << "Unable to open file: " << file.errorString();
        return -1;
    }
    
    qWarning() << "PCI device file open";
    QTextStream in(&file);
    devClass = in.readLine();
    file.close();
    
    return 0;
}


int ACPIEntry::readUSBEntries()
{
    qDebug() << "ACPIEntry: readUSBEntries";
    QString node = QString("/sys/bus/pci/devices/");
    node.append(sysfsNode.mid(4));
    node.append("/");
    
    QDir dir(node);
    QStringList filters;
    filters << "usb?";
    dir.setNameFilters(filters);
    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); i++) {
        QFileInfo fileInfo = list.at(i);
        QString busDir(node);
        busDir.append("/");
        busDir.append(fileInfo.fileName());
        qDebug() << "ACPIEntry: readUSBEntries " << busDir;
        
        // Now loop over devices on that USB busDir
        QDir usbBusDir(busDir);
        QStringList usbDevFilters;
        usbDevFilters << "?-*";
        usbBusDir.setNameFilters(usbDevFilters);
        QFileInfoList usbDevList = usbBusDir.entryInfoList();
        for (int j=0; j < usbDevList.size(); j++) {
            QFileInfo usbFileInfo = usbDevList.at(j);
            qDebug() << "Entry: " << usbFileInfo.absoluteFilePath();
            USBEntry *usbEntry = new USBEntry(usbFileInfo.absoluteFilePath());
            if(usbEntry->canWake())
                usbEntries.push_back(usbEntry);
            else
                delete usbEntry;
        }
        
    }
    return 0;
}


QCheckBox *ACPIEntry::createCheckBox()
{
    if(checkBox)
        return checkBox;
    
    QString description("");
    description += devClass + " (" + entry + ")";
    checkBox = new QCheckBox(description);
    checkBox -> setChecked(enabled);
    return checkBox;
}

QWidget *ACPIEntry::getUSBNodes()
{
    qDebug() << "gettingUSBNodes";
    
    if ( !usbEntries.size() )
        return NULL;
    
    QWidget *w = new QWidget();
    QVBoxLayout *l = new QVBoxLayout();
    w->setLayout(l);
    
    for (int i=0; i < usbEntries.size(); i++ ) {
        qDebug() << "adding usb entry";
        QCheckBox *c = usbEntries.at(i)->getCheckBox();
        w->layout()->addWidget(c);
    }
    w->layout()->setContentsMargins(20,9,9,9);
    return w;
}


QStringList ACPIEntry::changedUSBEntries(bool enabled)
{
    QStringList e;
    
    for (int i=0; i < usbEntries.size(); i++ ) {
        if(usbEntries.at(i)->getCheckBox()->isChecked() == usbEntries.at(i)->isEnabled())
            continue;

        if(usbEntries.at(i)->isEnabled() != enabled)
            continue;

        e.push_back(usbEntries.at(i)->getUSBDevNumber());
    }
    
    return e;
}


bool ACPIEntry::canWake()
{
    // XXXXXXXXXXXX Ethernet controllers, Lids, power button and so on
    
    if(usbEntries.size())
        return true;

    return false;
}

void ACPIEntry::handleStateChange(int state)
{
    qDebug() << "ACPIEntry: " << entry << ", State changed " << state;
//     for(int i=0; i<usbEntries.size(); i++) {
//         if(usbEntries.at(i)->getCheckBox()) {
//             usbEntries.at(i)->getCheckBox()->setEnabled(state);
//             qDebug() << "Enabling something..";
//         }
//     }
    if (usbEntries.size())
        usbEntries.at(0)->getCheckBox()->parentWidget()->setEnabled(state);
}


void ACPIEntry::debug()
{
    qDebug() << "ACPIEntry: " << entry << ", State " << sleepState << " " << enabled
                    << " " << sysfsNode << "Device class" << devClass;
}

