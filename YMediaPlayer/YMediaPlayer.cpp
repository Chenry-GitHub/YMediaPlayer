#include "YMediaPlayer.h"

#define CLEAR_MAP(map_ )  \
for (auto iter = map_.begin(); iter != map_.end();)\
	iter = map_.erase(iter);

#include <glm.hpp>
#include <ext.hpp>
#include <glfw3.h>
//#include "qaqlog\qaqlog.h"


GLfloat vertexArray[12] = { -0.9f, -0.9f, 0.0f,
0.9f, -0.9f, 0.0f,
0.9f,  0.9f, 0.0f,
-0.9f,  0.9f, 0.0f };

GLfloat texCoord[8] = { 0.0f, 0.0f,
1.0f, 0.0f,
1.0f, 1.0f,
0.0f, 1.0f };         //ÓëjpgÍ¼Æ¬´æ´¢ÓÐ¹Ø£¬µ¹ÖÃµÄ

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

YMediaPlayer::YMediaPlayer()
{
	alGenSources(1, &source_id_);
	ALfloat SourcePos[] = { 0.0, 0.0, 0.0 };
	ALfloat SourceVel[] = { 0.0, 0.0, 0.0 };
	ALfloat ListenerPos[] = { 0.0, 0, 0 };
	ALfloat ListenerVel[] = { 0.0, 0.0, 0.0 };
	// first 3 elements are "at", second 3 are "up"
	ALfloat ListenerOri[] = { 0.0, 0.0, -1.0,  0.0, 1.0, 0.0 };
	alSourcef(source_id_, AL_PITCH, 1.0);
	alSourcef(source_id_, AL_GAIN, 1.0);
	alSourcefv(source_id_, AL_POSITION, SourcePos);
	alSourcefv(source_id_, AL_VELOCITY, SourceVel);
	alSourcef(source_id_, AL_REFERENCE_DISTANCE, 50.0f);
	alSourcei(source_id_, AL_LOOPING, AL_FALSE);
	alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
	alListener3f(AL_POSITION, 0, 0, 0);

	alGenBuffers(NUMBUFFERS, audio_buf_);

	Stop();


}

YMediaPlayer::~YMediaPlayer()
{
	Stop();
	alDeleteBuffers(NUMBUFFERS, audio_buf_);
	alDeleteSources(1, &source_id_);
}

int YMediaPlayer::InitPlayer()
{
	ALCdevice* pDevice;
	ALCcontext* pContext;

	pDevice = alcOpenDevice(NULL);
	pContext = alcCreateContext(pDevice, NULL);
	alcMakeContextCurrent(pContext);

	if (alcGetError(pDevice) != ALC_NO_ERROR)
		return AL_FALSE;

	return AL_TRUE;
}

int YMediaPlayer::UnInitPlayer()
{
	ALCcontext* pCurContext;
	ALCdevice* pCurDevice;

	pCurContext = alcGetCurrentContext();
	pCurDevice = alcGetContextsDevice(pCurContext);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(pCurContext);
	alcCloseDevice(pCurDevice);

	return AL_TRUE;
}

bool YMediaPlayer::SetMediaFromFile(const std::string & path_file)
{
	Stop();
	printf("Stop\n");

	path_file_ = path_file;

	decoder_.SetMedia(path_file);
	printf("decoder_.SetMedia\n");

	audio_thread_ = std::move(std::thread(&YMediaPlayer::AudioPlayThread, this));

	//TODO
	video_thread_ = std::move(std::thread(&YMediaPlayer::VideoPlayThread, this));

	while (!is_prepare_)//wait for buffer file buff done!
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	printf("SetMediaFromFile\n");
	return true;
}

bool YMediaPlayer::Play()
{
	is_pause_ = false;

	int state;
	alGetSourcei(source_id_, AL_SOURCE_STATE, &state);
	if (state == AL_STOPPED || state == AL_INITIAL || state == AL_PAUSED)
	{
		alSourcePlay(source_id_);
	}
	return true;
}

bool YMediaPlayer::Pause()
{
	is_need_stop_ = false;
	is_pause_ = true;
	return true;
}

bool YMediaPlayer::Stop()
{
	audio_clock_ = 0.0f;
	video_clock_ = 0.0f;

	is_prepare_ = false;
	is_need_stop_ = true;
	is_pause_ = true;

	alSourceStop(source_id_);
	alSourcei(source_id_, AL_BUFFER, 0);

	if (video_thread_.joinable())
	{
		video_thread_.join();
	}

	if (audio_thread_.joinable())
	{
		audio_thread_.join(); //next time !block here! 
	}

	CLEAR_MAP(que_map_);

	decoder_.StopDecode();
	return true;
}

bool YMediaPlayer::IsPause()
{
	return is_pause_;
}

bool YMediaPlayer::FillAudioBuff(ALuint& buf)
{
	AudioPackageInfo info= decoder_.PopAudioQue();
	if (info .size <= 0)
		return false;
	alBufferData(buf, AL_FORMAT_STEREO16, info.data, info.size, info.sample_rate);
	alSourceQueueBuffers(source_id_, 1, &buf);
	decoder_.FreeAudioPackageInfo(&info);
	que_map_[buf] = info.pts;
	return true;
}

int YMediaPlayer::AudioPlayThread()
{
	is_need_stop_ = false;
	//first time ,need to fill the Source
	for (int i = 0; i < NUMBUFFERS; i++)
	{
		if (!FillAudioBuff(audio_buf_[i]))
		{
			is_need_stop_ = true;
			break;
		}
	}

	is_prepare_ = true;

	while ( false == is_need_stop_)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		if (IsPause())
		{
			continue;
		}
		else
		{
			Play();
		}

		ALint processed = 0;
		alGetSourcei(source_id_, AL_BUFFERS_PROCESSED, &processed);
		while (processed--)
		{
			ALuint bufferID = 0;
			alSourceUnqueueBuffers(source_id_, 1, &bufferID);
			audio_clock_ = que_map_[bufferID];


			if (!FillAudioBuff(bufferID))
			{
				break;
			}
		}
	}
	return true;
}

int YMediaPlayer::VideoPlayThread()
{
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

	while (false == is_need_stop_)
	{
		VideoPackageInfo info = decoder_.PopVideoQue(video_clock_);
		if (!info.data)
			continue;
		video_clock_ = info.clock;

		synchronize_video();

		glTexImage2D(GL_TEXTURE_2D, 0,
			GL_RGB,
			info.width, info.height, 0,
			GL_BGR, GL_UNSIGNED_BYTE, info.data);


		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		//GLuint sampler_location;
		glUseProgram(program);
		//sampler_location = glGetUniformLocation(program, "colorMap");
		//glUniform1i(sampler_location, 0);

		glBindTexture(GL_TEXTURE_2D, TextureID);

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);


		glfwSwapBuffers(g_hwnd);
	}

	glfwMakeContextCurrent(NULL);
	return 1;
}

void YMediaPlayer::synchronize_video()
{

	while (false == is_need_stop_)
	{
		printf("%f,%f \n", video_clock_,audio_clock_);
		if (video_clock_ <= audio_clock_)
			break;
		int delayTime = (video_clock_- audio_clock_) * 1000;
		delayTime = delayTime > 1 ? 1 : delayTime;
	//	printf("dealy time:%d\n",delayTime);
		std::this_thread::sleep_for(std::chrono::milliseconds(delayTime));
	}
}
