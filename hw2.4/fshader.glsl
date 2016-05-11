uniform vec4 eye_pos;

vec4 light_position = vec4(100.0, 100.0, 100.0, 1.0);
vec4 light_ambient = vec4(0.2, 0.2, 0.2, 1.0);
vec4 light_diffuse = vec4(1.0, 1.0, 1.0, 1.0);
vec4 light_specular = vec4(1.0, 1.0, 1.0, 1.0);

vec4 material_ambient = vec4(1.0, 0.0, 1.0, 1.0);
vec4 material_diffuse = vec4(1.0, 0.8, 0.0, 1.0);
vec4 material_specular = vec4(1.0, 0.8, 0.0, 1.0);
float material_shininess = 100.0;

varying vec4 norm;
varying vec4 pos;

void main() 
{ 
	vec4 light_dir = normalize(light_position - pos);
	vec4 eye_dir = normalize(eye_pos - pos);

	vec4 ambient_color, diffuse_color, specular_color;

    ambient_color = material_ambient * light_ambient;

    float dd = dot(light_dir, norm); 
    if (dd>0.0) diffuse_color = dd * light_diffuse * material_diffuse;
        else diffuse_color =  vec4(0.0, 0.0, 0.0, 1.0);

    float sd = 0.0;
    if ( (dd>0.0) && (dd>0.0) ){
        vec4 half_vec = normalize(light_dir + eye_dir);
        sd = dot(half_vec, norm);
    }
    if(sd > 0.0) specular_color = pow(sd, material_shininess) * light_specular * material_specular;
        else specular_color = vec4(0.0, 0.0, 0.0, 1.0);

    gl_FragColor = diffuse_color + ambient_color + specular_color;
} 

