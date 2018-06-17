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

#include "USBEntry.h"

#include <QDebug>
#include <QTextStream>
#include <QFile>

#include <QVBoxLayout>
#include <QCheckBox>
#include <KCModule>

USBEntry::USBEntry(QString sys)
    : checkBox(NULL), usbEntries(NULL)
{
    sysfsNode = sys;
    devClass = readUSBDeviceInfo("bDeviceClass");
    product = readUSBDeviceInfo("product");
    manufacturer = readUSBDeviceInfo("manufacturer");
    devID = readUSBDeviceInfo("idVendor");
    devID += ":" + readUSBDeviceInfo("idProduct");
    QString e = readUSBDeviceInfo("power/wakeup");
    if(e == "enabled")
        enabled = true;
    else
        enabled = false;
    debug();
}

USBEntry::~USBEntry()
{
    if(usbEntries) {
        qDeleteAll(usbEntries->begin(), usbEntries->end());
        delete usbEntries;
    }
}


QString USBEntry::readUSBDeviceInfo(QString info)
{
    QString node = sysfsNode;
    node.append("/");
    node.append(info);
    
    QFile file(node);
    
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    
    if ( !file.isOpen() ){
        qWarning() << "Unable to open file: " << node << file.errorString();
        return "";
    }
    
    qWarning() << "USB device file open";
    QTextStream in(&file);
    QString r = in.readLine();
    file.close();
    
    return r;
}


bool USBEntry::canWake()
{
    if(devClass == "e0") // Wireless controller, bDeviceProtocol=01 bDeviceSubClass = 01:Bluetooth
        return true;
    
    if(devClass == "00") // Proprietary
        return true;
    
    if(devClass == "03") // HID (Mouse, Keyboard)
        return true;
    
    if(devClass == "09") // USB Hub
        return true;
    
    if(devClass == "12") // USB-C Bridge
        return true;
    
    return false;
}


QCheckBox *USBEntry::getCheckBox()
{
    if(checkBox)
        return checkBox;
    
    QString description("");
    
    if(manufacturer.size())
        description += manufacturer + ": ";
    
    if(devClass == "e0")
        description += "Bluetooth receiver";
    
    if(devClass == "03")
        description += "HID device";
    
    if(devClass == "09")
        description += "USB Hub";
    
    if(devClass == "12")
        description += "USB-C Bridge";
    
    if(devClass == "00")
        description += product;
    
    description += " (" + devID + ")";
    
    checkBox = new QCheckBox(description);
    checkBox -> setChecked(enabled);
    return checkBox;
}


// Returns Checkboxes for USB sub-devices if a USB device is a USB hub
QWidget *USBEntry::getUSBNodes(KCModule *parent=NULL)
{
    qDebug() << "USBEntry::gettingUSBNodes";
    
    if ( !usbEntries->size() )
        return NULL;
    
    QWidget *w = new QWidget();
    QVBoxLayout *l = new QVBoxLayout();
    w->setLayout(l);
    
    for (int i=0; i < usbEntries->size(); i++ ) {
        qDebug() << "adding usb entry";
         QCheckBox *c = usbEntries->at(i)->getCheckBox();
        if(parent) {
            qDebug() << "Connecting to parent";
            connect(c, SIGNAL (stateChanged(int )), parent, SLOT(changed()));
        }
        w->layout()->addWidget(c);
    }
    w->layout()->setContentsMargins(20,9,9,9);
    return w;
}


// Called only if a USB device is a USB hub
QStringList USBEntry::changedUSBEntries(bool enabled)
{
    QStringList e;
 
    qDebug() << "USBEntry::changedUSBEntries";
    if(!usbEntries)
        return e;
    
    qDebug() << "USBEntry::changedUSBEntries found some";
    
    for (int i=0; i < usbEntries->size(); i++ ) {
        if(usbEntries->at(i)->getCheckBox()->isChecked() == usbEntries->at(i)->isEnabled())
            continue;

        if(usbEntries->at(i)->getCheckBox()->isChecked() != enabled)
            continue;

        qDebug() << "Adding " << usbEntries->at(i)->getUSBDevNumber() << "as enabled: " << enabled;
        e.push_back(usbEntries->at(i)->getUSBDevNumber());
    }
    
    return e;
}


QString USBEntry::getUSBDevNumber()
{
    return sysfsNode;
}


void USBEntry::debug()
{
    qDebug() << "USBEntry: " << sysfsNode << "Device class" << devClass << ", " << product << ", "
    << manufacturer;
}
