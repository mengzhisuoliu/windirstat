// TreeMapView.h - Declaration of CTreeMapView (the Treemap view)
//
// WinDirStat - Directory Statistics
// Copyright (C) 2003-2005 Bernhard Seifert
// Copyright (C) 2004-2024 WinDirStat Team (windirstat.net)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#pragma once

#include "TreeMap.h"

class CDirStatDoc;
class CItem;

//
// CTreeMapView. The treemap window.
//
class CTreeMapView final : public CView
{
protected:
    CTreeMapView() = default;
    DECLARE_DYNCREATE(CTreeMapView)

    ~CTreeMapView() override = default;

    // CTreeMap::Callback
    CDirStatDoc* GetDocument() const
    {
        return reinterpret_cast<CDirStatDoc*>(m_pDocument);
    }

    void SuspendRecalculationDrawing(bool suspend);
    bool IsShowTreeMap() const;
    void ShowTreeMap(bool show);
    void DrawEmptyView();

protected:
    BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;
    void OnDraw(CDC* pDC) override;
    bool IsDrawn() const;
    void Inactivate();
    void EmptyView();
    void DrawEmptyView(CDC* pDC);

    void DrawZoomFrame(CDC* pdc, CRect& rc);
    void DrawHighlights(CDC* pdc);

    void DrawHighlightExtension(CDC* pdc);
    void RecurseHighlightExtension(CDC* pdc, const CItem* item);

    void DrawSelection(CDC* pdc);

    void HighlightSelectedItem(CDC* pdc, const CItem* item, bool single);
    void RenderHighlightRectangle(CDC* pdc, CRect& rc);

    bool m_DrawingSuspended = false; // True while the user is resizing the window.
    bool m_ShowTreeMap = true;       // False, if the user switched off the treemap (by F9).
    CSize m_Size{ 0, 0 };            // Current size of view
    CTreeMap m_TreeMap;              // Treemap generator
    CBitmap m_Bitmap;                // Cached view. If m_hObject is NULL, the view must be recalculated.
    CSize m_DimmedSize{ 0,0 };       // Size of bitmap m_Dimmed
    CBitmap m_Dimmed;                // Dimmed view. Used during refresh to avoid the ooops-effect.
    UINT_PTR m_Timer = 0;            // We need a timer to realize when the mouse left our window.

    DECLARE_MESSAGE_MAP()
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnDestroy();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
};


