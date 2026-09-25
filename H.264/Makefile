#
# Makefile for H.264
#
# Copyright (C) 2004 Post Increment, All Rights Reserved
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and limitations
# under the License.
#
# The Original Code is Open H323 library.
#
# The Initial Developer of the Original Code is Post Increment
#
# Contributor(s): ______________________________________.
#
# $Log$
# Revision 1.8  2012/04/04 15:00:59  shorne
# fix for missing endif
#
# Revision 1.7  2012/04/03 18:14:44  shorne
# Static codec loading. Added VGA definition(disabled) Added EmphasisSpeed Added changing NAL Size
#
# Revision 1.6  2010/09/26 13:34:49  willamowius
# don't warn about unused definitions
#
# Revision 1.5  2010/09/26 13:24:46  willamowius
# fix comment
#
# Revision 1.4  2010/09/09 09:51:16  willamowius
# enable warnings, disable strict-aliasing optimization, because of conflict with PDynaLink
#
# Revision 1.3  2010/05/21 11:08:26  willamowius
# add compiler flag needed for x264 >= 0.94
#
# Revision 1.2  2010/03/01 23:56:18  willamowius
# don't include an OPAL version into the search path for the plugin
#
# Revision 1.1  2010/02/24 02:19:43  shorne
# First commit of h323plus mirror
#
# Revision 1.4  2007/09/02 12:03:19  dominance
# * dynamically load libx264 at runtime on *nix
# * Improved error handling
#
# Patch received from and thus thanks goes to Matthias Schneider.
#
# Revision 1.3  2007/08/23 19:20:14  dominance
# fix compilation of H.264 if CFLAGS is present from make
#
# Revision 1.2  2007/08/22 21:51:26  dsandras
# Applied patch from Matthias Schneider <ma30002000 yahoo de> to share
# things between codecs. Huge thanks!
#
# Revision 1.1  2007/06/16 21:36:59  dsandras
# Added H.264 support thanks to Matthias Schneider <ma30002000 yahoo de>.
# Thanks a lot !
#
# Baseline Profile:
# no B-frames
# We make use of the baseline profile (which is the designated profile for interactive vide) ,
# that means:
# no B-Frames (too much latency in interactive video)
# CBR (we want to get the max. quality making use of all the bitrate that is available)
# We allow one exeption: configuring a bitrate of > 786 kbit/s
#
# This plugin implements
# - Single Time Aggregation Packets A
# - Single NAL units
# - Fragmentation Units
# like described in RFC3984
#
# It requires x264 and ffmpeg.
#
# Revision 1.5  2007/04/19 06:09:04  csoutheren
# Fixed plugin build system to use better detection for OSX
# Resolved 1636262 - Fix for audio plugins for OSX
#
# Revision 1.4  2006/10/10 01:42:55  csoutheren
# Applied patches from Ekiga bug 360869 - fix OpenSolaris build problems
# Thanks to Boying Lu
#
# Revision 1.3  2006/08/20 00:32:25  csoutheren
# Added missing files to Makefile
#
# Revision 1.2  2006/08/12 10:59:14  rjongbloed
# Added Linux build for H.261 plug-in.
#

VC_PLUGIN_DIR=opal-1.28.0/codecs/video
prefix=/usr/local
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
target_os=darwin
DL_LIBS =-lc

SHAREDDIR	= ./shared
COMMONDIR	= ../common
GPLDIR      = ./gpl
PLUGINDIR	= ../../
OBJDIR	= ./obj
STATICBUILD=yes
# SUBDIRS not needed for static build - helper process not used
# SUBDIRS		+= gpl

BASENAME=h264
SONAME	=$(BASENAME)

CC	 	=gcc
CFLAGS 	= -Os  -fno-common -dynamic	-D__STDC_CONSTANT_MACROS
CXX	 	=g++
LDSO	 	=-dynamiclib -multiply_defined suppress
PLUGINEXT	=dylib
STDCCFLAGS	= -fPIC -D__MACOSX__
LDFLAGS		=
PKG_CONFIG ?= pkg-config
OPENH323DIR ?= $(if $(wildcard ../../../include/codec/opalplugin.h),$(abspath ../../../),$(abspath ../../h323plus))
STATIC_EXTERNAL_LIBS ?= yes
FFMPEG_STATIC_ROOT ?= $(OPENH323DIR)/third_party/static_ffmpeg/install
LIBAVCODEC_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags libavcodec libavutil 2>/dev/null)

# x264 settings for static linking
X264_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags x264 2>/dev/null)
X264_LIBDIR ?= $(shell $(PKG_CONFIG) --variable=libdir x264 2>/dev/null)
X264_STATIC_ARCHIVE ?= $(X264_LIBDIR)/libx264.a
LIBAVCODEC_DYNAMIC_LIBS ?= $(shell $(PKG_CONFIG) --libs libavcodec libavutil libswresample libswscale 2>/dev/null)
X264_DYNAMIC_LIBS ?= $(shell $(PKG_CONFIG) --libs x264 2>/dev/null)

ifeq ($(STATIC_EXTERNAL_LIBS),yes)
  ifeq ($(words $(wildcard $(FFMPEG_STATIC_ROOT)/lib/libavcodec.a $(FFMPEG_STATIC_ROOT)/lib/libavutil.a $(X264_STATIC_ARCHIVE))),3)
    LIBAVCODEC_CFLAGS = -I$(FFMPEG_STATIC_ROOT)/include
    LIBAVCODEC_STATIC_ARCHIVES = $(FFMPEG_STATIC_ROOT)/lib/libavcodec.a $(FFMPEG_STATIC_ROOT)/lib/libavutil.a
    LIBAVCODEC_STATIC_DEPS = -lm -lpthread -lz -framework CoreFoundation -framework CoreMedia -framework CoreVideo -framework CoreServices -framework VideoToolbox -framework AudioToolbox
    EXT_LINK_LIBS = $(LIBAVCODEC_STATIC_ARCHIVES) $(X264_STATIC_ARCHIVE) $(LIBAVCODEC_STATIC_DEPS)
  else
    $(warning Static FFmpeg/x264 archives are incomplete; falling back to dynamic pkg-config libs)
    EXT_LINK_LIBS = $(LIBAVCODEC_DYNAMIC_LIBS) $(X264_DYNAMIC_LIBS)
  endif
else
  EXT_LINK_LIBS = $(LIBAVCODEC_DYNAMIC_LIBS) $(X264_DYNAMIC_LIBS)
endif

EXTRACCFLAGS    +=  $(LIBAVCODEC_CFLAGS) $(X264_CFLAGS) -I$(COMMONDIR) -I$(PLUGINDIR) -DLIB_DIR='"$(libdir)"' -DVC_PLUGIN_DIR='"$(VC_PLUGIN_DIR)"' -DH323_STATIC_H264 -DX264_LINK_STATIC -D_STATIC_LINK

# For static x264 build, we don't use LICENCE_MPL so that x264 encoder functions are compiled
# ifeq ($(STATICBUILD),yes)
#   EXTRACCFLAGS    += -DX264_LINK_STATIC -D_STATIC_LINK
# else
#   EXTRACCFLAGS    += -DLICENCE_MPL
# endif


STDCCFLAGS += -Wall -Wno-unused -Wformat -Wformat-security -Wreorder -fno-strict-aliasing

vpath	%.cxx $(SHAREDDIR)
vpath	%.cxx $(COMMONDIR)
vpath	%.cxx $(GPLDIR)
vpath	%.o   $(OBJDIR)

SRCS    += $(SHAREDDIR)/h264frame.cxx \
           h264-x264.cxx \
           $(COMMONDIR)/dyna.cxx \
           $(COMMONDIR)/trace.cxx \
           h264pipe_static.cxx \
           $(GPLDIR)/enc-ctx.cxx

$(OBJDIR)/%.o : %.cxx
	@mkdir -p $(OBJDIR) >/dev/null 2>&1
	$(CXX) -I$(OPENH323DIR)/include -I$(OPENH323DIR)/plugins $(EXTRACCFLAGS) $(STDCCFLAGS) $(OPTCCFLAGS) $(CFLAGS) -c $< -o $@

PLUGIN	= ./$(BASENAME)_video_pwplugin.$(PLUGINEXT)
STATIC	= ./lib$(BASENAME)_video_s.a

OBJECTS = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(patsubst %.cxx,%.o,$(notdir $(SRCS)))))

$(PLUGIN): $(OBJECTS)
	@set -e; $(foreach dir,$(SUBDIRS),if test -d ${dir} ; then $(MAKE) -C $(dir); fi ; )
ifeq (,$(findstring solaris,$(target_os)))
	$(CXX) $(LDSO) $(LDFLAGS) $(DL_LIBS) $(EXT_LINK_LIBS) -o $@ $^
else
	$(CXX) $(LDSO) $(LDFLAGS) $(DL_LIBS) $(EXT_LINK_LIBS) $@ -o $@ $^
endif


install: $(PLUGIN)
	@set -e; $(foreach dir,$(SUBDIRS),if test -d ${dir} ; then $(MAKE) -C $(dir) install; fi ; )
	mkdir -p $(DESTDIR)$(libdir)/$(VC_PLUGIN_DIR)
	install $(PLUGIN) $(DESTDIR)$(libdir)/$(VC_PLUGIN_DIR)

uninstall:
	@set -e; $(foreach dir,$(SUBDIRS),if test -d ${dir} ; then $(MAKE) -C $(dir) uninstall; fi ; )
	rm -f $(DESTDIR)$(libdir)/$(VC_PLUGIN_DIR)/$(PLUGIN)

clean:
	rm -f $(OBJECTS) $(PLUGIN)
	@set -e; $(foreach dir,$(SUBDIRS),if test -d ${dir} ; then $(MAKE) -C $(dir) clean; fi ; )

###########################################
