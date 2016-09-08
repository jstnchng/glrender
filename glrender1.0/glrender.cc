
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/freeglut_ext.h>
#endif

#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include "amath.h"
#include "bezier_surface.h"
#include "parser.h"

using namespace std;;

typedef amath::vec4  point4;
typedef amath::vec4  color4;

Parser parser;

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

vec4 viewer = vec4(0.0, 0.0, 2.0, 0.0);
const point4 CENTER = point4(0.0, 0.0, 0.0, 1.0);
vec4 up;

int num_vertices;
vector<vec4> points;
vector<vec4> norms;
int pc_size;

bool bezier_file = false;
bool bezier_changed = false;
vector <Bezier_Surface> b_surfaces;
const int MIN_SAMPLES = 1;
const int MAX_SAMPLES = 10;
int bezier_samples = 1;

bool checkerboard_on = false;
const int MAX_FINE = 32;
const int MIN_COARSE = 1;
int cb_coarseness = 2;

GLint checkerboard, eye_pos, ctm, ptm;

GLuint program; //shaders

void load_obj(char *file){
    std::vector < int > tris;
    std::vector < float > verts;

    parser.read_wavefront_file(file, tris, verts);
    std::cout << "found this many tris, verts: " << tris.size () / 3.0 << "  " << verts.size () / 3.0 << std::endl;
    num_vertices = tris.size();

    pc_size = sizeof(point4) * num_vertices;

    vector<vec4> vert_norms;

    // calculate norms by summing up normals from each tri
    for (int i=0; i<verts.size()/3; i++){
        vert_norms.push_back(vec4(0.0, 0.0, 0.0, 0.0));
    }

    for (int i=0; i<num_vertices; i+=3){
        points.push_back(point4( verts[3*tris[i]], verts[3*tris[i]+1], verts[3*tris[i]+2], 1.0));
        points.push_back(point4( verts[3*tris[i+1]], verts[3*tris[i+1]+1], verts[3*tris[i+1]+2], 1.0));
        points.push_back(point4( verts[3*tris[i+2]], verts[3*tris[i+2]+1], verts[3*tris[i+2]+2], 1.0));

        vec4 n = normalize( vec4(cross(points[i+1]-points[i], points[i+2]-points[i]), 0.0) );

        vert_norms[ tris[i] ] += n;
        vert_norms[ tris[i+1] ] += n;
        vert_norms[ tris[i+2] ] += n;
    }

    for (int i=0; i<verts.size()/3; i++){
        vert_norms[i] = normalize(vert_norms[i]);
    }

    for (int i=0; i<num_vertices; i+=3){
        norms.push_back(vert_norms[tris[i]]);
        norms.push_back(vert_norms[tris[i+1]]);
        norms.push_back(vert_norms[tris[i+2]]);
    }
}

void load_bezier_verts_norms(){
    points.clear();
    norms.clear();

    for (int i=0; i<b_surfaces.size(); i++){
        b_surfaces[i].sample(bezier_samples, points, norms);
    }
    num_vertices = points.size();
    pc_size = num_vertices * sizeof(point4);
}

void load_bezier(char *file){
    parser.read_bezier_file(file, b_surfaces);
    bezier_file = true;
    load_bezier_verts_norms();
}

void init()
{
    
    // create a vertex array object - this defines mameory that is stored
    // directly on the GPU
    GLuint vao;
    
    // deending on which version of the mac OS you have, you may have to do this:
#ifdef __APPLE__
    glGenVertexArraysAPPLE( 1, &vao );  
    glBindVertexArrayAPPLE( vao );      
#else
    glGenVertexArrays( 1, &vao );   
    glBindVertexArray( vao );       
#endif
    
    glGenBuffers(1, buffers);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);  
    
    glBufferData(GL_ARRAY_BUFFER, pc_size + pc_size, NULL, GL_STATIC_DRAW);
    
    // program = InitShader("vshader_blinnphong.glsl", "fshader_passthrough.glsl");
    program = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);
    
    GLuint loc, loc2;
    
    loc = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    loc2 = glGetAttribLocation(program, "vNormal");
    glEnableVertexAttribArray(loc2);
    glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(pc_size));

    glBufferSubData( GL_ARRAY_BUFFER, 0, pc_size, &points[0] );
    glBufferSubData( GL_ARRAY_BUFFER, pc_size, pc_size, &norms[0] );

    eye_pos = glGetUniformLocation(program, "eye_pos");
    ctm = glGetUniformLocation(program, "ctm");
    ptm = glGetUniformLocation(program, "ptm");
    checkerboard = glGetUniformLocation(program, "checkerboard");

    glClearColor(1.0, 1.0, 1.0, 1.0); 

}

void display( void )
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    
    GLfloat p = DegreesToRadians*phi;
    GLfloat t = DegreesToRadians*theta;
    viewer = point4(r*sin(p)*sin(t), r*cos(p), r*sin(p)*cos(t), 1.0);
    vec4 to_center = normalize(CENTER - viewer);
    vec4 to_side = normalize( cross(to_center, vec4(0, 1, 0, 0)) );
    up = normalize( vec4(cross(to_side, to_center), 0.0) );

    glUniform4fv(eye_pos, 1, viewer);

    glUniformMatrix4fv( ctm, 1, GL_TRUE, LookAt(viewer, to_center, up) );
    glUniformMatrix4fv( ptm, 1, GL_TRUE, Perspective(40, 1.0, 1, 50) );

    if (bezier_file && bezier_changed){
        load_bezier_verts_norms();
        cout << "sampling rate: " << bezier_samples << endl;

        glBufferData(GL_ARRAY_BUFFER, pc_size + pc_size, NULL, GL_STATIC_DRAW);

        GLuint loc, loc2;
    
        loc = glGetAttribLocation(program, "vPosition");
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

        loc2 = glGetAttribLocation(program, "vNormal");
        glEnableVertexAttribArray(loc2);
        glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(pc_size));

        glBufferSubData( GL_ARRAY_BUFFER, 0, pc_size, &points[0] );
        glBufferSubData( GL_ARRAY_BUFFER, pc_size, pc_size, &norms[0] );
    }   
    bezier_changed = false;

    if (checkerboard_on)
        glUniform1i(checkerboard, cb_coarseness);
    else
        glUniform1i(checkerboard, 0);

    glDrawArrays(GL_TRIANGLES, 0, num_vertices);
    
    glutSwapBuffers();
}


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

    if (key == '<' && bezier_samples > MIN_SAMPLES){
        bezier_samples--;
        bezier_changed = true;
        glutPostRedisplay();
    }

    if (key == '>' && bezier_samples < MAX_SAMPLES){
        bezier_samples++;
        bezier_changed = true;
        glutPostRedisplay();
    }

    if (key == 's') {
        checkerboard_on = (checkerboard_on) ? checkerboard_on=false : checkerboard_on=true;
        glutPostRedisplay();
    }

    if (key == 't' && checkerboard_on && cb_coarseness < MAX_FINE) {
        cb_coarseness++;
        glutPostRedisplay();
    }

    if (key == 'g' && checkerboard_on && cb_coarseness > MIN_COARSE) {
        cb_coarseness--;
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
    if(parser.is_obj_file(argv[1])){
        load_obj(argv[1]);
    } else {
        load_bezier(argv[1]);
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    
    glutInitWindowSize(512, 512);
    glutCreateWindow("Crystal Ball");
    
    glutDisplayFunc(display);

    glutMouseFunc(mouse_click);

    glutMotionFunc(mouse_move_rotate);

    glutKeyboardFunc(mykey);
    
#ifndef __APPLE__
    // initialize the extension manager: sometimes needed, sometimes not!
    glewInit();
#endif

    init();

    glEnable(GL_DEPTH_TEST);

    glutMainLoop();
    return 0;
}
