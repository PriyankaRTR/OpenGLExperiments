#include <windows.h>
#include <stdio.h> // for FILE I/O

#include <gl\glew.h> // for GLSL extensions IMPORTANT : This Line Should Be Before #include<gl\gl.h> And #include<gl\glu.h>

#include <gl/GL.h>

#include "vmath.h"
#include "Shadow.h"
#include "Shader.h" // not used for this experiment

#include "OpenGLPP_Shadow_Mapping.h"

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

enum
{
	SHADOW_MAP = 1,
	NORMAL_SCENE,
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

GLuint gModelUniform, gViewUniform, gProjUniform;

GLuint gVShaderLightSourceObject;
GLuint gFShaderLightSourceObject;
GLuint gShaderProgLightSourceObject;

GLuint gVao_cube;
GLuint gVbo_cube_position;
GLuint gVbo_cube_normal, gVbo_cube_texture;

GLuint gVao_plane;
GLuint gVbo_plane_position, gVbo_plane_normal, gVbo_plane_texture;

GLuint planeVBO, planeVAO;

GLuint gTexture_Stone, gTexture_Wood, gTexture_Fabric, gTexture_BrickWall, gTextureFloor;

GLsizei currentWinWidth, currentWinHeight;

GLuint gModelMatrixUniform, gViewMatrixUniform, gProjectionMatrixUniform,  gLightSpaceMatrixUniform;
GLuint gLdUniform, gKdUniform, gLaUniform, gKaUniform, gLsUniform, gKsUniform, gLightPositionUniform, gViewPositionUniform;
GLuint gLKeyPressedUniform, gAKeyPressedUniform, gDKeyPressedUniform, gSKeyPressedUniform;
GLuint gTexSamplerUniform, gDepthTexSamplerUniform;
GLuint keyA, keyD, keyS;

mat4 gPerspectiveProjectionMatrix;
mat4 gOrthographicProjectionMatrix;

GLfloat gAngle = 0.0f;

bool gbAnimate;
bool gbLight;

Shadow* shadowInstance;
Shader* shadowDebugQuadSahder;
GLuint shadowShaderProgObj;

float mouseOffset = 45.0f;

float lightPosition[] = { -2.0f, 4.0f, -1.0f }; //  { -1.0f, 4.5f, -2.0f };  {-2.0f, 4.0f, -1.0f};//{ (r * sin(gAngle)), 0.0f, (r * cos(gAngle) - 5.0) };
float viewPosition[] = { -3.0f, 4.0f, 4.0f };//{ -1.0f, 3.0f, 0.0f };// { -1.0f, 2.0f, 0.0f };//{ -1.0f, 1.5f, 0.0f }; ////{ -1.0f, 6.0f, 0.0f }; // //-1.0f, 1.5f, 0.0f
mat4 lightSpaceMatrix = mat4::identity();


//main()
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
	//function prototype
	void initialize(void);
	void uninitialize(void);
	void display(void);
	void spin(void);

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

			if (gbAnimate == true)
				spin();

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
	static bool bIsAKeyPressed = false;
	static bool bIsDKeyPressed = false;
	static bool bIsSKeyPressed = false;
	static bool bIsLKeyPressed = false;

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
		case 0x41: // for 'A' or 'a'
			if (bIsAKeyPressed == false)
			{
				keyA = 1;
				gbAnimate = true;
				bIsAKeyPressed = true;
			}
			else
			{
				keyA = 0;
				gbAnimate = false;
				bIsAKeyPressed = false;
			}
			break;
		case 0x44: // for 'D' or 'd'
			if (bIsDKeyPressed == false)
			{
				keyD = 1;
				bIsDKeyPressed = true;
			}
			else
			{
				keyD = 0;
				bIsDKeyPressed = false;
			}
			break;
			
		case 0x53: // for 'S' or 's'
			if (bIsSKeyPressed == false)
			{
				keyS = 1;
				bIsSKeyPressed = true;
			}
			else
			{
				keyS = 0;
				bIsSKeyPressed = false;
			}
			break;
		case 0x4C: // for 'L' or 'l'
			if (bIsLKeyPressed == false)
			{
				gbLight = true;
				bIsLKeyPressed = true;
			}
			else
			{
				gbLight = false;
				bIsLKeyPressed = false;
			}
			break;
		default:
			break;
		}
		break;
	case WM_LBUTTONDOWN:  //Parallel to glutMouseFunc();
		break;
	case WM_MOUSEWHEEL:
		mouseOffset += ((GET_WHEEL_DELTA_WPARAM(wParam) / 120) * 5);
		if (mouseOffset > 60.0f)
			mouseOffset = 60.0f;
		if (mouseOffset < 1.0f)
			mouseOffset = 1.0f;
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
	int LoadGLTextures(GLuint * texture, TCHAR imageResourceId[]);
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



	// Light source shaders
	gVShaderLightSourceObject = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* vShadercode =
		"#version 330" \
		"\n" \
		"uniform mat4 model;" \
		"uniform mat4 view;" \
		"uniform mat4 proj;" \
		"in vec4 vPosition;" \
		"void main()" \
		"{" \
		"gl_Position = (proj * view * model) * vPosition;" \
		"}";

	glShaderSource(gVShaderLightSourceObject, 1, (const GLchar**)&vShadercode, NULL);

	// compile shader
	glCompileShader(gVShaderLightSourceObject);
	GLint iInfoLogLength = 0;
	GLint iShaderCompiledStatus = 0;
	char* szInfoLog = NULL;
	glGetShaderiv(gVShaderLightSourceObject, GL_COMPILE_STATUS, &iShaderCompiledStatus);
	if (iShaderCompiledStatus == GL_FALSE)
	{
		glGetShaderiv(gVShaderLightSourceObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (char*)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(gVShaderLightSourceObject, iInfoLogLength, &written, szInfoLog);
				fprintf(gpFile, "light Vertex Shader Compilation Log : %s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}

	gFShaderLightSourceObject = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar* fShaderCode =
		"#version 330" \
		"\n" \
		"out vec4 fragColor;" \
		"void main()" \
		"{" \
		"fragColor = vec4(1.0);" \
		"}";

	glShaderSource(gFShaderLightSourceObject, 1, (const GLchar**)&fShaderCode, NULL);

	// compile shader
	glCompileShader(gFShaderLightSourceObject);
	iInfoLogLength = 0;
	iShaderCompiledStatus = 0;
	szInfoLog = NULL;
	glGetShaderiv(gFShaderLightSourceObject, GL_COMPILE_STATUS, &iShaderCompiledStatus);
	if (iShaderCompiledStatus == GL_FALSE)
	{
		glGetShaderiv(gFShaderLightSourceObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (char*)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(gFShaderLightSourceObject, iInfoLogLength, &written, szInfoLog);
				fprintf(gpFile, "Frag light Vertex Shader Compilation Log : %s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}

	gShaderProgLightSourceObject = glCreateProgram();

	// attach vertex shader to shader program
	glAttachShader(gShaderProgLightSourceObject, gVShaderLightSourceObject);

	// attach fragment shader to shader program
	glAttachShader(gShaderProgLightSourceObject, gFShaderLightSourceObject);

	// pre-link binding of shader program object with vertex shader position attribute
	glBindAttribLocation(gShaderProgLightSourceObject, VDG_ATTRIBUTE_VERTEX, "vPosition");
	

	// link shader
	glLinkProgram(gShaderProgLightSourceObject);
	GLint iShaderProgramLinkStatus = 0;
	glGetProgramiv(gShaderProgLightSourceObject, GL_LINK_STATUS, &iShaderProgramLinkStatus);
	if (iShaderProgramLinkStatus == GL_FALSE)
	{
		glGetProgramiv(gShaderProgLightSourceObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (char*)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetProgramInfoLog(gShaderProgLightSourceObject, iInfoLogLength, &written, szInfoLog);
				fprintf(gpFile, "Shader Program Link Log : %s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}


	// get uniform locations
	gModelUniform = glGetUniformLocation(gShaderProgLightSourceObject, "model");
	gProjUniform = glGetUniformLocation(gShaderProgLightSourceObject, "proj");
	gViewUniform = glGetUniformLocation(gShaderProgLightSourceObject, "view");





	// *** VERTEX SHADER ***
	// create shader
	gVertexShaderObject = glCreateShader(GL_VERTEX_SHADER);

	// provide source code to shader
	const GLchar *vertexShaderSourceCode =
		"#version 330" \
		"\n" \
		"in vec4 vPosition;" \
		"in vec3 vNormal;" \
		"in vec2 vTexCoord;" \
		"out vec3 normalVec;" \
		"out vec3 fragPos;" \
		"out vec2 texCoord;" \
		"out vec4 fragPosLightSpace;" \
		"uniform mat4 u_model_matrix;" \
		"uniform mat4 u_view_matrix;" \
		"uniform mat4 u_projection_matrix;" \
		"uniform mat4 u_lightSpaceMatrix;" \
		"uniform int u_LKeyPressed;" \
		"uniform vec3 u_Ld;" \
		"uniform vec3 u_Kd;" \
		"void main(void)" \
		"{" \
		"if (u_LKeyPressed == 1)" \
		"{" \
		"}" \
		"fragPos = vec3(u_model_matrix * vPosition);" \
		"normalVec = transpose(inverse(mat3(u_model_matrix))) * vNormal;" \
		"gl_Position = (u_projection_matrix * u_view_matrix * u_model_matrix) * vPosition;" \
		"texCoord = vTexCoord;" \
		"fragPosLightSpace = vec4(u_lightSpaceMatrix * vec4(fragPos, 1.0));" \
		"}";


	glShaderSource(gVertexShaderObject, 1, (const GLchar **)&vertexShaderSourceCode, NULL);

	// compile shader
	glCompileShader(gVertexShaderObject);
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
	const GLchar* fragmentShaderSourceCode =
		"#version 330" \
		"\n" \
		"out vec4 FragColor;" \
		"in vec3 normalVec;" \
		"in vec4 vPosition;" \
		"in vec3 fragPos;" \
		"in vec2 texCoord;" \
		"in vec4 fragPosLightSpace;" \
		"uniform int u_LKeyPressed;" \
		"uniform int u_AKeyPressed;" \
		"uniform int u_DKeyPressed;" \
		"uniform int u_SKeyPressed;" \
		"uniform vec3 u_light_position;" \
		"uniform vec3 u_view_position;" \
		"uniform vec3 u_Ld;" \
		"uniform vec3 u_Kd;" \
		"uniform vec3 u_La;" \
		"uniform vec3 u_Ka;" \
		"uniform vec3 u_Ls;" \
		"uniform vec3 u_Ks;" \
		"uniform sampler2D texSampler;" \
		"uniform sampler2D shadowMap;" \
		"vec3 s;" \
		"vec3 normalNorm;" \
		"float shadowCalculation(vec4 fragPosLightSpace)" \
		"{" \
		"float bias = max(0.05*(1.0-dot(normalNorm, s)), 0.005);" /*to address issue of shadow acne*/ \
		"vec3 projCoords = (fragPosLightSpace.xyz/fragPosLightSpace.w);" \
		"projCoords = (projCoords * 0.5f) + 0.5f;" \
		"float closestDepth = texture( shadowMap, projCoords.xy).r;" \
		"float currentDepth = projCoords.z;" \
		"float shadow = 0.0;" \
		"vec2 texelSize = 1.0 / textureSize(shadowMap, 0);" \
		"for (int x=-1; x<=1; ++x)" \
		"{" \
		"for(int y = -1; y<=1; ++y)" \
		"{" \
		"float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x,y) * texelSize).r;" \
		"shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;" \
		/*"float shadow = (currentDepth - bias) > closestDepth ? 1.0 : 0.0;" \*/
		"}" \
		"}" \
		"shadow /=9.0;" \
		"if(projCoords.z > 1.0)" \
		"shadow = 0.0;" \
		"return shadow;" \
		"}" \
		"void main(void)" \
		"{" \
		"vec4 color;" \
		"vec3 ambient_light;" \
		"vec3 diffuse_light;" \
		"vec3 specular_light;" \
		"vec3 textureColorValue;" \
		"float shadow;" \
		"if (u_LKeyPressed == 1)" \
		"{" \
		"normalNorm = normalize(normalVec);"
		"s = normalize(vec3(u_light_position - fragPos));" \
		"float ambientStrength = 0.1;" \
		"ambient_light = u_La * u_Ka * ambientStrength;" \
		"diffuse_light = u_Ld * u_Kd * max(dot(s, normalNorm), 0.0);" \
		"float specularStrength = 0.5;" \
		"vec3 viewDir = normalize(u_view_position - fragPos);" \
		"vec3 reflectDir = reflect(-s, normalNorm);" \
		"float spec = pow(max(dot(viewDir, reflectDir), 0.0), 128);" \
		"specular_light = u_Ls * u_Ks * specularStrength * spec;" \
		"textureColorValue = texture(texSampler, texCoord).rgb;" \
		"shadow = shadowCalculation(fragPosLightSpace);" \
		"if(u_DKeyPressed == 1)" \
		"{" \
		"if(u_SKeyPressed == 1)" \
		"{" \
		"color = vec4( (ambient_light + diffuse_light + specular_light)* textureColorValue,1.0);" \
		"}" \
		"else" \
		"{" \
		"color = vec4( (ambient_light + diffuse_light + specular_light),1.0);" \
		"}" \
		"}" \
		"else" \
		"{" \
		"color = vec4( (ambient_light + (1.0 - shadow)) *(diffuse_light + specular_light)* textureColorValue,1.0);" \
		"}" \
		"}" \
		"else" \
		"{" \
		"color = vec4(1.0, 1.0, 1.0, 1.0);" \
		"}" \
		"FragColor = color;" \
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
	glBindAttribLocation(gShaderProgramObject, VDG_ATTRIBUTE_NORMAL, "vNormal");
	glBindAttribLocation(gShaderProgramObject, VDG_ATTRIBUTE_TEXTURE0, "vTexCoord");
	//glBindAttribLocation(gShaderProgramObject, VDG_ATTRIBUTE_TEXTURE0, "vNormal");
	// link shader
	glLinkProgram(gShaderProgramObject);
	iShaderProgramLinkStatus = 0;
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

	// get uniform locations
	gModelMatrixUniform = glGetUniformLocation(gShaderProgramObject, "u_model_matrix");
	gProjectionMatrixUniform = glGetUniformLocation(gShaderProgramObject, "u_projection_matrix");
	gViewMatrixUniform = glGetUniformLocation(gShaderProgramObject, "u_view_matrix");
	gLightSpaceMatrixUniform = glGetUniformLocation(gShaderProgramObject, "u_lightSpaceMatrix");

	gLKeyPressedUniform = glGetUniformLocation(gShaderProgramObject, "u_LKeyPressed");
	gAKeyPressedUniform = glGetUniformLocation(gShaderProgramObject, "u_AKeyPressed");
	gDKeyPressedUniform = glGetUniformLocation(gShaderProgramObject, "u_DKeyPressed");
	gSKeyPressedUniform = glGetUniformLocation(gShaderProgramObject, "u_SKeyPressed");

	gLaUniform = glGetUniformLocation(gShaderProgramObject, "u_La");
	gKaUniform = glGetUniformLocation(gShaderProgramObject, "u_Ka");
	gLdUniform = glGetUniformLocation(gShaderProgramObject, "u_Ld");
	gKdUniform = glGetUniformLocation(gShaderProgramObject, "u_Kd");
	gLsUniform = glGetUniformLocation(gShaderProgramObject, "u_Ls");
	gKsUniform = glGetUniformLocation(gShaderProgramObject, "u_Ks");

	gLightPositionUniform = glGetUniformLocation(gShaderProgramObject, "u_light_position");
	gViewPositionUniform = glGetUniformLocation(gShaderProgramObject, "u_view_position");
	gTexSamplerUniform = glGetUniformLocation(gShaderProgramObject, "texSampler");
	gDepthTexSamplerUniform = glGetUniformLocation(gShaderProgramObject, "shadowMap");


	//// shadow depth buffer setup
	shadowInstance = new Shadow();
	shadowShaderProgObj = shadowInstance->createShadowShaderProgram();
	shadowInstance->createDepthBuffer();
	shadowInstance->createTextureMap();
	shadowInstance->attachTextureToFBO();


	// debug depth buffer rendering to quad
	shadowDebugQuadSahder = new Shader("shadowDebugQuad.vs", "shadowDebugQuad.fs", gpFile);



	// *** vertices, colors, shader attribs, vbo, vao initializations ***
	GLfloat cubeVertices[] =
	{
		1.0f, 1.0f, -1.0f, // top
		-1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,

		1.0f, -1.0f, 1.0f, // bottom
		-1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,

		1.0f, 1.0f, 1.0f, // front
		-1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,

		1.0f, -1.0f, -1.0f, // back
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,

		-1.0f, 1.0f, 1.0f, // left
		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,

		1.0f, 1.0f, -1.0f, //right
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
	};


	const GLfloat cubeNormals[] =
	{
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,

		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,

		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,

		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,

		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f
	};



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
	// vao
	glGenVertexArrays(1, &gVao_cube);
	glBindVertexArray(gVao_cube);

	// position vbo
	glGenBuffers(1, &gVbo_cube_position);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_cube_position);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(VDG_ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// normal vbo
	glGenBuffers(1, &gVbo_cube_normal);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_cube_normal);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeNormals), cubeNormals, GL_STATIC_DRAW);

	glVertexAttribPointer(VDG_ATTRIBUTE_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(VDG_ATTRIBUTE_NORMAL);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// vbo for texture
	glGenBuffers(1, &gVbo_cube_texture);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_cube_texture);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeTexcoords), cubeTexcoords, GL_STATIC_DRAW);

	glVertexAttribPointer(VDG_ATTRIBUTE_TEXTURE0, 2, GL_FLOAT, GL_FALSE, 0, NULL); // 3 is for r,g,b in cubeColors array

	glEnableVertexAttribArray(VDG_ATTRIBUTE_TEXTURE0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);


	GLfloat planeVertices[] =
	{
		25.0f, 0.0f, -25.0f, // top
		-25.0f, 0.0f, -25.0f,
		-25.0f, 0.0f, 25.0f,
		25.0f, 0.0f, 25.0f
	};

	const GLfloat planeNormals[] =
	{
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f
	};

	const GLfloat planeTexcoords[] =
	{
		0.0f, 0.0f,
		25.0f, 0.0f,
		25.0f, 25.0f,
		0.0f, 25.0f
	};


	// PLANE CODE
	// vao
	glGenVertexArrays(1, &gVao_plane);
	glBindVertexArray(gVao_plane);

	// position vbo
	glGenBuffers(1, &gVbo_plane_position);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_plane_position);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(VDG_ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// normal vbo
	glGenBuffers(1, &gVbo_plane_normal);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_plane_normal);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeNormals), planeNormals, GL_STATIC_DRAW);

	glVertexAttribPointer(VDG_ATTRIBUTE_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(VDG_ATTRIBUTE_NORMAL);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// vbo for texture
	glGenBuffers(1, &gVbo_plane_texture);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_plane_texture);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeTexcoords), planeTexcoords, GL_STATIC_DRAW);

	glVertexAttribPointer(VDG_ATTRIBUTE_TEXTURE0, 2, GL_FLOAT, GL_FALSE, 0, NULL); // 3 is for r,g,b in planeColors array

	glEnableVertexAttribArray(VDG_ATTRIBUTE_TEXTURE0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);





	glShadeModel(GL_SMOOTH);
	// set-up depth buffer
	//glClearDepth(1.0f);
	// enable depth testing
	glEnable(GL_DEPTH_TEST);
	// depth test to do
	glDepthFunc(GL_LEQUAL);
	// set really nice percpective calculations ?
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	// We will always cull back faces for better performance
	//glEnable(GL_CULL_FACE);

	LoadGLTextures(&gTexture_Stone, MAKEINTRESOURCE(IDBITMAP_STONE));
	LoadGLTextures(&gTexture_Wood, MAKEINTRESOURCE(IDBITMAP_WOOD));
	LoadGLTextures(&gTexture_BrickWall, MAKEINTRESOURCE(IDBITMAP_BRICKWALL));
	LoadGLTextures(&gTexture_Fabric, MAKEINTRESOURCE(IDBITMAP_FABRIC));
	LoadGLTextures(&gTextureFloor, MAKEINTRESOURCE(IDBITMAP_FLOOR));
	
	// set background color
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // black

	// set perspective matrix to identitu matrix
	gPerspectiveProjectionMatrix = mat4::identity();
	gOrthographicProjectionMatrix = mat4::identity();

	gbAnimate = true;
	gbLight = true;

	// resize
	resize(WIN_WIDTH, WIN_HEIGHT);
}

void display(void)
{
	void renderToTheDepthMap(void);
	void renderNormalScene(void);
	void renderSceneForDepthMap(void);
	void renderDepthTextureQuad(void);

	

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//glCullFace(GL_FRONT);
	renderToTheDepthMap();
	//glCullFace(GL_BACK);



	glViewport(0, 0, currentWinWidth, currentWinHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderNormalScene();

	float near_plane = 1.0f, far_plane = 7.5f;

	// below code is to check the depth buffer rendering
	//shadowDebugQuadSahder->start();
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, shadowInstance->getDepthMap());
	//shadowDebugQuadSahder->setUniformValue(near_plane, far_plane);
	//renderDepthTextureQuad();
	//shadowDebugQuadSahder->stop();

	SwapBuffers(ghdc);
}

void renderToTheDepthMap(void)
{
	void renderScene(Shadow * shadowShader, GLuint typeOfRendering);

	mat4 prjectionMatrix = mat4::identity();
	mat4 lightViewMatrix = mat4::identity();

	float r = 5.0f;

	lightViewMatrix = lookat(vec3(lightPosition[0], lightPosition[1], lightPosition[2]), vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0));
	float near_plane = -10.0f, far_plane = 7.5;
	prjectionMatrix = ortho(-10.0f, 10.0f, -10.0, 10.0, near_plane, far_plane);

	lightSpaceMatrix = prjectionMatrix * lightViewMatrix;

	glUseProgram(shadowShaderProgObj);
	shadowInstance->setLightSpaceMatUniform(lightSpaceMatrix);

	glViewport(0, 0, shadowInstance->SHADOW_WIDTH, shadowInstance->SHADOW_HEIGHT);
	shadowInstance->bindDepthFramebuffer();
	glClear(GL_DEPTH_BUFFER_BIT);
	
	// OpenGL Drawing
	renderScene(shadowInstance, SHADOW_MAP);
	shadowInstance->unbindDepthFramebuffer();
	glUseProgram(0);
}

void renderNormalScene()
{
	void renderLightSourceCube(void);
	void renderFloorQuad(void);
	void renderDepthTextureQuad(void);
	void normalSceneSetup(void);
	void renderScene(Shadow * shadowShader, GLuint typeOfRendering);

	normalSceneSetup(); // set up all uniform and view , projection matrix
	renderScene(shadowInstance, NORMAL_SCENE);
	glUseProgram(0);
	renderLightSourceCube();

}





void renderCube(void)
{
	// OpenGL Drawing
	// *** bind vao ***
	glBindVertexArray(gVao_cube);

	// *** draw, either by glDrawTriangles() or glDrawArrays() or glDrawElements()
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 20, 4);

	// *** unbind vao ***
	glBindVertexArray(0);
}

void renderFloorQuad(void)
{
	
	glBindVertexArray(gVao_plane);
	// *** draw, either by glDrawTriangles() or glDrawArrays() or glDrawElements()
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	// *** unbind vao ***
	glBindVertexArray(0);
}




void renderScene(Shadow* shadowShader, GLuint typeOfRendering)
{
	void renderCube(void);
	void renderFloorQuad(void);
	mat4 modelMatrix = mat4::identity();
	mat4 rotationMatrix = mat4::identity();

	vec3 positions[3] = { vec3(0.0f, 2.0f, 0.0f), vec3(2.0f, 0.8f, 1.0f), vec3(-1.0f, 0.5f, 2.0f) };
	vec3 rotations[3] = { vec3(0.0f,0.0f,0.0f), vec3(0.0f, 40.0f, 0.0f), vec3(60.0f, 0.0f, 60.0f) };
	GLfloat scales[3] = { 0.5f,0.4f,0.25f };


	//modelMatrix = modelMatrix * rotationMatrix; // ORDER IS IMPORTANT
	if (typeOfRendering == SHADOW_MAP)
	{
		shadowShader->setModelMatUniform(modelMatrix);
	}
	else
	{
		glUniformMatrix4fv(gModelMatrixUniform, 1, GL_FALSE, modelMatrix);
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureFloor);
	glUniform1i(gTexSamplerUniform, 0);
	renderFloorQuad();

	modelMatrix = mat4::identity();
	rotationMatrix = mat4::identity();


	glActiveTexture(GL_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, gTexture_BrickWall);
	glUniform1i(gTexSamplerUniform, 0);
	for (int i = 0; i < 3; i++)
	{
		// apply z axis translation to go deep into the screen by -5.0,
		// so that triangle with same fullscreen co-ordinates, but due to above translation will look small
		modelMatrix = scale(scales[i]);
		modelMatrix = translate(positions[i]) * modelMatrix;
		rotationMatrix = rotate(rotations[i][0], rotations[i][1], rotations[i][2]); // all axes rotation by gAngle angle
		modelMatrix = modelMatrix * rotationMatrix; // ORDER IS IMPORTANT
		if (typeOfRendering == SHADOW_MAP)
		{
			shadowShader->setModelMatUniform(modelMatrix);
		}
		else
		{
			glUniformMatrix4fv(gModelMatrixUniform, 1, GL_FALSE, modelMatrix);
		}

		renderCube();

		modelMatrix = mat4::identity();
		rotationMatrix = mat4::identity();
	}

}

void normalSceneSetup(void)
{
	float r = 2.0f;
	// start using OpenGL program object
	glUseProgram(gShaderProgramObject);
	
	if (gbLight == true)
	{
		glUniform1i(gLKeyPressedUniform, 1);
		glUniform1i(gAKeyPressedUniform, keyA);
		glUniform1i(gDKeyPressedUniform, keyD);
		glUniform1i(gSKeyPressedUniform, keyS);

		glUniform3f(gLdUniform, 1.0f, 1.0f, 1.0f);
		glUniform3f(gKdUniform, 1.0f, 1.0f, 1.0f); //1.0f, 0.5f, 0.31f);
		glUniform3f(gLaUniform, 1.0f, 1.0f, 1.0f);
		glUniform3f(gKaUniform, 1.0f, 1.0f, 1.0f);
		glUniform3f(gLsUniform, 1.0f, 1.0f, 1.0f);
		glUniform3f(gKsUniform, 1.0f, 1.0f, 1.0f);

		glUniform3fv(gLightPositionUniform, 1, (GLfloat*)lightPosition);

		glUniform3fv(gViewPositionUniform, 1, (GLfloat*)viewPosition);
	}
	else
	{
		glUniform1i(gLKeyPressedUniform, 0);
	}

	mat4 viewMatrix = mat4::identity();
	viewMatrix = lookat(vec3(viewPosition[0], viewPosition[1], viewPosition[2]), vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0)); 
	gPerspectiveProjectionMatrix = perspective(mouseOffset, (GLfloat)currentWinWidth / (GLfloat)currentWinHeight, 0.1f, 100.0f);
	glUniformMatrix4fv(gViewMatrixUniform, 1, GL_FALSE, viewMatrix);
	// pass projection matrix to the vertex shader in 'u_projection_matrix' shader variable
	// whose position value we already calculated in initialize() by using glGetUniformLocation()
	glUniformMatrix4fv(gProjectionMatrixUniform, 1, GL_FALSE, gPerspectiveProjectionMatrix);
	glUniformMatrix4fv(gLightSpaceMatrixUniform, 1, GL_FALSE, lightSpaceMatrix);


	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, shadowInstance->getDepthMap());
	glUniform1i(gDepthTexSamplerUniform, 1);
}

void renderLightSourceCube(void)
{
	float r = 2.0f;
	glUseProgram(gShaderProgLightSourceObject);
	mat4 modelMat = mat4::identity();
	mat4 viewMat = mat4::identity();
	mat4 rotMat = mat4::identity();

	modelMat = scale(0.1f);
	// apply z axis translation to go deep into the screen by -5.0,
	// so that triangle with same fullscreen co-ordinates, but due to above translation will look small
	modelMat = translate(lightPosition[0], lightPosition[1], lightPosition[2]) * modelMat; //translate((float)(r * sin(gAngle)), 0.0f, (float)(r * cos(gAngle) - 5.0)) * modelMat;

	// all axes rotation by gAngle angle
	rotMat = rotate(0.0f, 45.0f, 0.0f);

	// multiply rotation matrix and model matrix to get modelView matrix
	modelMat = modelMat * rotMat; // ORDER IS IMPORTANT

	viewMat = lookat(vec3(viewPosition[0], viewPosition[1], viewPosition[2]), vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0));

	glUniformMatrix4fv(gModelUniform, 1, GL_FALSE, modelMat);
	glUniformMatrix4fv(gViewUniform, 1, GL_FALSE, viewMat);
	glUniformMatrix4fv(gProjUniform, 1, GL_FALSE, gPerspectiveProjectionMatrix);

	renderCube();

	glUseProgram(0);
}



void renderDepthTextureQuad(void)
{
	// OpenGL Drawing
	// *** bind vao ***
	glBindVertexArray(gVao_cube);

	// *** draw, either by glDrawTriangles() or glDrawArrays() or glDrawElements()
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
	// *** unbind vao ***
	glBindVertexArray(0);
}


void resize(int width, int height)
{
	//code
	if (height == 0)
		height = 1;
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	currentWinWidth = (GLsizei)width;
	currentWinHeight = (GLsizei)height;
	fprintf(gpFile, "size w- %d, h- %d\n", width, height);
	gPerspectiveProjectionMatrix = perspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);

	//if (width <= height)
	//	gOrthographicProjectionMatrix = ortho(-100.0f, 100.0f, (-100.0f * (height / width)), (100.0f * (height / width)), -100.0f, 100.0f); //co-ordinates written for glVertex3f() are relative to viewing volume of (-100.0f,100.0f,(-100.0f * (height/width)),(100.0f * (height/width)),-100.0f,100.0f)
	//else
	//	gOrthographicProjectionMatrix = ortho(-100.0f, 100.0f, (-100.0f * (width / height)), (100.0f * (width / height)), -100.0f, 100.0f); //co-ordinates written for glVertex3f() are relative to viewing volume of (-100.0f,100.0f,(-100.0f * (height/width)),(100.0f * (height/width)),-100.0f,100.0f) 
}

void spin(void)
{
	float r = 2.5;
	lightPosition[0] = (r * sin(gAngle));
	lightPosition[1] = 2.0f;
	lightPosition[2] = (r * cos(gAngle) - 1.0);

	// code
	gAngle = gAngle + 0.01f;
	if (gAngle >= 360.0f)
		gAngle = gAngle - 360.0f;
}




int LoadGLTextures(GLuint* texture, TCHAR imageResourceId[])
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

	// destroy vao
	if (gVao_cube)
	{
		glDeleteVertexArrays(1, &gVao_cube);
		gVao_cube = 0;
	}

	// destroy position vbo
	if (gVbo_cube_position)
	{
		glDeleteBuffers(1, &gVbo_cube_position);
		gVbo_cube_position = 0;
	}

	// destroy normal vbo
	if (gVbo_cube_normal)
	{
		glDeleteBuffers(1, &gVbo_cube_normal);
		gVbo_cube_normal = 0;
	}

	// destroy normal vbo
	if (gVbo_cube_texture)
	{
		glDeleteBuffers(1, &gVbo_cube_texture);
		gVbo_cube_texture = 0;
	}

	// destroy vao
	if (planeVAO)
	{
		glDeleteVertexArrays(1, &planeVAO);
		planeVAO = 0;
	}
	// destroy normal vbo
	if (planeVBO)
	{
		glDeleteBuffers(1, &planeVBO);
		planeVBO = 0;
	}

	if (gVao_plane)
	{
		glDeleteVertexArrays(1, &gVao_plane);
		gVao_plane = 0;
	}
	// destroy normal vbo
	if (gVbo_plane_position)
	{
		glDeleteBuffers(1, &gVbo_plane_position);
		gVbo_plane_position = 0;
	}
	if (gVbo_plane_normal)
	{
		glDeleteBuffers(1, &gVbo_plane_normal);
		gVbo_plane_normal = 0;
	}
	if (gVbo_plane_texture)
	{
		glDeleteBuffers(1, &gVbo_plane_texture);
		gVbo_plane_texture = 0;
	}



	if (gTexture_Stone)
	{
		glDeleteTextures(1, &gTexture_Stone);
		gTexture_Stone = 0;
	}
	if (gTexture_Wood)
	{
		glDeleteTextures(1, &gTexture_Wood);
		gTexture_Wood = 0;
	}
	if (gTexture_BrickWall)
	{
		glDeleteTextures(1, &gTexture_BrickWall);
		gTexture_BrickWall = 0;
	}
	if (gTexture_Fabric)
	{
		glDeleteTextures(1, &gTexture_Fabric);
		gTexture_Fabric = 0;
	}

	if (gTextureFloor)
	{
		glDeleteTextures(1, &gTextureFloor);
		gTextureFloor = 0;
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



	// detach vertex shader from shader program object
	glDetachShader(gShaderProgLightSourceObject, gVShaderLightSourceObject);
	// detach fragment  shader from shader program object
	glDetachShader(gShaderProgLightSourceObject, gFShaderLightSourceObject);

	// delete vertex shader object
	glDeleteShader(gVShaderLightSourceObject);
	gVShaderLightSourceObject = 0;
	// delete fragment shader object
	glDeleteShader(gFShaderLightSourceObject);
	gFShaderLightSourceObject = 0;

	// delete shader program object
	glDeleteProgram(gShaderProgLightSourceObject);
	gShaderProgramObject = 0;


	if (shadowInstance)
		delete shadowInstance;

	if (shadowDebugQuadSahder)
		delete shadowDebugQuadSahder;

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
