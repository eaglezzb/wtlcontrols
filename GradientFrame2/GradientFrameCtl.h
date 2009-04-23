#if !defined(AFX_GRADIENTFRAMECTL_H__9309121B_B4BF_4A61_9076_9B6F546ABBEF__INCLUDED_)
#define AFX_GRADIENTFRAMECTL_H__9309121B_B4BF_4A61_9076_9B6F546ABBEF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// GradientFrameCtl.h : Declaration of the CGradientFrameCtrl ActiveX Control class.

/////////////////////////////////////////////////////////////////////////////
// CGradientFrameCtrl : See GradientFrameCtl.cpp for implementation.

class CGradientFrameCtrl : public COleControl
{
	DECLARE_DYNCREATE(CGradientFrameCtrl)

// Constructor
public:
	CGradientFrameCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGradientFrameCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CGradientFrameCtrl();

	DECLARE_OLECREATE_EX(CGradientFrameCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CGradientFrameCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CGradientFrameCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CGradientFrameCtrl)		// Type name and misc status

	// Subclassed control support
	BOOL IsSubclassedControl();
	LRESULT OnOcmCommand(WPARAM wParam, LPARAM lParam);

// Message maps
	//{{AFX_MSG(CGradientFrameCtrl)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CGradientFrameCtrl)
	OLE_COLOR m_clrBackColor;
	afx_msg void OnBackColorChanged();
	OLE_COLOR m_clrForeColor;
	afx_msg void OnForeColorChanged();
	long m_radius;
	afx_msg void OnRadiusChanged();
	BOOL m_borderVisible;
	afx_msg void OnBorderVisibleChanged();
	long m_borderWidth;
	afx_msg void OnBorderWidthChanged();
	OLE_COLOR m_borderColor;
	afx_msg void OnBorderColorChanged();
	afx_msg void Refresh();
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

// Event maps
	//{{AFX_EVENT(CGradientFrameCtrl)
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CGradientFrameCtrl)
	dispidBackColor = 1L,
	dispidForeColor = 2L,
	dispidRadius = 3L,
	dispidBorderVisible = 4L,
	dispidBorderWidth = 5L,
	dispidBorderColor = 6L,
	//}}AFX_DISP_ID
	};

private:
	//circle radius
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRADIENTFRAMECTL_H__9309121B_B4BF_4A61_9076_9B6F546ABBEF__INCLUDED)
