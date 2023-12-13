#define _CRT_SECURE_NO_WARNINGS //--- ���α׷� �� �տ� ������ ��
#define _USE_MATH_DEFINES
#define STB_IMAGE_IMPLEMENTATION

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include<glm/glm.hpp>
#include<glm/ext.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <random>
#include <string>
#include "stb_image.h"

using namespace std;

random_device rd;
mt19937 gen(rd());
uniform_real_distribution<double> XYdis(-1, 1);
uniform_real_distribution<double> dis(0.0, 1.0);

void InitTexture();
int widthImage, heightImage, numberOfChannel;
unsigned int textures[8];


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

		glGenVertexArrays(1, &vao); //--- VAO �� �����ϰ� �Ҵ��ϱ�
		glBindVertexArray(vao); //--- VAO�� ���ε��ϱ�
		glGenBuffers(4, vbo); //--- 3���� VBO�� �����ϰ� �Ҵ��ϱ�

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
		glBindTexture(GL_TEXTURE_2D, textures[num]); //--- texture[0]�� ����Ͽ� �������� �׸���.
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindTexture(GL_TEXTURE_2D, textures[num]); //--- texture[0]�� ����Ͽ� �������� �׸���.
		glDrawArrays(GL_TRIANGLES, 6, 6);
		glBindTexture(GL_TEXTURE_2D, textures[num]); //--- texture[0]�� ����Ͽ� �������� �׸���.
		glDrawArrays(GL_TRIANGLES, 12, 6);
		glBindTexture(GL_TEXTURE_2D, textures[num]); //--- texture[0]�� ����Ͽ� �������� �׸���.
		glDrawArrays(GL_TRIANGLES, 18, 6);
		glBindTexture(GL_TEXTURE_2D, textures[num]); //--- texture[0]�� ����Ͽ� �������� �׸���.
		glDrawArrays(GL_TRIANGLES, 24, 6);
		glBindTexture(GL_TEXTURE_2D, textures[num]); //--- texture[0]�� ����Ͽ� �������� �׸���.
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
		glGenVertexArrays(1, &vao); //--- VAO �� �����ϰ� �Ҵ��ϱ�
		glBindVertexArray(vao); //--- VAO�� ���ε��ϱ�
		glGenBuffers(4, vbo); //--- 3���� VBO�� �����ϰ� �Ҵ��ϱ�

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
	}

	void draw(int shaderID)
	{
		unsigned int modelLocation = glGetUniformLocation(shaderID, "model");
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(GetTransform() * GetmodelTransform()));
		glBindVertexArray(vao);
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

		glGenVertexArrays(1, &vao); //--- VAO �� �����ϰ� �Ҵ��ϱ�
		glBindVertexArray(vao); //--- VAO�� ���ε��ϱ�
		glGenBuffers(4, vbo); //--- 4���� VBO�� �����ϰ� �Ҵ��ϱ�

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
		glBindTexture(GL_TEXTURE_2D, textures[0]); //--- texture[0]�� ����Ͽ� �������� �׸���.
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindTexture(GL_TEXTURE_2D, textures[0]); //--- texture[0]�� ����Ͽ� �������� �׸���.
		glDrawArrays(GL_TRIANGLES, 3, 3);
		glBindTexture(GL_TEXTURE_2D, textures[0]); //--- texture[0]�� ����Ͽ� �������� �׸���.
		glDrawArrays(GL_TRIANGLES, 6, 3);
		glBindTexture(GL_TEXTURE_2D, textures[0]); //--- texture[0]�� ����Ͽ� �������� �׸���.
		glDrawArrays(GL_TRIANGLES, 9, 3);
		glBindTexture(GL_TEXTURE_2D, textures[0]); //--- texture[0]�� ����Ͽ� �������� �׸���.
		glDrawArrays(GL_TRIANGLES, 12, 3);
		glBindTexture(GL_TEXTURE_2D, textures[0]); //--- texture[0]�� ����Ͽ� �������� �׸���.
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

//���ư��� �Ƕ���
CUBE rotatePlane[8];
//���Դٰ� �������
CUBE showonoffPlane[5][5];
bool show[5][5];
//��¡�����
CUBE squidPlane[2][10];
bool squidOX[2][10];
bool squidDraw[2][10];

GLfloat lineShape[10][2][3] = {};	//--- ���� ��ġ ��

glm::vec3 colors[12][3] = {};

GLfloat XYZShape[3][2][3] = {
	{{-1.0,0.0,0.0},{1.0,0.0,0.0}},
	{{0.0,-1.0,0.0},{0.0,1.0,0.0}},
	{{0.0,0.0,-1.0},{0.0,0.0,1.0}} };

GLfloat XYZcolors[6][3] = { //--- �� ����
	{ 1.0, 0.0, 0.0 },	   	{ 1.0, 0.0, 0.0 },
	{ 0.0, 1.0, 0.0 },	   	{ 0.0, 1.0, 0.0 },
	{ 0.0, 0.0, 1.0 },	   	{ 0.0, 0.0, 1.0 }
};

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -4.0f); //--- ī�޶� ��ġ
glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, 100.0f); //--- ī�޶� �ٶ󺸴� ����
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f); //--- ī�޶� ���� ����

glm::mat4 model = glm::mat4(1.0f);
glm::mat4 view = glm::mat4(1.0f);
glm::mat4 projection = glm::mat4(1.0f);

GLuint vao, vbo[3];
GLuint TriPosVbo, TriColorVbo;

GLchar* vertexSource, * fragmentSource; //--- �ҽ��ڵ� ���� ����
GLuint vertexShader, fragmentShader; //--- ���̴� ��ü
GLuint shaderProgramID; //--- ���̴� ���α׷�

int windowWidth = 600;
int windowHeight = 600;

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
float cameraDistance = 15.0f; // ������ �Ÿ�
float cameraHeight = 3.0f; // ī�޶��� ����
float cameraAngle = 180.0f; // ī�޶� ����

bool start = true;

float w, a, s, d;
float speed = 0.05;
int JSelection = 0;
int JCnt = 0;
float jumpSize = 0.1;
const float gravity = 0.01f; // �߷� ���ӵ�
const float initialHeight = 0.5f; // �ʱ� ����
const float jumpInitialVelocity = 0.3f; // �ʱ� ���� �ӵ�
float jumpVelocity = jumpInitialVelocity; // ���� �ӵ��� ��ȭ��ų ����
bool upKeyPressed = false;
bool downKeyPressed = false;
bool leftKeyPressed = false;
bool rightKeyPressed = false;

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

void main(int argc, char** argv) //--- ������ ����ϰ� �ݹ��Լ� ����
{
	//--- ������ �����ϱ�
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("Example1");

	//--- GLEW �ʱ�ȭ�ϱ�
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
	//���ư��� �Ƕ���
	for (int i = 0; i < 8; ++i) {
		rotatePlane[i].ReadObj("cube.obj");
	}
	//���Դٰ� �������
	for (int i = 0; i < 5; ++i) {
		for (int j = 0; j < 5; ++j) {
			showonoffPlane[i][j].ReadObj("cube.obj");
		}
	}
	//��¡�����
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 10; ++j) {
			squidPlane[i][j].ReadObj("cube.obj");
		}
	}
	//--- ���̴� �о�ͼ� ���̴� ���α׷� �����
	make_shaderProgram(); //--- ���̴� ���α׷� �����
	InitBuffer();
	InitTexture();
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE); //--- ���� ������ �ʿ��� ������ �ϸ� �ȴ�.
	//glDisable(GL_DEPTH_TEST | GL_CULL_FACE);	//����

	//���ư��� �Ƕ��� ��ġ  > ������ 2.5 -> �������� ����
	float xOffsets[] = { -5, 0, 5, -2.5, 2.5, -5, 0, 5 };
	float zOffsets[] = { 0, 0, 0, 5, 5, 10, 10, 10 };
	for (int i = 0; i < 8; ++i) {
		rotatePlane[i].worldmatrix.position.x = xOffsets[i];
		rotatePlane[i].worldmatrix.position.z = sphere.worldmatrix.position.z + zOffsets[i];
		rotatePlane[i].worldmatrix.scale = glm::vec3(4, 0.4, 4); //1.0 �� 4�� ����
	}
	//���Դٰ� ������� ��ġ
	for (int i = 0; i < 5; ++i) {
		float baseZ = rotatePlane[7].worldmatrix.position.z + i * 2.5;
		for (int j = 0; j < 5; ++j) {
			showonoffPlane[i][j].worldmatrix.position.x = j * 2.5 - 5;
			showonoffPlane[i][j].worldmatrix.position.z = baseZ + 5;
			showonoffPlane[i][j].worldmatrix.scale = glm::vec3(2.5, 0.25, 2.5);
		}
	}
	//��¡�����
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 10; ++j) {
			if (i % 2 == 0) {
				squidPlane[i][j].worldmatrix.position.x = -2.5;
			}
			else {
				squidPlane[i][j].worldmatrix.position.x = 2.5;
			}
			squidPlane[i][j].worldmatrix.position.z = 30 + j * 2.5;
			squidPlane[i][j].worldmatrix.scale = glm::vec3(2.5, 0.25, 2.5);
			squidDraw[i][j] = true;
		}
	}
	for (int i = 0; i < 5;++i) {
		int b = rand() % 10;
		squidOX[0][b] = true;
		b = rand() % 10;
		squidOX[1][b] = true;
		
	}
	
	glutTimerFunc(10, TimerFunction, 1);
	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutSpecialFunc(SpecialKeys); // ����Ű �ݹ� �Լ� ���
	glutSpecialUpFunc(SpecialKeysUp); // Ű ���� �̺�Ʈ ó�� �߰�
	//glutMouseFunc(Mouse);
	glutMotionFunc(Motion);
	glutMouseWheelFunc(mouseWheel);

	glutMainLoop();
}

GLvoid drawScene()
{
	glUseProgram(shaderProgramID);
	glClearColor(0.0, 0.0, 0.0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //--- ���� ���۸� Ŭ�����Ѵ�.

	glBindVertexArray(vao);

	// ���� �ٲٱ�
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), XYZcolors, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	int modelLocation = glGetUniformLocation(shaderProgramID, "model"); //--- ���ؽ� ���̴����� �𵨸� ��ȯ ��� �������� �޾ƿ´�.
	int viewLocation = glGetUniformLocation(shaderProgramID, "view"); //--- ���ؽ� ���̴����� ���� ��ȯ ��� �������� �޾ƿ´�.
	int projLocation = glGetUniformLocation(shaderProgramID, "projection"); //--- ���ؽ� ���̴����� ���� ��ȯ ��� �������� �޾ƿ´�.

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

	glm::mat4 lightmatrix = minicube.GetTransform(); // �־��� mat4 ���
	glm::vec3 lightposition = glm::vec3(lightmatrix[3]); // ����� ������ ���� ����Ͽ� ��ġ ����

	unsigned int lightPosLocation = glGetUniformLocation(shaderProgramID, "lightPos"); //--- lightPos �� ����: (0.0, 0.0, 5.0);
	glUniform3f(lightPosLocation, lightposition.x, lightposition.y, lightposition.z);
	unsigned int lightColorLocation = glGetUniformLocation(shaderProgramID, "lightColor"); //--- lightColor �� ����: (1.0, 1.0, 1.0) ���
	glUniform3f(lightColorLocation, 1.0, 1.0, 1.0);
	unsigned int objColorLocation = glGetUniformLocation(shaderProgramID, "objectColor"); //--- object Color�� ����: (1.0, 0.5, 0.3)�� ��
	glUniform3f(objColorLocation, 1.0, 0.5, 0.3);

	if (start)
	{
		start = false;
	}

	model = glm::mat4(1.0f);
	//�� �׸���
	for (int i = 0; i < 3; i++)
	{
		// ���� �ٲٱ�
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(GLfloat), XYZcolors[i * 2], GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		// modelTransform ������ ��ȯ �� �����ϱ�
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(GLfloat), XYZShape[i], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glLineWidth(2.0);
		glDrawArrays(GL_LINES, 0, 2);
	}

	//s r t p �ڵ� �ۼ��ÿ��� �ݴ� ��������.
	//���ư��� �Ƕ���
	model = glm::mat4(1.0f);
	for (int i = 0; i < 8; ++i) {
		rotatePlane[i].draw(shaderProgramID, 0);
	}
	//���Դٰ� �������
	model = glm::mat4(1.0f);
	for (int i = 0; i < 5; ++i) {
		for (int j = 0; j < 5; ++j) {
			if (show[i][j])
				showonoffPlane[i][j].draw(shaderProgramID, 2);
		}
	}
	
	model = glm::mat4(1.0f);
	sphere.draw(shaderProgramID);
	minicube.draw(shaderProgramID, 1);
	skybox.draw(shaderProgramID, 1);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//���� �׸��� ������ ���⿡ ��ü �׸���
	//��¡�� ����
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 10; ++j) {
			if (squidDraw[i][j])
				squidPlane[i][j].draw(shaderProgramID, 3);
		}
	}
	glDisable(GL_BLEND);
	glutSwapBuffers(); //--- ȭ�鿡 ����ϱ�
}

//--- �ٽñ׸��� �ݹ� �Լ�
GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
}

void InitBuffer()
{
	glGenVertexArrays(1, &vao); //--- VAO �� �����ϰ� �Ҵ��ϱ�
	glBindVertexArray(vao); //--- VAO�� ���ε��ϱ�
	glGenBuffers(2, vbo); //--- 2���� VBO�� �����ϰ� �Ҵ��ϱ�

	cube.Init();
	minicube.Init();
	minicube.parent = &cube;
	//���ư��� �Ƕ���
	for (int i = 0; i < 8; ++i) {
		rotatePlane[i].Init();
	}
	//���Դٰ� �������
	for (int i = 0; i < 5; ++i) {
		for (int j = 0; j < 5; ++j) {
			showonoffPlane[i][j].Init();
		}
	}
	//��¡�����
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 10; ++j) {
			squidPlane[i][j].Init();
		}
	}
	sphere.Init();
	sphere.modelmatrix.scale = glm::vec3(0.5, 0.5, 0.5);

	skybox.Init();
	skybox.worldmatrix.position.y = 10;
	skybox.worldmatrix.scale = glm::vec3(50.0, 50.0, 200.0);

	minicube.worldmatrix.position.z = -3;
	minicube.modelmatrix.scale = glm::vec3(0.5, 0.5, 0.5);
}

void make_shaderProgram()
{
	make_vertexShaders(); //--- ���ؽ� ���̴� �����
	make_fragmentShaders(); //--- �����׸�Ʈ ���̴� �����
	//-- shader Program
	shaderProgramID = glCreateProgram();
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	glLinkProgram(shaderProgramID);
	//--- ���̴� �����ϱ�
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	//--- Shader Program ����ϱ�
	glUseProgram(shaderProgramID);
}

void make_vertexShaders()
{
	vertexSource = filetobuf("vertex5.glsl");
	//--- ���ؽ� ���̴� ��ü �����
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	//--- ���̴� �ڵ带 ���̴� ��ü�� �ֱ�
	glShaderSource(vertexShader, 1, (const GLchar**)&vertexSource, 0);
	//--- ���ؽ� ���̴� �������ϱ�
	glCompileShader(vertexShader);
	//--- �������� ����� ���� ���� ���: ���� üũ
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		std::cout << "ERROR: vertex shader ������ ����\n" << errorLog << std::endl;
		return;
	}
}

void make_fragmentShaders()
{
	fragmentSource = filetobuf("fragment5.glsl");
	//--- �����׸�Ʈ ���̴� ��ü �����
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	//--- ���̴� �ڵ带 ���̴� ��ü�� �ֱ�
	glShaderSource(fragmentShader, 1, (const GLchar**)&fragmentSource, 0);
	//--- �����׸�Ʈ ���̴� ������
	glCompileShader(fragmentShader);
	//--- �������� ����� ���� ���� ���: ������ ���� üũ
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		std::cout << "ERROR: fragment shader ������ ����\n" << errorLog << std::endl;
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

//�� z+ �� x+

GLvoid Keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case 'q':
		glutLeaveMainLoop();
		break;
	}
	glutPostRedisplay(); //--- ������ �ٲ� ������ ��� �ݹ� �Լ��� ȣ���Ͽ� ȭ���� refresh �Ѵ�
}

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
	}
	glutPostRedisplay(); // ȭ�� ����
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
	}
}

void moveSphere()
{
	if (upKeyPressed)
	{
		sphere.worldmatrix.position.z += speed;
	}
	if (downKeyPressed)
	{
		sphere.worldmatrix.position.z -= speed;
	}
	if (leftKeyPressed)
	{
		sphere.worldmatrix.position.x += speed;
	}
	if (rightKeyPressed)
	{
		sphere.worldmatrix.position.x -= speed;
	}
}

//GLvoid Mouse(int button, int state, int x, int y)
//{
//	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
//	{
//		ox = x;
//		oy = y;
//		left_button = true;
//	}
//	else
//	{
//		ox = 0;
//		oy = 0;
//		pre_x_angle = x_angle;
//		pre_y_angle = y_angle;
//		left_button = false;
//	}
//}

//GLvoid Motion(int x, int y)
//{
//	if (left_button)
//	{
//		y_angle = x - ox;
//		x_angle = y - oy;
//		x_angle += pre_x_angle;
//		y_angle += pre_y_angle;
//
//		y_angle /= 2;
//		x_angle /= 2;
//	}
//	glutPostRedisplay();
//}

GLvoid Motion(int x, int y)
{
	ox = x;
	oy = y;
	left_button = true;

	float x_diff = x - ox;
	float y_diff = y - oy;
	cameraDirection += x_diff / 2;

	// ī�޶� ���̿� �Ÿ��� ������ ������� ��ġ�� ����
	cameraPos.x = sphere.worldmatrix.position.x + cameraDistance * sin(glm::radians(cameraAngle));
	cameraPos.y = sphere.worldmatrix.position.y + cameraHeight;
	cameraPos.z = sphere.worldmatrix.position.z + cameraDistance * cos(glm::radians(cameraAngle));

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
//���ư��� �Ƕ���
bool XZBoundingBox(CUBE obstalce[], int index, float r) {
	if (obstalce[index].worldmatrix.position.x - r < sphere.worldmatrix.position.x &&
		obstalce[index].worldmatrix.position.x + r > sphere.worldmatrix.position.x &&
		obstalce[index].worldmatrix.position.z + r > sphere.worldmatrix.position.z &&
		obstalce[index].worldmatrix.position.z - r < sphere.worldmatrix.position.z) {
		return true;
	}
	else return false;
}
//���Դٰ� �������
bool XZBoundingBox2(CUBE obstalce[5][5], int i, int j, float r) {
	if (obstalce[i][j].worldmatrix.position.x - r < sphere.worldmatrix.position.x &&
		obstalce[i][j].worldmatrix.position.x + r > sphere.worldmatrix.position.x &&
		obstalce[i][j].worldmatrix.position.z + r > sphere.worldmatrix.position.z &&
		obstalce[i][j].worldmatrix.position.z - r < sphere.worldmatrix.position.z) {
		return true;
	}
	else return false;
}
//��¡�����
bool XZBoundingBox3(CUBE obstalce[2][10], int i, int j, float r) {
	if (obstalce[i][j].worldmatrix.position.x - r < sphere.worldmatrix.position.x &&
		obstalce[i][j].worldmatrix.position.x + r > sphere.worldmatrix.position.x &&
		obstalce[i][j].worldmatrix.position.z + r > sphere.worldmatrix.position.z &&
		obstalce[i][j].worldmatrix.position.z - r < sphere.worldmatrix.position.z) {
		return true;
	}
	else return false;
}
int Counting = 0;
GLvoid TimerFunction(int value)
{
	switch (value)
	{
	case 1:

		moveSphere();

		//sphere.worldmatrix.position.y += jumpVelocity; // ���� ���� �ӵ� ����
		//sphere.worldmatrix.scale = glm::vec3(1.0f, 1.0f + jumpVelocity, 1.0f);

		// ī�޶� ��ġ ����
		cameraPos.x = sphere.worldmatrix.position.x + cameraDistance * sin(glm::radians(cameraAngle));
		cameraPos.y = sphere.worldmatrix.position.y + cameraHeight;
		cameraPos.z = sphere.worldmatrix.position.z + cameraDistance * cos(glm::radians(cameraAngle));

		//// ī�޶� ���� �ٶ󺸵��� ���� ����
		//cameraDirection = glm::normalize(sphere.worldmatrix.position - cameraPos);

		// �߷� ����
		jumpVelocity -= gravity;

		// ���� ����� ���� ó��
		if (sphere.worldmatrix.position.y <= initialHeight) {
			sphere.worldmatrix.position.y = initialHeight;
			jumpVelocity = jumpInitialVelocity; // �ٽ� �ʱ� ���� �ӵ��� ����
		}

		//���ư��� �Ƕ��� z ���� 0 ~ 5
		float angle[8] = { 2.5f,-2.5f,2.5f,-2.5f,2.5f,-2.5f,-2.5f,2.5f };
		for (int i = 0; i < 8; ++i) {
			rotatePlane[i].worldmatrix.rotation.y += angle[i];
			if (rotatePlane[i].worldmatrix.rotation.y > 360.0f || rotatePlane[i].worldmatrix.rotation.y < -360.0f) rotatePlane[i].worldmatrix.rotation.y = 0;
			//�浹�� �غ��߿�
			if (!upKeyPressed && !downKeyPressed && !rightKeyPressed && !leftKeyPressed && XZBoundingBox(rotatePlane ,i, 2.5)) {
				sphere.worldmatrix.position.x = rotatePlane[i].worldmatrix.position.x + 1.25 * cos(-rotatePlane[i].worldmatrix.rotation.y * M_PI / 180.0);
				sphere.worldmatrix.position.z = rotatePlane[i].worldmatrix.position.z + 1.25 * sin(-rotatePlane[i].worldmatrix.rotation.y * M_PI / 180.0);
			}
		}
		
		//���Դٰ� ������� z ���� 5 ~ 10
		if (Counting < 5)Counting += 1;
		if (Counting == 5) {
			int x = rand() % 5;
			int y = rand() % 5;
			if (!show[x][y]) show[x][y] = true;
			else show[x][y] = false;
			Counting = 0;
		}
		for (int i = 0; i < 5; ++i) {
			for (int j = 0; j < 5; ++j) {
				if (!show[i][j] && XZBoundingBox2(showonoffPlane, i, j, 1.25)) { //������ �����ؾ��ҵ�	
					//������ ��������
					cout << i << " " << j << endl;
				}
			}
		}
		//��¡�� ����
		for (int i = 0; i < 2; ++i) {
			for (int j = 0; j < 10; ++j) {
				if (XZBoundingBox3(squidPlane, i, j, 1.25)) { //������ �����ؾ��ҵ�		
					if (!squidOX[i][j]) squidDraw[i][j] = false;
				}
			}
		}
		break;
	}
	glutPostRedisplay();
	glutTimerFunc(10, TimerFunction, 1);
}

//update() : �ƿ� �����͸� �ٲٰ� ������ ����.

void InitTexture()
{
	int widthimage1, heightimage1, numberOfChannel1;
	stbi_set_flip_vertically_on_load(true);
	glGenTextures(8, textures);

	unsigned char* data1 = stbi_load("A.bmp", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[0]
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data1); //---�ؽ�ó �̹��� ����

	unsigned char* data2 = stbi_load("space2.png", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[1] ���� ���
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data2); //---�ؽ�ó �̹��� ����

	unsigned char* data3 = stbi_load("B.bmp", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[2]
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data3); //---�ؽ�ó �̹��� ����

	unsigned char* data4 = stbi_load("�λ�������.jpg", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[3]
	glBindTexture(GL_TEXTURE_2D, textures[3]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data4); //---�ؽ�ó �̹��� ����

	unsigned char* data5 = stbi_load("E.bmp", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[4]
	glBindTexture(GL_TEXTURE_2D, textures[4]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data5); //---�ؽ�ó �̹��� ����

	unsigned char* data6 = stbi_load("F.bmp", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[5]
	glBindTexture(GL_TEXTURE_2D, textures[5]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data6); //---�ؽ�ó �̹��� ����

	unsigned char* data7 = stbi_load("stone.bmp", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[6]
	glBindTexture(GL_TEXTURE_2D, textures[6]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data7); //---�ؽ�ó �̹��� ����

	unsigned char* data8 = stbi_load("plain.bmp", &widthimage1, &heightimage1, &numberOfChannel1, 0);
	//--- texture[6]
	glBindTexture(GL_TEXTURE_2D, textures[7]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, widthimage1, heightimage1, 0, GL_RGB, GL_UNSIGNED_BYTE, data8); //---�ؽ�ó �̹��� ����

	glUseProgram(shaderProgramID);
	int tLocation = glGetUniformLocation(shaderProgramID, "outTexture"); //--- outTexture1 ������ ���÷��� ��ġ�� ������
	glUniform1i(tLocation, 0); //--- ���÷��� 0�� �������� ����
}
