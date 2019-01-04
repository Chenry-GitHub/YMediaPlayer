#include "YMediaDecode.h"
#include "WavPlayer.h"


#include <glm.hpp>
#include <ext.hpp>
#include <glfw3.h>

GLfloat vertexArray[12] = { -0.9f, -0.9f, 0.0f,
0.9f, -0.9f, 0.0f,
0.9f,  0.9f, 0.0f,
-0.9f,  0.9f, 0.0f };

GLfloat texCoord[8] = { 0.0f, 0.0f,
1.0f, 0.0f,
1.0f, 1.0f,
0.0f, 1.0f };         //与jpg图片存储有关，倒置的

#define AUDIO_OUT_SAMPLE_RATE 44100
#define MAX_AUDIO_FRAME_SIZE 192000 /*one second bytes  44100*2*2 = 176400*/
#define AUDIO_OUT_CHANNEL 2
GLFWwindow  *g_hwnd;
 
GLuint vao;
GLuint vertex_buffer;
GLuint texture_buffer;
GLuint TextureID;
GLuint vert_shader, frag_shader;
GLuint program;


/************************************************************************************/
#include <stdio.h>
#include <assert.h>
#define MAX_SHADER_LENGTH 8192
#define MAX_TARGET_NUM  10

GLchar shaderText[MAX_SHADER_LENGTH];

void gltLoadShaderSrc(const char *szShaderSrc, GLuint shader)
{
	GLchar *fsStringPtr[1];

	fsStringPtr[0] = (GLchar *)szShaderSrc;
	glShaderSource(shader, 1, (const GLchar **)fsStringPtr, NULL);
}

bool gltLoadShaderFile(const char *szFile, GLuint shader)
{
	GLint shaderLength = 0;
	FILE *fp;

	// Open the shader file
	fp = fopen(szFile, "r");
	if (fp != NULL)
	{
		// See how long the file is
		while (fgetc(fp) != EOF)
			shaderLength++;

		// Allocate a block of memory to send in the shader
		assert(shaderLength < MAX_SHADER_LENGTH);   // make me bigger!
		if (shaderLength > MAX_SHADER_LENGTH)
		{
			fclose(fp);
			return false;
		}

		// Go back to beginning of file
		rewind(fp);

		// Read the whole file in
		if (shaderText != NULL)
			fread(shaderText, 1, shaderLength, fp);

		// Make sure it is null terminated and close the file
		shaderText[shaderLength] = '\0';
		fclose(fp);
	}
	else
		return false;

	// Load the string
	gltLoadShaderSrc((const char *)shaderText, shader);

	return true;
}




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

void YMediaDecode::PushAudioQue(void *data, int size, int sample_rate, int channel, YMediaPlayerError error)
{
	PackageInfo info;
	info.data = data;
	info.size = size;
	info.sample_rate = sample_rate;
	info.channels = channel;
	info.error = error;
	audio_que_.push(info);
}

void YMediaDecode::ReleasePackageInfo(PackageInfo*info)
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
		PushAudioQue(nullptr, 0, AUDIO_OUT_SAMPLE_RATE, AUDIO_OUT_CHANNEL, ERROR_FILE_ERROR);
		return;
	}


	CodecCtx audio_ctx(format.ctx_, AVMEDIA_TYPE_AUDIO);
	if (!audio_ctx.InitDecoder())
	{
		error_ = YMediaPlayerError::ERROR_FILE_ERROR;
		_YMEDIA_CALLBACK(call_back_, MEDIA_ERROR, error_)
		printf("audio_ctx.InitDecoder Error\n");
		PushAudioQue(nullptr, 0, AUDIO_OUT_SAMPLE_RATE, AUDIO_OUT_CHANNEL, ERROR_NO_QUIT);
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
			return ;
		}
		m_pFrameRGB = av_frame_alloc();
		avpicture_fill((AVPicture *)m_pFrameRGB, m_buf, AV_PIX_FMT_BGR24, video_ctx.codec_ctx_->width, video_ctx.codec_ctx_->height);
		m_pSwsCtx = sws_getContext(video_ctx.codec_ctx_->width, video_ctx.codec_ctx_->height, video_ctx.codec_ctx_->pix_fmt, video_ctx.codec_ctx_->width, video_ctx.codec_ctx_->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
	
	
		glfwMakeContextCurrent(g_hwnd);


		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER,
			4 * sizeof(GLfloat) * 3,
			vertexArray, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
		glEnableVertexAttribArray(0);

		glGenBuffers(1, &texture_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, texture_buffer);
		glBufferData(GL_ARRAY_BUFFER,
			4 * sizeof(GLfloat) * 2,
			texCoord, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
		glEnableVertexAttribArray(1);

		//create map texture object
		glGenTextures(1, &TextureID);
		glBindTexture(GL_TEXTURE_2D, TextureID);
		/* Setup some parameters for texture filters and mipmapping */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


		vert_shader = glCreateShader(GL_VERTEX_SHADER);
		frag_shader = glCreateShader(GL_FRAGMENT_SHADER);

		gltLoadShaderFile("C:/Identity.vp", vert_shader);
		gltLoadShaderFile("C:/Identity.fp", frag_shader);

		glCompileShader(vert_shader);
		glCompileShader(frag_shader);

		program = glCreateProgram();

		glAttachShader(program, vert_shader);
		glAttachShader(program, frag_shader);

		glLinkProgram(program);


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
			printf("format.read() Error\n");
			break;
		}

		if (format.pkg_->stream_index == audio_ctx.stream_index_)
		{
			DoDecodeAudio( &format, &audio_ctx, decoded_frame);
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
		DoDecodeAudio(&format, &audio_ctx, decoded_frame);
	}
	av_frame_free(&decoded_frame);


	printf("YMediaDecode quit\n");
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
		int dur = av_rescale_q(format_ctx->pkg_->duration, codec_ctx->codec_ctx_->time_base, AVRational{ 1, AV_TIME_BASE });;
		int outSizeCandidate = AUDIO_OUT_SAMPLE_RATE * 8 *double(dur) / 1000000.0;
		uint8_t *out_buffer = (uint8_t *)av_malloc(sizeof uint8_t *outSizeCandidate);
		int out_samples =	swr_convert(au_convert_ctx, &out_buffer, outSizeCandidate, (const uint8_t **)&frame->data[0], frame->nb_samples);

		int Audiobuffer_size = av_samples_get_buffer_size(NULL,
			AUDIO_OUT_CHANNEL, out_samples, AV_SAMPLE_FMT_S16, 1);

		//Audiobuffer_size -= 40;

		//printf("audio_stream :%d %d %d %d--size\n", frame->nb_samples, Audiobuffer_size, format_ctx->pkg_->pts, format_ctx->pkg_->size);

		void * data = av_malloc(Audiobuffer_size);
		memcpy_s(data, Audiobuffer_size, out_buffer, Audiobuffer_size);
		//push to media player
		PushAudioQue(data, Audiobuffer_size, AUDIO_OUT_SAMPLE_RATE, AUDIO_OUT_CHANNEL,ERROR_NO_ERROR);
		av_free(out_buffer);
	}
#else

	int got=0;
	int result = avcodec_decode_audio4(codec_ctx->codec_ctx_,frame,&got, format_ctx->pkg_);
	//if(result>0)

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
	PushAudioQue(data, Audiobuffer_size, AUDIO_OUT_SAMPLE_RATE, AUDIO_OUT_CHANNEL, ERROR_NO_ERROR);
	av_free(out_buffer);
#endif

	swr_free(&au_convert_ctx);
}
void ShowRGBToWnd(HWND hWnd, BYTE* data, int width, int height)
{
	if (data == NULL)
		return;

	static BITMAPINFO *bitMapinfo = NULL;
	static bool First = TRUE;

	if (First)
	{
		BYTE * m_bitBuffer = new BYTE[40 + 4 * 256];//开辟一个内存区域  

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

			glTexImage2D(GL_TEXTURE_2D, 0,
				GL_RGB,
				codec_ctx->codec_ctx_->width, codec_ctx->codec_ctx_->height, 0,
				GL_RGB, GL_UNSIGNED_BYTE, m_pFrameRGB->data[0]);
	

			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			GLuint sampler_location;
			glUseProgram(program);
			sampler_location = glGetUniformLocation(program, "colorMap");
			glUniform1i(sampler_location, 0);

			glBindTexture(GL_TEXTURE_2D, TextureID);

			glBindVertexArray(vao);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			glBindVertexArray(0);


			glfwSwapBuffers(g_hwnd);

			av_frame_unref(pFrame);
		}
	}

}
