#ifndef DIAG_H
#define DIAG_H

#include <Arduino.h>



enum VarType {tyint=0,tybool=1,tyfloat=2,tystring=3,tyuint16=4,tyuint32=5};

const int msgAll=0;
const int msgFehler=1;
const int msgMeldung=2;

class CVariable
{
    public : 
    VarType m_type;
    String strVariableText;
    void *pVariableAdress;
};


class CDiagManager
{

private:
	int m_LogSize;
	String *m_strLogData;

	int m_VariableSize;
	CVariable *m_pVariableMonitor;

    void InitStrings();
    void addLogData(const String *element);

    void InitVariableMonitor();

public : 
    void begin(int LogSize,int VariableMonitorSize);

    int GetLogSize();
    String GetLogString(int iElement);
    void PushDiagData(long msgTyp,const char *format, ...);
    void PushDiagData(long msgTyp,String strLogString);

    int GetVariableMonitorSize();
    String GetVariableMonitorName(int iElement);
    String GetVariableMonitorValue(int iElement);

    void AddVariableToMonitor(int index,String strVaiableName, int *VariableAdress);
    void AddVariableToMonitor(int index,String strVaiableName, long *VariableAdress);
    void AddVariableToMonitor(int index,String strVaiableName, volatile long *VariableAdress);
    void AddVariableToMonitor(int index,String strVaiableName, String *VariableAdress);
    void AddVariableToMonitor(int index,String strVaiableName, bool  *VariableAdress);
    void AddVariableToMonitor(int index,String strVaiableName, volatile bool  *VariableAdress);
    void AddVariableToMonitor(int index,String strVaiableName, float *VariableAdress);
    void AddVariableToMonitor(int index,String strVariableName, uint16_t *VariableAdress);
    void AddVariableToMonitor(int index,String strVariableName, uint32_t *VariableAdress);

    CDiagManager()
	{
		m_LogSize = 0;
	}

    ~CDiagManager()
	{
		delete []m_strLogData;
		delete []m_pVariableMonitor;
	}

};

extern CDiagManager DiagManager;

#endif


