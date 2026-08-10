#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H

#include <Arduino.h>
#include "preferences.h"

class CNVSManager
{

private:
    String m_PageName;
    Preferences stcPrefs;

public : 
    void begin();
    void GetString(String strKey, String *pValue, String DefaultValue);
    void WriteString(String strKey, String strValue);
    void WriteLong(String strKey, long longValue);
    void GetLong(String strKey, long *pValue, long DefaultValue);

    ~CNVSManager()
	{
	}
};

extern CNVSManager NVSManager;

#endif


