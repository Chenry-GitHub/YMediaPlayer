#include "YMediaPlayer.h"

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096
#define AUDIO_OUT_SAMPLE_RATE 44100
#define MAX_AUDIO_FRAME_SIZE 192000 




YMediaPlayer::YMediaPlayer()
	: call_back_(nullptr)
{
	
}

YMediaPlayer::~YMediaPlayer()
{

}

void YMediaPlayer::SetPlayerCallBack(Player_CallBack call_back)
{
	std::lock_guard<std::mutex> lg(g_audio_player_mutex);
	call_back_ = call_back;
}

bool YMediaPlayer::SetMedia(const std::string & path_file)
{
	Stop();
	path_file_ = path_file;
	return true;
}

bool YMediaPlayer::Play()
{
	if (!play_thread_.native_handle())
	{
		std::thread th(PlayThread,this);
		play_thread_.swap(th);
		play_thread_.detach();
		return true;
	}
	else
	{
		return false;
	}
	return true;
}

bool YMediaPlayer::Pause()
{
	return true;
}

bool YMediaPlayer::Stop()
{
	is_terminate_ = true;
	std::lock_guard<std::mutex> lg(g_audio_player_mutex);
	error_ = YMediaPlayerError::ERROR_NO_ERROR;
	return true;
}

void YMediaPlayer::PlayThread(YMediaPlayer* player)
{
	player->DoPlay(); //block here!!
}

void YMediaPlayer::DoPlay()
{
	std::lock_guard<std::mutex> lg(g_audio_player_mutex);

	is_terminate_ = false;
	
	av_register_all();

	WavPlayer wav_player;

	FormatCtx format;
	if (!format.InitFormatCtx(path_file_.c_str()))
	{
		error_ = YMediaPlayerError::ERROR_FILE_ERROR;
		_YMEDIA_CALLBACK(call_back_, MEDIA_ERROR, 0)
		return;
	}


	CodecCtx audio_ctx(format.ctx_, AVMEDIA_TYPE_AUDIO);
	if (!audio_ctx.InitDecoder())
	{
		error_ = YMediaPlayerError::ERROR_FILE_ERROR;
		_YMEDIA_CALLBACK(call_back_, MEDIA_ERROR, 0)
		return;
	}

	CodecCtx video_ctx(format.ctx_, AVMEDIA_TYPE_VIDEO);
	if (!video_ctx.InitDecoder())
	{
	
	}

	int out_channel = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
	int out_size = av_samples_get_buffer_size(NULL, out_channel, audio_ctx.codec_ctx_->frame_size, AV_SAMPLE_FMT_S16, 1);
	wav_player.InitPlayer(AUDIO_OUT_SAMPLE_RATE, out_channel, out_size);

	int in_sample_per_size =  av_get_bytes_per_sample(audio_ctx.codec_ctx_->sample_fmt);

	AVFrame *decoded_frame = av_frame_alloc();
	while (format.read() && !is_terminate_)
	{
		if (format.pkg_->stream_index == audio_ctx.stream_index_)
		{
			DecodeAudio(&wav_player,&format, &audio_ctx, decoded_frame,out_size);
		}
		else if (format.pkg_->stream_index == video_ctx.stream_index_)
		{
			//	printf("video_stream :%d %d %d\n",bytes_persample, formatCtx_.pkg_.pos, formatCtx_.pkg_.size);
		}
		format.release_package();
	}

	/* flush the decoder */
	if (audio_ctx.codec_ctx_)
	{
		format.pkg_->size = 0;
		format.pkg_->data = nullptr;
		DecodeAudio(&wav_player, &format, &audio_ctx, decoded_frame, out_size);
	}
	av_frame_free(&decoded_frame);

}

void YMediaPlayer::DecodeAudio(WavPlayer *wav_player ,FormatCtx* format_ctx, CodecCtx * codec_ctx,  AVFrame *frame, int out_sample_size)
{
	static  SwrContext *au_convert_ctx = nullptr;
	static uint8_t * out_buffer = nullptr;
	if (!au_convert_ctx)
	{
		au_convert_ctx = swr_alloc();
		au_convert_ctx = swr_alloc_set_opts(au_convert_ctx,
			AV_CH_LAYOUT_STEREO,
			AV_SAMPLE_FMT_S16,
			AUDIO_OUT_SAMPLE_RATE,
			av_get_default_channel_layout(codec_ctx->codec_ctx_->channels),
			codec_ctx->codec_ctx_->sample_fmt,
			codec_ctx->codec_ctx_->sample_rate,
			0,
			NULL);
		swr_init(au_convert_ctx);
		out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
	}

	int ret = avcodec_send_packet(codec_ctx->codec_ctx_, format_ctx->pkg_);
	if (ret != 0)
	{
		printf("avcodec_send_packet Error!\n");
		return;
	}

	/* read all the output frames (in general there may be any number of them */
	while ((ret = avcodec_receive_frame(codec_ctx->codec_ctx_, frame)) == 0)
	{
		swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)frame->data, frame->nb_samples);

		printf("audio_stream :%d %d %d %d--size\n", frame->nb_samples, out_sample_size, format_ctx->pkg_->pts, format_ctx->pkg_->size);

		wav_player->AddBuff((char*)out_buffer, out_sample_size);

	}
}
