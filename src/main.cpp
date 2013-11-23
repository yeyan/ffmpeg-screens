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
#include "ImageBuilder.hpp"
#include "ImageBuffer.hpp"

#include <string>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/exception/all.hpp>

#include <iostream>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>

#include <boost/program_options.hpp> 


struct FException: 
    virtual boost::exception, virtual std::exception
{};

typedef boost::error_info<struct tag_errinfo_message,
            const char*> errinfo_message;

namespace internal
{
    struct FFMPEG
    {
        FFMPEG() { av_register_all(); }
    } _ffmpeg;
} /* internal */ 

class Decoder
{
public:
    Decoder (const std::string &fileName):
        fmtCtx_(OpenFormat(fileName), Decoder::CloseFormat),
        videoStream_(FindFirstVideoStream(fmtCtx_)),
        cdcCtx_(OpenCodec(fmtCtx_), avcodec_close),
        frame_(avcodec_alloc_frame(), av_free)
    {
        //av_dump_format(fmtCtx_.get(), 0, fileName.c_str(), 0);
    }

    int Width() { return cdcCtx_->width; }
    int Height() { return cdcCtx_->height; }

    int DurationInSeconds() { return fmtCtx_->duration/AV_TIME_BASE; }

    void Seek(int timeInSeconds) 
    {
        int64_t timestamp = av_rescale(((uint64_t)timeInSeconds)*1000, 
                fmtCtx_->streams[videoStream_]->time_base.den,
                fmtCtx_->streams[videoStream_]->time_base.num)/1000;
        int ret;
        //if((ret = av_seek_frame(fmtCtx_.get(), -1, AV_TIME_BASE*timeInSeconds, 0)) < 0)
        //int64_t timestamp = AV_TIME_BASE*timeInSeconds; 
        if((ret = avformat_seek_file(fmtCtx_.get(), videoStream_, 
                        INT64_MIN, timestamp, INT64_MAX, AVSEEK_FLAG_FRAME)) < 0) 
        {
            BOOST_THROW_EXCEPTION(FException()
                    << boost::errinfo_api_function("avformat_seek_file")
                    << boost::errinfo_errno(ret));
        }
        avcodec_flush_buffers(cdcCtx_.get());
    }

    void DecodeFrame(ImageBuffer& imageBuffer) 
    {
        AVPacket packet;
        int frameComplete = 0;
        /* code */
        while((av_read_frame(fmtCtx_.get(), &packet)>=0) && !frameComplete) {

            // Is this a packet from the video stream?
            if(packet.stream_index==videoStream_) {

                /// Decode video frame
                avcodec_decode_video2(cdcCtx_.get(), frame_.get(), &frameComplete, &packet);

                // Did we get a video frame?
                if(frameComplete) {
                    imageBuffer.Fill(frame_);
                }
            }

            // Free the packet that was allocated by av_read_frame
            av_free_packet(&packet);
        }
    }

    ImageBuffer CreateImageBuffer(int width, int height)
    {
        return ImageBuffer(cdcCtx_, width, height);
    }

    std::string GetVideoInfo(const std::string &fileName)
    {
        std::ostringstream os;

        os << "File Name: " << boost::filesystem::path(fileName).filename() << std::endl;

        if (fmtCtx_->duration != AV_NOPTS_VALUE) {
            int hours, mins, secs, us;
            int64_t duration = fmtCtx_->duration + 5000;
            secs = duration / AV_TIME_BASE;
            us = duration % AV_TIME_BASE;
            mins = secs / 60;
            secs %= 60;
            hours = mins / 60;
            mins %= 60;
            os << "Duration: " << boost::format("%02d:%02d:%02d.%02d") % hours 
                    % mins % secs % ((100 * us)/AV_TIME_BASE) << std::endl;
        }

        os << "Resolution: " << boost::format("%dx%d") 
            % cdcCtx_->width % cdcCtx_->height << std::endl;

        double fileSize = boost::filesystem::file_size(fileName) / 1024;
       
        const char *unit[] = {"KB", "MB", "GB", "TB"};
        int unitIndex;

        for (unitIndex = 0; unitIndex < 4; ++unitIndex)
        {
            if (fileSize > 1024) 
                fileSize /= 1024; 
            else 
                break;
        }

        os << "File Size: " 
            << boost::format("%0.2f") % fileSize
            << unit[unitIndex] << std::endl;

        //os << "Bitrate: ";
        //if (fmtCtx_->bit_rate) {
        //    os << boost::format("%d kb/s") % (fmtCtx_->bit_rate / 1000);
        //} else {
        //    os << "N/A";
        //}
        //os << std::endl;

        return os.str();
    }

private:
    static void CloseFormat(AVFormatContext *&ptr)
    {
        avformat_close_input(&ptr);
    }

    static AVFormatContext* OpenFormat(const std::string &fileName)//const char *fileName)
    {
        AVFormatContext *fmtCtx = avformat_alloc_context();

        if (avformat_open_input(&fmtCtx, fileName.c_str(), NULL, NULL) != 0)
        {
            BOOST_THROW_EXCEPTION(FException() 
                << boost::errinfo_api_function("avformat_open_input"));
        }

        // Retrieve stream information
        if(avformat_find_stream_info(fmtCtx, NULL)<0)
        {
            BOOST_THROW_EXCEPTION(FException()
                << boost::errinfo_api_function("avformat_find_stream_info"));
        }

        return fmtCtx;
    }

    static int FindFirstVideoStream(const SFormatContextPtr& fmtCtx)
    {
        int videoStream=-1;
        for(int i=0; i<fmtCtx->nb_streams; i++)
            if(fmtCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
                videoStream=i;
                break;
            }
        if(videoStream==-1)
            BOOST_THROW_EXCEPTION(FException()
                    << errinfo_message("couldn't find video stream"));
        else
            return videoStream;
    }

    static AVCodecContext* OpenCodec(const SFormatContextPtr& fmtCtx)
    {
        AVCodecContext *cdcCtx = NULL;

        AVCodec *codec;
        int stream = FindFirstVideoStream(fmtCtx);

        // Get a pointer to the codec context for the video stream 
        cdcCtx=fmtCtx->streams[stream]->codec;

        // Find the decoder for the video stream
        codec=avcodec_find_decoder(cdcCtx->codec_id);
        if(codec==NULL) {
            BOOST_THROW_EXCEPTION(FException()
                    << errinfo_message("codec is not supported"));
        }
        // Open codec
        if(avcodec_open2(cdcCtx, codec, NULL)<0)
        {
            BOOST_THROW_EXCEPTION(FException()
                    << boost::errinfo_api_function("avcodec_find_decoder2")
                    << errinfo_message("codec is not supported"));
        }

        return cdcCtx;
    }

private:

    /* data */
    SFormatContextPtr fmtCtx_;
    SCodecContextPtr cdcCtx_;
    int videoStream_;
    SFramePtr frame_;
};

void GenerateScreens(const std::string &fileName,
        int frameWidth, int frameHeight, int rowCount, int colCount)
{
    std::cout<<"Generating screenshot for "<<fileName<<std::endl;

    int headerHeight = 120;

    try{
        Decoder decoder(fileName);

        if (frameWidth == 0 && frameHeight == 0)
        {
            frameWidth = decoder.Width(); frameHeight = decoder.Height();
        }

        if (frameWidth == 0 && frameHeight != 0)
        {
            frameWidth = (double) decoder.Width() / decoder.Height() * frameHeight;
        }

        if (frameWidth != 0 && frameHeight == 0)
        {
            frameHeight = (double) decoder.Height() / decoder.Width() * frameWidth;
        }

        //std::cout<<frameWidth<<" "<<frameHeight<<std::endl;

        ImageBuffer imageBuffer = decoder.CreateImageBuffer(frameWidth, frameHeight);

        int step = decoder.DurationInSeconds() / (rowCount * colCount + 2);

        //std::cout<<"Duration: "<<decoder.DurationInSeconds()<<" Seconds"<<std::endl;

        ImageBuilder imageBuilder(frameWidth * rowCount, 
                frameHeight * colCount + headerHeight);

        int i,j;

        for (j = 0; j < colCount; ++j)
        {
            for (i = 0; i < rowCount; ++i)
            {
                decoder.Seek(step*(j*rowCount+i+1));
                decoder.DecodeFrame(imageBuffer);

                imageBuilder.WriteImage(
                        imageBuffer,
                        i*frameWidth, j*frameHeight+headerHeight,
                        frameWidth, frameHeight);
            }
        }

        //std::cout<<decoder.GetVideoInfo(fileName);

        imageBuilder.DrawFilledRect(
                255.0/255,255.0/255,240.0/255,
                0, 0, frameWidth * rowCount, headerHeight);
        imageBuilder.WriteText(
                10, 0, "San", 20, 10, decoder.GetVideoInfo(fileName));

        imageBuilder.SaveToPng(
                boost::filesystem::path(fileName).stem().string() + ".png");

    }catch(FException& e){
        std::cout<<diagnostic_information(e)<<std::endl;
    }
}

int main(int argc, char *argv[])
{
    int headerHeight = 120;
    int frameWidth = 320, frameHeight = 240;
    int rowCount = 4, colCount = 10;

    namespace po = boost::program_options;
    using namespace std;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("frame-width,w", po::value<int>(&frameWidth)->default_value(320), "frame width")
        ("frame-height,h", po::value<int>(&frameHeight)->default_value(0), "frame height")
        ("row-count,r", po::value<int>(&rowCount)->default_value(4), "row count")
        ("col-count,r", po::value<int>(&rowCount)->default_value(10), "col count")
        ("input-file", po::value< vector<string> >(), "input file")
        ;

    po::positional_options_description p;
    p.add("input-file", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
          options(desc).positional(p).run(), vm);
    po::notify(vm);    

    if (vm.count("help")) {
        cout << desc << "\n";
        return 0;
    }

    if (vm.count("input-file"))
    {
        vector<string> inputFiles = 
            vm["input-file"].as<vector<string> >();

        for (vector<string>::const_iterator itr = inputFiles.begin();
                itr != inputFiles.end(); ++itr)
        {
            cout<<"Generating screens for "<<*itr<<endl;
            if(boost::filesystem::exists(*itr))
                GenerateScreens(*itr, frameWidth, frameHeight, rowCount, colCount);
            else
                cout<<"File: "<<*itr<<" does not exist!"<<endl;
        }
    }

    return 0;
}
