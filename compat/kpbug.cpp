
/*
   Copyright (c) 2003-2006 Clarence Dang <dang@kde.org>
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


#define DEBUG_KP_BUG 1


#include <kpbug.h>

#include <qabstractbutton.h>
#include <qbuttongroup.h>
#include <qpoint.h>
#include <qrect.h>

#include <kdebug.h>


// public static
QAbstractButton *kpBug::QButtonGroup_CheckedButton (const QButtonGroup *buttonGroup)
{
    QAbstractButton *button = buttonGroup->checkedButton ();
    QAbstractButton *ret = (button && button->isChecked () ? button : 0);
#if DEBUG_KP_BUG
    kDebug () << "kpBug::QButtonGroup_CheckedButton(" << buttonGroup << ")"
               << " QButtonGroup::checkedButton() would return " << button
               << " but we return " << ret
               << endl;
#endif
    return ret;
}


// public static
QRect kpBug::QRect_Normalized (const QRect &rect)
{
    const QPoint topLeft (qMin (rect.left (), rect.right ()),
                          qMin (rect.top (), rect.bottom ()));
    const QPoint bottomRight (qMax (rect.left (), rect.right ()),
                              qMax (rect.top (), rect.bottom ()));

    const QRect out (topLeft, bottomRight);
#if DEBUG_KP_BUG
    kDebug () << "kpBug::QRect_Normalized("
               << rect.topLeft () << "," << rect.bottomRight ()
               << ") returning ("
               << out.topLeft () << "," << out.bottomRight ()
               << ")" << endl;
#endif

    return out;
}