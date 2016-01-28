#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/glad.h>
#include <FTGL/ftgl.h>
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>

using namespace std;

struct VAO {
	GLuint VertexArrayID;
	GLuint VertexBuffer;
	GLuint ColorBuffer;
	GLuint TextureBuffer;
	GLuint TextureID;

	GLenum PrimitiveMode; // GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_LINE_STRIP_ADJACENCY, GL_LINES_ADJACENCY, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES, GL_TRIANGLE_STRIP_ADJACENCY and GL_TRIANGLES_ADJACENCY
	GLenum FillMode; // GL_FILL, GL_LINE
	int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID; // For use with normal shader
	GLuint TexMatrixID; // For use with texture shader
} Matrices;

struct FTGLFont {
	FTFont* font;
	GLuint fontMatrixID;
	GLuint fontColorID;
} GL3Font;

GLuint programID, fontProgramID, textureProgramID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	cout << "Compiling shader : " <<  vertex_file_path << endl;
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage( max(InfoLogLength, int(1)) );
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	cout << VertexShaderErrorMessage.data() << endl;

	// Compile Fragment Shader
	cout << "Compiling shader : " << fragment_file_path << endl;
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage( max(InfoLogLength, int(1)) );
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	cout << FragmentShaderErrorMessage.data() << endl;

	// Link the program
	cout << "Linking program" << endl;
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	cout << ProgramErrorMessage.data() << endl;

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
	cout << "Error: " << description << endl;
}

void quit(GLFWwindow *window)
{
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

glm::vec3 getRGBfromHue (int hue)
{
	float intp;
	float fracp = modff(hue/60.0, &intp);
	float x = 1.0 - abs((float)((int)intp%2)+fracp-1.0);

	if (hue < 60)
		return glm::vec3(1,x,0);
	else if (hue < 120)
		return glm::vec3(x,1,0);
	else if (hue < 180)
		return glm::vec3(0,1,x);
	else if (hue < 240)
		return glm::vec3(0,x,1);
	else if (hue < 300)
		return glm::vec3(x,0,1);
	else
		return glm::vec3(1,0,x);
}

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
						  0,                  // attribute 0. Vertices
						  3,                  // size (x,y,z)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
						  1,                  // attribute 1. Color
						  3,                  // size (r,g,b)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
	GLfloat* color_buffer_data = new GLfloat [3*numVertices];
	for (int i=0; i<numVertices; i++) {
		color_buffer_data [3*i] = red;
		color_buffer_data [3*i + 1] = green;
		color_buffer_data [3*i + 2] = blue;
	}

	return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

struct VAO* create3DTexturedObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* texture_buffer_data, GLuint textureID, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;
	vao->TextureID = textureID;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->TextureBuffer));  // VBO - textures

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
						  0,                  // attribute 0. Vertices
						  3,                  // size (x,y,z)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	glBindBuffer (GL_ARRAY_BUFFER, vao->TextureBuffer); // Bind the VBO textures
	glBufferData (GL_ARRAY_BUFFER, 2*numVertices*sizeof(GLfloat), texture_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
						  2,                  // attribute 2. Textures
						  2,                  // size (s,t)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	return vao;
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Enable Vertex Attribute 1 - Color
	glEnableVertexAttribArray(1);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

void draw3DTexturedObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Bind Textures using texture units
	glBindTexture(GL_TEXTURE_2D, vao->TextureID);

	// Enable Vertex Attribute 2 - Texture
	glEnableVertexAttribArray(2);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->TextureBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle

	// Unbind Textures to be safe
	glBindTexture(GL_TEXTURE_2D, 0);
}

/* Create an OpenGL Texture from an image */
GLuint createTexture (const char* filename)
{
	GLuint TextureID;
	// Generate Texture Buffer
	glGenTextures(1, &TextureID);
	// All upcoming GL_TEXTURE_2D operations now have effect on our texture buffer
	glBindTexture(GL_TEXTURE_2D, TextureID);
	// Set our texture parameters
	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Set texture filtering (interpolation)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Load image and create OpenGL texture
	int twidth, theight;
	unsigned char* image = SOIL_load_image(filename, &twidth, &theight, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, twidth, theight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D); // Generate MipMaps to use
	SOIL_free_image_data(image); // Free the data read from file after creating opengl texture
	glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess it up

	return TextureID;
}


/**************************
 * Customizable functions *
 **************************/
float formatAngle(float A)
{
    if(A<0.0f)
        return A+360.0f;
    if(A>=360.0f)
        return A-360.0f;
    return A;
}
float D2R(float A)
{
    return (A*M_PI)/180.0f;
}

double gravity=18,air_friction=1-0.000005;
double xmousePos1=0,xmousePos2=0,ymousePos1=0,ymousePos2=0;;
int in1=0;
float screen_shift=0,screen_shift_y=0;
float camera_zoom=1.05;
double angle_c=10,speed_of_canon_intial=0;
int a_pressed=0,w_pressed=0,s_pressed=0,d_pressed=0,c_pressed=0;
VAO *triangle, *circle1, *circle2, *half_circle, *rectangle, *bg_circle, *bg_ground, *bg_left, *bg_bottom, *speed_rect;
VAO *bg_speed;
double xmousePos=0,ymousePos=0,score=0;
float camera_rotation_angle = 90;
int left_button_Pressed=0,right_button_Pressed=0,canon_out=0;//canon_out=1 if it is out of barrel
double canon_x_position=0,canon_y_position=51,canon_start_time=0,canon_velocity=0,canon_theta=0,radius_of_canon=10;
double canon_x_initial_position=0,canon_y_initial_position=0,canon_x_velocity=0,canon_y_velocity=0;
int canon_x_direction=1;
float width=1350,height=720;
double coefficient_of_collision_with_walls=0.4,e=0.5;//e for collision
double friction=0.7;
double objects[100][17];
double fixe[10][4],no_of_fixed_objects=9;
VAO *fixed_object[10];
double coins[10][4],no_of_coins=4;
VAO *coins_objects[10];
int no_of_objects=3;//when ever u change this change the below line
VAO *objects_def[3];
double piggy_pos[3][3],no_of_piggy=3,radius_of_piggy=30,no_of_piggy_hit=0;
double r=1; //coefficient_of_collision
VAO *piggy_head,*piggy_eye,*piggy_ear,*piggy_big_nose,*piggy_small_nose,*piggy_big_eye,*cloud;
VAO *score_ver,*score_hor;
double a[10][7];
int no_of_collisions_allowed=60;
void intialize_a()
{
    a[0][0]=1;a[0][1]=1;a[0][2]=1;a[0][3]=1;a[0][4]=1;a[0][5]=1;a[0][6]=0;
    a[1][0]=0;a[1][1]=1;a[1][2]=1;a[1][3]=0;a[1][4]=0;a[1][5]=0;a[1][6]=0;
    a[2][0]=1;a[2][1]=1;a[2][2]=0;a[2][3]=1;a[2][4]=1;a[2][5]=0;a[2][6]=1;
    a[3][0]=1;a[3][1]=1;a[3][2]=1;a[3][3]=1;a[3][4]=0;a[3][5]=0;a[3][6]=1;
    a[4][0]=0;a[4][1]=1;a[4][2]=1;a[4][3]=0;a[4][4]=0;a[4][5]=1;a[4][6]=1;
    a[5][0]=1;a[5][1]=0;a[5][2]=1;a[5][3]=1;a[5][4]=0;a[5][5]=1;a[5][6]=1;
    a[6][0]=1;a[6][1]=0;a[6][2]=1;a[6][3]=1;a[6][4]=1;a[6][5]=1;a[6][6]=1;
    a[7][0]=1;a[7][1]=1;a[7][2]=1;a[7][3]=0;a[7][4]=0;a[7][5]=0;a[7][6]=0;
    a[8][0]=1;a[8][1]=1;a[8][2]=1;a[8][3]=1;a[8][4]=1;a[8][5]=1;a[8][6]=1;
    a[9][0]=1;a[9][1]=1;a[9][2]=1;a[9][3]=1;a[9][4]=0;a[9][5]=1;a[9][6]=1;
}
                    /*
                        0 x position
                        1 y position
                        2 x velocity
                        3 y velocity
                        4 circle/rectangle 0->circle
                        5 radius even for rectangle
                        6 length if 4==1
                        7 breadth if 4==1
                        8 start time
                        9 intial x pos
                        10 intial y pos
                        11 intial velocity
                        12 theta
                        13 in motion==1
                        14 x direction
                        15 immobile==0
                        16 no of hits if hits>4 disappear
                    */

VAO* createSector(float R,int parts,double clr[6][3])
{
  float diff=360.0f/parts;
  float A1=formatAngle(-diff/2);
  float A2=formatAngle(diff/2);
  GLfloat vertex_buffer_data[]={0.0f,0.0f,0.0f,R*cos(D2R(A1)),R*sin(D2R(A1)),0.0f,R*cos(D2R(A2)),R*sin(D2R(A2)),0.0f};
  GLfloat color_buffer_data[]={clr[0][0],clr[0][1],clr[0][2],clr[1][0],clr[1][1],clr[1][2],clr[2][0],clr[2][1],clr[2][2]};
  return create3DObject(GL_TRIANGLES,3,vertex_buffer_data,color_buffer_data,GL_FILL);
}
VAO* createtriangle()
{
  const GLfloat vertex_buffer_data [] = {
    0, 0,0, // vertex 0
    0,1,0, // vertex 1
    1,0,0, // vertex 2
  };
  const GLfloat color_buffer_data [] = {
    1,1,1, // color 0
    0,0,0, // color 1
    0,0,0, // color 2
  };
  return create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
}

VAO* createRectangle(double length, double breadth, double clr[6][3])
{
  // GL3 accepts only Triangles. Quads are not supported
  const GLfloat vertex_buffer_data [] = {
    0,0,0, // vertex 1
    length,0,0, // vertex 2
    length,breadth,0, // vertex 3

    0, 0,0, // vertex 3
    0, breadth,0, // vertex 4
    length,breadth,0  // vertex 1
  };

  const GLfloat color_buffer_data [] = {
    clr[0][0],clr[0][1],clr[0][2], // color 1
    clr[1][0],clr[1][1],clr[1][2], // color 2
    clr[2][0],clr[2][1],clr[2][2], // color 3

    clr[3][0],clr[3][1],clr[3][2], // color 3
    clr[4][0],clr[4][1],clr[4][2], // color 4
    clr[5][0],clr[5][1],clr[5][2]  // color 1
  };
  return create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void mousescroll(GLFWwindow* window, double xoffset, double yoffset)
{
    if (yoffset==-1)
         camera_zoom/=1.05;
    else if (yoffset==1)
        camera_zoom*=1.05;
    if (camera_zoom>=1.2)
        camera_zoom=1.2;
    if (camera_zoom<1)
        camera_zoom=1;
    float diff = width-width/camera_zoom;
    //cout << screen_shift << endl;
    Matrices.projection = glm::ortho((0.0f+diff+screen_shift)*1.0f, (width-diff+screen_shift)*1.0f, (0.0f+diff)*1.0f, (height-diff)*1.0f, 0.1f, 500.0f);
}
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_SPACE:
                a_pressed=0;
                //break;
            case GLFW_KEY_A:
                w_pressed=0;
                //break;
            case GLFW_KEY_B:
                s_pressed=0;
                //break;
            case GLFW_KEY_F:
                d_pressed=0;
                //break;
            case GLFW_KEY_S:
                c_pressed=0;
                break;
            default:
                break;
        }
    }
    else if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_SPACE:
                a_pressed=1;
                break;
            case GLFW_KEY_A:
                w_pressed=1;
                break;
            case GLFW_KEY_B:
                s_pressed=1;
                break;
            case GLFW_KEY_F:
                d_pressed=1;
                break;
            case GLFW_KEY_S:
                c_pressed=1;
                break;
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            default:
                break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_RELEASE)
            {
                left_button_Pressed = 0;
            }
            if(action==GLFW_PRESS)
            {
                left_button_Pressed=1;
            }
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_RELEASE)
            {
                right_button_Pressed=0;
            }
            if(action==GLFW_PRESS)
            {
                right_button_Pressed=1;
            }
            break;
        default:
            break;
    }
}

void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);
    Matrices.projection = glm::ortho((0.0f)*1.0f, width*1.0f, 0.0f, height*1.0f, 0.1f, 500.0f);
}
double distance(double x1,double y1, double x2, double y2)
{
    double var = (x2-x1)*(x2-x1)+(y2-y1)*(y2-y1);
    return sqrt(var);
}

void set_canon_position(double x,double y,double thetay,double thetax,int direction,double velocity,double u2x,double u2y)
{
    if (direction!=0)
        canon_x_direction=direction;
    canon_theta = atan2(thetay,thetax) ;
    //canon_out = 1;
    canon_start_time=glfwGetTime();
    canon_x_initial_position=x;
    canon_y_initial_position=y;
    canon_x_position=x;
    canon_y_position=y;
    canon_velocity=sqrt(u2x*u2x+u2y*u2y);
    //if (canon_velocity<20)
    //    canon_velocity=20;
    canon_x_velocity=u2x;
    canon_y_velocity=u2y;
}

void set_object_position(double x,double y,double thetay,double thetax,int direction,double velocity,double u2x,double u2y,int i)
{
    if (direction!=0)
        objects[i][14]=direction;
    objects[i][12]= atan2(thetay,thetax);
    objects[i][8]=glfwGetTime();
    objects[i][9]=x;
    objects[i][10]=y;
    objects[i][11]=sqrt(u2x*u2x+u2y*u2y);
    objects[i][2]=u2x;//*direction;
    objects[i][3]=u2y;
    if (objects[i][15]==1)
        objects[i][13]=1;
    objects[i][16]+=1;
}

void checkcollision()
{
    double velocity=sqrt(canon_x_velocity*canon_x_velocity+canon_y_velocity*canon_y_velocity);
    if (canon_x_position>=1350-15)
        set_canon_position(1350-15,canon_y_position,canon_y_velocity,-1*canon_x_velocity,-1,velocity,-1*canon_x_velocity*coefficient_of_collision_with_walls,canon_y_velocity*friction);
    if (canon_y_position+radius_of_canon>=650)
        set_canon_position(canon_x_position,650-radius_of_canon,-1*canon_y_velocity,canon_x_velocity,0,velocity,canon_x_velocity*friction,-1*canon_y_velocity*coefficient_of_collision_with_walls);
    if (canon_y_position<=50)
        set_canon_position(canon_x_position,50,-1*canon_y_velocity,canon_x_velocity,0,velocity,canon_x_velocity*friction,-1*canon_y_velocity*coefficient_of_collision_with_walls);
    if (canon_x_position<=11+15 && canon_out==1)
        set_canon_position(26,canon_y_position,canon_y_velocity,-1*canon_x_velocity,1,velocity,-1*canon_x_velocity*coefficient_of_collision_with_walls,canon_y_velocity*friction);
    double dist=0;
    for (int i = 0; i < no_of_objects; i++)
    {
        dist = distance(canon_x_position,canon_y_position,objects[i][0],objects[i][1]);
        if (dist<=10+objects[i][5]&&objects[i][16]<=no_of_collisions_allowed)
        {
        	score+=10;
            double m=objects[i][5]/(2*radius_of_canon);
            double u1x,u1y,u2x,u2y,v1x,v1y,v2x,v2y;
            u1x = canon_x_velocity;
            u1y = canon_y_velocity;
            v1x = objects[i][2];
            v1y = objects[i][3];
            v2x = (e*(u1x-v1x)+u1x+v1x)/(1+m);
            u2x = u1x+m*v1x-m*v2x;
            v2y = (e*(u1y-v1y)+u1y+v1y)/(1+m);
            u2x = u1y+m*v1y-m*v2y;
            int dir=1,valx,valy;
            if(u2x<0)
                dir=-1;
            double x=canon_x_position,y=canon_y_position;
            if (x>objects[i][0] && y>objects[i][1])
            {
                valx=4;
                valy=4;
            }
            else if (x>objects[i][0] && y<objects[i][1])
            {
                valx=4;
                valy=-4;
            }
            else if (x<objects[i][0] && y>objects[i][1])
            {
                valx=-4;
                valy=4;
            }
            else if (x<objects[i][0] && y<objects[i][1])
            {
                valx=-4;
                valy=-4;
            }
            set_canon_position(x+valx,y+valy,u2y,u2x,dir,sqrt(u2x*u2x+u2y*u2y),u2x,u2y);
            objects[i][9]=objects[i][0];
            objects[i][10]=objects[i][1];
            if (objects[i][15]==1)
            {
               /* objects[i][2] = v2x;
                objects[i][3] = v2y;
                if (v2x>0)
                    objects[i][14]=1;
                else
                    objects[i][14]=-1;
                objects[i][11] = sqrt(v2x*v2x+v2y*v2y);
                objects[i][12] = atan2(v2y,v2x);
                objects[i][8] = glfwGetTime();
                */objects[i][13]=1;
                int dir=1;
                if (v2x<0)
                    dir=-1;
                set_object_position(objects[i][0],objects[i][1],v2y,v2x,dir,0,v2x,v2y,i);
            }
        }
        double velocity1=sqrt(objects[i][2]*objects[i][2]+objects[i][3]*objects[i][3]);
        if (objects[i][0]>=1350-15)
            set_object_position(1350-15,objects[i][1],objects[i][3],-1*objects[i][2],-1,velocity1*coefficient_of_collision_with_walls,-1*objects[i][2]*coefficient_of_collision_with_walls,objects[i][3]*friction,i);
        if (objects[i][1]>=650-15)
            set_object_position(objects[i][0],650-20,-1*objects[i][3],objects[i][2],0,velocity1*coefficient_of_collision_with_walls,objects[i][2]*friction,objects[i][3]*-1*coefficient_of_collision_with_walls,i);
        if (objects[i][1]<50)
            set_object_position(objects[i][0],50,-1*objects[i][3],objects[i][2],0,velocity1*coefficient_of_collision_with_walls,objects[i][2]*friction,-1*objects[i][3]*coefficient_of_collision_with_walls,i);
        if (objects[i][0]<=11+15)
            set_object_position(26,objects[i][1],objects[i][3],-1*objects[i][2],1,velocity1*coefficient_of_collision_with_walls,-1*objects[i][2]*coefficient_of_collision_with_walls,objects[i][3]*friction,i);
        for (int i1 = 0; i1 < no_of_objects;i1++)
        {
            if (i!=i1)
            {
                dist=distance(objects[i][0],objects[i][1],objects[i1][0],objects[i1][1]);
                if (dist<=objects[i][5]+objects[i1][5])
                {
                    double m=objects[i1][5]/objects[i][5];
                    double u1x,u1y,u2x,u2y,v1x,v1y,v2x,v2y;
                    u1x = objects[i][2];
                    u1y = objects[i][3];
                    v1x = objects[i1][2];
                    v1y = objects[i1][3];
                    v2x = (e*(u1x-v1x)+u1x+v1x)/(1+m);
                    u2x = u1x+m*v1x-m*v2x;
                    v2y = (e*(u1y-v1y)+u1y+v1y)/(1+m);
                    u2x = u1y+m*v1y-m*v2y;
                    int dir=1,valx,valy;
                    if(u2x<0)
                        dir=-1;
                    double x=objects[i][0],y=objects[i][1];
                    if (x>objects[i1][0] && y>objects[i1][1])
                    {
                        valx=4;
                        valy=4;
                    }
                    else if (x>objects[i1][0] && y<objects[i1][1])
                    {
                        valx=4;
                        valy=-4;
                    }
                    else if (x<objects[i1][0] && y>objects[i1][1])
                    {
                        valx=-4;
                        valy=4;
                    }
                    else if (x<objects[i1][0] && y<objects[i1][1])
                    {
                        valx=-4;
                        valy=-4;
                    }
                    set_object_position(objects[i][0]+valx,objects[i][1]+valy,u2y,u2x,dir,sqrt(u2x*u2x+u2y*u2y),u2x,u2y,i);
                    dir=1;
                    if(v2x<0)
                        dir=-1;
                    set_object_position(objects[i1][0]-valx,objects[i1][1]-valy,v2y,v2x,dir,sqrt(v2x*v2x+v2y*v2y),v2x,v2y,i1);
                    /*
                    objects[i][9]=objects[i][0];
                    objects[i][10]=objects[i][1];
                    if (objects[i][15]==1)
                    {
                        objects[i][2] = u2x;
                        objects[i][3] = u2y;
                        if (v2x>0)
                            objects[i][14]=1;
                        else
                            objects[i][14]=-1;
                        objects[i][11] = sqrt(u2x*u2x+u2y*u2y);
                        objects[i][12] = atan2(u2y,u2x);
                        objects[i][8] = glfwGetTime();
                        objects[i][13]=1;
                    }
                    if (objects[i1][15]==1)
                    {
                        objects[i1][2] = v2x;
                        objects[i1][3] = v2y;
                        if (v2x>0)
                            objects[i1][14]=1;
                        else
                            objects[i1][14]=-1;
                        objects[i1][11] = sqrt(v2x*v2x+v2y*v2y);
                        objects[i1][12] = atan2(v2y,v2x);
                        objects[i1][8] = glfwGetTime();
                        objects[i1][13]=1;
                        
                    }*/
                }
            }
        }
        for (int i1 = 6; i1 < 9;i1++)
        {
        	double y=objects[i][1]-objects[i][5]-(fixe[i1][1]+fixe[i1][3]);
        	double x=objects[i][0]-fixe[i1][0];
            if (x<=fixe[i1][2] &&x>=0)
        	{
            	if(y<=5 && y>=0)
            		set_object_position(objects[i][0],objects[i][1],-1*objects[i][3],objects[i][2],0,0,objects[i][2]*friction,objects[i][3]*-1*coefficient_of_collision_with_walls,i);
            	y=objects[i][0]-(fixe[i1][1]);
        	    if (y>=0&&y<=10)
            		set_object_position(objects[i][0],objects[i][1]-5,-1*objects[i][3],objects[i][2],0,0,objects[i][2]*friction,objects[i][3]*-1*coefficient_of_collision_with_walls,i);
        	}
            double y1=objects[i][1]-objects[i][5]-(fixe[i1][1]);
        	double x1=objects[i][0]-objects[i][5]-fixe[i1][0];
        	y=objects[i][1]+objects[i][5]-fixe[i1][1];
        	x=objects[i][0]+objects[i][5]-fixe[i1][0];
        	if (((y<=fixe[i1][3]&&y>=0)||(y1<=fixe[i1][3]&&y1>=0)))
        	{
            	if (x>=0&&x<=10)
            		set_object_position(objects[i][0]-3,objects[i][1],objects[i][3],-1*objects[i][2],0,0,objects[i][2]*coefficient_of_collision_with_walls*-1,objects[i][3]*friction,i);
            	if (x1>=0&&x1<=5)
            		set_object_position(objects[i][0]+3,objects[i][1],objects[i][3],-1*objects[i][2],0,0,objects[i][2]*coefficient_of_collision_with_walls*-1,objects[i][3]*friction,i);
        	}
        }        
    }
    for (int i = 0; i < no_of_fixed_objects;i++)
    {
        int in=0;
        double y=canon_y_position-radius_of_canon-(fixe[i][1]+fixe[i][3]);
        double x=canon_x_position-radius_of_canon-fixe[i][0];
        if (x<=fixe[i][2] &&x>=0)
        {
            if(y<=5 && y>=0)
                set_canon_position(canon_x_position,canon_y_position,-1*canon_y_velocity,canon_x_velocity,0,0,canon_x_velocity*friction,canon_y_velocity*-1*coefficient_of_collision_with_walls);          
            y=canon_y_position+radius_of_canon-(fixe[i][1]);
            if (y>=0&&y<=10)
                set_canon_position(canon_x_position,canon_y_position-10,-1*canon_y_velocity,canon_x_velocity,0,0,canon_x_velocity*friction,canon_y_velocity*-1*coefficient_of_collision_with_walls);                          
            in=0;
        }
        double y1=canon_y_position-radius_of_canon-fixe[i][1];
        double x1=canon_x_position-radius_of_canon-fixe[i][0]-fixe[i][2];
        y=canon_y_position+radius_of_canon-fixe[i][1];
        x=canon_x_position+radius_of_canon-fixe[i][0];
        if (((y<=fixe[i][3]&&y>=0)||(y1<=fixe[i][3]&&y1>=0)))
        {
            if (x>=0&&x<=10)
                set_canon_position(canon_x_position-6,canon_y_position,canon_y_velocity,-1*canon_x_velocity,1,-1,-1*canon_x_velocity*coefficient_of_collision_with_walls,canon_y_velocity*friction);          
            if (x1>=0&&x1<=5)
                set_canon_position(canon_x_position+6,canon_y_position,canon_y_velocity,-1*canon_x_velocity,-1,1,-1*canon_x_velocity*coefficient_of_collision_with_walls,canon_y_velocity*friction);          
        }
    }
    for (int i = 0; i < no_of_coins; ++i)
    {
        double dist=distance(canon_x_position,canon_y_position,coins[i][0],coins[i][1]);
        if (dist<=radius_of_canon+coins[i][2] && coins[i][3]==1)
        {
        	set_canon_position(0,0,0,1,1,0,0,0);
            canon_x_position=0;
            canon_y_position=0;
            canon_out=0;
            coins[i][3]=0;
            score+=10;
        }
    }
    for (int i = 0; i < no_of_piggy;i++)
    {
        double dist=distance(canon_x_position,canon_y_position,piggy_pos[i][0],piggy_pos[i][1]);
        if (dist<=radius_of_canon+radius_of_piggy && piggy_pos[i][2]!=3)
        {
        	set_canon_position(0,0,0,1,1,0,0,0);
            canon_x_position=0;
            canon_y_position=0;
            canon_out=0;
            piggy_pos[i][2]+=1;
            score=score+piggy_pos[i][2]*10;
        }    
    }
}
void drawobject(VAO* obj,glm::vec3 trans,float angle,glm::vec3 rotat)
{
    Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0));
    glm::mat4 VP = Matrices.projection * Matrices.view;
    glm::mat4 MVP;  // MVP = Projection * View * Model
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translatemat = glm::translate(trans);
    glm::mat4 rotatemat = glm::rotate(D2R(formatAngle(angle)), rotat);
    Matrices.model *= (translatemat * rotatemat);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(obj);
}

void intialize_objects()
{
    for (int i = 0; i < no_of_objects;i++)
    {
        objects[i][0]=300+i*50;
        objects[i][1]=500;
        objects[i][2]=0;
        objects[i][3]=0;
        objects[i][4]=0;
        objects[i][6]=30;
        objects[i][7]=30;
        objects[i][5]=objects[i][6]/2;
        objects[i][8]=0;
        objects[i][9]=objects[i][0];
        objects[i][10]=objects[i][1];
        objects[i][11]=0;
        objects[i][12]=0;
        objects[i][13]=0;
        objects[i][14]=0;
        objects[i][15]=1;
        objects[i][16]=0;
    }/*
    objects[0][0]=800;
    objects[0][1]=300;
    objects[1][0]=900;
    objects[1][1]=300;
    objects[2][0]=850;
    objects[2][1]=300;
    */
    objects[0][0]=400;
    objects[0][1]=100;
    objects[1][0]=1200;
    objects[1][1]=150;
    objects[2][0]=1000;
    objects[2][1]=200;
    
    fixe[0][0]=300;
    fixe[0][1]=400;
    fixe[0][2]=100;
    fixe[0][3]=30;
    fixe[1][0]=400;
    fixe[1][1]=370;
    fixe[1][2]=100;
    fixe[1][3]=30;
    fixe[2][0]=500;
    fixe[2][1]=400;
    fixe[2][2]=100;
    fixe[2][3]=30;
    fixe[3][0]=1265;
    fixe[3][1]=500;
    fixe[3][2]=70;
    fixe[3][3]=30;
    fixe[4][0]=1235;
    fixe[4][1]=500;
    fixe[4][2]=30;
    fixe[4][3]=100;
    fixe[5][0]=150;
    fixe[5][1]=500;
    fixe[5][2]=100;
    fixe[5][3]=30;
    fixe[6][0]=850;
    fixe[6][1]=145;
    fixe[6][2]=100;
    fixe[6][3]=30;
    fixe[7][0]=1050;
    fixe[7][1]=45;
    fixe[7][2]=30;
    fixe[7][3]=100;
    fixe[8][0]=850;
    fixe[8][1]=45;
    fixe[8][2]=30;
    fixe[8][3]=100;
    
    coins[0][0]=350;
    coins[0][1]=445;
    coins[0][2]=15;
    coins[0][3]=1;
    coins[1][0]=550;
    coins[1][1]=445;
    coins[1][2]=15;
    coins[1][3]=1;
    coins[2][0]=990;
    coins[2][1]=55;
    coins[2][2]=15;
    coins[2][3]=1;
    coins[3][0]=900;
    coins[3][1]=190;
    coins[3][2]=15;
    coins[3][3]=1;

    piggy_pos[0][0]=450;
    piggy_pos[0][1]=430;
    piggy_pos[0][2]=0;
    piggy_pos[1][0]=1300;
    piggy_pos[1][1]=560;
    piggy_pos[1][2]=0;
    piggy_pos[2][0]=200;
    piggy_pos[2][1]=560;
    piggy_pos[2][2]=0;
}

void background()
{
    double clr[6][3];
    for (int i = 0; i < 6;i++)
    {
        clr[i][0]=0;
        clr[i][1]=0;
        clr[i][2]=0;
    }
    bg_circle=createSector(40,360,clr);
    for (int i = 0; i < 6;i++)
    {
        clr[i][0]=0;
        clr[i][1]=0.3;
        clr[i][2]=0;
    }
    bg_ground=createRectangle(1500,200,clr);
    for (int i = 0; i < 6;i++)
    {
        clr[i][0]=1;
        clr[i][1]=0.764;
        clr[i][2]=0.301;
    }
    bg_left=createRectangle(15,720,clr);
    bg_bottom=createRectangle(1360,15,clr);
    for (int i = 0; i < 6;i++)
    {
        clr[i][0]=0;
        clr[i][1]=0;
        clr[i][2]=0;
    }
    bg_speed=createRectangle(width/3,23,clr);
}
/*
void draw ()
{
	// clear the color and depth in the frame buffer
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram (programID);

	// Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
	// Target - Where is the camera looking at.  Don't change unless you are sure!!
	glm::vec3 target (0, 0, 0);
	// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
	glm::vec3 up (0, 1, 0);

	// Compute Camera matrix (view)
	// Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
	//  Don't change unless you are sure!!
	static float c = 0;
	c++;
	Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(sinf(c*M_PI/180.0),3*cosf(c*M_PI/180.0),0)); // Fixed camera for 2D (ortho) in XY plane

	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
	//  Don't change unless you are sure!!
	glm::mat4 VP = Matrices.projection * Matrices.view;

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	//  Don't change unless you are sure!!
	glm::mat4 MVP;	// MVP = Projection * View * Model

	// Load identity to model matrix
	Matrices.model = glm::mat4(1.0f);

	/* Render your scene */
	/*
	glm::mat4 translateTriangle = glm::translate (glm::vec3(-2.0f, 0.0f, 0.0f)); // glTranslatef
	glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
	glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
	Matrices.model *= triangleTransform;
	MVP = VP * Matrices.model; // MVP = p * V * M

	//  Don't change unless you are sure!!
	// Copy MVP to normal shaders
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(triangle);



	// Render with texture shaders now
	glUseProgram(textureProgramID);

	// Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
	// glPopMatrix ();
	Matrices.model = glm::mat4(1.0f);

	glm::mat4 translateRectangle = glm::translate (glm::vec3(2, 0, 0));        // glTranslatef
	glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= (translateRectangle * rotateRectangle);
	MVP = VP * Matrices.model;

	// Copy MVP to texture shaders
	glUniformMatrix4fv(Matrices.TexMatrixID, 1, GL_FALSE, &MVP[0][0]);

	// Set the texture sampler to access Texture0 memory
	glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DTexturedObject(rectangle);

	// Increment angles
	float increments = 1;

	// Render font on screen
	static int fontScale = 0;
	float fontScaleValue = 0.75 + 0.25*sinf(fontScale*M_PI/180.0f);
	glm::vec3 fontColor = getRGBfromHue (fontScale);



	// Use font Shaders for next part of code
	glUseProgram(fontProgramID);
	Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

	// Transform the text
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translateText = glm::translate(glm::vec3(-3,2,0));
	glm::mat4 scaleText = glm::scale(glm::vec3(,,));
	Matrices.model *= (translateText * scaleText);
	MVP = Matrices.projection * Matrices.view * Matrices.model;
	// send font's MVP and font color to fond shaders
	glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

	// Render font
	GL3Font.font->Render("Round n Round we go !!");


	//camera_rotation_angle++; // Simulating camera rotation
	triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
	rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;

	// font size and color changes
	fontScale = (fontScale + 1) % 360;
}*/

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
	GLFWwindow* window; // window desciptor/handle

	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	glfwSwapInterval( 1 );

	/* --- register callbacks with GLFW --- */

	/* Register function to handle window resizes */
	/* With Retina display on Mac OS X GLFW's FramebufferSize
	 is different from WindowSize */
	glfwSetFramebufferSizeCallback(window, reshapeWindow);
	glfwSetWindowSizeCallback(window, reshapeWindow);

	/* Register function to handle window close */
	glfwSetWindowCloseCallback(window, quit);

	/* Register function to handle keyboard input */
	glfwSetKeyCallback(window, keyboard);      // general keyboard input
	glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

	/* Register function to handle mouse click */
	glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

	return window;
}
void draw()
{
    set_canon_position(canon_x_position,canon_y_position,canon_y_velocity*air_friction,canon_x_velocity*air_friction,0,0,canon_x_velocity*air_friction,canon_y_velocity*air_friction);
    double clr[6][3];
    for (int i = 0; i < 6;i++)
    {
        clr[i][0]=1;
        clr[i][1]=0;
        clr[i][2]=0;
    }
    if (w_pressed==1)
    {
        angle_c+=5;
        if (angle_c>=90)
            angle_c=90;
    }
    if (s_pressed==1)
    {
        angle_c-=5;
        if (angle_c<10)
            angle_c=10;
    }
    if (d_pressed==1)
    {
        speed_of_canon_intial+=5;
        if (speed_of_canon_intial>width)
            speed_of_canon_intial=width;
    }
    else if (c_pressed==1)
    {
        speed_of_canon_intial-=5;
        if (speed_of_canon_intial<=0)
            speed_of_canon_intial=0;
    }
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram (programID);
    drawobject(bg_ground,glm::vec3(0,0,0),0,glm::vec3(0,0,1));
    drawobject(bg_left,glm::vec3(0,0,0),0,glm::vec3(0,0,1));
    drawobject(bg_left,glm::vec3(width-15,0,0),0,glm::vec3(0,0,1));
    drawobject(bg_bottom,glm::vec3(0,0,0),0,glm::vec3(0,0,1));
    drawobject(bg_bottom,glm::vec3(0,height-18,0),0,glm::vec3(0,0,1));
    drawobject(bg_bottom,glm::vec3(0,height-60,0),0,glm::vec3(0,0,1));
    for (int i = 0; i <=180;i+=6)
        drawobject(cloud,glm::vec3(800,550,0),i,glm::vec3(0,0,1));    
    for (int i = 0; i <=180;i+=6)
        drawobject(cloud,glm::vec3(860,550,0),i,glm::vec3(0,0,1));    
    for (int i = 0; i <=180;i+=6)
        drawobject(cloud,glm::vec3(920,550,0),i,glm::vec3(0,0,1));    
    for (int i = 0; i <=180;i+=6)
        drawobject(cloud,glm::vec3(830,555,0),i,glm::vec3(0,0,1));    
    for (int i = 0; i <=180;i+=6)
        drawobject(cloud,glm::vec3(880,555,0),i,glm::vec3(0,0,1));    
    for (int i = 0; i <=180;i+=6)
        drawobject(cloud,glm::vec3(860,570,0),i,glm::vec3(0,0,1));     
    if (right_button_Pressed==1)
        drawobject(rectangle,glm::vec3(55,50,0),atan((720-ymousePos)/xmousePos) * 180/M_PI,glm::vec3(0,0,1));
    else
        drawobject(rectangle,glm::vec3(55,50,0),angle_c,glm::vec3(0,0,1));
    drawobject(bg_speed,glm::vec3(18,height-44,0),0,glm::vec3(0,0,1));
    if(left_button_Pressed==0&&right_button_Pressed==1)
    {
        speed_of_canon_intial=sqrt((xmousePos-55)*(xmousePos-55)+(720-ymousePos)*(720-ymousePos));
        if (speed_of_canon_intial>=width)
            speed_of_canon_intial=width;
    }
    if (left_button_Pressed==1&&right_button_Pressed==0)
    {
        if(in1==0)
        {
            xmousePos1=xmousePos;
            ymousePos1=ymousePos;
            in1=1;
        }
    }
    if (left_button_Pressed==0&&in1==1)
    {
        xmousePos2=xmousePos;
        ymousePos2=ymousePos;
        in1=0;
        screen_shift+=xmousePos1-xmousePos2;
        screen_shift_y=ymousePos1-ymousePos2;
        if (xmousePos2==xmousePos1&&ymousePos2==ymousePos1)
        {
            screen_shift=0;
            screen_shift_y==0;
            camera_zoom=1;
        }
        cout<<screen_shift<<"   "<<screen_shift_y<<endl;
        float diff = width-width/camera_zoom;
        Matrices.projection = glm::ortho((0.0f+diff+screen_shift)*1.0f, (width-diff+screen_shift)*1.0f, (0.0f+diff-screen_shift_y)*1.0f, (height-diff-screen_shift_y)*1.0f, 0.1f, 500.0f);
    }
    speed_rect = createRectangle(speed_of_canon_intial/3,15,clr);
    drawobject(speed_rect,glm::vec3(18,height-40,0),0,glm::vec3(0,0,1));
    for (int i = 0; i < 360; ++i)
      drawobject(circle1,glm::vec3(30,40,0),i,glm::vec3(0,0,1));
    for (int i = 0; i < 360; ++i)
      drawobject(circle1,glm::vec3(80,40,0),i,glm::vec3(0,0,1));
    for (int i = 0; i <=180; ++i)
        drawobject(half_circle,glm::vec3(55,50,0),i,glm::vec3(0,0,1));
    for (int i = 0; i < no_of_piggy;i++)
    {
        if(piggy_pos[i][2]<=2)
        {
            for (int i1 = 0; i1 < 360;i1+=60)
                drawobject(piggy_ear,glm::vec3(piggy_pos[i][0]-24,piggy_pos[i][1]+15,0),i1,glm::vec3(0,0,1));            
            for (int i1 = 0; i1 < 360;i1+=60)
                drawobject(piggy_ear,glm::vec3(piggy_pos[i][0]+24,piggy_pos[i][1]+15,0),i1,glm::vec3(0,0,1));            
            for (int i1 = 0; i1 < 360;i1+=60)
                drawobject(piggy_head,glm::vec3(piggy_pos[i][0],piggy_pos[i][1],0),i1,glm::vec3(0,0,1));
            if (piggy_pos[i][2]>=1)
                for (int i1 = 0; i1 < 360;i1+=60)
                    drawobject(piggy_big_eye,glm::vec3(piggy_pos[i][0]-12,piggy_pos[i][1]+12,0),i1,glm::vec3(0,0,1));            
            if (piggy_pos[i][2]>1)
                for (int i1 = 0; i1 < 360;i1+=60)
                    drawobject(piggy_big_eye,glm::vec3(piggy_pos[i][0]+12,piggy_pos[i][1]+12,0),i1,glm::vec3(0,0,1));            
            for (int i1 = 0; i1 < 360;i1+=60)
                drawobject(piggy_eye,glm::vec3(piggy_pos[i][0]+12,piggy_pos[i][1]+12,0),i1,glm::vec3(0,0,1));
            for (int i1 = 0; i1 < 360;i1+=60)
                drawobject(piggy_eye,glm::vec3(piggy_pos[i][0]-12,piggy_pos[i][1]+12,0),i1,glm::vec3(0,0,1));
            for (int i1 = 0; i1 < 360;i1+=60)
                drawobject(piggy_big_nose,glm::vec3(piggy_pos[i][0],piggy_pos[i][1]-8,0),i1,glm::vec3(0,0,1));
            for (int i1 = 0; i1 < 360;i1+=60)
                drawobject(piggy_small_nose,glm::vec3(piggy_pos[i][0]-4,piggy_pos[i][1]-8,0),i1,glm::vec3(0,0,1));
            for (int i1 = 0; i1 < 360;i1+=60)
                drawobject(piggy_small_nose,glm::vec3(piggy_pos[i][0]+4,piggy_pos[i][1]-8,0),i1,glm::vec3(0,0,1));            
        }
    }
    for (int i = 0; i < no_of_coins;i++)
        if (coins[i][3]==1)
            for (int i1 = 0; i1 < 360; ++i1)
                drawobject(coins_objects[i],glm::vec3(coins[i][0],coins[i][1],0),i1,glm::vec3(0,0,1));
    for (int i = 0; i < no_of_fixed_objects; ++i)
        drawobject(fixed_object[i],glm::vec3(fixe[i][0],fixe[i][1],0),0,glm::vec3(0,0,1));
    for (int i = 0; i < no_of_objects;i++)
    {
        if (objects[i][13]==1)
        {
            double tim=glfwGetTime()-objects[i][8];
            objects[i][0]=objects[i][9]+(objects[i][11]*cos(objects[i][12])*tim*objects[i][14])*10;
            objects[i][1]=objects[i][10]+(objects[i][11]*sin(objects[i][12])*tim-(9.8*tim*tim)/2)*10;
            //objects[i][2]=objects[i][11]*cos(objects[i][12])*objects[i][14];
            objects[i][3]=objects[i][11]*sin(objects[i][12]) - 9.8*tim;
            //cout<<i<<"  "<<objects[i][2]<<endl;
            //cout<<objects[i][1]<<"  "<<objects[i][2]<<endl;
            if (objects[i][1]<51&&objects[i][2]==0)
            {
                objects[i][13]=0;
            }
        }
        if (objects[i][16]<=no_of_collisions_allowed)
        {
            if (objects[i][4]==0)
                for (int j = 0; j < 360; ++j)
                    drawobject(objects_def[i],glm::vec3(int(objects[i][0]),int(objects[i][1]),0),j,glm::vec3(0,0,1));
            else
                drawobject(objects_def[i],glm::vec3(objects[i][0],objects[i][1],0),0,glm::vec3(0,0,1));
        }
    }
    if (left_button_Pressed==1 && right_button_Pressed==1 && canon_out==0)
    {
    	canon_out=1;
        double theta = atan((720-ymousePos)/xmousePos);
        double v=sqrt((xmousePos-55)*(xmousePos-55)+(720-ymousePos)*(720-ymousePos));
        set_canon_position(55+100*cos(theta),60+100*sin(theta),(720-ymousePos),xmousePos,1,v/10,(v/10)*cos(theta),(v/10)*sin(theta));
    }
    else if (a_pressed==1 && canon_out==0)
    {
    	canon_out=1;
        double s=angle_c*M_PI/180;
        //cout<<angle_c<<"    "<<cos(angle_c)<<"    "<<sin(angle_c)<<endl;
        set_canon_position(55+100*cos(s),60+100*sin(s),tan(s),1,1,speed_of_canon_intial/10,(speed_of_canon_intial/10)*cos(s),(speed_of_canon_intial/10)*sin(s));        
    }
    if(canon_out==1)
    {
        double tim = glfwGetTime() - canon_start_time;
        canon_y_velocity=canon_velocity*sin(canon_theta)-gravity*tim;
        if (canon_x_velocity<0)
            canon_x_direction=-1;
        else
            canon_x_direction=1;
        for (int i = 0; i < 360; ++i)
            drawobject(circle1,glm::vec3(canon_x_position,canon_y_position,0),i,glm::vec3(0,0,1));
        canon_y_position=canon_y_initial_position+((canon_velocity*sin(canon_theta))*tim - (gravity*tim*tim)/2)*10;
        canon_x_position=canon_x_initial_position+((canon_velocity*cos(canon_theta))*tim)*10;
        if (canon_x_velocity<=1 && canon_x_velocity>=-1 && canon_y_velocity<=1 && canon_y_velocity>=-1)
        {
        	//cout<<"hello"<<"		"<<canon_x_velocity<<" 			  "<<canon_y_velocity<<endl;//"  "<<canon_x_position<<" "<<canon_y_position<<endl;
            canon_out=0;
        }
    }
    if (canon_x_velocity>70)
    {
        canon_x_velocity=70;
        //set_canon_position(canon_x_position,canon_y_position,canon_y_velocity,canon_x_velocity,0,0,canon_x_velocity,canon_y_velocity);
    }
    int score1=score,var_s;
    double x_cor=width-width/10,y_cor=height-height/40;
    while(score1!=0)
    {
        var_s=score1%10;
        if (a[var_s][0]==1)
            drawobject(score_hor,glm::vec3(x_cor,y_cor,0),0,glm::vec3(0,0,1));
        if (a[var_s][1]==1)
            drawobject(score_ver,glm::vec3(x_cor+15,y_cor-15,0),0,glm::vec3(0,0,1));
        if (a[var_s][2]==1)
            drawobject(score_ver,glm::vec3(x_cor+15,y_cor-30,0),0,glm::vec3(0,0,1));
        if (a[var_s][3]==1)
            drawobject(score_hor,glm::vec3(x_cor,y_cor-30,0),0,glm::vec3(0,0,1));
        if (a[var_s][4]==1)
            drawobject(score_ver,glm::vec3(x_cor,y_cor-30,0),0,glm::vec3(0,0,1));
        if (a[var_s][5]==1)
            drawobject(score_ver,glm::vec3(x_cor,y_cor-15,0),0,glm::vec3(0,0,1));
        if (a[var_s][6]==1)
            drawobject(score_hor,glm::vec3(x_cor,y_cor-15,0),0,glm::vec3(0,0,1));
        score1/=10;
        x_cor-=25;
    }
    glm::vec3 fontColor = glm::vec3(0,0,0);
	glUseProgram(fontProgramID);
	Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
	glm::mat4 MVP;
	// Transform the text
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translateText = glm::translate(glm::vec3(width*8/11,height*16/17,0));
	glm::mat4 scaleText = glm::scale(glm::vec3(50,50,50));
	Matrices.model *= (translateText * scaleText);
	MVP = Matrices.projection * Matrices.view * Matrices.model;
	// send font's MVP and font color to fond shaders
	glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

	// Render font
	GL3Font.font->Render("SCORE:");
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
	// Load Textures
	// Enable Texture0 as current texture memory
	glActiveTexture(GL_TEXTURE0);
	// load an image file directly as a new OpenGL texture
	// GLuint texID = SOIL_load_OGL_texture ("beach.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS); // Buggy for OpenGL3
	GLuint textureID = createTexture("beach2.png");
	// check for an error during the load process
	if(textureID == 0 )
		cout << "SOIL loading error: '" << SOIL_last_result() << "'" << endl;

	// Create and compile our GLSL program from the texture shaders
	textureProgramID = LoadShaders( "TextureRender.vert", "TextureRender.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.TexMatrixID = glGetUniformLocation(textureProgramID, "MVP");


	/* Objects should be created before any other gl function and shaders */
	// Create the models


	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL3.vert", "Sample_GL3.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


	reshapeWindow (window, width, height);

	// Background color of the scene
	glClearColor (0.701,1,0.898, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Initialise FTGL stuff
	const char* fontfile = "arial.ttf";
	GL3Font.font = new FTExtrudeFont(fontfile); // 3D extrude style rendering

	if(GL3Font.font->Error())
	{
		cout << "Error: Could not load font `" << fontfile << "'" << endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Create and compile our GLSL program from the font shaders
	fontProgramID = LoadShaders( "fontrender.vert", "fontrender.frag" );
	GLint fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform;
	fontVertexCoordAttrib = glGetAttribLocation(fontProgramID, "vertexPosition");
	fontVertexNormalAttrib = glGetAttribLocation(fontProgramID, "vertexNormal");
	fontVertexOffsetUniform = glGetUniformLocation(fontProgramID, "pen");
	GL3Font.fontMatrixID = glGetUniformLocation(fontProgramID, "MVP");
	GL3Font.fontColorID = glGetUniformLocation(fontProgramID, "fontColor");

	GL3Font.font->ShaderLocations(fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform);
	GL3Font.font->FaceSize(1);
	GL3Font.font->Depth(0);
	GL3Font.font->Outset(0, 0);
	GL3Font.font->CharMap(ft_encoding_unicode);

	intialize_a();
    background();
    double clr[6][3];
    for (int i = 0; i < 6; ++i)
    {
        for (int i1 = 0; i1 < 3; ++i1)
        {
            clr[i][i1]=1;
        }
        clr[i][0]=1;
    }
    for (int i = 0; i < no_of_objects; i++)
    {
        if (objects[i][4]==0)
            objects_def[i]=createSector(objects[i][5],360,clr);
        else if (objects[i][4]==1)
            objects_def[i]=createRectangle(objects[i][6],objects[i][7],clr);
    }
    for (int i = 0; i < 6; ++i)
    {
        clr[i][0]=1.0;
        clr[i][1]=0.4;
        clr[i][2]=0;
    }
    for (int i = 0; i < no_of_fixed_objects; ++i)
        fixed_object[i]=createRectangle(fixe[i][2],fixe[i][3],clr);
    for (int i = 0; i < 6; ++i)
    {
        clr[i][0]=1.0;
        clr[i][1]=0.83;
        clr[i][2]=0.2;
    }
    for (int i = 0; i < no_of_coins; ++i)
        coins_objects[i]=createSector(coins[i][2],360,clr);
    //programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
    circle1=createSector(10,360,clr);
    circle2=createSector(30,360,clr);
    for (int i = 0; i < 6; ++i)
    {
        clr[i][0]=1;
        clr[i][1]=1;
        clr[i][2]=1;
    }
    cloud=createSector(30,60,clr);
    half_circle = createSector(40,360,clr);
    rectangle = createRectangle(100,20,clr);
    for (int i = 0; i < 6; ++i)
    {
        clr[i][0]=1.0;
        clr[i][1]=0.4;
        clr[i][2]=0.6;
    }
    piggy_head=createSector(radius_of_piggy,6,clr);
    for (int i = 0; i < 6; ++i)
    {
        clr[i][0]=1;
        clr[i][1]=1;
        clr[i][2]=1;
    }
    piggy_eye=createSector(5,6,clr);
    for (int i = 0; i < 6; ++i)
    {
        clr[i][0]=0;
        clr[i][1]=0;
        clr[i][2]=0;
    }
    piggy_big_eye=createSector(7,6,clr);
    for (int i = 0; i < 6; ++i)
    {
        clr[i][0]=0;
        clr[i][1]=0;
        clr[i][2]=0;
    }
    piggy_big_nose=createSector(10,6,clr);
	for (int i = 0; i < 6; ++i)
    {
        clr[i][0]=1;
        clr[i][1]=1;
        clr[i][2]=1;
    }
    piggy_small_nose=createSector(3,6,clr);
    for (int i = 0; i < 6; ++i)
    {
        clr[i][0]=1;
        clr[i][1]=0;
        clr[i][2]=0.33;
    }
    piggy_ear=createSector(8,6,clr);
    for (int i = 0; i < 6; ++i)
    {
        clr[i][0]=1;
        clr[i][1]=0;
        clr[i][2]=0;
    }
    score_ver=createRectangle(4,18,clr);
    score_hor=createRectangle(18,4,clr);

	cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
	cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
	cout << "VERSION: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

}

int main (int argc, char** argv)
{
    intialize_objects();
    GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time;
    while (!glfwWindowShouldClose(window)) 
    {
        glfwGetCursorPos(window,&xmousePos,&ymousePos);
        draw();
        checkcollision();
        glfwSwapBuffers(window);
        glfwPollEvents();
        glfwSetScrollCallback(window, mousescroll);
       // reshapeWindow(window,width,height);
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.4) { // atleast 0.5s elapsed since last frame
            last_update_time = current_time;
        }
        no_of_piggy_hit=0;
        for (int i = 0; i < no_of_piggy; ++i)
            if (piggy_pos[i][2]==3)
                no_of_piggy_hit+=1;
        if (no_of_piggy_hit==no_of_piggy)
            quit(window);
    }
    glfwTerminate();
    exit(EXIT_SUCCESS);
}