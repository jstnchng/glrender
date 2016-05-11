// a contrived vertex shader, that colors each vertex in a triangle
// according to its position

// we are going to be getting an attribute from the main program, named
// "vPosition", one for each vertex.
attribute vec4 vPosition;

// we are also going to be getting color values, per-vertex, from the main
// program running on the cpu (and that are then stored in the VBO on the
// card. this color, called "vColor", is just going to be passed through 
// to the fragment shader, which will intrpolate its value per-fragment
// amongst the 3 vertex colors on the triangle that fragment is a part of.
attribute vec4 vNormal;

// we are going to be outputting a single 4-vector, called color, which
// may be different for each vertex.
// the fragment shader will be expecting these values, and interpolate
// them according to the distance of the fragment from thevertices
varying vec4 color;

uniform mat4 ctm;
uniform mat4 ptm;

uniform vec4 eye_pos;

vec4 light_position = vec4(100.0, 100.0, 100.0, 1.0);
vec4 light_ambient = vec4(0.2, 0.2, 0.2, 1.0);
vec4 light_diffuse = vec4(1.0, 1.0, 1.0, 1.0);
vec4 light_specular = vec4(1.0, 1.0, 1.0, 1.0);

vec4 material_ambient = vec4(1.0, 0.0, 1.0, 1.0);
vec4 material_diffuse = vec4(1.0, 0.8, 0.0, 1.0);
vec4 material_specular = vec4(1.0, 0.8, 0.0, 1.0);
float material_shininess = 100.0;

void main() 
{
    vec4 light_dir = normalize(light_position - vPosition);
    vec4 eye_dir = normalize(eye_pos - vPosition);

    vec4 ambient_color, diffuse_color, specular_color;

    ambient_color = material_ambient * light_ambient;

    // calculate blinn phong here
    float dd = dot(light_dir, vNormal); 
    if (dd>0.0) diffuse_color = dd * light_diffuse * material_diffuse;
        else diffuse_color =  vec4(0.0, 0.0, 0.0, 1.0);

    float sd = 0.0;
    if ( (dd>0.0) && (dd>0.0) ){
        vec4 half_vec = normalize(light_position + eye_pos);
        sd = dot(half_vec, vNormal);
    }
    if(sd > 0.0) specular_color = pow(sd, material_shininess) * light_specular * material_specular;
        else specular_color = vec4(0.0, 0.0, 0.0, 1.0);

    gl_Position = ptm * ctm * vPosition;

    color = diffuse_color + ambient_color + specular_color;
} 



