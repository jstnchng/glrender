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
#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

using namespace std;;

typedef amath::vec4  point4;
typedef amath::vec4  color4;

float theta = 0.0;  // rotation around the Y (up) axis
float phi = 90.0; // rotation around the X axis
float r = 5.0; // position on Z axis
float posx = 0.0;   // translation along X
float posy = 0.0;   // translation along Y

const float ZENITH = 175.0;
const float NADIR = 5.0;
const float RMAX = 50.0;
const float RMIN = 2.0;

GLuint buffers[2];

// viewer's position, for lighting calculations
vec4 viewer = vec4(0.0, 0.0, 2.0, 0.0);
// defines center for determining view vector
const point4 CENTER = point4(0.0, 0.0, 0.0, 1.0);
vec4 up;

int NumTris;
point4 *points;
vec4 *norms;
int pc_size;

// a transformation matrix, for the rotation, which we will apply to every vertex
// perspective transformation matrix
// eye_position (to pass to vshader)
GLint eye_pos, ctm, ptm;

GLuint program; //shaders

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
    std::vector < int > tris;
    std::vector < float > verts;

    read_wavefront_file(file, tris, verts);
    std::cout << "found this many tris, verts: " << tris.size () / 3.0 << "  " << verts.size () / 3.0 << std::endl;
    NumTris = tris.size();

    // array storing triangles & norms of each vertex
    points = new point4[NumTris];
    norms = new vec4[NumTris];
    pc_size = sizeof(point4) * NumTris;

    vec4 *vert_norms = new vec4[verts.size()/3];

    // calculate norms by summing up normals from each tri
    for (int i=0; i<verts.size()/3; i++){
        vert_norms[i] = vec4(0.0, 0.0, 0.0, 0.0);
    }

    for (int i=0; i<NumTris; i+=3){
        points[i] = point4( verts[3*tris[i]], verts[3*tris[i]+1], verts[3*tris[i]+2], 1.0);
        points[i+1] = point4( verts[3*tris[i+1]], verts[3*tris[i+1]+1], verts[3*tris[i+1]+2], 1.0);
        points[i+2] = point4( verts[3*tris[i+2]], verts[3*tris[i+2]+1], verts[3*tris[i+2]+2], 1.0);

        vec4 n = normalize( vec4(cross(points[i+1]-points[i], points[i+2]-points[i]), 0.0) );

        vert_norms[ tris[i] ] += n;
        vert_norms[ tris[i+1] ] += n;
        vert_norms[ tris[i+2] ] += n;
    }

    for (int i=0; i<verts.size()/3; i++){
        vert_norms[i] = normalize(vert_norms[i]);
    }

    for (int i=0; i<NumTris; i+=3){
        norms[i] = vert_norms[tris[i]];
        norms[i+1] = vert_norms[tris[i+1]];
        norms[i+2] = vert_norms[tris[i+2]];
    }

    delete vert_norms;
}

// product of components, which we will use for shading calculations:
vec4 product(vec4 a, vec4 b)
{   
    return vec4(a[0]*b[0], a[1]*b[1], a[2]*b[2], a[3]*b[3]);
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
    program = InitShader("vshader_blinnphong.glsl", "fshader_passthrough.glsl");
 
    // ...and set them to be active
    glUseProgram(program);
    
    
    // send two attributes: position & normal
    GLuint loc, loc2;
    
    loc = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(loc);
    
    // the vPosition attribute is a series of 4-vecs of floats, starting at the
    // beginning of the bufram
    glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    loc2 = glGetAttribLocation(program, "vNormal");
    glEnableVertexAttribArray(loc2);

    // the vNormal attribute is a series of 4-vecs of floats, starting just after
    // the points in the buffer
    glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(pc_size));

    glBufferSubData( GL_ARRAY_BUFFER, 0, pc_size, points );
    glBufferSubData( GL_ARRAY_BUFFER, pc_size, pc_size, norms );

    // get uniform locations for the GLints
    eye_pos = glGetUniformLocation(program, "eye_pos");
    ctm = glGetUniformLocation(program, "ctm");
    ptm = glGetUniformLocation(program, "ptm");
    
    // set the background color (white)
    glClearColor(1.0, 1.0, 1.0, 1.0); 

}

void display( void )
{
 
    // clear the window (with white) and clear the z-buffer (which isn't used
    // for this example).
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    
    // convert to cartesian, get up vector
    GLfloat p = DegreesToRadians*phi;
    GLfloat t = DegreesToRadians*theta;
    viewer = point4(r*sin(p)*sin(t), r*cos(p), r*sin(p)*cos(t), 1.0);
    vec4 to_center = normalize(CENTER - viewer);
    vec4 to_side = normalize( cross(to_center, vec4(0, 1, 0, 0)) );
    up = normalize( vec4(cross(to_side, to_center), 0.0) );

    glUniform4fv(eye_pos, 1, viewer);

    // construct ctm and ptm using LookAt and Perspective
    glUniformMatrix4fv( ctm, 1, GL_TRUE, LookAt(viewer, to_center, up) );
    glUniformMatrix4fv( ptm, 1, GL_TRUE, Perspective(40, 1.0, 1, 50) );
    
    // draw the VAO:
    glDrawArrays(GL_TRIANGLES, 0, NumTris);
    
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
    // keep track of where the mouse was last:
    static int lastx = 0;
    static int lasty = 0;
    
    int amntX = x - lastx; 
    if (amntX != 0) {
        theta +=  amntX;
        if (theta > 360.0 ) theta -= 360.0;
        if (theta < 0.0 ) theta += 360.0;
        
        lastx = x;
    }

    int amntY = y - lasty;
    if (amntY != 0) {
        phi += amntY;
        if (phi > ZENITH) phi = ZENITH;
        if (phi < NADIR) phi = NADIR;

        lasty = y;
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
        phi = 0;
        r = 5.0;
        glutPostRedisplay();
    }

    // moves view closer
    if (key == 'z' && r > RMIN) {
        r *= 0.9;
        glutPostRedisplay();
    }

    if (key == 'x' && r < RMAX) {
        r *= 1.1;
        glutPostRedisplay();
    }
}

void mouse_click (int button, int state, int x, int y) {
    static int lastx = 0;
    static int lasty = 0;

    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        lastx = x;
        lasty = y;
    }

    glutPostRedisplay();
}


int main(int argc, char** argv)
{
    load_obj(argv[1]);

    // initialize glut, and set the display modes
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    
    // give us a window in which to display, and set its title:
    glutInitWindowSize(512, 512);
    glutCreateWindow("Rotate OBJ file");
    
    // for displaying things, here is the callback specification:
    glutDisplayFunc(display);

    glutMouseFunc(mouse_click);
    
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
