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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}


#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

static void DecodeAudio(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame,
                   int bytes_persample,FILE *outfile)
{
	int ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret != 0)
	{
		printf("avcodec_send_packet Error!\n");
		return;
	}
	
    /* read all the output frames (in general there may be any number of them */
    while ( (ret = avcodec_receive_frame(dec_ctx, frame))  ==  0 ) 
	{
        for (int i = 0; i < frame->nb_samples; i++)
            for (int ch = 0; ch < dec_ctx->channels; ch++)
                fwrite(frame->data[ch] + bytes_persample*i, 1, bytes_persample, outfile);
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
		:stream_index_(-1)
		,codec_ctx_(nullptr)
	{
		for (size_t i = 0; i < format_ctx->nb_streams; ++i) {
			AVCodecContext* tc = format_ctx->streams[i]->codec;
			if (tc->codec_type == type) {
				codec_ctx_ = tc;
				stream_index_ = i;
				break;
			}
		}
		if (codec_ctx_)
		{
			AVCodec* codec = avcodec_find_decoder(codec_ctx_->codec_id);
			if (!codec) {
				fprintf(stderr, "Codec not found\n");
				exit(1);
			}

			codec_ctx_ = avcodec_alloc_context3(codec);
			if (!codec_ctx_) {
				fprintf(stderr, "Could not allocate audio codec context\n");
				exit(1);
			}

			/* open it */
			if (avcodec_open2(codec_ctx_, codec, NULL) < 0) {
				fprintf(stderr, "Could not open codec\n");
				exit(1);
			}
		}
	}

	~CodecCtx()
	{
		avcodec_free_context(&codec_ctx_);
	}

	AVCodecContext *codec_ctx_;
	int stream_index_;
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
	FormatCtx formatCtx_(filename);
    /* find the MPEG audio decoder */
	CodecCtx audio_codec_ctx(formatCtx_.ctx_, AVMEDIA_TYPE_AUDIO);
	CodecCtx video_codec_ctx(formatCtx_.ctx_, AVMEDIA_TYPE_VIDEO);



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
			printf("audio_stream :%d %d %d --size\n", bytes_persample, formatCtx_.pkg_->pos, formatCtx_.pkg_->size);
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
    return 0;
}
