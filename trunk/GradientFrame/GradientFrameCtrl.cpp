// GradientFrameCtrl.cpp : Implementation of CGradientFrameCtrl

#include "stdafx.h"
#include "GradientFrame.h"
#include "GradientFrameCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CGradientFrameCtrl


STDMETHODIMP CGradientFrameCtrl::Refresh()
{
	// TODO: Add your implementation code here
	Invalidate();
	SetDirty();

	ATL_DRAWINFO drawInfo;
	OnDraw(drawInfo);

	return S_OK;
}

STDMETHODIMP CGradientFrameCtrl::get_Radius(long *pVal)
{
	// TODO: Add your implementation code here
	*pVal = m_radius;
	return S_OK;
}

STDMETHODIMP CGradientFrameCtrl::put_Radius(long newVal)
{
	// TODO: Add your implementation code here
	m_radius =  newVal;
	return S_OK;
}
