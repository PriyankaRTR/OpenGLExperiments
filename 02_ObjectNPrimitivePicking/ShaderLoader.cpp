#include "ShaderLoader.h"

ShaderLoader::ShaderLoader()
{
	shaderObjectID = 0;
	//if (fopen_s(&gpFile, "Log.txt", "w") != 0)
	//	exit(0);
	//else
	//	fprintf(gpFile, "Log File Is Successfully Opened.\n");
}

GLuint ShaderLoader::loadShader(const GLchar* shaderSource, GLenum type, FILE* gpFile)
{
	GLuint shaderId = glCreateShader(type);
	glShaderSource(shaderId, 1, (const GLchar**)&shaderSource, NULL);
	glCompileShader(shaderId);
	GLint iInfoLogLength = 0;
	GLint iShaderCompiledStatus = 0;
	char* szInfoLog = NULL;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &iShaderCompiledStatus);
	if (iShaderCompiledStatus == GL_FALSE)
	{
		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (char*)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(shaderId, iInfoLogLength, &written, szInfoLog);
				fprintf(gpFile, "%d, shader compilation log : %s\n", type, szInfoLog);
				free(szInfoLog);
				cleanUp();
				exit(0);
			}
		}

	}
	return shaderId;
}


void ShaderLoader::cleanUp(void)
{/*
	if (gpFile)
	{
		fprintf(gpFile, "Log File Is Successfully Closed.\n");
		fclose(gpFile);
		gpFile = NULL;
	}*/

	//if (shaderObjectID)
	//{
	//	// detach vertex shader from shader program object
	//	glDetachShader(shaderObjectID, gVertexShaderObject);
	//	// detach fragment  shader from shader program object
	//	glDetachShader(shaderObjectID, gFragmentShaderObject);

	//	// delete vertex shader object
	//	glDeleteShader(gVertexShaderObject);
	//	gVertexShaderObject = 0;
	//	// delete fragment shader object
	//	glDeleteShader(gFragmentShaderObject);
	//	gFragmentShaderObject = 0;

	//	// delete shader program object
	//	glDeleteProgram(gShaderProgramObject);
	//	gShaderProgramObject = 0;
	//}
	if (shaderObjectID)
	{
		glDeleteProgram(shaderObjectID);
		shaderObjectID = 0;
	}
}
