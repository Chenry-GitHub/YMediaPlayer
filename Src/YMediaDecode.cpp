#include "YMediaDecode.h"



#define  SEEK_TIME_DEFAULT -1.0
#define  QUE_AUDIO_INNER_SIZE 400
#define  QUE_VIDEO_INNER_SIZE 300



YMediaDecode::YMediaDecode()
{
	is_manual_stop_ = false;
}

YMediaDecode::~YMediaDecode()
{

}

bool YMediaDecode::SetMedia(const std::string & path_file, int sample_rate, int channel)
{
	StopDecode();
	path_file_ = path_file;
	sample_rate_ = sample_rate;
	channel_ = channel;
	StartDecode();
	return true;
}


bool YMediaDecode::StopDecode()
{
	is_manual_stop_ = true;
	if (decode_thread_.joinable())
	{
		decode_thread_.join();//block here
	}
	EmptyAudioQue();
	EmptyVideoQue();
	return true;
}

bool YMediaDecode::StartDecode()
{
	//YMediaDecode::DecodeThread();
	decode_thread_ = std::move(std::thread(&YMediaDecode::DecodeThread, this));
	return true;
}


void YMediaDecode::EmptyAudioQue()
{
	std::lock_guard<std::mutex> lg(audio_seek_mutex_);
	while (!audio_inner_que_.IsEmpty())
	{
		InnerPacketInfo pkg_info;
		if (audio_inner_que_.TryPop(pkg_info))
		{
			av_packet_unref(pkg_info.pkg);
			av_packet_free(&pkg_info.pkg);
		}
	}

	while (!audio_que_.IsEmpty())
	{
		AudioPackageInfo info;
		if (audio_que_.TryPop(info))
		{
			FreeAudioPackageInfo(&info);
		}
	}
}

void YMediaDecode::EmptyVideoQue()
{
	std::lock_guard<std::mutex> lg(video_seek_mutex_);
	while (!video_inner_que_.IsEmpty())
	{
		InnerPacketInfo pkg_info;
		if (video_inner_que_.TryPop(pkg_info))
		{
			av_packet_unref(pkg_info.pkg);
			av_packet_free(&pkg_info.pkg);
		}
	}

	while (!video_que_.IsEmpty())
	{
		VideoPackageInfo info;
		video_que_.TryPop(info);
	}
}

void YMediaDecode::SeekPos(double pos)//ffmpeg pts 
{
	auto audio_ctx = audio_codec_.lock();
	auto video_ctx = video_codec_.lock();
	if (!audio_ctx || !video_ctx)
		return;
	is_seek_ = true;
	audio_seek_convert_dur_ = av_rescale_q(pos*AV_TIME_BASE, { 1, AV_TIME_BASE }, audio_ctx->GetStream()->time_base);
	video_seek_convert_dur_ = av_rescale_q(pos*AV_TIME_BASE, { 1, AV_TIME_BASE }, video_ctx->GetStream()->time_base);
	//video_seek_convert_dur_ = audio_seek_convert_dur_;
}

AudioPackageInfo YMediaDecode::PopAudioQue()
{
	AudioPackageInfo info;
	InnerPacketInfo pkg_info;
	audio_inner_que_.WaitPop(pkg_info);
	if (FLAG_PLAY == pkg_info.flag )
	{
		std::lock_guard<std::mutex> lg(audio_seek_mutex_);
		DoConvertAudio(pkg_info.pkg);
		av_packet_unref(pkg_info.pkg);
		av_packet_free(&pkg_info.pkg);
		audio_que_.TryPop(info);
		return info;
	}
	else
	{
		info.error = ERROR_PKG_ERROR;
	}
	av_packet_unref(pkg_info.pkg);
	av_packet_free(&pkg_info.pkg);

	audio_que_.TryPop(info);

	return info;
}

VideoPackageInfo YMediaDecode::PopVideoQue(double cur_clock)
{
	VideoPackageInfo info;
	InnerPacketInfo pkg_info;
	video_inner_que_.WaitPop(pkg_info);
	if (FLAG_PLAY == pkg_info.flag )
	{
		std::lock_guard<std::mutex> lg(video_seek_mutex_);
		DoConvertVideo(pkg_info.pkg, cur_clock);
		av_packet_unref(pkg_info.pkg);
		av_packet_free(&pkg_info.pkg);
		video_que_.TryPop(info);
		return info;
	}
	else
	{
		info.error = ERROR_PKG_ERROR;
	}
	
	av_packet_unref(pkg_info.pkg);
	av_packet_free(&pkg_info.pkg);
	video_que_.TryPop(info);
	return info;
}


void YMediaDecode::FreeAudioPackageInfo(AudioPackageInfo*info)
{
	if(info && info->error == ERROR_NO_ERROR)
		av_free(info->data);
}


void YMediaDecode::ConductAudioBlocking()
{
	InnerPacketInfo info_audio;
	info_audio.pkg = av_packet_alloc();
	info_audio.flag = FLAG_CONDUCT_QUE;
	audio_inner_que_.push(info_audio);

	audio_cnd_.notify_all();
}

void YMediaDecode::ConductVideoBlocking()
{
	InnerPacketInfo info_video;
	info_video.pkg = av_packet_alloc();
	info_video.flag = FLAG_CONDUCT_QUE;
	video_inner_que_.push(info_video);

	video_cnd_.notify_all();
}


bool YMediaDecode::JudgeBlockAudioSeek()
{
	unique_lock<std::mutex> unique(audio_cnd_lock_);
	audio_cnd_.wait(unique, [&] {return audio_seek_convert_dur_ == SEEK_TIME_DEFAULT; });
	return audio_seek_convert_dur_ != SEEK_TIME_DEFAULT;
}

bool YMediaDecode::JudgeBlockVideoSeek()
{
	unique_lock<std::mutex> unique(video_cnd_lock_);
	video_cnd_.wait(unique, [&] { return video_seek_convert_dur_ == SEEK_TIME_DEFAULT; });
	return video_seek_convert_dur_ != SEEK_TIME_DEFAULT;
}

void YMediaDecode::DecodeThread()
{
	is_seek_ = false;
	audio_seek_convert_dur_ = SEEK_TIME_DEFAULT;
	video_seek_convert_dur_ = SEEK_TIME_DEFAULT;
	is_manual_stop_ = false;
	uint8_t* pic_buff=nullptr;

	std::shared_ptr<FormatCtx> format = std::make_shared<FormatCtx>(ReadBuff, SeekBuff, MemReadStruct{this});
	format_ctx_ = format;
	if (!format->InitFormatCtx(path_file_.c_str()))
	{
		printf("InitFormatCtx Error\n");
		NotifyDecodeStatus(ERROR_FILE_ERROR);
		return;
	}

	//notify media info for player
	MediaInfo info;
	info.dur = (double)format->ctx_->duration / AV_TIME_BASE;
	NotifyMediaInfo(info);

	//this is for audio
	std::shared_ptr<CodecCtx> audio_ctx = std::make_shared<CodecCtx>(format->ctx_, AVMEDIA_TYPE_AUDIO);
	audio_codec_ = audio_ctx;

	std::shared_ptr<AVFrameManger > audio_frame = std::make_shared<AVFrameManger>();
	audio_frame_ = audio_frame;

	if (!audio_ctx->InitDecoder())
	{
		printf("audio_ctx.InitDecoder Error\n");
		NotifyDecodeStatus(ERROR_NO_QUIT);
		return;
	}

	AudioConvertParameter parameter;
	parameter.des_fmt = AV_SAMPLE_FMT_S16;/*default 16 bits*/
	parameter.des_layout = (2==channel_ ?AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO);
	parameter.des_sample_rate = sample_rate_;
	parameter.src_channel = av_get_default_channel_layout(audio_ctx->codec_ctx_->channels);
	parameter.src_sample_fmt= audio_ctx->codec_ctx_->sample_fmt;
	parameter.src_sample_rate= audio_ctx->codec_ctx_->sample_rate;
	std::shared_ptr<AudioConvertManger > audio_convert = std::make_shared<AudioConvertManger>(parameter);
	audio_convert_ = audio_convert;
	

	//this is for video
	std::shared_ptr<CodecCtx> video_ctx = std::make_shared<CodecCtx>(format->ctx_, AVMEDIA_TYPE_VIDEO);
	video_codec_ = video_ctx;

	std::shared_ptr<AVFrameManger > video_frame = std::make_shared<AVFrameManger>();
	video_frame_ = video_frame;

	std::shared_ptr<VideoConvertManger> video_convert;
	if (!video_ctx->InitDecoder())
	{
		printf("video_ctx.InitDecoder Failed\n");
		//NotifyDecodeStatus();
	}
	else
	{ 
		video_convert = std::make_shared<VideoConvertManger>(video_ctx->codec_ctx_->width, video_ctx->codec_ctx_->height, video_ctx->codec_ctx_->pix_fmt);
		video_convert_ = video_convert;
	}

	while (!is_manual_stop_)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		//seek operation
		if (is_seek_)
		{
			if (audio_ctx->IsValid())
			{
				if (av_seek_frame(format->ctx_, audio_ctx->stream_index_, audio_seek_convert_dur_, AVSEEK_FLAG_FRAME) >= 0)
				{
					EmptyAudioQue();
					avcodec_flush_buffers(audio_ctx->codec_ctx_);
				}
			}
			
			if (video_ctx->IsValid())
			{
				if (av_seek_frame(format->ctx_, video_ctx->stream_index_, video_seek_convert_dur_, AVSEEK_FLAG_FRAME) >= 0)
				{
					EmptyVideoQue();
					avcodec_flush_buffers(video_ctx->codec_ctx_);
				}
			}
			is_seek_ = false;
		}

		//if (video_inner_que_.GetSize() >= QUE_VIDEO_INNER_SIZE || audio_inner_que_.GetSize() >= QUE_AUDIO_INNER_SIZE)
		//{
		//	continue;
		//}

		//end seek operation
		if (!format->read())
		{
			continue;
		}
		if (format->pkg_->stream_index == audio_ctx->stream_index_)
		{
			if (audio_seek_convert_dur_!= SEEK_TIME_DEFAULT)
			{
				double abs_value = abs(format->pkg_->pts - audio_seek_convert_dur_);
				if (abs_value <= AV_TIME_BASE* 5)//5秒
				{
					audio_seek_convert_dur_ = SEEK_TIME_DEFAULT;
					audio_cnd_.notify_all();
					
					InnerPacketInfo info;
					info.pkg = av_packet_clone(format->pkg_);
					audio_inner_que_.push(info);
				}
			}
			else
			{
				InnerPacketInfo info;
				info.pkg = av_packet_clone(format->pkg_);
				audio_inner_que_.push(info);
			}
		}
		else if (format->pkg_->stream_index == video_ctx->stream_index_)
		{
			if (video_seek_convert_dur_ != SEEK_TIME_DEFAULT)
			{
				double abs_value = abs(format->pkg_->pts - video_seek_convert_dur_);
				if (abs_value <= AV_TIME_BASE * 5)
				{
					if (format->pkg_->flags&AV_PKT_FLAG_KEY)
					{
						video_seek_convert_dur_ = SEEK_TIME_DEFAULT;
						video_cnd_.notify_all();
						
						InnerPacketInfo info;
						info.pkg = av_packet_clone(format->pkg_);
						video_inner_que_.push(info);
					}
				}
			}
			else
			{
				InnerPacketInfo info;
				info.pkg = av_packet_clone(format->pkg_);
				video_inner_que_.push(info);
			}
		}
		format->release_package();
	}

	FlushAudioDecodec();
	FlushVideoDecodec();

	printf("YMediaDecode quit\n");
}



void YMediaDecode::DoConvertAudio(AVPacket *pkg)
{
#if 1
	auto audio_frame = audio_frame_.lock();
	auto codec_ctx = audio_codec_.lock();
	auto convert = audio_convert_.lock();
	if (!codec_ctx || !audio_frame||!convert)
	{
		return;
	}

	int ret = avcodec_send_packet(codec_ctx->codec_ctx_, pkg);
	if (ret != 0)
	{
		printf("avcodec_send_packet Error!\n");
		return;
	}

	/* read all the output frames (in general there may be any number of them */
	while ((ret = avcodec_receive_frame(codec_ctx->codec_ctx_, audio_frame->frame_)) == 0)
	{
		//Pre-calculate the out buffer size for two channels
		//int out_size_candidate = AUDIO_OUT_SAMPLE_RATE * AUDIO_OUT_CHANNEL * 2 * dur;
		int out_size_candidate = av_samples_get_buffer_size(NULL,
			codec_ctx->codec_ctx_->channels, audio_frame->frame_->nb_samples, codec_ctx->codec_ctx_->sample_fmt, codec_ctx->codec_ctx_->block_align);
		uint8_t *out_buffer = (uint8_t *)av_malloc(sizeof uint8_t *out_size_candidate);
		//two channel
		int out_samples = swr_convert(convert->audio_convert_ctx_, &out_buffer, out_size_candidate, (const uint8_t **)audio_frame->frame_->data, audio_frame->frame_->nb_samples);

		int Audiobuffer_size = av_samples_get_buffer_size(NULL,
			channel_, out_samples, AV_SAMPLE_FMT_S16, 1);

		//push to media player
		void * data = av_malloc(Audiobuffer_size);
		memcpy_s(data, Audiobuffer_size, out_buffer, Audiobuffer_size);
		double pts = audio_frame->frame_->pts *av_q2d(codec_ctx->GetStream()->time_base);
		//printf("pts:pts:pts:%f\n",pts);
		
		AudioPackageInfo info;
		info.data = data;
		info.size = Audiobuffer_size;
		info.pts = pts;
		info.error = ERROR_NO_ERROR;
		audio_que_.push(info);

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
}

double YMediaDecode::synchronize(std::shared_ptr<CodecCtx> codec, AVFrame *srcFrame, double pts, double cur_clock)
{
	double video_clock = cur_clock;
	double frame_delay;
	
	if (pts != 0)
		video_clock = pts; // Get pts,then set video clock to it
	else
		pts = video_clock; // Don't get pts,set it to video clock

	frame_delay = av_q2d(codec->GetStream()->time_base);
	frame_delay += srcFrame->repeat_pict * (frame_delay*0.5);

	video_clock += frame_delay;

	return video_clock;
}

void YMediaDecode::FlushVideoDecodec()
{
	auto video_codec = video_codec_.lock();
	if (video_codec->IsValid())
	{
		InnerPacketInfo info_video;
		info_video.pkg = av_packet_alloc();
		info_video.pkg->size = 0;
		info_video.pkg->data = nullptr;
		info_video.flag = FLAG_FLUSH_DECODEC;
		DoConvertVideo(info_video.pkg, 0);
	}
}

void YMediaDecode::FlushAudioDecodec()
{
	auto audio_codec = audio_codec_.lock();
	if (audio_codec->IsValid())
	{
		InnerPacketInfo info_audio;
		info_audio.pkg = av_packet_alloc();
		info_audio.pkg->size = 0;
		info_audio.pkg->data = nullptr;
		info_audio.flag = FLAG_FLUSH_DECODEC;
		DoConvertAudio(info_audio.pkg);
	}
}

int YMediaDecode::ReadBuff(void *opaque, uint8_t *read_buf, int read_buf_size)
{
	std::shared_ptr<MemReadStruct>* readst = (std::shared_ptr<MemReadStruct>*)opaque;
	if (readst  && *readst)
	{
		int result = (*readst)->target->read_func_((char*)read_buf, read_buf_size);
		if (result > 0)
		{
			return result;
		}
		return -1;
	}
	return -1;
}

int64_t YMediaDecode::SeekBuff(void *opaque, int64_t offset, int whence)
{
	std::shared_ptr<MemReadStruct>* op = (std::shared_ptr<MemReadStruct>*)opaque;
	if (op  && *op)
	{
		return (*op)->target->seek_func_(opaque, offset, whence);
	}
	return	-1;
}

void YMediaDecode::NotifyDecodeStatus(DecodeError error)
{
	if (error_func_)
		error_func_(error);
}

void YMediaDecode::NotifyMediaInfo(MediaInfo info)
{
	if(media_func_)
		media_func_(info);
}

void YMediaDecode::DoConvertVideo(AVPacket *pkg, double cur_clock)
{
	auto video_frame = video_frame_.lock();
	auto codec_ctx = video_codec_.lock();
	auto convert = video_convert_.lock();
	if (!codec_ctx || !video_frame||!convert)
		return;

	if (avcodec_send_packet(codec_ctx->codec_ctx_, pkg))
	{
		printf("avcodec_send_packet :video error\n");
		return;
	}


	while (avcodec_receive_frame(codec_ctx->codec_ctx_, video_frame->frame_) == 0)
	{
		convert->Convert(video_frame->frame_->data, video_frame->frame_->linesize, codec_ctx->codec_ctx_->height);
		//	sws_scale(convert->video_convert_ctx_, video_frame->frame_->data, video_frame->frame_->linesize, 0, codec_ctx->codec_ctx_->height, rgb_frame->frame_->data, rgb_frame->frame_->linesize);
		//	ShowRGBToWnd(GetDesktopWindow(), m_pFrameRGB->data[0], codec_ctx->codec_ctx_->width, codec_ctx->codec_ctx_->height);
		

			double video_pts;
			if (pkg->dts == AV_NOPTS_VALUE && convert->rgb_frame_.frame_->opaque&& *(uint64_t*)convert->rgb_frame_.frame_->opaque != AV_NOPTS_VALUE)
			{
				video_pts = *(uint64_t *)convert->rgb_frame_.frame_->opaque;
			}
			else if (pkg->dts != AV_NOPTS_VALUE)
			{
				video_pts = pkg->dts;
			}
			else
			{
				video_pts = 0;
			}

			video_pts *= av_q2d(codec_ctx->GetStream()->time_base);
			video_pts = synchronize(codec_ctx, convert->rgb_frame_.frame_, video_pts, cur_clock);

			VideoPackageInfo info;
			info.data = convert->rgb_frame_.frame_->data[0];
			info.width = codec_ctx->codec_ctx_->width;
			info.height = codec_ctx->codec_ctx_->height;
			info.pts = video_pts;
			info.error = ERROR_NO_ERROR;
			video_que_.push(info);
	}
}