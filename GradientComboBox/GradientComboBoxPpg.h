#if !defined(AFX_GRADIENTCOMBOBOXPPG_H__3796D8E3_7B71_41A1_A809_248988F11B33__INCLUDED_)
#define AFX_GRADIENTCOMBOBOXPPG_H__3796D8E3_7B71_41A1_A809_248988F11B33__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// GradientComboBoxPpg.h : Declaration of the CGradientComboBoxPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxPropPage : See GradientComboBoxPpg.cpp.cpp for implementation.

class CGradientComboBoxPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CGradientComboBoxPropPage)
	DECLARE_OLECREATE_EX(CGradientComboBoxPropPage)

// Constructor
public:
	CGradientComboBoxPropPage();

// Dialog Data
	//{{AFX_DATA(CGradientComboBoxPropPage)
	enum { IDD = IDD_PROPPAGE_GRADIENTCOMBOBOX };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CGradientComboBoxPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRADIENTCOMBOBOXPPG_H__3796D8E3_7B71_41A1_A809_248988F11B33__INCLUDED)
