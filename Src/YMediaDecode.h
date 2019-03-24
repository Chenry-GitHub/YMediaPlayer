#pragma  once
#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <string>
#include <memory>
#include <functional>
#include <algorithm>
using namespace std;




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
#include "YMediaComm.h"



struct InnerPacketInfo {
	AVPacket *pkg;
	FLAG_PKG flag = FLAG_PLAY;
};


class FormatCtx;
class CodecCtx;
class AVFrameManger;
class AudioConvertManger;
class VideoConvertManger;
class YMediaDecode
{
public:
	YMediaDecode();
	~YMediaDecode();

	/*设置媒体信息*/
	bool SetMedia(const std::string & path_file,int sample_rate,int channel);

	/*开始解码*/
	bool StartDecode();

	/*停止解码*/
	bool StopDecode();

	/*Play层调用，设置seek位置*/
	void SeekPos(double pos);

	/*Play层调用设置回调错误信息*/
	void SetErrorFunction(std::function<void(DecodeError)> error_func)
	{
		error_func_ = error_func;
	}

	/*Play层调用设置回调信息*/
	void SetMediaFunction(std::function<void(MediaInfo)> func)
	{
		media_func_ = func;
	}

	/*Play的Read回调方法*/
	void SetReadMemFunction(std::function<int (char *data, int len)> func)
	{
		read_func_ = func;
	}

	void SetSeekMemFunction(std::function<int64_t(int64_t offset, int whence)> func)
	{
		seek_func_ = func;
	}

	/*Play层调用，释放播放后的包体*/
	void FreeAudioPackageInfo(AudioPackageInfo*);

	/*清空音频，视频队列*/
	void EmptyAudioQue();
	void EmptyVideoQue();

	/*Play层调用获取数据，分别在不同的线程中调用*/
	AudioPackageInfo PopAudioQue();
	VideoPackageInfo PopVideoQue(double cur_clock); 

	/*导通Play层阻塞*/
	void ConductAudioBlocking();
	void ConductVideoBlocking();

	/*用于Play层调用，出现阻塞seek操作*/
	bool JudgeBlockAudioSeek();
	bool JudgeBlockVideoSeek();
protected:

	/*解码线程*/
	void DecodeThread();

	/*解码转换*/
	void DoConvertAudio(AVPacket *pkg);
	void DoConvertVideo(AVPacket *pkg,double cur_clock);

	/*获取视频恰当的pts*/
	double synchronize(std::shared_ptr<CodecCtx>,AVFrame *srcFrame, double pts, double cur_clock);

	/*退出解码线程需要调用的*/
	void FlushVideoDecodec();
	void FlushAudioDecodec();

	static int ReadBuff(void *opaque, uint8_t *buf, int buf_size);

	static int64_t SeekBuff(void *opaque, int64_t offset, int whence);
private:

	void NotifyDecodeStatus(DecodeError);

	void NotifyMediaInfo(MediaInfo info);

	std::string path_file_;
	std::thread decode_thread_;
	atomic_bool is_manual_stop_;
	atomic_int sample_rate_;
	atomic_int channel_;
	atomic_bool is_seek_;
	std::atomic<long long>audio_seek_convert_dur_;
	std::atomic<long long> video_seek_convert_dur_;

	ThreadSafe_Queue<AudioPackageInfo> audio_que_;
	ThreadSafe_Queue<VideoPackageInfo> video_que_;

	ThreadSafe_Queue<InnerPacketInfo> audio_inner_que_;
	ThreadSafe_Queue<InnerPacketInfo> video_inner_que_;

	std::weak_ptr<FormatCtx>		format_ctx_;
	std::weak_ptr<CodecCtx>		audio_codec_;
	std::weak_ptr<CodecCtx>		video_codec_;

	std::weak_ptr<AudioConvertManger> audio_convert_;
	std::weak_ptr<VideoConvertManger> video_convert_;
	

	std::weak_ptr<AVFrameManger> audio_frame_;
	std::weak_ptr<AVFrameManger> video_frame_;

	std::mutex audio_seek_mutex_;
	std::mutex video_seek_mutex_;

	std::mutex audio_cnd_lock_;
	std::mutex video_cnd_lock_;
	std::condition_variable audio_cnd_;
	std::condition_variable video_cnd_;
	
	std::function<void (DecodeError)> error_func_;
	std::function<void(MediaInfo)> media_func_;
	std::function<int(char *data, int len)> read_func_;
	std::function<int64_t(int64_t offset, int whence)> seek_func_;

};

struct MemReadStruct {
	YMediaDecode *target;
};
using ReadFunc = int(*) (void *opaque, uint8_t *buf, int buf_size);
using SeekFunc = int64_t(*) (void *opaque, int64_t offset, int whence);

class FormatCtx {
public:
	inline FormatCtx(ReadFunc read_func, SeekFunc seek_func,MemReadStruct read)
		: open_input_(false)
	{
		readst_ = std::make_shared<MemReadStruct>(read);
#define  READ_BUFFER_SIZE 32768 //32KB
		buffer_ = (unsigned char*)av_malloc(READ_BUFFER_SIZE);
		ioctx_ = avio_alloc_context(buffer_, READ_BUFFER_SIZE, 0, &readst_, read_func, NULL, seek_func);
		ctx_= avformat_alloc_context();

		std::memset(buffer_, 0, READ_BUFFER_SIZE);

	
		int size = READ_BUFFER_SIZE - AVPROBE_PADDING_SIZE;

	//	const size_t probeSize = 4096;
	//	const size_t bufSize = probeSize + AVPROBE_PADDING_SIZE;

	//	std::unique_ptr<std::uint8_t[]> buffer(new std::uint8_t[bufSize]);

		seek_func(&readst_, 0, SEEK_SET);

		int readBytes  = read_func(&readst_, buffer_, size);

		if (readBytes <= 0)
			return;

		// Fill any padding with 0s.
		//std::fill(buffer.get() + actuallyRead, buffer.get() + bufSize, 0);

		seek_func(&readst_, 0, SEEK_SET);
	

		/*if (actuallyRead < 1) {
			throw IOException(_("MediaParserFfmpeg could not read probe data "
				"from input"));
		}*/

		// Probe the file to detect the format
		AVProbeData probe_data;
		probe_data.filename = "";
		probe_data.buf = buffer_;
		probe_data.buf_size = readBytes;
#if LIBAVFORMAT_VERSION_MAJOR > 55
		probe_data.mime_type = nullptr;
#endif

		AVInputFormat* ret = av_probe_input_format(&probe_data, 1);
		
		ctx_->pb = ioctx_;
		ctx_->iformat = ret;
		ctx_->flags = AVFMT_FLAG_CUSTOM_IO;


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
		
		

		if (avformat_open_input(&ctx_, "", 0, 0) < 0)
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

	inline bool seek()
	{

	}

	void release_package()
	{
		av_packet_unref(pkg_);
	}

	bool open_input_;
	unsigned char *buffer_;
	AVIOContext *ioctx_;
	AVFormatContext* ctx_;
	AVPacket *pkg_;
	shared_ptr<MemReadStruct> readst_;
};

class CodecCtx {
public:
	inline CodecCtx(AVFormatContext * format_ctx, AVMediaType type)//AVMEDIA_TYPE_AUDIO
		:format_(format_ctx)
		, codec_ctx_(nullptr)
		, type_(type)
		, stream_index_(-1)
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
			return false;
		}
		else {
			stream_index = ret;
			st = format_->streams[stream_index];

			/* find decoder for the stream */
			dec = avcodec_find_decoder(st->codecpar->codec_id);
			if (!dec) {
				fprintf(stderr, "Failed to find %s codec\n",
					av_get_media_type_string(type_));
				return false;
			}

			/* Allocate a codec context for the decoder */
			codec_ctx_ = avcodec_alloc_context3(dec);
			if (!codec_ctx_) {
				fprintf(stderr, "Failed to allocate the %s codec context\n",
					av_get_media_type_string(type_));
				return false;
			}

			/* Copy codec parameters from input stream to output codec context */
			if ((ret = avcodec_parameters_to_context(codec_ctx_, st->codecpar)) < 0) {
				fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
					av_get_media_type_string(type_));
				return false;
			}

			/* Init the decoders, with or without reference counting */
			av_dict_set(&opts, "refcounted_frames", "0", 0);
			if ((ret = avcodec_open2(codec_ctx_, dec, &opts)) < 0) {
				fprintf(stderr, "Failed to open %s codec\n",
					av_get_media_type_string(type_));
				return false;
			}
			stream_index_ = stream_index;
			return true;
		}
	}

	AVStream *GetStream()
	{
		return format_->streams[stream_index_];
	}

	bool IsValid()
	{
		return stream_index_ >= 0;
	}

	AVFormatContext *format_;
	AVCodecContext *codec_ctx_;
	int stream_index_;
	AVMediaType type_;
};

class AVFrameManger
{
public:
	AVFrameManger()
	{
		frame_ = av_frame_alloc();
	}
	~AVFrameManger()
	{
		av_frame_free(&frame_);
	}
	AVFrame *frame_;
};

struct AudioConvertParameter
{
	int des_layout;
	AVSampleFormat des_fmt;
	int des_sample_rate;
	int src_channel;
	AVSampleFormat src_sample_fmt;
	int src_sample_rate;
};

class AudioConvertManger
{
public:
	AudioConvertManger(AudioConvertParameter parameter)
	{
		audio_convert_ctx_ = swr_alloc();
		audio_convert_ctx_ = swr_alloc_set_opts(audio_convert_ctx_,
			parameter.des_layout,
			parameter.des_fmt,
			parameter.des_sample_rate,
			parameter.src_channel,
			parameter.src_sample_fmt,
			parameter.src_sample_rate,
			0,
			NULL);
		swr_init(audio_convert_ctx_);
	}
	~AudioConvertManger()
	{
		swr_free(&audio_convert_ctx_);
	}

	SwrContext* audio_convert_ctx_;
};

class VideoConvertManger
{
public:
	VideoConvertManger(int width,int height,AVPixelFormat fmt)
	{
		int pic_size_ = av_image_get_buffer_size(AV_PIX_FMT_RGB32, width, height,1);
		pic_buff = (uint8_t*)av_malloc(pic_size_);
		av_image_fill_arrays(rgb_frame_.frame_->data, rgb_frame_.frame_->linesize, pic_buff, AV_PIX_FMT_RGB32, width, height,1);
		video_convert_ctx_ = sws_getContext(width, height, fmt, width, height, AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);

	}

	~VideoConvertManger()
	{
		av_free(pic_buff);
		sws_freeContext(video_convert_ctx_);
	}

	void Convert(const uint8_t *const* src_data,int *src_stride,int height)
	{
		sws_scale(video_convert_ctx_, src_data, src_stride, 0,height, rgb_frame_.frame_->data, rgb_frame_.frame_->linesize);
	}

	SwsContext* video_convert_ctx_;
	uint8_t * pic_buff;
	AVFrameManger rgb_frame_;
};
