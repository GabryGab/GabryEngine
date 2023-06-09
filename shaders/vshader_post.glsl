#version 460 core

out vec2 tc;

void main() {
	if (gl_VertexID == 0) {
		gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
		tc = vec2(0.0, 0.0);
	} else if (gl_VertexID == 1) {
		gl_Position = vec4(1.0, 1.0, 0.0, 1.0);
		tc = vec2(1.0, 1.0);
	} else if (gl_VertexID == 2) {
		gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);
		tc = vec2(0.0, 1.0);
	} else if (gl_VertexID == 3) {
		gl_Position = vec4(1.0, -1.0, 0.0, 1.0);
		tc = vec2(1.0, 0.0);
	} else if (gl_VertexID == 4) {
		gl_Position = vec4(1.0, 1.0, 0.0, 1.0);
		tc = vec2(1.0, 1.0);
	} else if (gl_VertexID == 5) {
		gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
		tc = vec2(0.0, 0.0);
	}
}	
