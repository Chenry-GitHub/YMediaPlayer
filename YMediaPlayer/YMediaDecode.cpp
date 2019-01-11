#include "YMediaDecode.h"
#include "WavPlayer.h"


#define AUDIO_OUT_SAMPLE_RATE 44100
#define MAX_AUDIO_FRAME_SIZE 192000 /*one second bytes  44100*2*2 = 176400*/
#define AUDIO_OUT_CHANNEL 2
GLFWwindow  *g_hwnd;



YMediaDecode::YMediaDecode()
	: call_back_(nullptr)
{
	is_need_stop_ = false;

	//shader_ptr_ = new Shader("C:/vert.shr", "C:/frame.shr");
	//


	//shader_ptr_->Use();
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
	//YMediaDecode::DecodecThread();
	decodec_thread_ = std::move(std::thread(&YMediaDecode::DecodecThread, this));
	return true;
}


void YMediaDecode::EmptyAudioQue()
{
	while (audio_que_.GetSize()>0)
	{
		AudioPackageInfo info;
		audio_que_.WaitPop(info);

		if(info.size>0)
			av_free(info.data);

	}
}

AudioPackageInfo YMediaDecode::PopAudioQue()
{
	AudioPackageInfo info;
	audio_que_.WaitPop(info);
	return info;
}

bool YMediaDecode::PopVideoQue(VideoPackageInfo&info)
{
	return video_que_.TryPop(info);
}

void YMediaDecode::PushAudioQue(void *data, int size, int sample_rate, int channel, double dur, double pts, YMediaPlayerError error)
{
	AudioPackageInfo info;
	info.data = data;
	info.size = size;
	info.dur = dur;
	info.pts = pts;
	info.sample_rate = sample_rate;
	info.channels = channel;
	info.error = error;
	audio_que_.push(info);
}

void YMediaDecode::ReleasePackageInfo(AudioPackageInfo*info)
{
	if (info->size > 0)
		av_free(info->data);
}

void YMediaDecode::DecodecThread()
{
	is_need_stop_ = false;

	FormatCtx format;
	if (!format.InitFormatCtx(path_file_.c_str()))
	{
		error_ = YMediaPlayerError::ERROR_FILE_ERROR;
		_YMEDIA_CALLBACK(call_back_, MEDIA_ERROR, error_)
		printf("InitFormatCtx Error\n");
		PushAudioQue(nullptr, 0, AUDIO_OUT_SAMPLE_RATE, AUDIO_OUT_CHANNEL, 0,0,ERROR_FILE_ERROR);
		return;
	}


	CodecCtx audio_ctx(format.ctx_, AVMEDIA_TYPE_AUDIO);
	if (!audio_ctx.InitDecoder())
	{
		error_ = YMediaPlayerError::ERROR_FILE_ERROR;
		_YMEDIA_CALLBACK(call_back_, MEDIA_ERROR, error_)
		printf("audio_ctx.InitDecoder Error\n");
		PushAudioQue(nullptr, 0, AUDIO_OUT_SAMPLE_RATE, AUDIO_OUT_CHANNEL,0, 0,ERROR_NO_QUIT);
		return;
	}

	CodecCtx video_ctx(format.ctx_, AVMEDIA_TYPE_VIDEO);
	if (!video_ctx.InitDecoder())
	{
		printf("video_ctx.InitDecoder Error\n");
	}
	else
	{
		m_PictureSize = avpicture_get_size(AV_PIX_FMT_BGR24, video_ctx.codec_ctx_->width, video_ctx.codec_ctx_->height);
		m_buf = (uint8_t*)av_malloc(m_PictureSize);
		if (m_buf == NULL)
		{
			avformat_close_input(&format.ctx_);
			avcodec_close(video_ctx.codec_ctx_);
			return;
		}
		m_pFrameRGB = av_frame_alloc();
		avpicture_fill((AVPicture *)m_pFrameRGB, m_buf, AV_PIX_FMT_BGR24, video_ctx.codec_ctx_->width, video_ctx.codec_ctx_->height);
		m_pSwsCtx = sws_getContext(video_ctx.codec_ctx_->width, video_ctx.codec_ctx_->height, video_ctx.codec_ctx_->pix_fmt, video_ctx.codec_ctx_->width, video_ctx.codec_ctx_->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL); 
	}

	SwrContext* au_convert_ctx = swr_alloc();
	au_convert_ctx = swr_alloc_set_opts(au_convert_ctx,
		AV_CH_LAYOUT_STEREO,
		AV_SAMPLE_FMT_S16,
		AUDIO_OUT_SAMPLE_RATE,
		av_get_default_channel_layout(audio_ctx.codec_ctx_->channels),
		audio_ctx.codec_ctx_->sample_fmt,
		audio_ctx.codec_ctx_->sample_rate,
		0,
		NULL);
	swr_init(au_convert_ctx);


	AVFrame *decoded_frame = av_frame_alloc();
	while (!is_need_stop_)
	{
		//if (audio_que_.GetSize() > QUE_PACKAGEINFO_SIZE)  //¿ØÖÆºÃ°üµÄÊýÁ¿
		//{
		//	std::this_thread::sleep_for(std::chrono::milliseconds(10));
		//	continue;
		//}

		if (!format.read())
		{
			printf("format.read() Error\n");
			break;
		}

		if (format.pkg_->stream_index == audio_ctx.stream_index_)
		{
			DoDecodeAudio( &format, &audio_ctx, decoded_frame, au_convert_ctx);
		}
		else if (format.pkg_->stream_index == video_ctx.stream_index_)
		{
			DoDecodeVideo(&format, &video_ctx, decoded_frame);
		}
		format.release_package();
	}

	/* flush the decoder */
	if (audio_ctx.codec_ctx_)
	{
		format.pkg_->size = 0;
		format.pkg_->data = nullptr;
		DoDecodeAudio(&format, &audio_ctx, decoded_frame, au_convert_ctx);
	}
	av_frame_free(&decoded_frame);
	swr_free(&au_convert_ctx);

	printf("YMediaDecode quit\n");
}



void YMediaDecode::DoDecodeAudio(FormatCtx* format_ctx, CodecCtx * codec_ctx, AVFrame *frame, SwrContext*au_convert_ctx)
{
#if 1
	int ret = avcodec_send_packet(codec_ctx->codec_ctx_, format_ctx->pkg_);
	if (ret != 0)
	{
		printf("avcodec_send_packet Error!\n");
		return;
	}

	/* read all the output frames (in general there may be any number of them */
	while ((ret = avcodec_receive_frame(codec_ctx->codec_ctx_, frame)) == 0)
	{
		//Pre-calculate the out buffer size for two channels
		//int out_size_candidate = AUDIO_OUT_SAMPLE_RATE * AUDIO_OUT_CHANNEL * 2 * dur;
		int out_size_candidate = av_samples_get_buffer_size(NULL,
			codec_ctx->codec_ctx_->channels, frame->nb_samples, codec_ctx->codec_ctx_->sample_fmt, codec_ctx->codec_ctx_->block_align);
		uint8_t *out_buffer = (uint8_t *)av_malloc(sizeof uint8_t *out_size_candidate);
		//two channel
		int out_samples = swr_convert(au_convert_ctx, &out_buffer, out_size_candidate, (const uint8_t **)frame->data, frame->nb_samples);

		int Audiobuffer_size = av_samples_get_buffer_size(NULL,
			AUDIO_OUT_CHANNEL, out_samples, AV_SAMPLE_FMT_S16, 1);

		//push to media player
		void * data = av_malloc(Audiobuffer_size);
		memcpy_s(data, Audiobuffer_size, out_buffer, Audiobuffer_size);
		double dur = frame->pkt_duration* av_q2d(codec_ctx->GetStream()->time_base);
		double pts = frame->pts *av_q2d(codec_ctx->GetStream()->time_base);
		//printf("pts:pts:pts:%f\n",pts);
		PushAudioQue(data, Audiobuffer_size, AUDIO_OUT_SAMPLE_RATE, AUDIO_OUT_CHANNEL,dur, pts,ERROR_NO_ERROR);
		av_free(out_buffer);
	}
#else

	int got=0;
	int result = avcodec_decode_audio4(codec_ctx->codec_ctx_,frame,&got, format_ctx->pkg_);
	if (result < 0)
		return;

	int dur = av_rescale_q(format_ctx->pkg_->duration, codec_ctx->codec_ctx_->time_base, AVRational{ 1, AV_TIME_BASE });;
	int outSizeCandidate = AUDIO_OUT_SAMPLE_RATE * 8 * double(dur) / 1000000.0;
	uint8_t *out_buffer = (uint8_t *)av_malloc(sizeof uint8_t *outSizeCandidate);
	int out_samples = swr_convert(au_convert_ctx, &out_buffer, outSizeCandidate, (const uint8_t **)frame->data, frame->nb_samples);

	int Audiobuffer_size = av_samples_get_buffer_size(NULL,
		AUDIO_OUT_CHANNEL, out_samples, AV_SAMPLE_FMT_S16, 1);

	//printf("audio_stream :%d %d %d %d--size\n", frame->nb_samples, Audiobuffer_size, format_ctx->pkg_->pts, format_ctx->pkg_->size);

	void * data = av_malloc(Audiobuffer_size);
	memcpy_s(data, Audiobuffer_size, out_buffer, Audiobuffer_size);
	//push to media player
	PushAudioQue(data, Audiobuffer_size, AUDIO_OUT_SAMPLE_RATE, AUDIO_OUT_CHANNEL,dur,0,ERROR_NO_ERROR);
	av_free(out_buffer);
#endif

	//swr_free(&au_convert_ctx);
}
void ShowRGBToWnd(HWND hWnd, BYTE* data, int width, int height)
{
	if (data == NULL)
		return;

	static BITMAPINFO *bitMapinfo = NULL;
	static bool First = TRUE;

	if (First)
	{
		BYTE * m_bitBuffer = new BYTE[40 + 4 * 256];//¿ª±ÙÒ»¸öÄÚ´æÇøÓò  

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
		{ //ÑÕÉ«µÄÈ¡Öµ·¶Î§ (0-255)  
			bitMapinfo->bmiColors[i].rgbBlue = bitMapinfo->bmiColors[i].rgbGreen = bitMapinfo->bmiColors[i].rgbRed = (BYTE)i;
		}
	}
	bitMapinfo->bmiHeader.biHeight = -height;
	bitMapinfo->bmiHeader.biWidth = width;
	bitMapinfo->bmiHeader.biBitCount = 3 * 8;
	RECT drect;
	GetClientRect(hWnd, &drect);    //pWndÖ¸ÏòCWndÀàµÄÒ»¸öÖ¸Õë   
	HDC hDC = GetDC(hWnd);     //HDCÊÇWindowsµÄÒ»ÖÖÊý¾ÝÀàÐÍ£¬ÊÇÉè±¸ÃèÊö¾ä±ú£»  
	SetStretchBltMode(hDC, COLORONCOLOR);
	StretchDIBits(hDC,
		0,
		0,
		drect.right,   //ÏÔÊ¾´°¿Ú¿í¶È  
		drect.bottom,  //ÏÔÊ¾´°¿Ú¸ß¶È  
		0,
		0,
		width,      //Í¼Ïñ¿í¶È  
		height,      //Í¼Ïñ¸ß¶È  
		data,
		bitMapinfo,
		DIB_RGB_COLORS,
		SRCCOPY
	);
	ReleaseDC(hWnd, hDC);
}
double YMediaDecode::synchronize(CodecCtx*codec, AVFrame *srcFrame, double pts)
{
	static double video_clock = 0.0f;
	double frame_delay;
	
	if (pts != 0)
		video_clock = pts; // Get pts,then set video clock to it
	else
		pts = video_clock; // Don't get pts,set it to video clock

	frame_delay = av_q2d(codec->codec_ctx_->time_base);
	frame_delay += srcFrame->repeat_pict * (frame_delay * 0.5);

	video_clock += frame_delay;

	return video_clock;
}

void YMediaDecode::DoDecodeVideo(FormatCtx* format_ctx, CodecCtx * codec_ctx, AVFrame *pFrame)
{
	/*struct SwsContext *img_convert_ctx;

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
	av_frame_free(&pFrameYUV);*/

	int frameFinished = avcodec_send_packet(codec_ctx->codec_ctx_, format_ctx->pkg_);
	while (!frameFinished) {
		// For decoding, call avcodec_receive_frame(). On success, it will return an AVFrame containing uncompressed audio or video data.
		frameFinished = avcodec_receive_frame(codec_ctx->codec_ctx_, pFrame);
		if (!frameFinished) {
			// m_log->debug(QString("%1 %2").arg(pFrame->format == AV_PIX_FMT_YUV420P ? "OK" : "NO").arg(count));
			// -----------------------------------------------
			sws_scale(m_pSwsCtx, pFrame->data, pFrame->linesize, 0, codec_ctx->codec_ctx_->height, m_pFrameRGB->data, m_pFrameRGB->linesize);
		//	ShowRGBToWnd(GetDesktopWindow(), m_pFrameRGB->data[0], codec_ctx->codec_ctx_->width, codec_ctx->codec_ctx_->height);
		/*	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, codec_ctx->codec_ctx_->width,
				codec_ctx->codec_ctx_->height, GL_RGB, GL_UNSIGNED_BYTE,
				m_pFrameRGB->data[0]);*/
			//long long pts = av_rescale_q(format_ctx->pkg_->pts, codec_ctx->codec_ctx_->time_base, AVRational{ 1, AV_TIME_BASE });

			double video_pts;
			if (format_ctx->pkg_->dts == AV_NOPTS_VALUE && pFrame->opaque&& *(uint64_t*)pFrame->opaque != AV_NOPTS_VALUE)
			{
				video_pts = *(uint64_t *)pFrame->opaque;
			}
			else if (format_ctx->pkg_->dts != AV_NOPTS_VALUE)
			{
				video_pts = format_ctx->pkg_->dts;
			}
			else
			{
				video_pts = 0;
			}

			video_pts *= av_q2d(codec_ctx->GetStream()->time_base);
			video_pts = synchronize(codec_ctx,pFrame, video_pts);
		//	printf("deco%f \n", video_pts);

			//frame->opaque = &pts;
		//	video_pts = synchronize_video(is, pFrame, video_pts);
		//	printf("video-pts:%f\n",video_pts);
			//static double last_clock;


			//double frame_delay;

			//if (video_pts == 0)
			//{
			//	/* if we have pts, set video clock to it */
			//	
			//	/* if we aren't given a pts, set it to the clock */
			//	video_pts = last_clock;
			//}
			///* update the video clock */
			//frame_delay = av_q2d(codec_ctx->codec_ctx_->time_base);
			///* if we are repeating a frame, adjust clock accordingly */
			//frame_delay += pFrame->repeat_pict * (frame_delay * 0.5);
			//video_pts += frame_delay;

			//last_clock = video_pts;

			VideoPackageInfo info;
			info.data = m_pFrameRGB->data[0];
			info.width = codec_ctx->codec_ctx_->width;
			info.height = codec_ctx->codec_ctx_->height;
			info.pts = video_pts;
			info.clock = video_pts;
			video_que_.push(info);
			

			//av_frame_unref(pFrame);
		}
	}

}

//double synchronize(AVFrame *srcFrame, double pts)
//{
//	double frame_delay;
//
//	if (pts != 0)
//		video_clock = pts; // Get pts,then set video clock to it
//	else
//		pts = video_clock; // Don't get pts,set it to video clock
//
//	frame_delay = av_q2d(stream->codec->time_base);
//	frame_delay += srcFrame->repeat_pict * (frame_delay * 0.5);
//
//	video_clock += frame_delay;
//
//	return pts;
//}
