
// QtMFCDrawDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "QtMFCDraw.h"
#include "QtMFCDrawDlg.h"
#include "qhdcimage.h"
#include <QtGui/QPainter>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CQtMFCDrawDlg �Ի���




CQtMFCDrawDlg::CQtMFCDrawDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CQtMFCDrawDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CQtMFCDrawDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON1, m_button);
}

BEGIN_MESSAGE_MAP(CQtMFCDrawDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON1, &CQtMFCDrawDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CQtMFCDrawDlg ��Ϣ�������

BOOL CQtMFCDrawDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CQtMFCDrawDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CQtMFCDrawDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CQtMFCDrawDlg::OnBnClickedButton1()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CWnd * wnd = this->GetDlgItem(IDC_STATIC);
	HDC drawDC = wnd->GetDC()->GetSafeHdc();
	CRect rect;
	wnd->GetClientRect(rect);
	QHDCImage hdcimage(rect.Width(), rect.Height(), QHDCImage::systemFormat(), false, drawDC);
	QPainter painter(&hdcimage.image);

	painter.setRenderHint(QPainter::Antialiasing, true);

	QPen pen(Qt::red, 10, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin);
	painter.setPen(pen);
	painter.drawLine(QPoint(10,10), QPoint(50,50));

	QPainterPath path;
	path.moveTo(20, 80);
	path.lineTo(20, 30);
	path.cubicTo(80, 0, 50, 50, 80, 80);

	painter.drawPath(path);
	QLinearGradient gradient(0, 0, 0, 100);
	gradient.setColorAt(0.0, Qt::red);
	gradient.setColorAt(1.0, Qt::yellow);
	painter.setBrush(gradient);
	painter.drawPath(path);

	painter.translate(100, 0);
	path.closeSubpath();

	painter.drawPath(path);

	::BitBlt(drawDC, 0, 0, hdcimage.width(), hdcimage.height(),
		hdcimage.hdc, 0, 0, SRCCOPY);

}
