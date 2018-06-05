#include "WakeupManager.h"
// KF5
#include <QApplication>
#include <QAction>

#include <KLocalizedString>
#include <KActionCollection>
#include <KStandardAction>

#include <KAuth>
using namespace KAuth;

#include <QMessageBox>
#include <QCheckBox>
#include <QDebug>


WakeupManager::WakeupManager(QWidget *parent) : KXmlGuiWindow(parent)
{
    firstLevelBox = new QGroupBox(i18n("ACPI Devices"));
    firstLevelLayout = new QVBoxLayout();
        
    qDebug() << "Acting..";
    readACPI();
    
    resetButton = new QPushButton("Reset");
    firstLevelLayout->addWidget(resetButton);
    applyButton = new QPushButton("Apply");
    firstLevelLayout->addWidget(applyButton);
    
    firstLevelBox->setLayout(firstLevelLayout);
    setCentralWidget(firstLevelBox);
  
    setupActions();
}


WakeupManager::~WakeupManager()
{
    qDeleteAll(acpiEntries.begin(), acpiEntries.end());
}


void WakeupManager::readACPI()
{
    qWarning() << "Reading ACPI file";
    QFile file("/proc/acpi/wakeup");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    
    if ( !file.isOpen() ){
        QMessageBox::information(this, tr("Unable to open file"), file.errorString());
        return;
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
            ACPIEntry *acpiEntry = new ACPIEntry(rx.cap(1), rx.cap(2),rx.cap(3),rx.cap(4));
            acpiEntries.push_back(acpiEntry);
            acpiEntries.last()->debug();
            if (acpiEntry->canWake()) {
                QCheckBox *checkBox = acpiEntry->createCheckBox();
                connect(checkBox, SIGNAL (stateChanged(int )), acpiEntry, SLOT (handleStateChange(int )));
                firstLevelLayout->addWidget(checkBox);
                QWidget *w= acpiEntry->getUSBNodes();
                if(w) {
                    if( !acpiEntry->isEnabled()) w->setEnabled(false);
                    firstLevelLayout->addWidget(w);
                }
            }
        } else {
            qDebug() << "No match!"; 
        }
        
    } while (1);
    
    firstLevelLayout->addStretch(1);
    
    qWarning() << "Closing ACPI file";
    file.close();
}

void WakeupManager::setupActions()
{
      
    KStandardAction::quit(qApp, SLOT(quit()), actionCollection());
    connect(resetButton, SIGNAL (released()), this, SLOT (handleResetButton()));
    connect(applyButton, SIGNAL (released()), this, SLOT (handleApplyButton()));
    
    setupGUI(Default, "wakeupmanagerui.rc");
}


void WakeupManager::handleApplyButton()
{
    qDebug() << "Apply pressed!";
    
    // configPtr = KSharedConfig::openConfig("",KConfig::CascadeConfig, QStandardPaths::ConfigLocation);
    
    QVariantMap args;
    QString acpiEnabled;
    QString acpiDisabled;
    QString usbEnabled;
    QString usbDisabled;

    for (int i=0; i < acpiEntries.size(); i++) {
        
        // Entry is not configurable
        if (!acpiEntries.at(i)->getCheckBox())
            continue;
        
        QString s(acpiEntries.at(i)->changedUSBEntries(true));
        if(s.size()){
            if(usbEnabled.size())
                usbEnabled += ",";
            usbEnabled += s;
        }
        s=acpiEntries.at(i)->changedUSBEntries(false);
        if(s.size()){
            if(usbDisabled.size())
                usbDisabled += ",";
            usbDisabled += s;
        }
        
        // Entry did not change
        if(acpiEntries.at(i)->getCheckBox()->isChecked() == acpiEntries.at(i)->isEnabled())
            continue;
        
        if(acpiEntries.at(i)->getCheckBox()->isChecked() ) {
            if(acpiEnabled.size())
                acpiEnabled += ",";
            acpiEnabled += acpiEntries.at(i)->getName();
        } else {
            if(acpiDisabled.size())
                acpiDisabled += ",";
            acpiDisabled += acpiEntries.at(i)->getName();
        }
    }
    
    qDebug() << "ACPIEnabled: " << acpiEnabled;    
    qDebug() << "ACPIDisabled: " << acpiDisabled;
    qDebug() << "USBEnabled: " << usbEnabled;    
    qDebug() << "USBDisabled: " << usbDisabled;
    
    args["ACPIEnabled"] = acpiEnabled;
    args["ACPIDisabled"] = acpiDisabled;
    args["USBEnabled"] = usbEnabled;
    args["USBDisabled"] = usbDisabled;
    
    qDebug() << "Creating action!";
    KAuth::Action toggleACPIAction(QStringLiteral("org.kde.wakeupmanager.updateconfig"));
    toggleACPIAction.setHelperId("org.kde.wakeupmanager");
    toggleACPIAction.setArguments(args);
    qDebug() << "Executing job.";
    ExecuteJob *job = toggleACPIAction.execute();
    if (!job->exec()) {
       qDebug() << "KAuth returned an error code:" << job->error() << ": " << job->errorString();
    } else {
    //   QString contents = job->data()["contents"].toString();
       qDebug() << "Success";
    }
}

void WakeupManager::handleResetButton()
{
    qDebug() << "Reset pressed!";
    
    QVariantMap args;
    args["entry"] = "XHC0";
    
    qDebug() << "Creating action!";
    KAuth::Action toggleACPIAction(QStringLiteral("org.kde.wakeupmanager.toggleacpiwakeup"));
    toggleACPIAction.setHelperId("org.kde.wakeupmanager");
    toggleACPIAction.setArguments(args);
    qDebug() << "Executing job.";
    ExecuteJob *job = toggleACPIAction.execute();
    if (!job->exec()) {
       qDebug() << "KAuth returned an error code:" << job->error() << ": " << job->errorString();
    } else {
    //   QString contents = job->data()["contents"].toString();
       qDebug() << "Success";
    }
    
}

