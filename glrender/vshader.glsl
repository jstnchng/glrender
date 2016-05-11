attribute vec4 vPosition;
attribute vec4 vNormal;

uniform mat4 ctm;
uniform mat4 ptm;

varying vec4 norm;
varying vec4 pos;

void main() 
{
	norm = vNormal;
	pos = vPosition;
	gl_Position = ptm * ctm * vPosition;
} 
