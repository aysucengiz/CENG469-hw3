#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include <GL/glew.h>
//#include <OpenGL/gl3.h>   // The GL Header File
#include <GLFW/glfw3.h> // The GLFW header
#include <glm/glm.hpp> // GL Math library header
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

using namespace std;


#define TOTAL_OBJ 5

////////////// INITS FOR THE3 /////////////////
GLint pointSize = 10;
GLint pointCount = 10;



///////// INITS FOR OPENGL ////////////////////
GLuint vao[TOTAL_OBJ];
GLuint gProgram[TOTAL_OBJ];
unsigned int texture;
int gWidth = 640, gHeight = 480;
bool particleMove = true;
double xpos = -1;
double ypos = -1;
bool vsync = true;

GLint modelingMatrixLoc[TOTAL_OBJ];
GLint viewingMatrixLoc[TOTAL_OBJ];
GLint projectionMatrixLoc[TOTAL_OBJ];
GLint eyePosLoc[TOTAL_OBJ];
GLint lightPos1Loc[TOTAL_OBJ];

glm::mat4 projectionMatrix;
glm::mat4 viewingMatrix;
glm::mat4 modelingMatrix;
glm::mat4 modelingMatrix_quad;
bool initModelingMatrix_quad = false;
glm::vec3 eyePos(0, 0, 0);
glm::vec3 lightPos1Init(10,7, 10);
glm::vec3 lightPos1(0, 0, -8);
glm::vec3 eyeGaze(0, 0, -1);
glm::vec3 eyeUp(0, 1, 0);

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    GLuint TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
    GLuint Advance;    // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;

struct Vertex
{
	Vertex(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) {}
	GLfloat x, y, z;

    Vertex operator+(const Vertex& other) const {
        return Vertex(x + other.x, y + other.y, z + other.z);
    }

    Vertex operator-(const Vertex& other) const {
        return Vertex(x - other.x, y - other.y, z - other.z);
    }

    Vertex operator*(GLfloat scalar) const {
        return Vertex(x * scalar, y * scalar, z * scalar);
    }

    friend Vertex operator*(GLfloat scalar, const Vertex& v) {
        return Vertex(v.x * scalar, v.y * scalar, v.z * scalar);
    }

    friend Vertex operator/(const Vertex& v,GLfloat scalar) {
        return Vertex(v.x / scalar, v.y / scalar, v.z / scalar);
    }

    GLfloat dot_product(const Vertex& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    Vertex cross_product(const Vertex& other) const {
        return Vertex(
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x
        );
    }

    GLfloat mag(){
        return sqrt(x*x+y*y+z*z);
    }
};

struct Texture
{
	Texture(GLfloat inU, GLfloat inV) : u(inU), v(inV) {}
	GLfloat u, v;
};

struct Normal
{
	Normal(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) {}
	GLfloat x, y, z;

    Normal operator+(const Normal& other) const {
        return Normal(x + other.x, y + other.y, z + other.z);
    }

    Normal operator-(const Normal& other) const {
        return Normal(x - other.x, y - other.y, z - other.z);
    }

    Normal operator*(GLfloat scalar) const {
        return Normal(x * scalar, y * scalar, z * scalar);
    }

    friend Normal operator*(GLfloat scalar, const Normal& v) {
        return Normal(v.x * scalar, v.y * scalar, v.z * scalar);
    }

    friend Normal operator/(const Normal& v,GLfloat scalar) {
        return Normal(v.x / scalar, v.y / scalar, v.z / scalar);
    }

    GLfloat dot_product(const Normal& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    Normal cross_product(const Normal& other) const {
        return Normal(
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x
        );
    }

    int mag(){
        return sqrt(x*x+y*y+z*z);
    }
};

struct Face
{
	Face(int v[], int t[], int n[]) {
		vIndex[0] = v[0];
		vIndex[1] = v[1];
		vIndex[2] = v[2];
		tIndex[0] = t[0];
		tIndex[1] = t[1];
		tIndex[2] = t[2];
		nIndex[0] = n[0];
		nIndex[1] = n[1];
		nIndex[2] = n[2];
	}
	GLuint vIndex[3], tIndex[3], nIndex[3];
};

vector<Vertex> gVertices[TOTAL_OBJ];
vector<Texture> gTextures[TOTAL_OBJ];
vector<Normal> gNormals[TOTAL_OBJ];
vector<Face> gFaces[TOTAL_OBJ];

GLuint gVertexAttribBuffer[TOTAL_OBJ], gIndexBuffer[TOTAL_OBJ], gTextVBO;
GLint gInVertexLoc[TOTAL_OBJ], gInNormalLoc[TOTAL_OBJ];
int gVertexDataSizeInBytes[TOTAL_OBJ], gNormalDataSizeInBytes[TOTAL_OBJ], gTextureDataSizeInBytes[TOTAL_OBJ];

bool ParseObj(const string& fileName, int objId)
{
	fstream myfile;

	// Open the input 
	myfile.open(fileName.c_str(), std::ios::in);

	if (myfile.is_open())
	{
		string curLine;

		while (getline(myfile, curLine))
		{
			stringstream str(curLine);
			GLfloat c1, c2, c3;
			GLuint index[9];
			string tmp;

			if (curLine.length() >= 2)
			{
				if (curLine[0] == 'v')
				{
					if (curLine[1] == 't') // texture
					{
						str >> tmp; // consume "vt"
						str >> c1 >> c2;
						gTextures[objId].push_back(Texture(c1, c2));
					}
					else if (curLine[1] == 'n') // normal
					{
						str >> tmp; // consume "vn"
						str >> c1 >> c2 >> c3;
						gNormals[objId].push_back(Normal(c1, c2, c3));
					}
					else // vertex
					{
						str >> tmp; // consume "v"
						str >> c1 >> c2 >> c3;
						gVertices[objId].push_back(Vertex(c1, c2, c3));
					}
				}
				else if (curLine[0] == 'f') // face
				{
					str >> tmp; // consume "f"
					char c;
					int vIndex[3], nIndex[3], tIndex[3];
					str >> vIndex[0]; str >> c >> c; // consume "//"
					str >> nIndex[0];
					str >> vIndex[1]; str >> c >> c; // consume "//"
					str >> nIndex[1];
					str >> vIndex[2]; str >> c >> c; // consume "//"
					str >> nIndex[2];

					assert(vIndex[0] == nIndex[0] &&
						vIndex[1] == nIndex[1] &&
						vIndex[2] == nIndex[2]); // a limitation for now

					// make indices start from 0
					for (int c = 0; c < 3; ++c)
					{
						vIndex[c] -= 1;
						nIndex[c] -= 1;
						tIndex[c] -= 1;
					}

					gFaces[objId].push_back(Face(vIndex, tIndex, nIndex));
				}
				else
				{
					cout << "Ignoring unidentified line in obj file: " << curLine << endl;
				}
			}

			//data += curLine;
			if (!myfile.eof())
			{
				//data += "\n";
			}
		}

		myfile.close();
	}
	else
	{
		return false;
	}
    std::cout << "gVertices[objId].size():" << gVertices[objId].size() << std::endl;
	assert(gVertices[objId].size() == gNormals[objId].size());

	return true;
}

bool ReadDataFromFile(
	const string& fileName, ///< [in]  Name of the shader file
	string& data)     ///< [out] The contents of the file
{
	fstream myfile;

	// Open the input 
	myfile.open(fileName.c_str(), std::ios::in);

	if (myfile.is_open())
	{
		string curLine;

		while (getline(myfile, curLine))
		{
			data += curLine;
			if (!myfile.eof())
			{
				data += "\n";
			}
		}

		myfile.close();
	}
	else
	{
		return false;
	}

	return true;
}

GLuint createVS(const char* shaderName)
{
	string shaderSource;

	string filename(shaderName);
	if (!ReadDataFromFile(filename, shaderSource))
	{
		cout << "Cannot find file name: " + filename << endl;
		exit(-1);
	}

	GLint length = shaderSource.length();
	const GLchar* shader = (const GLchar*)shaderSource.c_str();

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &shader, &length);
	glCompileShader(vs);

	char output[1024] = { 0 };
	glGetShaderInfoLog(vs, 1024, &length, output);
	printf("VS compile log: %s\n", output);

	return vs;
}

GLuint createFS(const char* shaderName)
{
	string shaderSource;

	string filename(shaderName);
	if (!ReadDataFromFile(filename, shaderSource))
	{
		cout << "Cannot find file name: " + filename << endl;
		exit(-1);
	}

	GLint length = shaderSource.length();
	const GLchar* shader = (const GLchar*)shaderSource.c_str();

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &shader, &length);
	glCompileShader(fs);

	char output[1024] = { 0 };
	glGetShaderInfoLog(fs, 1024, &length, output);
	printf("FS compile log: %s\n", output);

	return fs;
}


unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
unsigned int gBuffer;
unsigned int gPosition, gNormal, gAlbedoSpec, rboDepth;

void initGBuffer(){
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);


    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, gWidth, gHeight, 0, GL_RGB, GL_FLOAT, nullptr);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gPosition, 0);

    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, gWidth, gHeight, 0, GL_RGB, GL_FLOAT, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, gWidth, gHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, gAlbedoSpec, 0);

    glDrawBuffers(3, attachments);

    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, gWidth, gHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "G-buffer framebuffer not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);


}

unsigned int attachmentsBlur[1] = { GL_COLOR_ATTACHMENT0};
unsigned int gBlurbuffer;
unsigned int gTexture;
GLuint gBlurDepth;


void initBlurBuffer(){
    glGenFramebuffers(1, &gBlurbuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBlurbuffer);

    glGenTextures(1, &gTexture);
    glBindTexture(GL_TEXTURE_2D, gTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, gWidth, gHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, gTexture, 0);

    glDrawBuffers(1, attachmentsBlur);



    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "G-buffer framebuffer not complete!" << std::endl;
    }


    glGenTextures(1, &gBlurDepth);
    glBindTexture(GL_TEXTURE_2D, gBlurDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, gWidth, gHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gBlurDepth, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);


}


void resizeText(int windowWidth, int windowHeight){
    initGBuffer();
    initBlurBuffer();
    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(windowWidth), 0.0f, static_cast<GLfloat>(windowHeight));
    glUseProgram(gProgram[2]);
    glUniformMatrix4fv(glGetUniformLocation(gProgram[2], "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}


void initFonts(int windowWidth, int windowHeight)
{
    // Set OpenGL options
    //glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::cout << "windowWidth = " << windowWidth << std::endl;
    std::cout << "windowHeight = " << windowHeight << std::endl;

    resizeText(windowWidth,windowHeight);

    // FreeType
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    }

    // Load font as face
    FT_Face face;
    if (FT_New_Face(ft, "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 0, &face))
    //if (FT_New_Face(ft, "/usr/share/fonts/truetype/gentium-basic/GenBkBasR.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    }

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

    // Load first 128 characters of ASCII set
    for (GLubyte c = 0; c < 128; c++)
    {
        // Load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
                );
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            (GLuint) face->glyph->advance.x
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    //
    // Configure VBO for texture quads
    //
    GLuint vaoLocal, vbo;
    glGenVertexArrays(1, &vaoLocal);
    assert(vaoLocal > 0);
    vao[2] = vaoLocal;
    glBindVertexArray(vaoLocal);

    glGenBuffers(1, &gTextVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLfloat getFPS(){
    static double prevTime=0;
    double currTime;
    static double fps = 0;
    static int counter = 0;
    counter++;
    currTime = glfwGetTime();

    double elapsed = currTime - prevTime;
    if(elapsed > 1.0 ) {

        prevTime = currTime;
        fps = counter;
        counter = 0;
        std::cout << "FPS: " << fps << std::endl;

    }
    return fps;
}

void renderText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
    // Activate corresponding render state
    glUseProgram(gProgram[2]);
    glUniform3f(glGetUniformLocation(gProgram[2], "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        // Update VBO for each character
        GLfloat vertices[6][4] = {
                { xpos,     ypos + h,   0.0, 0.0 },
                { xpos,     ypos,       0.0, 1.0 },
                { xpos + w, ypos,       1.0, 1.0 },

                { xpos,     ypos + h,   0.0, 0.0 },
                { xpos + w, ypos,       1.0, 1.0 },
                { xpos + w, ypos + h,   1.0, 0.0 }
        };

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        glBindVertexArray(vao[2]);
        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

        //glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)

        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

std::string printFloat(float x) {
    stringstream s;
    s << x;
    return s.str();
}

int pressedKey;
double keyStart = 0.0;
double keyMax = 1.0;

void writeText(){
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    if(keyStart>0 && glfwGetTime()<keyStart+keyMax){
        renderText(std::string(1, static_cast<char>(pressedKey)), gWidth-50, 20, 1.0, glm::vec3(0, 0, 1));
    }else{
        keyStart=0.0;
    }

    // render the text
    std::string exposureText = "exposure = " ;
    std::string vsyncText = "vsync = " + std::string(vsync ? "true": "false");
    std::string motionBlurText = "motionblur = " ;
    std::string keyText = "key = ";
    std::string gammaCorrText = "gamma = " ;
    std::string fpsText = "FPS: " + std::to_string((int) getFPS());


    //fps
    int fpsTextHeight = gHeight - 25;
    float fpsTextWidth = 0.6;
    renderText(fpsText.c_str(), 0, fpsTextHeight, fpsTextWidth, glm::vec3(1, 1, 0));

    // values
    int spacing=25;
    int valueTextHeight = spacing * 5;
    int valueTextX = 10;
    float valueTextWidth = 0.5;
    renderText(exposureText.c_str(), valueTextX, valueTextHeight, valueTextWidth, glm::vec3(1, 1, 0)); valueTextHeight-=spacing;
    renderText(vsyncText, valueTextX, valueTextHeight, valueTextWidth, glm::vec3(1, 1, 0)); valueTextHeight-=spacing;
    renderText(motionBlurText, valueTextX, valueTextHeight, valueTextWidth, glm::vec3(1, 1, 0)); valueTextHeight-=spacing;
    renderText(keyText, valueTextX, valueTextHeight, valueTextWidth, glm::vec3(1, 1, 0)); valueTextHeight-=spacing;
    renderText(gammaCorrText, valueTextX, valueTextHeight, valueTextWidth, glm::vec3(1, 1, 0));

    // modes
    int modeTextY = gHeight - 25;
    int modeTextX = gWidth - 320;
    float modeTextScale = 0.6;
    glm::vec3 textVec = glm::vec3(1, 0, 0);
    renderText("TONEMAPPED", modeTextX, modeTextY, modeTextScale, textVec);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void initVBO()
{
    for (size_t t = 0; t < 5; t++) // 2 objects.-> haha no t=0 is armadillo, t=1 is background quad.
    {
        if(t==2) continue;
        glGenVertexArrays(1, &vao[t]);
        assert(vao[t] > 0);

        glBindVertexArray(vao[t]);
        cout << "vao = " << vao[t] << endl;

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        assert(glGetError() == GL_NONE);

        glGenBuffers(1, &gVertexAttribBuffer[t]);
        glGenBuffers(1, &gIndexBuffer[t]);

        assert(gVertexAttribBuffer[t] > 0 && gIndexBuffer[t] > 0);

        glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer[t]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer[t]);

        gVertexDataSizeInBytes[t] = gVertices[t].size() * 3 * sizeof(GLfloat);
        gNormalDataSizeInBytes[t] = gNormals[t].size() * 3 * sizeof(GLfloat);
        gTextureDataSizeInBytes[t] = gTextures[t].size() * 2 * sizeof(GLfloat);
        int indexDataSizeInBytes = gFaces[t].size() * 3 * sizeof(GLuint);

        GLfloat* vertexData = new GLfloat[gVertices[t].size() * 3];
        GLfloat* normalData = new GLfloat[gNormals[t].size() * 3];
        GLfloat* textureData = new GLfloat[gTextures[t].size() * 2];
        GLuint* indexData = new GLuint[gFaces[t].size() * 3];

        float minX = 1e6, maxX = -1e6;
        float minY = 1e6, maxY = -1e6;
        float minZ = 1e6, maxZ = -1e6;

        for (int i = 0; i < gVertices[t].size(); ++i)
        {
            vertexData[3 * i] = gVertices[t][i].x;
            vertexData[3 * i + 1] = gVertices[t][i].y;
            vertexData[3 * i + 2] = gVertices[t][i].z;

            minX = std::min(minX, gVertices[t][i].x);
            maxX = std::max(maxX, gVertices[t][i].x);
            minY = std::min(minY, gVertices[t][i].y);
            maxY = std::max(maxY, gVertices[t][i].y);
            minZ = std::min(minZ, gVertices[t][i].z);
            maxZ = std::max(maxZ, gVertices[t][i].z);
        }

        std::cout << "minX = " << minX << std::endl;
        std::cout << "maxX = " << maxX << std::endl;
        std::cout << "minY = " << minY << std::endl;
        std::cout << "maxY = " << maxY << std::endl;
        std::cout << "minZ = " << minZ << std::endl;
        std::cout << "maxZ = " << maxZ << std::endl;

        for (int i = 0; i < gNormals[t].size(); ++i)
        {
            normalData[3 * i] = gNormals[t][i].x;
            normalData[3 * i + 1] = gNormals[t][i].y;
            normalData[3 * i + 2] = gNormals[t][i].z;
        }

        for (int i = 0; i < gTextures[t].size(); ++i)
        {
            textureData[2 * i] = gTextures[t][i].u;
            textureData[2 * i + 1] = gTextures[t][i].v;
        }

        for (int i = 0; i < gFaces[t].size(); ++i)
        {
            indexData[3 * i] = gFaces[t][i].vIndex[0];
            indexData[3 * i + 1] = gFaces[t][i].vIndex[1];
            indexData[3 * i + 2] = gFaces[t][i].vIndex[2];
        }


        glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes[t] + gNormalDataSizeInBytes[t] + gTextureDataSizeInBytes[t], 0, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, gVertexDataSizeInBytes[t], vertexData);
        glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes[t], gNormalDataSizeInBytes[t], normalData);
        glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes[t] + gNormalDataSizeInBytes[t], gTextureDataSizeInBytes[t], textureData);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSizeInBytes, indexData, GL_STATIC_DRAW);

        // done copying; can free now
        delete[] vertexData;
        delete[] normalData;
        delete[] textureData;
        delete[] indexData;

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes[t]));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes[t] + gNormalDataSizeInBytes[t]));
    }
}

GLuint fs1=0;
void bindShader(std::string filename_vert,std::string filename_frag,  int programId){

    if(gProgram[programId]) glDeleteProgram(programId);
    gProgram[programId] = glCreateProgram();
    fs1 = createFS(filename_frag.c_str());
    glAttachShader(gProgram[programId], fs1);

    GLuint vs1 = createVS(filename_vert.c_str());
    glAttachShader(gProgram[programId], vs1);

    glLinkProgram(gProgram[programId]);
    GLint status;
    glGetProgramiv(gProgram[programId], GL_LINK_STATUS, &status);

    if (status != GL_TRUE)
    {
        cout << "Program link failed for " << programId << endl;
        exit(-1);
    }
    int i=programId;
    glUseProgram(gProgram[i]);

    modelingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "modelingMatrix");
    viewingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "viewingMatrix");
    projectionMatrixLoc[i] = glGetUniformLocation(gProgram[i], "projectionMatrix");
    eyePosLoc[i] = glGetUniformLocation(gProgram[i], "eyePos");
    lightPos1Loc[i] = glGetUniformLocation(gProgram[i], "lightPos1");
}

void initShaders()
{
    bindShader("vert.glsl","frag.glsl",0); //for armadillo
}


unsigned int quadVAO[2] = {0,0},  quadVBO[2];

void initTexture(int id) {

    float quadVertices[] = {
            // position   // texCoords
            -1.0f,  1.0f,   0.0f, 1.0f,
            -1.0f, -1.0f,   0.0f, 0.0f,
            1.0f, -1.0f,   1.0f, 0.0f,

            -1.0f,  1.0f,   0.0f, 1.0f,
            1.0f, -1.0f,   1.0f, 0.0f,
            1.0f,  1.0f,   1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO[id]);
    glGenBuffers(1, &quadVBO[id]);

    glBindVertexArray(quadVAO[id]);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO[id]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void init()
{
	//ParseObj("armadillo.obj", 0);
	//ParseObj("cubemap.obj", 1);
	//ParseObj("quad.obj", 3);

	glEnable(GL_DEPTH_TEST);
	initTexture(0);
    initTexture(1);
	initShaders();
	initVBO();
    initFonts(gWidth, gHeight);
    initGBuffer();
    initBlurBuffer();
}




void setUniforms(size_t t){
    glUseProgram(gProgram[t]);
    glUniformMatrix4fv(projectionMatrixLoc[t], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(viewingMatrixLoc[t], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
    glUniformMatrix4fv(modelingMatrixLoc[t], 1, GL_FALSE, glm::value_ptr(modelingMatrix_quad));
    glUniform3fv(eyePosLoc[t], 1, glm::value_ptr(eyePos));
    glUniform3fv(lightPos1Loc[t], 1, glm::value_ptr(lightPos1));
    if (t == 3) {
        glUniform1i(glGetUniformLocation(gProgram[t], "gPosition"), 0);
        glUniform1i(glGetUniformLocation(gProgram[t], "gNormal"), 1);
        glUniform1i(glGetUniformLocation(gProgram[t], "gAlbedoSpec"), 2);
    }else if(t==4) {
        glUniform1i(glGetUniformLocation(gProgram[t], "gTexture"), 0);

    }
}

void drawObj(size_t t){
    if(t >= 3)
        glBindVertexArray(quadVAO[t-3]);
    else
        glBindVertexArray(vao[t]);

    if (t == 1|| t==2 || t==3) {
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
    }

    if(t>=3) glDrawArrays(GL_TRIANGLES, 0, 6);
    else     glDrawElements(GL_TRIANGLES, gFaces[t].size() * 3, GL_UNSIGNED_INT, 0);

    if (t == 1 || t==2 || t==3) {
        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
    }
}

int sizeText = log2(std::max(gWidth, gHeight));


void drawScene()
{

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);



    glDisable(GL_BLEND);
    // geometry pass
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glDrawBuffers(3, attachments);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);
    setUniforms(0);
    drawObj(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    // Lighting pass
    glBindFramebuffer(GL_FRAMEBUFFER, gBlurbuffer);
    glDrawBuffers(1, attachmentsBlur);
    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glBlitNamedFramebuffer(gBuffer, gBlurbuffer, 0, 0, gWidth, gHeight, 0, 0, gWidth, gHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, gPosition);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTextureBarrier();

    setUniforms(3);
    drawObj(3);

    // render cubemap
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glTextureBarrier();
    setUniforms(1);
    drawObj(1);


    // Render composite
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBlitNamedFramebuffer(gBlurbuffer, 0, 0, 0, gWidth, gHeight, 0, 0, gWidth, gHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, gTexture);
    glTextureBarrier();
    setUniforms(4);
    drawObj(4);

}


void display(GLFWwindow *window)
{

    glClearColor(0, 0, 0, 0);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Compute the modeling matrix
    static float changePitch = (float)(1.0 / 180.f) * M_PI;
    if(particleMove) {
        glm::quat pitchQuat(cos(changePitch / 2), 0, 1 * sin(changePitch / 2), 0);
        modelingMatrix *= glm::toMat4(pitchQuat);
    }

    // Draw the scene
    if(!initModelingMatrix_quad){
        glm::mat4 matT = glm::translate(glm::mat4(1.0), glm::vec3(-0.0f, -0.3f, -4.2f));
        modelingMatrix = matT;
        modelingMatrix_quad = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        initModelingMatrix_quad = true;
    }
    lightPos1= glm::vec3( viewingMatrix * modelingMatrix_quad * glm::vec4(lightPos1Init, 1));

    drawScene();
    writeText();

}


void reshape(GLFWwindow* window, int w, int h)
{
	w = w < 1 ? 1 : w;
	h = h < 1 ? 1 : h;

	gWidth = w;
	gHeight = h;

	glViewport(0, 0, w, h);


	float fovyRad = (float)(90.0 / 180.0) * M_PI;
	projectionMatrix = glm::perspective(fovyRad, (float)w/(float)h, 1.0f, 1000.0f);

	viewingMatrix = glm::lookAt(eyePos, eyePos + eyeGaze, eyeUp);

    resizeText(gWidth,gHeight);
    sizeText = log2(std::max(gWidth, gHeight));
}

///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// KEYBOARD & MOUSE /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        pressedKey = key;  // store the key code globally
        keyStart = glfwGetTime();
    }

    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    else if ((key == GLFW_KEY_KP_ADD  || key == GLFW_KEY_UP) && action == GLFW_PRESS)
    {

    }
    else if ((key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_DOWN) && action == GLFW_PRESS)
    {

    }
    else if (key == GLFW_KEY_0 && action == GLFW_PRESS)
    {

    }
    else if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        particleMove = !particleMove;
    }
    else if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {

    }
    else if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {

    }
    else if (key == GLFW_KEY_3 && action == GLFW_PRESS)
    {

    }
    else if (key == GLFW_KEY_4 && action == GLFW_PRESS)
    {

    }
    else if (key == GLFW_KEY_5 && action == GLFW_PRESS)
    {

    }
    else if (key == GLFW_KEY_6 && action == GLFW_PRESS)
    {

    }
    else if (key == GLFW_KEY_G && action == GLFW_PRESS)
    {

    }
    else if (key == GLFW_KEY_V && action == GLFW_PRESS)
    {
        vsync = !vsync;
        glfwSwapInterval(vsync);
    }
    else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        static bool fullscreen = false;
        static int pastgWidth = gWidth;
        static int pastgHeight = gHeight;
        static int wx;
        static int wy;
        fullscreen = ! fullscreen;
        if(fullscreen) {
            glfwGetWindowPos(window,&wx,&wy);
            pastgWidth = gWidth;
            pastgHeight = gHeight;
            GLFWmonitor *monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode *mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, 0);

        }else{
            glfwSetWindowMonitor(window, nullptr, wx, wy, pastgWidth, pastgHeight, 0);
        }
    }

}

void mouse(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_MIDDLE)
    {
        if(action == GLFW_PRESS){
            glfwGetCursorPos(window, &xpos, &ypos);
            std::cout << "mouse x,y: " << xpos << ","<< ypos<<std::endl;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MAINLOOP ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


void mainLoop(GLFWwindow* window)
{
	while (!glfwWindowShouldClose(window))
	{
		display(window);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

int main(int argc, char** argv)   // Create Main Function For Bringing It All Together
{
	GLFWwindow* window;
	if (!glfwInit())
	{
		exit(-1);
	}

    cout << argv[1] ;
    if(argc > 1) pointCount = atoi(argv[1]);
    if(argc > 2) pointSize = atoi(argv[2]);


	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	window = glfwCreateWindow(gWidth, gHeight, "CENG 469 THE3", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		exit(-1);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	// Initialize GLEW to setup the OpenGL Function pointers
	if (GLEW_OK != glewInit())
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return EXIT_FAILURE;
	}

	char rendererInfo[512] = { 0 };
	strcpy(rendererInfo, (const char*)glGetString(GL_RENDERER));
	strcat(rendererInfo, " - ");
	strcat(rendererInfo, (const char*)glGetString(GL_VERSION));
	glfwSetWindowTitle(window, rendererInfo);

	init();

    glfwSetMouseButtonCallback(window,mouse);
	glfwSetKeyCallback(window, keyboard);
	glfwSetWindowSizeCallback(window, reshape);

	reshape(window, gWidth, gHeight); // need to call this once ourselves
	mainLoop(window); // this does not return unless the window is closed

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
