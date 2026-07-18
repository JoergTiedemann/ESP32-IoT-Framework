
#include "DiagManager.h"
#include "configManager.h"
#include "timeSync.h"
#include <inttypes.h>


//create global object
CDiagManager DiagManager;

//function to call in setup
void CDiagManager::begin(int LogSize,int VariableMonitorSize)
{    
    m_LogSize = LogSize;
    InitStrings();
    m_VariableSize = VariableMonitorSize;
    InitVariableMonitor();
}

void CDiagManager::InitStrings()
{
    m_strLogData  = new String[m_LogSize];
    for (int i = 0;i<m_LogSize;i++)
        m_strLogData[i] = "";
}

int CDiagManager::GetLogSize()
{
    return m_LogSize;
}


String CDiagManager::GetLogString(int iElement)
{
    if ((iElement>=0) && (iElement<m_LogSize))
        return m_strLogData[iElement];
    else
        return String("");
}

void CDiagManager::addLogData(const String *element)
{
    for (int i = (m_LogSize-2);i>=0;i--)
        m_strLogData[i+1] = m_strLogData[i];
	
    m_strLogData[0] = *element;
}

void CDiagManager::PushDiagData(long msgTyp,String strLogString)
{
   if (msgTyp <= configManager.data.Messagelevel)
   {
        String strTime = NTPSync::instance().GetDateTime();
        String strLog = strTime+strLogString;
        addLogData(&strLog);
   }
}

void CDiagManager::PushDiagData(long msgTyp,const char *format, ...)
{
   if (msgTyp <= configManager.data.Messagelevel)
   {
        String strTime = NTPSync::instance().GetDateTime();
     
        va_list arg;
        va_start(arg, format);
        char temp[64];
        char* buffer = temp;
        size_t len = vsnprintf(temp, sizeof(temp), format, arg);
        va_end(arg);
        if (len > sizeof(temp) - 1) {
            buffer = new char[len + 1];
            if (!buffer) {
                return;
            }
            va_start(arg, format);
            vsnprintf(buffer, len + 1, format, arg);
            va_end(arg);
        }
        String strtmplogstr(buffer);
        String strLog = strTime+strtmplogstr;
        addLogData(&strLog);
        // len = write((const uint8_t*) buffer, len);
        if (buffer != temp) {
            delete[] buffer;
        }
   }
}

void CDiagManager::InitVariableMonitor()
{
    m_pVariableMonitor  = new CVariable[m_VariableSize];
    for (int i = 0;i<m_VariableSize;i++)
    {
        m_pVariableMonitor[i].strVariableText = "";
        m_pVariableMonitor[i].pVariableAdress = NULL;
    }
}


void CDiagManager::AddVariableToMonitor(int index,String strVaiableName, long *VariableAdress)
{
        m_pVariableMonitor[index].strVariableText = strVaiableName;
        m_pVariableMonitor[index].pVariableAdress = VariableAdress;
        m_pVariableMonitor[index].m_type = VarType::tyuint32;
}

void CDiagManager::AddVariableToMonitor(int index,String strVaiableName, volatile long *VariableAdress)
{
        m_pVariableMonitor[index].strVariableText = strVaiableName;
        m_pVariableMonitor[index].pVariableAdress = (void*)VariableAdress;
        m_pVariableMonitor[index].m_type = VarType::tyuint32;
}

void CDiagManager::AddVariableToMonitor(int index,String strVaiableName, int *VariableAdress)
{
        m_pVariableMonitor[index].strVariableText = strVaiableName;
        m_pVariableMonitor[index].pVariableAdress = VariableAdress;
        m_pVariableMonitor[index].m_type = VarType::tyint;
}

void CDiagManager::AddVariableToMonitor(int index,String strVaiableName, String *VariableAdress)
{
        m_pVariableMonitor[index].strVariableText = strVaiableName;
        m_pVariableMonitor[index].pVariableAdress = VariableAdress;
        m_pVariableMonitor[index].m_type = VarType::tystring;
}

void CDiagManager::AddVariableToMonitor(int index,String strVaiableName, bool *VariableAdress)
{
        m_pVariableMonitor[index].strVariableText = strVaiableName;
        m_pVariableMonitor[index].pVariableAdress = VariableAdress;
        m_pVariableMonitor[index].m_type = VarType::tybool;
}

void CDiagManager::AddVariableToMonitor(int index,String strVaiableName, volatile bool *VariableAdress)
{
        m_pVariableMonitor[index].strVariableText = strVaiableName;
        m_pVariableMonitor[index].pVariableAdress = (void*)VariableAdress;
        m_pVariableMonitor[index].m_type = VarType::tybool;
}

void CDiagManager::AddVariableToMonitor(int index,String strVariableName, float *VariableAdress)
{
        m_pVariableMonitor[index].strVariableText = strVariableName;
        m_pVariableMonitor[index].pVariableAdress = VariableAdress;
        m_pVariableMonitor[index].m_type = VarType::tyfloat;
}

void CDiagManager::AddVariableToMonitor(int index,String strVariableName, uint16_t *VariableAdress)
{
        m_pVariableMonitor[index].strVariableText = strVariableName;
        m_pVariableMonitor[index].pVariableAdress = VariableAdress;
        m_pVariableMonitor[index].m_type = VarType::tyuint16;
}

void CDiagManager::AddVariableToMonitor(int index,String strVariableName, uint32_t *VariableAdress)
{
        m_pVariableMonitor[index].strVariableText = strVariableName;
        m_pVariableMonitor[index].pVariableAdress = VariableAdress;
        m_pVariableMonitor[index].m_type = VarType::tyuint32;
}

int CDiagManager::GetVariableMonitorSize()
{
    return m_VariableSize;
}

String CDiagManager::GetVariableMonitorValue(int iElement)
{
    if ((iElement>=0) && (iElement<m_VariableSize) && (m_pVariableMonitor[iElement].pVariableAdress != NULL))
    {
        String strRet("");
        int *pint;
        uint16_t *puint16;
        uint32_t *puint32;
        bool *pbool;
        String *pstring;
        char  *pchar;
        float* pfloat;
        switch (m_pVariableMonitor[iElement].m_type)
        {
            case VarType::tyint:
                pint = (int*)m_pVariableMonitor[iElement].pVariableAdress;
                strRet = String(*pint);
                break;        
            case VarType::tyuint16:
                puint16 = (uint16_t*)m_pVariableMonitor[iElement].pVariableAdress;
                strRet = String(*puint16);
                break;        
            case VarType::tybool:
                pbool = (bool*)m_pVariableMonitor[iElement].pVariableAdress;
                strRet = String(*pbool);
                break;        
            case VarType::tystring:
                pstring = (String*)m_pVariableMonitor[iElement].pVariableAdress;
                strRet = String(*pstring);
                break;        
            case VarType::tyfloat:
                pfloat = (float*)m_pVariableMonitor[iElement].pVariableAdress;
                strRet = String(*pfloat);
                break;        
            case VarType::tyuint32:
                puint32 = (uint32_t*)m_pVariableMonitor[iElement].pVariableAdress;
                strRet = String(*puint32);
                break;        
        }
        return strRet;
    }
    else
        return String("");
}

String CDiagManager::GetVariableMonitorName(int iElement)
{
    if ((iElement>=0) && (iElement<m_VariableSize))
        return m_pVariableMonitor[iElement].strVariableText;
    else
        return String("");
}


