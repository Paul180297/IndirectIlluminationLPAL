#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texcoord;
layout(location = 2) in vec3 in_normal;

out vec3 f_vertPosWorld;
out vec2 f_texcoord;
out vec3 f_normalWorld;

uniform vec3 u_lightPos;
uniform mat4 u_lightMat;

uniform mat4 u_mMat;
uniform mat4 u_mvMat;
uniform mat4 u_mvpMat;
uniform mat4 u_normMat;

void main(void) {
    gl_Position = u_mvpMat * vec4(in_position, 1.0);
    f_vertPosWorld = (u_mMat * vec4(in_position, 1.0)).xyz;
    f_texcoord = vec2(in_texcoord.x, in_texcoord.y);
    f_normalWorld = (u_mMat * vec4(in_normal, 0.0)).xyz;
}
