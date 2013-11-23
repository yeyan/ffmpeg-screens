/*
 * (C) Copyright 2013 Ye Yan and others.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library/program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * Contributors:
 */

#ifndef FFMPEG_H_1DPZGUTU
#define FFMPEG_H_1DPZGUTU


extern "C" {

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

}

#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<AVFormatContext> SFormatContextPtr;
typedef boost::shared_ptr<AVCodecContext> SCodecContextPtr;
typedef boost::shared_ptr<AVFrame> SFramePtr;


#endif /* end of include guard: FFMPEG_H_1DPZGUTU */

