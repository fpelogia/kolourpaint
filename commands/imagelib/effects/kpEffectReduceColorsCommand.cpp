
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


#define DEBUG_KP_EFFECT_REDUCE_COLORS 0


#include <kpEffectReduceColorsCommand.h>

#include <qbitmap.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qimage.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qradiobutton.h>

#include <kdebug.h>
#include <klocale.h>

#include <kpEffectReduceColors.h>
#include <kppixmapfx.h>


kpEffectReduceColorsCommand::kpEffectReduceColorsCommand (int depth, bool dither,
                                                          bool actOnSelection,
                                                          kpMainWindow *mainWindow)
    : kpEffectCommandBase (commandName (depth, dither), actOnSelection, mainWindow),
      m_depth (depth), m_dither (dither)
{
}

kpEffectReduceColorsCommand::~kpEffectReduceColorsCommand ()
{
}


// public
QString kpEffectReduceColorsCommand::commandName (int depth, int dither) const
{
    if (depth == 1)
    {
        if (dither)
            return i18n ("Reduce to Monochrome (Dithered)");
        else
            return i18n ("Reduce to Monochrome");
    }
    else if (depth == 8)
    {
        if (dither)
            return i18n ("Reduce to 256 Color (Dithered)");
        else
            return i18n ("Reduce to 256 Color");
    }
    else
    {
        return QString::null;
    }
}


//
// kpEffectReduceColorsCommand implements kpEffectCommandBase interface
//

// protected virtual [base kpEffectCommandBase]
kpImage kpEffectReduceColorsCommand::applyEffect (const kpImage &image)
{
    return kpEffectReduceColors::applyEffect (image, m_depth, m_dither);
}


#include <kpEffectReduceColorsCommand.moc>
