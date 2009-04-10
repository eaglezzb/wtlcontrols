// TimeCounter.h
//


// File is used to measure how much time take some parts of the program
//
// Using:
//
// 1) include this file to any C++ source file
//
// 2) Write
//    SHOW_TIME_START(<variable name>, _T("<section name>"));
//
//    in the start of some program section; write
//    SHOW_TIME_END(<variable name>);
//
//    in the end of this section.
//
//    Result is line  
//    <section name>: n ms 
//    in program debug stream (number of milliseconds program section was executed)
//
// Any number of macros may be used in one function, and they may be overlapped.
// Variable names should be unique.
// If program executes SHOW_TIME_START macro and doesn't execute SHOW_TIME_END macro,
// no message is printed.
//
//
// Example:
// 
// void Function1()
// {
//     SHOW_TIME_START(t, _T("Initialization"));
//     SHOW_TIME_START(t1, _T("Initialization. Step 1"));
//     
//     ... some code
//
//     SHOW_TIME_END(t1);
//     SHOW_TIME_START(t2, _T("Initialization. Step 2"));
//     
//     ... some code
//
//     SHOW_TIME_END(t2);
//     SHOW_TIME_END(t);
//  }
//
//
// Result in debug stream looks like this:
//
// Initialization. Step 1  400 ms
// Initialization. Step 2  200 ms
// Initialization  600 ms


#ifndef TIME_COUNTER_H
#define TIME_COUNTER_H

// comment next line to disable time counting completely:
#define TIME_COUNT_ENABLED


// Class prints to Debug output stream 
// time between instance creating and destroying
class CTimeCounter
{
public:
    CTimeCounter(LPCTSTR sName)
    {
        m_sName = sName;        // keep only pointer and hope
                                // user will not destroy original string

        //m_nStart = GetTickCount();

		//获取精确时间
		if (QueryPerformanceFrequency(&m_nFreq))
		{
			QueryPerformanceCounter(&m_nLastTime1);
		}
        m_bShowTime = FALSE;
    }
    ~CTimeCounter()
    {
        if ( m_bShowTime )
        {
			LARGE_INTEGER nLastTime2;
			QueryPerformanceCounter(&nLastTime2);

			float fInterval = nLastTime2.QuadPart - m_nLastTime1.QuadPart;
            TCHAR* s = new TCHAR[lstrlen(m_sName) + 64];
			//_stprintf(s, _T("%s  %d ms\n"), m_sName, GetTickCount() - m_nStart);
            //AtlTrace(s);
			_stprintf(s, _T("%s  %.2f mis\n"), "microscends", fInterval);
            AtlTrace(s);
            delete[] s;
        }
    }

    void SetShowTime()          // prints only after call to this function
    {
        m_bShowTime = TRUE;
    }

protected:
    //long m_nStart;
    LPCTSTR m_sName;
    BOOL m_bShowTime;
	
	LARGE_INTEGER m_nFreq;
	LARGE_INTEGER m_nLastTime1;
	//LARGE_INTEGER m_nLastTime2;
};

// smart pointer to CTimeCounter
class CTimeCounterSmartPtr
{
public:
    CTimeCounterSmartPtr(LPCTSTR sName)
    {
        m_ptr = new CTimeCounter(sName);
    }

    ~CTimeCounterSmartPtr()
    {
        // if CTimeCounterPtr instance is deleted here (SHOW_TIME_END was not executed),
        // message is not printed
        if ( m_ptr )
            delete m_ptr;
    }

    void DeleteObject()
    {
        // if CTimeCounterPtr instance is deleted here (SHOW_TIME_END is executed),
        // message is printed
        m_ptr->SetShowTime();
        delete m_ptr;
        m_ptr = NULL;
    }

protected:
    CTimeCounter* m_ptr;
};

#ifdef TIME_COUNT_ENABLED

#define SHOW_TIME_START(x, y) CTimeCounterSmartPtr x(y);
#define SHOW_TIME_END(x) x.DeleteObject();

#else

#define SHOW_TIME_START(x, y)
#define SHOW_TIME_END(x)

#endif

#endif