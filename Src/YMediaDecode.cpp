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

void YMediaDecode::setDelegate(YMediaDecode::Delegate*dele)
{
	delegate_ = dele;
}

YMediaDecode::Delegate* YMediaDecode::getDelegate()
{
	return delegate_;
}

bool YMediaDecode::setMedia(const std::string & path_file, int sample_rate, int channel)
{
	stopDecode();
	path_file_ = path_file;
	sample_rate_ = sample_rate;
	channel_ = channel;
	startDecode();
	return true;
}


bool YMediaDecode::stopDecode()
{
	is_manual_stop_ = true;
	if (decode_thread_.joinable())
	{
		decode_thread_.join();//block here
	}
	emptyAudioQue();
	emptyVideoQue();
	return true;
}

bool YMediaDecode::startDecode()
{
	//YMediaDecode::DecodeThread();
	decode_thread_ = std::move(std::thread(&YMediaDecode::decodeThread, this));
	return true;
}


void YMediaDecode::emptyAudioQue()
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
			freeAudioPackageInfo(&info);
		}
	}
}

void YMediaDecode::emptyVideoQue()
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

void YMediaDecode::seekPos(double pos)//ffmpeg pts 
{
	auto audio_ctx = audio_codec_.lock();
	auto video_ctx = video_codec_.lock();
	if (!audio_ctx || !video_ctx)
		return;
	is_seek_ = true;
	if(audio_ctx->isValid())
	audio_seek_convert_dur_ = av_rescale_q(pos*AV_TIME_BASE, { 1, AV_TIME_BASE }, audio_ctx->getStream()->time_base);
	
	if(video_ctx->isValid())
	video_seek_convert_dur_ = av_rescale_q(pos*AV_TIME_BASE, { 1, AV_TIME_BASE }, video_ctx->getStream()->time_base);
}

AudioPackageInfo YMediaDecode::popAudioQue()
{
	AudioPackageInfo info;
	InnerPacketInfo pkg_info;
	audio_inner_que_.WaitPop(pkg_info);
	if (FLAG_PLAY == pkg_info.flag )
	{
		std::lock_guard<std::mutex> lg(audio_seek_mutex_);
		doConvertAudio(pkg_info.pkg);
		av_packet_unref(pkg_info.pkg);
		av_packet_free(&pkg_info.pkg);
		audio_que_.TryPop(info);
		return info;
	}
	else
	{
		info.error = ymc::ERROR_PKG_ERROR;
	}
	av_packet_unref(pkg_info.pkg);
	av_packet_free(&pkg_info.pkg);

	audio_que_.TryPop(info);

	return info;
}

VideoPackageInfo YMediaDecode::popVideoQue(double cur_clock)
{
	VideoPackageInfo info;
	InnerPacketInfo pkg_info;
	video_inner_que_.WaitPop(pkg_info);
	if (FLAG_PLAY == pkg_info.flag )
	{
		std::lock_guard<std::mutex> lg(video_seek_mutex_);
		doConvertVideo(pkg_info.pkg, cur_clock);
		av_packet_unref(pkg_info.pkg);
		av_packet_free(&pkg_info.pkg);
		video_que_.TryPop(info);
		return info;
	}
	else
	{
		info.error = ymc::ERROR_PKG_ERROR;
	}
	
	av_packet_unref(pkg_info.pkg);
	av_packet_free(&pkg_info.pkg);
	video_que_.TryPop(info);
	return info;
}


void YMediaDecode::freeAudioPackageInfo(AudioPackageInfo*info)
{
	if(info && info->error == ymc::ERROR_NO_ERROR)
		av_free(info->data);
}


void YMediaDecode::conductAudioBlocking()
{
	InnerPacketInfo info_audio;
	info_audio.pkg = av_packet_alloc();
	info_audio.flag = FLAG_CONDUCT_QUE;
	audio_inner_que_.push(info_audio);

	audio_cnd_.notify_all();
}

void YMediaDecode::conductVideoBlocking()
{
	InnerPacketInfo info_video;
	info_video.pkg = av_packet_alloc();
	info_video.flag = FLAG_CONDUCT_QUE;
	video_inner_que_.push(info_video);

	video_cnd_.notify_all();
}


bool YMediaDecode::judgeBlockAudioSeek()
{
	unique_lock<std::mutex> unique(audio_cnd_lock_);
	audio_cnd_.wait(unique, [&] {return audio_seek_convert_dur_ == SEEK_TIME_DEFAULT; });
	return audio_seek_convert_dur_ != SEEK_TIME_DEFAULT;
}

bool YMediaDecode::judgeBlockVideoSeek()
{
	unique_lock<std::mutex> unique(video_cnd_lock_);
	video_cnd_.wait(unique, [&] { return video_seek_convert_dur_ == SEEK_TIME_DEFAULT; });
	return video_seek_convert_dur_ != SEEK_TIME_DEFAULT;
}

void YMediaDecode::addError(ymc::DecodeError error)
{
	error_ |= error;
}

void YMediaDecode::decodeThread()
{
	is_seek_ = false;
	audio_seek_convert_dur_ = SEEK_TIME_DEFAULT;
	video_seek_convert_dur_ = SEEK_TIME_DEFAULT;
	is_manual_stop_ = false;
	uint8_t* pic_buff=nullptr;

	std::shared_ptr<FormatCtx> format = std::make_shared<FormatCtx>(YMediaDecode::readBuff, YMediaDecode::seekBuff, this);
	format_ctx_ = format;
	if (!format->initFormatCtx(path_file_.c_str()))
	{
		printf("InitFormatCtx Error\n");
		
		if (error_ & ymc::ERROR_READ_TIME_OUT)
		{
			notifyDecodeStatus(ymc::ERROR_READ_TIME_OUT);
		}
		else if (error_ & ymc::ERROR_READ_USER_INTERRUPT)
		{
			notifyDecodeStatus(ymc::ERROR_READ_USER_INTERRUPT);
		}
		else if (error_ & ymc::ERROR_FORMAT)
		{
			notifyDecodeStatus(ymc::ERROR_FORMAT);
		}
		return;
	}

	//notify media info for player
	MediaInfo info;
	info.dur = (double)format->ctx_->duration / AV_TIME_BASE;
	notifyMediaInfo(info);

	//this is for audio
	std::shared_ptr<CodecCtx> audio_ctx = std::make_shared<CodecCtx>(format->ctx_, AVMEDIA_TYPE_AUDIO);
	audio_codec_ = audio_ctx;

	std::shared_ptr<AVFrameManger > audio_frame = std::make_shared<AVFrameManger>();
	audio_frame_ = audio_frame;

	if (!audio_ctx->initDecoder())
	{
		printf("audio_ctx.InitDecoder Error\n");
		notifyDecodeStatus(ymc::ERROR_FORMAT);
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
	if (!video_ctx->initDecoder())
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
			if (audio_ctx->isValid())
			{
				if (av_seek_frame(format->ctx_, audio_ctx->stream_index_, audio_seek_convert_dur_, AVSEEK_FLAG_FRAME) >= 0)
				{
					emptyAudioQue();
					avcodec_flush_buffers(audio_ctx->codec_ctx_);
				}
			}
			
			if (video_ctx->isValid())
			{
				if (av_seek_frame(format->ctx_, video_ctx->stream_index_, video_seek_convert_dur_, AVSEEK_FLAG_FRAME) >= 0)
				{
					emptyVideoQue();
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

	flushAudioDecodec();
	flushVideoDecodec();

	printf("YMediaDecode quit\n");
}



void YMediaDecode::doConvertAudio(AVPacket *pkg)
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
		double pts = audio_frame->frame_->pts *av_q2d(codec_ctx->getStream()->time_base);
		//printf("pts:pts:pts:%f\n",pts);
		
		AudioPackageInfo info;
		info.data = data;
		info.size = Audiobuffer_size;
		info.pts = pts;
		info.error = ymc::ERROR_NO_ERROR;
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

	frame_delay = av_q2d(codec->getStream()->time_base);
	frame_delay += srcFrame->repeat_pict * (frame_delay*0.5);

	video_clock += frame_delay;

	return video_clock;
}

void YMediaDecode::flushVideoDecodec()
{
	auto video_codec = video_codec_.lock();
	if (video_codec->isValid())
	{
		InnerPacketInfo info_video;
		info_video.pkg = av_packet_alloc();
		info_video.pkg->size = 0;
		info_video.pkg->data = nullptr;
		info_video.flag = FLAG_FLUSH_DECODEC;
		doConvertVideo(info_video.pkg, 0);
	}
}

void YMediaDecode::flushAudioDecodec()
{
	auto audio_codec = audio_codec_.lock();
	if (audio_codec->isValid())
	{
		InnerPacketInfo info_audio;
		info_audio.pkg = av_packet_alloc();
		info_audio.pkg->size = 0;
		info_audio.pkg->data = nullptr;
		info_audio.flag = FLAG_FLUSH_DECODEC;
		doConvertAudio(info_audio.pkg);
	}
}

int YMediaDecode::readBuff(void *opaque, uint8_t *read_buf, int read_buf_size)
{
	YMediaDecode* op = (YMediaDecode*)opaque;
	if (op)
	{
		int64_t result = op->getDelegate()->onRead((char*)read_buf, read_buf_size);
		if (result > 0)
		{
			return result;
		}
		else if (result  == -2)
		{
			op->addError(ymc::ERROR_READ_TIME_OUT);
		}
		else if (result == -3)
		{
			op->addError(ymc::ERROR_READ_USER_INTERRUPT);
		}
		return -1;
	}
	return -1;
}

int64_t YMediaDecode::seekBuff(void *opaque, int64_t offset, int whence)
{
	YMediaDecode* op = (YMediaDecode*)opaque;
	if (op)
	{
		int64_t result = op->getDelegate()->onSeek(offset, whence);
		return result;
	}
	return	-1;
}

void YMediaDecode::notifyDecodeStatus(ymc::DecodeError error)
{
	if (delegate_)
		delegate_->onDecodeError(error);
}

void YMediaDecode::notifyMediaInfo(MediaInfo info)
{
	if(delegate_)
		delegate_->onMediaInfo(info);
}

void YMediaDecode::doConvertVideo(AVPacket *pkg, double cur_clock)
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
		convert->convert(video_frame->frame_->data, video_frame->frame_->linesize, codec_ctx->codec_ctx_->height);
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

			video_pts *= av_q2d(codec_ctx->getStream()->time_base);
			video_pts = synchronize(codec_ctx, convert->rgb_frame_.frame_, video_pts, cur_clock);

			VideoPackageInfo info;
			info.data = convert->rgb_frame_.frame_->data[0];
			info.width = codec_ctx->codec_ctx_->width;
			info.height = codec_ctx->codec_ctx_->height;
			info.pts = video_pts;
			info.error = ymc::ERROR_NO_ERROR;
			video_que_.push(info);
	}
}