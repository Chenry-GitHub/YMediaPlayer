#include "YMediaDecode.h"
#include "WavPlayer.h"
#include "WavPlayerOpenAlWarpper.h"

#define AUDIO_OUT_SAMPLE_RATE 44100
#define MAX_AUDIO_FRAME_SIZE 192000 /*one second bytes  44100*2*2 = 176400*/
#define AUDIO_OUT_CHANNEL 2


void ShowRGBToWnd(HWND hWnd, BYTE* data, int width, int height)
{
	if (data == NULL)
		return;

	static BITMAPINFO *bitMapinfo = NULL;
	static bool First = TRUE;

	if (First)
	{
		BYTE*m_bitBuffer = new BYTE[40 + 4 * 256];//开辟一个内存区域  

		if (m_bitBuffer == NULL)
		{
			return;
		}
		First = FALSE;
		memset(m_bitBuffer, 0, 40 + 4 * 256);
		bitMapinfo = (BITMAPINFO *)m_bitBuffer;
		bitMapinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitMapinfo->bmiHeader.biPlanes = 1;
		for (int i = 0; i < 256; i++)
		{ //颜色的取值范围 (0-255)  
			bitMapinfo->bmiColors[i].rgbBlue = bitMapinfo->bmiColors[i].rgbGreen = bitMapinfo->bmiColors[i].rgbRed = (BYTE)i;
		}
	}
	bitMapinfo->bmiHeader.biHeight = -height;
	bitMapinfo->bmiHeader.biWidth = width;
	bitMapinfo->bmiHeader.biBitCount = 3 * 8;
	RECT drect;
	GetClientRect(hWnd, &drect);    //pWnd指向CWnd类的一个指针   
	HDC hDC = GetDC(hWnd);     //HDC是Windows的一种数据类型，是设备描述句柄；  
	SetStretchBltMode(hDC, COLORONCOLOR);
	StretchDIBits(hDC,
		0,
		0,
		drect.right,   //显示窗口宽度  
		drect.bottom,  //显示窗口高度  
		0,
		0,
		width,      //图像宽度  
		height,      //图像高度  
		data,
		bitMapinfo,
		DIB_RGB_COLORS,
		SRCCOPY
	);
	ReleaseDC(hWnd, hDC);
}


YMediaDecode::YMediaDecode()
	: call_back_(nullptr)
{
	is_need_stop_ = false;
}

YMediaDecode::~YMediaDecode()
{

}

void YMediaDecode::SetPlayerCallBack(Player_CallBack call_back)
{
	call_back_ = call_back;
}

bool YMediaDecode::SetMedia(const std::string & path_file)
{
	StopDecode();
	path_file_ = path_file;
	StartDecode();
	return true;
}

bool YMediaDecode::Pause()
{
	return true;
}

bool YMediaDecode::StopDecode()
{
	is_need_stop_ = true;
	if (decodec_thread_.joinable())
	{
		decodec_thread_.join();//block here
	}
	EmptyAudioQue();
	error_ = YMediaPlayerError::ERROR_NO_ERROR;
	return true;
}

bool YMediaDecode::StartDecode()
{
	std::thread th(&YMediaDecode::DecodecThread, this);
	decodec_thread_.swap(th);
	return true;
}


void YMediaDecode::EmptyAudioQue()
{
	while (audio_que_.GetSize()>0)
	{
		PackageInfo info;
		audio_que_.WaitPop(info);

		if(info.size>0)
			av_free(info.data);

	}
}

PackageInfo YMediaDecode::PopAudioQue()
{
	PackageInfo info;
	audio_que_.WaitPop(info);
	return info;
}

void YMediaDecode::ReleasePackageInfo(PackageInfo*info)
{
	if (info->size > 0)
		av_free(info->data);
}

void YMediaDecode::DecodecThread()
{
	is_need_stop_ = false;

	av_register_all();


	FormatCtx format;
	if (!format.InitFormatCtx(path_file_.c_str()))
	{
		error_ = YMediaPlayerError::ERROR_FILE_ERROR;
		_YMEDIA_CALLBACK(call_back_, MEDIA_ERROR, error_)
		return;
	}


	CodecCtx audio_ctx(format.ctx_, AVMEDIA_TYPE_AUDIO);
	if (!audio_ctx.InitDecoder())
	{
		error_ = YMediaPlayerError::ERROR_FILE_ERROR;
		_YMEDIA_CALLBACK(call_back_, MEDIA_ERROR, error_)
		return;
	}

	CodecCtx video_ctx(format.ctx_, AVMEDIA_TYPE_VIDEO);
	if (video_ctx.InitDecoder())
	{

	}
	
	AVFrame *decoded_frame = av_frame_alloc();
	while (!is_need_stop_)
	{
		if (audio_que_.GetSize() > QUE_PACKAGEINFO_SIZE)  //控制好包的数量
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		if (!format.read())
		{
			break;
		}

		if (format.pkg_->stream_index == audio_ctx.stream_index_)
		{
			DoDecodeAudio( &format, &audio_ctx, decoded_frame);
		}
		else if (format.pkg_->stream_index == video_ctx.stream_index_)
		{
			//DoDecodeVideo(GetDesktopWindow(), &format, &video_ctx, decoded_frame);
		}
		format.release_package();
	}

	/* flush the decoder */
	if (audio_ctx.codec_ctx_)
	{
		format.pkg_->size = 0;
		format.pkg_->data = nullptr;
		DoDecodeAudio(&format, &audio_ctx, decoded_frame);
	}
	av_frame_free(&decoded_frame);

	EmptyAudioQue();
}



void YMediaDecode::DoDecodeAudio(FormatCtx* format_ctx, CodecCtx * codec_ctx, AVFrame *frame)
{
	SwrContext *au_convert_ctx = swr_alloc_set_opts(NULL,
			AV_CH_LAYOUT_STEREO,
			AV_SAMPLE_FMT_S16,
			AUDIO_OUT_SAMPLE_RATE,
			av_get_default_channel_layout(codec_ctx->codec_ctx_->channels),
			codec_ctx->codec_ctx_->sample_fmt,
			codec_ctx->codec_ctx_->sample_rate,
			0,
			NULL);
	swr_init(au_convert_ctx);
	uint8_t *out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE);

	int ret = avcodec_send_packet(codec_ctx->codec_ctx_, format_ctx->pkg_);
	if (ret != 0)
	{
		printf("avcodec_send_packet Error!\n");
		return;
	}

	/* read all the output frames (in general there may be any number of them */
	while ((ret = avcodec_receive_frame(codec_ctx->codec_ctx_, frame)) == 0)
	{
		int out_samples =	swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)frame->data, frame->nb_samples);

		int Audiobuffer_size = av_samples_get_buffer_size(NULL,
			AUDIO_OUT_CHANNEL, out_samples, AV_SAMPLE_FMT_S16, 1);

		printf("audio_stream :%d %d %d %d--size\n", frame->nb_samples, Audiobuffer_size, format_ctx->pkg_->pts, format_ctx->pkg_->size);

		void * data = av_malloc(Audiobuffer_size);
		memcpy_s(data, Audiobuffer_size, out_buffer, Audiobuffer_size);
		PackageInfo info;
		info.data = data;
		info.size = Audiobuffer_size;
		info.sample_rate = AUDIO_OUT_SAMPLE_RATE;
		info.channels = AUDIO_OUT_CHANNEL;
		audio_que_.push(info);
	}

	av_free(out_buffer);
	swr_free(&au_convert_ctx);
}


void YMediaDecode::DoDecodeVideo(HWND hwnd, FormatCtx* format_ctx, CodecCtx * codec_ctx, AVFrame *frame)
{
	struct SwsContext *img_convert_ctx;

	AVFrame *pFrameYUV = av_frame_alloc();
	img_convert_ctx = sws_getContext(codec_ctx->codec_ctx_->width, codec_ctx->codec_ctx_->height, codec_ctx->codec_ctx_->pix_fmt,
		codec_ctx->codec_ctx_->width, codec_ctx->codec_ctx_->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	unsigned char *out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, codec_ctx->codec_ctx_->width, codec_ctx->codec_ctx_->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
		AV_PIX_FMT_YUV420P, codec_ctx->codec_ctx_->width, codec_ctx->codec_ctx_->height, 1);


	int ret = avcodec_send_packet(codec_ctx->codec_ctx_, format_ctx->pkg_);
	if (ret != 0)
	{
		return;
	}

	while ((ret = avcodec_receive_frame(codec_ctx->codec_ctx_, frame)) == 0)
	{
		sws_scale(img_convert_ctx, (const unsigned char* const*)frame->data, frame->linesize, 0, codec_ctx->codec_ctx_->height,
			pFrameYUV->data, pFrameYUV->linesize);

		ShowRGBToWnd(hwnd, pFrameYUV->data[0], codec_ctx->codec_ctx_->width, codec_ctx->codec_ctx_->height);

	}

	sws_freeContext(img_convert_ctx);
	av_frame_free(&pFrameYUV);

}
