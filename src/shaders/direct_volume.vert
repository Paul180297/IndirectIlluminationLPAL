#version 450

vec3 positions[8] = vec3[](
    vec3(-1.0f, -1.0f, -1.0f),
    vec3( 1.0f, -1.0f, -1.0f),
    vec3(-1.0f,  1.0f, -1.0f),
    vec3(-1.0f, -1.0f,  1.0f),
    vec3( 1.0f,  1.0f, -1.0f),
    vec3(-1.0f,  1.0f,  1.0f),
    vec3( 1.0f, -1.0f,  1.0f),
    vec3( 1.0f,  1.0f,  1.0f)
);

int indices[36] = int[](
    3, 7, 5, 3, 6, 7, 1, 7, 6, 1, 4, 7, 
    0, 4, 1, 0, 2, 4, 0, 5, 2, 0, 3, 5,
    2, 5, 7, 2, 7, 4, 0, 1, 6, 0, 6, 3 
);

out vec3 f_texcoord;
out vec3 f_vertPosWorld;

uniform mat4 u_mMat;
uniform mat4 u_mvpMat;

void main(){
    vec3 pos = positions[indices[gl_VertexID]];
    gl_Position = u_mvpMat * vec4(pos, 1.0);
    f_texcoord = pos * 0.5 + 0.5;
    f_vertPosWorld = (u_mMat * vec4(pos, 1.0)).xyz;
}
