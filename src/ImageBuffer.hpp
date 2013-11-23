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

#include "ffmpeg.h"
#include <boost/shared_ptr.hpp>
#include <boost/exception/all.hpp>


class ImageBuffer
{
public:
    typedef boost::shared_ptr<uint8_t> BufferPtr;

    ImageBuffer(const SCodecContextPtr& cdcCtx, int width, int height):
        data_(avcodec_alloc_frame(), av_free), 
        buffer_(), width_(width), height_(height)
    {
        int numBytes = avpicture_get_size(PIX_FMT_RGB32, width, height);
        buffer_.reset((uint8_t *)av_malloc(numBytes*sizeof(uint8_t)));

        // Assign appropriate parts of buffer to image planes in pFrameRGB
        // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
        // of AVPicture
        avpicture_fill(
                (AVPicture *)data_.get(), buffer_.get(), PIX_FMT_RGB32, width, height);

        cvtCtx_ = sws_getContext(
                cdcCtx->width, cdcCtx->height, cdcCtx->pix_fmt,
                width, height, PIX_FMT_RGB32,
                SWS_POINT, NULL, NULL, NULL); 
    }

    void Fill(const SFramePtr& frame)
    {
        sws_scale(
            cvtCtx_,
            (uint8_t const* const*) frame->data,
            frame->linesize,
            0,
            frame->height,
            data_->data,
            data_->linesize);
    }

    uint8_t *Data() const
    {
        return data_->data[0];
    }

    int Width() const 
    {
        return width_;
    }

    int Height() const
    {
        return height_;
    }

private:
    SFramePtr data_;
    int width_, height_;

    BufferPtr buffer_;
    SwsContext *cvtCtx_;
};
