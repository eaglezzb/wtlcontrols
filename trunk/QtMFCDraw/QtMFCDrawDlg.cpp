
// QtMFCDrawDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "QtMFCDraw.h"
#include "QtMFCDrawDlg.h"
#include "qhdcimage.h"
#include <QtGui/QPainter>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CQtMFCDrawDlg 对话框




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


// CQtMFCDrawDlg 消息处理程序

BOOL CQtMFCDrawDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CQtMFCDrawDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CQtMFCDrawDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CQtMFCDrawDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
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
