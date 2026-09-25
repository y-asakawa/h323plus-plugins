/*
 * h264pipe_static.cxx
 *
 * H.264 static codec for H323plus
 *
 * Copyright (c) 2012 Spranto Australia Pty Ltd. All Rights Reserved.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the General Public License (the  "GNU License"), in which case the
 * provisions of GNU License are applicable instead of those
 * above. If you wish to allow use of your version of this file only
 * under the terms of the GNU License and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GNU License. If you do not delete
 * the provisions above, a recipient may use your version of this file
 * under either the MPL or the GNU License."
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is derived from and used in conjunction with the 
 * H323Plus Project (www.h323plus.org/)
 *
 * The Original Code is H323plus Library.
 *
 * Contributor(s): ______________________________________.
 *
 * $Id$
 *
 */

/* Enable with either _STATIC_LINK or H323_STATIC_H264 for direct x264 linking */
#if defined(_STATIC_LINK) || defined(H323_STATIC_H264)
#include "gpl/enc-ctx.h"

#include "h264pipe_static.h"

#include "../common/trace.h"

// NOTE: 静的ビルド時はプロセス全体で1つのエンコーダコンテキストを共有していたが、
// H.239送信とメイン映像の2本立てでコンテキスト破壊が起きるため、
// インスタンスごとに X264EncoderContext を持つように変更する。

H264EncCtx::H264EncCtx()
  : x264()
  , loaded(false)
{}

H264EncCtx::~H264EncCtx()
{
    InternalUnLoad();
}

bool H264EncCtx::Load()
{  
    return true;
}

bool H264EncCtx::InternalLoad()
{
   if (loaded && x264 != NULL)
       return true;

   InternalUnLoad(); // clean stale

   x264 = std::make_unique<X264EncoderContext>();
   loaded = x264->Initialise();
   if (!loaded)
       x264.reset();
   return loaded;
}

void H264EncCtx::InternalUnLoad()
{
    if (x264) {
        x264->Uninitialise();
        x264.reset();
    }
    loaded = false;
}

void H264EncCtx::call(unsigned msg)
{
  if (!InternalLoad())
      return;

  switch (msg) {
        case H264ENCODERCONTEXT_CREATE:
        // already loaded by InternalLoad()
            break;
        case H264ENCODERCONTEXT_DELETE:
            InternalUnLoad();
            break;
        case APPLY_OPTIONS:
            if (x264)
                x264->ApplyOptions();
            break;
        default:
            break;
  } 
}

void H264EncCtx::call(unsigned msg, unsigned value)
{
  switch (msg) {
    case SET_FRAME_WIDTH:
        if (InternalLoad() && x264) x264->SetFrameWidth(value);
        break;
    case SET_FRAME_HEIGHT:
        if (InternalLoad() && x264) x264->SetFrameHeight(value);
        break;
    case SET_FRAME_RATE:
        if (InternalLoad() && x264) x264->SetFrameRate(value);
        break;
    case SET_MAX_KEY_FRAME_PERIOD:
        if (InternalLoad() && x264) x264->SetMaxKeyFramePeriod(value);
        break;
    case SET_TSTO:
        if (InternalLoad() && x264) x264->SetTSTO(value);
        break;
    case SET_PROFILE_LEVEL:
        if (InternalLoad() && x264) x264->SetProfileLevel(value);
        break;
    default:
        break;
   }
}

void H264EncCtx::call(unsigned msg , const u_char * src, unsigned & srcLen, u_char * dst, unsigned & dstLen, unsigned & headerLen, unsigned int & flags, int & ret)
{
  if (!InternalLoad() || x264 == NULL)
      return;

  switch (msg) {
    case ENCODE_FRAMES:
    case ENCODE_FRAMES_BUFFERED:
        ret = x264->EncodeFrames( src,  srcLen, dst, dstLen, flags);
        break;
    default:
        break;
  }
}

#endif // _STATIC_LINK || H323_STATIC_H264

