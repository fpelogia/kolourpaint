
/* This file is part of the KolourPaint project
   Copyright (c) 2003 Clarence Dang <dang@kde.org>
   All rights reserved.
   
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. Neither the names of the copyright holders nor the names of
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define DEBUG_KP_VIEW_MANAGER 1

#include <kdebug.h>

#include <kpdefs.h>
#include <kpdocument.h>
#include <kpmainwindow.h>
#include <kpview.h>
#include <kpviewmanager.h>


kpViewManager::kpViewManager (kpMainWindow *mainWindow)
    : m_mainWindow (mainWindow),
      m_viewUnderCursor (0),
      m_tempPixmapType (NoPixmap)
{
}

kpDocument *kpViewManager::document ()
{
    return m_mainWindow ? m_mainWindow->document () : 0;
}

kpViewManager::~kpViewManager ()
{
    unregisterAllViews ();
}


void kpViewManager::registerView (kpView *view)
{
#if DEBUG_KP_VIEW_MANAGER && 0
    kdDebug () << "kpViewManager::registerView (" << view << ")" << endl;
#endif
    if (view && m_views.findRef (view) < 0)
    {
    #if DEBUG_KP_VIEW_MANAGER && 0
        kdDebug () << "\tadded view" << endl;
    #endif
        m_views.append (view);
    }
    else
    {
    #if DEBUG_KP_VIEW_MANAGER && 0
        kdDebug () << "\tignored register view attempt" << endl;
    #endif
    }
}

void kpViewManager::unregisterView (kpView *view)
{
    if (view)
    {
        m_views.removeRef (view);
    }
}

void kpViewManager::unregisterAllViews ()
{
    // no autoDelete
    m_views.clear ();
}


void kpViewManager::setSelectionBorderType (enum kpViewManager::SelectionBorderType sb, bool update)
{
    m_selectionBorder = sb;
    if (selectionActive () && update)
        updateViews (m_tempPixmapRect);
}

enum kpViewManager::SelectionBorderType kpViewManager::selectionBorderType () const
{
    return m_selectionBorder;
}

void kpViewManager::setTempPixmapAt (const QPixmap &pixmap, const QPoint &at,
                                     enum TempPixmapType type,
                                     enum SelectionBorderType selBorderType)
{
#if DEBUG_KP_VIEW_MANAGER && 0
    kdDebug () << "kpViewManager::setTempPixmapAt (pixmap (w="
               << pixmap.width ()
               << ",h=" << pixmap.height ()
               << "), x=" << at.x ()
               << ",y=" << at.y ()
               << endl;
#endif

    bool oldPixmapActive = tempPixmapActive ();
    QRect oldPixmapRect = m_tempPixmapRect;  // only valid if oldPixmapActive
    bool tempPixmapWasSelection = selectionActive ();

    m_tempPixmap = pixmap;
    m_tempPixmapRect = QRect (at.x (), at.y (), pixmap.width (), pixmap.height ());

    // OPT: don't need bounding rect - could just update 2 rects
    QRect updateRect;
    if (oldPixmapActive)
        updateRect = m_tempPixmapRect.unite (oldPixmapRect);
    else
        updateRect = m_tempPixmapRect;

    m_tempPixmapType = type;
    setSelectionBorderType (selBorderType, false/*don't update*/);

    updateViews (updateRect);

    if (tempPixmapWasSelection && !selectionActive ())
        emit selectionEnabled (false);
    else if (!tempPixmapWasSelection && selectionActive ())
        emit selectionEnabled (true);
}

void kpViewManager::invalidateTempPixmap (const bool doUpdate)
{
    if (!tempPixmapActive ()) return;

    bool tempPixmapWasSelection = selectionActive ();
    m_tempPixmapType = NoPixmap;

    if (doUpdate)
        updateViews (m_tempPixmapRect);

    if (tempPixmapWasSelection)
        emit selectionEnabled (false);
}

enum kpViewManager::TempPixmapType kpViewManager::tempPixmapType () /*const*/
{
    return m_tempPixmapType;
}

bool kpViewManager::tempPixmapActive () /*const*/
{
    return tempPixmapType () != NoPixmap;
}

bool kpViewManager::normalActive () /*const*/
{
    return tempPixmapType () == NormalPixmap;
}

bool kpViewManager::selectionActive () /*const*/
{
    return tempPixmapType () == SelectionPixmap;
}

bool kpViewManager::brushActive () /*const*/
{
    return tempPixmapType () == BrushPixmap;
}

QRect kpViewManager::tempPixmapRect () const
{
    return m_tempPixmapRect;
}

QPixmap kpViewManager::tempPixmap () const
{
    return m_tempPixmap;
}


void kpViewManager::setCursor (const QCursor &cursor)
{
    for (kpView *view = m_views.first (); m_views.current (); view = m_views.next ())
    {
        view->setCursor (cursor);
    }
}

void kpViewManager::unsetCursor ()
{
    for (kpView *view = m_views.first (); m_views.current (); view = m_views.next ())
    {
        view->unsetCursor ();
    }
}


kpView *kpViewManager::viewUnderCursor () /*const*/
{
    if (m_viewUnderCursor && m_views.findRef (m_viewUnderCursor) < 0)
    {
        kdError () << "kpViewManager::viewUnderCursor(): invalid view" << endl;
        m_viewUnderCursor = 0;
    }
        
    return m_viewUnderCursor;
}

void kpViewManager::setViewUnderCursor (kpView *view)
{
#if DEBUG_KP_VIEW_MANAGER && 0
    kdDebug () << "kpViewManager::setViewUnderCursor (" << view << ")" << endl;
#endif
    m_viewUnderCursor = view;

    repaintBrushPixmap ();
}


void kpViewManager::repaintBrushPixmap ()
{
#if DEBUG_KP_VIEW_MANAGER && 0
    kdDebug () << "kpViewManager::repaintBrushPixmap () viewUnderCursor="
               << viewUnderCursor () << endl;
#endif

    // sync with updateViews()
    for (kpView *view = m_views.first (); m_views.current (); view = m_views.next ())
    {
        view->repaint (view->zoomDocToView (tempPixmapRect ()), false /* don't erase */);
    }
}


void kpViewManager::updateViews ()
{
    kpDocument *doc = document ();
    if (doc)
        updateViews (QRect (0, 0, doc->width (), doc->height ()));
}

void kpViewManager::updateViews (const QRect &docRect)
{
#if DEBUG_KP_VIEW_MANAGER && 0
    kdDebug () << "KpViewManager::updateViews (" << docRect << ")" << endl;
#endif

    // sync with repaintBrushPixmap()
    for (kpView *view = m_views.first (); m_views.current (); view = m_views.next ())
    {
        // HACK: use repaint to get instant feedback so that it _feels_
        //       more responsive
        if (view->zoomLevelX () % 100 == 0 && view->zoomLevelY () % 100 == 0)
        {
            view->repaint (view->zoomDocToView (docRect), false/*no erase*/);
        }
        else
        {
            QRect viewRect = view->zoomDocToView (docRect);
        
            int diff = qRound (double (QMAX (view->zoomLevelX (), view->zoomLevelY ())) / 100.0) + 1;
            
            QRect newRect = QRect (viewRect.x () - diff,
                                   viewRect.y () - diff,
                                   viewRect.width () + 2 * diff,
                                   viewRect.height () + 2 * diff)
                                .intersect (QRect (0, 0, view->width (), view->height ()));
        
            view->repaint (newRect, false/*no erase*/);
        }
    }
}

void kpViewManager::updateViews (int x, int y, int w, int h)
{
    updateViews (QRect (x, y, w, h));
}

void kpViewManager::resizeViews (int docWidth, int docHeight)
{
    for (kpView *view = m_views.first (); m_views.current (); view = m_views.next ())
    {
        if (view->hasVariableZoom ())
        {
            view->slotUpdateVariableZoom ();
        }
        else
        {
            view->resize (view->zoomDocToViewX (docWidth),
                          view->zoomDocToViewY (docHeight));
        }
    }
}

#include <kpviewmanager.moc>
