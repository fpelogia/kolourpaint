
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


#ifndef __kp_defs_h__
#define __kp_defs_h__


#include <qstring.h>


// TODO: actually use these
#define KP_BIG_IMAGE_WIDTH 4000
#define KP_BIG_IMAGE_HEIGHT 4000

#define KP_PI 3.141592653589793238462
#define KP_EPSILON 0.0001


#define KP_DEGREES_TO_RADIANS(deg) ((deg) * KP_PI / 180.0)
#define KP_RADIANS_TO_DEGREES(rad) ((rad) * 180.0 / KP_PI)


//
// Settings
//

#define kpSettingsGroupGeneral QString::fromLatin1 ("General Settings")
#define kpSettingFirstTime QString::fromLatin1 ("First Time")
#define kpSettingShowGrid QString::fromLatin1 ("Show Grid")
#define kpSettingShowPath QString::fromLatin1 ("Show Path")
#define kpSettingDefaultOutputMimetype QString::fromLatin1 ("Default Output Mimetype")
#define kpSettingColorSimilarity QString::fromLatin1 ("Color Similarity")

#define kpSettingsGroupText QString::fromLatin1 ("Text Settings")
#define kpSettingFontFamily QString::fromLatin1 ("Font Family")
#define kpSettingFontSize QString::fromLatin1 ("Font Size")
#define kpSettingBold QString::fromLatin1 ("Bold")
#define kpSettingItalic QString::fromLatin1 ("Italic")
#define kpSettingUnderline QString::fromLatin1 ("Underline")
#define kpSettingStrikeThru QString::fromLatin1 ("Strike Thru")


#endif  // __kp_defs_h__

