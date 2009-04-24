// GradientComboBoxPpg.cpp : Implementation of the CGradientComboBoxPropPage property page class.

#include "stdafx.h"
#include "GradientComboBox.h"
#include "GradientComboBoxPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CGradientComboBoxPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CGradientComboBoxPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CGradientComboBoxPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CGradientComboBoxPropPage, "GRADIENTCOMBOBOX.GradientComboBoxPropPage.1",
	0x1fe71c3, 0x6a9f, 0x41f8, 0x9c, 0xbd, 0x30, 0x41, 0x33, 0xda, 0x63, 0xfd)


/////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxPropPage::CGradientComboBoxPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CGradientComboBoxPropPage

BOOL CGradientComboBoxPropPage::CGradientComboBoxPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_GRADIENTCOMBOBOX_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxPropPage::CGradientComboBoxPropPage - Constructor

CGradientComboBoxPropPage::CGradientComboBoxPropPage() :
	COlePropertyPage(IDD, IDS_GRADIENTCOMBOBOX_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CGradientComboBoxPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxPropPage::DoDataExchange - Moves data between page and properties

void CGradientComboBoxPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CGradientComboBoxPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxPropPage message handlers
