// GradientFrameCtrl.h : Declaration of the CGradientFrameCtrl



#ifndef __GRADIENTFRAMECTRL_H_
#define __GRADIENTFRAMECTRL_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include <atlapp.h>
#include <atlgdi.h>

#include "TimeCounter.h"

#ifndef ULONG_PTR
#define ULONG_PTR unsigned long*
#endif

#include "gdiplus.h"
#include "initgdiplus.h"

using namespace Gdiplus;

static InitGDIPlus GDI_Plus_Controler;

/////////////////////////////////////////////////////////////////////////////
// CGradientFrameCtrl
class ATL_NO_VTABLE CGradientFrameCtrl : 
public CComObjectRootEx<CComSingleThreadModel>,
public CStockPropImpl<CGradientFrameCtrl, IGradientFrameCtrl, &IID_IGradientFrameCtrl, &LIBID_GRADIENTFRAMELib>,
public CComControl<CGradientFrameCtrl>,
public IPersistStreamInitImpl<CGradientFrameCtrl>,
public IOleControlImpl<CGradientFrameCtrl>,
public IOleObjectImpl<CGradientFrameCtrl>,
public IOleInPlaceActiveObjectImpl<CGradientFrameCtrl>,
public IViewObjectExImpl<CGradientFrameCtrl>,
public IOleInPlaceObjectWindowlessImpl<CGradientFrameCtrl>,
public IPersistStorageImpl<CGradientFrameCtrl>,
public ISpecifyPropertyPagesImpl<CGradientFrameCtrl>,
public IQuickActivateImpl<CGradientFrameCtrl>,
public IDataObjectImpl<CGradientFrameCtrl>,
public IProvideClassInfo2Impl<&CLSID_GradientFrameCtrl, NULL, &LIBID_GRADIENTFRAMELib>,
public CComCoClass<CGradientFrameCtrl, &CLSID_GradientFrameCtrl>
{
public:
	CContainedWindow m_ctlStatic;
	
	
	CGradientFrameCtrl() :	
		m_ctlStatic(_T("Static"), this, 1), 
		m_clrBackColor(16777215), m_clrBorderColor(RGB(255,0,0)), m_clrForeColor(12999969),
		m_pMemBitmap(NULL), m_pCachedBitmap(NULL), m_pMemGraphics(NULL),
		m_nBorderWidth(2), m_bBorderVisible(FALSE), m_radius(40)
	{
		GDI_Plus_Controler.Initialize(); //GDI_Plus_Controler is a static global
		m_bWindowOnly = TRUE;
	}
	
	~CGradientFrameCtrl()
	{
		if ( m_pMemGraphics ) {
            delete m_pMemGraphics;
			m_pMemGraphics = NULL;
		}
		
        if ( m_pMemBitmap ) {
            delete m_pMemBitmap;
			m_pMemBitmap = NULL;
		}
		
        if ( m_pCachedBitmap ) {
            delete m_pCachedBitmap;
			m_pCachedBitmap = NULL;
		}
		
		GDI_Plus_Controler.Deinitialize(); //GDI_Plus_Controler is a static global
	}
	
	DECLARE_REGISTRY_RESOURCEID(IDR_GRADIENTFRAMECTRL)
		
		DECLARE_PROTECT_FINAL_CONSTRUCT()
		
		BEGIN_COM_MAP(CGradientFrameCtrl)
		COM_INTERFACE_ENTRY(IGradientFrameCtrl)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(IViewObjectEx)
		COM_INTERFACE_ENTRY(IViewObject2)
		COM_INTERFACE_ENTRY(IViewObject)
		COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
		COM_INTERFACE_ENTRY(IOleInPlaceObject)
		COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
		COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
		COM_INTERFACE_ENTRY(IOleControl)
		COM_INTERFACE_ENTRY(IOleObject)
		COM_INTERFACE_ENTRY(IPersistStreamInit)
		COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
		COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
		COM_INTERFACE_ENTRY(IQuickActivate)
		COM_INTERFACE_ENTRY(IPersistStorage)
		COM_INTERFACE_ENTRY(IDataObject)
		COM_INTERFACE_ENTRY(IProvideClassInfo)
		COM_INTERFACE_ENTRY(IProvideClassInfo2)
		END_COM_MAP()
		
		BEGIN_PROP_MAP(CGradientFrameCtrl)
		PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
		PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
		PROP_ENTRY("BackColor", DISPID_BACKCOLOR, CLSID_StockColorPage)
		PROP_ENTRY("BorderColor", DISPID_BORDERCOLOR, CLSID_StockColorPage)
		PROP_ENTRY("BorderVisible", DISPID_BORDERVISIBLE, CLSID_NULL)
		PROP_ENTRY("BorderWidth", DISPID_BORDERWIDTH, CLSID_NULL)
		PROP_ENTRY("ForeColor", DISPID_FORECOLOR, CLSID_StockColorPage)
		// Example entries
		// PROP_ENTRY("Property Description", dispid, clsid)
		// PROP_PAGE(CLSID_StockColorPage)
		END_PROP_MAP()
		
		BEGIN_MSG_MAP(CGradientFrameCtrl)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		//MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		CHAIN_MSG_MAP(CComControl<CGradientFrameCtrl>)
		ALT_MSG_MAP(1)
		// Replace this with message map entries for superclassed Static
		END_MSG_MAP()
		// Handler prototypes:
		//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
		//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
		
		
	LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		LRESULT lRes = CComControl<CGradientFrameCtrl>::OnSetFocus(uMsg, wParam, lParam, bHandled);
		if (m_bInPlaceActive)
		{
			DoVerbUIActivate(&m_rcPos,  NULL);
			if(!IsChild(::GetFocus()))
				m_ctlStatic.SetFocus();
		}
		return lRes;
	}
	
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		RECT rc;
		GetWindowRect(&rc);
		rc.right -= rc.left;
		rc.bottom -= rc.top;
		rc.top = rc.left = 0;
		m_ctlStatic.Create(m_hWnd, rc);
		return 0;
	}
	
	
	LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		//CDCHandle dc = (HDC)wParam;
		return 0;
	}
	
	LRESULT OnDraw(ATL_DRAWINFO& )
	{
		#ifdef _DEBUG
		SHOW_TIME_START(t, _T("OnPaint"));		
		//测试时间
		//SHOW_TIME_START(t1, _T("Prepare off-screen bitmap"));
		#endif

		CPaintDC dc(m_ctlStatic.m_hWnd);
		RECT rect;
		GetClientRect(&rect);
		int nWidth = rect.right - rect.left + 1;
		int nHeight = rect.bottom - rect.top + 1;
		//
	
		CDCHandle MemDC; //首先定义一个显示设备对象
		CBitmap MemBitmap;//定义一个位图对象
		//随后建立与屏幕显示兼容的内存显示设备
		MemDC.CreateCompatibleDC(dc.m_hDC);
		//这时还不能绘图，因为没有地方画 ^_^
		//下面建立一个与屏幕显示兼容的位图，至于位图的大小嘛，可以用窗口的大小
		MemBitmap.CreateCompatibleBitmap(dc.m_hDC, nWidth, nHeight);

		//将位图选入到内存显示设备中
		//只有选入了位图的内存显示设备才有地方绘图，画到指定的位图上
		CBitmapHandle OldBit;
		OldBit.m_hBitmap = MemDC.SelectBitmap (MemBitmap.m_hBitmap);
		//先用背景色将位图清除干净，这里我用的是白色作为背景
		//你也可以用自己应该用的颜色
		MemDC.FillSolidRect(0,0,nWidth,nHeight, m_clrBackColor);

		//绘图

		CBrushHandle brush;
		brush.CreateSolidBrush(m_clrForeColor);	
		CRgnHandle rgn;
		rgn.CreateRoundRectRgn(0, 0, nWidth, nHeight, m_radius, m_radius);
		MemDC.FillRgn(rgn.m_hRgn, brush);

		if (m_bBorderVisible)
		{
			HPEN pen = ::CreatePen(m_nBorderStyle, m_nBorderWidth, m_clrBorderColor);
			HPEN oldPen = MemDC.SelectPen(pen);
			LOGBRUSH logBrush;
			logBrush.lbStyle = BS_SOLID;
			logBrush.lbColor = m_clrBorderColor;
			logBrush.lbHatch = 0;
			CBrushHandle rgnBrush;
			rgnBrush.CreateBrushIndirect(&logBrush);
			BOOL framed = MemDC.FrameRgn(rgn.m_hRgn, rgnBrush, m_nBorderWidth, m_nBorderWidth);
			MemDC.SelectPen(oldPen);
		}

		
		//MemDC.FillRect(&rect, brush.m_hBrush);
		//将内存中的图拷贝到屏幕上进行显示
		BOOL result = dc.BitBlt(0,0,nWidth,nHeight,MemDC.m_hDC,0,0,SRCCOPY);
		//绘图完成后的清理
		brush.DeleteObject();
		MemBitmap.DeleteObject();
		MemDC.DeleteDC(); 

// 		SHOW_TIME_START(t1, _T("Prepare off-screen bitmap"));
// 		HRGN hRgn = ::CreateRoundRectRgn(0, 0, nWidth, nHeight, m_radius, m_radius);
// 		::SelectClipRgn(dc.m_hDC, hRgn);
// 		CBrush brush;
// 		brush.CreateSolidBrush(m_clrForeColor);		
// 		::FillRect(dc.m_hDC, &rect, brush.m_hBrush);
// 		SHOW_TIME_END(t1);
// 		Graphics graphics(dc.m_hDC);		
// 		if ( ! m_pMemBitmap )
// 		{
// 			SHOW_TIME_START(t1, _T("Prepare off-screen bitmap"));
// 			
// 			CreateOffScreeenGraphics(nWidth, nHeight, &graphics);
// 			
// 			SHOW_TIME_END(t1);
// 		}
// 		
// 		// draw from cached bitmap to window
// 		if ( graphics.DrawCachedBitmap(m_pCachedBitmap, 0, 0) != Ok )
// 		{
// 			// make bitmap again (display parameters are changed)
// 			SetDirty();
// 			CreateOffScreeenGraphics(nWidth, nHeight, &graphics);
// 			graphics.DrawCachedBitmap(m_pCachedBitmap, 0, 0);
// 		}
// 		
		#ifdef _DEBUG
			SHOW_TIME_END(t);
		#endif


		return 0;
	}

// 	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
// 	{		
// 		return 0;		
// 	}
// 	
	
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		//UINT nType = (UINT)wParam;
		//CSize size = _WTYPES_NS::CSize(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		Invalidate();
        SetDirty();
		return 0;
	}
	
	STDMETHOD(SetObjectRects)(LPCRECT prcPos,LPCRECT prcClip)
	{
		IOleInPlaceObjectWindowlessImpl<CGradientFrameCtrl>::SetObjectRects(prcPos, prcClip);
		int cx, cy;
		cx = prcPos->right - prcPos->left;
		cy = prcPos->bottom - prcPos->top;
		::SetWindowPos(m_ctlStatic.m_hWnd, NULL, 0,
			0, cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);
		return S_OK;
	}
	
	// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)
		
		private:
			// create off-screen graphics and draw to it
			void CreateOffScreeenGraphics(int nWidth, int nHeight, Graphics* pGraphics)
			{
				
				// Create off-screen bitmap
				m_pMemBitmap =  new Bitmap(nWidth, nHeight);
				
				// Create off-screen graphics
				m_pMemGraphics = Graphics::FromImage(m_pMemBitmap);
				m_pMemGraphics->SetSmoothingMode(SmoothingModeAntiAlias);
				// draw to off-screen graphics
				//Draw(m_pMemGraphics, nWidth, nHeight);
				DrawRoundedRectangle(m_pMemGraphics, nWidth, nHeight, m_radius);
				
				// Create cashed bitmap
				m_pCachedBitmap = new CachedBitmap(m_pMemBitmap, pGraphics);
				
			}

			//COLORREF to GDI+ Color
			inline Color COLORREF2Color(COLORREF color)
			{
				UCHAR iRed = GetRValue(color);
				UCHAR iBlue = GetBValue(color);
				UCHAR iGreen = GetGValue(color);
				return Color(iRed, iGreen, iBlue);
			}

			//draw rounded rectangle
			void DrawRoundedRectangle(Graphics* pGraphics, int wid, int hei, int radius)
			{
				GraphicsPath path;
				path.StartFigure();
				
				REAL width = wid - 1.5;
				REAL height = hei - 1.0;
				Rect r(0, 0, width, height);
				int dia = radius;
				// diameter can't exceed width or height
				if(dia > r.Width)    dia = r.Width;
				if(dia > r.Height)    dia = r.Height;
				
				// define a corner 
				Rect Corner(r.X, r.Y, dia, dia);
				
				// top left
				path.AddArc(Corner, 180, 90);    				
				// tweak needed for radius of 10 (dia of 20)
				if(dia == 20)
				{
					Corner.Width += 1; 
					Corner.Height += 1; 
					r.Width -=1; r.Height -= 1;
				}
				
				// top right
				Corner.X += (r.Width - dia - 1);
				path.AddArc(Corner, 270, 90);    
				
				// bottom right
				Corner.Y += (r.Height - dia - 1);
				path.AddArc(Corner,   0, 90);    
				
				// bottom left
				Corner.X -= (r.Width - dia - 1);
				path.AddArc(Corner,  90, 90);

				path.CloseFigure();				
					
				SolidBrush solidBrush(COLORREF2Color(m_clrBackColor));
				pGraphics->FillRectangle(&solidBrush, 0.0, 0.0, width, height);		
				solidBrush.SetColor(COLORREF2Color(m_clrForeColor));
				pGraphics->FillPath(&solidBrush, &path);

				//COLORREF fore = 12999969;//12999969Color(236, 236, 246)
				if (m_bBorderVisible)
				{
					Pen pen(COLORREF2Color(m_clrBorderColor), m_nBorderWidth);
					pGraphics->DrawPath(&pen, &path);
				}

			}

			// draw contet to Graphics
			void Draw(Graphics* pGraphics, int width, int height)
			{
				// fill background
								
				SolidBrush solidBrush(Color(0, 255, 255));
				pGraphics->FillRectangle(&solidBrush, 0, 0, width, height);		

				
				LinearGradientBrush blueGradient(Point(1,1), Point(width-2, height-2), Color(0, 0, 0, 255), Color(192, 0, 0, 255));
				
				// Create path for the ellipse
				GraphicsPath gp;
				gp.StartFigure();
				gp.AddEllipse(2,2, width -4, height- 4);
				
				// Set up a radial gradient brush
				PathGradientBrush whiteGradientHighlight(&gp);
				whiteGradientHighlight.SetCenterColor(Color(255, 255, 255, 255));
				//whiteGradientHighlight.SetCenterPoint(Point(m_center.x, m_center.y)); //m_center is manipulated by the mouse handlers
				whiteGradientHighlight.SetFocusScales(0.1f, 0.1f);
				Color surroundColors[] = {Color(0, 255, 255, 255)};
				int surroundColorsCount = 1;
				whiteGradientHighlight.SetSurroundColors(surroundColors, &surroundColorsCount);
				// Done Setting up radial gradient brush
				
				pGraphics->SetSmoothingMode(SmoothingModeAntiAlias);
				
				pGraphics->FillPath(&blueGradient, &gp);
				pGraphics->FillPath(&whiteGradientHighlight, &gp);
				
				// 		if(m_border)
				{
					Pen pen(Color(255,0,0,0), 2);
					pGraphics->DrawPath(&pen, &gp);
				}				
			}
			
			// cause to create offscreen graphics when window will be redrawn
			void SetDirty()
			{
				if ( m_pMemGraphics )
				{
					delete m_pMemGraphics;
					m_pMemGraphics = NULL;
				}
				
				if ( m_pMemBitmap )
				{
					delete m_pMemBitmap;
					m_pMemBitmap = NULL;
				}
				
				if ( m_pCachedBitmap )
				{
					delete m_pCachedBitmap;
					m_pCachedBitmap = NULL;
				}
			}
			
			private:
				Bitmap* m_pMemBitmap;
				CachedBitmap* m_pCachedBitmap;
				Graphics* m_pMemGraphics;
				
				// IGradientFrameCtrl
			public:
				OLE_COLOR m_clrBackColor;
				OLE_COLOR m_clrBorderColor;
				LONG m_nBorderStyle;
				BOOL m_bBorderVisible;
				LONG m_nBorderWidth;
				OLE_COLOR m_clrForeColor;
				LONG m_radius;

			public:
				STDMETHOD(Refresh)();

				STDMETHOD(get_Radius)(/*[out, retval]*/ long *pVal);
				STDMETHOD(put_Radius)(/*[in]*/ long newVal);
};

#endif //__GRADIENTFRAMECTRL_H_
