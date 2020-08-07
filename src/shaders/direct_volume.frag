#version 450

in vec3 f_texcoord;
in vec3 f_vertPosWorld;

out vec4 out_color;

uniform vec3 u_cameraPos;
uniform int u_sampleNum;
uniform vec3 u_albedo;
uniform vec3 u_marginCubeVertices[8];

uniform sampler3D u_filteredTex;

const float eps = 1.0e-6;

vec3 gammaCorrection(vec3 color) { 
    return pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));
}

float toGrayScale(vec3 col) {
    return col.r * 0.299 + col.g * 0.589 + col.b * 0.112;
}

void main(void) {
  float edgeLength = length(u_marginCubeVertices[0] - u_marginCubeVertices[3]);

  float scale = edgeLength / u_sampleNum;
  float texScale = scale / edgeLength;

  vec3 rayDir = normalize(f_vertPosWorld - u_cameraPos) * scale;
  vec3 U = u_marginCubeVertices[1] - u_marginCubeVertices[0];
  vec3 V = u_marginCubeVertices[2] - u_marginCubeVertices[0];
  vec3 W = u_marginCubeVertices[3] - u_marginCubeVertices[0];
  vec3 eU = normalize(U);
  vec3 eV = normalize(V);
  vec3 eW = normalize(W);
  float lU = length(U);
  float lV = length(V);
  float lW = length(W);

  float du = dot(rayDir, eU) / lU;
  float dv = dot(rayDir, eV) / lV;
  float dw = dot(rayDir, eW) / lW;
  vec3 texRayDir = normalize(vec3(du, dv, dw)) * texScale;

  // parameters that will be updated
  vec3 texPos = vec3(f_texcoord.x, f_texcoord.y, f_texcoord.z);

  // transmittance
  vec3 T = vec3(1.0);
  // in-scattered radiance
  vec3 Lo = vec3(0.0);

  for (int i = 0; i < u_sampleNum; i++) {      
      // skip empty space
      float density = textureLod(u_filteredTex, texPos, 0).w;
      if (density > eps) {
          // sample radiance and density
          vec3 Lri = textureLod(u_filteredTex, texPos, 0).xyz;
          vec3 Jss = Lri;

          Lo += T * Jss * scale;

          // attenuate ray-throughput
          vec3 sigmaS = u_albedo * density;
          vec3 sigmaA = density - sigmaS;
          vec3 sigmaT = sigmaS + sigmaA;
          T *= exp(-scale * sigmaT);

          if (all(lessThan(T, vec3(eps)))) { break; }
      }

      texPos += texRayDir;

      if (texPos.x > 1.0 || texPos.y > 1.0 || texPos.z > 1.0 ||
          texPos.x < 0.0 || texPos.y < 0.0 || texPos.z < 0.0) { break; }
  }

  out_color.rgb = gammaCorrection(Lo);
  out_color.a = 1.0 - toGrayScale(T);
}
