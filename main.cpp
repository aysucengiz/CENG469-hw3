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

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    GLuint TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
    GLuint Advance;    // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;

int gWidth = 640, gHeight = 480;
GLfloat origin_x = (float) gWidth / 2.0,origin_y=  (float) gHeight / 2.0, startAge=1.0; // birth point

struct Vertex
{
    Vertex(GLfloat inX=origin_x, GLfloat inY=origin_y, GLfloat inZ=startAge, GLfloat inW=0.0) : x(inX), y(inY), z(inZ) {}
    GLfloat x, y, z, w;

    Vertex operator+(const Vertex& other) const {return Vertex(x + other.x, y + other.y, z + other.z);}
    Vertex operator-(const Vertex& other) const {return Vertex(x - other.x, y - other.y, z - other.z);}
    Vertex operator*(GLfloat scalar) const {return Vertex(x * scalar, y * scalar, z * scalar);}
    friend Vertex operator*(GLfloat scalar, const Vertex& v) {return Vertex(v.x * scalar, v.y * scalar, v.z * scalar);}
    friend Vertex operator/(const Vertex& v,GLfloat scalar) {return Vertex(v.x / scalar, v.y / scalar, v.z / scalar);}
};

///////// INITS FOR OPENGL ////////////////////
#define TOTAL_OBJ 3
#define TEXT_NUM 2
GLuint vao[TOTAL_OBJ];
GLuint gProgram[TOTAL_OBJ];
bool vsync = true;

GLint modelingMatrixLoc[TOTAL_OBJ];
GLint viewingMatrixLoc[TOTAL_OBJ];
GLint projectionMatrixLoc[TOTAL_OBJ];
GLint particleSizeLoc[TOTAL_OBJ];
GLint dtLoc[TOTAL_OBJ];
GLint currAttractorLoc[TOTAL_OBJ];
GLint origin_xLoc[TOTAL_OBJ];
GLint origin_yLoc[TOTAL_OBJ];
GLint gWidthLoc[TOTAL_OBJ];
GLint gHeightLoc[TOTAL_OBJ];
GLint startAgeLoc[TOTAL_OBJ];

glm::mat4 projectionMatrix;
glm::mat4 viewingMatrix;
glm::mat4 modelingMatrix_quad = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

GLuint gVertexAttribBuffer[TOTAL_OBJ], gIndexBuffer[TOTAL_OBJ], gTextVBO;
int gVertexDataSizeInBytes[TOTAL_OBJ], gNormalDataSizeInBytes[TOTAL_OBJ], gTextureDataSizeInBytes[TOTAL_OBJ];


////////////// THE3 //////////////
GLfloat particleSize = 10.0;
GLint particleCount = 10;
bool showText = true;
bool particleMove = true;
double xpos = -1;
double ypos = -1;

// Each particle will have its position, velocity, and age.
vector<Vertex> gParticles;  // xy: location of the particle z: age
vector<Vertex> gVelocity;

// There will be attractors in the scene, each with its own mass value set.
#define ATTRACTOR_COUNT 12
vector<Vertex> gAttractor(ATTRACTOR_COUNT);  // xy: location of the attractor z: mass

// Initially, the program should launch with some attractors put into the scene
#define DEFAULT_MASS 100.0
#define MASS_UNIT 10.0
#define MASS_MIN 10.0
#define MASS_MAX 1000.0
int currAttractor = 0;
GLfloat currMass = DEFAULT_MASS;
void addAttractor(GLfloat locx, GLfloat locy){
    if(currAttractor >= 12){
        for(int i=0; i<11; i++){
            gAttractor[i].x = gAttractor[i+1].x;
            gAttractor[i].y = gAttractor[i+1].y;
            gAttractor[i].z = gAttractor[i+1].z;
        }
        gAttractor[11].x =locx;
        gAttractor[11].y =locy;
        gAttractor[11].z = currMass;

    }else{
        gAttractor[currAttractor].x =locx;
        gAttractor[currAttractor].y =locy;
        gAttractor[currAttractor].z = currMass;
        currAttractor++;
    }

    currMass = DEFAULT_MASS;
}

void removeAttractor(){
    if(currAttractor != 0) {
        currAttractor--;
    }
}

void changeOrigin(){
    origin_y = ypos;
    origin_x = xpos;
}



// The velocity of a point is also recomputed by formulating a gravitational pull physics with all attractors in the scene.

// Points will be colored according to their current age.
// The points should be drawn blended into the frame so that overlapping points result in a blended color value.


// A delta time value can be used to animate the scene at some speed.
// This value should be editable by user interactions to slow down or speed up the runtime.
#define TIME_UNIT 0.1
#define TIME_MIN 0.0
#define TIME_MAX 10.0
GLfloat dt = 0.2;


// A 3-line text must be rendered on the bottom left of the screen.
// # of curr attractors = currAttractor + 1
// curr mass value = gAttractor[currAttractor].z
// mode = currMode (diff colour, origin or attractor)
enum mode{
    ORIGIN,
    ATTRACTOR
};
mode currMode = ORIGIN;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int buffers[2];
unsigned int bufferstex[2];
int gParticlesDataSizeInBytes,gVelocityDataSizeInBytes, gAttractorDataSizeInBytes;

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

GLuint createS(const char* shaderName, int shaderType)
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

    GLuint s;
    if(shaderType==0){
        s = glCreateShader(GL_FRAGMENT_SHADER);
    }else if(shaderType==1){
        s = glCreateShader(GL_VERTEX_SHADER);
    }else if(shaderType==2){
        s = glCreateShader(GL_COMPUTE_SHADER);
    }
    glShaderSource(s, 1, &shader, &length);
    glCompileShader(s);

    char output[1024] = { 0 };
    glGetShaderInfoLog(s, 1024, &length, output);
    printf("%s log: %s\n", shaderName, output);

    return s;
}

void resizeText(int windowWidth, int windowHeight){
    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(windowWidth), 0.0f, static_cast<GLfloat>(windowHeight));
    glUseProgram(gProgram[TEXT_NUM]);
    glUniformMatrix4fv(glGetUniformLocation(gProgram[TEXT_NUM], "projection"), 1, GL_FALSE, glm::value_ptr(projection));
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
    vao[TEXT_NUM] = vaoLocal;
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
    glUseProgram(gProgram[TEXT_NUM]);
    glUniform3f(glGetUniformLocation(gProgram[TEXT_NUM], "textColor"), color.x, color.y, color.z);
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

        glBindVertexArray(vao[TEXT_NUM]);
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
    int modeTextY = 0;
    int modeTextX = 0;
    float modeTextScale = 0.6;
    glm::vec3 textVec = glm::vec3(1, 0, 0);
    renderText("TONEMAPPED", modeTextX, modeTextY, modeTextScale, textVec);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void initVBO()
{
    int t= 0;
    glEnable(GL_PROGRAM_POINT_SIZE);
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "GL error before glBindVertexArray: " << std::hex << err << std::endl;
    }

    glGenVertexArrays(1, &vao[t]);
    assert(vao[t] > 0);

    std::cout << "vao =: " << vao[t] << std::endl;

    glBindVertexArray(vao[t]);
    err = glGetError(); if (err) std::cerr << "glBindVertexArray error: " << err << std::endl;

    glEnableVertexAttribArray(0);
    err = glGetError(); if (err) std::cerr << "glEnableVertexAttribArray error: " << err << std::endl;

    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void defineLocations(int i){
    glUseProgram(gProgram[i]);

    modelingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "modelingMatrix");
    viewingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "viewingMatrix");
    projectionMatrixLoc[i] = glGetUniformLocation(gProgram[i], "projectionMatrix");
    particleSizeLoc[i] = glGetUniformLocation(gProgram[i], "particleSize");
    dtLoc[i] = glGetUniformLocation(gProgram[i], "dt");
    currAttractorLoc[i] = glGetUniformLocation(gProgram[i], "currAttractor");
    origin_xLoc[i] = glGetUniformLocation(gProgram[i], "origin_x");
    origin_yLoc[i] = glGetUniformLocation(gProgram[i], "origin_y");
    gWidthLoc[i] = glGetUniformLocation(gProgram[i], "gWidth");
    gHeightLoc[i] = glGetUniformLocation(gProgram[i], "gHeight");
    startAgeLoc[i] = glGetUniformLocation(gProgram[i], "startAge");
}



GLuint fs1=0;
void bindShader(std::string filename_vert,std::string filename_frag,  int programId){

    if(gProgram[programId]) glDeleteProgram(programId);
    gProgram[programId] = glCreateProgram();
    fs1 = createS(filename_frag.c_str(),0);
    glAttachShader(gProgram[programId], fs1);

    GLuint vs1 = createS(filename_vert.c_str(),1);
    glAttachShader(gProgram[programId], vs1);

    glLinkProgram(gProgram[programId]);
    GLint status;
    glGetProgramiv(gProgram[programId], GL_LINK_STATUS, &status);

    if (status != GL_TRUE)
    {
        GLint logLength;
        glGetProgramiv(gProgram[programId], GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength);
        glGetProgramInfoLog(gProgram[programId], logLength, nullptr, log.data());
        std::cerr << "Program link failed for " << programId << ":\n" << log.data() << std::endl;
        exit(-1);
    }

    defineLocations(programId);
}


void bindComputeShader(std::string filename_comp,  int programId){

    if(gProgram[programId]) glDeleteProgram(programId);
    gProgram[programId] = glCreateProgram();

    GLuint cs1 = createS(filename_comp.c_str(),2);
    glAttachShader(gProgram[programId], cs1);

    glLinkProgram(gProgram[programId]);
    GLint status;
    glGetProgramiv(gProgram[programId], GL_LINK_STATUS, &status);

    if (status != GL_TRUE)
    {
        GLint logLength;
        glGetProgramiv(gProgram[programId], GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength);
        glGetProgramInfoLog(gProgram[programId], logLength, nullptr, log.data());
        std::cerr << "Program link failed for " << programId << ":\n" << log.data() << std::endl;
        exit(-1);
    }
    defineLocations(programId);
}


void initShaders()
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "GL error before initShaders: " << std::hex << err << std::endl;
    }
    bindShader("vert.glsl","frag.glsl",0);
    bindComputeShader("comp.glsl",1);
    bindShader("vert_text.glsl","frag_text.glsl",2);
    bindComputeShader("comp_text.glsl",3);
}



GLuint ubo;
void initComputeBuffers(){
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, 12 * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
    gAttractorDataSizeInBytes = gAttractor.size() * 4 * sizeof(GLfloat);

    GLfloat *attractorData = (GLfloat *) glMapBufferRange(GL_UNIFORM_BUFFER, 0, gAttractorDataSizeInBytes,
                                                         GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    if(attractorData) {
        for (int i = 0; i < 12; ++i) {
            attractorData[4 * i] = gAttractor[i].x;
            attractorData[4 * i + 1] = gAttractor[i].y;
            attractorData[4 * i + 2] = gAttractor[i].z;
            attractorData[4 * i + 3] = 0.0f;
        }
        glUnmapBuffer(GL_UNIFORM_BUFFER);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
    } else {
        std::cerr << "Failed to map attractor buffer!" << std::endl;
    }

    gParticlesDataSizeInBytes = gParticles.size() * 4 * sizeof(GLfloat);
    gVelocityDataSizeInBytes = gVelocity.size() * 4 * sizeof(GLfloat);
    cout << "Vertex struct size:" << sizeof(Vertex) << 4 * sizeof(GLfloat)<< endl;
    glGenBuffers(2, buffers);
    glGenTextures(2, bufferstex);

    /////////////////////// position
    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, gParticlesDataSizeInBytes, NULL, GL_DYNAMIC_COPY);

    GLfloat *positionData = (GLfloat *) glMapBufferRange(GL_ARRAY_BUFFER, 0, gParticlesDataSizeInBytes,
                          GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    if(positionData) {
        for (int i = 0; i < gParticles.size(); ++i) {
            positionData[4 * i] = gParticles[i].x;
            positionData[4 * i + 1] = gParticles[i].y;
            positionData[4 * i + 2] = gParticles[i].z;
            positionData[4 * i + 3] = 0.0f;
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);
    } else {
        std::cerr << "Failed to map position buffer!" << std::endl;
    }

    glBindTexture(GL_TEXTURE_BUFFER, bufferstex[0]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffers[0]);


    /////////////////////// velocity
    glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
    glBufferData(GL_ARRAY_BUFFER, gVelocityDataSizeInBytes, NULL, GL_DYNAMIC_COPY);

    GLfloat *velocityData= (GLfloat *) glMapBufferRange(GL_ARRAY_BUFFER, 0, gVelocityDataSizeInBytes,
                          GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    if (velocityData) {
        for (int i = 0; i < gVelocity.size(); ++i) {
            velocityData[4 * i] = gVelocity[i].x;
            velocityData[4 * i + 1] = gVelocity[i].y;
            velocityData[4 * i + 2] = gVelocity[i].z;
            velocityData[4 * i + 3] = 0.0f;
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);
    } else {
        std::cerr << "Failed to map position buffer!" << std::endl;
    }
    glBindTexture(GL_TEXTURE_BUFFER, bufferstex[1]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffers[1]);


    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "GL error after initComputeBuffers: " << std::hex << err << std::endl;
    }

}

void init()
{
    addAttractor(origin_x-100,origin_y-100);
	glEnable(GL_DEPTH_TEST);
    initComputeBuffers();
	initShaders();
	initVBO();
    initFonts(gWidth, gHeight);
}

void setUniforms(size_t t){
    glUseProgram(gProgram[t]);
    glUniformMatrix4fv(projectionMatrixLoc[t], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(viewingMatrixLoc[t], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
    glUniformMatrix4fv(modelingMatrixLoc[t], 1, GL_FALSE, glm::value_ptr(modelingMatrix_quad));
    glUniform1f(particleSizeLoc[t], particleSize);

    glUniform1f(dtLoc[t], dt);
    glUniform1i(currAttractorLoc[t], currAttractor);
    glUniform1f(origin_xLoc[t], origin_x);
    glUniform1f(origin_yLoc[t], origin_y);
    glUniform1i(gWidthLoc[t], gWidth);
    glUniform1i(gHeightLoc[t], gHeight);
    glUniform1f(startAgeLoc[t], startAge);
}

void drawObj(size_t t){
    glBindVertexArray(vao[t]);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glDrawArrays(GL_POINTS, 0, particleCount);

    glEnable(GL_DEPTH_TEST);
}

int sizeText = log2(std::max(gWidth, gHeight));

void drawScene()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    // arrange the buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // draw using frag and vert shaders
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glTextureBarrier();
    setUniforms(0);
    drawObj(0);

    cout << gAttractor[currAttractor-1].x << " " << gAttractor[currAttractor-1].y << " " << gAttractor[currAttractor-1].z << " "  << endl;
    //cout << currAttractor  << endl;
}


void display(GLFWwindow *window)
{
    // computations
    setUniforms(1);
    glBindImageTexture(0, bufferstex[1], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(1, bufferstex[0], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glDispatchCompute(particleCount / 128 + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // show on the screen
    drawScene();
    //writeText();
}


void reshape(GLFWwindow* window, int w, int h)
{
	w = w < 1 ? 1 : w;
	h = h < 1 ? 1 : h;

    /*origin_x *= w / gWidth;
    origin_y *=  h / gHeight;
    for(int i=0; i<12; i++){
        gAttractor[i].x *= w / gWidth;
        gAttractor[i].y *= h / gHeight;
    }*/
	gWidth = w;
	gHeight = h;

	glViewport(0, 0, w, h);

    projectionMatrix = glm::ortho( 0.0f,(float) w, (float) h,0.0f,-1.0f, 1.0f);

    resizeText(gWidth,gHeight);
    sizeText = log2(std::max(gWidth, gHeight));
}

///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// KEYBOARD & MOUSE /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        pressedKey = key;
        keyStart = glfwGetTime();
    }

    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    else if (key == GLFW_KEY_W && action == GLFW_PRESS)
    {
        std::cout << "dt: " << dt << std::endl;
        if(dt+TIME_UNIT <= TIME_MAX)dt += TIME_UNIT;
    }
    else if (key == GLFW_KEY_S  && action == GLFW_PRESS)
    {
        std::cout << "dt: " << dt << std::endl;
        if(dt-TIME_UNIT >= TIME_MIN)dt -= TIME_UNIT;
    }
    else if (key == GLFW_KEY_T && action == GLFW_PRESS)
    {
        showText = !showText;
    }
    else if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        particleMove = !particleMove;
    }
    else if (key == GLFW_KEY_G && action == GLFW_PRESS)
    {
        currMode = (mode) ((currMode + 1) % 2);
    }
    else if (key == GLFW_KEY_V && action == GLFW_PRESS)
    {
        vsync = !vsync;
        glfwSwapInterval(vsync);
    }
    else if (key == GLFW_KEY_F && action == GLFW_PRESS)
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
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        glfwGetCursorPos(window, &xpos, &ypos);
        std::cout << "mouse x,y: " << xpos << "," << ypos << std::endl;
        if (currMode == ORIGIN) {
            changeOrigin();
        } else if (currMode == ATTRACTOR) {
            addAttractor(xpos,ypos);
        }
    }else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        removeAttractor();
        std::cout << "Removed attractor" << std::endl;
    }
}

void scroll(GLFWwindow* window, double xoffset, double yoffset)
{
    currMass += yoffset * MASS_UNIT;
    currMass = currMass >= 0 ? currMass : 0.0;
    std::cout << "curr_mass: " << currMass  << std::endl;
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

    if(argc > 1) particleCount = atoi(argv[1]);
    if(argc > 2) particleSize = atoi(argv[2]);
    cout << "particleCount: " << particleCount<< endl;
    cout << "particleSize: " << particleSize<< endl;
    gParticles.resize(particleCount);
    for(int i=0; i<particleCount; i++){
        gParticles[i].z = startAge / (float) particleCount * i;
    }
    gVelocity.resize(particleCount);
    for(int x = 0; x < particleCount; x++) {
        gVelocity[x].x = (((double)rand()) / RAND_MAX - 0.5 )* 10.0;
        gVelocity[x].y = (((double)rand()) / RAND_MAX - 0.5) * 10.0;
        gVelocity[x].z = gVelocity[x].x;
        gVelocity[x].w = gVelocity[x].y;
    }

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(gWidth, gHeight, "CENG 469 THE3", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		exit(-1);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
    glewExperimental = GL_TRUE;
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

    glfwSetScrollCallback(window, scroll);
    glfwSetMouseButtonCallback(window,mouse);
	glfwSetKeyCallback(window, keyboard);
	glfwSetWindowSizeCallback(window, reshape);

	reshape(window, gWidth, gHeight); // need to call this once ourselves
	mainLoop(window); // this does not return unless the window is closed

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
