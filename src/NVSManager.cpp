#include "NVSManager.h"

//create global object
CNVSManager NVSManager;


//function to call in setup
void CNVSManager::begin()
{
    m_PageName = "PersistData";
}


void CNVSManager::GetString(String strKey, String *pValue, String DefaultValue)
{
    stcPrefs.begin(m_PageName.c_str(), false);
    *pValue = stcPrefs.getString(strKey.c_str(), DefaultValue);
    stcPrefs.end();
}

void CNVSManager::WriteString(String strKey, String strValue)
{
    stcPrefs.begin(m_PageName.c_str(), false);
    stcPrefs.putString(strKey.c_str(), strValue);
    stcPrefs.end();
}

void CNVSManager::WriteLong(String strKey, long longValue)
{
    stcPrefs.begin(m_PageName.c_str(), false);
    stcPrefs.putLong(strKey.c_str(), longValue);
    stcPrefs.end();
}

void CNVSManager::GetLong(String strKey, long *pValue, long DefaultValue)
{
    stcPrefs.begin(m_PageName.c_str(), false);
    *pValue = stcPrefs.getLong(strKey.c_str(), DefaultValue);
    stcPrefs.end();
}