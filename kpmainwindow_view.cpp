
/*
   Copyright (c) 2003,2004,2005 Clarence Dang <dang@kde.org>
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

#define DEBUG_ZOOM_FLICKER 0

#include <kpmainwindow.h>

#include <qdatetime.h>
#include <qpainter.h>
#include <qtimer.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kstdaction.h>

#include <kpdefs.h>
#include <kpdocument.h>
#include <kpthumbnail.h>
#include <kptool.h>
#include <kpview.h>
#include <kpviewmanager.h>
#include <kpviewscrollablecontainer.h>
#include <kpwidgetmapper.h>

#if KP_IS_KDE_3_2
    #include <kactionclasses.h>
#else
    #include <kaction.h>
#endif

#if !KP_IS_KDE_3_2
    #include <kdebugclasses.h>
#endif


// private
void kpMainWindow::setupViewMenuActions ()
{
    m_viewMenuDocumentActionsEnabled = false;
    m_thumbnailSaveConfigTimer = 0;


    KActionCollection *ac = actionCollection ();

    /*m_actionFullScreen = KStdAction::fullScreen (0, 0, ac);
    m_actionFullScreen->setEnabled (false);*/


    m_actionActualSize = KStdAction::actualSize (this, SLOT (slotActualSize ()), ac);
    /*m_actionFitToPage = KStdAction::fitToPage (this, SLOT (slotFitToPage ()), ac);
    m_actionFitToWidth = KStdAction::fitToWidth (this, SLOT (slotFitToWidth ()), ac);
    m_actionFitToHeight = KStdAction::fitToHeight (this, SLOT (slotFitToHeight ()), ac);*/


    m_actionZoomIn = KStdAction::zoomIn (this, SLOT (slotZoomIn ()), ac);
    m_actionZoomOut = KStdAction::zoomOut (this, SLOT (slotZoomOut ()), ac);


    m_actionZoom = new KSelectAction (i18n ("&Zoom"), 0,
        this, SLOT (slotZoom ()), actionCollection (), "view_zoom_to");
    m_actionZoom->setEditable (true);

    // create the zoom list for the 1st call to zoomTo() below
    m_zoomList.push_back (10); m_zoomList.push_back (25); m_zoomList.push_back (33);
    m_zoomList.push_back (50); m_zoomList.push_back (67); m_zoomList.push_back (75);
    m_zoomList.push_back (100);
    m_zoomList.push_back (200); m_zoomList.push_back (300);
    m_zoomList.push_back (400); m_zoomList.push_back (600); m_zoomList.push_back (800);
    m_zoomList.push_back (1000); m_zoomList.push_back (1200); m_zoomList.push_back (1600);


    m_actionShowGrid = new KToggleAction (i18n ("Show &Grid"), CTRL + Key_G,
        this, SLOT (slotShowGridToggled ()), actionCollection (), "view_show_grid");
#if KP_IS_KDE_3_3
    m_actionShowGrid->setCheckedState (i18n ("Hide &Grid"));
#endif


    m_actionShowThumbnail = new KToggleAction (i18n ("Show T&humbnail"), CTRL + Key_H,
        this, SLOT (slotShowThumbnailToggled ()), actionCollection (), "view_show_thumbnail");
#if KP_IS_KDE_3_3
    m_actionShowThumbnail->setCheckedState (i18n ("Hide T&humbnail"));
#endif


    enableViewMenuDocumentActions (false);
}

// private
bool kpMainWindow::viewMenuDocumentActionsEnabled () const
{
    return m_viewMenuDocumentActionsEnabled;
}

// private
void kpMainWindow::enableViewMenuDocumentActions (bool enable)
{
    m_viewMenuDocumentActionsEnabled = enable;


    m_actionActualSize->setEnabled (enable);
    /*m_actionFitToPage->setEnabled (enable);
    m_actionFitToWidth->setEnabled (enable);
    m_actionFitToHeight->setEnabled (enable);*/

    m_actionZoomIn->setEnabled (enable);
    m_actionZoomOut->setEnabled (enable);

    m_actionZoom->setEnabled (enable);

    actionShowGridUpdate ();
    m_actionShowThumbnail->setEnabled (enable);

    // TODO: for the time being, assume that we start at zoom 100%
    //       with no grid

    // This function is only called when a new document is created
    // or an existing document is closed.  So the following will
    // always be correct:

    zoomTo (100);
}

// private
void kpMainWindow::actionShowGridUpdate ()
{
#if DEBUG_KP_MAIN_WINDOW
    kdDebug () << "kpMainWindow::actionShowGridUpdate()" << endl;
#endif
    const bool enable = (viewMenuDocumentActionsEnabled () &&
                         m_mainView && m_mainView->canShowGrid ());

    m_actionShowGrid->setEnabled (enable);
    m_actionShowGrid->setChecked (enable && m_configShowGrid);
}


// private
void kpMainWindow::sendZoomListToActionZoom ()
{
    QStringList items;

    const QValueVector <int>::const_iterator zoomListEnd (m_zoomList.end ());
    for (QValueVector <int>::const_iterator it = m_zoomList.begin ();
         it != zoomListEnd;
         it++)
    {
        items << zoomLevelToString (*it);
    }

    // Work around a KDE bug - KSelectAction::setItems() enables the action.
    // David Faure said it won't be fixed because it's a feature used by
    // KRecentFilesAction.
    bool e = m_actionZoom->isEnabled ();
    m_actionZoom->setItems (items);
    if (e != m_actionZoom->isEnabled ())
        m_actionZoom->setEnabled (e);
}

// private
int kpMainWindow::zoomLevelFromString (const QString &string)
{
    // loop until not digit
    int i;
    for (i = 0; i < (int) string.length () && string.at (i).isDigit (); i++)
        ;

    // convert zoom level to number
    bool ok = false;
    int zoomLevel = string.left (i).toInt (&ok);

    if (!ok || zoomLevel <= 0 || zoomLevel > 3200)
        return 0;  // error
    else
        return zoomLevel;
}

// private
QString kpMainWindow::zoomLevelToString (int zoomLevel)
{
    return i18n ("%1%").arg (zoomLevel);
}

// private
void kpMainWindow::zoomTo (int zoomLevel)
{
#if DEBUG_KP_MAIN_WINDOW
    kdDebug () << "kpMainWindow::zoomTo (" << zoomLevel << ")" << endl;
#endif

    if (zoomLevel <= 0)
        zoomLevel = m_mainView ? m_mainView->zoomLevelX () : 100;

// mute point since the thumbnail suffers from this too
#if 0
    else if (m_mainView && m_mainView->zoomLevelX () % 100 == 0 && zoomLevel % 100)
    {
        if (KMessageBox::warningContinueCancel (this,
            i18n ("Setting the zoom level to a value that is not a multiple of 100% "
                  "results in imprecise editing and redraw glitches.\n"
                  "Do you really want to set to zoom level to %1%?")
                .arg (zoomLevel),
            QString::null/*caption*/,
            i18n ("Set Zoom Level to %1%").arg (zoomLevel),
            "DoNotAskAgain_ZoomLevelNotMultipleOf100") != KMessageBox::Continue)
        {
            zoomLevel = m_mainView->zoomLevelX ();
        }
    }
#endif

    int index = 0;
    QValueVector <int>::iterator it = m_zoomList.begin ();

    while (index < (int) m_zoomList.size () && zoomLevel > *it)
        it++, index++;

    if (zoomLevel != *it)
        m_zoomList.insert (it, zoomLevel);

    sendZoomListToActionZoom ();
    m_actionZoom->setCurrentItem (index);


    if (viewMenuDocumentActionsEnabled ())
    {
        m_actionActualSize->setEnabled (zoomLevel != 100);

        m_actionZoomIn->setEnabled (m_actionZoom->currentItem () < (int) m_zoomList.size () - 1);
        m_actionZoomOut->setEnabled (m_actionZoom->currentItem () > 0);
    }


    if (m_viewManager)
        m_viewManager->setQueueUpdates ();


    if (m_scrollView)
    {
        m_scrollView->setUpdatesEnabled (false);
        if (m_scrollView->viewport ())
            m_scrollView->viewport ()->setUpdatesEnabled (false);
    }

    if (m_mainView)
    {
        m_mainView->setUpdatesEnabled (false);

        if (m_scrollView && m_scrollView->viewport ())
        {
            // Ordinary flicker is better than the whole view moving
            QPainter p (m_mainView);
            p.fillRect (m_mainView->rect (),
                        m_scrollView->viewport ()->colorGroup ().background ());
        }
    }


    if (m_scrollView && m_mainView)
    {
    #if DEBUG_KP_MAIN_WINDOW && 1
        kdDebug () << "\tscrollView   contentsX=" << m_scrollView->contentsX ()
                   << " contentsY=" << m_scrollView->contentsY ()
                   << " contentsWidth=" << m_scrollView->contentsWidth ()
                   << " contentsHeight=" << m_scrollView->contentsHeight ()
                   << " visibleWidth=" << m_scrollView->visibleWidth ()
                   << " visibleHeight=" << m_scrollView->visibleHeight ()
                   << " oldZoomX=" << m_mainView->zoomLevelX ()
                   << " oldZoomY=" << m_mainView->zoomLevelY ()
                   << " newZoom=" << zoomLevel
                   << " mainViewX=" << m_scrollView->childX (m_mainView)
                   << " mainViewY=" << m_scrollView->childY (m_mainView)
                  << endl;
    #endif

        // TODO: when changing from no scrollbars to scrollbars, Qt lies about
        //       visibleWidth() & visibleHeight() (doesn't take into account the
        //       space taken by the would-be scrollbars) until it updates the
        //       scrollview; hence the centring is off by about 5-10 pixels.

        int newCenterX = (m_scrollView->contentsX ()
                         + QMIN (m_mainView->width (), m_scrollView->visibleWidth ()) / 2)
                         * zoomLevel / m_mainView->zoomLevelX ();
        int newCenterY = (m_scrollView->contentsY ()
                         + QMIN (m_mainView->height (), m_scrollView->visibleHeight ()) / 2)
                         * zoomLevel / m_mainView->zoomLevelY ();

        m_mainView->setZoomLevel (zoomLevel, zoomLevel);
    #if DEBUG_ZOOM_FLICKER
    {
        kdDebug () << "FLICKER: just setZoomLevel" << endl;
        QTime timer; timer.start ();
        while (timer.elapsed () < 1000)
            ;
    }
    #endif

    #if DEBUG_KP_MAIN_WINDOW && 1
        kdDebug () << "\tvisibleWidth=" << m_scrollView->visibleWidth ()
                    << " visibleHeight=" << m_scrollView->visibleHeight ()
                    << endl;
        kdDebug () << "\tnewCenterX=" << newCenterX
                    << " newCenterY=" << newCenterY << endl;
    #endif

        m_scrollView->center (newCenterX, newCenterY);
    #if DEBUG_ZOOM_FLICKER
    {
        kdDebug () << "FLICKER: just centred" << endl;
        QTime timer; timer.start ();
        while (timer.elapsed () < 2000)
            ;
    }
    #endif

    #if DEBUG_KP_MAIN_WINDOW && 1
        kdDebug () << "\t\tcheck (contentsX=" << m_scrollView->contentsX ()
                    << ",contentsY=" << m_scrollView->contentsY ()
                    << ")" << endl;
    #endif
    }

    if (m_mainView)
    {
        actionShowGridUpdate ();
    #if DEBUG_ZOOM_FLICKER
    {
        kdDebug () << "FLICKER: updated grid action" << endl;
        QTime timer; timer.start ();
        while (timer.elapsed () < 1000)
            ;
    }
    #endif


        updateMainViewGrid ();
    #if DEBUG_ZOOM_FLICKER
    {
        kdDebug () << "FLICKER: just updated grid" << endl;
        QTime timer; timer.start ();
        while (timer.elapsed () < 1000)
            ;
    }
    #endif


        // Since Zoom Level KSelectAction on ToolBar grabs focus after changing
        // Zoom, switch back to the Main View.
        // TODO: back to the last view
        m_mainView->setFocus ();
    #if DEBUG_ZOOM_FLICKER
    {
        kdDebug () << "FLICKER: just set focus to mainview" << endl;
        QTime timer; timer.start ();
        while (timer.elapsed () < 1000)
            ;
    }
    #endif

    }
#if 1
    // The view magnified and moved beneath the cursor
    if (tool ())
        tool ()->somethingBelowTheCursorChanged ();
    #if DEBUG_ZOOM_FLICKER
    {
        kdDebug () << "FLICKER: signalled something below cursor" << endl;
        QTime timer; timer.start ();
        while (timer.elapsed () < 1000)
            ;
    }
    #endif
#endif

    // HACK: make sure all of Qt's update() calls trigger
    //       kpView::paintEvent() _now_ so that they can be queued by us
    //       (until kpViewManager::restoreQueueUpdates()) to reduce flicker
    //       caused mainly by m_scrollView->center()
    //
    // TODO: remove flicker completely
    //QTimer::singleShot (0, this, SLOT (finishZoomTo ()));

    // Later: I don't think there is an update() that needs to be queued
    //        - let's reduce latency instead.
    finishZoomTo ();
}

// private slot
void kpMainWindow::finishZoomTo ()
{
#if DEBUG_KP_MAIN_WINDOW && 1
    kdDebug () << "\tkpMainWindow::finishZoomTo enter" << endl;
#endif

#if DEBUG_ZOOM_FLICKER
{
    kdDebug () << "FLICKER: inside finishZoomTo" << endl;
    QTime timer; timer.start ();
    while (timer.elapsed () < 2000)
        ;
}
#endif

    if (m_mainView)
    {
        m_mainView->setUpdatesEnabled (true);
        m_mainView->update ();
    }

#if DEBUG_ZOOM_FLICKER
{
    kdDebug () << "FLICKER: just updated mainView" << endl;
    QTime timer; timer.start ();
    while (timer.elapsed () < 1000)
        ;
}
#endif

    if (m_scrollView)
    {
        if (m_scrollView->viewport ())
        {
            m_scrollView->viewport ()->setUpdatesEnabled (true);
            m_scrollView->viewport ()->update ();
        }
    #if DEBUG_ZOOM_FLICKER
    {
        kdDebug () << "FLICKER: just updated scrollView::viewport()" << endl;
        QTime timer; timer.start ();
        while (timer.elapsed () < 1000)
            ;
    }
    #endif

        m_scrollView->setUpdatesEnabled (true);
        m_scrollView->update ();
    #if DEBUG_ZOOM_FLICKER
    {
        kdDebug () << "FLICKER: just updated scrollView" << endl;
        QTime timer; timer.start ();
        while (timer.elapsed () < 1000)
            ;
    }
    #endif

    }


    if (m_viewManager && m_viewManager->queueUpdates ()/*just in case*/)
        m_viewManager->restoreQueueUpdates ();
#if DEBUG_ZOOM_FLICKER
{
    kdDebug () << "FLICKER: restored vm queued updates" << endl;
    QTime timer; timer.start ();
    while (timer.elapsed () < 1000)
        ;
}
#endif

    setStatusBarZoom (m_mainView ? m_mainView->zoomLevelX () : 0);

#if DEBUG_KP_MAIN_WINDOW && 1
    kdDebug () << "\tkpMainWindow::finishZoomTo done" << endl;
#endif

#if DEBUG_ZOOM_FLICKER
{
    kdDebug () << "FLICKER: finishZoomTo done" << endl;
    QTime timer; timer.start ();
    while (timer.elapsed () < 1000)
        ;
}
#endif
}


// private slot
void kpMainWindow::slotActualSize ()
{
    zoomTo (100);
}

// private slot
void kpMainWindow::slotFitToPage ()
{
    if (!m_scrollView || !m_document)
        return;

    // doc_width * zoom / 100 <= view_width &&
    // doc_height * zoom / 100 <= view_height &&
    // 1 <= zoom <= 3200

    zoomTo (QMIN (3200, QMAX (1, QMIN (m_scrollView->visibleWidth () * 100 / m_document->width (),
                              m_scrollView->visibleHeight () * 100 / m_document->height ()))));
}

// private slot
void kpMainWindow::slotFitToWidth ()
{
    if (!m_scrollView || !m_document)
        return;

    // doc_width * zoom / 100 <= view_width &&
    // 1 <= zoom <= 3200

    zoomTo (QMIN (3200, QMAX (1, m_scrollView->visibleWidth () * 100 / m_document->width ())));
}

// private slot
void kpMainWindow::slotFitToHeight ()
{
    if (!m_scrollView || !m_document)
        return;

    // doc_height * zoom / 100 <= view_height &&
    // 1 <= zoom <= 3200

    zoomTo (QMIN (3200, QMAX (1, m_scrollView->visibleHeight () * 100 / m_document->height ())));
}


// private slot
void kpMainWindow::slotZoomIn ()
{
#if DEBUG_KP_MAIN_WINDOW
    kdDebug () << "kpMainWindow::slotZoomIn ()" << endl;
#endif

    m_actionZoom->setCurrentItem (m_actionZoom->currentItem () + 1);
    slotZoom ();
}

// private slot
void kpMainWindow::slotZoomOut ()
{
#if DEBUG_KP_MAIN_WINDOW
    kdDebug () << "kpMainWindow::slotZoomOut ()" << endl;
#endif

    m_actionZoom->setCurrentItem (m_actionZoom->currentItem () - 1);
    slotZoom ();
}

// private slot
void kpMainWindow::slotZoom ()
{
#if DEBUG_KP_MAIN_WINDOW
    kdDebug () << "kpMainWindow::slotZoom () index=" << m_actionZoom->currentItem ()
               << " text='" << m_actionZoom->currentText () << "'" << endl;
#endif
    zoomTo (zoomLevelFromString (m_actionZoom->currentText ()));
}


// private slot
void kpMainWindow::slotShowGridToggled ()
{
#if DEBUG_KP_MAIN_WINDOW
    kdDebug () << "kpMainWindow::slotActionShowGridToggled()" << endl;
#endif

    updateMainViewGrid ();


    KConfigGroupSaver cfgGroupSaver (kapp->config (), kpSettingsGroupGeneral);
    KConfigBase *cfg = cfgGroupSaver.config ();

    cfg->writeEntry (kpSettingShowGrid, m_configShowGrid = m_actionShowGrid->isChecked ());
    cfg->sync ();
}

// private
void kpMainWindow::updateMainViewGrid ()
{
#if DEBUG_KP_MAIN_WINDOW
    kdDebug () << "kpMainWindow::updateMainViewGrid ()" << endl;
#endif

    if (m_mainView)
        m_mainView->showGrid (m_actionShowGrid->isChecked ());
}


// private
QRect kpMainWindow::mapToGlobal (const QRect &rect) const
{
    return kpWidgetMapper::toGlobal (this, rect);
}

// private
QRect kpMainWindow::mapFromGlobal (const QRect &rect) const
{
    return kpWidgetMapper::fromGlobal (this, rect);
}


// public slot
void kpMainWindow::slotDestroyThumbnailIfNotVisible (bool tnIsVisible)
{
#if DEBUG_KP_MAIN_WINDOW
    kdDebug () << "slotDestroyThumbnailIfNotVisible(isVisible="
               << tnIsVisible
               << ")"
               << endl;
#endif

    if (!tnIsVisible)
    {
        slotDestroyThumbnailInitatedByUser ();
    }
}

// private slot
void kpMainWindow::slotDestroyThumbnail ()
{
    m_actionShowThumbnail->setChecked (false);
    updateThumbnail ();
}

// private slot
void kpMainWindow::slotDestroyThumbnailInitatedByUser ()
{
    m_actionShowThumbnail->setChecked (false);
    slotShowThumbnailToggled ();
}

// private slot
void kpMainWindow::slotCreateThumbnail ()
{
    m_actionShowThumbnail->setChecked (true);
    updateThumbnail ();
}

// public
void kpMainWindow::notifyThumbnailGeometryChanged ()
{
#if DEBUG_KP_MAIN_WINDOW
    kdDebug () << "kpMainWindow::notifyThumbnailGeometryChanged()" << endl;
#endif

    if (!m_thumbnailSaveConfigTimer)
    {
        m_thumbnailSaveConfigTimer = new QTimer (this);
        connect (m_thumbnailSaveConfigTimer, SIGNAL (timeout ()),
                 this, SLOT (slotSaveThumbnailGeometry ()));
    }

    m_thumbnailSaveConfigTimer->start (500/*msec*/, true/*single shot*/);
}

// private slot
void kpMainWindow::slotSaveThumbnailGeometry ()
{
#if DEBUG_KP_MAIN_WINDOW
    kdDebug () << "kpMainWindow::saveThumbnailGeometry()" << endl;
#endif

    if (!m_thumbnail)
        return;

    QRect rect (m_thumbnail->x (), m_thumbnail->y (),
                m_thumbnail->width (), m_thumbnail->height ());
#if DEBUG_KP_MAIN_WINDOW
    kdDebug () << "\tthumbnail relative geometry=" << rect << endl;
#endif

    m_configThumbnailGeometry = mapFromGlobal (rect);

#if DEBUG_KP_MAIN_WINDOW
    kdDebug () << "\tCONFIG: saving thumbnail geometry "
                << m_configThumbnailGeometry
                << endl;
#endif

    KConfigGroupSaver cfgGroupSaver (kapp->config (), kpSettingsGroupThumbnail);
    KConfigBase *cfg = cfgGroupSaver.config ();

    cfg->writeEntry (kpSettingThumbnailGeometry, m_configThumbnailGeometry);
    cfg->sync ();
}

// private slot
void kpMainWindow::slotShowThumbnailToggled ()
{
#if DEBUG_KP_MAIN_WINDOW
    kdDebug () << "kpMainWindow::slotShowThumbnailToggled()" << endl;
#endif

    updateThumbnail ();


    m_configThumbnailShown = m_actionShowThumbnail->isChecked ();

    KConfigGroupSaver cfgGroupSaver (kapp->config (), kpSettingsGroupThumbnail);
    KConfigBase *cfg = cfgGroupSaver.config ();

    cfg->writeEntry (kpSettingThumbnailShown, m_configThumbnailShown);
    cfg->sync ();
}

// private
void kpMainWindow::updateThumbnail ()
{
#if DEBUG_KP_MAIN_WINDOW
    kdDebug () << "kpMainWindow::updateThumbnail()" << endl;
#endif
   bool enable = m_actionShowThumbnail->isChecked ();

#if DEBUG_KP_MAIN_WINDOW
    kdDebug () << "\tthumbnail="
               << bool (m_thumbnail)
               << " action_isChecked="
               << enable
               << endl;
#endif

    if (bool (m_thumbnail) == enable)
        return;

    if (!m_thumbnail)
    {
    #if DEBUG_KP_MAIN_WINDOW
        kdDebug () << "\tcreating thumbnail" << endl;
    #endif

        // Read last saved geometry before creating thumbnail & friends
        // in case they call notifyThumbnailGeometryChanged()
        QRect thumbnailGeometry = m_configThumbnailGeometry;
    #if DEBUG_KP_MAIN_WINDOW
        kdDebug () << "\t\tlast used geometry=" << thumbnailGeometry << endl;
    #endif

        m_thumbnail = new kpThumbnail (this, "thumbnail");

    #if DEBUG_KP_MAIN_WINDOW
        kdDebug () << "\t\tcreating new kpView:" << endl;
    #endif
        m_thumbnailView = new kpView (m_thumbnail,
                                      "thumbnailView", this,
                                      m_thumbnail->width (), m_thumbnail->height (),
                                      true /*autoVariableZoom*/);
    #if DEBUG_KP_MAIN_WINDOW
        kdDebug () << "\t\tgive kpThumbnail the kpView:" << endl;
    #endif
        m_thumbnail->setView (m_thumbnailView);
    #if DEBUG_KP_MAIN_WINDOW
        kdDebug () << "\t\tregistering the kpView:" << endl;
    #endif
        m_viewManager->registerView (m_thumbnailView);

    #if DEBUG_KP_MAIN_WINDOW
        kdDebug () << "\t\tconnecting doc::sizeChanged() to thumbnail caption" << endl;
    #endif
        if (m_document)
        {
            connect (m_document, SIGNAL (sizeChanged (int, int)),
                     m_thumbnail, SLOT (updateCaption ()));
        }


    #if DEBUG_KP_MAIN_WINDOW
        kdDebug () << "\t\tmoving thumbnail to right place" << endl;
    #endif
        if (!thumbnailGeometry.isEmpty () &&
            QRect (0, 0, width (), height ()).intersects (thumbnailGeometry))
        {
            const QRect geometry = mapToGlobal (thumbnailGeometry);
            m_thumbnail->resize (geometry.size ());
            m_thumbnail->move (geometry.topLeft ());
        }
        else
        {
            if (m_scrollView)
            {
                const int margin = 20;
                const int initialWidth = 160, initialHeight = 120;

                QRect geometryRect (width () - initialWidth - margin * 2,
                                    m_scrollView->y () + margin,
                                    initialWidth,
                                    initialHeight);

            #if DEBUG_KP_MAIN_WINDOW
                kdDebug () << "\t\tcreating geometry=" << geometryRect << endl;
            #endif

                geometryRect = mapToGlobal (geometryRect);
            #if DEBUG_KP_MAIN_WINDOW
                kdDebug () << "\t\tmap to global=" << geometryRect << endl;
            #endif
                m_thumbnail->resize (geometryRect.size ());
                m_thumbnail->move (geometryRect.topLeft ());
            }
        }

    #if DEBUG_KP_MAIN_WINDOW
        kdDebug () << "\t\tshowing thumbnail" << endl;
    #endif
        m_thumbnail->show ();

    #if DEBUG_KP_MAIN_WINDOW
        kdDebug () << "\t\tconnecting thumbnail::visibilityChange to destroy slot" << endl;
    #endif
        connect (m_thumbnail, SIGNAL (visibilityChanged (bool)),
                 this, SLOT (slotDestroyThumbnailIfNotVisible (bool)));
    #if DEBUG_KP_MAIN_WINDOW
        kdDebug () << "\t\tDONE" << endl;
    #endif
    }
    else
    {
    #if DEBUG_KP_MAIN_WINDOW
        kdDebug () << "\tdestroying thumbnail" << endl;
    #endif

        if (m_thumbnailSaveConfigTimer && m_thumbnailSaveConfigTimer->isActive ())
        {
            m_thumbnailSaveConfigTimer->stop ();
            slotSaveThumbnailGeometry ();
        }


        if (m_document)
        {
            disconnect (m_document, SIGNAL (sizeChanged (int, int)),
                        m_thumbnail, SLOT (updateCaption ()));
        }


        m_viewManager->unregisterView (m_thumbnailView);

        delete m_thumbnailView; m_thumbnailView = 0;


        disconnect (m_thumbnail, SIGNAL (visibilityChanged (bool)),
                    this, SLOT (slotDestroyThumbnailIfNotVisible (bool)));

        // From kolourpaint_1_0_branch: revision 1.16.2.2
        // date: 2004/04/11 10:46:47;  author: dang;  state: Exp;  lines: +19 -1
        // - fix thumbnail-related crashes with early Qt 3.x versions
        //   (at least Qt 3.0 but not Qt 3.2):
        //   - crash on thumbnail close with accelerator (pending events in event loop)
        //   - crash on mainwindow close if the thumbnail was ever closed
        //     (QDockWindow didn't remove itself from QMainWindow::dockWindows() list)
        // (...)
        removeDockWindow (m_thumbnail);
        m_thumbnail->deleteLater (); m_thumbnail = 0;
    }
}