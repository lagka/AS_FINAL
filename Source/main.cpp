#include "../Externals/Include/Include.h"
#include <set>
#include <fstream>
#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define WIDTH 800
#define HEIGHT 800

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

// Shader attributes
GLint iLocPosition;
GLint iLocNormal;
GLint iLocMVP;
GLint iLocf = 0;
GLint iLocMDiffuse, iLocMAmbient, iLocMSpecular, iLocMShininess, iLocambient, iLocdiffuse, iLocspecular, iLocdir, iLocpoint, iLocspot;
GLint iLocLDAmbient, iLocLDPosition, iLocRotate, iLocLDDiffuse, iLocLDSpecular;
GLint iLocLPPosition, iLocR;
GLint  iLocLPDiffuse, iLocLPSpecular, iLocLSDiffuse, iLocLSPosition, iLocSSepcular, iLocConstantAttenuation2, iLocQuadraticAttenuation2, iLocLinearAttenuation2;
GLint iLocSpotExponent, iLocSpotCutoff, iLocSpotCosCutoff, iLocSpotDirection, iLocSwitch, iLocConstantAttenuation, iLocLinearAttenuation, iLocQuadraticAttenuation;
GLint iLocLDPosition2;

using namespace glm;
using namespace std;

GLfloat skyboxVertices[] = {
	// Positions          
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,
	1.0f,  1.0f, -1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	1.0f, -1.0f,  1.0f
};
vector<const GLchar*> faces;
GLuint cubemapTexture;

struct _Shape {
	GLuint vao;
	GLuint vbo_position;
	GLuint vbo_normal;
	GLuint vbo_texcoord;
	GLuint ibo;
	int drawCount;
	int materialID;
};
struct _Material {
	GLuint diffuse_tex;
	aiColor3D ambient;
	aiColor3D diffuse;
	aiColor3D specular;
	string name;
	bool isTextured;
};

struct LightSourceParameters {
	float ambient[4];
	float diffuse[4];
	float specular[4];
	float position[4];
	float halfVector[4];
	float spotDirection[4];
	float spotExponent;
	float spotCutoff; // (range: [0.0,90.0], 180.0)
	float spotCosCutoff; // (range: [1.0,0.0],-1.0)
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
}typedef LightSource;

int mapH, mapW;
unsigned char *mapData;
LightSource lightsource[4];

vector<_Material> SceneMaterial;
vector<_Shape> SceneShape;
vector<_Material> CharMaterial;
vector<_Shape> CharShape;
vector<_Material> ShipMaterial;
vector<_Shape> ShipShape;

GLuint program;
GLuint mv_location, proj_location, texture_location, state_location, isTextured_location;
GLuint mA_location, mS_location, mD_location;
GLuint skyboxVAO, skyboxVBO;
GLuint Camera_location, CameraFront_location;
mat4 proj_matrix, view;
float viewportAspect;

vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
vec3 cameraFront = glm::vec3(1.0f, 0.0f, 0.0f);
vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
GLfloat _yaw = -90.0f;	// Yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right (due to how Eular angles work) so we initially rotate a bit to the left.
GLfloat _pitch = 0.0f;
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool firstMouse = true;

int Forward_Step = 0, MoveFore = 0;
int Backward_Step = 0, MoveBack = 0;
int Left_Step = 0, MoveLeft = 0;
int Right_Step = 0, MoveRight = 0;
int isThirdPerson = 0;
int stepsize = 50;
int obj_index[3];
int material_index[3];
int displayNo = 0;

const string SCENE_TEXTURE_PATH = "Textures/";
const float GROUND_HEIGHT = -770.0;
const float G = 9.8f;

char** loadShaderSource(const char* file)
{
	FILE* fp = fopen(file, "rb");
	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *src = new char[sz + 1];
	fread(src, sizeof(char), sz, fp);
	src[sz] = '\0';
	char **srcp = new char*[1];
	srcp[0] = src;
	return srcp;
}

void freeShaderSource(char** srcp)
{
	delete[] srcp[0];
	delete[] srcp;
}
void shaderLog(GLuint shader)
{
	GLint isCompiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		GLchar* errorLog = new GLchar[maxLength];
		glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

		printf("%s\n", errorLog);
		delete errorLog;
	}
}
// define a simple data structure for storing texture image raw data
typedef struct _TextureData
{
	_TextureData(void) :
		width(0),
		height(0),
		data(0)
	{
	}

	int width;
	int height;
	unsigned char* data;
} TextureData;

// load a png image and return a TextureData structure with raw data
// not limited to png format. works with any image format that is RGBA-32bit
TextureData loadPNG(const char* const pngFilepath)
{
	TextureData texture;
	int components;

	// load the texture with stb image, force RGBA (4 components required)
	stbi_uc *data = stbi_load(pngFilepath, &texture.width, &texture.height, &components, 4);

	// is the image successfully loaded?
	if (data != NULL)
	{
		// copy the raw data
		size_t dataSize = texture.width * texture.height * 4 * sizeof(unsigned char);
		texture.data = new unsigned char[dataSize];
		memcpy(texture.data, data, dataSize);

		// mirror the image vertically to comply with OpenGL convention
		for (size_t i = 0; i < texture.width; ++i)
		{
			for (size_t j = 0; j < texture.height / 2; ++j)
			{
				for (size_t k = 0; k < 4; ++k)
				{
					size_t coord1 = (j * texture.width + i) * 4 + k;
					size_t coord2 = ((texture.height - j - 1) * texture.width + i) * 4 + k;
					std::swap(texture.data[coord1], texture.data[coord2]);
				}
			}
		}

		// release the loaded image
		stbi_image_free(data);
		printf("success load PNG %s\n", pngFilepath);
	}
	else {
		printf("fail to load PNG %s\n", pngFilepath);
	}


	return texture;
}

GLuint loadCubemap(vector<const GLchar*> faces)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glActiveTexture(GL_TEXTURE0);

	int width, height;
	unsigned char* image;

	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	for (GLuint i = 0; i < faces.size(); i++)
	{
		TextureData texture = loadPNG(faces[i]);
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
			GL_RGBA8, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture.data
		);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	return textureID;
}
void setSkybox() {
	faces.push_back("skybox/right.png");
	faces.push_back("skybox/left.png");
	faces.push_back("skybox/top.png");
	faces.push_back("skybox/bottom.png");
	faces.push_back("skybox/front.png");
	faces.push_back("skybox/back.png");

	cubemapTexture = loadCubemap(faces);

	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
}
set<string> groundName = { "g qishang365_com647", "g qishang365_com346", "g qishang365_com347", "g qishang365_com837", "g qishang365_com348" };
vector<aiMesh*> grounds;
void LoadOBJ(std::string objfile, vector<_Material> &v_Material, vector<_Shape> &v_Shape, bool isTextured) {
	const aiScene *scene = aiImportFile(objfile.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);

	cout << "Load " << objfile << std::endl;
	cout << "# of meshs: " << scene->mNumMeshes << std::endl;
	cout << "# of materials: " << scene->mNumMaterials << std::endl;

	for (int i = 0; i < scene->mNumMaterials; i++) {
		aiMaterial *material = scene->mMaterials[i];
		_Material _material;
		aiString texturePath;

		material->Get(AI_MATKEY_COLOR_AMBIENT, _material.ambient);
		material->Get(AI_MATKEY_COLOR_SPECULAR, _material.specular);
		material->Get(AI_MATKEY_COLOR_DIFFUSE, _material.diffuse);
		
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS) {
			string path = texturePath.C_Str();
			TextureData texture;
			if(objfile == "AncientCity.obj") path = SCENE_TEXTURE_PATH + path;

			cout << texturePath.C_Str() << endl;
			_material.name = texturePath.C_Str();

			texture = loadPNG(path.c_str());
			glGenTextures(1, &_material.diffuse_tex);
			glBindTexture(GL_TEXTURE_2D, _material.diffuse_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture.data);
			glGenerateMipmap(GL_TEXTURE_2D);

			_material.isTextured = true;
		}
		else {
			_material.isTextured = false;
		}
		v_Material.push_back(_material);
	}

	for (int i = 0; i < scene->mNumMeshes; i++) {
		aiMesh *mesh = scene->mMeshes[i];
		_Shape _shape;

		if (groundName.find(string(mesh->mName.C_Str())) == groundName.end()) {}
		else {
			grounds.push_back(mesh);
			cout << grounds.size() << endl;
		}

		glGenVertexArrays(1, &_shape.vao);
		glBindVertexArray(_shape.vao);

		glGenBuffers(1, &_shape.vbo_normal);
		glGenBuffers(1, &_shape.vbo_position);
		glGenBuffers(1, &_shape.vbo_texcoord);
		glGenBuffers(1, &_shape.ibo);

		float *positions = new float[mesh->mNumVertices * 3];
		float *normals = new float[mesh->mNumVertices * 3];
		float *texCoords = new float[mesh->mNumVertices * 2];

		int idx = 0, tex_idx = 0;

		for (int v = 0; v < mesh->mNumVertices; v++, idx += 3, tex_idx += 2) {
			positions[idx] = mesh->mVertices[v][0];
			positions[idx + 1] = mesh->mVertices[v][1];
			positions[idx + 2] = mesh->mVertices[v][2];

			normals[idx] = mesh->mNormals[v][0];
			normals[idx + 1] = mesh->mNormals[v][1];
			normals[idx + 2] = mesh->mNormals[v][2];

			if (mesh->HasTextureCoords(0)) {
				texCoords[tex_idx] = mesh->mTextureCoords[0][v][0];
				texCoords[tex_idx + 1] = mesh->mTextureCoords[0][v][1];
			}
			else {
				texCoords[tex_idx] = 0;
				texCoords[tex_idx + 1] = 0;
			}
		}

		unsigned int *indice = new unsigned int[mesh->mNumFaces * 3];

		int face_idx = 0;
		for (int f = 0; f < mesh->mNumFaces; f++, face_idx += 3) {
			indice[face_idx] = mesh->mFaces[f].mIndices[0];
			indice[face_idx + 1] = mesh->mFaces[f].mIndices[1];
			indice[face_idx + 2] = mesh->mFaces[f].mIndices[2];
		}

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, _shape.vbo_position);
		glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(float) * 3, positions, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, _shape.vbo_texcoord);
		glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(float) * 2, texCoords, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, _shape.vbo_normal);
		glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * sizeof(float) * 3, normals, GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _shape.ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->mNumFaces * sizeof(unsigned int) * 3, indice, GL_STATIC_DRAW);

		_shape.materialID = mesh->mMaterialIndex;
		_shape.drawCount = mesh->mNumFaces * 3;

		v_Shape.push_back(_shape);
	}
	aiReleaseImport(scene);
}
void setLightingSource() {
	float PLRange = 100;

	lightsource[0].position[0] = 0;
	lightsource[0].position[1] = 0;
	lightsource[0].position[2] = -1;
	lightsource[0].position[3] = 1;
	lightsource[0].ambient[0] = 0.75;
	lightsource[0].ambient[1] = 0.75;
	lightsource[0].ambient[2] = 0.75;
	lightsource[0].ambient[3] = 1;
	//To Do : Setup your own lighting source parameters;

	//dirctional light 
	lightsource[1].position[0] = 0;
	lightsource[1].position[1] = 1000;
	lightsource[1].position[2] = 500;
	lightsource[1].position[3] = 1;
	lightsource[1].diffuse[0] = 1;
	lightsource[1].diffuse[1] = 1;
	lightsource[1].diffuse[2] = 1;
	lightsource[1].diffuse[3] = 1;
	lightsource[1].specular[0] = 1;
	lightsource[1].specular[1] = 1;
	lightsource[1].specular[2] = 1;
	lightsource[1].specular[3] = 1;
	lightsource[1].constantAttenuation = 1.0;
	lightsource[1].linearAttenuation = 4.5 / PLRange;
	lightsource[1].quadraticAttenuation = 75 / (PLRange* PLRange);

	//point light 
	lightsource[2].position[0] = 0;
	lightsource[2].position[1] = -1;
	lightsource[2].position[2] = 0;
	lightsource[2].position[3] = 1;
	lightsource[2].diffuse[0] = 1;
	lightsource[2].diffuse[1] = 1;
	lightsource[2].diffuse[2] = 1;
	lightsource[2].diffuse[3] = 1;
	lightsource[2].specular[0] = 1;
	lightsource[2].specular[1] = 1;
	lightsource[2].specular[2] = 1;
	lightsource[2].specular[3] = 1;
	lightsource[2].constantAttenuation = 1;
	lightsource[2].linearAttenuation = 4.5 / PLRange;
	lightsource[2].quadraticAttenuation = 75 / (PLRange* PLRange);

	//spot light
	lightsource[3].position[0] = 0;
	lightsource[3].position[1] = 0.15;
	lightsource[3].position[2] = 2;
	lightsource[3].position[3] = 1;
	lightsource[3].diffuse[0] = 1;
	lightsource[3].diffuse[1] = 1;
	lightsource[3].diffuse[2] = 1;
	lightsource[3].diffuse[3] = 1;
	lightsource[3].specular[0] = 1;
	lightsource[3].specular[1] = 1;
	lightsource[3].specular[2] = 1;
	lightsource[3].specular[3] = 1;
	lightsource[3].constantAttenuation = 1;
	lightsource[3].linearAttenuation = 4.5 / PLRange;
	lightsource[3].quadraticAttenuation = 75 / (PLRange* PLRange);
	lightsource[3].spotDirection[0] = 0;
	lightsource[3].spotDirection[1] = 0;
	lightsource[3].spotDirection[2] = -2;
	lightsource[3].spotDirection[3] = 0;
	lightsource[3].spotExponent = 0.1;
	lightsource[3].spotCutoff = 45;
	lightsource[3].spotCosCutoff = 0.96592582628;
}
int width, height;
int header_size;
float* terrain;
unsigned char *info;
unsigned char *heightmap;
unsigned char* readBMP(char* filename)
{
	int i;
	FILE* f = fopen(filename, "rb");
	/* find the header_size */
	fseek(f, 10, SEEK_SET);
	fread(&header_size, sizeof(unsigned int), 1, f);

	info = (unsigned char*)malloc(sizeof(unsigned char)*header_size);
	/* read the whole header */
	fseek(f, 0, SEEK_SET);
	fread(info, sizeof(unsigned char), header_size, f);

	/* find width and height from header info */
	width = *(int*)&info[18];
	height = *(int*)&info[22];

	/* each pixel contains 3 bytes representing RGB separately */
	int size = 3 * width * height;

	/* allocate 3 bytes per pixel */
	unsigned char* data = new unsigned char[size];
	/* we've read the whole header, now the file pointer
	is pointed to the starting point of the image pixel data */
	fread(data, sizeof(unsigned char), size, f);
	fclose(f);

	return data;
}
void setTerrain() {
	/* left-corner (-5650,-6200), y: -770 -> 74/255 */
	/*							  y: 3000 -> 255/255*/
	terrain = new float[height*width];
	float HeightPerUnit = (3000.0 + 1500.0) / 255.0 - 74.0;
	for (int i = 0; i < height*width; i++) {
		terrain[i] = -1500 + (heightmap[i] - 74.0) * HeightPerUnit;
	}
}
void My_Init()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	program = glCreateProgram();

	// Create customize shader by tell openGL specify shader type
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Load shader file
	char** vertexShaderSource = loadShaderSource("vertex.vs");
	char** fragmentShaderSource = loadShaderSource("fragment.fs");

	// Assign content of these shader files to those shaders we created before
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);

	// Free the shader file string(won't be used any more)
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);

	// Compile these shaders
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);

	// Assign the program we created before with these shaders
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	texture_location = glGetUniformLocation(program, "tex");
	mv_location = glGetUniformLocation(program, "um4mv");
	proj_location = glGetUniformLocation(program, "um4p");
	state_location = glGetUniformLocation(program, "state");
	isTextured_location = glGetUniformLocation(program, "isTextured");
	Camera_location = glGetUniformLocation(program, "cameraPos");
	CameraFront_location = glGetUniformLocation(program, "cameraFront");

	mA_location = glGetUniformLocation(program, "Material.ambient");
	mS_location = glGetUniformLocation(program, "Material.specular");
	mD_location = glGetUniformLocation(program, "Material.diffuse");
	/////////////////////////////////////////////////////////////////////
	iLocLDAmbient = glGetUniformLocation(program, "LightSource[0].ambient");
	iLocLDPosition = glGetUniformLocation(program, "LightSource[0].position");
	////////////////////////////////////////////////////////////////////////
	iLocLDPosition2 = glGetUniformLocation(program, "LightSource[1].position");
	iLocLDDiffuse = glGetUniformLocation(program, "LightSource[1].diffuse");
	iLocLDSpecular = glGetUniformLocation(program, "LightSource[1].specular");
	//////////////////////////////////////////////////////////////////////////
	iLocLPPosition = glGetUniformLocation(program, "LightSource[2].position");
	iLocLPDiffuse = glGetUniformLocation(program, "LightSource[2].diffuse");
	iLocLPSpecular = glGetUniformLocation(program, "LightSource[2].specular");
	iLocConstantAttenuation = glGetUniformLocation(program, "LightSource[2].constantAttenuation");
	iLocLinearAttenuation = glGetUniformLocation(program, "LightSource[2].linearAttenuation");
	iLocQuadraticAttenuation = glGetUniformLocation(program, "LightSource[2].quadraticAttenuation");
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	iLocLSPosition = glGetUniformLocation(program, "LightSource[3].position");
	iLocLSDiffuse = glGetUniformLocation(program, "LightSource[3].diffuse");
	iLocSSepcular = glGetUniformLocation(program, "LightSource[3].specular");
	iLocConstantAttenuation2 = glGetUniformLocation(program, "LightSource[3].constantAttenuation");
	iLocLinearAttenuation2 = glGetUniformLocation(program, "LightSource[3].linearAttenuation");
	iLocQuadraticAttenuation2 = glGetUniformLocation(program, "LightSource[3].quadraticAttenuation");
	iLocSpotExponent = glGetUniformLocation(program, "LightSource[3].spotExponent");
	iLocSpotCutoff = glGetUniformLocation(program, "LightSource[3].spotCutoff");
	iLocSpotCosCutoff = glGetUniformLocation(program, "LightSource[3].spotCosCutoff");
	iLocSpotDirection = glGetUniformLocation(program, "LightSource[3].spotDirection");
	iLocSwitch = glGetUniformLocation(program, "switch_light");
	iLocf = glGetUniformLocation(program, "f");
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	glUseProgram(program);

	setSkybox();

	LoadOBJ("AncientCity.obj", SceneMaterial, SceneShape, true);
	LoadOBJ("IronMan.obj", CharMaterial, CharShape, false);
	LoadOBJ("ship.obj", ShipMaterial, ShipShape, true);
	//heightmap = readBMP("1.bmp");
	setTerrain();
}
float getHeight(float x, float z) {
	int index = width * (width - (x + 5650) / 2) + ((z + 6200) / 2);
	return terrain[index];
}
double time = 0;
void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);

	MoveFore = Forward_Step - MoveFore;
	MoveBack = Backward_Step - MoveBack;
	MoveRight = Right_Step - MoveRight;
	MoveLeft = Left_Step - MoveLeft;

	vec3 PrevCameraPos = cameraPos;
	cameraPos += cameraFront * float(MoveFore * stepsize);
	cameraPos -= cameraFront * float(MoveBack * stepsize);
	cameraPos += normalize(cross(cameraFront, cameraUp)) * float(MoveRight * stepsize);
	cameraPos -= normalize(cross(cameraFront, cameraUp)) * float(MoveLeft * stepsize);

	if (cameraPos.y > GROUND_HEIGHT)
	{
		time += .01;
		float dist = 0.5 * G * pow(time, 2);

		cameraPos.y -= dist;
	}
	else {
		time = 0;
		cameraPos.y = GROUND_HEIGHT;
	}
	MoveFore = Forward_Step;
	MoveBack = Backward_Step;
	MoveRight = Right_Step;
	MoveLeft = Left_Step;

	vec3 ThirdPerson_offset = vec3(0.0f, 270.0f, 150.0f);
	vec3 FirstPerson_offset = vec3(0.0f, 200.0f, -15.0f);
	
	if (!isThirdPerson) {
		view = lookAt(cameraPos + FirstPerson_offset, cameraPos + FirstPerson_offset + cameraFront, cameraUp);
		glUniform4fv(Camera_location, 1, (value_ptr)(cameraPos + FirstPerson_offset));
	}
	else {
		view = lookAt(cameraPos + ThirdPerson_offset, cameraPos + ThirdPerson_offset + cameraFront, cameraUp);
		glUniform4fv(Camera_location, 1, (value_ptr)(cameraPos + ThirdPerson_offset));
	}

	proj_matrix = perspective(radians(45.0f), viewportAspect, 0.1f, 15000.0f);
	proj_matrix = proj_matrix * view;
	
	/* render scene */
	glUniform1i(state_location, 0);
	glUniform1i(isTextured_location, 1);


	glUniform4fv(iLocLDAmbient, 1, lightsource[0].ambient);
	glUniform4fv(iLocLDPosition, 1, lightsource[0].position);
	glUniform4fv(iLocLDPosition2, 1, lightsource[1].position);
	glUniform4fv(iLocLDDiffuse, 1, lightsource[1].diffuse);
	glUniform4fv(iLocLDSpecular, 1, lightsource[1].specular);

	/*************************************************************************************************/
	glUniform4fv(iLocLPDiffuse, 1, lightsource[2].diffuse);
	glUniform4fv(iLocLPSpecular, 1, lightsource[2].specular);
	glUniform4fv(iLocLPPosition, 1, lightsource[2].position);
	glUniform1f(iLocConstantAttenuation, lightsource[2].constantAttenuation);
	glUniform1f(iLocLinearAttenuation, lightsource[2].linearAttenuation);
	glUniform1f(iLocQuadraticAttenuation, lightsource[2].quadraticAttenuation);

	/*************************************************************************************************/
	glUniform4fv(iLocLSPosition, 1, lightsource[3].position);
	glUniform4fv(iLocLSDiffuse, 1, lightsource[3].diffuse);
	glUniform4fv(iLocSSepcular, 1, lightsource[3].specular);
	glUniform1f(iLocConstantAttenuation2, lightsource[3].constantAttenuation);
	glUniform1f(iLocLinearAttenuation2, lightsource[3].linearAttenuation);
	glUniform1f(iLocQuadraticAttenuation2, lightsource[3].quadraticAttenuation);
	glUniform1f(iLocSpotExponent, lightsource[3].spotExponent);
	glUniform1f(iLocSpotCutoff, lightsource[3].spotCutoff);
	glUniform1f(iLocSpotCosCutoff, lightsource[3].spotCosCutoff);
	glUniform4fv(iLocSpotDirection, 1, lightsource[3].spotDirection);

	/*************************************************************************************************/

	mat4 Identity(1.0);

	glUniformMatrix4fv(proj_location, 1, GL_FALSE, &proj_matrix[0][0]);
	glUniformMatrix4fv(mv_location, 1, GL_FALSE, &Identity[0][0]);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(texture_location, 0);

	for (int i = 0; i < SceneShape.size(); i++) {
		glBindVertexArray(SceneShape[i].vao);

		int mID = SceneShape[i].materialID;
		if (SceneMaterial[mID].isTextured == true) {
			glUniform1i(isTextured_location, 1);
			aiColor3D aD = SceneMaterial[mID].diffuse;
			vec4 D = vec4(aD.r, aD.g, aD.b, 0);
			glUniform4fv(mD_location, 1, value_ptr(D));
			glBindTexture(GL_TEXTURE_2D, SceneMaterial[mID].diffuse_tex);
		}
		else {
			glUniform1i(isTextured_location, 0);
			aiColor3D aD = SceneMaterial[mID].diffuse;
			aiColor3D aS = SceneMaterial[mID].specular;
			aiColor3D aA = SceneMaterial[mID].ambient;
			vec4 D = vec4(aD.r, aD.g, aD.b, 0);
			vec4 A = vec4(aA.r, aA.g, aA.b, 0);
			vec4 S = vec4(aS.g, aS.g, aS.b, 0);

			glUniform4fv(mA_location, 1, value_ptr(A));
			glUniform4fv(mD_location, 1, value_ptr(D));
			glUniform4fv(mS_location, 1, value_ptr(S));
		}
		glDrawElements(GL_TRIANGLES, SceneShape[i].drawCount, GL_UNSIGNED_INT, 0);
	}

	mat4 CharT = translate(Identity, cameraPos);
	mat4 CharR = rotate(Identity, -radians(_yaw-90), vec3(0.0f, 1.0f, 0.0f));
	mat4 Char_mv = CharT * CharR;

	glUniformMatrix4fv(mv_location, 1, GL_FALSE, &Char_mv[0][0]);

	glUniform1i(isTextured_location, 0);
	for (int i = 0; i < CharShape.size(); i++) {
		glBindVertexArray(CharShape[i].vao);
		int mID = CharShape[i].materialID;
		aiColor3D aD = CharMaterial[mID].diffuse;
		aiColor3D aS = CharMaterial[mID].specular;
		aiColor3D aA = CharMaterial[mID].ambient;
		vec4 D = vec4(aD.r, aD.g, aD.b, 0);
		vec4 A = vec4(aA.r, aA.g, aA.b, 0);
		vec4 S = vec4(aS.g, aS.g, aS.b, 0);

		glUniform4fv(mA_location, 1, value_ptr(A));
		glUniform4fv(mD_location, 1, value_ptr(D));
		glUniform4fv(mS_location, 1, value_ptr(S));

		glDrawElements(GL_TRIANGLES, CharShape[i].drawCount, GL_UNSIGNED_INT, 0);
	}
	mat4 ShipS = scale(Identity, vec3(0.2f, 0.2f, 0.2f));
	mat4 ShipT = translate(Identity, vec3(800.0f, -1200.0f, 800.0f));
	mat4 Ship_mv = ShipT * ShipS;
	glUniformMatrix4fv(mv_location, 1, GL_FALSE, &Ship_mv[0][0]);

	/* render skybox */
	for (int i = 0; i < ShipShape.size(); i++) {
		glBindVertexArray(ShipShape[i].vao);

		int mID = ShipShape[i].materialID;
		if (ShipMaterial[mID].isTextured == true) {
			glUniform1i(isTextured_location, 1);
			aiColor3D aD = ShipMaterial[mID].diffuse;
			vec4 D = vec4(aD.r, aD.g, aD.b, 0);
			glUniform4fv(mD_location, 1, value_ptr(D));
			glBindTexture(GL_TEXTURE_2D, ShipMaterial[mID].diffuse_tex);
		}
		else {
			glUniform1i(isTextured_location, 0);
			aiColor3D aD = ShipMaterial[mID].diffuse;
			aiColor3D aS = ShipMaterial[mID].specular;
			aiColor3D aA = ShipMaterial[mID].ambient;
			vec4 D = vec4(aD.r, aD.g, aD.b, 0);
			vec4 A = vec4(aA.r, aA.g, aA.b, 0);
			vec4 S = vec4(aS.g, aS.g, aS.b, 0);

			glUniform4fv(mA_location, 1, value_ptr(A));
			glUniform4fv(mD_location, 1, value_ptr(D));
			glUniform4fv(mS_location, 1, value_ptr(S));
		}
		glDrawElements(GL_TRIANGLES, ShipShape[i].drawCount, GL_UNSIGNED_INT, 0);
	}
	glUniform1i(state_location, 1);
	glUniform1i(isTextured_location, 1);


	mat4 S = scale(Identity, vec3(9000.0f, 9000.0f, 9000.0f));
	glUniformMatrix4fv(mv_location, 1, GL_FALSE, &S[0][0]);

	glDepthMask(GL_FALSE);

	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE1);
	glUniform1i(glGetUniformLocation(program, "skybox"), 1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glDepthMask(GL_TRUE);

	glutSwapBuffers();
}
void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);

	viewportAspect = (float)width / (float)height;
	proj_matrix = perspective(radians(45.0f), viewportAspect, 0.1f, 5000.0f);
	view = lookAt(vec3(-300.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

	proj_matrix = proj_matrix* view;
}

void My_Timer(int val)
{	
	glutPostRedisplay();
	glutTimerFunc(timer_speed, My_Timer, val);
}

void onMouseMotion(int x, int y)
{
	if (firstMouse)
	{
		lastX = x;
		lastY = y;
		firstMouse = false;
	}

	GLfloat xoffset = x - lastX;
	GLfloat yoffset = -(y - lastY); // Reversed since y-coordinates go from bottom to left
	lastX = x;
	lastY = y;

	GLfloat sensitivity = 0.3;	// Change this value to your liking
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	_yaw += xoffset;
	_pitch += yoffset;

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (_pitch > 89.0f)
		_pitch = 89.0f;
	if (_pitch < -89.0f)
		_pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
	front.y = sin(glm::radians(_pitch));
	front.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
	cameraFront = glm::normalize(front);
}

void My_Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'w':
		Forward_Step++;
		printf("camera pos: (%f,%f,%f)\n", cameraPos.x, cameraPos.y, cameraPos.z);
		break;
	case 's':
		Backward_Step++;
		break;
	case 'd':
		Right_Step++;
		break;
	case 'a':
		Left_Step++;
		break;
	case 'z':
		if (displayNo == 1) cameraPos.y++;
		else cameraPos.y += 100;
		break;
	case 'x':
		if (displayNo == 1) cameraPos.y--;
		else cameraPos.y -= 20;
		break;
	case 'q':
		isThirdPerson = !isThirdPerson;
		break;
	}
}

void My_SpecialKeys(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_F1:
		displayNo = (displayNo + 1) % 2;
		if (displayNo == 0) {
			cameraPos = vec3(0.0f, 0.0f, 0.0f);
			cameraFront = vec3(1.0f, 0.0f, 0.0f);
			cameraUp = vec3(0.0f, 1.0f, 0.0f);
			stepsize = 20;
		}
		else {
			cameraPos = vec3(0.0f, 3.0f, 0.0f);
			cameraFront = vec3(1.0f, 0.0f, 0.0f);
			cameraUp = vec3(0.0f, 1.0f, 0.0f);
			stepsize = 1;
		}
		break;
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}

void My_Menu(int id)
{
	switch (id)
	{
	case MENU_TIMER_START:
		if (!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	default:
		break;
	}
}

int main(int argc, char *argv[])
{
#ifdef __APPLE__
	// Change working directory to source code path
	chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow("AS2_Framework"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	glPrintContextInfo();
	My_Init();
	setLightingSource();

	// Create a menu and bind it to mouse right button.
	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddMenuEntry("Exit", MENU_EXIT);

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutPassiveMotionFunc(onMouseMotion);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0);

	// Enter main event loop.
	glutMainLoop();

	return 0;
}