#if !defined(AFX_GRADIENTFRAME_H__5C18DD6C_D458_4BBC_BC09_0D50C5D3F589__INCLUDED_)
#define AFX_GRADIENTFRAME_H__5C18DD6C_D458_4BBC_BC09_0D50C5D3F589__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// GradientFrame.h : main header file for GRADIENTFRAME.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CGradientFrameApp : See GradientFrame.cpp for implementation.

class CGradientFrameApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRADIENTFRAME_H__5C18DD6C_D458_4BBC_BC09_0D50C5D3F589__INCLUDED)
