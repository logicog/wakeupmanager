#ifndef WAKEUPMANAGER_H
#define WAKEUPMANAGER_H
 
#include <KXmlGuiWindow>
#include "ACPIEntry.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>

#include <KSharedConfig>

class WakeupManager : public KXmlGuiWindow
{    
     Q_OBJECT
    
  public:
    explicit WakeupManager(QWidget *parent = nullptr);
    ~WakeupManager();
    
  private:
    QGroupBox           *firstLevelBox;
    QVBoxLayout         *firstLevelLayout;
    void                readACPI();
    void                setupActions();
    QList<ACPIEntry *> acpiEntries;
    QPushButton         *resetButton;
    QPushButton         *testButton;
    QPushButton         *applyButton;
    
    public Q_SLOTS:
        void handleApplyButton();
        void handleResetButton();
        void handleTestButton();
};
 
#endif
