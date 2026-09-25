/*
 * H.263 Plugin codec for OpenH323/OPAL
 *
 * This code is based on the following files from the OPAL project which
 * have been removed from the current build and distributions but are still
 * available in the CVS "attic"
 *
 *    src/codecs/h263codec.cxx
 *    include/codecs/h263codec.h

 * The original files, and this version of the original code, are released under the same
 * MPL 1.0 license. Substantial portions of the original code were contributed
 * by Salyens and March Networks and their right to be identified as copyright holders
 * of the original code portions and any parts now included in this new copy is asserted through
 * their inclusion in the copyright notices below.
 *
 * Copyright (C) 2006 Post Increment
 * Copyright (C) 2005 Salyens
 * Copyright (C) 2001 March Networks Corporation
 * Copyright (C) 1999-2000 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Guilhem Tardy (gtardy@salyens.com)
 *                 Craig Southeren (craigs@postincrement.com)
 *                 Y. Asakawa (2025) - Updated for modern FFmpeg 5.x/6.x API
 *
 * $Id$
 */

/*
  Notes
  -----

  This codec implements a H.263 encoder and decoder with RTP packaging as per
  RFC 2190 "RTP Payload Format for H.263 Video Streams". As per this specification,
  The RTP payload code is always set to 34

 */

#define _CRT_NONSTDC_NO_DEPRECATE 1
#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef SOLARIS
#include <alloca.h>
#endif

#ifndef PLUGIN_CODEC_DLL_EXPORTS
#include "plugin-config.h"
#endif

#include <codec/opalplugin.h>

#if defined(_WIN32) || defined(_WIN64) || defined(_WIN32_WCE)
  #include <windows.h>
  #define STRCMPI  _strcmpi
#else
  #include <limits.h>
  #include <semaphore.h>
  #include <dlfcn.h>
  #define STRCMPI  strcasecmp
  typedef unsigned char BYTE;
#endif

#include "../common/trace.h"
#include "../common/dyna.h"

#ifdef _MSC_VER
#pragma warning(disable:4800)
#endif

extern "C" {
#ifdef LIBAVCODEC_HEADER
  #include LIBAVCODEC_HEADER
#else
  #include <libavcodec/avcodec.h>
#endif
#if LIBAVCODEC_VERSION_MAJOR >= 58
  #include <libavformat/avformat.h>
  #include <libavutil/opt.h>
#endif
};

// Use FF_CodecID for compatibility with common/dyna.h
#ifndef CODEC_ID_H263
  #define CODEC_ID_H263 AV_CODEC_ID_H263
#endif
#ifndef CODEC_ID_H263P
  #define CODEC_ID_H263P AV_CODEC_ID_H263P
#endif

#  ifdef  _WIN32
#    define P_DEFAULT_PLUGIN_DIR "C:\\PTLIB_PLUGINS;C:\\PWLIB_PLUGINS"
#    define DIR_SEPERATOR "\\"
#    define DIR_TOKENISER ";"
#  else
#    define P_DEFAULT_PLUGIN_DIR "/usr/lib/ptlib:/usr/lib/pwlib"
#    define DIR_SEPERATOR "/"
#    define DIR_TOKENISER ":"
#  endif

#include <vector>

// if defined, the FFMPEG code is access via another DLL
// otherwise, the FFMPEG code is assumed to be statically linked into this plugin
// USE_DLL_AVCODEC - disabled for modern FFmpeg, now using direct linking
// #define USE_DLL_AVCODEC   1

#define RTP_RFC2190_PAYLOAD  34
#define RTP_DYNAMIC_PAYLOAD  96

#define H263_CLOCKRATE    90000
#define H263_BITRATE      327600

#define CIF_WIDTH       352
#define CIF_HEIGHT      288

#define CIF4_WIDTH      (CIF_WIDTH*2)
#define CIF4_HEIGHT     (CIF_HEIGHT*2)

#define CIF16_WIDTH     (CIF_WIDTH*4)
#define CIF16_HEIGHT    (CIF_HEIGHT*4)

#define QCIF_WIDTH     (CIF_WIDTH/2)
#define QCIF_HEIGHT    (CIF_HEIGHT/2)

#define SQCIF_WIDTH     128
#define SQCIF_HEIGHT    96

#define MAX_H263_PACKET_SIZE     10000
#ifndef FF_INPUT_BUFFER_PADDING_SIZE
  #define FF_INPUT_BUFFER_PADDING_SIZE AV_INPUT_BUFFER_PADDING_SIZE
#endif
#define MAX_YUV420P_PACKET_SIZE (((CIF16_WIDTH * CIF16_HEIGHT * 3) / 2) + FF_INPUT_BUFFER_PADDING_SIZE)

#define MIN(v1, v2) ((v1) < (v2) ? (v1) : (v2))
#define MAX(v1, v2) ((v1) > (v2) ? (v1) : (v2))

// Compatibility defines for removed FFmpeg flags
#ifndef CODEC_FLAG_INPUT_PRESERVED
  #define CODEC_FLAG_INPUT_PRESERVED 0
#endif
#ifndef CODEC_FLAG_EMU_EDGE
  #define CODEC_FLAG_EMU_EDGE 0
#endif
#ifndef CODEC_FLAG_PASS1
  #define CODEC_FLAG_PASS1 AV_CODEC_FLAG_PASS1
#endif
#ifndef CODEC_FLAG_4MV
  #ifdef AV_CODEC_FLAG_4MV
    #define CODEC_FLAG_4MV AV_CODEC_FLAG_4MV
  #else
    #define CODEC_FLAG_4MV 0
  #endif
#endif
#ifndef CODEC_FLAG_H263P_UMV
  #define CODEC_FLAG_H263P_UMV 0
#endif
#ifndef CODEC_FLAG_H263P_AIC
  #define CODEC_FLAG_H263P_AIC 0
#endif
#ifndef CODEC_FLAG_RFC2190
  #define CODEC_FLAG_RFC2190 0
#endif
#ifndef FF_MB_DECISION_SIMPLE
  #define FF_MB_DECISION_SIMPLE 0
#endif



static struct StdSizes {
  enum {
    SQCIF,
    QCIF,
    CIF,
    CIF4,
    CIF16,
    NumStdSizes,
    UnknownStdSize = NumStdSizes
  };

  int width;
  int height;
  const char * optionName;
} StandardVideoSizes[StdSizes::NumStdSizes] = {
  { SQCIF_WIDTH, SQCIF_HEIGHT, PLUGINCODEC_SQCIF_MPI },
  {  QCIF_WIDTH,  QCIF_HEIGHT, PLUGINCODEC_QCIF_MPI  },
  {   CIF_WIDTH,   CIF_HEIGHT, PLUGINCODEC_CIF_MPI   },
  {  CIF4_WIDTH,  CIF4_HEIGHT, PLUGINCODEC_CIF4_MPI  },
  { CIF16_WIDTH, CIF16_HEIGHT, PLUGINCODEC_CIF16_MPI },
};

// Use FFMPEGLibrary from common/dyna.h
// Create the global instance with H.263 codec ID
static FFMPEGLibrary FFMPEGLibraryInstance(CODEC_ID_H263);

/////////////////////////////////////////////////////////////////////////////
//
// define some simple RTP packet routines
//

#define RTP_MIN_HEADER_SIZE 12

class RTPFrame
{
  public:
    RTPFrame(const unsigned char * _packet, int _maxPacketLen)
      : packet((unsigned char *)_packet), maxPacketLen(_maxPacketLen), packetLen(_maxPacketLen)
    {
    }

    RTPFrame(unsigned char * _packet, int _maxPacketLen, unsigned char payloadType)
      : packet(_packet), maxPacketLen(_maxPacketLen), packetLen(_maxPacketLen)
    {
      if (packetLen > 0)
        packet[0] = 0x80;    // set version, no extensions, zero contrib count
      SetPayloadType(payloadType);
    }

    inline unsigned long GetLong(unsigned offs) const
    {
      if (offs + 4 > packetLen)
        return 0;
      return (packet[offs + 0] << 24) + (packet[offs+1] << 16) + (packet[offs+2] << 8) + packet[offs+3];
    }

    inline void SetLong(unsigned offs, unsigned long n)
    {
      if (offs + 4 <= packetLen) {
        packet[offs + 0] = (BYTE)((n >> 24) & 0xff);
        packet[offs + 1] = (BYTE)((n >> 16) & 0xff);
        packet[offs + 2] = (BYTE)((n >> 8) & 0xff);
        packet[offs + 3] = (BYTE)(n & 0xff);
      }
    }

    inline unsigned short GetShort(unsigned offs) const
    {
      if (offs + 2 > packetLen)
        return 0;
      return (packet[offs + 0] << 8) + packet[offs + 1];
    }

    inline void SetShort(unsigned offs, unsigned short n)
    {
      if (offs + 2 <= packetLen) {
        packet[offs + 0] = (BYTE)((n >> 8) & 0xff);
        packet[offs + 1] = (BYTE)(n & 0xff);
      }
    }

    inline int GetPacketLen() const                    { return packetLen; }
    inline int GetMaxPacketLen() const                 { return maxPacketLen; }
    inline unsigned GetVersion() const                 { return (packetLen < 1) ? 0 : (packet[0]>>6)&3; }
    inline bool GetExtension() const                   { return (packetLen < 1) ? 0 : (packet[0]&0x10) != 0; }
    inline bool GetMarker()  const                     { return (packetLen < 2) ? false : ((packet[1]&0x80) != 0); }
    inline unsigned char GetPayloadType() const        { return (packetLen < 2) ? false : (packet[1] & 0x7f);  }
    inline unsigned short GetSequenceNumber() const    { return GetShort(2); }
    inline unsigned long GetTimestamp() const          { return GetLong(4); }
    inline unsigned long GetSyncSource() const         { return GetLong(8); }
    inline int GetContribSrcCount() const              { return (packetLen < 1) ? 0  : (packet[0]&0xf); }
    inline int GetExtensionSize() const                { return !GetExtension() ? 0  : GetShort(RTP_MIN_HEADER_SIZE + 4*GetContribSrcCount() + 2); }
    inline int GetExtensionType() const                { return !GetExtension() ? -1 : GetShort(RTP_MIN_HEADER_SIZE + 4*GetContribSrcCount()); }
    inline int GetPayloadSize() const                  { return packetLen - GetHeaderSize(); }
    inline unsigned char * GetPayloadPtr() const       { return packet + GetHeaderSize(); }

    inline unsigned int GetHeaderSize() const
    {
      unsigned int sz = RTP_MIN_HEADER_SIZE + 4*GetContribSrcCount();
      if (GetExtension())
        sz += 4 + GetExtensionSize();
      return sz;
    }

    inline void SetMarker(bool m)                    { if (packetLen >= 2) packet[1] = (packet[1] & 0x7f) | (m ? 0x80 : 0x00); }
    inline void SetPayloadType(unsigned char t)      { if (packetLen >= 2) packet[1] = (packet[1] & 0x80) | (t & 0x7f); }
    inline void SetSequenceNumber(unsigned short v)  { SetShort(2, v); }
    inline void SetTimestamp(unsigned long n)        { SetLong(4, n); }
    inline void SetSyncSource(unsigned long n)       { SetLong(8, n); }

    inline bool SetPayloadSize(int payloadSize)
    {
      if (GetHeaderSize() + payloadSize > maxPacketLen)
        return false;
      packetLen = GetHeaderSize() + payloadSize;
      return true;
    }

  protected:
    unsigned char * packet;
    unsigned maxPacketLen;
    unsigned packetLen;
};

/////////////////////////////////////////////////////////////////////////////

class H263Packet
{
  public:
    H263Packet() { data_size = hdr_size = 0; hdr = data = NULL; };
    ~H263Packet() {};

    void Store(void * _data, int _data_size, void * _hdr, int _hdr_size)
    {
      data      = _data;
      data_size = _data_size;
      hdr       = _hdr;
      hdr_size  = _hdr_size;
    }

    int Read(RTPFrame & frame)
    {
      if (!frame.SetPayloadSize(hdr_size + data_size)) {
        //PTRACE(1, "H263Pck\tNot enough memory for packet of " << length << " bytes");
        return -1;
      }
      memcpy(frame.GetPayloadPtr(), hdr, hdr_size);
      memcpy(frame.GetPayloadPtr() + hdr_size, data, data_size);

      const unsigned char * packet = (const unsigned char *)data;

      data = NULL;
      hdr = NULL;

      if (packet[0] != 0 || packet[1] != 0 || (packet[2]&0xfc) != 0x80)
        return 0;

      if ((packet[4]&0x1c) != 0x1c) // Baseline?
        return (packet[4]&2) == 0 ? 1 : 0;

      // PLUSPTYPE
      if ((packet[5]&0x80) == 0)
        return (packet[5]&0x70) == 0 ? 1 : 0;

      // PLUSPTYPE with UFEP
      return (packet[7]&0x1C) == 0 ? 1 : 0;
    }

  private:
    void *data;
    int data_size;
    void *hdr;
    int hdr_size;
};

class H263EncoderContext
{
  public:
    typedef std::vector<H263Packet *> H263PacketList;
    static void RtpCallback(void *data, int data_size,
                            void *hdr, int hdr_size, void *priv_data);

    H263EncoderContext();
    ~H263EncoderContext();
    int EncodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags);

    bool OpenCodec();
    void CloseCodec();

    unsigned GetNextEncodedPacket(RTPFrame & dstRTP, unsigned char payloadCode, unsigned long lastTimeStamp, unsigned & flags);

    H263PacketList encodedPackets;
    H263PacketList unusedPackets;

    unsigned char encFrameBuffer[MAX_YUV420P_PACKET_SIZE];
    int encFrameLen;

    unsigned char rawFrameBuffer[MAX_YUV420P_PACKET_SIZE];
    int rawFrameLen;

    AVCodec        *avcodec;
    AVCodecContext *avcontext;
    AVFrame        *avpicture;
#if LIBAVCODEC_VERSION_MAJOR >= 58
    AVFormatContext *rtpMuxer;
    std::vector<std::vector<BYTE> > rtpPackets;
    bool lastFrameIsKey;
    bool firstPacketOfFrame;
#if LIBAVFORMAT_VERSION_MAJOR < 60
    static int WriteRtpPacket(void *opaque, BYTE *buf, int size);
#else
    static int WriteRtpPacket(void *opaque, const BYTE *buf, int size);
#endif
    bool OpenRtpMuxer();
    unsigned GetNextModernPacket(RTPFrame &dstRTP, unsigned &flags);
#endif

    int videoQMax, videoQMin; // dynamic video quality min/max limits, 1..31
    int videoQuality; // current video encode quality setting, 1..31
    int frameNum;
    unsigned frameWidth, frameHeight;
    unsigned long lastTimeStamp;
    unsigned bitRate;
    unsigned frameRate;

    void Lock() { _mutex.Wait(); }
    void Unlock() { _mutex.Signal(); }

    static int GetStdSize(int width, int height)
    {
      int sizeIndex;
      for (sizeIndex = 0; sizeIndex < StdSizes::NumStdSizes; ++sizeIndex )
        if (StandardVideoSizes[sizeIndex].width == width && StandardVideoSizes[sizeIndex].height == height )
          return sizeIndex;
      return StdSizes::UnknownStdSize;
    }

  protected:
		CriticalSection _mutex;
};

H263EncoderContext::H263EncoderContext()
{
  avcodec = NULL;
  avcontext = NULL;
  avpicture = NULL;
#if LIBAVCODEC_VERSION_MAJOR >= 58
  rtpMuxer = NULL;
  lastFrameIsKey = false;
  firstPacketOfFrame = false;
#endif
  frameNum = 0;
  if (!FFMPEGLibraryInstance.IsLoaded())
    return;

  if ((avcodec = FFMPEGLibraryInstance.AvcodecFindEncoder(CODEC_ID_H263)) == NULL) {
    //PTRACE(1, "H263\tCodec not found for encoder");
    return;
  }

  frameWidth  = CIF_WIDTH;
  frameHeight = CIF_HEIGHT;
  rawFrameLen = (CIF_HEIGHT * CIF_WIDTH * 3) / 2;

  avcontext = FFMPEGLibraryInstance.AvcodecAllocContext();
  if (avcontext == NULL) {
    //PTRACE(1, "H263\tFailed to allocate context for encoder");
    return;
  }

  avpicture = FFMPEGLibraryInstance.AvcodecAllocFrame();
  if (avpicture == NULL) {
    //PTRACE(1, "H263\tFailed to allocate frame for encoder");
    return;
  }

  avcontext->codec = NULL;

  // set some reasonable values for quality as default
  videoQuality = 20;
  videoQMin = 10;
  videoQMax = 31;
  frameNum = 0;
  bitRate = 327600;
  frameRate = 15;

  //PTRACE(3, "Codec\tH263 encoder created");
}

H263EncoderContext::~H263EncoderContext()
{
  WaitAndSignal m(_mutex);

  if (FFMPEGLibraryInstance.IsLoaded()) {
#if LIBAVCODEC_VERSION_MAJOR >= 58
    if (rtpMuxer != NULL) {
      avio_context_free(&rtpMuxer->pb);
      avformat_free_context(rtpMuxer);
    }
    avcodec_free_context(&avcontext);
    av_frame_free(&avpicture);
#else
    CloseCodec();

    FFMPEGLibraryInstance.AvcodecFree(avcontext);
    FFMPEGLibraryInstance.AvcodecFree(avpicture);
#endif

    while (encodedPackets.size() > 0) {
      delete *encodedPackets.begin();
      encodedPackets.erase(encodedPackets.begin());
    }
    while (unusedPackets.size() > 0) {
      delete *unusedPackets.begin();
      unusedPackets.erase(unusedPackets.begin());
    }
  }
}

bool H263EncoderContext::OpenCodec()
{
  if (avcontext == NULL || avpicture == NULL || avcodec == NULL)
    return false;
  // avoid copying input/output
  avcontext->flags |= CODEC_FLAG_INPUT_PRESERVED; // we guarantee to preserve input for max_b_frames+1 frames
  avcontext->flags |= CODEC_FLAG_EMU_EDGE; // don't draw edges

  avcontext->width  = frameWidth;
  avcontext->height = frameHeight;
  avcontext->pix_fmt = AV_PIX_FMT_YUV420P;

  avpicture->linesize[0] = frameWidth;
  avpicture->linesize[1] = frameWidth / 2;
  avpicture->linesize[2] = frameWidth / 2;
  avpicture->width = frameWidth;
  avpicture->height = frameHeight;
  avpicture->format = AV_PIX_FMT_YUV420P;

  int _bitRate = bitRate;
  avcontext->bit_rate = (_bitRate * 3) >> 2; // average bit rate
  avcontext->bit_rate_tolerance = _bitRate >> 1;
  avcontext->rc_min_rate = 0;               // minimum bitrate
  avcontext->rc_max_rate = _bitRate;         // maximum bitrate
  avcontext->rc_buffer_size = _bitRate;      // matching one-second VBV buffer
  avcontext->qmin = videoQMin;
  avcontext->qmax = videoQMax;
  avcontext->max_qdiff = 3; // max q difference between frames

  avcontext->qcompress = 0.5; // qscale factor between easy & hard scenes (0.0-1.0)
  avcontext->i_quant_factor = (float)-0.6; // qscale factor between p and i frames
  avcontext->i_quant_offset = (float)0.0; // qscale offset between p and i frames

  avcontext->flags |= CODEC_FLAG_PASS1;

  avcontext->mb_decision = FF_MB_DECISION_SIMPLE; // choose only one MB type at a time

  // Use time_base instead of frame_rate
  avcontext->time_base.num = 1;
  avcontext->time_base.den = frameRate;

  avcontext->gop_size = 125;

  avcontext->flags &= ~CODEC_FLAG_H263P_UMV;
  avcontext->flags &= ~CODEC_FLAG_4MV;
  avcontext->max_b_frames = 0;
  avcontext->flags &= ~CODEC_FLAG_H263P_AIC; // advanced intra coding (not handled by H323_FFH263Capability)

  avcontext->flags |= CODEC_FLAG_RFC2190;

  // RTP callback removed in modern FFmpeg - we'll handle packetization differently
  avcontext->opaque = this; // used to separate out packets from different encode threads

  if (FFMPEGLibraryInstance.AvcodecOpen(avcontext, avcodec) != 0)
    return false;
#if LIBAVCODEC_VERSION_MAJOR >= 58
  // FFmpeg supplies macroblock boundaries to its RFC 2190 RTP muxer.
  if (av_opt_set_int(avcontext->priv_data, "mb_info", 1200, 0) < 0)
    return false;
  return OpenRtpMuxer();
#else
  return true;
#endif
}

void H263EncoderContext::CloseCodec()
{
#if LIBAVCODEC_VERSION_MAJOR >= 58
  if (rtpMuxer != NULL) {
    avio_context_free(&rtpMuxer->pb);
    avformat_free_context(rtpMuxer);
    rtpMuxer = NULL;
  }
  rtpPackets.clear();
  avcodec_free_context(&avcontext);
  avcontext = FFMPEGLibraryInstance.AvcodecAllocContext();
#else
  if (avcontext != NULL) {
    if (avcontext->codec != NULL) {
      FFMPEGLibraryInstance.AvcodecClose(avcontext);
      //PTRACE(5, "H263\tClosed H.263 encoder" );
    }
  }
#endif
}

#if LIBAVCODEC_VERSION_MAJOR >= 58
#if LIBAVFORMAT_VERSION_MAJOR < 60
int H263EncoderContext::WriteRtpPacket(void *opaque, BYTE *buf, int size)
#else
int H263EncoderContext::WriteRtpPacket(void *opaque, const BYTE *buf, int size)
#endif
{
  H263EncoderContext *context = static_cast<H263EncoderContext *>(opaque);
  if (size >= 12 && (buf[0] & 0xc0) == 0x80)
    context->rtpPackets.push_back(std::vector<BYTE>(buf, buf + size));
  return size;
}

bool H263EncoderContext::OpenRtpMuxer()
{
  if (avformat_alloc_output_context2(&rtpMuxer, NULL, "rtp", NULL) < 0 || rtpMuxer == NULL)
    return false;
  AVStream *stream = avformat_new_stream(rtpMuxer, NULL);
  if (stream == NULL || avcodec_parameters_from_context(stream->codecpar, avcontext) < 0)
    return false;
  stream->time_base.num = 1;
  stream->time_base.den = 90000;
  BYTE *buffer = static_cast<BYTE *>(av_malloc(1200));
  if (buffer == NULL)
    return false;
  rtpMuxer->pb = avio_alloc_context(buffer, 1200, 1, this, NULL, WriteRtpPacket, NULL);
  if (rtpMuxer->pb == NULL) {
    av_free(buffer);
    return false;
  }
  rtpMuxer->pb->max_packet_size = 1200;
  rtpMuxer->packet_size = 1200;
  rtpMuxer->flags |= AVFMT_FLAG_CUSTOM_IO;
  if (av_opt_set_int(rtpMuxer->priv_data, "rtpflags", 2 | 4, 0) < 0)
    return false; // RFC 2190, with RTCP suppressed.
  return avformat_write_header(rtpMuxer, NULL) >= 0;
}

unsigned H263EncoderContext::GetNextModernPacket(RTPFrame &dstRTP, unsigned &flags)
{
  if (rtpPackets.empty())
    return 0;
  const std::vector<BYTE> &packet = rtpPackets.front();
  const unsigned payloadSize = packet.size() - 12;
  if (payloadSize + 12 > dstRTP.GetMaxPacketLen()) {
    flags |= PluginCodec_ReturnCoderBufferTooSmall;
    return 0;
  }
  memcpy(dstRTP.GetPayloadPtr(), packet.data() + 12, payloadSize);
  dstRTP.SetPayloadSize(payloadSize);
  dstRTP.SetMarker((packet[1] & 0x80) != 0);
  dstRTP.SetTimestamp(lastTimeStamp);
  if (dstRTP.GetMarker())
    flags |= PluginCodec_ReturnCoderLastFrame;
  if (lastFrameIsKey && firstPacketOfFrame)
    flags |= PluginCodec_ReturnCoderIFrame;
  firstPacketOfFrame = false;
  rtpPackets.erase(rtpPackets.begin());
  return dstRTP.GetPacketLen();
}
#endif

void H263EncoderContext::RtpCallback(void *data, int data_size, void *hdr, int hdr_size, void *priv_data)
{
  H263EncoderContext *c = (H263EncoderContext *) priv_data;
  H263Packet *p;
  if (c->unusedPackets.size() == 0)
    p = new H263Packet();
  else {
    p = *c->unusedPackets.begin();
    c->unusedPackets.erase(c->unusedPackets.begin());
  }
  p->Store(data, data_size, hdr, hdr_size);
  c->encodedPackets.push_back(p);
}

unsigned int H263EncoderContext::GetNextEncodedPacket(RTPFrame & dstRTP, unsigned char payloadCode, unsigned long lastTimeStamp, unsigned & flags)
{
  if (encodedPackets.size() == 0)
    return 0;

  // get the next packet from the unencoded list
  H263Packet *p = *encodedPackets.begin();
  encodedPackets.erase(encodedPackets.begin());

  // this packet will be shortly unused
  unusedPackets.push_back(p);

  // if the packet is too long, throw it away
  switch (p->Read(dstRTP)) {
    case -1:
      return 0;
    case 1 :
      flags |= PluginCodec_ReturnCoderIFrame;
    default:;
  }

  if (encodedPackets.size() > 0)
    dstRTP.SetMarker(false);
  else {
    dstRTP.SetMarker(true); // marker bit on last frame of video
    flags |= PluginCodec_ReturnCoderLastFrame;
  }

  dstRTP.SetPayloadType(payloadCode);
  dstRTP.SetTimestamp(lastTimeStamp);

  return dstRTP.GetPacketLen();
}

int H263EncoderContext::EncodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags)
{
  WaitAndSignal m(_mutex);
  const bool forceIFrame = (flags & PluginCodec_CoderForceIFrame) != 0;

  if (!FFMPEGLibraryInstance.IsLoaded() || avpicture == NULL)
    return 0;

  // create RTP frame from source buffer
  RTPFrame srcRTP(src, srcLen);

  // create RTP frame from destination buffer
  RTPFrame dstRTP(dst, dstLen, RTP_RFC2190_PAYLOAD);
  dstLen = 0;
  flags = 0;

  //WaitAndSignal mutex(updateMutex);

  // if there are RTP packets to return, return them
#if LIBAVCODEC_VERSION_MAJOR >= 58
  if (!rtpPackets.empty()) {
    dstLen = GetNextModernPacket(dstRTP, flags);
    return dstLen != 0;
  }
#else
  if (encodedPackets.size() > 0) {
    dstLen = GetNextEncodedPacket(dstRTP, RTP_RFC2190_PAYLOAD, lastTimeStamp, flags);
    return 1;
  }
#endif

  // from here, we are encoding a new frame
  lastTimeStamp = srcRTP.GetTimestamp();

  if (srcRTP.GetHeaderSize() > srcLen ||
      srcRTP.GetPayloadSize() < (int)sizeof(PluginCodec_Video_FrameHeader)) {
    //PTRACE(1,"H263\tVideo grab too small, Close down video transmission thread.");
    return 0;
  }

  PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)srcRTP.GetPayloadPtr();
  if (header->x != 0 || header->y != 0) {
    //PTRACE(1,"H263\tVideo grab of partial frame unsupported, Close down video transmission thread.");
    return false;
  }

  // if this is the first frame, or the frame size has changed, deal wth it
  if (frameNum == 0 ||
      frameWidth != header->width ||
      frameHeight != header->height) {

//#ifndef h323pluslib
    int sizeIndex = GetStdSize(header->width, header->height);
    if (sizeIndex == StdSizes::UnknownStdSize) {
      //PTRACE(3, "H263\tCannot resize to " << header->width << "x" << header->height << " (non-standard format), Close down video transmission thread.");
      return false;
    }
//#endif

    frameWidth  = header->width;
    frameHeight = header->height;

    rawFrameLen = (frameWidth * frameHeight * 12) / 8;
    memset(rawFrameBuffer + rawFrameLen, 0, FF_INPUT_BUFFER_PADDING_SIZE);

    encFrameLen = rawFrameLen; // this could be set to some lower value

    CloseCodec();
    if (!OpenCodec())
      return false;
  }

  if (srcRTP.GetPayloadSize() < (int)sizeof(PluginCodec_Video_FrameHeader) + rawFrameLen)
    return 0;

  unsigned char * payload;

  // get payload and ensure correct padding
  if (srcRTP.GetHeaderSize() + (unsigned)(srcRTP.GetPayloadSize() + FF_INPUT_BUFFER_PADDING_SIZE <= srcRTP.GetMaxPacketLen()))
    payload = OPAL_VIDEO_FRAME_DATA_PTR(header);
  else {
    payload = rawFrameBuffer;
    memcpy(payload, OPAL_VIDEO_FRAME_DATA_PTR(header), rawFrameLen);
  }

  int size = frameWidth * frameHeight;
  avpicture->data[0] = payload;
  avpicture->data[1] = avpicture->data[0] + size;
  avpicture->data[2] = avpicture->data[1] + (size / 4);
  avpicture->pict_type = forceIFrame ? AV_PICTURE_TYPE_I : AV_PICTURE_TYPE_NONE;
  avpicture->pts = frameNum;

#if LIBAVCODEC_VERSION_MAJOR >= 58
  if (rtpMuxer == NULL || avcodec_send_frame(avcontext, avpicture) < 0)
    return 0;
  AVPacket packet = {};
  if (avcodec_receive_packet(avcontext, &packet) < 0)
    return 0;
  lastFrameIsKey = (packet.flags & AV_PKT_FLAG_KEY) != 0;
  firstPacketOfFrame = true;
  packet.pts = packet.dts = lastTimeStamp;
  const int muxResult = av_write_frame(rtpMuxer, &packet);
  av_packet_unref(&packet);
  if (muxResult < 0)
    return 0;
  frameNum++;
  dstLen = GetNextModernPacket(dstRTP, flags);
  return dstLen != 0;
#else
  const int encodedSize = FFMPEGLibraryInstance.AvcodecEncodeVideo(
      avcontext, encFrameBuffer, encFrameLen, avpicture);
  frameNum++; // increment the number of frames encoded
  if (encodedPackets.size() == 0) {
    //PTRACE(1, "H263\tEncoder internal error - there should be outstanding packets at this point");
    return 1;
  }

  dstLen = GetNextEncodedPacket(dstRTP, RTP_RFC2190_PAYLOAD, lastTimeStamp, flags);

  //PTRACE(6, "H263\tEncoded " << src.GetPayloadSize() << " bytes of YUV420P raw data into " << dst.GetSize() << " RTP frame(s)");

  return 1;
#endif
}

static void * create_encoder(const struct PluginCodec_Definition * /*codec*/)
{
  return new H263EncoderContext;
}

static int encoder_set_options(const PluginCodec_Definition *,
                               void * _context,
                               const char * ,
                               void * parm,
                               unsigned * parmLen)
{
  H263EncoderContext * context = (H263EncoderContext *)_context;
  if (parmLen == NULL || *parmLen != sizeof(const char **) || parm == NULL)
    return 0;

  context->Lock();
  context->CloseCodec();

  // get the "frame width" media format parameter to use as a hint for the encoder to start off
  for (const char * const * option = (const char * const *)parm; *option != NULL; option += 2) {
    if (STRCMPI(option[0], PLUGINCODEC_OPTION_FRAME_WIDTH) == 0)
      context->frameWidth = atoi(option[1]);
    if (STRCMPI(option[0], PLUGINCODEC_OPTION_FRAME_HEIGHT) == 0)
      context->frameHeight = atoi(option[1]);
    if (STRCMPI(option[0], "Encoding Quality") == 0)
      context->videoQuality = MIN(context->videoQMax, MAX(atoi(option[1]), context->videoQMin));
    if (STRCMPI(option[0], PLUGINCODEC_OPTION_TARGET_BIT_RATE) == 0)
      context->bitRate = atoi(option[1]);
    if (STRCMPI(option[0], PLUGINCODEC_OPTION_FRAME_TIME) == 0) {
      const int frameTime = atoi(option[1]);
      if (frameTime > 0)
        context->frameRate = MAX(1, 90000 / frameTime);
    }
    if (STRCMPI(option[0], "set_min_quality") == 0)
      context->videoQMin = atoi(option[1]);
    if (STRCMPI(option[0], "set_max_quality") == 0)
      context->videoQMax = atoi(option[1]);
  }

  const bool opened = context->OpenCodec();
  context->Unlock();

  return opened ? 1 : 0;
}

static void destroy_encoder(const struct PluginCodec_Definition * /*codec*/, void * _context)
{
  H263EncoderContext * context = (H263EncoderContext *)_context;
  delete context;
}

static int codec_encoder(const struct PluginCodec_Definition * ,
                                           void * _context,
                                     const void * from,
                                       unsigned * fromLen,
                                           void * to,
                                       unsigned * toLen,
                                   unsigned int * flag)
{
  H263EncoderContext * context = (H263EncoderContext *)_context;
  return context->EncodeFrames((const BYTE *)from, *fromLen, (BYTE *)to, *toLen, *flag);
}


/////////////////////////////////////////////////////////////////////////////

class H263DecoderContext
{
  public:
    H263DecoderContext();
    ~H263DecoderContext();

    bool DecodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags);

  protected:
    bool OpenCodec();
    void CloseCodec();

    unsigned char encFrameBuffer[MAX_H263_PACKET_SIZE];

    AVCodec        *avcodec;
    AVCodecContext *avcontext;
    AVFrame        *picture;
    std::vector<BYTE> compressedFrame;
    size_t compressedBits;

    int frameNum;
    unsigned int frameWidth;
    unsigned int frameHeight;
};

H263DecoderContext::H263DecoderContext()
{
  avcodec = NULL;
  avcontext = NULL;
  picture = NULL;
  frameNum = 0;
  compressedBits = 0;
  if (!FFMPEGLibraryInstance.IsLoaded())
    return;

  if ((avcodec = FFMPEGLibraryInstance.AvcodecFindDecoder(CODEC_ID_H263)) == NULL) {
    //PTRACE(1, "H263\tCodec not found for decoder");
    return;
  }

  frameWidth  = CIF_WIDTH;
  frameHeight = CIF_HEIGHT;

  avcontext = FFMPEGLibraryInstance.AvcodecAllocContext();
  if (avcontext == NULL) {
    //PTRACE(1, "H263\tFailed to allocate context for decoder");
    return;
  }

  picture = FFMPEGLibraryInstance.AvcodecAllocFrame();
  if (picture == NULL) {
    //PTRACE(1, "H263\tFailed to allocate frame for decoder");
    return;
  }

  if (!OpenCodec()) { // decoder will re-initialise context with correct frame size
    //PTRACE(1, "H263\tFailed to open codec for decoder");
    return;
  }

  frameNum = 0;

  //PTRACE(3, "Codec\tH263 decoder created");
}

H263DecoderContext::~H263DecoderContext()
{
  if (FFMPEGLibraryInstance.IsLoaded()) {
#if LIBAVCODEC_VERSION_MAJOR >= 58
    avcodec_free_context(&avcontext);
    av_frame_free(&picture);
#else
    CloseCodec();

    FFMPEGLibraryInstance.AvcodecFree(avcontext);
    FFMPEGLibraryInstance.AvcodecFree(picture);
#endif
  }
}

bool H263DecoderContext::OpenCodec()
{
  if (avcontext == NULL || picture == NULL || avcodec == NULL)
    return false;
  // avoid copying input/output
  avcontext->flags |= CODEC_FLAG_INPUT_PRESERVED; // we guarantee to preserve input for max_b_frames+1 frames
  avcontext->flags |= CODEC_FLAG_EMU_EDGE; // don't draw edges

  avcontext->width  = frameWidth;
  avcontext->height = frameHeight;
  avcontext->pix_fmt = AV_PIX_FMT_YUV420P;

  avcontext->workaround_bugs = 0; // no workaround for buggy H.263 implementations
  avcontext->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
  // error_resilience removed in newer FFmpeg

  if (FFMPEGLibraryInstance.AvcodecOpen(avcontext, avcodec) < 0) {
    //PTRACE(1, "H263\tFailed to open H.263 decoder");
    return false;
  }

  return true;
}

void H263DecoderContext::CloseCodec()
{
#if LIBAVCODEC_VERSION_MAJOR >= 58
  avcodec_free_context(&avcontext);
#else
  if (avcontext != NULL) {
    if (avcontext->codec != NULL) {
      FFMPEGLibraryInstance.AvcodecClose(avcontext);
      //PTRACE(5, "H263\tClosed H.263 decoder" );
    }
  }
#endif
}

bool H263DecoderContext::DecodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags)
{
  if (!FFMPEGLibraryInstance.IsLoaded() || avcontext == NULL || picture == NULL)
    return 0;
  if (dstLen < RTP_MIN_HEADER_SIZE)
    return 0;

  // create RTP frame from source buffer
  RTPFrame srcRTP(src, srcLen);

  // create RTP frame from destination buffer
  RTPFrame dstRTP(dst, dstLen, 0);
  dstLen = 0;
  flags = 0;

#if LIBAVCODEC_VERSION_MAJOR >= 58
  if (srcRTP.GetPayloadType() != RTP_RFC2190_PAYLOAD || srcRTP.GetPayloadSize() < 4)
    return 0;
  const BYTE *fragment = srcRTP.GetPayloadPtr();
  const unsigned headerSize = (fragment[0] & 0x80) == 0 ? 4 :
                              (fragment[0] & 0x40) == 0 ? 8 : 12;
  const unsigned startBits = (fragment[0] >> 3) & 7;
  const unsigned endBits = fragment[0] & 7;
  const unsigned payloadSize = srcRTP.GetPayloadSize();
  if (payloadSize <= headerSize ||
      (payloadSize - headerSize) * 8 <= startBits + endBits ||
      compressedBits + (payloadSize - headerSize) * 8 > MAX_YUV420P_PACKET_SIZE * 8)
    return 0;
  // RFC 2190 fragments can begin and end within a byte.
  const BYTE *bits = fragment + headerSize;
  const unsigned usefulBits = (payloadSize - headerSize) * 8 - endBits;
  for (unsigned bit = startBits; bit < usefulBits; ++bit) {
    if ((compressedBits & 7) == 0)
      compressedFrame.push_back(0);
    compressedFrame.back() |= ((bits[bit >> 3] >> (7 - (bit & 7))) & 1)
                              << (7 - (compressedBits & 7));
    ++compressedBits;
  }
  if (!srcRTP.GetMarker())
    return 1;

  const size_t encodedSize = compressedFrame.size();
  compressedFrame.resize(encodedSize + AV_INPUT_BUFFER_PADDING_SIZE, 0);
  AVPacket packet = {};
  packet.data = compressedFrame.data();
  packet.size = encodedSize;
  int len = avcodec_send_packet(avcontext, &packet);
  compressedFrame.clear();
  compressedBits = 0;
  if (len < 0) {
    flags = PluginCodec_ReturnCoderRequestIFrame;
    return 1;
  }
  av_frame_unref(picture);
  len = avcodec_receive_frame(avcontext, picture);
  if (len < 0)
    return 1;
#else
  int srcPayloadSize = srcRTP.GetPayloadSize();
  unsigned char * payload;

  // copy payload to a temporary buffer if there are not enough bytes after the end of the payload
  if (srcRTP.GetHeaderSize() + srcPayloadSize + FF_INPUT_BUFFER_PADDING_SIZE > srcLen) {
    if (srcPayloadSize + FF_INPUT_BUFFER_PADDING_SIZE > (int)sizeof(encFrameBuffer))
      return 0;

    memcpy(encFrameBuffer, srcRTP.GetPayloadPtr(), srcPayloadSize);
    payload = encFrameBuffer;
  }
  else
    payload = (unsigned char *) srcRTP.GetPayloadPtr();

  // ensure the first 24 bits past the end of the payload are all zero
  {
    unsigned char * padding = payload + srcPayloadSize;
    padding[0] = padding[1] = padding[2] = 0;
  }

  // only accept RFC 2190 for now
  switch (srcRTP.GetPayloadType()) {
    case RTP_RFC2190_PAYLOAD:
      avcontext->flags |= CODEC_FLAG_RFC2190;
      break;

    //case RTP_DYNAMIC_PAYLOAD:
    //  avcontext->flags |= RTPCODEC_FLAG_RFC2429
    //  break;
    default:
      return 1;
  }

  // decode the frame
  int got_picture;
  int len = FFMPEGLibraryInstance.AvcodecDecodeVideo(avcontext, picture, &got_picture, payload, srcPayloadSize);

  // if that was not the last packet for the frame, keep going
  if (!srcRTP.GetMarker()) {
    return 1;
  }

  // cause decoder to end the frame
  len = FFMPEGLibraryInstance.AvcodecDecodeVideo(avcontext, picture, &got_picture, NULL, -1);

  // if error occurred, tell the other end to send another I-frame and hopefully we can resync
  if (len < 0) {
    flags = PluginCodec_ReturnCoderRequestIFrame;
    return 1;
  }

  // no picture was decoded - shrug and do nothing
  if (!got_picture)
    return 1;
#endif

  // if decoded frame size is not legal, request an I-Frame
  if (avcontext->width == 0 || avcontext->height == 0) {
    flags = PluginCodec_ReturnCoderRequestIFrame;
    return 1;
  }

  // see if frame size has changed
  if (frameWidth != (unsigned)avcontext->width || frameHeight != (unsigned)avcontext->height) {
    frameWidth  = avcontext->width;
    frameHeight = avcontext->height;
  }

  int frameBytes = (frameWidth * frameHeight * 12) / 8;

  // if the frame decodes to more than we can handle, ignore the frame
  if ((sizeof(PluginCodec_Video_FrameHeader) + frameBytes) > (size_t)dstRTP.GetPayloadSize())
    return 1;

  PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)dstRTP.GetPayloadPtr();
  header->x = header->y = 0;
  header->width = frameWidth;
  header->height = frameHeight;
  int size = frameWidth * frameHeight;
  if (picture->data[1] == picture->data[0] + size
      && picture->data[2] == picture->data[1] + (size >> 2))
    memcpy(OPAL_VIDEO_FRAME_DATA_PTR(header), picture->data[0], frameBytes);
  else {
    unsigned char *dstData = OPAL_VIDEO_FRAME_DATA_PTR(header);
    for (int i=0; i<3; i ++) {
      unsigned char *srcData = picture->data[i];
      int dst_stride = i ? frameWidth >> 1 : frameWidth;
      int src_stride = picture->linesize[i];
      int h = i ? frameHeight >> 1 : frameHeight;

      if (src_stride==dst_stride) {
        memcpy(dstData, srcData, dst_stride*h);
        dstData += dst_stride*h;
      } else {
        while (h--) {
          memcpy(dstData, srcData, dst_stride);
          dstData += dst_stride;
          srcData += src_stride;
        }
      }
    }
  }

  dstRTP.SetPayloadSize(sizeof(PluginCodec_Video_FrameHeader) + frameBytes);
  dstRTP.SetPayloadType(RTP_DYNAMIC_PAYLOAD);
  dstRTP.SetTimestamp(srcRTP.GetTimestamp());
  dstRTP.SetMarker(true);

  dstLen = dstRTP.GetPacketLen();

  flags = PluginCodec_ReturnCoderLastFrame;
  // Use pict_type instead of deprecated key_frame
  if (picture->pict_type == AV_PICTURE_TYPE_I)
    flags |= PluginCodec_ReturnCoderIFrame;

  frameNum++;

  return 1;
}


static void * create_decoder(const struct PluginCodec_Definition *)
{
  return new H263DecoderContext;
}

static void destroy_decoder(const struct PluginCodec_Definition * /*codec*/, void * _context)
{
  H263DecoderContext * context = (H263DecoderContext *)_context;
  delete context;
}

static int codec_decoder(const struct PluginCodec_Definition *,
                                           void * _context,
                                     const void * from,
                                       unsigned * fromLen,
                                           void * to,
                                       unsigned * toLen,
                                   unsigned int * flag)
{
  H263DecoderContext * context = (H263DecoderContext *)_context;
  return context->DecodeFrames((const BYTE *)from, *fromLen, (BYTE *)to, *toLen, *flag);
}

static int decoder_get_output_data_size(const PluginCodec_Definition * codec, void *, const char *, void *, unsigned *)
{
  // this is really frame height * frame width;
  return RTP_MIN_HEADER_SIZE + sizeof(PluginCodec_Video_FrameHeader) + ((codec->parm.video.maxFrameWidth * codec->parm.video.maxFrameHeight * 3) / 2);
}


static int get_codec_options(const struct PluginCodec_Definition * codec,
                             void *,
                             const char *,
                             void * parm,
                             unsigned * parmLen)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(struct PluginCodec_Option **))
    return 0;

  *(const void **)parm = codec->userData;
  *parmLen = 0;
  return 1;
}


static char * num2str(int num)
{
  char buf[20];
  snprintf(buf, sizeof(buf), "%i", num);
  return strdup(buf);
}

#define PMAX(a,b) ((a)>=(b)?(a):(b))
#define PMIN(a,b) ((a)<=(b)?(a):(b))

static void FindBoundingBox(const char * const * * parm,
                                             int * mpi,
                                             int & minWidth,
                                             int & minHeight,
                                             int & maxWidth,
                                             int & maxHeight,
                                             int & frameTime,
                                             int & bitRate)
{
  // initialise the MPI values to disabled
  int i;
  for (i = 0; i < 5; i++)
    mpi[i] = PLUGINCODEC_MPI_DISABLED;

  // following values will be set while scanning for options
  minWidth      = INT_MAX;
  minHeight     = INT_MAX;
  maxWidth      = 0;
  maxHeight     = 0;
  int rxMinWidth    = QCIF_WIDTH;
  int rxMinHeight   = QCIF_HEIGHT;
  int rxMaxWidth    = QCIF_WIDTH;
  int rxMaxHeight   = QCIF_HEIGHT;
  int frameRate     = 10;      // 10 fps
  int origFrameTime = 900;     // 10 fps in video RTP timestamps
  int maxBR = 0;
  int maxBitRate = 0;
  int targetBitRate = 0;

  // extract the MPI values set in the custom options, and find the min/max of them
  frameTime = 0;

  for (const char * const * option = *parm; *option != NULL; option += 2) {
    if (STRCMPI(option[0], "MaxBR") == 0)
      maxBR = atoi(option[1]);
    else if (STRCMPI(option[0], PLUGINCODEC_OPTION_MAX_BIT_RATE) == 0)
      maxBitRate = atoi(option[1]);
    else if (STRCMPI(option[0], PLUGINCODEC_OPTION_TARGET_BIT_RATE) == 0)
      targetBitRate = atoi(option[1]);
    else if (STRCMPI(option[0], PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH) == 0)
      rxMinWidth  = atoi(option[1]);
    else if (STRCMPI(option[0], PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT) == 0)
      rxMinHeight = atoi(option[1]);
    else if (STRCMPI(option[0], PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH) == 0)
      rxMaxWidth  = atoi(option[1]);
    else if (STRCMPI(option[0], PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT) == 0)
      rxMaxHeight = atoi(option[1]);
    else if (STRCMPI(option[0], PLUGINCODEC_OPTION_FRAME_TIME) == 0)
      origFrameTime = atoi(option[1]);
    else {
      for (i = 0; i < 5; i++) {
        if (STRCMPI(option[0], StandardVideoSizes[i].optionName) == 0) {
          mpi[i] = atoi(option[1]);
          if (mpi[i] != PLUGINCODEC_MPI_DISABLED) {
            int thisTime = 3003*mpi[i];
            if (minWidth > StandardVideoSizes[i].width)
              minWidth = StandardVideoSizes[i].width;
            if (minHeight > StandardVideoSizes[i].height)
              minHeight = StandardVideoSizes[i].height;
            if (maxWidth < StandardVideoSizes[i].width)
              maxWidth = StandardVideoSizes[i].width;
            if (maxHeight < StandardVideoSizes[i].height)
              maxHeight = StandardVideoSizes[i].height;
            if (thisTime > frameTime)
              frameTime = thisTime;
          }
        }
      }
    }
  }

  // if no MPIs specified, then the spec says to use QCIF
  if (frameTime == 0) {
    int ft;
    if (frameRate != 0)
      ft = 90000 / frameRate;
    else
      ft = origFrameTime;
    mpi[1] = (ft + 1502) / 3003;
    minWidth  = maxWidth  = QCIF_WIDTH;
    minHeight = maxHeight = QCIF_HEIGHT;
  }

  // find the smallest MPI size that is larger than the min frame size
  for (i = 0; i < 5; i++) {
    if (StandardVideoSizes[i].width >= rxMinWidth && StandardVideoSizes[i].height >= rxMinHeight) {
      rxMinWidth = StandardVideoSizes[i].width;
      rxMinHeight = StandardVideoSizes[i].height;
      break;
    }
  }

  // find the largest MPI size that is smaller than the max frame size
  for (i = 4; i >= 0; i--) {
    if (StandardVideoSizes[i].width <= rxMaxWidth && StandardVideoSizes[i].height <= rxMaxHeight) {
      rxMaxWidth  = StandardVideoSizes[i].width;
      rxMaxHeight = StandardVideoSizes[i].height;
      break;
    }
  }

  // the final min/max is the smallest bounding box that will enclose both the MPI information and the min/max information
  minWidth  = PMAX(rxMinWidth, minWidth);
  maxWidth  = PMIN(rxMaxWidth, maxWidth);
  minHeight = PMAX(rxMinHeight, minHeight);
  maxHeight = PMIN(rxMaxHeight, maxHeight);

  // turn off any MPI that are outside the final bounding box
  for (i = 0; i < 5; i++) {
    if (StandardVideoSizes[i].width < minWidth ||
        StandardVideoSizes[i].width > maxWidth ||
        StandardVideoSizes[i].height < minHeight ||
        StandardVideoSizes[i].height > maxHeight)
     mpi[i] = PLUGINCODEC_MPI_DISABLED;
  }

  // find an appropriate max bit rate
  bitRate = 0;
  if (maxBR == 0)
    bitRate = maxBitRate;
  else if (maxBitRate == 0)
    bitRate = maxBR * 100;
  else
    bitRate = PMIN(maxBR * 100, maxBitRate);
}

/* Convert the custom options for the codec to normalised options.
   For H.261 the custom options are "QCIF MPI" and "CIF MPI" which will
   restrict the min/max width/height and maximum frame rate.
 */
static int to_normalised_options(const struct PluginCodec_Definition *, void *, const char *, void * parm, unsigned * parmLen)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char ***))
    return 0;

  // find bounding box enclosing all MPI values
  int mpi[5];
  int minWidth, minHeight, maxHeight, maxWidth, frameTime, bitRate;
  FindBoundingBox((const char * const * *)parm, mpi, minWidth, minHeight, maxWidth, maxHeight, frameTime, bitRate);

  char ** options = (char **)calloc(16+(5*2)+2, sizeof(char *));
  *(char ***)parm = options;
  if (options == NULL)
    return 0;

  options[ 0] = strdup(PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH);
  options[ 1] = num2str(minWidth);
  options[ 2] = strdup(PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT);
  options[ 3] = num2str(minHeight);
  options[ 4] = strdup(PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH);
  options[ 5] = num2str(maxWidth);
  options[ 6] = strdup(PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT);
  options[ 7] = num2str(maxHeight);
  options[ 8] = strdup(PLUGINCODEC_OPTION_FRAME_TIME);
  options[ 9] = num2str(frameTime);
  options[10] = strdup(PLUGINCODEC_OPTION_MAX_BIT_RATE);
  options[11] = num2str(bitRate);
  options[12] = strdup(PLUGINCODEC_OPTION_TARGET_BIT_RATE);
  options[13] = num2str(bitRate);
  options[14] = strdup("MaxBR");
  options[15] = num2str((bitRate+50)/100);
  for (int i = 0; i < 5; i++) {
    options[16+i*2] = strdup(StandardVideoSizes[i].optionName);
    options[16+i*2+1] = num2str(mpi[i]);
  }

  return 1;
}


/* Convert the normalised options to the codec custom options.
   For H.261 the custom options are "QCIF MPI" and "CIF MPI" which are
   set according to the min/max width/height and frame time.
 */
static int to_customised_options(const struct PluginCodec_Definition *, void *, const char *, void * parm, unsigned * parmLen)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char ***))
    return 0;

  // find bounding box enclosing all MPI values
  int mpi[5];
  int minWidth, minHeight, maxHeight, maxWidth, frameTime, bitRate;
  FindBoundingBox((const char * const * *)parm, mpi, minWidth, minHeight, maxWidth, maxHeight, frameTime, bitRate);

  char ** options = (char **)calloc(14+5*2+2, sizeof(char *));
  *(char ***)parm = options;
  if (options == NULL)
    return 0;

  options[ 0] = strdup(PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH);
  options[ 1] = num2str(minWidth);
  options[ 2] = strdup(PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT);
  options[ 3] = num2str(minHeight);
  options[ 4] = strdup(PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH);
  options[ 5] = num2str(maxWidth);
  options[ 6] = strdup(PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT);
  options[ 7] = num2str(maxHeight);
  options[ 8] = strdup(PLUGINCODEC_OPTION_MAX_BIT_RATE);
  options[ 9] = num2str(bitRate);
  options[10] = strdup(PLUGINCODEC_OPTION_TARGET_BIT_RATE);
  options[11] = num2str(bitRate);
  options[12] = strdup("MaxBR");
  options[13] = num2str((bitRate+50)/100);
  for (int i = 0; i < 5; i++) {
    options[14+i*2] = strdup(StandardVideoSizes[i].optionName);
    options[14+i*2+1] = num2str(mpi[i]);
  }

  return 1;
}


static int free_codec_options(const struct PluginCodec_Definition *, void *, const char *, void * parm, unsigned * parmLen)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char ***))
    return 0;

  char ** strings = (char **) parm;
  for (char ** string = strings; *string != NULL; string++)
    free(*string);
  free(strings);
  return 1;
}


static int valid_for_protocol(const struct PluginCodec_Definition *, void *, const char *, void * parm, unsigned * parmLen)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char *))
    return 0;

  return (STRCMPI((const char *)parm, "h.323") == 0 ||
          STRCMPI((const char *)parm, "h323") == 0) ? 1 : 0;

}


/////////////////////////////////////////////////////////////////////////////


static struct PluginCodec_information licenseInfo = {
  1145863600,                                                   // timestamp =  Mon 24 Apr 2006 07:26:40 AM UTC

  "Craig Southeren, Guilhem Tardy, Derek Smithies",             // source code author
  "1.0",                                                        // source code version
  "openh323@openh323.org",                                      // source code email
  "http://sourceforge.net/projects/openh323",                   // source code URL
  "Copyright (C) 2006 by Post Increment",                       // source code copyright
  ", Copyright (C) 2005 Salyens"
  ", Copyright (C) 2001 March Networks Corporation"
  ", Copyright (C) 1999-2000 Equivalence Pty. Ltd."
  "MPL 1.0",                                                    // source code license
  PluginCodec_License_MPL,                                      // source code license

  "FFMPEG",                                                     // codec description
  "Michael Niedermayer, Fabrice Bellard",                       // codec author
  "4.7.1",                                                      // codec version
  "ffmpeg-devel-request@ mplayerhq.hu",                         // codec email
  "http://sourceforge.net/projects/ffmpeg/",                    // codec URL
  "Copyright (c) 2000-2001 Fabrice Bellard"                     // codec copyright information
  ", Copyright (c) 2002-2003 Michael Niedermayer",
  "GNU LESSER GENERAL PUBLIC LICENSE, Version 2.1, February 1999", // codec license
  PluginCodec_License_LGPL                                         // codec license code
};

static const char YUV420PDesc[]  = { "YUV420P" };

static const char h263QCIFDesc[]  = { "H.263-QCIF" };
static const char h263CIFDesc[]   = { "H.263-CIF" };
static const char h263Desc[]      = { "H.263" };

static const char sdpH263[]   = { "h263" };

static PluginCodec_ControlDefn h323EncoderControls[] = {
  { PLUGINCODEC_CONTROL_GET_CODEC_OPTIONS,     get_codec_options },
  { PLUGINCODEC_CONTROL_TO_NORMALISED_OPTIONS, to_normalised_options },
  { PLUGINCODEC_CONTROL_TO_CUSTOMISED_OPTIONS, to_customised_options },
  { PLUGINCODEC_CONTROL_FREE_CODEC_OPTIONS,    free_codec_options },
  { PLUGINCODEC_CONTROL_VALID_FOR_PROTOCOL,    valid_for_protocol },
  { PLUGINCODEC_CONTROL_SET_CODEC_OPTIONS,     encoder_set_options },
  { NULL }
};

static PluginCodec_ControlDefn h323DecoderControls[] = {
  { PLUGINCODEC_CONTROL_GET_CODEC_OPTIONS,     get_codec_options },
  { PLUGINCODEC_CONTROL_TO_NORMALISED_OPTIONS, to_normalised_options },
  { PLUGINCODEC_CONTROL_TO_CUSTOMISED_OPTIONS, to_customised_options },
  { PLUGINCODEC_CONTROL_FREE_CODEC_OPTIONS,    free_codec_options },
  { PLUGINCODEC_CONTROL_VALID_FOR_PROTOCOL,    valid_for_protocol },
  { PLUGINCODEC_CONTROL_GET_OUTPUT_DATA_SIZE,  decoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn EncoderControls[] = {
  { PLUGINCODEC_CONTROL_GET_CODEC_OPTIONS,     get_codec_options },
  { PLUGINCODEC_CONTROL_TO_NORMALISED_OPTIONS, to_normalised_options },
  { PLUGINCODEC_CONTROL_TO_CUSTOMISED_OPTIONS, to_customised_options },
  { PLUGINCODEC_CONTROL_FREE_CODEC_OPTIONS,    free_codec_options },
  { PLUGINCODEC_CONTROL_SET_CODEC_OPTIONS,     encoder_set_options },
  { NULL }
};

static PluginCodec_ControlDefn DecoderControls[] = {
  { PLUGINCODEC_CONTROL_GET_CODEC_OPTIONS,     get_codec_options },
  { PLUGINCODEC_CONTROL_TO_NORMALISED_OPTIONS, to_normalised_options },
  { PLUGINCODEC_CONTROL_TO_CUSTOMISED_OPTIONS, to_customised_options },
  { PLUGINCODEC_CONTROL_FREE_CODEC_OPTIONS,    free_codec_options },
  { PLUGINCODEC_CONTROL_GET_OUTPUT_DATA_SIZE,  decoder_get_output_data_size },
  { NULL }
};

static struct PluginCodec_Option const sqcifMPI =
{
  PluginCodec_IntegerOption,          // Option type
  PLUGINCODEC_SQCIF_MPI,              // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MaxMerge,               // Merge mode
  "1",                                // Initial value
  "SQCIF",                            // FMTP option name
  STRINGIZE(PLUGINCODEC_MPI_DISABLED),// FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "1",                                // Minimum value
  STRINGIZE(PLUGINCODEC_MPI_DISABLED) // Maximum value
};

static struct PluginCodec_Option const qcifMPI =
{
  PluginCodec_IntegerOption,          // Option type
  PLUGINCODEC_QCIF_MPI,               // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MaxMerge,               // Merge mode
  "1",                                // Initial value
  "QCIF",                             // FMTP option name
  STRINGIZE(PLUGINCODEC_MPI_DISABLED),// FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "1",                                // Minimum value
  STRINGIZE(PLUGINCODEC_MPI_DISABLED) // Maximum value
};

static struct PluginCodec_Option const cifMPI =
{
  PluginCodec_IntegerOption,          // Option type
  PLUGINCODEC_CIF_MPI,                // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MaxMerge,               // Merge mode
  "2",                                // Initial value
  "CIF",                              // FMTP option name
  STRINGIZE(PLUGINCODEC_MPI_DISABLED),// FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "1",                                // Minimum value
  STRINGIZE(PLUGINCODEC_MPI_DISABLED) // Maximum value
};

static struct PluginCodec_Option const cif4MPI =
{
  PluginCodec_IntegerOption,          // Option type
  PLUGINCODEC_CIF4_MPI,               // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MaxMerge,               // Merge mode
  "4",                                // Initial value
  "CIF4",                             // FMTP option name
  STRINGIZE(PLUGINCODEC_MPI_DISABLED),// FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "1",                                // Minimum value
  STRINGIZE(PLUGINCODEC_MPI_DISABLED) // Maximum value
};

static struct PluginCodec_Option const cif16MPI =
{
  PluginCodec_IntegerOption,          // Option type
  PLUGINCODEC_CIF16_MPI,              // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MaxMerge,               // Merge mode
  STRINGIZE(PLUGINCODEC_MPI_DISABLED),// Initial value
  "CIF16",                            // FMTP option name
  STRINGIZE(PLUGINCODEC_MPI_DISABLED),// FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "1",                                // Minimum value
  STRINGIZE(PLUGINCODEC_MPI_DISABLED) // Maximum value
};

static struct PluginCodec_Option const maxBR =
{
  PluginCodec_IntegerOption,          // Option type
  "MaxBR",                            // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "0",                                // Initial value
  "maxbr",                            // FMTP option name
  "0",                                // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "32767"                             // Maximum value
};

static struct PluginCodec_Option const videoQuality =
{
  PluginCodec_IntegerOption,          // Option type
  "Encoding Quality",                 // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "10",                               // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "1",                                // Minimum value
  "31"                                // Maximum value
};

static struct PluginCodec_Option const minVideoQuality =
{
  PluginCodec_IntegerOption,          // Option type
  "set_min_quality",                  // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "1",                                // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "1",                                // Minimum value
  "31"                                // Maximum value
};

static struct PluginCodec_Option const maxVideoQuality =
{
  PluginCodec_IntegerOption,          // Option type
  "set_max_quality",                  // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "31",                                // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "1",                                // Minimum value
  "31"                                // Maximum value
};

static struct PluginCodec_Option const mediaPacketization =
{
  PluginCodec_StringOption,           // Option type
  PLUGINCODEC_MEDIA_PACKETIZATION,    // User visible name
  true,                               // User Read/Only flag
  PluginCodec_EqualMerge,             // Merge mode
  "RFC2190"                           // Initial value
};

/* All of the annexes below are turned off and set to read/only because this
   implementation does not support them. Their presence here is so that if
   someone out there does a different implementation of the codec and copies
   this file as a template, they will get them and hopefully notice that they
   can just make them read/write and/or turned on.
 */
static struct PluginCodec_Option const annexF = { PluginCodec_BoolOption,   "Annex F", true, PluginCodec_AndMerge,  "0", "F", "0" };
static struct PluginCodec_Option const annexI = { PluginCodec_BoolOption,   "Annex I", true, PluginCodec_AndMerge,  "0", "I", "0" };
static struct PluginCodec_Option const annexJ = { PluginCodec_BoolOption,   "Annex J", true, PluginCodec_AndMerge,  "0", "J", "0" };
static struct PluginCodec_Option const annexK = { PluginCodec_IntegerOption,"Annex K", true, PluginCodec_EqualMerge,"0", "K", "0", 0, "0", "4" };
static struct PluginCodec_Option const annexN = { PluginCodec_BoolOption,   "Annex N", true, PluginCodec_AndMerge,  "0", "N", "0" };
static struct PluginCodec_Option const annexP = { PluginCodec_BoolOption,   "Annex P", true, PluginCodec_AndMerge,  "0", "P", "0" };
static struct PluginCodec_Option const annexT = { PluginCodec_BoolOption,   "Annex T", true, PluginCodec_AndMerge,  "0", "T", "0" };

static struct PluginCodec_Option const * const qcifOptionTable[] = {
  &mediaPacketization,
  &maxBR,
  &videoQuality,
  &minVideoQuality,
  &maxVideoQuality,
  &qcifMPI,
  NULL
};

static struct PluginCodec_Option const * const cifOptionTable[] = {
  &mediaPacketization,
  &maxBR,
  &videoQuality,
  &minVideoQuality,
  &maxVideoQuality,
  &cifMPI,
  NULL
};

static struct PluginCodec_Option const * const cif4OptionTable[] = {
  &mediaPacketization,
  &maxBR,
  &videoQuality,
  &minVideoQuality,
  &maxVideoQuality,
  &cif4MPI,
  NULL
};

static struct PluginCodec_Option const * const xcifOptionTable[] = {
  &mediaPacketization,
  &maxBR,
  &videoQuality,
  &minVideoQuality,
  &maxVideoQuality,
  &qcifMPI,
  &cifMPI,
  &sqcifMPI,
  &cif4MPI,
  &cif16MPI,
  &annexF,
  &annexI,
  &annexJ,
  &annexK,
  &annexN,
  &annexP,
  &annexT,
  NULL
};


/////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Definition h263CodecDefn[] =
{
  {
    // CIF only encoder
    PLUGIN_CODEC_VERSION_OPTIONS,       // codec API version
    &licenseInfo,                       // license information

    PluginCodec_MediaTypeVideo |        // video codec
    PluginCodec_RTPTypeExplicit,        // specified RTP type

    h263CIFDesc,                        // text decription
    YUV420PDesc,                        // source format
    h263CIFDesc,                        // destination format

    cifOptionTable,                     // user data

    H263_CLOCKRATE,                     // samples per second
    H263_BITRATE,                       // raw bits per second
    20000,                              // nanoseconds per frame

    {{
      CIF_WIDTH,                        // frame width
      CIF_HEIGHT,                       // frame height
      10,                               // recommended frame rate
      60,                               // maximum frame rate
    }},

    RTP_RFC2190_PAYLOAD,                // IANA RTP payload code
    sdpH263,                            // RTP payload name

    create_encoder,                     // create codec function
    destroy_encoder,                    // destroy codec
    codec_encoder,                      // encode/decode
    h323EncoderControls,                // codec controls

    PluginCodec_H323VideoCodec_h263,    // h323CapabilityType
    NULL                                // h323CapabilityData
  },
  {
    // CIF only decoder
    PLUGIN_CODEC_VERSION_OPTIONS,       // codec API version
    &licenseInfo,                       // license information

    PluginCodec_MediaTypeVideo |        // video codec
    PluginCodec_RTPTypeExplicit,        // specified RTP type

    h263CIFDesc,                        // text decription
    h263CIFDesc,                        // source format
    YUV420PDesc,                        // destination format

    cifOptionTable,                     // user data

    H263_CLOCKRATE,                     // samples per second
    H263_BITRATE,                       // raw bits per second
    20000,                              // nanoseconds per frame

    {{
      CIF_WIDTH,                          // frame width
      CIF_HEIGHT,                         // frame height
      10,                                 // recommended frame rate
      60,                                 // maximum frame rate
    }},

    RTP_RFC2190_PAYLOAD,                // IANA RTP payload code
    sdpH263,                            // RTP payload name

    create_decoder,                     // create codec function
    destroy_decoder,                    // destroy codec
    codec_decoder,                      // encode/decode
    h323DecoderControls,                // codec controls

    PluginCodec_H323VideoCodec_h263,    // h323CapabilityType
    NULL                                // h323CapabilityData
  },

  {
    // QCIF only encoder
    PLUGIN_CODEC_VERSION_OPTIONS,       // codec API version
    &licenseInfo,                       // license information

    PluginCodec_MediaTypeVideo |        // audio codec
    PluginCodec_RTPTypeExplicit,        // specified RTP type

    h263QCIFDesc,                       // text decription
    YUV420PDesc,                        // source format
    h263QCIFDesc,                       // destination format

    qcifOptionTable,                    // user data

    H263_CLOCKRATE,                     // samples per second
    H263_BITRATE,                       // raw bits per second
    20000,                              // nanoseconds per frame

    {{
      QCIF_WIDTH,                         // frame width
      QCIF_HEIGHT,                        // frame height
      10,                                 // recommended frame rate
      60,                                 // maximum frame rate
    }},

    RTP_RFC2190_PAYLOAD,                // IANA RTP payload code
    sdpH263,                            // RTP payload name

    create_encoder,                     // create codec function
    destroy_encoder,                    // destroy codec
    codec_encoder,                      // encode/decode
    h323EncoderControls,                // codec controls

    PluginCodec_H323VideoCodec_h263,    // h323CapabilityType
    NULL                                // h323CapabilityData
  },
  {
    // QCIF only decoder
    PLUGIN_CODEC_VERSION_OPTIONS,       // codec API version
    &licenseInfo,                       // license information

    PluginCodec_MediaTypeVideo |        // audio codec
    PluginCodec_RTPTypeExplicit,        // specified RTP type

    h263QCIFDesc,                       // text decription
    h263QCIFDesc,                       // source format
    YUV420PDesc,                        // destination format

    qcifOptionTable,                    // user data

    H263_CLOCKRATE,                     // samples per second
    H263_BITRATE,                       // raw bits per second
    20000,                              // nanoseconds per frame

    {{
      QCIF_WIDTH,                         // frame width
      QCIF_HEIGHT,                        // frame height
      10,                                 // recommended frame rate
      60,                                 // maximum frame rate
    }},

    RTP_RFC2190_PAYLOAD,                // IANA RTP payload code
    sdpH263,                            // RTP payload name

    create_decoder,                     // create codec function
    destroy_decoder,                    // destroy codec
    codec_decoder,                      // encode/decode
    h323DecoderControls,                // codec controls

    PluginCodec_H323VideoCodec_h263,    // h323CapabilityType
    NULL                                // h323CapabilityData
  },

  {
    // 4CIF H.239 encoder
    PLUGIN_CODEC_VERSION_OPTIONS,       // codec API version
    &licenseInfo,                       // license information

    PluginCodec_MediaTypeExtended |     // video codec
    PluginCodec_MediaTypeH239     |     // Extended video codec
    PluginCodec_RTPTypeExplicit,        // specified RTP type

    h263Desc,                           // text decription
    YUV420PDesc,                        // source format
    h263Desc,                           // destination format

    cif4OptionTable,                    // user data

    H263_CLOCKRATE,                     // samples per second
    H263_BITRATE,                       // raw bits per second
    20000,                              // nanoseconds per frame

    {{
      CIF4_WIDTH,                       // frame width
      CIF4_HEIGHT,                      // frame height
      5,                                // recommended frame rate
      10,                               // maximum frame rate
    }},

    RTP_RFC2190_PAYLOAD,                // IANA RTP payload code
    sdpH263,                            // RTP payload name

    create_encoder,                     // create codec function
    destroy_encoder,                    // destroy codec
    codec_encoder,                      // encode/decode
    h323EncoderControls,                // codec controls

    PluginCodec_H323VideoCodec_h263,    // h323CapabilityType
    NULL                                // h323CapabilityData
  },
  {
  // 4CIF H.239 decoder
    PLUGIN_CODEC_VERSION_OPTIONS,       // codec API version
    &licenseInfo,                       // license information

    PluginCodec_MediaTypeExtended |     // video codec
    PluginCodec_MediaTypeH239     |     // Extended video codec
    PluginCodec_RTPTypeExplicit,        // specified RTP type

    h263Desc,                           // text decription
    h263Desc,                           // source format
    YUV420PDesc,                        // destination format

    cif4OptionTable,                    // user data

    H263_CLOCKRATE,                     // samples per second
    H263_BITRATE,                       // raw bits per second
    20000,                              // nanoseconds per frame

    {{
      CIF4_WIDTH,                        // frame width
      CIF4_HEIGHT,                       // frame height
      5,                                 // recommended frame rate
      10,                                // maximum frame rate
    }},

    RTP_RFC2190_PAYLOAD,                // IANA RTP payload code
    sdpH263,                            // RTP payload name

    create_decoder,                     // create codec function
    destroy_decoder,                    // destroy codec
    codec_decoder,                      // encode/decode
    h323DecoderControls,                // codec controls

    PluginCodec_H323VideoCodec_h263,    // h323CapabilityType
    NULL                                // h323CapabilityData
  }
};


/////////////////////////////////////////////////////////////////////////////

extern "C" {
  PLUGIN_CODEC_IMPLEMENT(FFMPEG_H263)

  PLUGIN_CODEC_DLL_API struct PluginCodec_Definition * PLUGIN_CODEC_GET_CODEC_FN(unsigned * count, unsigned version)
  {
    fprintf(stderr, "[H263-FFMPEG] GetCodecs called, version=%u (required=%u)\n", version, PLUGIN_CODEC_VERSION_OPTIONS);
    fflush(stderr);
    
    // check version numbers etc
    if (version < PLUGIN_CODEC_VERSION_OPTIONS) {
      fprintf(stderr, "[H263-FFMPEG] ❌ Version too old\n");
      fflush(stderr);
      *count = 0;
      return NULL;
    }
    
    fprintf(stderr, "[H263-FFMPEG] Loading FFMPEG library...\n");
    fflush(stderr);
    
    if (!FFMPEGLibraryInstance.Load()) {
      fprintf(stderr, "[H263-FFMPEG] ❌ FFMPEG library load failed\n");
      fflush(stderr);
      *count = 0;
      return NULL;
    }
    
    fprintf(stderr, "[H263-FFMPEG] ✅ FFMPEG library loaded successfully\n");
    fflush(stderr);

    *count = sizeof(h263CodecDefn) / sizeof(struct PluginCodec_Definition);
    fprintf(stderr, "[H263-FFMPEG] ✅ Returning %d codec definitions\n", *count);
    fflush(stderr);
    return h263CodecDefn;
  }

};
