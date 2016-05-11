// Very simple display triangle program, that allows you to rotate the
// triangle around the Y axis.
//
// This program does NOT use a vertex shader to define the vertex colors.
// Instead, it computes the colors in the display callback (using Blinn/Phong)
// and passes those color values, one per vertex, to the vertex shader, which
// passes them directly to the fragment shader. This achieves what is called
// "gouraud shading".

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/freeglut_ext.h>
#endif

#include "amath.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

using namespace std;;

int NumVertices;

typedef amath::vec4  point4;
typedef amath::vec4  color4;

float theta = 0.0;  // rotation around the Y (up) axis
float posx = 0.0;   // translation along X
float posy = 0.0;   // translation along Y

GLuint buffers[2];
GLint matrix_loc;

// viewer's position, for lighting calculations
vec4 viewer = vec4(0.0, 0.0, -1.0, 0.0);

// light & material definitions, again for lighting calculations:
point4 light_position = point4(0.0, 0.0, -1.0, 0.0);
color4 light_ambient = color4(0.2, 0.2, 0.2, 1.0);
color4 light_diffuse = color4(1.0, 1.0, 1.0, 1.0);
color4 light_specular = color4(1.0, 1.0, 1.0, 1.0);

color4 material_ambient = color4(1.0, 0.0, 1.0, 1.0);
color4 material_diffuse = color4(1.0, 0.8, 0.0, 1.0);
color4 material_specular = color4(1.0, 0.8, 0.0, 1.0);
float material_shininess = 100.0;

// // we will copy our transformed points to here:
// point4 points[NumVertices];

// // and we will store the colors, per face per vertex, here. since there is
// // only 1 triangle, with 3 vertices, there will just be 3 here:
// color4 colors[NumVertices];
point4 *points;
color4 *colors;
int pc_size;

// a transformation matrix, for the rotation, which we will apply to every
// vertex:
mat4 ctm;

GLuint program; //shaders

std::vector < int > tris;
std::vector < float > verts;

void read_wavefront_file(char *file, 
    std::vector < int > &tris,
    std::vector< float > &verts){

    tris.clear ();
    verts.clear ();

    ifstream in(file);
    char buffer[1025];
    string cmd;
    
    for (int line=1; in.good(); line++) {
        in.getline(buffer,1024);
        buffer[in.gcount()]=0;
                
        cmd="";
        
        istringstream iss (buffer);
        
        iss >> cmd;
        
        if (cmd[0]=='#' or cmd.empty()) {
            // ignore comments or blank lines
            continue;
        } 
        else if (cmd=="v") {
            // got a vertex:
            
            // read in the parameters:
            double pa, pb, pc;
            iss >> pa >> pb >> pc;
 
            verts.push_back (pa);
            verts.push_back (pb);
            verts.push_back (pc);
         } 
        else if (cmd=="f") {
            // got a face (triangle)
            
            // read in the parameters:
            int i, j, k;
            iss >> i >> j >> k;
            
            // vertex numbers in OBJ files start with 1, but in C++ array
            // indices start with 0, so we're shifting everything down by
            // 1
            tris.push_back (i-1);
            tris.push_back (j-1);
            tris.push_back (k-1);
        } 
        else {
            std::cerr << "Parser error: invalid command at line " << line << std::endl;
        }
        
     }
    
    in.close();
    
    std::cout << "found this many tris, verts: " << tris.size () / 3.0 << "  " 
              << verts.size () / 3.0 << std::endl;
}

void load_obj(char *file){
    read_wavefront_file(file, tris, verts);
    std::cout << "found this many tris, verts: " << tris.size () / 3.0 << "  " << verts.size () / 3.0 << std::endl;
    // NumVertices = verts.size () / 3.0;
    NumVertices = tris.size();
}

// product of components, which we will use for shading calculations:
vec4 product(vec4 a, vec4 b)
{   
    return vec4(a[0]*b[0], a[1]*b[1], a[2]*b[2], a[3]*b[3]);
}    

// transform the triangle's vertex data and put it into the points array.
// also, compute the lighting at each vertex, and put that into the colors
// array.
void tri()
{
    for(int i=0; i<tris.size()/3; i++){
        point4  vertices[3] = {
            point4( verts[3*tris[3*i]], verts[3*tris[3*i]+1], verts[3*tris[3*i]+2], 1.0),
            point4( verts[3*tris[3*i+1]], verts[3*tris[3*i+1]+1], verts[3*tris[3*i+1]+2], 1.0),
            point4( verts[3*tris[3*i+2]], verts[3*tris[3*i+2]+1], verts[3*tris[3*i+2]+2], 1.0)
        };

        // compute the lighting at each vertex, then set it as the color there:
        vec3 n1 = normalize(cross(ctm*vertices[1] - ctm*vertices[0],
                                  ctm*vertices[2] - ctm*vertices[1]));
        vec4 n = vec4(n1[0], n1[1], n1[2], 0.0);
        vec4 half = normalize(light_position+viewer);
        color4 ambient_color, diffuse_color, specular_color;

        ambient_color = product(material_ambient, light_ambient);
        float dd = dot(light_position, n);

        if(dd>0.0) diffuse_color = dd*product(light_diffuse, material_diffuse);
        else diffuse_color =  color4(0.0, 0.0, 0.0, 1.0);

        dd = dot(half, n);
        if(dd > 0.0) specular_color = exp(material_shininess*log(dd))*product(light_specular, material_specular);
        else specular_color = vec4(0.0, 0.0, 0.0, 1.0);

        // now transform the vertices according to the ctm transformation matrix,
        // and set the colors for each of them as well. as we are going to give
        // flat shading, we will ingore the specular component for now.
        points[3*i] = ctm*vertices[0];
        colors[3*i] = ambient_color + diffuse_color;
        
        points[3*i+1] = ctm*vertices[1];
        colors[3*i+1] = ambient_color + diffuse_color;
        
        points[3*i+2] = ctm*vertices[2];
        colors[3*i+2] = ambient_color + diffuse_color;
    }
}


// initialization: set up a Vertex Array Object (VAO) and then
void init()
{
    
    // create a vertex array object - this defines mameory that is stored
    // directly on the GPU
    GLuint vao;
    
    // deending on which version of the mac OS you have, you may have to do this:
#ifdef __APPLE__
    glGenVertexArraysAPPLE( 1, &vao );  // give us 1 VAO:
    glBindVertexArrayAPPLE( vao );      // make it active
#else
    glGenVertexArrays( 1, &vao );   // give us 1 VAO:
    glBindVertexArray( vao );       // make it active
#endif
    
    // set up vertex buffer object - this will be memory on the GPU where
    // we are going to store our vertex data (that is currently in the "points"
    // array)
    glGenBuffers(1, buffers);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);  // make it active
    
    // specify that its part of a VAO, what its size is, and where the
    // data is located, and finally a "hint" about how we are going to use
    // the data (the driver will put it in a good memory location, hopefully)
    glBufferData(GL_ARRAY_BUFFER, pc_size + pc_size, NULL, GL_STATIC_DRAW);
    
    // load in these two shaders...  (note: InitShader is defined in the
    // accompanying initshader.c code).     
    // the shaders themselves must be text glsl files in the same directory
    // as we are running this program:
    program = InitShader("vshader_passthrough.glsl", "fshader_passthrough.glsl");
 
    // ...and set them to be active
    glUseProgram(program);
    
    
    // this time, we are sending TWO attributes through: the position of each
    // transformed vertex, and the color we have calculated in tri().
    GLuint loc, loc2;
    
    loc = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(loc);
    
    // the vPosition attribute is a series of 4-vecs of floats, starting at the
    // beginning of the buffer
    glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    loc2 = glGetAttribLocation(program, "vColor");
    glEnableVertexAttribArray(loc2);

    // the vColor attribute is a series of 4-vecs of floats, starting just after
    // the points in the buffer
    glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(pc_size));
    
    // set the background color (white)
    glClearColor(1.0, 1.0, 1.0, 1.0); 
}



void display( void )
{
 
    // clear the window (with white) and clear the z-buffer (which isn't used
    // for this example).
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    
    // based on where the mouse has moved to, construct a transformation matrix:
    ctm = Translate(posx*.01,posy*.01,0.)* RotateY(theta); 

    // now build transform all the vertices and put them in the points array,
    // and their colors in the colors array:
    tri();
    
    // tell the VBO to get the data from the points arrat and the colors array:
    glBufferSubData( GL_ARRAY_BUFFER, 0, pc_size, points );
    glBufferSubData( GL_ARRAY_BUFFER, pc_size, pc_size, colors );
    
    // draw the VAO:
    glDrawArrays(GL_TRIANGLES, 0, NumVertices);
    
    
    // move the buffer we drew into to the screen, and give us access to the one
    // that was there before:
    glutSwapBuffers();
}


// use this motionfunc to demonstrate rotation - it adjusts "theta" based
// on how the mouse has moved. Theta is then used the the display callback
// to generate the transformation, ctm, that is applied
// to all the vertices before they are displayed:
void mouse_move_rotate (int x, int y)
{
    
    static int lastx = 0;// keep track of where the mouse was last:
    
    int amntX = x - lastx; 
    if (amntX != 0) {
        theta +=  amntX;
        if (theta > 360.0 ) theta -= 360.0;
        if (theta < 0.0 ) theta += 360.0;
        
        lastx = x;
    }

    // force the display routine to be called as soon as possible:
    glutPostRedisplay();
    
}


// use this motionfunc to demonstrate translation - it adjusts posx and
// posy based on the mouse movement. posx and posy are then used in the
// display callback to generate the transformation, ctm, that is applied
// to all the vertices before they are displayed:
void mouse_move_translate (int x, int y)
{
    
    static int lastx = 0;
    static int lasty = 0;  // keep track of where the mouse was last:

    if (x - lastx < 0) --posx;
    else if (x - lastx > 0) ++posx;
    lastx = x;

    if (y - lasty > 0) --posy;
    else if (y - lasty < 0) ++posy;
    lasty = y;
    
    // force the display routine to be called as soon as possible:
    glutPostRedisplay();
    
}


// the keyboard callback, called whenever the user types something with the
// regular keys.
void mykey(unsigned char key, int mousex, int mousey)
{
    if(key=='q'||key=='Q') exit(0);
    
    // and r resets the view:
    if (key =='r') {
        posx = 0;
        posy = 0;
        theta = 0;
        glutPostRedisplay();
    }
}



int main(int argc, char** argv)
{
    load_obj(argv[1]);
    point4 *t_points = (point4 *) malloc(sizeof(point4) * NumVertices); 
    color4 *t_colors = (color4 *) malloc(sizeof(color4) * NumVertices);
    points = t_points;
    colors = t_colors;
    pc_size = sizeof(point4) * NumVertices;

    // initialize glut, and set the display modes
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    
    // give us a window in which to display, and set its title:
    glutInitWindowSize(512, 512);
    glutCreateWindow("Rotate / Translate Triangle");
    
    // for displaying things, here is the callback specification:
    glutDisplayFunc(display);
    
    // when the mouse is moved, call this function!
    // you can change this to mouse_move_translate to see how it works
    glutMotionFunc(mouse_move_rotate);
 
    // for any keyboard activity, here is the callback:
    glutKeyboardFunc(mykey);
    
#ifndef __APPLE__
    // initialize the extension manager: sometimes needed, sometimes not!
    glewInit();
#endif

    // call the init() function, defined above:
    init();
    
    // enable the z-buffer for hidden surface removel:
    glEnable(GL_DEPTH_TEST);

    // once we call this, we no longer have control except through the callbacks:
    glutMainLoop();
    return 0;
}
