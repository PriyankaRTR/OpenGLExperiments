#include <windows.h>
#include <windowsx.h>

#include <stdio.h> // for FILE I/O

#include <gl\glew.h> // for GLSL extensions IMPORTANT : This Line Should Be Before #include<gl\gl.h> And #include<gl\glu.h>

#include <gl/GL.h>

#include "vmath.h"

#include "OpenGLPP_ObjectPicking.h"
#include "ShaderLoader.h"
#include <picking_texture.h>

#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"opengl32.lib")

#define WIN_WIDTH 800
#define WIN_HEIGHT 600

using namespace vmath;

enum
{
	VDG_ATTRIBUTE_VERTEX = 0,
	VDG_ATTRIBUTE_COLOR,
	VDG_ATTRIBUTE_NORMAL,
	VDG_ATTRIBUTE_TEXTURE0,
};


//Prototype Of WndProc() declared Globally
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//Global variable declarations
FILE *gpFile = NULL;

HWND ghwnd = NULL;
HDC ghdc = NULL;
HGLRC ghrc = NULL;

DWORD dwStyle;
WINDOWPLACEMENT wpPrev = { sizeof(WINDOWPLACEMENT) };

bool gbActiveWindow = false;
bool gbEscapeKeyIsPressed = false;
bool gbFullscreen = false;

GLuint gVertexShaderObject;
GLuint gFragmentShaderObject;
GLuint gShaderProgramObject;

GLuint gVao_pyramid;
GLuint gVbo_pyramid_position;
GLuint gVbo_pyramid_texture;
GLuint gEbo_pyramid;

GLuint gVao_cube;
GLuint gVbo_cube_position;
GLuint gVbo_cube_texture;

GLuint gMVPUniform;

// for perspective
mat4 gPerspectiveProjectionMatrix;

GLuint gTexture_sampler_uniform;
GLuint gTexture_Brickwall;
GLuint gTexture_Stone;

GLfloat gAnglePyramid = 0.0f;
GLfloat gAngleCube = 0.0f;


int COLOR_INDEX = 0;


void createShaderObjects(void);
void createFrameBufferObject(void);
void pickingLoop(void);
void simpleColorLoop(void);
void renderDiffColorLoop(void);
void cleanUpObjectPickingData(void);

GLuint gVertexPickingShdrObj;
GLuint gFragmentPickingShdrObj;
GLuint gPickingShaderProgObj;

GLuint gVertexBasicShdrObj;
GLuint gFragmentBasicShdrObj;
GLuint gBasicShaderProgObj;

GLuint gVertexSimpleColorShdrObj;
GLuint gFragmentSimpleColorBasicShdrObj;
GLuint gSimpleColorShaderProgObj;

ShaderLoader* shaderLoader;


GLuint gMVPUniformPicking;
GLuint gObjIndexUniformPicking;
GLuint gDrawIndexUniformPicking;
GLuint gMVPUniformSimpleClr;
GLuint gMVPUniformBasic;
GLuint gTexture_samplerBasic;
GLuint gColorModBasic;

mat4 gMVPPickingMatrix;
mat4 gMVPBasicMatrix;
mat4 gMVPSimpleColorMatrix;

bool switchShaderFlag;

PickingTexture* pickingEffect;

enum shaderType
{
	PICKING,
	BASIC,
	SIMPLE_COLOR
};

GLuint gWin_Width;
GLuint gWin_Height;

struct {
	bool IsPressed = false;
	int x;
	int y;
} leftMouseButton;


//main()
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
	//function prototype
	void initialize(void);
	void uninitialize(void);
	void display(void);
	void update(void);

	//variable declaration
	WNDCLASSEX wndclass;
	HWND hwnd;
	MSG msg;
	TCHAR szClassName[] = TEXT("OpenGLPP");
	bool bDone = false;

	//code
	// create log file
	if (fopen_s(&gpFile, "Log.txt", "w") != 0)
	{
		MessageBox(NULL, TEXT("Log File Can Not Be Created\nExitting ..."), TEXT("Error"), MB_OK | MB_TOPMOST | MB_ICONSTOP);
		exit(0);
	}
	else
	{
		fprintf(gpFile, "Log File Is Successfully Opened.\n");
	}

	//initializing members of struct WNDCLASSEX
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.lpfnWndProc = WndProc;
	wndclass.lpszClassName = szClassName;
	wndclass.lpszMenuName = NULL;
	//Registering Class
	RegisterClassEx(&wndclass);

	//Create Window
	//Parallel to glutInitWindowSize(), glutInitWindowPosition() and glutCreateWindow() all three together
	hwnd = CreateWindow(szClassName,
		TEXT("OpenGL Programmable Pipeline Window"),
		WS_OVERLAPPEDWINDOW,
		100,
		100,
		WIN_WIDTH,
		WIN_HEIGHT,
		NULL,
		NULL,
		hInstance,
		NULL);

	ghwnd = hwnd;

	ShowWindow(hwnd, iCmdShow);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);

	//initialize
	initialize();

	//Message Loop
	while (bDone == false) //Parallel to glutMainLoop();
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				bDone = true;
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			// rendring function
			display();
			update();

			if (gbActiveWindow == true)
			{
				if (gbEscapeKeyIsPressed == true) //Continuation to glutLeaveMainLoop();
					bDone = true;
			}
		}
	}

	uninitialize();

	return((int)msg.wParam);
}

//WndProc()
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	//function prototype
	void resize(int, int);
	void ToggleFullscreen(void);
	void uninitialize(void);

	//variable declarations
	static WORD xMouse = NULL;
	static WORD yMouse = NULL;

	//code
	switch (iMsg)
	{
	case WM_ACTIVATE:
		if (HIWORD(wParam) == 0) //if 0, the window is active
			gbActiveWindow = true;
		else //if non-zero, the window is not active
			gbActiveWindow = false;
		break;
	case WM_ERASEBKGND:
		return(0);
	case WM_SIZE: //Parallel to glutReshapeFunc();
		resize(LOWORD(lParam), HIWORD(lParam)); //Parallel to glutReshapeFunc(resize);
		break;
	case WM_KEYDOWN: //Parallel to glutKeyboardFunc();
		switch (wParam)
		{
		case VK_ESCAPE: //case 27
			if (gbEscapeKeyIsPressed == false)
				gbEscapeKeyIsPressed = true; //Parallel to glutLeaveMainLoop();
			break;
		case 0x46: //for 'f' or 'F'
			if (gbFullscreen == false)
			{
				ToggleFullscreen();
				gbFullscreen = true;
			}
			else
			{
				ToggleFullscreen();
				gbFullscreen = false;
			}
			break;

		case 'A':
			COLOR_INDEX = 1;
			break;

		case 'B':
			COLOR_INDEX = 2;
			break;

		case 'C':
			COLOR_INDEX = 3;
			break;

		case 'D':
			COLOR_INDEX = 4;
			break;

		case 'R':
			COLOR_INDEX = 0;
			break;

		case 'S':
		{
			if (switchShaderFlag == TRUE)
				switchShaderFlag = FALSE;
			else
				switchShaderFlag = TRUE;
		}
			
		default:
			break;
		}
		break;
	case WM_LBUTTONDOWN:  //Parallel to glutMouseFunc();
		leftMouseButton.IsPressed = true;
		leftMouseButton.x = GET_X_LPARAM(lParam);
		leftMouseButton.y = GET_Y_LPARAM(lParam);		
		break;
	case WM_LBUTTONUP:
		//leftMouseButton.IsPressed = false;
		break;
	case WM_CLOSE: //Parallel to glutCloseFunc();
		uninitialize(); //Parallel to glutCloseFunc(uninitialize);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		break;
	}
	return(DefWindowProc(hwnd, iMsg, wParam, lParam));
}

void createShaderObjects(void)
{
	//void bindAttributes(GLuint attribute, const char* varName);
	
	void createPickingShader(void); // to render into framebuffer as a texture
	void createBasicShader(void); // to render simple model with texture
	void createSimpleColorShader(void); // simply color selected part of the rendererd image
	void getAllUniformLocationsPicking(shaderType type);

	createPickingShader();
	createBasicShader();
	createSimpleColorShader();
	
}

void createFrameBufferObject(void)
{
	pickingEffect = new PickingTexture();
	pickingEffect->Init(gWin_Width, gWin_Height);

}

void cleanUpObjectPickingData(void)
{
	if (gPickingShaderProgObj)
	{
		glDetachShader(gPickingShaderProgObj, gVertexPickingShdrObj);
		glDetachShader(gPickingShaderProgObj, gFragmentPickingShdrObj);

		glDeleteShader(gVertexPickingShdrObj);
		gVertexPickingShdrObj = 0;
		glDeleteShader(gFragmentPickingShdrObj);
		gFragmentPickingShdrObj = 0;
		glDeleteProgram(gPickingShaderProgObj);
		gPickingShaderProgObj = 0;
		// unlink shader program
		glUseProgram(0);
	}
	if (gBasicShaderProgObj)
	{
		glDetachShader(gBasicShaderProgObj, gVertexBasicShdrObj);
		glDetachShader(gBasicShaderProgObj, gFragmentBasicShdrObj);

		glDeleteShader(gVertexBasicShdrObj);
		gVertexBasicShdrObj = 0;
		glDeleteShader(gFragmentBasicShdrObj);
		gFragmentBasicShdrObj = 0;
		glDeleteProgram(gBasicShaderProgObj);
		gBasicShaderProgObj = 0;
		// unlink shader program
		glUseProgram(0);
	}

	if (gSimpleColorShaderProgObj)
	{
		glDetachShader(gSimpleColorShaderProgObj, gVertexSimpleColorShdrObj);
		glDetachShader(gSimpleColorShaderProgObj, gFragmentSimpleColorBasicShdrObj);

		glDeleteShader(gVertexSimpleColorShdrObj);
		gVertexSimpleColorShdrObj = 0;
		glDeleteShader(gFragmentSimpleColorBasicShdrObj);
		gFragmentSimpleColorBasicShdrObj = 0;
		glDeleteProgram(gSimpleColorShaderProgObj);
		gSimpleColorShaderProgObj = 0;
		// unlink shader program
		glUseProgram(0);
	}

	if (pickingEffect)
	{
		free(pickingEffect);
		pickingEffect = NULL;
	}

}

void createPickingShader(void) // to render into framebuffer as a texture
{
	void bindAttributes(GLuint shaderObjID, GLuint attribute, const char* varName);
	void getAllUniformLocations(shaderType type);

	const GLchar* picking_vs =
		"#version 330" \
		"\n" \
		"layout (location = 0) in vec3 Position;" \
		"uniform mat4 gWVP;" \
		"void main()" \
		"{" \
		"gl_Position = gWVP * vec4(Position, 1.0);" \
		"}";

	const GLchar* picking_fs =
		"#version 330" \
		"\n" \
		"uniform uint gObjectIndex;" \
		"uniform uint gDrawIndex;" \
		"out uvec3 FragColor;" \
		"void main()" \
		"{" \
		"FragColor = uvec3(gObjectIndex, gDrawIndex, gl_PrimitiveID);" \
		"}";

	gVertexPickingShdrObj = shaderLoader->loadShader(picking_vs, GL_VERTEX_SHADER, gpFile);
	gFragmentPickingShdrObj = shaderLoader->loadShader(picking_fs, GL_FRAGMENT_SHADER, gpFile);
	gPickingShaderProgObj = glCreateProgram();

	glAttachShader(gPickingShaderProgObj, gVertexPickingShdrObj);
	glAttachShader(gPickingShaderProgObj, gFragmentPickingShdrObj);

	bindAttributes(gPickingShaderProgObj, 0, "Position");
	//bindAttributes(gPickingShaderProgObj, 1, "textureCoordinates");
	//bindAttributes(gPickingShaderProgObj, 2, "normal");

	glLinkProgram(gPickingShaderProgObj);
	glValidateProgram(gPickingShaderProgObj);

	getAllUniformLocations(PICKING);

}

void createBasicShader(void)
{
	void uninitialize(void);
	void bindAttributes(GLuint shaderObjID, GLuint attribute, const char* varName);
	void getAllUniformLocations(shaderType type);

	const GLchar* basic_vs =
		"#version 330" \
		"\n" \
		"in vec4 vPosition;" \
		"in vec2 vTexture0_Coord;" \
		"out vec2 out_texture0_coord;" \
		"uniform mat4 u_mvp_matrix;" \
		"void main(void)" \
		"{" \
		"gl_Position = u_mvp_matrix * vPosition;" \
		"out_texture0_coord = vTexture0_Coord;" \
		"}";

	const GLchar* basic_fs =
		"#version 330" \
		"\n" \
		"out vec4 FragColor;" \
		"in vec2 out_texture0_coord;" \
		"uniform sampler2D u_texture0_sampler;" \
		"uniform vec4 gColorMod;" \
		"void main(void)" \
		"{" \
		"vec4 tempColor = texture(u_texture0_sampler, out_texture0_coord);" \
		"FragColor = tempColor*gColorMod;" \
		"}";

	gVertexBasicShdrObj = shaderLoader->loadShader(basic_vs, GL_VERTEX_SHADER, gpFile);
	gFragmentBasicShdrObj = shaderLoader->loadShader(basic_fs, GL_FRAGMENT_SHADER, gpFile);
	gBasicShaderProgObj = glCreateProgram();

	glAttachShader(gBasicShaderProgObj, gVertexBasicShdrObj);
	glAttachShader(gBasicShaderProgObj, gFragmentBasicShdrObj);

	bindAttributes(gBasicShaderProgObj, VDG_ATTRIBUTE_VERTEX, "vPosition");
	bindAttributes(gBasicShaderProgObj, VDG_ATTRIBUTE_TEXTURE0, "vTexture0_Coord");
	//bindAttributes(gBasicShaderProgObj, 2, "normal");

	glLinkProgram(gBasicShaderProgObj);
	GLint iInfoLogLength = 0;
	GLint iShaderCompiledStatus = 0;
	GLint iShaderProgramLinkStatus = 0;
	char* szInfoLog = NULL;
	glGetProgramiv(gBasicShaderProgObj, GL_LINK_STATUS, &iShaderProgramLinkStatus);
	if (iShaderProgramLinkStatus == GL_FALSE)
	{
		glGetProgramiv(gBasicShaderProgObj, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (char*)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetProgramInfoLog(gBasicShaderProgObj, iInfoLogLength, &written, szInfoLog);
				fprintf(gpFile, "basic shader Program Link Log : %s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}
	glValidateProgram(gBasicShaderProgObj);

	getAllUniformLocations(BASIC);
}

void createSimpleColorShader(void)
{
	void bindAttributes(GLuint shaderObjID, GLuint attribute, const char* varName);
	void getAllUniformLocations(shaderType type);

	const GLchar* simpleColor_vs =
		"#version 330" \
		"\n" \
		"layout (location = 0) in vec3 Position;" \
		"uniform mat4 u_mvp_matrix;" \
		"void main()" \
		"{" \
		"gl_Position = u_mvp_matrix * vec4(Position, 1.0);" \
		"}";

	const GLchar* simpleColor_fs =
		"#version 330" \
		"\n" \
		"layout(location = 0) out vec4 FragColor;" \
		"void main()" \
		"{" \
		"FragColor = vec4(1.0, 0.0, 0.0, 1.0);" \
		"}";

	gVertexSimpleColorShdrObj = shaderLoader->loadShader(simpleColor_vs, GL_VERTEX_SHADER, gpFile);
	gFragmentSimpleColorBasicShdrObj = shaderLoader->loadShader(simpleColor_fs, GL_FRAGMENT_SHADER, gpFile);
	gSimpleColorShaderProgObj = glCreateProgram();

	glAttachShader(gSimpleColorShaderProgObj, gVertexSimpleColorShdrObj);
	glAttachShader(gSimpleColorShaderProgObj, gFragmentSimpleColorBasicShdrObj);

	bindAttributes(gSimpleColorShaderProgObj, 0, "Position");
	//bindAttributes(gSimpleColorShaderProgObj, 1, "textureCoordinates");
	//bindAttributes(gSimpleColorShaderProgObj, 2, "normal");

	glLinkProgram(gSimpleColorShaderProgObj);
	glValidateProgram(gSimpleColorShaderProgObj);

	getAllUniformLocations(SIMPLE_COLOR);
}

void bindAttributes(GLuint shaderObjID, GLuint attribute, const char* varName)
{
	glBindAttribLocation(shaderObjID, attribute, varName);
}



void getAllUniformLocations(shaderType type)
{
	switch (type)
	{
		case PICKING:
			gMVPUniformPicking = glGetUniformLocation(gPickingShaderProgObj, "gWVP");
			gObjIndexUniformPicking = glGetUniformLocation(gPickingShaderProgObj, "gObjectIndex");
			gDrawIndexUniformPicking = glGetUniformLocation(gPickingShaderProgObj, "gDrawIndex");					
			break;
		case BASIC:
			gMVPUniformBasic = glGetUniformLocation(gBasicShaderProgObj, "u_mvp_matrix");
			gTexture_samplerBasic = glGetUniformLocation(gBasicShaderProgObj, "u_texture0_sampler");
			gColorModBasic = glGetUniformLocation(gBasicShaderProgObj, "gColorMod");
			break;
		case SIMPLE_COLOR:
			gMVPUniformSimpleClr = glGetUniformLocation(gSimpleColorShaderProgObj, "u_mvp_matrix");
			break;
		default:
			break;
	}
}


void updateAllUniformLocations(shaderType type)
{

	switch (type)
	{
		case PICKING:
			glUniformMatrix4fv(gMVPUniformPicking, 1, GL_FALSE, gMVPPickingMatrix);
			break;
		case BASIC:
			glUniformMatrix4fv(gMVPUniformBasic, 1, GL_FALSE, gMVPBasicMatrix);
			break;
		case SIMPLE_COLOR:
			glUniformMatrix4fv(gMVPUniformSimpleClr, 1, GL_FALSE, gMVPSimpleColorMatrix);
			break;
		default:
			break;
	}
}

void pickingDrawStartCB(GLuint DrawIndex)
{
	glUniform1ui(gDrawIndexUniformPicking, DrawIndex);
}


void pickingSetObjectIndex(GLuint ObjectIndex)
{
	glUniform1ui(gObjIndexUniformPicking, ObjectIndex);
}








void ToggleFullscreen(void)
{
	//variable declarations
	MONITORINFO mi;

	//code
	if (gbFullscreen == false)
	{
		dwStyle = GetWindowLong(ghwnd, GWL_STYLE);
		if (dwStyle & WS_OVERLAPPEDWINDOW)
		{
			mi = { sizeof(MONITORINFO) };
			if (GetWindowPlacement(ghwnd, &wpPrev) && GetMonitorInfo(MonitorFromWindow(ghwnd, MONITORINFOF_PRIMARY), &mi))
			{
				SetWindowLong(ghwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(ghwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOZORDER | SWP_FRAMECHANGED);
			}
		}
		ShowCursor(FALSE);
	}

	else
	{
		//code
		SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(ghwnd, &wpPrev);
		SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);

		ShowCursor(TRUE);
	}
}

//FUNCTION DEFINITIONS
void initialize(void)
{
	//function prototypes
	void uninitialize(void);
	void resize(int, int);
	int LoadGLTextures(GLuint *texture, TCHAR imageResourceId[]);

	//variable declarations
	PIXELFORMATDESCRIPTOR pfd;
	int iPixelFormatIndex;

	//code
	ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));

	//Initialization of structure 'PIXELFORMATDESCRIPTOR'
	//Parallel to glutInitDisplayMode()
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cRedBits = 8;
	pfd.cGreenBits = 8;
	pfd.cBlueBits = 8;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 32;

	ghdc = GetDC(ghwnd);

	//choose a pixel format which best matches with that of 'pfd'
	iPixelFormatIndex = ChoosePixelFormat(ghdc, &pfd);
	if (iPixelFormatIndex == 0)
	{
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	//set the pixel format chosen above
	if (SetPixelFormat(ghdc, iPixelFormatIndex, &pfd) == false)
	{
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	//create OpenGL rendering context
	ghrc = wglCreateContext(ghdc);
	if (ghrc == NULL)
	{
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	//make the rendering context created above as current n the current hdc
	if (wglMakeCurrent(ghdc, ghrc) == false)
	{
		wglDeleteContext(ghrc);
		ghrc = NULL;
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	// GLEW Initialization Code For GLSL ( IMPORTANT : It Must Be Here. Means After Creating OpenGL Context But Before Using Any OpenGL Function )
	GLenum glew_error = glewInit();
	if (glew_error != GLEW_OK)
	{
		wglDeleteContext(ghrc);
		ghrc = NULL;
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	shaderLoader = new ShaderLoader();

	// *** VERTEX SHADER ***
	// create shader
	gVertexShaderObject = glCreateShader(GL_VERTEX_SHADER);

	// provide source code to shader
	const GLchar *vertexShaderSourceCode =
		"#version 130" \
		"\n" \
		"in vec4 vPosition;" \
		"in vec2 vTexture0_Coord;" \
		"out vec2 out_texture0_coord;" \
		"uniform mat4 u_mvp_matrix;" \
		"void main(void)" \
		"{" \
		"gl_Position = u_mvp_matrix * vPosition;" \
		"out_texture0_coord = vTexture0_Coord;" \
		"}";
	glShaderSource(gVertexShaderObject, 1, (const GLchar **)&vertexShaderSourceCode, NULL);

	// compile shader
	glCompileShader(gVertexShaderObject);
	GLint iInfoLogLength = 0;
	GLint iShaderCompiledStatus = 0;
	char *szInfoLog = NULL;
	glGetShaderiv(gVertexShaderObject, GL_COMPILE_STATUS, &iShaderCompiledStatus);
	if (iShaderCompiledStatus == GL_FALSE)
	{
		glGetShaderiv(gVertexShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (char *)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(gVertexShaderObject, iInfoLogLength, &written, szInfoLog);
				fprintf(gpFile, "Vertex Shader Compilation Log : %s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}

	// *** FRAGMENT SHADER ***
	// create shader
	gFragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);

	// provide source code to shader
	const GLchar *fragmentShaderSourceCode =
		"#version 130" \
		"\n" \
		"out vec4 FragColor;" \
		"in vec2 out_texture0_coord;" \
		"uniform sampler2D u_texture0_sampler;" \
		"void main(void)" \
		"{" \
		"FragColor = texture(u_texture0_sampler, out_texture0_coord);" \
		"}";
	glShaderSource(gFragmentShaderObject, 1, (const GLchar **)&fragmentShaderSourceCode, NULL);

	// compile shader
	glCompileShader(gFragmentShaderObject);
	glGetShaderiv(gFragmentShaderObject, GL_COMPILE_STATUS, &iShaderCompiledStatus);
	if (iShaderCompiledStatus == GL_FALSE)
	{
		glGetShaderiv(gFragmentShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (char *)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(gFragmentShaderObject, iInfoLogLength, &written, szInfoLog);
				fprintf(gpFile, "Fragment Shader Compilation Log : %s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}

	// *** SHADER PROGRAM ***
	// create
	gShaderProgramObject = glCreateProgram();

	// attach vertex shader to shader program
	glAttachShader(gShaderProgramObject, gVertexShaderObject);

	// attach fragment shader to shader program
	glAttachShader(gShaderProgramObject, gFragmentShaderObject);

	// pre-link binding of shader program object with vertex shader position attribute
	glBindAttribLocation(gShaderProgramObject, VDG_ATTRIBUTE_VERTEX, "vPosition");

	// pre-link binding of shader program object with vertex shader position attribute
	glBindAttribLocation(gShaderProgramObject, VDG_ATTRIBUTE_TEXTURE0, "vTexture0_Coord");

	// link shader
	glLinkProgram(gShaderProgramObject);
	GLint iShaderProgramLinkStatus = 0;
	glGetProgramiv(gShaderProgramObject, GL_LINK_STATUS, &iShaderProgramLinkStatus);
	if (iShaderProgramLinkStatus == GL_FALSE)
	{
		glGetProgramiv(gShaderProgramObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength>0)
		{
			szInfoLog = (char *)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetProgramInfoLog(gShaderProgramObject, iInfoLogLength, &written, szInfoLog);
				fprintf(gpFile, "Shader Program Link Log : %s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}

	// get MVP uniform location
	gMVPUniform = glGetUniformLocation(gShaderProgramObject, "u_mvp_matrix");
	gTexture_sampler_uniform = glGetUniformLocation(gShaderProgramObject, "u_texture0_sampler");
	
	//************************* PYRAMID Vao*****************************
	// SHADER------------
	createShaderObjects();
	createFrameBufferObject();

	// ----------- 
	// *** vertices, colors, shader attribs, vbo, vao initializations ***
	const GLfloat pyramidVertices[] =
	{
		0, 1, 0,  // front-top
		-1, -1, 1,  // front-left
		1, -1, 1,  // front-right

		0, 1, 0    // right-top
		//1, -1, 1,   // right-left
		//1, -1, -1,  // right-right

		//0, 1, 0,    // back-top
		//1, -1, -1,  // back-left
		//-1, -1, -1, // back-right

		//0, 1, 0,    // left-top
		//-1, -1, -1, // left-left
		//-1, -1, 1   // left-right
	};
	const GLfloat triangleVertices[] =
	{
		-1.0, 1.0, 0.0,  // front-top P
		0.5, 1.0, // front-top T
		-1.0, -1.0, 0.0,  // front-left P
		0.0, 0.0, // front-left T
		1.0, -1.0, 0.0,  // front-right P
		1.0, 0.0, // front-right T
		1.0, 1.0, 0.0,  // front-right P
		1.0, 0.0, // front-right T
	};

	const GLushort triangleIndices[] =
	{
		0,1,2,0,2,3
	};

	const GLfloat pyramidTexcoords[] =
	{
		0.5, 1.0, // front-top
		0.0, 0.0, // front-left
		1.0, 0.0, // front-right

		0.5, 1.0, // right-top
		//1.0, 0.0, // right-left
		//0.0, 0.0, // right-right

		//0.5, 1.0, // back-top
		//1.0, 0.0, // back-left
		//0.0, 0.0, // back-right

		//0.5, 1.0, // left-top
		//0.0, 0.0, // left-left
		//1.0, 0.0, // left-right
	};
	

	glGenVertexArrays(1, &gVao_pyramid);
	glBindVertexArray(gVao_pyramid);
	UINT temp = sizeof(triangleIndices);
	glGenBuffers(1, &gEbo_pyramid);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gEbo_pyramid);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(triangleIndices), triangleIndices, GL_STATIC_DRAW);

	GLuint tempSize = sizeof(triangleVertices);
	glGenBuffers(1, &gVbo_pyramid_position);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_pyramid_position);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);
	unsigned int NumFloats = 0;

	glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);
	glVertexAttribPointer(VDG_ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, (5 * sizeof(GLfloat)), (const void*)(NumFloats * sizeof(float)));
	NumFloats += 3;

	glEnableVertexAttribArray(VDG_ATTRIBUTE_TEXTURE0);
	glVertexAttribPointer(VDG_ATTRIBUTE_TEXTURE0, 2, GL_FLOAT, GL_FALSE, (5 * sizeof(GLfloat)), (const void*)(NumFloats * sizeof(float)));
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	//********************* CUBE ************************
	// separated two arrays of cube according to above mixed array
	GLfloat cubeVertices[] =
	{
		// top surface
		1.0f, 1.0f, -1.0f,  // top-right of top
		-1.0f, 1.0f, -1.0f, // top-left of top
		-1.0f, 1.0f, 1.0f, // bottom-left of top
		1.0f, 1.0f, 1.0f,  // bottom-right of top

		// bottom surface
		1.0f, -1.0f, 1.0f,  // top-right of bottom
		-1.0f, -1.0f, 1.0f, // top-left of bottom
		-1.0f, -1.0f, -1.0f, // bottom-left of bottom
		1.0f, -1.0f, -1.0f,  // bottom-right of bottom

		// front surface
		1.0f, 1.0f, 1.0f,  // top-right of front
		-1.0f, 1.0f, 1.0f, // top-left of front
		-1.0f, -1.0f, 1.0f, // bottom-left of front
		1.0f, -1.0f, 1.0f,  // bottom-right of front

		// back surface
		1.0f, -1.0f, -1.0f,  // top-right of back
		-1.0f, -1.0f, -1.0f, // top-left of back
		-1.0f, 1.0f, -1.0f, // bottom-left of back
		1.0f, 1.0f, -1.0f,  // bottom-right of back

		// left surface
		-1.0f, 1.0f, 1.0f, // top-right of left
		-1.0f, 1.0f, -1.0f, // top-left of left
		-1.0f, -1.0f, -1.0f, // bottom-left of left
		-1.0f, -1.0f, 1.0f, // bottom-right of left

		// right surface
		1.0f, 1.0f, -1.0f,  // top-right of right
		1.0f, 1.0f, 1.0f,  // top-left of right
		1.0f, -1.0f, 1.0f,  // bottom-left of right
		1.0f, -1.0f, -1.0f,  // bottom-right of right
	};

	// If above -1.0f Or +1.0f Values Make Cube Much Larger Than Pyramid,
	// then follow the code in following loop which will convertt all 1s And -1s to -0.75 or +0.75
	for (int i = 0; i<72; i++)
	{
		if (cubeVertices[i]<0.0f)
			cubeVertices[i] = cubeVertices[i] + 0.25f;
		else if (cubeVertices[i]>0.0f)
			cubeVertices[i] = cubeVertices[i] - 0.25f;
		else
			cubeVertices[i] = cubeVertices[i]; // no change
	}

	const GLfloat cubeTexcoords[] =
	{
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
	};

	// CUBE CODE
	glGenVertexArrays(1, &gVao_cube);
	glBindVertexArray(gVao_cube);

	// vbo for position
	glGenBuffers(1, &gVbo_cube_position);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_cube_position);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(VDG_ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL); // 3 is for x,y,z in cubeVertices array

	glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// vbo for texture
	glGenBuffers(1, &gVbo_cube_texture);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_cube_texture);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeTexcoords), cubeTexcoords, GL_STATIC_DRAW);

	glVertexAttribPointer(VDG_ATTRIBUTE_TEXTURE0, 2, GL_FLOAT, GL_FALSE, 0, NULL); // 3 is for r,g,b in cubeColors array

	glEnableVertexAttribArray(VDG_ATTRIBUTE_TEXTURE0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
	// ==================

	//createShaderObjects();


	glShadeModel(GL_SMOOTH);
	// set-up depth buffer
	glClearDepth(1.0f);
	// enable depth testing
	glEnable(GL_DEPTH_TEST);
	// depth test to do
	glDepthFunc(GL_LEQUAL);
	// set really nice percpective calculations ?
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	// We will always cull back faces for better performance
	//glEnable(GL_CULL_FACE);

	LoadGLTextures(&gTexture_Brickwall, MAKEINTRESOURCE(IDBITMAP_BRICKWALL));
	LoadGLTextures(&gTexture_Stone, MAKEINTRESOURCE(IDBITMAP_STONE));

	glEnable(GL_TEXTURE_2D);
	// set background color to which it will display even if it will empty. THIS LINE CAN BE IN drawRect().
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // black

										  // set orthographicMatrix to identitu matrix
	gPerspectiveProjectionMatrix = mat4::identity();

	// resize
	resize(WIN_WIDTH, WIN_HEIGHT);
}

int LoadGLTextures(GLuint *texture, TCHAR imageResourceId[])
{
	// variable declarations
	HBITMAP hBitmap;
	BITMAP bmp;
	int iStatus = FALSE;
	// code
	glGenTextures(1, texture);// 1 image
	hBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL), imageResourceId, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	if (hBitmap)// if bitmap exists ( means hBitmap is not null )
	{
		iStatus = TRUE;
		GetObject(hBitmap, sizeof(bmp), &bmp);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);// set 1 rather than default 4, for better performance
		glBindTexture(GL_TEXTURE_2D, *texture);// bind texture
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGB,
			bmp.bmWidth,
			bmp.bmHeight,
			0,
			GL_BGR,
			GL_UNSIGNED_BYTE,
			bmp.bmBits);

		// Create mipmaps for this texture for better image quality
		glGenerateMipmap(GL_TEXTURE_2D);

		DeleteObject(hBitmap);// delete unwanted bitmap handle
	}
	return(iStatus);
}

void display(void)
{
	void renderScene(void);
	
	renderScene();

	SwapBuffers(ghdc);
	
}

void renderScene(void)
{
	void pickingPhase(void);
	void renderPhase(void);
	void renderCube(void);

	//render object 
	if (leftMouseButton.IsPressed)
	{
		pickingPhase();
	}
	renderPhase();

}


void renderPhase(void)
{
	void renderElement(void* baseIndex, GLint baseVertex);
	//code
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// set modelview & modelviewprojection matrices to identity
	mat4 modelViewMatrix = mat4::identity();
	mat4 modelViewProjectionMatrix = mat4::identity();
	mat4 rotationMatrix = mat4::identity();

	int clicked_Obj_Id = -1;
	if (leftMouseButton.IsPressed)
	{
		PickingTexture::PixelInfo pixel = pickingEffect->ReadPixel(leftMouseButton.x, gWin_Height - leftMouseButton.y - 1); // since origin at top left corner
		if (pixel.ObjectID != 0)
		{
			clicked_Obj_Id = pixel.ObjectID - 1;
			glUseProgram(gSimpleColorShaderProgObj);
			// translate for perspective
			modelViewMatrix = translate(0.0f, 0.0f, -6.0f);

			// rotate pyramid
			rotationMatrix = rotate(gAnglePyramid, 0.0f, 1.0f, 0.0f);
			modelViewMatrix = modelViewMatrix * rotationMatrix;

			// multiply the modelview and orthographic matrix to get modelviewprojection matrix
			modelViewProjectionMatrix = gPerspectiveProjectionMatrix * modelViewMatrix; // ORDER IS IMPORTANT
			glUniformMatrix4fv(gMVPUniformSimpleClr, 1, GL_FALSE, modelViewProjectionMatrix);

			renderElement((void*)(sizeof(GLushort) * clicked_Obj_Id * 3), 0);

		}
		glUseProgram(0);
	}
	//else
	{
		glUseProgram(gBasicShaderProgObj);
		// translate for perspective
		modelViewMatrix = translate(0.0f, 0.0f, -6.0f);

		// rotate pyramid
		rotationMatrix = rotate(gAnglePyramid, 0.0f, 1.0f, 0.0f);
		modelViewMatrix = modelViewMatrix * rotationMatrix;

		// multiply the modelview and orthographic matrix to get modelviewprojection matrix
		modelViewProjectionMatrix = gPerspectiveProjectionMatrix * modelViewMatrix; // ORDER IS IMPORTANT
		glUniformMatrix4fv(gMVPUniformBasic, 1, GL_FALSE, modelViewProjectionMatrix);

		glActiveTexture(GL_TEXTURE);
		glBindTexture(GL_TEXTURE_2D, gTexture_Stone);
		glUniform1i(gTexture_samplerBasic, 0);
		glUniform4f(gColorModBasic, 1.0, 1.0, 1.0, 1.0);

		for (int i = 0; i < 2; i++)
		{
			if (i == clicked_Obj_Id) {}
			else
			renderElement((void*)(sizeof(GLushort) * i * 3), 0);
		}
		glUseProgram(0);
	}

	
}

void pickingPhase(void)
{
	void renderElement(void* baseIndex, GLint baseVertex);
	pickingEffect->EnableWriting();

	GLuint objID;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(gPickingShaderProgObj);
	// set modelview & modelviewprojection matrices to identity
	mat4 modelViewMatrix = mat4::identity();
	mat4 modelViewProjectionMatrix = mat4::identity();
	mat4 rotationMatrix = mat4::identity();
	// translate for perspective
	modelViewMatrix = translate(0.0f, 0.0f, -6.0f);

	
	// rotate pyramid
	rotationMatrix = rotate(gAnglePyramid, 0.0f, 1.0f, 0.0f);
	modelViewMatrix = modelViewMatrix * rotationMatrix;

	// multiply the modelview and orthographic matrix to get modelviewprojection matrix
	modelViewProjectionMatrix = gPerspectiveProjectionMatrix * modelViewMatrix; // ORDER IS IMPORTANT
	glUniformMatrix4fv(gMVPUniformPicking, 1, GL_FALSE, modelViewProjectionMatrix);


	for (int i = 0; i < 2; i++)
	{
		objID = i + 1;
		glUniform1ui(gObjIndexUniformPicking, ((GLuint)objID));
		glUniform1ui(gDrawIndexUniformPicking, (GLuint)1);
		renderElement((void*)(sizeof(GLushort) * i * 3), 0);
	}

	glUseProgram(0);

	pickingEffect->DisableWriting();
}

void renderElement(void * baseIndex, GLint baseVertex)
{
	// *** bind vao ***
	glBindVertexArray(gVao_pyramid);

	glDrawElementsBaseVertex(GL_TRIANGLES,
		3,
		GL_UNSIGNED_SHORT,
		baseIndex,
		baseVertex);
	glBindVertexArray(0);
	

}


void renderCube(void)
{
	// set modelview & modelviewprojection matrices to identity
	mat4 modelViewMatrix = mat4::identity();
	mat4 modelViewProjectionMatrix = mat4::identity();
	mat4 rotationMatrix = mat4::identity();

	glUseProgram(gShaderProgramObject);
	// LOAD IDENTITY AGAIN
	modelViewMatrix = mat4::identity();
	modelViewProjectionMatrix = mat4::identity();
	rotationMatrix = mat4::identity();

	modelViewMatrix = translate(1.5f, 0.0f, -6.0f);
	rotationMatrix = rotate(gAngleCube, gAngleCube, gAngleCube);
	modelViewMatrix = modelViewMatrix * rotationMatrix;

	modelViewProjectionMatrix = gPerspectiveProjectionMatrix * modelViewMatrix;

	glUniformMatrix4fv(gMVPUniform, 1, GL_FALSE, modelViewProjectionMatrix);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexture_Brickwall);
	glUniform1i(gTexture_sampler_uniform, 0);

	// draw cube
	glBindVertexArray(gVao_cube);
	// *** draw, either by glDrawTriangles() or glDrawArrays() or glDrawElements()
	// actually 2 triangles make 1 square, so there should be 6 vertices,
	// but as 2 tringles while making square meet each other at diagonal,
	// 2 of 6 vertices are common to both triangles, and hence 6-2=4
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 20, 4);
	// *** unbind vao ***
	glBindVertexArray(0);

	// stop using OpenGL program object
	glUseProgram(0);
}


void update(void)
{
	// code
	gAnglePyramid = gAnglePyramid + 0.2f;
	if (gAnglePyramid >= 360.0f)
		gAnglePyramid = gAnglePyramid - 360.0f;

	gAngleCube = gAngleCube + 0.1f;
	if (gAngleCube >= 360.0f)
		gAngleCube = gAngleCube - 360.0f;
}

void resize(int width, int height)
{
	//code
	if (height == 0)
		height = 1;
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	
	//for obj picking
	gWin_Width = width;
	gWin_Height = height;
	//glOrtho(left,right,bottom,top,near,far);

		gPerspectiveProjectionMatrix = perspective(45.0, (GLfloat)width / (GLfloat)height, 1.0f, 100.0f); //co-ordinates written for glVertex3f() are relative to viewing volume of (-100.0f,100.0f,(-100.0f * (height/width)),(100.0f * (height/width)),-100.0f,100.0f)
	}

void uninitialize(void)
{
	//UNINITIALIZATION CODE
	if (gbFullscreen == true)
	{
		dwStyle = GetWindowLong(ghwnd, GWL_STYLE);
		SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(ghwnd, &wpPrev);
		SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);

		ShowCursor(TRUE);

	}

	// PYRAMID
	// destroy vao
	if (gVao_pyramid)
	{
		glDeleteVertexArrays(1, &gVao_pyramid);
		gVao_pyramid = 0;
	}

	// destroy vbo position
	if (gVbo_pyramid_position)
	{
		glDeleteBuffers(1, &gVbo_pyramid_position);
		gVbo_pyramid_position = 0;
	}

	// destroy vbo texture
	if (gVbo_pyramid_texture)
	{
		glDeleteBuffers(1, &gVbo_pyramid_texture);
		gVbo_pyramid_texture = 0;
	}

	if (gTexture_Stone)
	{
		glDeleteTextures(1, &gTexture_Stone);
		gTexture_Stone = 0;
	}

	// CUBE
	if (gVao_cube)
	{
		glDeleteVertexArrays(1, &gVao_cube);
		gVao_cube = 0;
	}

	// destroy vbo position
	if (gVbo_cube_position)
	{
		glDeleteBuffers(1, &gVbo_cube_position);
		gVbo_cube_position = 0;
	}

	// destroy vbo texture
	if (gVbo_cube_texture)
	{
		glDeleteBuffers(1, &gVbo_cube_texture);
		gVbo_cube_texture = 0;
	}

	if (gTexture_Brickwall)
	{
		glDeleteTextures(1, &gTexture_Brickwall);
		gTexture_Brickwall = 0;
	}

	// detach vertex shader from shader program object
	glDetachShader(gShaderProgramObject, gVertexShaderObject);
	// detach fragment  shader from shader program object
	glDetachShader(gShaderProgramObject, gFragmentShaderObject);

	// delete vertex shader object
	glDeleteShader(gVertexShaderObject);
	gVertexShaderObject = 0;
	// delete fragment shader object
	glDeleteShader(gFragmentShaderObject);
	gFragmentShaderObject = 0;

	// delete shader program object
	glDeleteProgram(gShaderProgramObject);
	gShaderProgramObject = 0;

	// unlink shader program
	glUseProgram(0);


	cleanUpObjectPickingData();

	//Deselect the rendering context
	wglMakeCurrent(NULL, NULL);

	//Delete the rendering context
	wglDeleteContext(ghrc);
	ghrc = NULL;

	//Delete the device context
	ReleaseDC(ghwnd, ghdc);
	ghdc = NULL;

	if (gpFile)
	{
		fprintf(gpFile, "Log File Is Successfully Closed.\n");
		fclose(gpFile);
		gpFile = NULL;
	}
}
