
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


#define DEBUG_KP_TOOL_SELECTION 0

#include <qbitmap.h>
#include <qcursor.h>
#include <qpainter.h>

#include <kdebug.h>
#include <klocale.h>

#include <kpcommandhistory.h>
#include <kpdefs.h>
#include <kpdocument.h>
#include <kpmainwindow.h>
#include <kpselection.h>
#include <kptoolselection.h>
#include <kptooltoolbar.h>
#include <kptoolwidgetopaqueortransparent.h>
#include <kpviewmanager.h>


kpToolSelection::kpToolSelection (kpMainWindow *mainWindow)
    : kpTool (QString::null, QString::null, mainWindow, "tool_selection_base_class"),
      m_currentPullFromDocumentCommand (0),
      m_currentMoveCommand (0),
      m_toolWidgetOpaqueOrTransparent (0),
      m_currentCreateTextCommand (0)
{
    setMode (kpToolSelection::Rectangle);
}

kpToolSelection::~kpToolSelection ()
{
}


// private
void kpToolSelection::pushOntoDocument ()
{
#if DEBUG_KP_TOOL_SELECTION && 1
    kdDebug () << "kpToolSelection::pushOntoDocument() CALLED" << endl;
#endif
    mainWindow ()->slotDeselect ();
}


// public
QString kpToolSelection::haventBegunDrawUserMessage () const
{
#if DEBUG_KP_TOOL_SELECTION && 1
    kdDebug () << "kpToolSelection::haventBegunDrawUserMessage()"
                  " cancelledShapeButStillHoldingButtons="
               << m_cancelledShapeButStillHoldingButtons
               << endl;
#endif

    if (m_cancelledShapeButStillHoldingButtons)
        return i18n ("Let go of all the mouse buttons.");

    if (document () && document ()->selection () && document ()->selection ()->contains (m_currentPoint))
    {
        if (m_mode == Text)
        {
            if (document ()->selection ()->pointIsInTextArea (m_currentPoint))
                return i18n ("Left click to change cursor position.");
            else
                return i18n ("Left drag to move text box.");
        }
        else
            return i18n ("Left drag to move selection.");
    }
    else
    {
        if (m_mode == Text)
            return i18n ("Left drag to create text box.");
        else
            return i18n ("Left drag to create selection.");
    }
}


// virtual
void kpToolSelection::begin ()
{
#if DEBUG_KP_TOOL_SELECTION
    kdDebug () << "kpToolSelection::begin()" << endl;
#endif

    kpToolToolBar *tb = toolToolBar ();

    if (tb && m_mode != Text)
    {
        m_toolWidgetOpaqueOrTransparent = tb->toolWidgetOpaqueOrTransparent ();

        if (m_toolWidgetOpaqueOrTransparent)
        {
            connect (m_toolWidgetOpaqueOrTransparent, SIGNAL (isOpaqueChanged (bool)),
                     this, SLOT (slotIsOpaqueChanged ()));
            m_toolWidgetOpaqueOrTransparent->show ();
        }
    }
    else
    {
        m_toolWidgetOpaqueOrTransparent = 0;
    }

    viewManager ()->setSelectionBorderVisible (true);
    viewManager ()->setSelectionBorderFinished (true);

    m_startDragFromSelectionTopLeft = QPoint ();
    m_dragType = Unknown;
    m_dragHasBegun = false;

    m_currentPullFromDocumentCommand = 0;
    m_currentMoveCommand = 0;
    m_currentCreateTextCommand = 0;

    m_cancelledShapeButStillHoldingButtons = false;

    setUserMessage (haventBegunDrawUserMessage ());
}

// virtual
void kpToolSelection::end ()
{
#if DEBUG_KP_TOOL_SELECTION
    kdDebug () << "kpToolSelection::end()" << endl;
#endif

    if (document ()->selection ())
        pushOntoDocument ();

    if (m_toolWidgetOpaqueOrTransparent)
    {
        disconnect (m_toolWidgetOpaqueOrTransparent, SIGNAL (isOpaqueChanged (bool)),
                    this, SLOT (slotIsOpaqueChanged ()));
        m_toolWidgetOpaqueOrTransparent = 0;
    }
}

// virtual
void kpToolSelection::reselect ()
{
#if DEBUG_KP_TOOL_SELECTION
    kdDebug () << "kpToolSelection::reselect()" << endl;
#endif

    if (document ()->selection ())
        pushOntoDocument ();
}


// virtual
void kpToolSelection::beginDraw ()
{
#if DEBUG_KP_TOOL_SELECTION
    kdDebug () << "kpToolSelection::beginDraw() CALLED" << endl;
#endif

    // In case the cursor was wrong to start with
    // (forgot to call kpTool::somethingBelowTheCursorChanged()),
    // make sure it is correct during this operation.
    hover (m_currentPoint);

    // Dragging with the RMB would make no sense
    // TODO: RMB click brings up Image popupmenu
    if (m_mouseButton != 0/*left*/)
        return;

    // Currently used only to end the current text
    if (hasBegunShape ())
        endShape (m_currentPoint, QRect (m_startPoint/* TODO: wrong */, m_currentPoint).normalize ());

    m_dragType = Create;
    m_dragHasBegun = false;

    kpSelection *sel = document ()->selection ();
    if (sel)
    {
    #if DEBUG_KP_TOOL_SELECTION
        kdDebug () << "\thas sel region" << endl;
    #endif
        QRect selectionRect = sel->boundingRect ();

        if (sel->contains (m_currentPoint))
        {
            if (m_mode == Text && sel->pointIsInTextArea (m_currentPoint))
            {
            #if DEBUG_KP_TOOL_SELECTION
                kdDebug () << "\t\tis select cursor pos" << endl;
            #endif

                m_dragType = SelectText;

                viewManager ()->setTextCursorPosition (sel->textRowForPoint (m_currentPoint),
                                                       sel->textColForPoint (m_currentPoint));
            }
            else
            {
            #if DEBUG_KP_TOOL_SELECTION
                kdDebug () << "\t\tis move" << endl;
            #endif

                m_startDragFromSelectionTopLeft = m_currentPoint - selectionRect.topLeft ();
                m_dragType = Move;

                // don't show border while moving (or when we start to move)
                viewManager ()->setSelectionBorderVisible (false);
                viewManager ()->setSelectionBorderFinished (true);
                viewManager ()->setTextCursorEnabled (false);
            }
        }
        else
        {
        #if DEBUG_KP_TOOL_SELECTION
            kdDebug () << "\t\tis new sel" << endl;
        #endif

            pushOntoDocument ();
        }
    }

    // creating new selection?
    if (m_dragType == Create)
    {
        viewManager ()->setSelectionBorderVisible (true);
        viewManager ()->setSelectionBorderFinished (false);
        viewManager ()->setTextCursorEnabled (false);
    }

    if (m_dragType != SelectText)
    {
        setUserMessage (i18n ("%1 to cancel.")
                            .arg (mouseClickText (true/*other mouse button*/,
                                                  true/*start of sentence*/)));
    }
}

// virtual
void kpToolSelection::hover (const QPoint &point)
{
    if (document () && document ()->selection () && document ()->selection ()->contains (point))
    {
        if (m_mode == Text && document ()->selection ()->pointIsInTextArea (point))
            viewManager ()->setCursor (Qt::ibeamCursor);
        else
            viewManager ()->setCursor (Qt::sizeAllCursor);
    }
    else
    {
        viewManager ()->setCursor (Qt::crossCursor);
    }

    setUserShapePoints (point, KP_INVALID_POINT, false/*don't set size*/);
    if (document () && document ()->selection ())
    {
        setUserShapeSize (document ()->selection ()->width (),
                          document ()->selection ()->height ());
    }
    else
    {
        setUserShapeSize (KP_INVALID_SIZE);
    }

    QString mess = haventBegunDrawUserMessage ();
    if (mess != userMessage ())
        setUserMessage (mess);
}

// virtual
void kpToolSelection::draw (const QPoint &thisPoint, const QPoint & /*lastPoint*/,
                            const QRect &normalizedRect)
{
#if DEBUG_KP_TOOL_SELECTION && 1
    kdDebug () << "kpToolSelection::draw() CALLED" << endl;
#endif

    // Dragging with the RMB would make no sense
    // TODO: RMB click brings up Image popupmenu
    if (m_mouseButton != 0/*left*/)
    {
        setUserShapePoints (thisPoint, KP_INVALID_POINT, false/*don't set size*/);
        setUserMessage (i18n ("Let go of all the mouse buttons."));
        return;
    }

    // Prevent both NOP drag-moves and unintentional 1-pixel selections
    const bool beganOperation = (m_dragHasBegun ||
                                 (!m_dragHasBegun && thisPoint != m_startPoint));
    if (!beganOperation)
    {
        if (m_dragType == Create)
        {
            /*if (m_mode == Rectangle || m_mode == Ellipse)
                setUserShapePoints (thisPoint, thisPoint);
            else*/
                setUserShapePoints (thisPoint);
        }
        else if (m_dragType == Move)
        {
            setUserShapePoints (document ()->selection ()->topLeft (),
                                document ()->selection ()->topLeft (),
                                false/*don't set size*/);
            setUserShapeSize (0, 0);
        }

        return;
    }


    if (m_dragType == Create)
    {
    #if DEBUG_KP_TOOL_SELECTION && 1
        kdDebug () << "\tnot moving - resizing rect to" << normalizedRect
                   << endl;
    #endif

        switch (m_mode)
        {
        case kpToolSelection::Rectangle:
        {
            const QRect usefulRect = normalizedRect.intersect (document ()->rect ());
            document ()->setSelection (kpSelection (kpSelection::Rectangle, usefulRect,
                                                    mainWindow ()->selectionTransparency ()));

            setUserShapePoints (m_startPoint,
                                QPoint (QMAX (0, QMIN (m_currentPoint.x (), document ()->width () - 1)),
                                        QMAX (0, QMIN (m_currentPoint.y (), document ()->height () - 1))));
            break;
        }
        case kpToolSelection::Text:
        {
            QRect usefulRect = normalizedRect;

            if (usefulRect.width () < kpSelection::minimumWidth ())
            {
                if (thisPoint.x () >= m_startPoint.x ())
                    usefulRect.setWidth (kpSelection::minimumWidth ());
                else
                    usefulRect.setX (usefulRect.right () - kpSelection::minimumWidth () + 1);
            }

            if (usefulRect.height () < kpSelection::minimumHeight ())
            {
                if (thisPoint.y () >= m_startPoint.y ())
                    usefulRect.setHeight (kpSelection::minimumHeight ());
                else
                    usefulRect.setY (usefulRect.bottom () - kpSelection::minimumHeight () + 1);
            }
        #if DEBUG_KP_TOOL_SELECTION && 1
            kdDebug () << "\t\tnormalizedRect=" << normalizedRect
                       << " usedRect=" << usefulRect
                       << " kpSelection::minimumSize=" << kpSelection::minimumSize ()
                       << endl;
        #endif

            QValueVector <QString> textLines (1, QString ());
            kpSelection sel (usefulRect, textLines, mainWindow ()->textStyle ());

            if (!m_currentCreateTextCommand)
            {
                m_currentCreateTextCommand = new kpToolSelectionCreateCommand (
                    i18n ("Text: Create Box"),
                    sel,
                    mainWindow ());
            }
            else
                m_currentCreateTextCommand->setFromSelection (sel);

            viewManager ()->setTextCursorPosition (0, 0);
            document ()->setSelection (sel);

            QPoint actualEndPoint = KP_INVALID_POINT;
            if (m_startPoint == usefulRect.topLeft ())
                actualEndPoint = usefulRect.bottomRight ();
            else if (m_startPoint == usefulRect.bottomRight ())
                actualEndPoint = usefulRect.topLeft ();
            else if (m_startPoint == usefulRect.topRight ())
                actualEndPoint = usefulRect.bottomLeft ();
            else if (m_startPoint == usefulRect.bottomLeft ())
                actualEndPoint = usefulRect.topRight ();

            setUserShapePoints (m_startPoint, actualEndPoint);
            break;
        }
        case kpToolSelection::Ellipse:
            document ()->setSelection (kpSelection (kpSelection::Ellipse, normalizedRect,
                                                    mainWindow ()->selectionTransparency ()));
            setUserShapePoints (m_startPoint, m_currentPoint);
            break;
        case kpToolSelection::FreeForm:
            QPointArray points;

            if (document ()->selection ())
                points = document ()->selection ()->points ();


            // (not detached so will modify "points" directly but
            //  still need to call kpDocument::setSelection() to
            //  update screen)

            if (!m_dragHasBegun)
            {
                // We thought the drag at startPoint was a NOP
                // but it turns out that it wasn't...
                points.putPoints (points.count (), 1, m_startPoint.x (), m_startPoint.y ());
            }

            // TODO: there should be an upper limit on this before drawing the
            //       polygon becomes too slow
            points.putPoints (points.count (), 1, thisPoint.x (), thisPoint.y ());


            document ()->setSelection (kpSelection (points, mainWindow ()->selectionTransparency ()));
        #if DEBUG_KP_TOOL_SELECTION && 0
            kdDebug () << "\t\tfreeform; #points=" << document ()->selection ()->points ().count () << endl;
        #endif

            setUserShapePoints (m_currentPoint);
            break;
        }

        viewManager ()->setSelectionBorderVisible (true);
    }
    else if (m_dragType == Move)
    {
    #if DEBUG_KP_TOOL_SELECTION && 0
       kdDebug () << "\tmoving selection" << endl;
    #endif

        if (!document ()->selection ()->pixmap () && !m_currentPullFromDocumentCommand)
        {
            m_currentPullFromDocumentCommand = new kpToolSelectionPullFromDocumentCommand (
                QString::null/*uninteresting child of macro cmd*/,
                mainWindow ());
            m_currentPullFromDocumentCommand->execute ();
        }

        if (!m_currentMoveCommand)
        {
            m_currentMoveCommand = new kpToolSelectionMoveCommand (
                QString::null/*uninteresting child of macro cmd*/,
                mainWindow ());
            m_currentMoveCommandIsSmear = false;
        }


        //viewManager ()->setQueueUpdates ();
        //viewManager ()->setFastUpdates ();

        if (m_shiftPressed)
            m_currentMoveCommandIsSmear = true;

        if (!m_dragHasBegun && (m_controlPressed || m_shiftPressed))
            m_currentMoveCommand->copyOntoDocument ();

    #if DEBUG_KP_TOOL_SELECTION && 0
        kdDebug () << "\t\ttopLeft=" << normalizedRect.topLeft ()
                   << " startPoint=" << m_startPoint
                   << " startDragFromSel=" << m_startDragFromSelectionTopLeft
                   << endl;

        kdDebug () << "\t\tthisPoint=" << thisPoint
                   << " destPoint=" << thisPoint - m_startDragFromSelectionTopLeft
                   << endl;
    #endif
        m_currentMoveCommand->moveTo (thisPoint - m_startDragFromSelectionTopLeft);

        if (m_shiftPressed)
            m_currentMoveCommand->copyOntoDocument ();

        //viewManager ()->restoreFastUpdates ();
        //viewManager ()->restoreQueueUpdates ();

        QPoint start = m_currentMoveCommand->originalSelection ().topLeft ();
        QPoint end = thisPoint - m_startDragFromSelectionTopLeft;
        setUserShapePoints (start, end, false/*don't set size*/);
        setUserShapeSize (end.x () - start.x (), end.y () - start.y ());
    }

    m_dragHasBegun = true;
}

// virtual
void kpToolSelection::cancelShape ()
{
#if DEBUG_KP_TOOL_SELECTION
    kdDebug () << "kpToolSelection::cancelShape() mouseButton=" << m_mouseButton << endl;
#endif

    if (m_mouseButton == 0/*left*/)
    {
        if (m_dragType == Move)
        {
        #if DEBUG_KP_TOOL_SELECTION
            kdDebug () << "\twas drag moving - undo drag and undo acquire" << endl;
        #endif

            if (m_currentMoveCommand)
            {
            #if DEBUG_KP_TOOL_SELECTION
                kdDebug () << "\t\tundo currentMoveCommand" << endl;
            #endif
                m_currentMoveCommand->finalize ();
                m_currentMoveCommand->unexecute ();
                delete m_currentMoveCommand;
                m_currentMoveCommand = 0;

                if (document ()->selection ()->isText ())
                    viewManager ()->setTextCursorBlinkState (true);
            }

            if (m_currentPullFromDocumentCommand)
            {
            #if DEBUG_KP_TOOL_SELECTION
                kdDebug () << "\t\tundo pullFromDocumentCommand" << endl;
            #endif
                m_currentPullFromDocumentCommand->unexecute ();
                delete m_currentPullFromDocumentCommand;
                m_currentPullFromDocumentCommand = 0;
            }
        }
        else if (m_dragType == Create)
        {
        #if DEBUG_KP_TOOL_SELECTION
            kdDebug () << "\twas creating sel - kill" << endl;
        #endif

            // TODO: should we give the user back the selection s/he had before (if any)?
            document ()->selectionDelete ();

            if (m_currentCreateTextCommand)
            {
                delete m_currentCreateTextCommand;
                m_currentCreateTextCommand = 0;
            }
        }

        viewManager ()->setSelectionBorderVisible (true);
        viewManager ()->setSelectionBorderFinished (true);
        viewManager ()->setTextCursorEnabled (m_mode == Text && true);
    }
    else
    {
    #if DEBUG_KP_TOOL_SELECTION
        kdDebug () << "\tstarted draw with right button (which is banned)" << endl;
    #endif
    }

    m_dragType = Unknown;
    m_cancelledShapeButStillHoldingButtons = true;
    setUserMessage (i18n ("Let go of all the mouse buttons."));
}

// virtual
void kpToolSelection::releasedAllButtons ()
{
    m_cancelledShapeButStillHoldingButtons = false;
    setUserMessage (haventBegunDrawUserMessage ());
}

// virtual
void kpToolSelection::endDraw (const QPoint & /*thisPoint*/, const QRect & /*normalizedRect*/)
{
    if (m_mouseButton != 0/*left*/)
        return;

    if (m_currentCreateTextCommand)
    {
        commandHistory ()->addCommand (m_currentCreateTextCommand, false/*no exec*/);
        m_currentCreateTextCommand = 0;
    }

    KMacroCommand *cmd = 0;
    if (m_currentMoveCommand)
    {
        if (m_currentMoveCommandIsSmear)
        {
            cmd = new KMacroCommand (i18n ("%1: Smear")
                                         .arg (document ()->selection ()->name ()));
        }
        else
        {
            cmd = new KMacroCommand (document ()->selection ()->isText () ?
                                         i18n ("Text: Move Box") :
                                         i18n ("Selection: Move"));
        }

        if (document ()->selection ()->isText ())
            viewManager ()->setTextCursorBlinkState (true);
    }

    if (m_currentPullFromDocumentCommand)
    {
        if (!m_currentMoveCommand)
        {
            kdError () << "kpToolSelection::endDraw() pull without move" << endl;
            delete m_currentPullFromDocumentCommand;
            m_currentPullFromDocumentCommand = 0;
        }
        else
        {
            kpSelection selection = m_currentMoveCommand->originalSelection ();

            // just the border
            selection.setPixmap (QPixmap ());

            commandHistory ()->addCommand (new kpToolSelectionCreateCommand (
                i18n ("Selection: Create"),
                selection,
                mainWindow ()),
                false/*no exec - user already dragged out sel*/);


            cmd->addCommand (m_currentPullFromDocumentCommand);
            m_currentPullFromDocumentCommand = 0;
        }
    }

    if (m_currentMoveCommand)
    {
        m_currentMoveCommand->finalize ();
        cmd->addCommand (m_currentMoveCommand);
        m_currentMoveCommand = 0;

        if (document ()->selection ()->isText ())
            viewManager ()->setTextCursorBlinkState (true);
    }

    if (cmd)
        commandHistory ()->addCommand (cmd, false/*no exec*/);

    viewManager ()->setSelectionBorderVisible (true);
    viewManager ()->setSelectionBorderFinished (true);
    viewManager ()->setTextCursorEnabled (m_mode == Text && true);

    m_dragType = Unknown;
    setUserMessage (haventBegunDrawUserMessage ());
}


// protected virtual [base kpTool]
void kpToolSelection::keyPressEvent (QKeyEvent *e)
{
#if DEBUG_KP_TOOL_SELECTION
    kdDebug () << "kpToolSelection::keyPressEvent(e->text='" << e->text () << "')" << endl;
#endif


    e->ignore ();


    if (document ()->selection () &&
        !hasBegunDraw () &&
         e->key () == Qt::Key_Escape)
    {
    #if DEBUG_KP_TOOL_SELECTION
        kdDebug () << "\tescape pressed with sel when not begun draw - deselecting" << endl;
    #endif

        pushOntoDocument ();
        e->accept ();
    }


    if (!e->isAccepted ())
    {
    #if DEBUG_KP_TOOL_SELECTION
        kdDebug () << "\tkey processing did not accept (text was '"
                   << e->text ()
                   << "') - passing on event to kpTool"
                   << endl;
    #endif

        kpTool::keyPressEvent (e);
        return;
    }
}


// private slot
void kpToolSelection::selectionTransparencyChanged (const QString & /*name*/)
{
#if 0
#if DEBUG_KP_TOOL_SELECTION
    kdDebug () << "kpToolSelection::selectionTransparencyChanged(" << name << ")" << endl;
#endif

    if (mainWindow ()->settingSelectionTransparency ())
    {
    #if DEBUG_KP_TOOL_SELECTION
        kdDebug () << "\trecursion - abort setting selection transparency: "
                   << mainWindow ()->settingSelectionTransparency () << endl;
    #endif
        return;
    }

    if (document ()->selection ())
    {
    #if DEBUG_KP_TOOL_SELECTION
        kdDebug () << "\thave sel - set transparency" << endl;
    #endif

        kpSelectionTransparency oldST = document ()->selection ()->transparency ();
        kpSelectionTransparency st = mainWindow ()->selectionTransparency ();

        // TODO: This "NOP" check causes us a great deal of trouble e.g.:
        //
        //       Select a solid red rectangle.
        //       Switch to transparent and set red as the background colour.
        //       (the selection is now invisible)
        //       Invert Colours.
        //       (the selection is now cyan)
        //       Change the background colour to green.
        //       (no command is added to undo this as the selection does not change)
        //       Undo.
        //       The rectangle is no longer invisible.
        //
        //if (document ()->selection ()->setTransparency (st, true/*check harder for no change in mask*/))

        document ()->selection ()->setTransparency (st);
        if (true)
        {
        #if DEBUG_KP_TOOL_SELECTION
            kdDebug () << "\t\twhich changed the pixmap" << endl;
        #endif

            commandHistory ()->addCommand (new kpToolSelectionTransparencyCommand (
                i18n ("Selection: Transparency"), // name,
                st, oldST,
                mainWindow ()),
                false/* no exec*/);
        }
    }
#endif

    // TODO: I've duplicated the code (see below 3x) to make sure
    //       kpSelectionTransparency(oldST)::transparentColor() is defined
    //       and not taken from kpDocument (where it may not be defined because
    //       the transparency may be opaque).
    //
    //       That way kpToolSelectionTransparencyCommand can force set colours.
}


// protected slot virtual
void kpToolSelection::slotIsOpaqueChanged ()
{
#if DEBUG_KP_TOOL_SELECTION
    kdDebug () << "kpToolSelection::slotIsOpaqueChanged()" << endl;
#endif

    if (mainWindow ()->settingSelectionTransparency ())
    {
    #if DEBUG_KP_TOOL_SELECTION
        kdDebug () << "\trecursion - abort setting selection transparency: "
                   << mainWindow ()->settingSelectionTransparency () << endl;
    #endif
        return;
    }

    if (document ()->selection ())
    {
    #if DEBUG_KP_TOOL_SELECTION
        kdDebug () << "\thave sel - set transparency" << endl;
    #endif

        kpSelectionTransparency st = mainWindow ()->selectionTransparency ();
        kpSelectionTransparency oldST = st;
        oldST.setOpaque (!oldST.isOpaque ());

        document ()->selection ()->setTransparency (st);
        commandHistory ()->addCommand (new kpToolSelectionTransparencyCommand (
            i18n ("Selection: Transparency"),
            st, oldST,
            mainWindow ()),
            false/* no exec*/);
    }
}

// protected slot virtual [base kpTool]
void kpToolSelection::slotBackgroundColorChanged (const kpColor &)
{
#if DEBUG_KP_TOOL_SELECTION
    kdDebug () << "kpToolSelection::slotBackgroundColorChanged()" << endl;
#endif

    if (mainWindow ()->settingSelectionTransparency ())
    {
    #if DEBUG_KP_TOOL_SELECTION
        kdDebug () << "\trecursion - abort setting selection transparency: "
                   << mainWindow ()->settingSelectionTransparency () << endl;
    #endif
        return;
    }

    if (document ()->selection ())
    {
    #if DEBUG_KP_TOOL_SELECTION
        kdDebug () << "\thave sel - set transparency" << endl;
    #endif

        kpSelectionTransparency st = mainWindow ()->selectionTransparency ();
        kpSelectionTransparency oldST = st;
        oldST.setTransparentColor (oldBackgroundColor ());

        document ()->selection ()->setTransparency (st);
        commandHistory ()->addCommand (new kpToolSelectionTransparencyCommand (
            i18n ("Selection: Transparency"),
            st, oldST,
            mainWindow ()),
            false/* no exec*/);
    }
}

// protected slot virtual [base kpTool]
void kpToolSelection::slotColorSimilarityChanged (double, int)
{
#if DEBUG_KP_TOOL_SELECTION
    kdDebug () << "kpToolSelection::slotColorSimilarityChanged()" << endl;
#endif

    if (mainWindow ()->settingSelectionTransparency ())
    {
    #if DEBUG_KP_TOOL_SELECTION
        kdDebug () << "\trecursion - abort setting selection transparency: "
                   << mainWindow ()->settingSelectionTransparency () << endl;
    #endif
        return;
    }

    if (document ()->selection ())
    {
    #if DEBUG_KP_TOOL_SELECTION
        kdDebug () << "\thave sel - set transparency" << endl;
    #endif

        kpSelectionTransparency st = mainWindow ()->selectionTransparency ();
        kpSelectionTransparency oldST = st;
        oldST.setColorSimilarity (oldColorSimilarity ());

        document ()->selection ()->setTransparency (st);
        commandHistory ()->addCommand (new kpToolSelectionTransparencyCommand (
            i18n ("Selection: Transparency"),
            st, oldST,
            mainWindow ()),
            false/* no exec*/);
    }
}


/*
 * kpToolSelectionCreateCommand
 */

kpToolSelectionCreateCommand::kpToolSelectionCreateCommand (const QString &name,
                                                            const kpSelection &fromSelection,
                                                            kpMainWindow *mainWindow)
    : m_name (name),
      m_fromSelection (0),
      m_mainWindow (mainWindow),
      m_textRow (0), m_textCol (0)
{
    setFromSelection (fromSelection);
}

// public virtual [base KCommand]
QString kpToolSelectionCreateCommand::name () const
{
    return m_name;
}

kpToolSelectionCreateCommand::~kpToolSelectionCreateCommand ()
{
    delete m_fromSelection;
}


// private
kpDocument *kpToolSelectionCreateCommand::document () const
{
    return m_mainWindow ? m_mainWindow->document () : 0;
}


// public
void kpToolSelectionCreateCommand::setFromSelection (const kpSelection &fromSelection)
{
    delete m_fromSelection;
    m_fromSelection = new kpSelection (fromSelection);
}

// public virtual [base KCommand]
void kpToolSelectionCreateCommand::execute ()
{
#if DEBUG_KP_TOOL_SELECTION
    kdDebug () << "kpToolSelectionCreateCommand::execute()" << endl;
#endif

    kpDocument *doc = document ();
    if (!doc)
    {
        kdError () << "kpToolSelectionCreateCommand::execute() without doc" << endl;
        return;
    }

    if (m_fromSelection)
    {
    #if DEBUG_KP_TOOL_SELECTION
        kdDebug () << "\tusing fromSelection" << endl;
        kdDebug () << "\t\thave sel=" << doc->selection ()
                   << " pixmap=" << (doc->selection () ? doc->selection ()->pixmap () : 0)
                   << endl;
    #endif
        if (!m_fromSelection->isText ())
        {
            if (m_fromSelection->transparency () != m_mainWindow->selectionTransparency ())
                m_mainWindow->setSelectionTransparency (m_fromSelection->transparency ());
        }
        else
        {
            if (m_fromSelection->textStyle () != m_mainWindow->textStyle ())
                m_mainWindow->setTextStyle (m_fromSelection->textStyle ());
        }

        m_mainWindow->viewManager ()->setTextCursorPosition (m_textRow, m_textCol);
        doc->setSelection (*m_fromSelection);

        if (m_mainWindow->tool ())
            m_mainWindow->tool ()->somethingBelowTheCursorChanged ();
    }
}

// public virtual [base KCommand]
void kpToolSelectionCreateCommand::unexecute ()
{
    kpDocument *doc = document ();
    if (!doc)
    {
        kdError () << "kpToolSelectionCreateCommand::unexecute() without doc" << endl;
        return;
    }

    if (!doc->selection ())
    {
        // TODO: be a kdError() after I fix Bug #5 (2 "Selection: Create" Undo entries)
        kdDebug () << "kpToolSelectionCreateCommand::unexecute() without sel region" << endl;
        return;
    }

    m_textRow = m_mainWindow->viewManager ()->textCursorRow ();
    m_textCol = m_mainWindow->viewManager ()->textCursorCol ();

    doc->selectionDelete ();

    if (m_mainWindow->tool ())
        m_mainWindow->tool ()->somethingBelowTheCursorChanged ();
}


/*
 * kpToolSelectionPullFromDocumentCommand
 */

kpToolSelectionPullFromDocumentCommand::kpToolSelectionPullFromDocumentCommand (const QString &name,
                                                                                kpMainWindow *mainWindow)
    : m_name (name),
      m_mainWindow (mainWindow),
      m_backgroundColor (mainWindow ? mainWindow->backgroundColor () : kpColor::invalid),
      m_originalSelectionRegion (0)
{
#if DEBUG_KP_TOOL_SELECTION && 1
    kdDebug () << "kpToolSelectionPullFromDocumentCommand::<ctor>() mainWindow="
               << m_mainWindow
               << endl;
#endif
}

// public virtual [base KCommand]
QString kpToolSelectionPullFromDocumentCommand::name () const
{
    return m_name;
}

kpToolSelectionPullFromDocumentCommand::~kpToolSelectionPullFromDocumentCommand ()
{
    delete m_originalSelectionRegion;
}


// private
kpDocument *kpToolSelectionPullFromDocumentCommand::document () const
{
    return m_mainWindow ? m_mainWindow->document () : 0;
}


// public virtual [base KCommand]
void kpToolSelectionPullFromDocumentCommand::execute ()
{
#if DEBUG_KP_TOOL_SELECTION && 1
    kdDebug () << "kpToolSelectionPullFromDocumentCommand::execute()" << endl;
#endif

    kpDocument *doc = document ();

    if (!doc)
    {
        kdError () << "kpToolSelectionPullFromDocumentCommand::execute() without doc" << endl;
        return;
    }

    kpViewManager *vm = m_mainWindow ? m_mainWindow->viewManager () : 0;
    if (vm)
        vm->setQueueUpdates ();

    // must have selection region but not pixmap
    if (!doc->selection () || doc->selection ()->pixmap ())
    {
        kdError () << "kpToolSelectionPullFromDocumentCommand::execute() sel="
                   << doc->selection ()
                   << " pixmap="
                   << (doc->selection () ? doc->selection ()->pixmap () : 0)
                   << endl;
        if (vm)
            vm->restoreQueueUpdates ();
        return;
    }

    // In case the user CTRL+Z'ed, selected a random region to throw us off
    // and then CTRL+Shift+Z'ed putting us here.  Make sure we pull from the
    // originally requested region - not the random one.
    if (m_originalSelectionRegion)
    {
        if (m_originalSelectionRegion->transparency () != m_mainWindow->selectionTransparency ())
            m_mainWindow->setSelectionTransparency (m_originalSelectionRegion->transparency ());

        doc->setSelection (*m_originalSelectionRegion);
    }

    doc->selectionPullFromDocument (m_backgroundColor);

    if (vm)
        vm->restoreQueueUpdates ();
}

// public virtual [base KCommand]
void kpToolSelectionPullFromDocumentCommand::unexecute ()
{
#if DEBUG_KP_TOOL_SELECTION && 1
    kdDebug () << "kpToolSelectionPullFromDocumentCommand::unexecute()" << endl;
#endif

    kpDocument *doc = document ();

    if (!doc)
    {
        kdError () << "kpToolSelectionPullFromDocumentCommand::unexecute() without doc" << endl;
        return;
    }

    // must have selection pixmap
    if (!doc->selection () || !doc->selection ()->pixmap ())
    {
        kdError () << "kpToolSelectionPullFromDocumentCommand::unexecute() sel="
                   << doc->selection ()
                   << " pixmap="
                   << (doc->selection () ? doc->selection ()->pixmap () : 0)
                   << endl;
        return;
    }


    // We can have faith that this is the state of the selection after
    // execute(), rather than after the user tried to throw us off by
    // simply selecting another region as to do that, a destroy command
    // must have been used.
    doc->selectionCopyOntoDocument (false/*use opaque pixmap*/);
    doc->selection ()->setPixmap (QPixmap ());

    delete m_originalSelectionRegion;
    m_originalSelectionRegion = new kpSelection (*doc->selection ());
}


/*
 * kpToolSelectionTransparencyCommand
 */

kpToolSelectionTransparencyCommand::kpToolSelectionTransparencyCommand (const QString &name,
    const kpSelectionTransparency &st,
    const kpSelectionTransparency &oldST,
    kpMainWindow *mainWindow)
    : m_name (name),
      m_st (st),
      m_oldST (oldST),
      m_mainWindow (mainWindow)
{
}

// public virtual [base KCommand]
QString kpToolSelectionTransparencyCommand::name () const
{
    return m_name;
}

kpToolSelectionTransparencyCommand::~kpToolSelectionTransparencyCommand ()
{
}

// private
kpDocument *kpToolSelectionTransparencyCommand::document () const
{
    return m_mainWindow ? m_mainWindow->document () : 0;
}

// public virtual [base KCommand]
void kpToolSelectionTransparencyCommand::execute ()
{
#if DEBUG_KP_TOOL_SELECTION && 1
    kdDebug () << "kpToolSelectionTransparencyCommand::execute()" << endl;
#endif
    kpDocument *doc = document ();
    if (!doc)
        return;

    m_mainWindow->setSelectionTransparency (m_st, true/*force colour change*/);

    if (doc->selection ())
        doc->selection ()->setTransparency (m_st);
}

// public virtual [base KCommand]
void kpToolSelectionTransparencyCommand::unexecute ()
{
#if DEBUG_KP_TOOL_SELECTION && 1
    kdDebug () << "kpToolSelectionTransparencyCommand::unexecute()" << endl;
#endif

    kpDocument *doc = document ();
    if (!doc)
        return;

    m_mainWindow->setSelectionTransparency (m_oldST, true/*force colour change*/);

    if (doc->selection ())
        doc->selection ()->setTransparency (m_oldST);
}


/*
 * kpToolSelectionMoveCommand
 */

kpToolSelectionMoveCommand::kpToolSelectionMoveCommand (const QString &name,
                                                        kpMainWindow *mainWindow)
    : m_name (name),
      m_mainWindow (mainWindow)
{
    kpDocument *doc = document ();
    if (doc && doc->selection ())
    {
        m_startPoint = m_endPoint = doc->selection ()->topLeft ();
    }
}

// public virtual [base KCommand]
QString kpToolSelectionMoveCommand::name () const
{
    return m_name;
}

kpToolSelectionMoveCommand::~kpToolSelectionMoveCommand ()
{
}


// private
kpDocument *kpToolSelectionMoveCommand::document () const
{
    return m_mainWindow ? m_mainWindow->document () : 0;
}


// public
kpSelection kpToolSelectionMoveCommand::originalSelection () const
{
    kpDocument *doc = document ();
    if (!doc || !doc->selection ())
    {
        kdError () << "kpToolSelectionMoveCommand::originalSelection() doc="
                   << doc
                   << " sel="
                   << (doc ? doc->selection () : 0)
                   << endl;
        return kpSelection (kpSelection::Rectangle, QRect ());
    }

    kpSelection selection = *doc->selection();
    selection.moveTo (m_startPoint);

    return selection;
}


// public virtual [base KCommand]
void kpToolSelectionMoveCommand::execute ()
{
#if DEBUG_KP_TOOL_SELECTION && 1
    kdDebug () << "kpToolSelectionMoveCommand::execute()" << endl;
#endif

    kpDocument *doc = document ();
    if (!doc)
    {
        kdError () << "kpToolSelectionMoveCommand::execute() no doc" << endl;
        return;
    }

    kpSelection *sel = doc->selection ();

    // have to have pulled pixmap by now
    if (!sel || !sel->pixmap ())
    {
        kdError () << "kpToolSelectionMoveCommand::execute() but haven't pulled pixmap yet: "
                   << "sel=" << sel << " sel->pixmap=" << (sel ? sel->pixmap () : 0)
                   << endl;
        return;
    }

    kpViewManager *vm = m_mainWindow ? m_mainWindow->viewManager () : 0;

    if (vm)
        vm->setQueueUpdates ();

    QPointArray::ConstIterator copyOntoDocumentPointsEnd = m_copyOntoDocumentPoints.end ();
    for (QPointArray::ConstIterator it = m_copyOntoDocumentPoints.begin ();
         it != copyOntoDocumentPointsEnd;
         it++)
    {
        sel->moveTo (*it);
        doc->selectionCopyOntoDocument ();
    }

    sel->moveTo (m_endPoint);

    if (m_mainWindow->tool ())
        m_mainWindow->tool ()->somethingBelowTheCursorChanged ();

    if (vm)
        vm->restoreQueueUpdates ();
}

// public virtual [base KCommand]
void kpToolSelectionMoveCommand::unexecute ()
{
#if DEBUG_KP_TOOL_SELECTION && 1
    kdDebug () << "kpToolSelectionMoveCommand::unexecute()" << endl;
#endif

    kpDocument *doc = document ();
    if (!doc)
    {
        kdError () << "kpToolSelectionMoveCommand::unexecute() no doc" << endl;
        return;
    }

    kpSelection *sel = doc->selection ();

    // have to have pulled pixmap by now
    if (!sel || !sel->pixmap ())
    {
        kdError () << "kpToolSelectionMoveCommand::unexecute() but haven't pulled pixmap yet: "
                   << "sel=" << sel << " sel->pixmap=" << (sel ? sel->pixmap () : 0)
                   << endl;
        return;
    }

    kpViewManager *vm = m_mainWindow ? m_mainWindow->viewManager () : 0;

    if (vm)
        vm->setQueueUpdates ();

    if (!m_oldDocumentPixmap.isNull ())
        doc->setPixmapAt (m_oldDocumentPixmap, m_documentBoundingRect.topLeft ());
#if DEBUG_KP_TOOL_SELECTION && 1
    kdDebug () << "\tmove to startPoint=" << m_startPoint << endl;
#endif
    sel->moveTo (m_startPoint);

    if (m_mainWindow->tool ())
        m_mainWindow->tool ()->somethingBelowTheCursorChanged ();

    if (vm)
        vm->restoreQueueUpdates ();
}

// public
void kpToolSelectionMoveCommand::moveTo (const QPoint &point, bool moveLater)
{
#if DEBUG_KP_TOOL_SELECTION && 0
    kdDebug () << "kpToolSelectionMoveCommand::moveTo" << point
               << " moveLater=" << moveLater
               <<endl;
#endif

    if (!moveLater)
    {
        kpDocument *doc = document ();
        if (!doc)
        {
            kdError () << "kpToolSelectionMoveCommand::moveTo() without doc" << endl;
            return;
        }

        kpSelection *sel = doc->selection ();

        // have to have pulled pixmap by now
        if (!sel)
        {
            kdError () << "kpToolSelectionMoveCommand::moveTo() no sel region" << endl;
            return;
        }

        if (!sel->pixmap ())
        {
            kdError () << "kpToolSelectionMoveCommand::moveTo() no sel pixmap" << endl;
            return;
        }

        if (point == sel->topLeft ())
            return;

        sel->moveTo (point);
    }

    m_endPoint = point;
}

// public
void kpToolSelectionMoveCommand::moveTo (int x, int y, bool moveLater)
{
    moveTo (QPoint (x, y), moveLater);
}

// public
void kpToolSelectionMoveCommand::copyOntoDocument ()
{
#if DEBUG_KP_TOOL_SELECTION
    kdDebug () << "kpToolSelectionMoveCommand::copyOntoDocument()" << endl;
#endif

    kpDocument *doc = document ();
    if (!doc)
        return;

    kpSelection *sel = doc->selection ();

    // have to have pulled pixmap by now
    if (!sel)
    {
        kdError () << "\tkpToolSelectionMoveCommand::copyOntoDocument() without sel region" << endl;
        return;
    }

    if (!sel->pixmap ())
    {
        kdError () << "kpToolSelectionMoveCommand::moveTo() no sel pixmap" << endl;
        return;
    }

    if (m_oldDocumentPixmap.isNull ())
        m_oldDocumentPixmap = *doc->pixmap ();

    QRect selBoundingRect = sel->boundingRect ();
    m_documentBoundingRect.unite (selBoundingRect);

    doc->selectionCopyOntoDocument ();

    m_copyOntoDocumentPoints.putPoints (m_copyOntoDocumentPoints.count (),
                                        1,
                                        selBoundingRect.x (),
                                        selBoundingRect.y ());
}

// public
void kpToolSelectionMoveCommand::finalize ()
{
    if (!m_oldDocumentPixmap.isNull () && !m_documentBoundingRect.isNull ())
    {
        m_oldDocumentPixmap = kpTool::neededPixmap (m_oldDocumentPixmap,
                                                    m_documentBoundingRect);
    }
}


/*
 * kpToolSelectionDestroyCommand
 */

kpToolSelectionDestroyCommand::kpToolSelectionDestroyCommand (const QString &name,
                                                              bool pushOntoDocument,
                                                              kpMainWindow *mainWindow)
    : m_name (name),
      m_pushOntoDocument (pushOntoDocument),
      m_oldSelection (0),
      m_mainWindow (mainWindow)
{
}

// public virtual [base KCommand]
QString kpToolSelectionDestroyCommand::name () const
{
    return m_name;
}

kpToolSelectionDestroyCommand::~kpToolSelectionDestroyCommand ()
{
    delete m_oldSelection;
}


// private
kpDocument *kpToolSelectionDestroyCommand::document () const
{
    return m_mainWindow ? m_mainWindow->document () : 0;
}


// public virtual [base KCommand]
void kpToolSelectionDestroyCommand::execute ()
{
#if DEBUG_KP_TOOL_SELECTION
    kdDebug () << "kpToolSelectionDestroyCommand::execute () CALLED" << endl;
#endif

    kpDocument *doc = document ();
    if (!doc)
    {
        kdError () << "kpToolSelectionDestroyCommand::execute() without doc" << endl;
        return;
    }

    if (!doc->selection ())
    {
        kdError () << "kpToolSelectionDestroyCommand::execute() without sel region" << endl;
        return;
    }

    m_textRow = m_mainWindow->viewManager ()->textCursorRow ();
    m_textCol = m_mainWindow->viewManager ()->textCursorCol ();

    m_oldSelection = new kpSelection (*doc->selection ());
    if (m_pushOntoDocument)
    {
        m_oldDocPixmap = doc->getPixmapAt (doc->selection ()->boundingRect ());
        doc->selectionPushOntoDocument ();
    }
    else
        doc->selectionDelete ();

    if (m_mainWindow->tool ())
        m_mainWindow->tool ()->somethingBelowTheCursorChanged ();
}

// public virtual [base KCommand]
void kpToolSelectionDestroyCommand::unexecute ()
{
#if DEBUG_KP_TOOL_SELECTION
    kdDebug () << "kpToolSelectionDestroyCommand::unexecute () CALLED" << endl;
#endif

    kpDocument *doc = document ();
    if (!doc)
    {
        kdError () << "kpToolSelectionDestroyCommand::unexecute() without doc" << endl;
        return;
    }

    if (doc->selection ())
    {
        // not error because it's possible that the user dragged out a new
        // region (without pulling pixmap), and then CTRL+Z
    #if DEBUG_KP_TOOL_SELECTION
        kdDebug () << "kpToolSelectionDestroyCommand::unexecute() already has sel region" << endl;
    #endif

        if (doc->selection ()->pixmap ())
        {
            kdError () << "kpToolSelectionDestroyCommand::unexecute() already has sel pixmap" << endl;
            return;
        }
    }

    if (!m_oldSelection)
    {
        kdError () << "kpToolSelectionDestroyCommand::unexecute() without old sel" << endl;
        return;
    }

    if (m_pushOntoDocument)
    {
    #if DEBUG_KP_TOOL_SELECTION
        kdDebug () << "\tunpush oldDocPixmap onto doc first" << endl;
    #endif
        doc->setPixmapAt (m_oldDocPixmap, m_oldSelection->topLeft ());
    }

#if DEBUG_KP_TOOL_SELECTION
    kdDebug () << "\tsetting selection to: rect=" << m_oldSelection->boundingRect ()
               << " pixmap=" << m_oldSelection->pixmap ()
               << " pixmap.isNull()=" << (m_oldSelection->pixmap ()
                                              ?
                                          m_oldSelection->pixmap ()->isNull ()
                                              :
                                          true)
               << endl;
#endif
    if (!m_oldSelection->isText ())
    {
        if (m_oldSelection->transparency () != m_mainWindow->selectionTransparency ())
            m_mainWindow->setSelectionTransparency (m_oldSelection->transparency ());
    }
    else
    {
        if (m_oldSelection->textStyle () != m_mainWindow->textStyle ())
            m_mainWindow->setTextStyle (m_oldSelection->textStyle ());
    }

    m_mainWindow->viewManager ()->setTextCursorPosition (m_textRow, m_textCol);
    doc->setSelection (*m_oldSelection);

    if (m_mainWindow->tool ())
        m_mainWindow->tool ()->somethingBelowTheCursorChanged ();

    delete m_oldSelection;
    m_oldSelection = 0;
}

#include <kptoolselection.moc>
