#pragma  once
#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <string>
#include <memory>
using namespace std;

#include <windows.h>

class GLFWwindow ;
extern GLFWwindow  *g_hwnd;

extern "C"
{
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include "ThreadSafe_Queue.h"
#include <glew.h>
//#include "Shader.h"


#define  QUE_PACKAGEINFO_SIZE 200

enum YMediaCallBackType
{
	MEDIA_ERROR,
};





enum YMediaPlayerError
{
	ERROR_NO_ERROR = 0,
	ERROR_NO_QUIT,
	ERROR_FILE_ERROR,
	
};

struct VideoPackageInfo
{
	void *data;
	int width;
	int height;
	double pts;
	double clock;
};


struct AudioPackageInfo
{
	void *data;
	int size = 0;
	double pts;
	double dur;
	int sample_rate;
	int channels;
	YMediaPlayerError error = ERROR_NO_ERROR;
};


using Player_CallBack = void(*) (LPARAM lp, WPARAM wp);

#define _YMEDIA_CALLBACK(callback, val1 ,val2) if(callback){ callback(val1,val2); }

class FormatCtx;
class CodecCtx;
class YMediaDecode
{
public:
	YMediaDecode();
	~YMediaDecode();

	void SetPlayerCallBack(Player_CallBack call_back);

	bool SetMedia(const std::string & path_file);

	bool Pause();

	bool StopDecode();

	bool StartDecode();

	void EmptyAudioQue();

	AudioPackageInfo PopAudioQue();

	VideoPackageInfo PopVideoQue();

	void PushAudioQue(void *data,int size,int sample_rate,int channel, double dur, double pts, YMediaPlayerError error);

	void ReleasePackageInfo(AudioPackageInfo*);

protected:

	void DecodecThread();

	void DoConvertAudio(AVPacket *pkg);

	void DoConvertVideo(AVPacket *pkg);

	double synchronize(std::shared_ptr<CodecCtx>,AVFrame *srcFrame, double pts);
private:

	std::string path_file_;

	std::thread decodec_thread_;

	YMediaPlayerError error_;

	Player_CallBack  call_back_;

	atomic_bool is_need_stop_;

	ThreadSafe_Queue<AudioPackageInfo> audio_que_;
	ThreadSafe_Queue<VideoPackageInfo> video_que_;

	ThreadSafe_Queue<AVPacket> audio_inner_que_;
	ThreadSafe_Queue<AVPacket> video_inner_que_;

	std::weak_ptr<FormatCtx>		format_ctx_;
	std::weak_ptr<CodecCtx>		audio_codec_;
	std::weak_ptr<CodecCtx>		video_codec_;

	SwrContext* audio_convert_ctx_;
	SwsContext* video_convert_ctx_;

	//for video
	int pic_size_;
	uint8_t* pic_buff_;
	AVFrame *rgb_frame_;
	//

	//for audio
	AVFrame *audio_frame_;


	///
	//Shader * shader_ptr_;
	GLuint vao;
	GLuint vert_buf;
	GLuint elem_buf;
	GLuint frame_tex;
	GLuint program;
	GLuint attribs[2];
	GLuint uniforms[2];
	///

};



class FormatCtx {
public:
	inline FormatCtx()
		: open_input_(false){
		ctx_= avformat_alloc_context();
		pkg_ = av_packet_alloc();
		av_init_packet(pkg_);
	}

	inline ~FormatCtx() {
		if (open_input_)
		{
			avformat_close_input(&ctx_);
		}
		av_packet_free(&pkg_);
		avformat_free_context(ctx_);
	}

	bool InitFormatCtx(const char* filename)
	{
		if (avformat_open_input(&ctx_, filename, 0, 0) != 0)
		{
			return false;
		}
		open_input_ = true;
		if (avformat_find_stream_info(ctx_, 0) < 0)
		{
			return false;
		}
		return true;
	}


	inline bool read() {
		return  av_read_frame(ctx_, pkg_) >= 0;
	}

	void release_package()
	{
		av_packet_unref(pkg_);
	}

	bool open_input_;
	AVFormatContext* ctx_;
	AVPacket *pkg_;
};

class CodecCtx {
public:
	inline CodecCtx(AVFormatContext * format_ctx, AVMediaType type)//AVMEDIA_TYPE_AUDIO
		:format_(format_ctx)
		, codec_ctx_(nullptr)
		, type_(type)
	{

	}

	~CodecCtx()
	{
		if (codec_ctx_)
		{
			avcodec_close(codec_ctx_);
			avcodec_free_context(&codec_ctx_);
		}
	}

	bool InitDecoder()
	{
		if (!format_)
			return false;

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

	AVStream *GetStream()
	{
		return format_->streams[stream_index_];
	}

	AVFormatContext *format_;
	AVCodecContext *codec_ctx_;
	int stream_index_;
	AVMediaType type_;
};

