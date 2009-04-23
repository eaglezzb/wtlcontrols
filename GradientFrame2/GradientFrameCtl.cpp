// GradientFrameCtl.cpp : Implementation of the CGradientFrameCtrl ActiveX Control class.

#include "stdafx.h"
#include "GradientFrame.h"
#include "GradientFrameCtl.h"
#include "GradientFramePpg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CGradientFrameCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CGradientFrameCtrl, COleControl)
	//{{AFX_MSG_MAP(CGradientFrameCtrl)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
	ON_MESSAGE(OCM_COMMAND, OnOcmCommand)
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CGradientFrameCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CGradientFrameCtrl)
	DISP_PROPERTY_NOTIFY(CGradientFrameCtrl, "BackColor", m_clrBackColor, OnBackColorChanged, VT_COLOR)
	DISP_PROPERTY_NOTIFY(CGradientFrameCtrl, "ForeColor", m_clrForeColor, OnForeColorChanged, VT_COLOR)
	DISP_PROPERTY_NOTIFY(CGradientFrameCtrl, "Radius", m_radius, OnRadiusChanged, VT_I4)
	DISP_PROPERTY_NOTIFY(CGradientFrameCtrl, "BorderVisible", m_borderVisible, OnBorderVisibleChanged, VT_BOOL)
	DISP_PROPERTY_NOTIFY(CGradientFrameCtrl, "BorderWidth", m_borderWidth, OnBorderWidthChanged, VT_I4)
	DISP_PROPERTY_NOTIFY(CGradientFrameCtrl, "BorderColor", m_borderColor, OnBorderColorChanged, VT_COLOR)
	DISP_STOCKPROP_ENABLED()
	DISP_FUNCTION_ID(CGradientFrameCtrl, "Refresh", DISPID_REFRESH, Refresh, VT_EMPTY, VTS_NONE)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CGradientFrameCtrl, COleControl)
	//{{AFX_EVENT_MAP(CGradientFrameCtrl)
	// NOTE - ClassWizard will add and remove event map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CGradientFrameCtrl, 1)
	PROPPAGEID(CGradientFramePropPage::guid)
END_PROPPAGEIDS(CGradientFrameCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CGradientFrameCtrl, "GRADIENTFRAME.GradientFrameCtrl.1",
	0x1e448df8, 0xe7b, 0x43ed, 0x8c, 0xe1, 0x2e, 0x80, 0xdf, 0x28, 0x35, 0xcf)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CGradientFrameCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DGradientFrame =
		{ 0xdf226003, 0x637, 0x440e, { 0xb9, 0x67, 0xc5, 0x4e, 0x3b, 0x60, 0x97, 0x17 } };
const IID BASED_CODE IID_DGradientFrameEvents =
		{ 0xcbb78c23, 0x5d6d, 0x4003, { 0xb3, 0x5a, 0x47, 0xeb, 0x2d, 0x30, 0x9f, 0x9d } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwGradientFrameOleMisc =
	OLEMISC_SIMPLEFRAME |
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_IGNOREACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CGradientFrameCtrl, IDS_GRADIENTFRAME, _dwGradientFrameOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CGradientFrameCtrl::CGradientFrameCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CGradientFrameCtrl

BOOL CGradientFrameCtrl::CGradientFrameCtrlFactory::UpdateRegistry(BOOL bRegister)
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
			IDS_GRADIENTFRAME,
			IDB_GRADIENTFRAME,
			afxRegInsertable | afxRegApartmentThreading,
			_dwGradientFrameOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CGradientFrameCtrl::CGradientFrameCtrl - Constructor

CGradientFrameCtrl::CGradientFrameCtrl():
	m_radius(20), m_borderVisible(FALSE), m_borderWidth(2), m_borderColor(16777215)
{
	InitializeIIDs(&IID_DGradientFrame, &IID_DGradientFrameEvents);

	EnableSimpleFrame();

	m_clrBackColor = 16777215;
	m_clrForeColor = 12999969;

	// TODO: Initialize your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CGradientFrameCtrl::~CGradientFrameCtrl - Destructor

CGradientFrameCtrl::~CGradientFrameCtrl()
{
	// TODO: Cleanup your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CGradientFrameCtrl::OnDraw - Drawing function

void CGradientFrameCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	int nWidth = rcBounds.right - rcBounds.left + 1;
	int nHeight = rcBounds.bottom - rcBounds.top + 1;
	//
	
	CDC MemDC; //���ȶ���һ����ʾ�豸����
	CBitmap MemBitmap;//����һ��λͼ����
	//���������Ļ��ʾ���ݵ��ڴ���ʾ�豸
	MemDC.CreateCompatibleDC(pdc);
	//��ʱ�����ܻ�ͼ����Ϊû�еط��� ^_^
	//���潨��һ������Ļ��ʾ���ݵ�λͼ������λͼ�Ĵ�С������ô��ڵĴ�С
	MemBitmap.CreateCompatibleBitmap(pdc, nWidth, nHeight);
	
	//��λͼѡ�뵽�ڴ���ʾ�豸��
	//ֻ��ѡ����λͼ���ڴ���ʾ�豸���еط���ͼ������ָ����λͼ��
	HBITMAP OldBit = (HBITMAP) MemDC.SelectObject(MemBitmap);
	//���ñ���ɫ��λͼ����ɾ����������õ��ǰ�ɫ��Ϊ����
	//��Ҳ�������Լ�Ӧ���õ���ɫ
	MemDC.FillSolidRect(0, 0, nWidth, nHeight, m_clrBackColor);
	
	//��ͼ
	
	CBrush brush;
	brush.CreateSolidBrush(m_clrForeColor);	
	CRgn rgn;
	rgn.CreateRoundRectRgn(0, 0, nWidth, nHeight, m_radius, m_radius);
	MemDC.FillRgn(&rgn, &brush);
	
	if (m_borderVisible)
	{
// 		HPEN pen = ::CreatePen(m_nBorderStyle, m_borderWidth, m_borderColor);
// 		HPEN oldPen = (HPEN)MemDC.SelectObject(pen);
		LOGBRUSH logBrush;
		logBrush.lbStyle = BS_SOLID;
		logBrush.lbColor = m_borderColor;
		logBrush.lbHatch = 0;
		CBrush rgnBrush;
		rgnBrush.CreateBrushIndirect(&logBrush);
		BOOL framed = MemDC.FrameRgn(&rgn, &rgnBrush, m_borderWidth, m_borderWidth);
// 		MemDC.SelectPen(oldPen);
	}
	
	
	//���ڴ��е�ͼ��������Ļ�Ͻ�����ʾ
	BOOL result = pdc->BitBlt(0,0,nWidth,nHeight, &MemDC,0,0,SRCCOPY);
	//��ͼ��ɺ������
	brush.DeleteObject();
	MemBitmap.DeleteObject();
	MemDC.DeleteDC();
	//DoSuperclassPaint(pdc, rcBounds);
}


/////////////////////////////////////////////////////////////////////////////
// CGradientFrameCtrl::DoPropExchange - Persistence support

void CGradientFrameCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.
	if (pPX->GetVersion() == (DWORD)MAKELONG(_wVerMinor, _wVerMajor))
	{
		PX_Color(pPX, _T("BackColor"), m_clrBackColor, 16777215);
		PX_Color(pPX, _T("ForeColor"), m_clrForeColor, 12999969);
		PX_Long(pPX, _T("Radius"), m_radius, 20);
		PX_Color(pPX, _T("BorderColor"), m_borderColor, 16777215);
		PX_Long(pPX, _T("BorderWidth"), m_borderWidth, 2);
		PX_Bool(pPX, _T("BorderVisible"), m_borderVisible, FALSE);
	}
	else if (pPX->IsLoading())
	{
		// Skip over persistent data
		PX_Color(pPX, _T("BackColor"), m_clrBackColor, 16777215);
		PX_Color(pPX, _T("ForeColor"), m_clrForeColor, 12999969);
		PX_Long(pPX, _T("Radius"), m_radius, 20);
		PX_Color(pPX, _T("BorderColor"), m_borderColor, 16777215);
		PX_Long(pPX, _T("BorderWidth"), m_borderWidth, 2);
		PX_Bool(pPX, _T("BorderVisible"), m_borderVisible, FALSE);

		// Force property values to these defaults
		m_radius = 20;
		m_clrBackColor = 16777215;
		m_clrForeColor = 12999969;
		m_borderVisible = FALSE;
		m_borderWidth = 2;
		m_borderColor = 16777215;
	}

}



/////////////////////////////////////////////////////////////////////////////
// CGradientFrameCtrl::OnResetState - Reset control to default state

void CGradientFrameCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	m_radius = 20;
	m_clrBackColor = 16777215;
	m_clrForeColor = 12999969;
	m_borderVisible = FALSE;
	m_borderWidth = 2;
	m_borderColor = 16777215;
	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CGradientFrameCtrl::PreCreateWindow - Modify parameters for CreateWindowEx

BOOL CGradientFrameCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = _T("STATIC");
	return COleControl::PreCreateWindow(cs);
}


/////////////////////////////////////////////////////////////////////////////
// CGradientFrameCtrl::IsSubclassedControl - This is a subclassed control

BOOL CGradientFrameCtrl::IsSubclassedControl()
{
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CGradientFrameCtrl::OnOcmCommand - Handle command messages

LRESULT CGradientFrameCtrl::OnOcmCommand(WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32
	WORD wNotifyCode = HIWORD(wParam);
#else
	WORD wNotifyCode = HIWORD(lParam);
#endif

	// TODO: Switch on wNotifyCode here.

	return 0;
}


/////////////////////////////////////////////////////////////////////////////
// CGradientFrameCtrl message handlers

void CGradientFrameCtrl::Refresh() 
{
	// TODO: Add your dispatch handler code here
	Invalidate(TRUE);

}

void CGradientFrameCtrl::OnBackColorChanged() 
{
	// TODO: Add notification handler code	
	SetModifiedFlag();

	Invalidate(TRUE);
}

void CGradientFrameCtrl::OnForeColorChanged() 
{
	// TODO: Add notification handler code

	SetModifiedFlag();

	Invalidate(TRUE);
}


void CGradientFrameCtrl::OnRadiusChanged() 
{
	// TODO: Add notification handler code

	SetModifiedFlag();

	Invalidate(TRUE);
}

void CGradientFrameCtrl::OnBorderVisibleChanged() 
{
	// TODO: Add notification handler code

	SetModifiedFlag();

	Invalidate(TRUE);
}

void CGradientFrameCtrl::OnBorderWidthChanged() 
{
	// TODO: Add notification handler code

	SetModifiedFlag();
	Invalidate(TRUE);
}

void CGradientFrameCtrl::OnBorderColorChanged() 
{
	// TODO: Add notification handler code

	SetModifiedFlag();

	Invalidate(TRUE);
}
