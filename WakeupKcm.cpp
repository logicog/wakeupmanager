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


#include "WakeupKcm.h"
// KF5
#include <QAction>

#include <KLocalizedString>

#include <KAuth>
using namespace KAuth;

#include <QMessageBox>
#include <QCheckBox>
#include <QDebug>
#include <QFile>

#include <KSharedConfig>
#include <KPluginFactory>
#include <KAboutData>

K_PLUGIN_FACTORY(WakeupKcmFactory, registerPlugin<WakeupKcm>();)
K_EXPORT_PLUGIN(WakeupKcmFactory("kcm_wakeup" /* kcm name */, "kcm_wakeup" /* catalog name */))
    
WakeupKcm::WakeupKcm(QWidget *parent, const QVariantList &args) 
    : KCModule(parent, args)
{
    
    
    KAboutData* aboutData = new KAboutData("kcmwakeup", i18n("Wakeup KDE Config"), PROJECT_VERSION);

    aboutData->setShortDescription(i18n("Configure Wakeup from Sleep/Hibernate"));
    aboutData->setLicense(KAboutLicense::GPL_V2);
//    aboutData->setHomepage("https://projects.kde.org/projects/whatever");

    aboutData->addAuthor("Birger Koblitz", i18n("Author"));
    setAboutData(aboutData);
    
    
    firstLevelBox = new QGroupBox(i18n("ACPI Devices"), this);
    firstLevelLayout = new QVBoxLayout();

    qDebug() << "Acting..";
    readACPI();
    
    testButton = new QPushButton("Test");
    firstLevelLayout->addWidget(testButton);
    
    firstLevelBox->setLayout(firstLevelLayout);
}


WakeupKcm::~WakeupKcm()
{
    qDeleteAll(acpiEntries.begin(), acpiEntries.end());
}


void WakeupKcm::readACPI()
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
                connect(checkBox, SIGNAL (stateChanged(int )), SLOT(changed()));
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


void WakeupKcm::save()
{
    qDebug() << "Apply pressed!";
    
    // configPtr = KSharedConfig::openConfig("",KConfig::CascadeConfig, QStandardPaths::ConfigLocation);
    
    QVariantMap args;
    QStringList acpiEnabled;
    QStringList acpiDisabled;
    QStringList usbEnabled;
    QStringList usbDisabled;

    for (int i=0; i < acpiEntries.size(); i++) {
        
        // Entry is not configurable
        if (!acpiEntries.at(i)->getCheckBox())
            continue;
        
        usbEnabled += acpiEntries.at(i)->changedUSBEntries(true);
        usbDisabled += acpiEntries.at(i)->changedUSBEntries(false);
        
        // Entry did not change
        if(acpiEntries.at(i)->getCheckBox()->isChecked() == acpiEntries.at(i)->isEnabled())
            continue;
        
        if(acpiEntries.at(i)->getCheckBox()->isChecked() ) {
            acpiEnabled.push_back(acpiEntries.at(i)->getName());
        } else {
            acpiDisabled.push_back(acpiEntries.at(i)->getName());
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
    KAuth::Action updateConfigAction(QStringLiteral("org.kde.wakeupmanager.updateconfig"));
    updateConfigAction.setHelperId("org.kde.wakeupmanager");
    updateConfigAction.setArguments(args);
    qDebug() << "Executing job.";
    ExecuteJob *job = updateConfigAction.execute();
    if (!job->exec()) {
       qDebug() << "KAuth returned an error code:" << job->error() << ": " << job->errorString();
    } else {
    //   QString contents = job->data()["contents"].toString();
       qDebug() << "Success";
    }
}


void WakeupKcm::handleResetButton()
{
    qDebug() << "Reset pressed!";
    
    for (int i=0; i < acpiEntries.size(); i++) {
        
        // Entry is not configurable
        if (!acpiEntries.at(i)->getCheckBox())
            continue;
        
        acpiEntries.at(i)->resetUSBEntries();
        acpiEntries.at(i)->getCheckBox()->setChecked(acpiEntries.at(i)->isEnabled());
    }
}


extern "C"
{
    Q_DECL_EXPORT void kcminit_wakeup() // should be KDEEXPORT??
    {
        qDebug() << "Initializing kcm_wakeup";
    
        QVariantMap args;

        qDebug() << "Creating action!";
        KAuth::Action applyConfigAction(QStringLiteral("org.kde.wakeupmanager.applyconfig"));
        applyConfigAction.setHelperId("org.kde.wakeupmanager");
        applyConfigAction.setArguments(args);
        qDebug() << "Executing job.";
        ExecuteJob *job = applyConfigAction.execute();
        if (!job->exec()) {
            qDebug() << "KAuth returned an error code:" << job->error() << ": " << job->errorString();
        } else {
    //   QString contents = job->data()["contents"].toString();
            qDebug() << "Success";
        }
    }
    
}

#include "WakeupKcm.moc"

