
/*
   Copyright (c) 2003-2007 Clarence Dang <dang@kde.org>
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
   IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#define DEBUG_KP_VIEW_MANAGER 0


#include <kpViewManager.h>

#include <qapplication.h>
#include <qlist.h>
#include <qtimer.h>

#include <kdebug.h>

#include <kpDefs.h>
#include <kpDocument.h>
#include <kpMainWindow.h>
#include <kpSelection.h>
#include <kpTempPixmap.h>
#include <kpTool.h>
#include <kpView.h>


kpViewManager::kpViewManager (kpMainWindow *mainWindow)
    : m_textCursorBlinkTimer (0),
      m_textCursorRow (-1),
      m_textCursorCol (-1),
      m_textCursorBlinkState (true),
      m_mainWindow (mainWindow),
      m_tempPixmap (0),
      m_viewUnderCursor (0),
      m_selectionBorderVisible (false),
      m_selectionBorderFinished (false)
{
    m_queueUpdatesCounter = m_fastUpdatesCounter =  0;
}

// private
kpDocument *kpViewManager::document () const
{
    return m_mainWindow ? m_mainWindow->document () : 0;
}

kpViewManager::~kpViewManager ()
{
    unregisterAllViews ();

    delete m_tempPixmap; m_tempPixmap = 0;
}


void kpViewManager::registerView (kpView *view)
{
#if DEBUG_KP_VIEW_MANAGER && 1
    kDebug () << "kpViewManager::registerView (" << view << ")" << endl;
#endif
    if (view && !m_views.contains (view))
    {
    #if DEBUG_KP_VIEW_MANAGER && 1
        kDebug () << "\tadded view" << endl;
    #endif
        view->setCursor (m_cursor);
        m_views.append (view);
    }
    else
    {
    #if DEBUG_KP_VIEW_MANAGER && 1
        kDebug () << "\tignored register view attempt" << endl;
    #endif
    }
}

void kpViewManager::unregisterView (kpView *view)
{
    if (view)
    {
        if (view == m_viewUnderCursor)
            m_viewUnderCursor = 0;

        view->unsetCursor ();
        m_views.removeAll (view);
    }
}

void kpViewManager::unregisterAllViews ()
{
    // no autoDelete
    m_views.clear ();
}


// public
const kpTempPixmap *kpViewManager::tempPixmap () const
{
    return m_tempPixmap;
}

// public
void kpViewManager::setTempPixmap (const kpTempPixmap &tempPixmap)
{
#if DEBUG_KP_VIEW_MANAGER
    kDebug () << "kpViewManager::setTempPixmap(isBrush="
               << tempPixmap.isBrush ()
               << ",topLeft=" << tempPixmap.topLeft ()
               << ",pixmap.rect=" << tempPixmap.pixmap ().rect ()
               << ")" << endl;
#endif

    QRect oldRect;

    if (m_tempPixmap)
    {
        oldRect = m_tempPixmap->rect ();
        delete m_tempPixmap;
        m_tempPixmap = 0;
    }

    m_tempPixmap = new kpTempPixmap (tempPixmap);


    setQueueUpdates ();

    if (oldRect.isValid ())
        updateViews (oldRect);
    updateViews (m_tempPixmap->rect ());

    restoreQueueUpdates ();
}

// public
void kpViewManager::invalidateTempPixmap ()
{
    if (!m_tempPixmap)
        return;

    QRect oldRect = m_tempPixmap->rect ();

    delete m_tempPixmap;
    m_tempPixmap = 0;

    updateViews (oldRect);
}


// public
bool kpViewManager::selectionBorderVisible () const
{
    return m_selectionBorderVisible;
}

// public
void kpViewManager::setSelectionBorderVisible (bool yes)
{
    if (m_selectionBorderVisible == yes)
        return;

    m_selectionBorderVisible = yes;

    if (document () && document ()->selection ())
        updateViews (document ()->selection ()->boundingRect ());
}


// public
bool kpViewManager::selectionBorderFinished () const
{
    return m_selectionBorderFinished;
}

// public
void kpViewManager::setSelectionBorderFinished (bool yes)
{
    if (m_selectionBorderFinished == yes)
        return;

    m_selectionBorderFinished = yes;

    if (document () && document ()->selection ())
        updateViews (document ()->selection ()->boundingRect ());
}


bool kpViewManager::textCursorEnabled () const
{
    return (bool) m_textCursorBlinkTimer;
}

void kpViewManager::setTextCursorEnabled (bool yes)
{
#if DEBUG_KP_VIEW_MANAGER && 1
    kDebug () << "kpViewManager::setTextCursorEnabled(" << yes << ")" << endl;
#endif

    if (yes == textCursorEnabled ())
        return;

    delete m_textCursorBlinkTimer;
    m_textCursorBlinkTimer = 0;

    setFastUpdates ();
    setQueueUpdates ();

    m_textCursorBlinkState = true;

    if (yes)
    {
        m_textCursorBlinkTimer = new QTimer (this);
        m_textCursorBlinkTimer->setSingleShot (true);
        connect (m_textCursorBlinkTimer, SIGNAL (timeout ()),
                 this, SLOT (slotTextCursorBlink ()));
        slotTextCursorBlink ();
    }

    restoreQueueUpdates ();
    restoreFastUpdates ();
}


int kpViewManager::textCursorRow () const
{
    bool handledErrors = false;
    if (m_mainWindow)
    {
        kpDocument *doc = m_mainWindow->document ();
        if (doc)
        {
            kpSelection *sel = doc->selection ();
            if (sel && sel->isText ())
            {
                if (m_textCursorRow >= (int) sel->textLines ().size ())
                {
                #if DEBUG_KP_VIEW_MANAGER && 1
                    kDebug () << "kpViewManager::textCursorRow() row="
                               << m_textCursorRow
                               << endl;
                #endif
                    (const_cast <kpViewManager *> (this))->m_textCursorRow =
                        sel->textLines ().size () - 1;
                }

                handledErrors = true;
            }
        }
    }

    if (!handledErrors)
    {
    #if DEBUG_KP_VIEW_MANAGER && 1
        kDebug () << "kpViewManager::textCursorRow() no mw, doc or text sel" << endl;
    #endif
        (const_cast <kpViewManager *> (this))->m_textCursorRow = -1;
    }

    return m_textCursorRow;
}

int kpViewManager::textCursorCol () const
{
    int row = textCursorRow ();
    if (row < 0)
        return -1;

    bool handledErrors = false;
    if (m_mainWindow)
    {
        kpDocument *doc = m_mainWindow->document ();
        if (doc)
        {
            kpSelection *sel = doc->selection ();
            if (sel && sel->isText ())
            {
                if (m_textCursorCol > (int) sel->textLines () [row].length ())
                {
                #if DEBUG_KP_VIEW_MANAGER && 1
                    kDebug () << "kpViewManager::textCursorRow() col="
                               << m_textCursorCol
                               << endl;
                #endif
                    (const_cast <kpViewManager *> (this))->m_textCursorCol =
                        sel->textLines () [row].length ();
                }

                handledErrors = true;
            }
        }
    }

    if (!handledErrors)
    {
    #if DEBUG_KP_VIEW_MANAGER && 1
        kDebug () << "kpViewManager::textCursorCol() no mw, doc or text sel" << endl;
    #endif
        (const_cast <kpViewManager *> (this))->m_textCursorCol = -1;
    }

    return m_textCursorCol;
}

void kpViewManager::setTextCursorPosition (int row, int col, bool isUpdateMicroFocusHint)
{
    if (row == m_textCursorRow && col == m_textCursorCol)
        return;

    setFastUpdates ();
    setQueueUpdates ();

    m_textCursorBlinkState = false;
    updateTextCursor ();

    m_textCursorRow = row;
    m_textCursorCol = col;

    m_textCursorBlinkState = true;
    updateTextCursor ();

    restoreQueueUpdates ();
    restoreFastUpdates ();

    if (isUpdateMicroFocusHint)
    {
        kpDocument *doc = m_mainWindow->document ();
        if (!doc)
            return;
        
        kpSelection *sel = doc->selection ();
        if (!sel || !sel->isText ())
            return;

        if (m_viewUnderCursor)
        {
            QPoint topLeft = sel->pointForTextRowCol (m_textCursorRow, m_textCursorCol);
            if (topLeft != KP_INVALID_POINT)
            {
                m_viewUnderCursor->updateMicroFocusHint(QRect (topLeft.x (), topLeft.y (), 1, sel->textStyle ().fontMetrics ().height ())); 
            }
        }
    }
}


bool kpViewManager::textCursorBlinkState () const
{
    return m_textCursorBlinkState;
}

void kpViewManager::setTextCursorBlinkState (bool on)
{
    if (on == m_textCursorBlinkState)
        return;

    m_textCursorBlinkState = on;

    updateTextCursor ();
}


// protected
void kpViewManager::updateTextCursor ()
{
#if DEBUG_KP_VIEW_MANAGER && 0
    kDebug () << "kpViewManager::updateTextCursor()" << endl;
#endif

    if (!m_mainWindow)
        return;

    kpDocument *doc = m_mainWindow->document ();
    if (!doc)
        return;

    kpSelection *sel = doc->selection ();
    if (!sel || !sel->isText ())
        return;

    // TODO: fix code duplication with kpView::paintEventDrawSelection()
    QPoint topLeft = sel->pointForTextRowCol (m_textCursorRow, m_textCursorCol);
    if (topLeft != KP_INVALID_POINT)
    {
        setFastUpdates ();
        updateViews (QRect (topLeft.x (), topLeft.y (), 1, sel->textStyle ().fontMetrics ().height ()));
        restoreFastUpdates ();
    }
}

// protected slot
void kpViewManager::slotTextCursorBlink ()
{
#if DEBUG_KP_VIEW_MANAGER && 0
    kDebug () << "kpViewManager::slotTextCursorBlink() cursorBlinkState="
               << m_textCursorBlinkState << endl;
#endif

    if (m_textCursorBlinkTimer)
    {
        // (single shot)
        m_textCursorBlinkTimer->start (QApplication::cursorFlashTime () / 2);
    }

    updateTextCursor ();
    m_textCursorBlinkState = !m_textCursorBlinkState;
}


void kpViewManager::setCursor (const QCursor &cursor)
{
    for (QLinkedList <kpView *>::const_iterator it = m_views.begin ();
         it != m_views.end ();
         it++)
    {
        (*it)->setCursor (cursor);
    }

    m_cursor = cursor;
}

void kpViewManager::unsetCursor ()
{
    for (QLinkedList <kpView *>::const_iterator it = m_views.begin ();
         it != m_views.end ();
         it++)
    {
        (*it)->unsetCursor ();
    }

    m_cursor = QCursor ();
}


kpView *kpViewManager::viewUnderCursor (bool usingQt) const
{
    if (!usingQt)
    {
        kpViewManager *nonConstThis = const_cast <kpViewManager *> (this);

        if (m_viewUnderCursor && !nonConstThis->m_views.contains (m_viewUnderCursor))
        {
            kError () << "kpViewManager::viewUnderCursor(): invalid view" << endl;
            nonConstThis->m_viewUnderCursor = 0;
        }


        return m_viewUnderCursor;
    }
    else
    {
        for (QLinkedList <kpView *>::const_iterator it = m_views.begin ();
             it != m_views.end ();
             it++)
        {
            if ((*it)->underMouse ())
                return (*it);
        }

        return 0;
    }
}

void kpViewManager::setViewUnderCursor (kpView *view)
{
#if DEBUG_KP_VIEW_MANAGER && 1
    kDebug () << "kpViewManager::setViewUnderCursor ("
               << (view ? view->name () : "(none)") << ")"
               << "  old=" << (m_viewUnderCursor ? m_viewUnderCursor->name () : "(none)")
               << endl;
#endif
    if (view == m_viewUnderCursor)
        return;

    m_viewUnderCursor = view;

    if (!m_viewUnderCursor)
    {
        // Hide the brush if the mouse cursor just left the view
        if (m_tempPixmap && m_tempPixmap->isBrush ())
        {
        #if DEBUG_KP_VIEW_MANAGER && 1
            kDebug () << "\thiding brush pixmap since cursor left view" << endl;
        #endif
            updateViews (m_tempPixmap->rect ());
        }
    }
    else
    {
        if (m_mainWindow && m_mainWindow->tool ())
        {
        #if DEBUG_KP_VIEW_MANAGER && 1
            kDebug () << "\tnotify tool that something changed below cursor" << endl;
        #endif
            m_mainWindow->tool ()->somethingBelowTheCursorChanged ();
        }
    }
}


// public
kpView *kpViewManager::activeView () const
{
    for (QLinkedList <kpView *>::const_iterator it = m_views.begin ();
         it != m_views.end ();
         it++)
    {
        if ((*it)->isActiveWindow ())
            return (*it);
    }

    return 0;
}


// public
bool kpViewManager::queueUpdates () const
{
    return (m_queueUpdatesCounter > 0);
}

// public
void kpViewManager::setQueueUpdates ()
{
    m_queueUpdatesCounter++;
#if DEBUG_KP_VIEW_MANAGER && 1
    kDebug () << "kpViewManager::setQueueUpdates() counter="
               << m_queueUpdatesCounter << endl;
#endif
}

// public
void kpViewManager::restoreQueueUpdates ()
{
    m_queueUpdatesCounter--;
#if DEBUG_KP_VIEW_MANAGER && 1
    kDebug () << "kpViewManager::restoreQueueUpdates() counter="
               << m_queueUpdatesCounter << endl;
#endif
    if (m_queueUpdatesCounter < 0)
    {
        kError () << "kpViewManager::restoreQueueUpdates() counter="
                   << m_queueUpdatesCounter;
    }

    if (m_queueUpdatesCounter <= 0)
    {
        for (QLinkedList <kpView *>::const_iterator it = m_views.begin ();
             it != m_views.end ();
             it++)
        {
            (*it)->updateQueuedArea ();
        }
    }
}


// public
bool kpViewManager::fastUpdates () const
{
    return (m_fastUpdatesCounter > 0);
}

// public
void kpViewManager::setFastUpdates ()
{
    m_fastUpdatesCounter++;
#if DEBUG_KP_VIEW_MANAGER && 0
    kDebug () << "kpViewManager::setFastUpdates() counter="
               << m_fastUpdatesCounter << endl;
#endif
}

// public
void kpViewManager::restoreFastUpdates ()
{
    m_fastUpdatesCounter--;
#if DEBUG_KP_VIEW_MANAGER && 0
    kDebug () << "kpViewManager::restoreFastUpdates() counter="
               << m_fastUpdatesCounter << endl;
#endif
    if (m_fastUpdatesCounter < 0)
    {
        kError () << "kpViewManager::restoreFastUpdates() counter="
                   << m_fastUpdatesCounter;
    }
}


void kpViewManager::updateView (kpView *v)
{
    updateView (v, QRect (0, 0, v->width (), v->height ()));
}

void kpViewManager::updateView (kpView *v, const QRect &viewRect)
{
    if (!queueUpdates ())
    {
        if (fastUpdates ())
            v->repaint (viewRect);
        else
            v->update (viewRect);
    }
    else
        v->addToQueuedArea (viewRect);
}

void kpViewManager::updateView (kpView *v, int x, int y, int w, int h)
{
    updateView (v, QRect (x, y, w, h));
}

void kpViewManager::updateView (kpView *v, const QRegion &viewRegion)
{
    if (!queueUpdates ())
    {
        if (fastUpdates ())
            v->repaint (viewRegion);
        else
            v->update (viewRegion.boundingRect ());
    }
    else
        v->addToQueuedArea (viewRegion);
}

void kpViewManager::updateViewRectangleEdges (kpView *v, const QRect &viewRect)
{
    if (viewRect.height () <= 0 || viewRect.width () <= 0)
        return;

    // Top line
    updateView (v, QRect (viewRect.x (), viewRect.y (),
                          viewRect.width (), 1));

    if (viewRect.height () >= 2)
    {
        // Bottom line
        updateView (v, QRect (viewRect.x (), viewRect.bottom (),
                              viewRect.width (), 1));

        if (viewRect.height () > 2)
        {
            // Left line
            updateView (v, QRect (viewRect.x (), viewRect.y () + 1,
                                  1, viewRect.height () - 2));

            if (viewRect.width () >= 2)
            {
                // Right line
                updateView (v, QRect (viewRect.right (), viewRect.y () + 1,
                                      1, viewRect.height () - 2));
            }
        }
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
    kDebug () << "kpViewManager::updateViews (" << docRect << ")" << endl;
#endif

    for (QLinkedList <kpView *>::const_iterator it = m_views.begin ();
         it != m_views.end ();
         it++)
    {
        kpView *view = *it;

    #if DEBUG_KP_VIEW_MANAGER && 0
        kDebug () << "\tupdating view " << view->name () << endl;
    #endif
        if (view->zoomLevelX () % 100 == 0 && view->zoomLevelY () % 100 == 0)
        {
        #if DEBUG_KP_VIEW_MANAGER && 0
            kDebug () << "\t\tviewRect=" << view->transformDocToView (docRect) << endl;
        #endif
            updateView (view, view->transformDocToView (docRect));
        }
        else
        {
            QRect viewRect = view->transformDocToView (docRect);

            int diff = qRound (double (qMax (view->zoomLevelX (), view->zoomLevelY ())) / 100.0) + 1;

            QRect newRect = QRect (viewRect.x () - diff,
                                   viewRect.y () - diff,
                                   viewRect.width () + 2 * diff,
                                   viewRect.height () + 2 * diff)
                                .intersect (QRect (0, 0, view->width (), view->height ()));

        #if DEBUG_KP_VIEW_MANAGER && 0
            kDebug () << "\t\tviewRect (+compensate)=" << newRect << endl;
        #endif
            updateView (view, newRect);
        }
    }
}

void kpViewManager::updateViews (int x, int y, int w, int h)
{
    updateViews (QRect (x, y, w, h));
}


void kpViewManager::adjustViewsToEnvironment ()
{
#if DEBUG_KP_VIEW_MANAGER && 1
    kDebug () << "kpViewManager::adjustViewsToEnvironment()"
               << " numViews=" << m_views.count ()
               << endl;
#endif
    for (QLinkedList <kpView *>::const_iterator it = m_views.begin ();
         it != m_views.end ();
         it++)
    {
        kpView *view = *it;

    #if DEBUG_KP_VIEW_MANAGER && 1
        kDebug () << "\tview: " << view->name ()
                   << endl;
    #endif
        view->adjustToEnvironment ();
    }
}

#include <kpViewManager.moc>