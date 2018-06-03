#ifndef WAKEUPKCM_H
#define WAKEUPKCM_H

#include "ACPIEntry.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <KCModule>

class WakeupKcm : public KCModule
{    
     Q_OBJECT
    
  public:
    explicit WakeupKcm(QWidget *parent, const QVariantList &args);
    ~WakeupKcm();
    
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
        void save();
        void handleResetButton();
};
 
#endif
