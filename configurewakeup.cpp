
#include <WakeupConfig.h>
#include <syslog.h>


int main()
{
    // Log to syslog
    syslog(LOG_INFO, "configurewakeup called");
    
    QVariantMap config = WakeupConfig::readConfig();
    WakeupConfig::getUSBDeviceNodes(config);
    int ret = WakeupConfig::configureDevices(config);
    if (ret)
	return ret;

    ret = WakeupConfig::enableWOL(config);

    return ret;
}
