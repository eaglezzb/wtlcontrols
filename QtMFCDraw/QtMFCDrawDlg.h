
// QtMFCDrawDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// CQtMFCDrawDlg �Ի���
class CQtMFCDrawDlg : public CDialog
{
// ����
public:
	CQtMFCDrawDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_QTMFCDRAW_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CButton m_button;
	afx_msg void OnBnClickedButton1();
};
