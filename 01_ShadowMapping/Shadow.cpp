#include "Shadow.h"

Shadow::Shadow()
{
	SHADOW_HEIGHT = 1024;
	SHADOW_WIDTH = 1024;
	depthMapFBO = 0;
	depthMap = 0;
	gLightSpaceMatUniform = 0;
	gModelUniform = 0;
	vertexShaderObject = 0;
	fragmentShaderObject = 0;
	shadowShaderProgramObject = 0;

	createDepthBuffer();
	createTextureMap();
	attachTextureToFBO();
}

Shadow::~Shadow()
{
	if (depthMapFBO)
	{
		glDeleteFramebuffers(1, &depthMapFBO);
		depthMapFBO = 1;
	}
		
	if (depthMap)
	{
		glDeleteTextures(1, &depthMap);
		depthMap = 1;
	}


	glDetachShader(shadowShaderProgramObject, vertexShaderObject);
	glDetachShader(shadowShaderProgramObject, fragmentShaderObject);

	glDeleteShader(vertexShaderObject);
	vertexShaderObject = 0;

	glDeleteShader(fragmentShaderObject);
	fragmentShaderObject = 0;

	glDeleteProgram(shadowShaderProgramObject);
	shadowShaderProgramObject = 0;

}

GLuint Shadow::getDepthMap(void)
{
	return depthMap;
}

void Shadow::createDepthBuffer(void)
{
	glGenFramebuffers(1, &depthMapFBO);
	
}

void Shadow::createTextureMap(void)
{
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
}

void Shadow::attachTextureToFBO(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

GLuint Shadow::createShadowShaderProgram(void)
{
	// Light source shaders
	vertexShaderObject = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* vShadercode =
		"#version 330" \
		"\n" \
		"uniform mat4 lightSpaceMatrix;" \
		"uniform mat4 model;" \
		"in vec4 vPosition;" \
		"void main()" \
		"{" \
		"gl_Position = lightSpaceMatrix * model * vPosition;" \
		"}";

	glShaderSource(vertexShaderObject, 1, (const GLchar**)&vShadercode, NULL);

	// compile shader
	glCompileShader(vertexShaderObject);
	GLint iInfoLogLength = 0;
	GLint iShaderCompiledStatus = 0;
	char* szInfoLog = NULL;
	glGetShaderiv(vertexShaderObject, GL_COMPILE_STATUS, &iShaderCompiledStatus);
	if (iShaderCompiledStatus == GL_FALSE)
	{
		glGetShaderiv(vertexShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (char*)malloc(iInfoLogLength);
				                                                                                   
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(vertexShaderObject, iInfoLogLength, &written, szInfoLog);
				//fprintf(gpFile, "light Vertex Shader Compilation Log : %s\n", szInfoLog);
				free(szInfoLog);
				//uninitialize();
				exit(0);
			}
		}
	}

	fragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar* fShaderCode =
		"#version 330" \
		"\n" \
		"void main()" \
		"{" \
		"}";

	glShaderSource(fragmentShaderObject, 1, (const GLchar**)&fShaderCode, NULL);

	// compile shader
	glCompileShader(fragmentShaderObject);
	iInfoLogLength = 0;
	iShaderCompiledStatus = 0;
	szInfoLog = NULL;
	glGetShaderiv(fragmentShaderObject, GL_COMPILE_STATUS, &iShaderCompiledStatus);
	if (iShaderCompiledStatus == GL_FALSE)
	{
		glGetShaderiv(fragmentShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (char*)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(fragmentShaderObject, iInfoLogLength, &written, szInfoLog);
				//fprintf(gpFile, "Frag light Vertex Shader Compilation Log : %s\n", szInfoLog);
				free(szInfoLog);
				//uninitialize();
				exit(0);
			}
		}
	}

	shadowShaderProgramObject = glCreateProgram();

	// attach vertex shader to shader program
	glAttachShader(shadowShaderProgramObject, vertexShaderObject);

	// attach fragment shader to shader program
	glAttachShader(shadowShaderProgramObject, fragmentShaderObject);

	// pre-link binding of shader program object with vertex shader position attribute
	glBindAttribLocation(shadowShaderProgramObject, VDG_ATTRIBUTE_VERTEX, "vPosition");
	//glBindAttribLocation(shadowShaderProgramObject, VDG_ATTRIBUTE_VERTEX, "vTexCoords");

	// link shader
	glLinkProgram(shadowShaderProgramObject);
	GLint iShaderProgramLinkStatus = 0;
	glGetProgramiv(shadowShaderProgramObject, GL_LINK_STATUS, &iShaderProgramLinkStatus);
	if (iShaderProgramLinkStatus == GL_FALSE)
	{
		glGetProgramiv(shadowShaderProgramObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (char*)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetProgramInfoLog(shadowShaderProgramObject, iInfoLogLength, &written, szInfoLog);
				//fprintf(gpFile, "Shader Program Link Log : %s\n", szInfoLog);
				free(szInfoLog);
				//uninitialize();
				exit(0);
			}
		}
	}

	// get uniform locations
	gLightSpaceMatUniform = glGetUniformLocation(shadowShaderProgramObject, "lightSpaceMatrix");
	gModelUniform = glGetUniformLocation(shadowShaderProgramObject, "model");
	
	return shadowShaderProgramObject;
}

void Shadow::setLightSpaceMatUniform(vmath::mat4 matrix)
{
	glUniformMatrix4fv(gLightSpaceMatUniform, 1, GL_FALSE, matrix);
}

void Shadow::setModelMatUniform(vmath::mat4 matrix)
{
	glUniformMatrix4fv(gModelUniform, 1, GL_FALSE, matrix);
}

void Shadow::bindDepthFramebuffer(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
}

void Shadow::unbindDepthFramebuffer(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
