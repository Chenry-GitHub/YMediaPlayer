/*
 * Copyright (c) 2001 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * audio decoding with libavcodec API example
 *
 * @example decode_audio.c
 */
#if  0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "WavPlayer.h"

extern "C"
{
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include "libswresample/swresample.h"
}

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096
#define AUDIO_OUT_SAMPLE_RATE 44100
#define MAX_AUDIO_FRAME_SIZE 192000 


static WavPlayer s_player;

static void DecodeAudio(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame,
                   int bytes_persample,FILE *outfile)
{
	static  SwrContext *au_convert_ctx = nullptr;
	static uint8_t * out_buffer=nullptr;
	if (!au_convert_ctx)
	{
		au_convert_ctx = swr_alloc();
		au_convert_ctx = swr_alloc_set_opts(au_convert_ctx,
																	AV_CH_LAYOUT_STEREO, 
																	AV_SAMPLE_FMT_S16, 
																	AUDIO_OUT_SAMPLE_RATE,
																	av_get_default_channel_layout(dec_ctx->channels), 
																	dec_ctx->sample_fmt,
																	dec_ctx->sample_rate, 
																	0,
																	NULL);
		swr_init(au_convert_ctx);
		out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
	}

	int ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret != 0)
	{
		printf("avcodec_send_packet Error!\n");
		return;
	}
	
    /* read all the output frames (in general there may be any number of them */
    while ( (ret = avcodec_receive_frame(dec_ctx, frame))  ==  0 ) 
	{
		int out_buffer_size = av_samples_get_buffer_size(NULL, av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO), dec_ctx->frame_size, AV_SAMPLE_FMT_S16, 1);
	    swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)frame->data, frame->nb_samples);

		s_player.AddBuff((char*)out_buffer, out_buffer_size);
		/*   for (int i = 0; i < frame->nb_samples; i++)
			   for (int ch = 0; ch < dec_ctx->channels; ch++)
			   {

				   fwrite(frame->data[ch] + bytes_persample*i, 1, bytes_persample, outfile);
				   s_player.AddBuff((char*)frame->data[ch] + bytes_persample*i, bytes_persample);
			   }*/
    }
}

struct FormatCtx {
	inline FormatCtx(const char* filename)
		: ctx_(avformat_alloc_context()) {
		pkg_ = av_packet_alloc();
		av_init_packet(pkg_);
		if (avformat_open_input(&ctx_, filename, 0, 0) != 0)
			abort();
		if (avformat_find_stream_info(ctx_, 0) < 0)
			abort();
	}

	inline ~FormatCtx() {
		avformat_free_context(ctx_);
		av_packet_free(&pkg_);
	}

	inline bool read() {
		bool result = av_read_frame(ctx_, pkg_) >= 0;
		return result;
	}

	void release_package()
	{
		av_packet_unref(pkg_);
	}


	AVFormatContext* ctx_;
	AVPacket *pkg_;
} ;

struct CodecCtx {
	inline CodecCtx(AVFormatContext * format_ctx , AVMediaType type)//AVMEDIA_TYPE_AUDIO
		:format_(format_ctx)
		, type_(type)
	{

	}

	~CodecCtx()
	{
		avcodec_free_context(&codec_ctx_);
	}

	bool InitDecoder()
	{
		int ret, stream_index;
		AVStream *st;
		AVCodec *dec = NULL;
		AVDictionary *opts = NULL;

		ret = av_find_best_stream(format_, type_, -1, -1, NULL, 0);
		if (ret < 0) {
			/*	fprintf(stderr, "Could not find %s stream in input file '%s'\n",
			av_get_media_type_string(type), src_filename);*/
			return ret;
		}
		else {
			stream_index = ret;
			st = format_->streams[stream_index];

			/* find decoder for the stream */
			dec = avcodec_find_decoder(st->codecpar->codec_id);
			if (!dec) {
				fprintf(stderr, "Failed to find %s codec\n",
					av_get_media_type_string(type_));
				return AVERROR(EINVAL);
			}

			/* Allocate a codec context for the decoder */
			codec_ctx_ = avcodec_alloc_context3(dec);
			if (!codec_ctx_) {
				fprintf(stderr, "Failed to allocate the %s codec context\n",
					av_get_media_type_string(type_));
				return AVERROR(ENOMEM);
			}

			/* Copy codec parameters from input stream to output codec context */
			if ((ret = avcodec_parameters_to_context(codec_ctx_, st->codecpar)) < 0) {
				fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
					av_get_media_type_string(type_));
				return ret;
			}

			/* Init the decoders, with or without reference counting */
			av_dict_set(&opts, "refcounted_frames", "0", 0);
			if ((ret = avcodec_open2(codec_ctx_, dec, &opts)) < 0) {
				fprintf(stderr, "Failed to open %s codec\n",
					av_get_media_type_string(type_));
				return ret;
			}
			stream_index_ = stream_index;
		}
	}
	AVFormatContext *format_;
	AVCodecContext *codec_ctx_;
	int stream_index_;
	AVMediaType type_;
};



int main(int argc, char **argv)
{
    const char *outfilename, *filename;
    FILE *f, *outfile;
	av_register_all();
    if (argc <= 2) {
        fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
        exit(0);
    }
    filename    = argv[1];
    outfilename = argv[2];

	filename = "C:/audio.mp3";

	FormatCtx formatCtx_(filename);
    /* find the MPEG audio decoder */
	CodecCtx audio_codec_ctx(formatCtx_.ctx_, AVMEDIA_TYPE_AUDIO);

	audio_codec_ctx.InitDecoder();

	CodecCtx video_codec_ctx(formatCtx_.ctx_, AVMEDIA_TYPE_VIDEO);

	video_codec_ctx.InitDecoder();

	int out_channel = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
	int out_size = av_samples_get_buffer_size(NULL, out_channel, audio_codec_ctx.codec_ctx_->frame_size, AV_SAMPLE_FMT_S16, 1);

	s_player.InitPlayer(AUDIO_OUT_SAMPLE_RATE, out_channel, out_size);

    outfile = fopen(outfilename, "wb");
    if (!outfile) {
        exit(1);
    }
	int bytes_persample = av_get_bytes_per_sample(audio_codec_ctx.codec_ctx_->sample_fmt);

	AVFrame *decoded_frame = av_frame_alloc();
    while (formatCtx_.read()) 
	{
		if (formatCtx_.pkg_->stream_index == audio_codec_ctx.stream_index_)
		{
			printf("audio_stream :%d %d %d %d--size\n", bytes_persample, formatCtx_.pkg_->pos,  formatCtx_.pkg_->pts, formatCtx_.pkg_->size);
			DecodeAudio(audio_codec_ctx.codec_ctx_, formatCtx_.pkg_, decoded_frame, bytes_persample, outfile);
		}
		else if (formatCtx_.pkg_->stream_index == video_codec_ctx.stream_index_)
		{
		//	printf("video_stream :%d %d %d\n",bytes_persample, formatCtx_.pkg_.pos, formatCtx_.pkg_.size);
		}
		formatCtx_.release_package();
    }

    /* flush the decoder */
	if (audio_codec_ctx.stream_index_!=-1)
	{
		formatCtx_.pkg_->size = 0;
		formatCtx_.pkg_->data = nullptr;
		DecodeAudio(audio_codec_ctx.codec_ctx_, formatCtx_.pkg_, decoded_frame, bytes_persample, outfile);
	}
    fclose(outfile);

    av_frame_free(&decoded_frame);
	system("pause");
    return 0;
}
#else 
/**
* 最简单的基于FFmpeg的音频播放器 2
* Simplest FFmpeg Audio Player 2
*
* 雷霄骅 Lei Xiaohua
* leixiaohua1020@126.com
* 中国传媒大学/数字电视技术
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* 本程序实现了音频的解码和播放。
* 是最简单的FFmpeg音频解码方面的教程。
* 通过学习本例子可以了解FFmpeg的解码流程。
*
* 该版本使用SDL 2.0替换了第一个版本中的SDL 1.0。
* 注意：SDL 2.0中音频解码的API并无变化。唯一变化的地方在于
* 其回调函数的中的Audio Buffer并没有完全初始化，需要手动初始化。
* 本例子中即SDL_memset(stream, 0, len);
*
* This software decode and play audio streams.
* Suitable for beginner of FFmpeg.
*
* This version use SDL 2.0 instead of SDL 1.2 in version 1
* Note:The good news for audio is that, with one exception,
* it's entirely backwards compatible with 1.2.
* That one really important exception: The audio callback
* does NOT start with a fully initialized buffer anymore.
* You must fully write to the buffer in all cases. In this
* example it is SDL_memset(stream, 0, len);
*
* Version 2.0
*/

#include "YMediaPlayer.h"


int main(int argc, char* argv[])
{
	YMediaPlayer::InitPlayer();
	YMediaPlayer *player=new YMediaPlayer;
	player->SetMediaFromFile("C:/audio.mp3");
//	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	player->Play();
	//std::this_thread::sleep_for(std::chrono::milliseconds(100));
	//player->Pause();
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		player->Pause();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		player->Play();
	}
	system("pause");
	player->Pause();
	system("pause");
	player->Play();

	//player->SetMediaFromFile("C:/audio5d.mp3");
	//player->Play();
	system("pause");
	delete player;

	YMediaPlayer::UnInitPlayer();
	return 0;
}

#endif