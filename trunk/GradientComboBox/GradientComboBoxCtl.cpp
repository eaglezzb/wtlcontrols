// GradientComboBoxCtl.cpp : Implementation of the CGradientComboBoxCtrl ActiveX Control class.

#include "stdafx.h"
#include "GradientComboBox.h"
#include "GradientComboBoxCtl.h"
#include "GradientComboBoxPpg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CGradientComboBoxCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CGradientComboBoxCtrl, COleControl)
	//{{AFX_MSG_MAP(CGradientComboBoxCtrl)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_CTLCOLORLISTBOX, OnCtlColorListBox)
	ON_MESSAGE(OCM_COMMAND, OnOcmCommand)
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CGradientComboBoxCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CGradientComboBoxCtrl)
	DISP_FUNCTION(CGradientComboBoxCtrl, "AddString", AddString, VT_I4, VTS_BSTR)
	DISP_STOCKPROP_ENABLED()
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CGradientComboBoxCtrl, COleControl)
	//{{AFX_EVENT_MAP(CGradientComboBoxCtrl)
	// NOTE - ClassWizard will add and remove event map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CGradientComboBoxCtrl, 1)
	PROPPAGEID(CGradientComboBoxPropPage::guid)
END_PROPPAGEIDS(CGradientComboBoxCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CGradientComboBoxCtrl, "GRADIENTCOMBOBOX.GradientComboBoxCtrl.1",
	0xd593b2f2, 0x1fc4, 0x477e, 0xb2, 0x18, 0x1a, 0x27, 0x3e, 0x28, 0x6f, 0x55)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CGradientComboBoxCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DGradientComboBox =
		{ 0x109b54c7, 0xe1b9, 0x4278, { 0xb4, 0x1e, 0x47, 0xff, 0x29, 0x5a, 0x76, 0x7 } };
const IID BASED_CODE IID_DGradientComboBoxEvents =
		{ 0x7ddb5a60, 0x7baa, 0x4681, { 0xbc, 0x86, 0x71, 0x65, 0x6b, 0xd9, 0x1a, 0x2f } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwGradientComboBoxOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CGradientComboBoxCtrl, IDS_GRADIENTCOMBOBOX, _dwGradientComboBoxOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxCtrl::CGradientComboBoxCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CGradientComboBoxCtrl

BOOL CGradientComboBoxCtrl::CGradientComboBoxCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegInsertable | afxRegApartmentThreading to afxRegInsertable.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_GRADIENTCOMBOBOX,
			IDB_GRADIENTCOMBOBOX,
			afxRegInsertable | afxRegApartmentThreading,
			_dwGradientComboBoxOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxCtrl::CGradientComboBoxCtrl - Constructor

CGradientComboBoxCtrl::CGradientComboBoxCtrl()
{
	InitializeIIDs(&IID_DGradientComboBox, &IID_DGradientComboBoxEvents);

	// TODO: Initialize your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxCtrl::~CGradientComboBoxCtrl - Destructor

CGradientComboBoxCtrl::~CGradientComboBoxCtrl()
{
	// TODO: Cleanup your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxCtrl::OnDraw - Drawing function

void CGradientComboBoxCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	DoSuperclassPaint(pdc, rcBounds);
}


/////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxCtrl::DoPropExchange - Persistence support

void CGradientComboBoxCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxCtrl::OnResetState - Reset control to default state

void CGradientComboBoxCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxCtrl::PreCreateWindow - Modify parameters for CreateWindowEx

BOOL CGradientComboBoxCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = _T("COMBOBOX");
	//cs.style &= ~0x0001L;
	//cs.style |= CBS_DROPDOWN|CBS_HASSTRINGS|CBS_AUTOHSCROLL;
	cs.style |= CBS_DROPDOWN|CBS_AUTOHSCROLL|WS_VSCROLL;
	//return  COleControl::PreCreateWindow(cs);
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxCtrl::IsSubclassedControl - This is a subclassed control

BOOL CGradientComboBoxCtrl::IsSubclassedControl()
{
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxCtrl::OnOcmCommand - Handle command messages

LRESULT CGradientComboBoxCtrl::OnOcmCommand(WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32
	WORD wNotifyCode = HIWORD(wParam);
#else
	WORD wNotifyCode = HIWORD(lParam);
#endif

	// TODO: Switch on wNotifyCode here.

	return 0;
}


LRESULT CGradientComboBoxCtrl::OnCtlColorListBox(WPARAM wParam, LPARAM lParam) 
{
	// Here we need to get a reference to the listbox of the combobox
	// (the dropdown part). We can do it using 
	//TRACE("OnCtlColorListBox()\n");
// 	if (this->m_listbox.m_hWnd == 0) {
// 		HWND hWnd = (HWND)lParam;
// 		
// 		if (hWnd != 0 && hWnd != m_hWnd) 
// 		{
// 			// Save the handle
// 			m_listbox.m_hWnd = hWnd;
// 			
// 			// Subclass ListBox
// 			m_pWndProc = (WNDPROC)GetWindowLong(m_listbox.m_hWnd, GWL_WNDPROC);
// 			SetWindowLong(m_listbox.m_hWnd, GWL_WNDPROC, (LONG)BitComboBoxListBoxProc);
// 		}
// 	}
// 	
	SetDroppedWidth(100);
	
	return DefWindowProc(WM_CTLCOLORLISTBOX, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxCtrl message handlers


// void CGradientComboBoxCtrl::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
// {
// 	COleControl::OnActivate(nState, pWndOther, bMinimized);
// 	
// 	// TODO: Add your message handler code here
// 	
// }

// void CGradientComboBoxCtrl::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
// {
// 	// TODO: Add your message handler code here and/or call default
// 	
// 	COleControl::OnDrawItem(nIDCtl, lpDrawItemStruct);
// }

void CGradientComboBoxCtrl::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	// TODO: Add your message handler code here and/or call default
	
	COleControl::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}

long CGradientComboBoxCtrl::AddString(LPCTSTR text) 
{
	long index = AddStringInternal(text);

	SetModifiedFlag(TRUE);
	InvalidateControl();

	//CString str;
	//GetLBText(index, str);
	return SetCurSel(index);

}


// int CGradientComboBoxCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
// {
// // 	if (COleControl::OnCreate(lpCreateStruct) == -1)
// // 		return -1;
// // 	
// // 	// TODO: Add your specialized creation code here
// 	RECT rect;
// 	rect.left = lpCreateStruct->x;
// 	rect.top = lpCreateStruct->y;
// 	rect.right = lpCreateStruct->x+lpCreateStruct->cx;
// 	rect.bottom = lpCreateStruct->y + lpCreateStruct->cy;
// 	return this->Create(lpCreateStruct->dwExStyle, rect, CWnd::FromHandlePermanent(lpCreateStruct->hwndParent), 10);
// 
// 	return 0;
// }
