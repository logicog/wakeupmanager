#include "USBEntry.h"

#include <QDebug>
#include <QTextStream>
#include <QFile>



USBEntry::USBEntry(QString sys)
    : checkBox(NULL)
{
    sysfsNode = sys;
    devClass = readUSBDeviceInfo("bDeviceClass");
    product = readUSBDeviceInfo("product");
    manufacturer = readUSBDeviceInfo("manufacturer");
    QString e = readUSBDeviceInfo("power/wakeup");
    if(e == "enabled")
        enabled = true;
    else
        enabled = false;
    debug();
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
    
    if(devClass == "00")
        description += product;
       
    checkBox = new QCheckBox(description);
    checkBox -> setChecked(enabled);
    return checkBox;
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
