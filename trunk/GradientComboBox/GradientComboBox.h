#if !defined(AFX_GRADIENTCOMBOBOX_H__6E7229F3_809A_4115_848D_6E7E7A1CC029__INCLUDED_)
#define AFX_GRADIENTCOMBOBOX_H__6E7229F3_809A_4115_848D_6E7E7A1CC029__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// GradientComboBox.h : main header file for GRADIENTCOMBOBOX.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxApp : See GradientComboBox.cpp for implementation.

class CGradientComboBoxApp : public COleControlModule
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

#endif // !defined(AFX_GRADIENTCOMBOBOX_H__6E7229F3_809A_4115_848D_6E7E7A1CC029__INCLUDED)
