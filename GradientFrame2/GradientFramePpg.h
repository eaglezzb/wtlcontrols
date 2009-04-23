#if !defined(AFX_GRADIENTFRAMEPPG_H__CB44C218_F120_488F_9636_A044F791E219__INCLUDED_)
#define AFX_GRADIENTFRAMEPPG_H__CB44C218_F120_488F_9636_A044F791E219__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// GradientFramePpg.h : Declaration of the CGradientFramePropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CGradientFramePropPage : See GradientFramePpg.cpp.cpp for implementation.

class CGradientFramePropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CGradientFramePropPage)
	DECLARE_OLECREATE_EX(CGradientFramePropPage)

// Constructor
public:
	CGradientFramePropPage();

// Dialog Data
	//{{AFX_DATA(CGradientFramePropPage)
	enum { IDD = IDD_PROPPAGE_GRADIENTFRAME };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

private:
	long m_radius;

// Message maps
protected:
	//{{AFX_MSG(CGradientFramePropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRADIENTFRAMEPPG_H__CB44C218_F120_488F_9636_A044F791E219__INCLUDED)
