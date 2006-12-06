
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


#include <kpcoloreffect.h>

#include <kdialog.h>
#include <klocale.h>

#include <kpdefs.h>
#include <kpdocument.h>
#include <kpmainwindow.h>
#include <kpselection.h>
#include <kpSetOverrideCursorSaver.h>


struct kpColorEffectCommandPrivate
{
    QString name;
    bool actOnSelection;

    kpImage oldImage;
};

kpColorEffectCommand::kpColorEffectCommand (const QString &name,
                                            bool actOnSelection,
                                            kpMainWindow *mainWindow)
    : kpCommand (mainWindow),
      d (new kpColorEffectCommandPrivate ())
{
    d->name = name;
    d->actOnSelection = actOnSelection;
}

kpColorEffectCommand::~kpColorEffectCommand ()
{
    delete d;
}


// public virtual [base kpCommand]
QString kpColorEffectCommand::name () const
{
    if (d->actOnSelection)
        return i18n ("Selection: %1", d->name);
    else
        return d->name;
}


// public virtual [base kpCommand]
int kpColorEffectCommand::size () const
{
    return kpPixmapFX::pixmapSize (d->oldImage);
}


// public virtual [base kpCommand]
void kpColorEffectCommand::execute ()
{
    kpSetOverrideCursorSaver cursorSaver (Qt::WaitCursor);
    
    kpDocument *doc = document ();
    Q_ASSERT (doc);


    const kpImage oldImage = *doc->pixmap (d->actOnSelection);

    if (!isInvertible ())
    {
        d->oldImage = oldImage;
    }


    kpImage newImage = /*pure virtual*/applyColorEffect (oldImage);

    doc->setPixmap (d->actOnSelection, newImage);
}

// public virtual [base kpCommand]
void kpColorEffectCommand::unexecute ()
{
    kpSetOverrideCursorSaver cursorSaver (Qt::WaitCursor);
    
    kpDocument *doc = document ();
    Q_ASSERT (doc);


    kpImage newImage;

    if (!isInvertible ())
    {
        newImage = d->oldImage;
    }
    else
    {
        newImage = /*pure virtual*/applyColorEffect (*doc->pixmap (d->actOnSelection));
    }

    doc->setPixmap (d->actOnSelection, newImage);


    d->oldImage = kpImage ();
}


#include <kpcoloreffect.moc>
