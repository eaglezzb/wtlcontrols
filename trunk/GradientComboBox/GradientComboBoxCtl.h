#if !defined(AFX_GRADIENTCOMBOBOXCTL_H__F228F4AB_A178_4DA0_9040_E8E903E7F7C6__INCLUDED_)
#define AFX_GRADIENTCOMBOBOXCTL_H__F228F4AB_A178_4DA0_9040_E8E903E7F7C6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// GradientComboBoxCtl.h : Declaration of the CGradientComboBoxCtrl ActiveX Control class.

/////////////////////////////////////////////////////////////////////////////
// CGradientComboBoxCtrl : See GradientComboBoxCtl.cpp for implementation.

class CGradientComboBoxCtrl : public COleControl
{
	DECLARE_DYNCREATE(CGradientComboBoxCtrl)

// Constructor
public:
	CGradientComboBoxCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGradientComboBoxCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CGradientComboBoxCtrl();

	DECLARE_OLECREATE_EX(CGradientComboBoxCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CGradientComboBoxCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CGradientComboBoxCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CGradientComboBoxCtrl)		// Type name and misc status

	// Subclassed control support
	BOOL IsSubclassedControl();
	LRESULT OnOcmCommand(WPARAM wParam, LPARAM lParam);

// Message maps
	//{{AFX_MSG(CGradientComboBoxCtrl)
// 	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
// 	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
// 	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	afx_msg LRESULT OnCtlColorListBox(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CGradientComboBoxCtrl)
	afx_msg long AddString(LPCTSTR text);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

// Event maps
	//{{AFX_EVENT(CGradientComboBoxCtrl)
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CGradientComboBoxCtrl)
	dispidAddString = 1L,
	//}}AFX_DISP_ID
	};

//copy from Microsoft MFC code library, wrap messages
private:

	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID) {
		CWnd* pWnd = this;
		return pWnd->Create(_T("COMBOBOX"), NULL, dwStyle, rect, pParentWnd, nID);
	}

	// Attributes
	// for entire combo box
	int GetCount() const {
		return (int)::SendMessage(m_hWnd, CB_GETCOUNT, 0, 0);
	}

	int GetCurSel() const {
		return (int)::SendMessage(m_hWnd, CB_GETCURSEL, 0, 0);
	}
	int SetCurSel(int nSelect) {
		return (int)::SendMessage(m_hWnd, CB_SETCURSEL, nSelect, 0);
	}

	LCID GetLocale() const {
		return (LCID)::SendMessage(m_hWnd, CB_GETLOCALE, 0, 0); 
	}
	LCID SetLocale(LCID nNewLocale) {
		return (LCID)::SendMessage(m_hWnd, CB_SETLOCALE, (WPARAM)nNewLocale, 0); 
	}
	// Win4
	int GetTopIndex() const {
		return (int)::SendMessage(m_hWnd, CB_GETTOPINDEX, 0, 0);
	}

	int SetTopIndex(int nIndex) {
		return (int)::SendMessage(m_hWnd, CB_SETTOPINDEX, nIndex, 0); 
	}

	int InitStorage(int nItems, UINT nBytes) {
		return (int)::SendMessage(m_hWnd, CB_INITSTORAGE, (WPARAM)nItems, nBytes); 
	}

	void SetHorizontalExtent(UINT nExtent) {
		::SendMessage(m_hWnd, CB_SETHORIZONTALEXTENT, nExtent, 0);
	}

	UINT GetHorizontalExtent() const {
		return (UINT)::SendMessage(m_hWnd, CB_GETHORIZONTALEXTENT, 0, 0);
	}

	int SetDroppedWidth(UINT nWidth) {
		return (int)::SendMessage(m_hWnd, CB_SETDROPPEDWIDTH, nWidth, 0); 
	}

	int GetDroppedWidth() const {
		return (int)::SendMessage(m_hWnd, CB_GETDROPPEDWIDTH, 0, 0); 
	}

	
	// for edit control
	DWORD GetEditSel() const {
		return ::SendMessage(m_hWnd, CB_GETEDITSEL, 0, 0); 
	}

	BOOL LimitText(int nMaxChars) {
		return (BOOL)::SendMessage(m_hWnd, CB_LIMITTEXT, nMaxChars, 0);
	}

	BOOL SetEditSel(int nStartChar, int nEndChar) {
		return (BOOL)::SendMessage(m_hWnd, CB_SETEDITSEL, 0, MAKELONG(nStartChar, nEndChar));
	}

	
	// for combobox item
	DWORD GetItemData(int nIndex) const {
		return ::SendMessage(m_hWnd, CB_GETITEMDATA, nIndex, 0);
	}

	int SetItemData(int nIndex, DWORD dwItemData) {
		return (int)::SendMessage(m_hWnd, CB_SETITEMDATA, nIndex, (LPARAM)dwItemData);
	}

	void* GetItemDataPtr(int nIndex) const {
		return (LPVOID)GetItemData(nIndex); 
	}

	int SetItemDataPtr(int nIndex, void* pData) {
		return SetItemData(nIndex, (DWORD)(LPVOID)pData); 
	}

	int GetLBText(int nIndex, LPTSTR lpszText) const {
		return (int)::SendMessage(m_hWnd, CB_GETLBTEXT, nIndex, (LPARAM)lpszText); 
	}

	void GetLBText(int nIndex, CString& rString) const {
		ASSERT(::IsWindow(m_hWnd));
		GetLBText(nIndex, rString.GetBufferSetLength(GetLBTextLen(nIndex)));
		rString.ReleaseBuffer();
	}

	int GetLBTextLen(int nIndex) const {
		return (int)::SendMessage(m_hWnd, CB_GETLBTEXTLEN, nIndex, 0); 
	}

	int SetItemHeight(int nIndex, UINT cyItemHeight) {
		return (int)::SendMessage(m_hWnd, CB_SETITEMHEIGHT, nIndex, MAKELONG(cyItemHeight, 0)); 
	}

	int GetItemHeight(int nIndex) const {
		return (int)::SendMessage(m_hWnd, CB_GETITEMHEIGHT, nIndex, 0L); 
	}

	int FindStringExact(int nIndexStart, LPCTSTR lpszFind) const {
		return (int)::SendMessage(m_hWnd, CB_FINDSTRINGEXACT, nIndexStart, (LPARAM)lpszFind); 
	}

	int SetExtendedUI(BOOL bExtended = TRUE) {
		return (int)::SendMessage(m_hWnd, CB_SETEXTENDEDUI, bExtended, 0L); 
	}

	BOOL GetExtendedUI() const {
		return (BOOL)::SendMessage(m_hWnd, CB_GETEXTENDEDUI, 0, 0L); 
	}

	void GetDroppedControlRect(LPRECT lprect) const {
		::SendMessage(m_hWnd, CB_GETDROPPEDCONTROLRECT, 0, (DWORD)lprect); 
	}

	BOOL GetDroppedState() const {
		return (BOOL)::SendMessage(m_hWnd, CB_GETDROPPEDSTATE, 0, 0L);
	}


// Operations
	// for drop-down combo boxes
	void ShowDropDown(BOOL bShowIt = TRUE) {
		::SendMessage(m_hWnd, CB_SHOWDROPDOWN, bShowIt, 0); 
	}

	// manipulating listbox items
	int AddStringInternal(LPCTSTR lpszString, int index = 0) {
		return (int)::SendMessage(m_hWnd, CB_ADDSTRING, 0, (LPARAM)lpszString);
	}

	int DeleteString(UINT nIndex) {
		return (int)::SendMessage(m_hWnd, CB_DELETESTRING, nIndex, 0);
	}
	int InsertString(int nIndex, LPCTSTR lpszString) {
		return (int)::SendMessage(m_hWnd, CB_INSERTSTRING, nIndex, (LPARAM)lpszString);
	}

	void ResetContent() {
		::SendMessage(m_hWnd, CB_RESETCONTENT, 0, 0);
	}

	int Dir(UINT attr, LPCTSTR lpszWildCard) {
		return (int)::SendMessage(m_hWnd, CB_DIR, attr, (LPARAM)lpszWildCard);
	}
	
	// selection helpers
	int FindString(int nStartAfter, LPCTSTR lpszString) const {
		return (int)::SendMessage(m_hWnd, CB_FINDSTRING, nStartAfter, (LPARAM)lpszString); 
	}

	int SelectString(int nStartAfter, LPCTSTR lpszString) {
		return (int)::SendMessage(m_hWnd, CB_SELECTSTRING, nStartAfter, (LPARAM)lpszString); 	
	}

	// Clipboard operations
	void Clear() {
		::SendMessage(m_hWnd, WM_CLEAR, 0, 0); 
	}

	void Copy() {
		::SendMessage(m_hWnd, WM_COPY, 0, 0);
	}

	void Cut() {
		::SendMessage(m_hWnd, WM_CUT, 0, 0);
	}

	void Paste() {
		::SendMessage(m_hWnd, WM_PASTE, 0, 0);
	}

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRADIENTCOMBOBOXCTL_H__F228F4AB_A178_4DA0_9040_E8E903E7F7C6__INCLUDED)
