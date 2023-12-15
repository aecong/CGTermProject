#define _CRT_SECURE_NO_WARNINGS //--- 프로그램 맨 앞에 선언할 것
#define STB_IMAGE_IMPLEMENTATION
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <random>
#include <string>
#include "stb_image.h"
#include "fmod.hpp"
#include "fmod_errors.h"
using namespace std;

random_device rd;
mt19937 gen(rd());
uniform_real_distribution<double> XYdis(-1, 1);
uniform_real_distribution<double> dis(0.0, 1.0);
uniform_int_distribution<int> onoffrandom(0, 400);
uniform_int_distribution<int> glass(0, 1);

void InitTexture();
int widthImage, heightImage, numberOfChannel;
unsigned int textures[15];

FMOD::System* ssystem;
FMOD::Sound* background, * jump, * glassbroken, * tramjump, * hit, * gameclear;
FMOD::Channel* bgm_channel = 0;
FMOD::Channel* end_channel = 0;
FMOD::Channel* effect_channel = 0;
FMOD_RESULT result;
void* extradriverdata = 0;

struct Transform
{
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale = glm::vec3(1.0, 1.0, 1.0);

	glm::mat4 GetTransform()
	{
		glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 S = glm::scale(glm::mat4(1.0), scale);
		glm::mat4 RX = glm::rotate(glm::mat4(1.0f), (float)glm::radians(rotation.x), glm::vec3(1.0, 0.0, 0.0));
		glm::mat4 RY = glm::rotate(glm::mat4(1.0f), (float)glm::radians(rotation.y), glm::vec3(0.0, 1.0, 0.0));
		glm::mat4 RZ = glm::rotate(glm::mat4(1.0f), (float)glm::radians(rotation.z), glm::vec3(0.0, 0.0, 1.0));
		return T * RX * RY * RZ * S;
	}
};

struct OBJECT {
	GLuint vao, vbo[4];
	Transform worldmatrix;
	Transform modelmatrix;
	OBJECT* parent{ nullptr };

	glm::vec3* vertex;
	glm::vec3* face;

	glm::vec3* vertexdata;
	glm::vec3* normaldata;
	glm::vec4* colordata;
	glm::vec3* texturedata;

	int v_count = 0;
	int f_count = 0;

	int vertex_count = f_count * 3;

	void ReadObj(string fileName)
	{
		ifstream in{ fileName };

		string s;

		while (in >> s)
		{
			if (s == "v") v_count++;
			else if (s == "f") f_count++;
		}
		in.close();
		in.open(fileName);

		vertex_count = f_count * 3;

		vertex = new glm::vec3[v_count];
		face = new glm::vec3[f_count];
		vertexdata = new glm::vec3[vertex_count];
		normaldata = new glm::vec3[vertex_count];
		colordata = new glm::vec4[vertex_count];
		texturedata = new glm::vec3[vertex_count];

		int v_incount = 0;
		int f_incount = 0;

		while (in >> s)
		{
			if (s == "v") {
				in >> vertex[v_incount].x >> vertex[v_incount].y >> vertex[v_incount].z;
				v_incount++;
			}
			else if (s == "f") {
				in >> face[f_incount].x >> face[f_incount].y >> face[f_incount].z;
				vertexdata[f_incount * 3 + 0] = vertex[static_cast<int>(face[f_incount].x - 1)];
				vertexdata[f_incount * 3 + 1] = vertex[static_cast<int>(face[f_incount].y - 1)];
				vertexdata[f_incount * 3 + 2] = vertex[static_cast<int>(face[f_incount].z - 1)];
				f_incount++;
			}
		}

		for (int i = 0; i < f_count; i++)
		{
			glm::vec3 normal = glm::cross(vertexdata[i * 3 + 1] - vertexdata[i * 3 + 0], vertexdata[i * 3 + 2] - vertexdata[i * 3 + 0]);
			//glm::vec3 normal = glm::vec3(0.0, 1.0, 0.0);
			normaldata[i * 3 + 0] = normal;
			normaldata[i * 3 + 1] = normal;
			normaldata[i * 3 + 2] = normal;
		}
	}

	glm::mat4 GetTransform()
	{
		if (parent)
			return parent->GetTransform() * worldmatrix.GetTransform();
		return worldmatrix.GetTransform();
	}

	glm::mat4 GetmodelTransform()
	{
		return modelmatrix.GetTransform();
	}
};

struct CUBE :OBJECT
{
	double width = 0.25, depth = 0.25, height = 0.25;

	void Init()
	{
		for (int i = 0; i < vertex_count; i++)
		{
			double random_color = dis(gen);

			colordata[i].x = 1.0;
			colordata[i].y = 1.0;
			colordata[i].z = 1.0;
			colordata[i].a = 0.5;
		}
		for (int i = 0; i < vertex_count; i++)
		{
			vertexdata[i] -= glm::vec3(0.5, 0.5, 0.5);
		}
		for (int i = 0; i < 6; i++)
		{
			texturedata[i * 6 + 0] = glm::vec3(0.0, 0.0, 0.0);
			texturedata[i * 6 + 1] = glm::vec3(1.0, 0.0, 0.0);
			texturedata[i * 6 + 2] = glm::vec3(1.0, 1.0, 0.0);
			texturedata[i * 6 + 3] = glm::vec3(0.0, 0.0, 0.0);
			texturedata[i * 6 + 4] = glm::vec3(1.0, 1.0, 0.0);
			texturedata[i * 6 + 5] = glm::vec3(0.0, 1.0, 0.0);
		}

		glGenVertexArrays(1, &vao); //--- VAO 를 지정하고 할당하기
		glBindVertexArray(vao); //--- VAO를 바인드하기
		glGenBuffers(4, vbo); //--- 3개의 VBO를 지정하고 할당하기

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), vertexdata, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec4), colordata, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), normaldata, GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), texturedata, GL_STATIC_DRAW);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(3);
	}

	void draw(int shaderID, int num)
	{
		unsigned int modelLocation = glGetUniformLocation(shaderID, "model");
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(GetTransform() * GetmodelTransform()));
		glBindVertexArray(vao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textures[num]); //--- texture[0]을 사용하여 폴리곤을 그린다.
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindTexture(GL_TEXTURE_2D, textures[num]); //--- texture[0]을 사용하여 폴리곤을 그린다.
		glDrawArrays(GL_TRIANGLES, 6, 6);
		glBindTexture(GL_TEXTURE_2D, textures[num]); //--- texture[0]을 사용하여 폴리곤을 그린다.
		glDrawArrays(GL_TRIANGLES, 12, 6);
		glBindTexture(GL_TEXTURE_2D, textures[num]); //--- texture[0]을 사용하여 폴리곤을 그린다.
		glDrawArrays(GL_TRIANGLES, 18, 6);
		glBindTexture(GL_TEXTURE_2D, textures[num]); //--- texture[0]을 사용하여 폴리곤을 그린다.
		glDrawArrays(GL_TRIANGLES, 24, 6);
		glBindTexture(GL_TEXTURE_2D, textures[num]); //--- texture[0]을 사용하여 폴리곤을 그린다.
		glDrawArrays(GL_TRIANGLES, 30, 6);
	}

	void update()
	{
		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), vertexdata, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);
	}
};
CUBE cube;
CUBE skybox;
CUBE minicube;
CUBE checkpoint[7];
CUBE rotatePlane[5];
double theta = 0;
double W, H;
CUBE onoffPlane[51];
int onoffPlaneNum = 51;
int onoffPlaneTime[51];
bool onoff[51];
CUBE glassPlane[18];
int glassPlaneNum = 18;
int glassrandom[18];
CUBE punchbox[4];
CUBE punch[4];
CUBE punchPlane;
int punchNum = 4;
int punchDirection[4];
double punchMoveCnt[4];
CUBE trampoline[3];
int trampolineNum = 3;
CUBE jumpmapcube[10];
int jumpmapcubeNum = 10;
CUBE startTitle;
CUBE endTitle;

struct SPHERE :OBJECT
{
	void Init()
	{
		for (int i = 0; i < vertex_count; i++)
		{
			double random_color = dis(gen);

			colordata[i].x = 1.0;
			colordata[i].y = 1.0;
			colordata[i].z = 1.0;
			colordata[i].a = 0.5;
		}
		glGenVertexArrays(1, &vao); //--- VAO 를 지정하고 할당하기
		glBindVertexArray(vao); //--- VAO를 바인드하기
		glGenBuffers(3, vbo); //--- 3개의 VBO를 지정하고 할당하기

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), vertexdata, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec4), colordata, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), normaldata, GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);

		/*glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), texturedata, GL_STATIC_DRAW);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(3);*/
	}

	void draw(int shaderID)
	{
		unsigned int modelLocation = glGetUniformLocation(shaderID, "model");
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(GetTransform() * GetmodelTransform()));
		glBindVertexArray(vao);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textures[2]);
		glDrawArrays(GL_TRIANGLES, 0, vertex_count);
	}

	void update(glm::vec3 color)
	{
		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), colordata, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);
	}
};
SPHERE sphere;
SPHERE axis;

struct PYRAMID :OBJECT
{
	void Init()
	{
		for (int i = 0; i < vertex_count; i++)
		{
			colordata[i].x = 1.0;
			colordata[i].y = 1.0;
			colordata[i].z = 1.0;
			colordata[i].a = 0.5;
		}
		for (int i = 0; i < vertex_count; i++)
		{
			vertexdata[i] -= glm::vec3(0.5, 0.5, 0.5);
		}
		for (int i = 0; i < 4; i++)
		{
			texturedata[i * 3 + 0] = glm::vec3(0.0, 0.0, 0.0);
			texturedata[i * 3 + 1] = glm::vec3(1.0, 0.0, 0.0);
			texturedata[i * 3 + 2] = glm::vec3(0.5, 1.0, 0.0);
		}
		texturedata[12] = glm::vec3(0.0, 0.0, 0.0);
		texturedata[13] = glm::vec3(1.0, 0.0, 0.0);
		texturedata[14] = glm::vec3(1.0, 1.0, 0.0);
		texturedata[15] = glm::vec3(0.0, 0.0, 0.0);
		texturedata[16] = glm::vec3(1.0, 1.0, 0.0);
		texturedata[17] = glm::vec3(0.0, 1.0, 0.0);

		glGenVertexArrays(1, &vao); //--- VAO 를 지정하고 할당하기
		glBindVertexArray(vao); //--- VAO를 바인드하기
		glGenBuffers(4, vbo); //--- 4개의 VBO를 지정하고 할당하기

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), vertexdata, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec4), colordata, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), normaldata, GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), texturedata, GL_STATIC_DRAW);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(3);
	}

	void draw(int shaderID)
	{
		unsigned int modelLocation = glGetUniformLocation(shaderID, "model");
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(GetTransform() * GetmodelTransform()));
		glBindVertexArray(vao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textures[0]); //--- texture[0]을 사용하여 폴리곤을 그린다.
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindTexture(GL_TEXTURE_2D, textures[0]); //--- texture[0]을 사용하여 폴리곤을 그린다.
		glDrawArrays(GL_TRIANGLES, 3, 3);
		glBindTexture(GL_TEXTURE_2D, textures[0]); //--- texture[0]을 사용하여 폴리곤을 그린다.
		glDrawArrays(GL_TRIANGLES, 6, 3);
		glBindTexture(GL_TEXTURE_2D, textures[0]); //--- texture[0]을 사용하여 폴리곤을 그린다.
		glDrawArrays(GL_TRIANGLES, 9, 3);
		glBindTexture(GL_TEXTURE_2D, textures[0]); //--- texture[0]을 사용하여 폴리곤을 그린다.
		glDrawArrays(GL_TRIANGLES, 12, 3);
		glBindTexture(GL_TEXTURE_2D, textures[0]); //--- texture[0]을 사용하여 폴리곤을 그린다.
		glDrawArrays(GL_TRIANGLES, 15, 3);
	}

	void update()
	{
		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), vertexdata, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);
	}
};
PYRAMID pyramid;

GLfloat lineShape[10][2][3] = {};	//--- 선분 위치 값

glm::vec3 colors[12][3] = {};

GLfloat XYZShape[3][2][3] = {
	{{-1.0,0.0,0.0},{1.0,0.0,0.0}},
	{{0.0,-1.0,0.0},{0.0,1.0,0.0}},
	{{0.0,0.0,-1.0},{0.0,0.0,1.0}} };

GLfloat XYZcolors[6][3] = { //--- 축 색상
	{ 1.0, 0.0, 0.0 },	   	{ 1.0, 0.0, 0.0 },
	{ 0.0, 1.0, 0.0 },	   	{ 0.0, 1.0, 0.0 },
	{ 0.0, 0.0, 1.0 },	   	{ 0.0, 0.0, 1.0 }
};

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -4.0f); //--- 카메라 위치
glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, 200.0f); //--- 카메라 바라보는 방향
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f); //--- 카메라 위쪽 방향

glm::mat4 model = glm::mat4(1.0f);
glm::mat4 view = glm::mat4(1.0f);
glm::mat4 projection = glm::mat4(1.0f);

glm::vec3 cameraPos3 = glm::vec3(0.0f, -0.5f, 0.0f); //--- 카메라 위치
glm::vec3 cameraDirection3 = glm::vec3(0.0f, 0.0f, 0.0f); //--- 카메라 바라보는 방향
glm::vec3 cameraUp3 = glm::vec3(0.0f, 0.0f, 1.0f); //--- 카메라 위쪽 방향
glm::mat4 view3 = glm::mat4(1.0f);
glm::mat4 pTransform3 = glm::mat4(1.0f);

GLuint vao, vbo[3];
GLuint TriPosVbo, TriColorVbo;

GLchar* vertexSource, * fragmentSource; //--- 소스코드 저장 변수
GLuint vertexShader, fragmentShader; //--- 세이더 객체
GLuint shaderProgramID; //--- 셰이더 프로그램

int windowWidth = 800;
int windowHeight = 800;

float openGLX, openGLY;
int movingRectangle = -1;

float ox = 0, oy = 0;
float x_angle = 0;
float y_angle = 0;
float z_angle = 0;
float pre_x_angle = 0;
float pre_y_angle = 0;
float wheel_scale = 0.15;
bool left_button = 0;
float fovy = 45;
float near_1 = 0.1;
float far_1 = 200.0;
float persfect_z = -2.0;

int movingMouse = -1;
float beforeX, beforeY;
float cameraDistance = 15.0f; // 구와의 거리
float cameraHeight = 3.0f; // 카메라의 높이
float cameraAngle = 180.0f; // 카메라 각도

bool start = true;
bool endpoint = false;

float w, a, s, d;
float speed = 0.3;
int JSelection = 0;
int JCnt = 0;
float jumpSize = 0.1;
const float gravity = 0.01f; // 중력 가속도
const float initialHeight = 0.5f; // 초기 높이
const float jumpInitialVelocity = 0.3f; // 초기 점프 속도
float jumpVelocity = jumpInitialVelocity; // 점프 속도를 변화시킬 변수
bool upKeyPressed = false;
bool downKeyPressed = false;
bool leftKeyPressed = false;
bool rightKeyPressed = false;
bool falling = false;
int checknum = 0;

bool adminmode = false;

//카메라
bool viewpoint = false;	//false : 3인칭, true : 1인칭

void make_shaderProgram();
void make_vertexShaders();
void make_fragmentShaders();
GLvoid drawScene();
GLvoid Reshape(int w, int h);
GLvoid Keyboard(unsigned char key, int x, int y);
void InitBuffer();
char* filetobuf(const char*);
GLvoid Mouse(int button, int state, int x, int y);
GLvoid WindowToOpenGL(int mouseX, int mouseY, float& x, float& y);
GLvoid Motion(int x, int y);
GLvoid TimerFunction(int value);
GLvoid SpecialKeys(int key, int x, int y);
GLvoid mouseWheel(int button, int dir, int x, int y);
GLvoid SpecialKeysUp(int key, int x, int y);

void main(int argc, char** argv) //--- 윈도우 출력하고 콜백함수 설정
{
	//--- 윈도우 생성하기
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("Example1");

	//--- GLEW 초기화하기
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Unable to initialize GLEW" << std::endl;
		exit(EXIT_FAILURE);
	}
	else
	{
		std::cout << "GLEW Initialized\n";
	}
	cube.ReadObj("cube.obj");
	skybox.ReadObj("cube.obj");
	minicube.ReadObj("cube.obj");
	sphere.ReadObj("sphere.obj");
	axis.ReadObj("sphere.obj");
	for (int i = 0; i < 7; i++)
	{
		checkpoint[i].ReadObj("cube.obj");
	}
	for (int i = 0; i < 5; i++)
	{
		rotatePlane[i].ReadObj("cube.obj");
	}
	for (int i = 0; i < onoffPlaneNum; i++)
	{
		onoffPlane[i].ReadObj("cube.obj");
	}
	for (int i = 0; i < glassPlaneNum; i++)
	{
		glassPlane[i].ReadObj("cube.obj");
	}
	for (int i = 0; i < punchNum; i++)
	{
		punchbox[i].ReadObj("cube.obj");
	}
	for (int i = 0; i < punchNum; i++)
	{
		punch[i].ReadObj("cube.obj");
	}
	punchPlane.ReadObj("cube.obj");
	for (int i = 0; i < trampolineNum; i++)
	{
		trampoline[i].ReadObj("cube.obj");
	}
	for (int i = 0; i < jumpmapcubeNum; i++)
	{
		jumpmapcube[i].ReadObj("cube.obj");
	}
	startTitle.ReadObj("cube.obj");
	endTitle.ReadObj("cube.obj");

	//--- 세이더 읽어와서 세이더 프로그램 만들기
	make_shaderProgram(); //--- 세이더 프로그램 만들기
	InitBuffer();
	InitTexture();
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE); //--- 상태 설정은 필요한 곳에서 하면 된다.
	//glDisable(GL_DEPTH_TEST | GL_CULL_FACE);	//해제

	glutTimerFunc(10, TimerFunction, 1);
	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutSpecialFunc(SpecialKeys); // 방향키 콜백 함수 등록
	glutSpecialUpFunc(SpecialKeysUp); // 키 떼는 이벤트 처리 추가
	//glutMouseFunc(Mouse);
	glutMotionFunc(Motion);
	glutMouseWheelFunc(mouseWheel);

	glutMainLoop();
}

GLvoid drawScene()
{
	glUseProgram(shaderProgramID);
	glClearColor(1.0, 1.0, 1.0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //--- 깊이 버퍼를 클리어한다.

	glBindVertexArray(vao);

	// 색상 바꾸기
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), XYZcolors, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	int modelLocation = glGetUniformLocation(shaderProgramID, "model"); //--- 버텍스 세이더에서 모델링 변환 행렬 변수값을 받아온다.
	int viewLocation = glGetUniformLocation(shaderProgramID, "view"); //--- 버텍스 세이더에서 뷰잉 변환 행렬 변수값을 받아온다.
	int projLocation = glGetUniformLocation(shaderProgramID, "projection"); //--- 버텍스 세이더에서 투영 변환 행렬 변수값을 받아온다.

	/*projection = glm::mat4(1.0f);
	projection = glm::scale(projection, glm::vec3(wheel_scale, wheel_scale, wheel_scale));
	projection = glm::rotate(projection, (float)glm::radians(x_angle + 30), glm::vec3(1.0, 0.0, 0.0));
	projection = glm::rotate(projection, (float)glm::radians(y_angle - 30), glm::vec3(0.0, 1.0, 0.0));

	unsigned int cameraLocation = glGetUniformLocation(shaderProgramID, "view");
	glUniformMatrix4fv(cameraLocation, 1, GL_FALSE, glm::value_ptr(projection));*/

	view = glm::lookAt(cameraPos, cameraDirection, cameraUp);
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

	glm::mat4 perspect = glm::mat4(1.0f);
	perspect = glm::perspective(glm::radians(fovy), (float)windowWidth / (float)windowHeight, near_1, far_1);
	perspect = glm::translate(perspect, glm::vec3(0.0, 0.0, persfect_z));
	unsigned int projectionLocation = glGetUniformLocation(shaderProgramID, "projection");
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(perspect));

	glm::mat4 lightmatrix = minicube.GetTransform(); // 주어진 mat4 행렬
	glm::vec3 lightposition = glm::vec3(lightmatrix[3]); // 행렬의 마지막 열을 사용하여 위치 추출

	unsigned int lightPosLocation = glGetUniformLocation(shaderProgramID, "lightPos"); //--- lightPos 값 전달: (0.0, 0.0, 5.0);
	glUniform3f(lightPosLocation, lightposition.x, lightposition.y, lightposition.z);
	unsigned int lightColorLocation = glGetUniformLocation(shaderProgramID, "lightColor"); //--- lightColor 값 전달: (1.0, 1.0, 1.0) 백색
	glUniform3f(lightColorLocation, 1.0, 1.0, 1.0);
	unsigned int objColorLocation = glGetUniformLocation(shaderProgramID, "objectColor"); //--- object Color값 전달: (1.0, 0.5, 0.3)의 색
	glUniform3f(objColorLocation, 1.0, 0.5, 0.3);

	//s r t p 코드 작성시에는 반대 방향으로.
	model = glm::mat4(1.0f);
	if (!viewpoint)
	{
		sphere.draw(shaderProgramID);
	}
	//minicube.draw(shaderProgramID,1);
	skybox.draw(shaderProgramID, 1);
	for (int i = 0; i < 7; i++)
	{
		checkpoint[i].draw(shaderProgramID, 2);
	}
	for (int i = 0; i < 5; i++)
	{
		rotatePlane[i].draw(shaderProgramID, 3);
	}
	for (int i = 0; i < onoffPlaneNum; i++)
	{
		if (onoff[i])
		{
			onoffPlane[i].draw(shaderProgramID, 4);
		}
	}

	for (int i = 0; i < punchNum; i++)
	{
		punchbox[i].draw(shaderProgramID, 6);
		punch[i].draw(shaderProgramID, 6);
	}
	punchPlane.draw(shaderProgramID, 6);
	for (int i = 0; i < trampolineNum; i++)
	{
		trampoline[i].draw(shaderProgramID, 7);
	}
	for (int i = 0; i < jumpmapcubeNum; i++)
	{
		jumpmapcube[i].draw(shaderProgramID, 2);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//투명도 그리고 싶으면 여기에 객체 그리기
	for (int i = 0; i < glassPlaneNum; i++)
	{
		if (glassrandom[i] >= 0)
		{
			glassPlane[i].draw(shaderProgramID, 5);
		}
	}
	//cube.draw(shaderProgramID, 1);
	glDisable(GL_BLEND);

	//시작화면
	if (start)
	{
		glViewport(0, 0, windowWidth, windowHeight);
		pTransform3 = glm::mat4(1.0f);
		pTransform3 = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		glUniformMatrix4fv(projLocation, 1, GL_FALSE, &pTransform3[0][0]);
		view3 = glm::lookAt(cameraPos3, cameraDirection3, cameraUp3);
		glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view3[0][0]);
		startTitle.draw(shaderProgramID, 8);
	}
	if (endpoint)
	{
		glViewport(0, 0, windowWidth, windowHeight);
		pTransform3 = glm::mat4(1.0f);
		pTransform3 = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		glUniformMatrix4fv(projLocation, 1, GL_FALSE, &pTransform3[0][0]);
		view3 = glm::lookAt(cameraPos3, cameraDirection3, cameraUp3);
		glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view3[0][0]);
		endTitle.draw(shaderProgramID, 9);
	}


	glutSwapBuffers(); //--- 화면에 출력하기
}

//--- 다시그리기 콜백 함수
GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
}

void InitBuffer()
{
	glGenVertexArrays(1, &vao); //--- VAO 를 지정하고 할당하기
	glBindVertexArray(vao); //--- VAO를 바인드하기
	glGenBuffers(2, vbo); //--- 2개의 VBO를 지정하고 할당하기

	result = FMOD::System_Create(&ssystem); //--- 사운드 시스템 생성
	if (result != FMOD_OK)
		exit(0);
	ssystem->init(32, FMOD_INIT_NORMAL, extradriverdata); //--- 사운드 시스템 초기화
	ssystem->createSound("sound/realbackground_bgm.mp3", FMOD_LOOP_NORMAL, 0, &background); //--- 1번 사운드 생성 및 설정
	ssystem->createSound("sound/jump_bgm.wav", FMOD_LOOP_OFF, 0, &jump);
	ssystem->createSound("sound/hit_bgm.wav", FMOD_LOOP_OFF, 0, &hit);
	ssystem->createSound("sound/glassbroken_bgm.wav", FMOD_LOOP_OFF, 0, &glassbroken);
	ssystem->createSound("sound/trampoline_bgm.wav", FMOD_LOOP_OFF, 0, &tramjump);
	ssystem->createSound("sound/gameclear_bgm.wav", FMOD_LOOP_OFF, 0, &gameclear);
	bgm_channel->setVolume(0.000001);
	effect_channel->setVolume(1);
	end_channel->setVolume(1);
	ssystem->playSound(background, 0, false, &bgm_channel);
	cube.Init();
	minicube.Init();
	//minicube.parent = &cube;
	sphere.Init();
	axis.Init();
	//sphere.parent = &axis;
	skybox.Init();
	for (int i = 0; i < 7; i++)
	{
		checkpoint[i].Init();
		checkpoint[i].worldmatrix.position.y -= 0.5;
		checkpoint[i].worldmatrix.position.z = i * 100;
		checkpoint[i].worldmatrix.scale = glm::vec3(7.0, 0.3, 7.0);
		checkpoint[i].width = 7.0 / 2;
		checkpoint[i].depth = 0.3 / 2;
		checkpoint[i].height = 7.0 / 2;
	}
	for (int i = 0; i < 5; i++)
	{
		rotatePlane[i].Init();
		rotatePlane[i].worldmatrix.position.y -= 0.5;
		rotatePlane[i].worldmatrix.scale = glm::vec3(10.0, 0.3, 10.0);
		rotatePlane[i].width = 10.0 / 2;
		rotatePlane[i].depth = 0.3 / 2;
		rotatePlane[i].height = 10.0 / 2;
	}
	rotatePlane[0].worldmatrix.position.x = -5;
	rotatePlane[1].worldmatrix.position.x = 0;
	rotatePlane[2].worldmatrix.position.x = 5;
	rotatePlane[3].worldmatrix.position.x = -5;
	rotatePlane[4].worldmatrix.position.x = 0;
	rotatePlane[0].worldmatrix.position.z = 15;
	rotatePlane[1].worldmatrix.position.z = 35;
	rotatePlane[2].worldmatrix.position.z = 55;
	rotatePlane[3].worldmatrix.position.z = 75;
	rotatePlane[4].worldmatrix.position.z = 90;
	rotatePlane[4].worldmatrix.scale = glm::vec3(6.0, 0.3, 6.0);
	rotatePlane[4].width = 6.0 / 2;
	rotatePlane[4].depth = 0.3 / 2;
	rotatePlane[4].height = 6.0 / 2;

	for (int i = 0; i < onoffPlaneNum; i++)
	{
		onoffPlane[i].Init();
		onoffPlane[i].worldmatrix.position.y -= 0.5;
		onoffPlane[i].worldmatrix.position.z = (i % 17) * 5 + 110;
		if (i >= 0 && i < 17)
		{
			onoffPlane[i].worldmatrix.position.x = -5;
		}
		else if (i >= 17 && i < 34)
		{
			onoffPlane[i].worldmatrix.position.x = 0;
		}
		else if (i >= 34 && i < 51)
		{
			onoffPlane[i].worldmatrix.position.x = 5;
		}
		onoffPlane[i].worldmatrix.scale = glm::vec3(5.0, 0.3, 5.0);
		onoffPlane[i].width = 5.0 / 2;
		onoffPlane[i].depth = 0.3 / 2;
		onoffPlane[i].height = 5.0 / 2;
		onoffPlaneTime[i] = onoffrandom(gen);
		onoff[i] = true;
	}
	for (int i = 0; i < glassPlaneNum; i++)
	{
		glassPlane[i].Init();
		glassPlane[i].worldmatrix.position.y -= 0.5;
		glassPlane[i].worldmatrix.position.z = (i % 9) * 10 + 210;
		if (i >= 0 && i < 9)
		{
			glassPlane[i].worldmatrix.position.x = -5;
		}
		else if (i >= 9 && i < 18)
		{
			glassPlane[i].worldmatrix.position.x = 5;
		}
		glassPlane[i].worldmatrix.scale = glm::vec3(10.0, 0.3, 10.0);
		glassPlane[i].width = 10.0 / 2;
		glassPlane[i].depth = 0.3 / 2;
		glassPlane[i].height = 10.0 / 2;
		if (i < 9)
		{
			glassrandom[i] = glass(gen);
			if (glassrandom[i] == 0)
			{
				glassrandom[i + 9] = 1;
			}
			if (glassrandom[i] == 1)
			{
				glassrandom[i + 9] = 0;
			}
		}
	}
	for (int i = 0; i < punchNum; i++)
	{
		punchbox[i].Init();
		punchbox[i].worldmatrix.position.y -= 0.5;
		punchbox[i].worldmatrix.position.z = i * 23 + 320;
		if (i % 2 == 0)
		{
			punchbox[i].worldmatrix.position.x = -20;
			punchDirection[i] = 0;
		}
		else if (i % 2 == 1)
		{
			punchbox[i].worldmatrix.position.x = 20;
			punchDirection[i] = 1;
		}
		punchbox[i].worldmatrix.scale = glm::vec3(20.0, 20.0, 20.0);
		punchbox[i].width = 20.0 / 2;
		punchbox[i].depth = 20.0 / 2;
		punchbox[i].height = 20.0 / 2;

		punch[i].Init();
		punch[i].worldmatrix.position = punchbox[i].worldmatrix.position;
		punch[i].worldmatrix.position.y += 4;
		punch[i].worldmatrix.scale = glm::vec3(10.0, 10.0, 10.0);
		punch[i].width = 10.0 / 2;
		punch[i].depth = 10.0 / 2;
		punch[i].height = 10.0 / 2;

		punchMoveCnt[i] = 0.2;
	}
	punchPlane.Init();
	punchPlane.worldmatrix.position.y -= 0.6;
	punchPlane.worldmatrix.position.z = 350;
	punchPlane.worldmatrix.scale = glm::vec3(7.0, 0.2, 100.0);
	punchPlane.width = 7.0 / 2;
	punchPlane.depth = 0.3 / 2;
	punchPlane.height = 100.0 / 2;
	for (int i = 0; i < trampolineNum; i++)
	{
		trampoline[i].Init();
		trampoline[i].worldmatrix.position.y -= 0.5;
		trampoline[i].worldmatrix.position.z = i * 25 + 410;
		trampoline[i].worldmatrix.position.y += i * 5;
		trampoline[i].worldmatrix.scale = glm::vec3(5.0, 5.0, 5.0);
		trampoline[i].width = 5.0 / 2;
		trampoline[i].depth = 5.0 / 2;
		trampoline[i].height = 5.0 / 2;
	}
	for (int i = 0; i < jumpmapcubeNum; i++)
	{
		jumpmapcube[i].Init();
		jumpmapcube[i].worldmatrix.position.y -= 0.5;
		jumpmapcube[i].worldmatrix.position.z = 505 + i * 10;
		jumpmapcube[i].worldmatrix.scale = glm::vec3(11 - i, 0.3, (15 - i) / 2);
		jumpmapcube[i].width = (11 - i) / 2;
		jumpmapcube[i].depth = 0.3 / 2;
		jumpmapcube[i].height = ((12 - i) / 2) / 2;
	}

	startTitle.Init();
	startTitle.worldmatrix.scale = glm::vec3(2, 2, 2);
	endTitle.Init();
	endTitle.worldmatrix.scale = glm::vec3(2, 2, 2);

	sphere.worldmatrix.scale = glm::vec3(0.5, 0.5, 0.5);
	axis.modelmatrix.scale = glm::vec3(0.5, 0.5, 0.5);

	skybox.worldmatrix.scale = glm::vec3(200.0, 200.0, 200.0);

	cube.worldmatrix.position.z = 10;

	minicube.worldmatrix.position.z = -7;
	minicube.modelmatrix.scale = glm::vec3(0.35, 0.35, 0.35);
}

void make_shaderProgram()
{
	make_vertexShaders(); //--- 버텍스 세이더 만들기
	make_fragmentShaders(); //--- 프래그먼트 세이더 만들기
	//-- shader Program
	shaderProgramID = glCreateProgram();
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	glLinkProgram(shaderProgramID);
	//--- 세이더 삭제하기
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	//--- Shader Program 사용하기
	glUseProgram(shaderProgramID);
}

void make_vertexShaders()
{
	vertexSource = filetobuf("vertex5.glsl");
	//--- 버텍스 세이더 객체 만들기
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	//--- 세이더 코드를 세이더 객체에 넣기
	glShaderSource(vertexShader, 1, (const GLchar**)&vertexSource, 0);
	//--- 버텍스 세이더 컴파일하기
	glCompileShader(vertexShader);
	//--- 컴파일이 제대로 되지 않은 경우: 에러 체크
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		std::cout << "ERROR: vertex shader 컴파일 실패\n" << errorLog << std::endl;
		return;
	}
}

void make_fragmentShaders()
{
	fragmentSource = filetobuf("fragment5.glsl");
	//--- 프래그먼트 세이더 객체 만들기
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	//--- 세이더 코드를 세이더 객체에 넣기
	glShaderSource(fragmentShader, 1, (const GLchar**)&fragmentSource, 0);
	//--- 프래그먼트 세이더 컴파일
	glCompileShader(fragmentShader);
	//--- 컴파일이 제대로 되지 않은 경우: 컴파일 에러 체크
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		std::cout << "ERROR: fragment shader 컴파일 실패\n" << errorLog << std::endl;
		return;
	}
}

char* filetobuf(const char* file)
{
	FILE* fptr;
	long length;
	char* buf;
	fptr = fopen(file, "rb"); // Open file for reading 
	if (!fptr) // Return NULL on failure 
		return NULL;
	fseek(fptr, 0, SEEK_END); // Seek to the end of the file 
	length = ftell(fptr); // Find out how many bytes into the file we are 
	buf = (char*)malloc(length + 1); // Allocate a buffer for the entire length of the file and a null terminator 
	fseek(fptr, 0, SEEK_SET); // Go back to the beginning of the file 
	fread(buf, length, 1, fptr); // Read the contents of the file in to the buffer 
	fclose(fptr); // Close the file 
	buf[length] = 0; // Null terminator 
	return buf; // Return the buffer 
}

//위 z+ 왼 x+

GLvoid Keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
		checknum = key - '1' + 1;
		sphere.worldmatrix.position.z = 100 * checknum;
		cameraDirection.z = 1000;
		break;
	case 'a':
		adminmode = !adminmode;
		break;
	case 's':
		start = false;
		break;
	case 32:
		if (JSelection == 0)
		{
			ssystem->playSound(jump, 0, false, &effect_channel);
			JSelection = 1;
		}
		break;
	case 'v':
		viewpoint = !viewpoint;
		break;
	case 'q':
		glutLeaveMainLoop();
		break;
	}
	glutPostRedisplay(); //--- 배경색이 바뀔 때마다 출력 콜백 함수를 호출하여 화면을 refresh 한다
}

bool spacePressed = true;

GLvoid SpecialKeys(int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_UP:
		upKeyPressed = true;
		break;
	case GLUT_KEY_DOWN:
		downKeyPressed = true;
		break;
	case GLUT_KEY_LEFT:
		leftKeyPressed = true;
		break;
	case GLUT_KEY_RIGHT:
		rightKeyPressed = true;
		break;
	case 32:
		spacePressed = true;
		break;
	}
	glutPostRedisplay(); // 화면 갱신
}

GLvoid SpecialKeysUp(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_UP:
		upKeyPressed = false;
		break;
	case GLUT_KEY_DOWN:
		downKeyPressed = false;
		break;
	case GLUT_KEY_LEFT:
		leftKeyPressed = false;
		break;
	case GLUT_KEY_RIGHT:
		rightKeyPressed = false;
		break;
	case 32:
		spacePressed = false;
		break;
	}
}

void moveSphere()
{
	if (upKeyPressed)
	{
		sphere.worldmatrix.position.z += speed;
		sphere.modelmatrix.rotation.x += speed * 50;
		cameraDirection.z += speed;
	}
	if (downKeyPressed)
	{
		sphere.worldmatrix.position.z -= speed;
		sphere.modelmatrix.rotation.x -= speed * 50;
		cameraDirection.z -= speed;
	}
	if (leftKeyPressed)
	{
		sphere.worldmatrix.position.x += speed;
		sphere.modelmatrix.rotation.z -= speed * 50;
	}
	if (rightKeyPressed)
	{
		sphere.worldmatrix.position.x -= speed;
		sphere.modelmatrix.rotation.z += speed * 50;
	}
}

GLvoid Mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		ox = x;
		oy = y;
		left_button = true;
	}
	else
	{
		ox = 0;
		oy = 0;
		pre_x_angle = x_angle;
		pre_y_angle = y_angle;
		left_button = false;
	}
}

GLvoid Motion(int x, int y)
{
	if (left_button)
	{
		y_angle = x - ox;
		x_angle = y - oy;
		x_angle += pre_x_angle;
		y_angle += pre_y_angle;

		y_angle /= 2;
		x_angle /= 2;
	}
	glutPostRedisplay();
}


GLvoid mouseWheel(int button, int dir, int x, int y)
{
	if (dir > 0)
	{
		wheel_scale += dir * 0.1;
	}
	else if (dir < 0)
	{
		wheel_scale += dir * 0.1;
		if (wheel_scale < 0.1)
		{
			wheel_scale = 0.1;
		}
	}
	glutPostRedisplay();
}

GLvoid WindowToOpenGL(int mouseX, int mouseY, float& x, float& y)
{
	x = (2.0f * mouseX) / windowWidth - 1.0f;
	y = 1.0f - (2.0f * mouseY) / windowHeight;
}

glm::vec3 prevSpherePosition = sphere.worldmatrix.position;

float fall = 0;

void collision()
{

}
float angles[5]{};
glm::vec3 destination;

GLvoid TimerFunction(int value)
{
	switch (value)
	{
	case 1:
		if (!start && !endpoint)
		{
			sphere.worldmatrix.scale = glm::vec3(1, 1, 1);
			if (!falling || adminmode)
			{
				moveSphere();
			}

			if (JSelection == 1)
			{
				sphere.worldmatrix.position.y += jumpVelocity; // 구에 점프 속도 적용
				//sphere.worldmatrix.scale = glm::vec3(1.0f, 1.0f + jumpVelocity, 1.0f);
				sphere.worldmatrix.scale = glm::vec3(1.0f, 1.0f + jumpVelocity + jumpInitialVelocity / 2, 1.0f);
				// 중력 적용
				jumpVelocity -= gravity;
			}
			// 카메라 위치 조정
			cameraPos.x = sphere.worldmatrix.position.x + cameraDistance * sin(glm::radians(cameraAngle));
			cameraPos.y = sphere.worldmatrix.position.y + cameraHeight + viewpoint * -3.1;
			cameraPos.z = sphere.worldmatrix.position.z + cameraDistance * cos(glm::radians(cameraAngle)) + viewpoint * 17;

			// 땅에 닿았을 때의 처리
			if (sphere.worldmatrix.position.y <= initialHeight) {
				sphere.worldmatrix.position.y = initialHeight;
				jumpVelocity = jumpInitialVelocity; // 다시 초기 점프 속도로 설정
				JSelection = 0;
			}

			//회전하는 판
			for (int i = 0; i < 5; i++)
			{
				if (i % 2 == 0)
				{
					rotatePlane[i].worldmatrix.rotation.y += 1;
					if (!upKeyPressed && !downKeyPressed && !rightKeyPressed && !leftKeyPressed) {
						angles[i] += 1;
					}
					else {
						angles[i] = 0;
					}
				}
				if (i % 2 == 1)
				{
					rotatePlane[i].worldmatrix.rotation.y -= 1;
					if (!upKeyPressed && !downKeyPressed && !rightKeyPressed && !leftKeyPressed) {
						angles[i] -= 1;
					}
					else {
						angles[i] = 0;
					}
				}
			}
			//온-오프 판
			for (int i = 0; i < onoffPlaneNum; i++)
			{
				onoffPlaneTime[i]++;
				if (onoffPlaneTime[i] > 0 && onoffPlaneTime[i] < 200)
				{
					onoff[i] = true;
				}
				else if (onoffPlaneTime[i] > 200 && onoffPlaneTime[i] < 400)
				{
					onoff[i] = false;
				}
				else if (onoffPlaneTime[i] > 400)
				{
					onoffPlaneTime[i] = 0;
				}
			}
			//펀치
			for (int i = 0; i < punchNum; i++)
			{
				if (punchDirection[i] == 0)
				{
					if (i % 2 == 0)
					{
						punch[i].worldmatrix.position.x += punchMoveCnt[i] * 3;
					}
					else if (i % 2 == 1)
					{
						punch[i].worldmatrix.position.x += punchMoveCnt[i];
					}
				}
				else if (punchDirection[i] == 1)
				{
					if (i % 2 == 0)
					{
						punch[i].worldmatrix.position.x -= punchMoveCnt[i];
					}
					else if (i % 2 == 1)
					{
						punch[i].worldmatrix.position.x -= punchMoveCnt[i] * 3;
					}
				}
				if (punch[i].worldmatrix.position.x < -21)
				{
					punchDirection[i] = 0;
				}
				else if (punch[i].worldmatrix.position.x > 21)
				{
					punchDirection[i] = 1;
				}
			}

			falling = true;
			//충돌 체크
			for (int i = 0; i < 7; i++)
			{
				if (((sphere.worldmatrix.position.x > (checkpoint[i].worldmatrix.position.x - checkpoint[i].width))
					&& (sphere.worldmatrix.position.x < (checkpoint[i].worldmatrix.position.x + checkpoint[i].width))
					&& (sphere.worldmatrix.position.z > (checkpoint[i].worldmatrix.position.z - checkpoint[i].height))
					&& (sphere.worldmatrix.position.z < (checkpoint[i].worldmatrix.position.z + checkpoint[i].height)))
					|| JSelection == 1)
				{
					falling = false;
					if (i == 6)
					{
						endpoint = true;
					}
					break;
				}
			}
			for (int i = 0; i < 5; i++)	// rotatePlane
			{
				if (((sphere.worldmatrix.position.x > (rotatePlane[i].worldmatrix.position.x - rotatePlane[i].width))
					&& (sphere.worldmatrix.position.x < (rotatePlane[i].worldmatrix.position.x + rotatePlane[i].width))
					&& (sphere.worldmatrix.position.z > (rotatePlane[i].worldmatrix.position.z - rotatePlane[i].height))
					&& (sphere.worldmatrix.position.z < (rotatePlane[i].worldmatrix.position.z + rotatePlane[i].height)))
					)
				{

					if (!upKeyPressed && !downKeyPressed && !rightKeyPressed && !leftKeyPressed) {
						sphere.worldmatrix.position.x = rotatePlane[i].worldmatrix.position.x + 4 * cos(-angles[i] * 2 * 3.141592 / 180.0);
						sphere.worldmatrix.position.z = rotatePlane[i].worldmatrix.position.z + 4 * sin(-angles[i] * 2 * 3.141592 / 180.0);
					}
					/*else {
						if (upKeyPressed) {
							if (i % 2 == 0) sphere.worldmatrix.position.x -= speed /2;
							else sphere.worldmatrix.position.x += speed / 2;
						}
					}*/
					falling = false;
					break;
				}
			}

			for (int i = 0; i < onoffPlaneNum; i++)	//on-off Plane
			{
				if (((sphere.worldmatrix.position.x > (onoffPlane[i].worldmatrix.position.x - onoffPlane[i].width))
					&& (sphere.worldmatrix.position.x < (onoffPlane[i].worldmatrix.position.x + onoffPlane[i].width))
					&& (sphere.worldmatrix.position.z > (onoffPlane[i].worldmatrix.position.z - onoffPlane[i].height))
					&& (sphere.worldmatrix.position.z < (onoffPlane[i].worldmatrix.position.z + onoffPlane[i].height)))
					|| JSelection == 1)
				{
					if (onoff[i])
					{
						falling = false;
					}
					break;
				}
			}
			for (int i = 0; i < glassPlaneNum; i++)	//glassPlane
			{
				if (((sphere.worldmatrix.position.x > (glassPlane[i].worldmatrix.position.x - glassPlane[i].width))
					&& (sphere.worldmatrix.position.x < (glassPlane[i].worldmatrix.position.x + glassPlane[i].width))
					&& (sphere.worldmatrix.position.z > (glassPlane[i].worldmatrix.position.z - glassPlane[i].height))
					&& (sphere.worldmatrix.position.z < (glassPlane[i].worldmatrix.position.z + glassPlane[i].height)))
					|| JSelection == 1)
				{
					if (glassrandom[i] == 0)
					{
						falling = false;
					}
					else if (glassrandom[i] == 1)
					{
						effect_channel->setVolume(1);
						ssystem->playSound(glassbroken, 0, false, &effect_channel);
						glassrandom[i] = -1;
					}
					break;
				}
			}
			//punch 발판
			if (((sphere.worldmatrix.position.x > (punchPlane.worldmatrix.position.x - punchPlane.width))
				&& (sphere.worldmatrix.position.x < (punchPlane.worldmatrix.position.x + punchPlane.width))
				&& (sphere.worldmatrix.position.z > (punchPlane.worldmatrix.position.z - punchPlane.height))
				&& (sphere.worldmatrix.position.z < (punchPlane.worldmatrix.position.z + punchPlane.height)))
				|| JSelection == 1)
			{
				falling = false;
			}
			for (int i = 0; i < punchNum; i++)	//punch
			{
				if (((sphere.worldmatrix.position.x > (punch[i].worldmatrix.position.x - punch[i].width))
					&& (sphere.worldmatrix.position.x < (punch[i].worldmatrix.position.x + punch[i].width))
					&& (sphere.worldmatrix.position.z > (punch[i].worldmatrix.position.z - punch[i].height))
					&& (sphere.worldmatrix.position.z < (punch[i].worldmatrix.position.z + punch[i].height)))
					)
				{
					ssystem->playSound(hit, 0, false, &effect_channel);
					if (punchDirection[i] == 0)
					{
						sphere.worldmatrix.position.x += punchMoveCnt[i] * 2;
					}
					else if (punchDirection[i] == 1)
					{
						sphere.worldmatrix.position.x -= punchMoveCnt[i] * 2;
					}
					break;
				}
			}
			//점프대
			for (int i = 0; i < trampolineNum; i++)	//punch
			{
				if ((sphere.worldmatrix.position.x > (trampoline[i].worldmatrix.position.x - trampoline[i].width))
					&& (sphere.worldmatrix.position.x < (trampoline[i].worldmatrix.position.x + trampoline[i].width))
					&& (sphere.worldmatrix.position.z > (trampoline[i].worldmatrix.position.z - trampoline[i].height))
					&& (sphere.worldmatrix.position.z < (trampoline[i].worldmatrix.position.z + trampoline[i].height))
					&& (sphere.worldmatrix.position.y > (trampoline[i].worldmatrix.position.y - trampoline[i].depth))
					&& (sphere.worldmatrix.position.y < (trampoline[i].worldmatrix.position.y + trampoline[i].depth)))
				{
					ssystem->playSound(tramjump, 0, false, &effect_channel);

					falling = false;
					jumpVelocity = 0.6;
					break;
				}
			}
			//피날레
			for (int i = 0; i < jumpmapcubeNum; i++)	//glassPlane
			{
				if (((sphere.worldmatrix.position.x > (jumpmapcube[i].worldmatrix.position.x - jumpmapcube[i].width))
					&& (sphere.worldmatrix.position.x < (jumpmapcube[i].worldmatrix.position.x + jumpmapcube[i].width))
					&& (sphere.worldmatrix.position.z > (jumpmapcube[i].worldmatrix.position.z - jumpmapcube[i].height))
					&& (sphere.worldmatrix.position.z < (jumpmapcube[i].worldmatrix.position.z + jumpmapcube[i].height)))
					|| JSelection == 1)
				{
					falling = false;
					break;
				}
			}


			if (sphere.worldmatrix.position.z >= 100 && sphere.worldmatrix.position.z < 200)
			{
				checknum = 1;
			}
			else if (sphere.worldmatrix.position.z >= 200 && sphere.worldmatrix.position.z < 300)
			{
				checknum = 2;
			}
			else if (sphere.worldmatrix.position.z >= 300 && sphere.worldmatrix.position.z < 400)
			{
				checknum = 3;
			}
			else if (sphere.worldmatrix.position.z >= 400 && sphere.worldmatrix.position.z < 500)
			{
				checknum = 4;
			}
			else if (sphere.worldmatrix.position.z >= 500 && sphere.worldmatrix.position.z < 600)
			{
				checknum = 5;
			}
			else if (sphere.worldmatrix.position.z >= 600 && sphere.worldmatrix.position.z < 700)
			{
				checknum = 6;
				bgm_channel->stop();
				effect_channel->stop();
				ssystem->playSound(gameclear, 0, false, &end_channel);
			}


			//테스트 확인용
			/*checknum = 6;
			cameraDirection.z = 1000;*/

			//추락하기
			if (falling && !adminmode)
			{
				fall += 0.3;
				sphere.worldmatrix.position.y -= fall;
				if (sphere.worldmatrix.position.y < -10)
				{
					sphere.worldmatrix.position = checkpoint[checknum].worldmatrix.position;
					falling = false;
					fall = 0;
				}
			}

			//큐브맵 같이 이동
			skybox.worldmatrix.position = sphere.worldmatrix.position;
			skybox.worldmatrix.position.z += 50;
		}


		break;
	}
	glutPostRedisplay();
	glutTimerFunc(10, TimerFunction, 1);
}

//update() : 아예 데이터를 바꾸고 싶을때 쓴다.

void InitTexture()
{
	int widthimage1, heightimage1, numberOfChannel1;
	stbi_set_flip_vertically_on_load(true);
	glGenTextures(15, textures);

	unsigned char* data1 = stbi_load("crysta5.png", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[0]
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data1); //---텍스처 이미지 정의

	unsigned char* data2 = stbi_load("space2.png", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[1] 우주 배경
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data2); //---텍스처 이미지 정의

	unsigned char* data3 = stbi_load("crystal1.jpg", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[2]
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data3); //---텍스처 이미지 정의

	unsigned char* data4 = stbi_load("crystal5.jpg", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[3]
	glBindTexture(GL_TEXTURE_2D, textures[3]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data4); //---텍스처 이미지 정의

	unsigned char* data5 = stbi_load("crystal3.jpg", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[4]
	glBindTexture(GL_TEXTURE_2D, textures[4]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data5); //---텍스처 이미지 정의

	unsigned char* data6 = stbi_load("crystal4.jpg", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[5]
	glBindTexture(GL_TEXTURE_2D, textures[5]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data6); //---텍스처 이미지 정의

	unsigned char* data7 = stbi_load("1212.jpg", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[6]
	glBindTexture(GL_TEXTURE_2D, textures[6]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data7); //---텍스처 이미지 정의

	unsigned char* data8 = stbi_load("crystal6.jpg", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[6]
	glBindTexture(GL_TEXTURE_2D, textures[7]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data8); //---텍스처 이미지 정의

	unsigned char* data9 = stbi_load("startview.bmp", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[6]
	glBindTexture(GL_TEXTURE_2D, textures[8]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data9); //---텍스처 이미지 정의

	unsigned char* data10 = stbi_load("endview.bmp", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[6]
	glBindTexture(GL_TEXTURE_2D, textures[9]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data10); //---텍스처 이미지 정의

	glUseProgram(shaderProgramID);
	int tLocation = glGetUniformLocation(shaderProgramID, "outTexture"); //--- outTexture1 유니폼 샘플러의 위치를 가져옴
	glUniform1i(tLocation, 0); //--- 샘플러를 0번 유닛으로 설정
}
