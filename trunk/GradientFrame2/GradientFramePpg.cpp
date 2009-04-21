// GradientFramePpg.cpp : Implementation of the CGradientFramePropPage property page class.

#include "stdafx.h"
#include "GradientFrame.h"
#include "GradientFramePpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CGradientFramePropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CGradientFramePropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CGradientFramePropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CGradientFramePropPage, "GRADIENTFRAME.GradientFramePropPage.1",
	0x8729e11a, 0x43e7, 0x4e0c, 0x90, 0x3d, 0x9f, 0xa8, 0x3a, 0x6, 0x6d, 0x80)


/////////////////////////////////////////////////////////////////////////////
// CGradientFramePropPage::CGradientFramePropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CGradientFramePropPage

BOOL CGradientFramePropPage::CGradientFramePropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_GRADIENTFRAME_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CGradientFramePropPage::CGradientFramePropPage - Constructor

CGradientFramePropPage::CGradientFramePropPage() :
	COlePropertyPage(IDD, IDS_GRADIENTFRAME_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CGradientFramePropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CGradientFramePropPage::DoDataExchange - Moves data between page and properties

void CGradientFramePropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CGradientFramePropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CGradientFramePropPage message handlers
