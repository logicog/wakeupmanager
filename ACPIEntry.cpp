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

#include "ACPIEntry.h"

#include <QDebug>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QVBoxLayout>
#include <KCModule>

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
        qDebug() << "ACPIEntry pci device class: " << devClass << " of type " << trivialName;
        if(devClass.startsWith("0x0c03")) {
            trivialName = "USB Hub";
            scanPCIUSBHub();
        }
    }
    if(devClass.startsWith("0x02")) {
        trivialName = "Network controller";  // Unspecified network controller
    }
    if(devClass.startsWith("0x0200")) {
        trivialName = "Ethernet controller";
    }
    if ( entry == "PS2K" )
        trivialName = "PS2 Keyboard";
    if ( entry == "PS2M" )
        trivialName = "PS2 Mouse";
    if ( entry.startsWith("LID") )
        trivialName = "Laptop Lid";
    if ( entry.startsWith("PWRB") )
        trivialName = "Power Button";
    if ( entry.startsWith("SLPB") )
        trivialName = "Sleep button";

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


int ACPIEntry::scanPCIUSBHub()
{ 
    QString node = QString("/sys/bus/pci/devices/");
    node.append(sysfsNode.mid(4));
    node.append("/");
    qDebug() << "ACPIEntry: readUSBEntries: " << node;
    
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
        readUSBBus(busDir, usbEntries);
    }
    return 0;
}


int ACPIEntry::readUSBBus(QString &busDir, QList<USBEntry *> &usbEntriesList)
{
    qDebug() << "readUSBBus: readUSBEntries: " << busDir;
    
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
        if(usbEntry->isHub()) {
            QString path = usbFileInfo.absoluteFilePath() + "/";
            qDebug() << "DETECTED HUB, reading sub entries of " << path;
            QList<USBEntry *> *subEntries = new QList<USBEntry *>;
            readUSBBus(path, *subEntries);
            usbEntry->setSubEntries(subEntries);
        }
        if(usbEntry->canWake())
            usbEntriesList.push_back(usbEntry);
        else
            delete usbEntry;
    }
    
    return 0;
}


QCheckBox *ACPIEntry::createCheckBox()
{
    if(checkBox)
        return checkBox;
    
    QString description("");
    description += trivialName + " (" + entry + ")";
    checkBox = new QCheckBox(description);
    checkBox -> setChecked(enabled);
    return checkBox;
}


QWidget *ACPIEntry::getUSBNodes(KCModule *parent=NULL)
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
        if(parent) {
            qDebug() << "Connecting to parent";
            connect(c, SIGNAL (stateChanged(int )), parent, SLOT(changed()));
        }
        w->layout()->addWidget(c);
        if(usbEntries.at(i)->isHub()) {
            QWidget *boxes = usbEntries.at(i)->getUSBNodes(parent);
            if(boxes) {
                w->layout()->addWidget(boxes);
            }
        }
    }
    w->layout()->setContentsMargins(20,9,9,9);
    return w;
}


void ACPIEntry::resetUSBEntries()
{
    for (int i=0; i < usbEntries.size(); i++ ) {
        qDebug() << "Resetting " << usbEntries.at(i)->getUSBDevNumber();
        
        usbEntries.at(i)->getCheckBox()->setChecked(usbEntries.at(i)->isEnabled());
    }
    
}



QStringList ACPIEntry::changedUSBEntries(bool enabled)
{
    qDebug() << "ACPIEntry::changedUSBEntries";
    QStringList e;
    
    for (int i=0; i < usbEntries.size(); i++ ) {
        
        if(usbEntries.at(i)->isHub()) {
            qDebug() << "ACPIEntry::changedUSBEntries found a hub";
            QStringList l = usbEntries.at(i)->changedUSBEntries(enabled);
            e += l;
        }
        
        if(usbEntries.at(i)->getCheckBox()->isChecked() == usbEntries.at(i)->isEnabled())
            continue;

        if(usbEntries.at(i)->getCheckBox()->isChecked() != enabled)
            continue;

        qDebug() << "Adding " << usbEntries.at(i)->getUSBDevNumber() << "as enabled: " << enabled;
        e.push_back(usbEntries.at(i)->getUSBDevNumber());
    }
    
    return e;
}


bool ACPIEntry::canWake()
{
    if(usbEntries.size())
        return true;
        
    if (trivialName != "" ) // Device class is known
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
                    << " " << sysfsNode << "Device class" << devClass << " Name: " << trivialName;
}

