#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"

#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define PI 3.14159265358979323846
#define DIRECTIONALLIGHT 0
#define POINTLIGHT 1
#define SPOTLIGHT 2
#define PERVERTEXLIGHTING 0
#define PERPIXELLIGHTING 1

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
int curr_window_width = WINDOW_WIDTH;
int curr_window_height = WINDOW_HEIGHT;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	LightEdit = 3,
	ShininessEdit = 4,
};

int lightSource = DIRECTIONALLIGHT;

struct Uniform
{
	GLint iLocLightSource;	// directional light, point light, spot light
	GLint iLocLightingMode; // per-vertex, per-pixel

	GLint iLocModelMatrix;
	GLint iLocViewMatrix;
	GLint iLocProjectionMatrix;

	GLint iLocDirectionalPosition;
	GLint iLocDirectionalDirection;
	GLint iLocDirectionalAmbientIntensity;
	GLint iLocDirectionalDiffuseIntensity;
	GLint iLocDirectionalSpecularIntensity;
	GLint iLocDirectionalShininess;

	GLint iLocPointPosition;
	GLint iLocPointAmbientIntensity;
	GLint iLocPointDiffuseIntensity;
	GLint iLocPointSpecularIntensity;
	GLint iLocPointShininess;
	GLint iLocPointConstant;
	GLint iLocPointLinear;
	GLint iLocPointQuadratic;

	GLint iLocSpotPosition;
	GLint iLocSpotDirection;
	GLint iLocSpotExponent;
	GLint iLocSpotCutoff;
	GLint iLocSpotAmbientIntensity;
	GLint iLocSpotDiffuseIntensity;
	GLint iLocSpotSpecularIntensity;
	GLint iLocSpotShininess;
	GLint iLocSpotConstant;
	GLint iLocSpotLinear;
	GLint iLocSpotQuadratic;

	GLint iLocKa;
	GLint iLocKd;
	GLint iLocKs;

	GLint iLocCameraPosition;	
};
Uniform uniform;

struct DirectionalLight
{
	Vector3 position = Vector3(1, 1, 1);
	Vector3 direction = Vector3(0, 0, 0);
	Vector3 diffuse_intensity = Vector3(1, 1, 1);
	Vector3 ambient_intensity = Vector3(0.15, 0.15, 0.15);
	Vector3 specular_intensity = Vector3(1, 1, 1);
	GLfloat shininess = 64;
};
DirectionalLight directional_light;

struct PointLight
{
	Vector3 position = Vector3(0, 2, 1);
	Vector3 diffuse_intensity = Vector3(1, 1, 1);
	Vector3 ambient_intensity = Vector3(0.15, 0.15, 0.15);
	Vector3 specular_intensity = Vector3(1, 1, 1);
	GLfloat shininess = 64;
	// attenuation
	GLfloat constant = 0.01;
	GLfloat linear = 0.8;
	GLfloat quadratic = 0.1;
};
PointLight point_light;

struct SpotLight
{
	Vector3 position = Vector3(0, 0, 2);
	Vector3 direction = Vector3(0, 0, -1);
	GLfloat exponent = 50;
	GLfloat cutoff = 30; // degree
	Vector3 diffuse_intensity = Vector3(1, 1, 1);
	Vector3 ambient_intensity = Vector3(0.15, 0.15, 0.15);
	Vector3 specular_intensity = Vector3(1, 1, 1);
	GLfloat shininess = 64;
	// attenuation
	GLfloat constant = 0.05;
	GLfloat linear = 0.3;
	GLfloat quadratic = 0.6;
};
SpotLight spot_light;

vector<string> filenames; // .obj filename list

struct PhongMaterial
{
	Vector3 Ka;
	Vector3 Kd;
	Vector3 Ks;
};

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	PhongMaterial material;
	int indexCount;
	GLuint m_texture;
} Shape;

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form

	vector<Shape> shapes;
};
vector<model> models;

struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};
project_setting proj;

TransMode cur_trans_mode = GeoTranslation;

Matrix4 view_matrix;
Matrix4 project_matrix;

int cur_idx = 0; // represent which model should be rendered now


static GLvoid Normalize(GLfloat v[3])
{
	GLfloat l;

	l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}


// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
	Matrix4 mat;

	/*
	mat = Matrix4(
		...
	);
	*/
	mat = Matrix4(
		1, 0, 0, vec.x,
		0, 1, 0, vec.y,
		0, 0, 1, vec.z,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
	Matrix4 mat;

	/*
	mat = Matrix4(
		...
	);
	*/
	mat = Matrix4(
		vec.x, 0, 0, 0,
		0, vec.y, 0, 0,
		0, 0, vec.z, 0,
		0, 0, 0, 1
	);

	return mat;
}


// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
	Matrix4 mat;

	/*
	mat = Matrix4(
		...
	);
	*/
	val = val * PI / 180.0;

	mat = Matrix4(
		1, 0, 0, 0,
		0, cos(val), -sin(val), 0,
		0, sin(val), cos(val), 0,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
	Matrix4 mat;

	/*
	mat = Matrix4(
		...
	);
	*/
	val = val * PI / 180.0;

	mat = Matrix4(
		cos(val), 0, sin(val), 0,
		0, 1, 0, 0,
		-sin(val), 0, cos(val), 0,
		0, 0, 0, 1
	);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
	Matrix4 mat;

	/*
	mat = Matrix4(
		...
	);
	*/
	val = val * PI / 180.0;

	mat = Matrix4(
		cos(val), -sin(val), 0, 0,
		sin(val), cos(val), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);

	return mat;
}

Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
	// view_matrix[...] = ...
	// Translate eye position to the origin
	Matrix4 T = Matrix4(
		1, 0, 0, -main_camera.position.x,
		0, 1, 0, -main_camera.position.y,
		0, 0, 1, -main_camera.position.z,
		0, 0, 0, 1
	);

	// Find the bases with respect to the new space
	Vector3 view_direction = main_camera.center - main_camera.position;
	Vector3 up_direction = main_camera.up_vector;
	GLfloat P1P2[3] = { view_direction.x, view_direction.y, view_direction.z };
	GLfloat P1P3[3] = { up_direction.x, up_direction.y, up_direction.z };

	Normalize(P1P2);
	Normalize(P1P3);

	GLfloat Rx[3], Ry[3], Rz[3];

	for (int i = 0; i < 3; i++) {
		Rz[i] = -P1P2[i];
	}
	Cross(P1P2, P1P3, Rx);
	Cross(Rz, Rx, Ry);

	Matrix4 R = Matrix4(
		Rx[0], Rx[1], Rx[2], 0,
		Ry[0], Ry[1], Ry[2], 0,
		Rz[0], Rz[1], Rz[2], 0,
		0, 0, 0, 1
	);

	view_matrix = R * T;
}

// [TODO] compute persepective projection matrix
void setPerspective()
{
	// GLfloat f = ...
	// project_matrix [...] = ...
	GLfloat near = proj.nearClip, far = proj.farClip;

	GLfloat f = 1 / tan(proj.fovy * PI / 180.0 / 2);

	f = f / proj.top * min(proj.top, proj.right);

	project_matrix = Matrix4(
		f / proj.aspect, 0, 0, 0,
		0, f, 0, 0,
		0, 0, (far + near) / (near - far), 2 * far * near / (near - far),
		0, 0, -1, 0
	);
}

void setGLMatrix(GLfloat* glm, Matrix4& m) {
	glm[0] = m[0];  glm[4] = m[1];   glm[8] = m[2];    glm[12] = m[3];
	glm[1] = m[4];  glm[5] = m[5];   glm[9] = m[6];    glm[13] = m[7];
	glm[2] = m[8];  glm[6] = m[9];   glm[10] = m[10];   glm[14] = m[11];
	glm[3] = m[12];  glm[7] = m[13];  glm[11] = m[14];   glm[15] = m[15];
}

// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	// [TODO] change your aspect ratio
	proj.left = -1 * (float)width / (float)min(width, height);
	proj.right = 1 * (float)width / (float)min(width, height);
	proj.top = 1 * (float)height / (float)min(width, height);
	proj.bottom = -1 * (float)height / (float)min(width, height);
	proj.aspect = (float)width / (float)height;

	curr_window_width = width;
	curr_window_height = height;

	setPerspective();
}

// Render function for display rendering
void RenderScene(void) {	
	// clear canvas
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	Matrix4 T, R, S;
	// [TODO] update translation, rotation and scaling
	T = translate(models[cur_idx].position);
	R = rotate(models[cur_idx].rotation);
	S = scaling(models[cur_idx].scale);

	Matrix4 M, V, P;
	GLfloat m[16], v[16], p[16];

	// [TODO] multiply all the matrix
	M = T * R * S;
	V = view_matrix;
	P = project_matrix;

	// row-major ---> column-major
	setGLMatrix(m, M);
	setGLMatrix(v, V);
	setGLMatrix(p, P);

	// use uniform to send mvp to vertex shader
	glUniform1i(uniform.iLocLightSource, lightSource);

	glUniformMatrix4fv(uniform.iLocModelMatrix, 1, GL_FALSE, m);
	glUniformMatrix4fv(uniform.iLocViewMatrix, 1, GL_FALSE, v);
	glUniformMatrix4fv(uniform.iLocProjectionMatrix, 1, GL_FALSE, p);

	glUniform3f(uniform.iLocDirectionalPosition, directional_light.position.x, directional_light.position.y, directional_light.position.z);
	glUniform3f(uniform.iLocDirectionalDirection, directional_light.direction.x, directional_light.direction.y, directional_light.direction.z);
	glUniform3f(uniform.iLocDirectionalAmbientIntensity, directional_light.ambient_intensity.x, directional_light.ambient_intensity.y, directional_light.ambient_intensity.z);
	glUniform3f(uniform.iLocDirectionalDiffuseIntensity, directional_light.diffuse_intensity.x, directional_light.diffuse_intensity.y, directional_light.diffuse_intensity.z);
	glUniform3f(uniform.iLocDirectionalSpecularIntensity, directional_light.specular_intensity.x, directional_light.specular_intensity.y, directional_light.specular_intensity.z);
	glUniform1f(uniform.iLocDirectionalShininess, directional_light.shininess);

	glUniform3f(uniform.iLocPointPosition, point_light.position.x, point_light.position.y, point_light.position.z);
	glUniform3f(uniform.iLocPointAmbientIntensity, point_light.ambient_intensity.x, point_light.ambient_intensity.y, point_light.ambient_intensity.z);
	glUniform3f(uniform.iLocPointDiffuseIntensity, point_light.diffuse_intensity.x, point_light.diffuse_intensity.y, point_light.diffuse_intensity.z);
	glUniform3f(uniform.iLocPointSpecularIntensity, point_light.specular_intensity.x, point_light.specular_intensity.y, point_light.specular_intensity.z);
	glUniform1f(uniform.iLocPointShininess, point_light.shininess);
	glUniform1f(uniform.iLocPointConstant, point_light.constant);
	glUniform1f(uniform.iLocPointLinear, point_light.linear);
	glUniform1f(uniform.iLocPointQuadratic, point_light.quadratic);

	glUniform3f(uniform.iLocSpotPosition, spot_light.position.x, spot_light.position.y, spot_light.position.z);
	glUniform3f(uniform.iLocSpotDirection, spot_light.direction.x, spot_light.direction.y, spot_light.direction.z);
	glUniform1f(uniform.iLocSpotExponent, spot_light.exponent);
	glUniform1f(uniform.iLocSpotCutoff, spot_light.cutoff);
	glUniform3f(uniform.iLocSpotAmbientIntensity, spot_light.ambient_intensity.x, spot_light.ambient_intensity.y, spot_light.ambient_intensity.z);
	glUniform3f(uniform.iLocSpotDiffuseIntensity, spot_light.diffuse_intensity.x, spot_light.diffuse_intensity.y, spot_light.diffuse_intensity.z);
	glUniform3f(uniform.iLocSpotSpecularIntensity, spot_light.specular_intensity.x, spot_light.specular_intensity.y, spot_light.specular_intensity.z);
	glUniform1f(uniform.iLocSpotShininess, spot_light.shininess);
	glUniform1f(uniform.iLocSpotConstant, spot_light.constant);
	glUniform1f(uniform.iLocSpotLinear, spot_light.linear);
	glUniform1f(uniform.iLocSpotQuadratic, spot_light.quadratic);

	glUniform3f(uniform.iLocCameraPosition, main_camera.position.x, main_camera.position.y, main_camera.position.z);
	
	for (int shader = 0; shader < 2; shader++) {
		if (shader == 0) {
			// vertex shader (per vertex lighting)
			glUniform1i(uniform.iLocLightingMode, PERVERTEXLIGHTING);
			glViewport(0, 0, curr_window_width / 2, curr_window_height);
		} else {
			// fragment shader (per pixel lighting)
			glUniform1i(uniform.iLocLightingMode, PERPIXELLIGHTING);
			glViewport(curr_window_width / 2, 0, curr_window_width / 2, curr_window_height);
		}

		for (int i = 0; i < models[cur_idx].shapes.size(); i++) {
			glUniform3f(uniform.iLocKa, models[cur_idx].shapes[i].material.Ka.x, models[cur_idx].shapes[i].material.Ka.y, models[cur_idx].shapes[i].material.Ka.z);
			glUniform3f(uniform.iLocKd, models[cur_idx].shapes[i].material.Kd.x, models[cur_idx].shapes[i].material.Kd.y, models[cur_idx].shapes[i].material.Kd.z);
			glUniform3f(uniform.iLocKs, models[cur_idx].shapes[i].material.Ks.x, models[cur_idx].shapes[i].material.Ks.y, models[cur_idx].shapes[i].material.Ks.z);

			glBindVertexArray(models[cur_idx].shapes[i].vao);
			glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
		}
	}

}


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// [TODO] Call back function for keyboard
	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_W:
			GLint polygonMode;
			glGetIntegerv(GL_POLYGON_MODE, &polygonMode);

			if (polygonMode == GL_FILL) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}
			else {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
			break;
		case GLFW_KEY_Z:
			cur_idx += models.size() - 1;
			cur_idx %= models.size();
			break;
		case GLFW_KEY_X:
			cur_idx += 1;
			cur_idx %= models.size();
			break;
		case GLFW_KEY_T:
			cur_trans_mode = GeoTranslation;
			break;
		case GLFW_KEY_S:
			cur_trans_mode = GeoScaling;
			break;
		case GLFW_KEY_R:
			cur_trans_mode = GeoRotation;
			break;
		case GLFW_KEY_L:
			lightSource += 1;
			lightSource %= 3;
			if (lightSource == DIRECTIONALLIGHT) {
				cout << "Directional light" << endl;
			}
			else if (lightSource == POINTLIGHT) {
				cout << "Point light" << endl;
			}
			else if (lightSource == SPOTLIGHT) {
				cout << "Spot light" << endl;
			} else {
				cout << "Error" << endl;
			}
			break;
		case GLFW_KEY_K:
			cur_trans_mode = LightEdit;
			break;
		case GLFW_KEY_J:
			cur_trans_mode = ShininessEdit;
			break;
		case GLFW_KEY_H:
			cout << "W: switch between solid and wireframe mode" << endl;
			cout << "Z/X: switch the model" << endl;
			cout << "T: switch to translation mode" << endl;
			cout << "S: switch to scale mode" << endl;
			cout << "R: switch to rotation mode" << endl;
			cout << "L: switch between directional/point/spot light" << endl;
			cout << "K: switch to light editing mode" << endl;
			cout << "J: switch to shininess editing mode" << endl;
			break;
		default:
			break;
		}
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// [TODO] scroll up positive, otherwise it would be negtive
	switch (cur_trans_mode) {
		case GeoTranslation:
			models[cur_idx].position.z += (float)yoffset * 0.1;
			break;
		case GeoRotation:
			models[cur_idx].rotation.z += (float)yoffset * 360 * 0.01;
			break;
		case GeoScaling:
			models[cur_idx].scale.z += (float)yoffset * 0.1;
			break;
		case LightEdit:
			if (lightSource == DIRECTIONALLIGHT) {
				directional_light.diffuse_intensity.x += (float)yoffset * 0.1;
				directional_light.diffuse_intensity.y += (float)yoffset * 0.1;
				directional_light.diffuse_intensity.z += (float)yoffset * 0.1;
			}
			else if (lightSource == POINTLIGHT) {
				point_light.diffuse_intensity.x += (float)yoffset * 0.1;
				point_light.diffuse_intensity.y += (float)yoffset * 0.1;
				point_light.diffuse_intensity.z += (float)yoffset * 0.1;
			}
			else if (lightSource == SPOTLIGHT) {
				spot_light.cutoff += (float)yoffset * 360 * 0.01;
			}
			break;
		case ShininessEdit:
			directional_light.shininess += (float)yoffset;
			point_light.shininess += (float)yoffset;
			spot_light.shininess += (float)yoffset;
			break;
		default:
			break;
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// [TODO] mouse press callback function
	if (action == GLFW_PRESS) {
		mouse_pressed = true;
	}
	else if (action == GLFW_RELEASE) {
		mouse_pressed = false;
	}
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	// [TODO] cursor position callback function
	if (mouse_pressed == true) {
		GLfloat x_diff = (float)(xpos - starting_press_x) * 2.0 / (float)curr_window_width;
		GLfloat y_diff = (float)(ypos - starting_press_y) * 2.0 / (float)curr_window_height;

		switch (cur_trans_mode) {
			case GeoTranslation:
				models[cur_idx].position.x += (float)x_diff;
				models[cur_idx].position.y += (float)-y_diff;
				break;
			case GeoRotation:
				models[cur_idx].rotation.y += (float)-x_diff * 360.0;
				models[cur_idx].rotation.x += (float)-y_diff * 360.0;
				break;
			case GeoScaling:
				models[cur_idx].scale.x += (float)-x_diff;
				models[cur_idx].scale.y += (float)-y_diff;
				break;
			case LightEdit:
				if (lightSource == DIRECTIONALLIGHT) {
					directional_light.position.x += (float)x_diff;
					directional_light.position.y += (float)-y_diff;
				}
				else if (lightSource == POINTLIGHT) {
					point_light.position.x += (float)x_diff;
					point_light.position.y += (float)-y_diff;
				}
				else if (lightSource == SPOTLIGHT) {
					spot_light.position.x += (float)x_diff;
					spot_light.position.y += (float)-y_diff;
				}
				break;
			default:
				break;
		}
	}

	starting_press_x = xpos;
	starting_press_y = ypos;
}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vs");
	fs = textFileRead("shader.fs");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p,f);
	glAttachShader(p,v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	uniform.iLocLightSource = glGetUniformLocation(p, "lightSource");
	uniform.iLocLightingMode = glGetUniformLocation(p, "lightingMode");

	uniform.iLocModelMatrix = glGetUniformLocation(p, "modelMatrix");
	uniform.iLocViewMatrix = glGetUniformLocation(p, "viewMatrix");
	uniform.iLocProjectionMatrix = glGetUniformLocation(p, "projectionMatrix");

	uniform.iLocDirectionalPosition = glGetUniformLocation(p, "directionalLight_position");
	uniform.iLocDirectionalDirection = glGetUniformLocation(p, "directionalLight_direction");
	uniform.iLocDirectionalAmbientIntensity = glGetUniformLocation(p, "directionalLight_ambientIntensity");
	uniform.iLocDirectionalDiffuseIntensity = glGetUniformLocation(p, "directionalLight_diffuseIntensity");
	uniform.iLocDirectionalSpecularIntensity = glGetUniformLocation(p, "directionalLight_specularIntensity");
	uniform.iLocDirectionalShininess = glGetUniformLocation(p, "directionalLight_shininess");

	uniform.iLocPointPosition = glGetUniformLocation(p, "pointLight_position");
	uniform.iLocPointAmbientIntensity = glGetUniformLocation(p, "pointLight_ambientIntensity");
	uniform.iLocPointDiffuseIntensity = glGetUniformLocation(p, "pointLight_diffuseIntensity");
	uniform.iLocPointSpecularIntensity = glGetUniformLocation(p, "pointLight_specularIntensity");
	uniform.iLocPointShininess = glGetUniformLocation(p, "pointLight_shininess");
	uniform.iLocPointConstant = glGetUniformLocation(p, "pointLight_constant");
	uniform.iLocPointLinear = glGetUniformLocation(p, "pointLight_linear");
	uniform.iLocPointQuadratic = glGetUniformLocation(p, "pointLight_quadratic");

	uniform.iLocSpotPosition = glGetUniformLocation(p, "spotLight_position");
	uniform.iLocSpotDirection = glGetUniformLocation(p, "spotLight_direction");
	uniform.iLocSpotExponent = glGetUniformLocation(p, "spotLight_exponent");
	uniform.iLocSpotCutoff = glGetUniformLocation(p, "spotLight_cutoff");
	uniform.iLocSpotAmbientIntensity = glGetUniformLocation(p, "spotLight_ambientIntensity");
	uniform.iLocSpotDiffuseIntensity = glGetUniformLocation(p, "spotLight_diffuseIntensity");
	uniform.iLocSpotSpecularIntensity = glGetUniformLocation(p, "spotLight_specularIntensity");
	uniform.iLocSpotShininess = glGetUniformLocation(p, "spotLight_shininess");
	uniform.iLocSpotConstant = glGetUniformLocation(p, "spotLight_constant");
	uniform.iLocSpotLinear = glGetUniformLocation(p, "spotLight_linear");
	uniform.iLocSpotQuadratic = glGetUniformLocation(p, "spotLight_quadratic");
	
	uniform.iLocKa = glGetUniformLocation(p, "Ka");
	uniform.iLocKd = glGetUniformLocation(p, "Kd");
	uniform.iLocKs = glGetUniformLocation(p, "Ks");
	
	uniform.iLocCameraPosition = glGetUniformLocation(p, "camera_position");

	if (success)
		glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}

	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
	}
	size_t index_offset = 0;
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
			// Optional: vertex normals
			if (idx.normal_index >= 0) {
				normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
			}
		}
		index_offset += fv;
	}
}

string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;
	vector<GLfloat> normals;

	string err;
	string warn;

	string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Material size %d\n", int(shapes.size()), int(materials.size()));
	model tmp_model;

	vector<PhongMaterial> allMaterial;
	for (int i = 0; i < materials.size(); i++)
	{
		PhongMaterial material;
		material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
		material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
		allMaterial.push_back(material);
	}

	for (int i = 0; i < shapes.size(); i++)
	{

		vertices.clear();
		colors.clear();
		normals.clear();
		normalization(&attrib, vertices, colors, normals, &shapes[i]);
		// printf("Vertices size: %d", vertices.size() / 3);

		Shape tmp_shape;
		glGenVertexArrays(1, &tmp_shape.vao);
		glBindVertexArray(tmp_shape.vao);

		glGenBuffers(1, &tmp_shape.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		tmp_shape.vertex_count = vertices.size() / 3;

		glGenBuffers(1, &tmp_shape.p_color);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
		glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &tmp_shape.p_normal);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// not support per face material, use material of first face
		if (allMaterial.size() > 0)
			tmp_shape.material = allMaterial[shapes[i].mesh.material_ids[0]];
		tmp_model.shapes.push_back(tmp_shape);
	}
	shapes.clear();
	materials.clear();
	models.push_back(tmp_model);
}

void initParameter()
{
	// [TODO] Setup some parameters if you need
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.001;
	proj.farClip = 100.0;
	proj.fovy = 80;
	proj.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

	setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
	vector<string> model_list{ "../NormalModels/bunny5KN.obj", "../NormalModels/dragon10KN.obj", "../NormalModels/lucy25KN.obj", "../NormalModels/teapot4KN.obj", "../NormalModels/dolphinN.obj"};
	// [TODO] Load five model at here
	for (int i = 0; i < model_list.size(); i++) {
		LoadModels(model_list[i]);
	}
}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}


int main(int argc, char **argv)
{
    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "109062134_HW2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
	// register glfw callback functions
    glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

    glfwSetFramebufferSizeCallback(window, ChangeSize);
	glEnable(GL_DEPTH_TEST);
	// Setup render context
	setupRC();

	// main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
        RenderScene();
        
        // swap buffer from back to front
        glfwSwapBuffers(window);
        
        // Poll input event
        glfwPollEvents();
    }
	
	// just for compatibiliy purposes
	return 0;
}
