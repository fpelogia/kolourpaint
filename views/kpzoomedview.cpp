
/*
   Copyright (c) 2003-2004 Clarence Dang <dang@kde.org>
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


#define DEBUG_KP_ZOOMED_VIEW 0


#include <kpzoomedview.h>

#include <kdebug.h>

#include <kpdocument.h>
#include <kpview.h>
#include <kpviewmanager.h>


kpZoomedView::kpZoomedView (kpDocument *document,
        kpToolControllerIface *toolController,
        kpViewManager *viewManager,
        kpView *buddyView,
        kpViewScrollableContainer *buddyViewScrollView,
        QWidget *parent, const char *name)

    : kpView (document, toolController, viewManager,
              buddyView, buddyViewScrollView,
              parent, name)
{
    // Call to virtual function - this is why the class is sealed
    adjustToEnvironment ();
}

kpZoomedView::~kpZoomedView ()
{
}


// public virtual [base kpView]
void kpZoomedView::setZoomLevel (int hzoom, int vzoom)
{
#if DEBUG_KP_ZOOMED_VIEW
    kdDebug () << "kpZoomedView(" << name () << ")::setZoomLevel("
               << hzoom << "," << vzoom << ")" << endl;
#endif

    if (viewManager ())
        viewManager ()->setQueueUpdates ();
  
    {
        kpView::setZoomLevel (hzoom, vzoom);

        adjustToEnvironment ();
    }

    if (viewManager ())
        viewManager ()->restoreQueueUpdates ();
}


// public slot virtual [base kpView]
void kpZoomedView::adjustToEnvironment ()
{
#if DEBUG_KP_ZOOMED_VIEW
    kdDebug () << "kpZoomedView(" << name () << ")::adjustToEnvironment()"
               << " doc: width=" << document ()->width ()
               << " height=" << document ()->height ()
               << endl;
#endif

    if (document ())
    {
        // TODO: use zoomedDocWidth() & zoomedDocHeight()?
        resize (transformDocToViewX (document ()->width ()),
                transformDocToViewY (document ()->height ()));
    }
}


#include <kpzoomedview.moc>