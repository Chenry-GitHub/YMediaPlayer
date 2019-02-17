#include "OpenGLVideo.h"

#define MAX_SHADER_LENGTH 8192
#define MAX_TARGET_NUM  10

GLfloat vertexArray[12] = { -0.9f, -0.9f, 0.0f,
0.9f, -0.9f, 0.0f,
0.9f,  0.9f, 0.0f,
-0.9f,  0.9f, 0.0f };
GLfloat texCoord[8] = { 0.0f, 0.0f,
1.0f, 0.0f,
1.0f, 1.0f,
0.0f, 1.0f };


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
		//assert(shaderLength < MAX_SHADER_LENGTH);   // make me bigger!
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



OpenGLVideo::OpenGLVideo()
{
}


OpenGLVideo::~OpenGLVideo()
{
}


void OpenGLVideo::PlayThread()
{
	//glfwMakeContextCurrent(win_);
	
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

	while (false == is_stop_ )
	{
		
		seek_func_();
		//decoder_.JudgeBlockVideoSeek();

		char *data;
		int width;
		int height;
		double pts;
		if (!data_func_(&data, &width, &height, &pts))
		{
			continue;
		}

		clock_= pts;

		sync_func_();//may block here

		glTexImage2D(GL_TEXTURE_2D, 0,
			GL_RGB,
			width, height, 0,
			GL_BGR, GL_UNSIGNED_BYTE, data);


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


//		glfwSwapBuffers(g_hwnd);
	}

//	glfwMakeContextCurrent(NULL);
}
