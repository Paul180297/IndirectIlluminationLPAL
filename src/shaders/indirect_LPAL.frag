#version 450

// ----------------------------------------------------------------------------
// Input
// ----------------------------------------------------------------------------
in vec3 f_vertPosWorld;
in vec2 f_texcoord;
in vec3 f_normalWorld;

// ----------------------------------------------------------------------------
// Output
// ----------------------------------------------------------------------------
out vec4 out_color;

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
const float EPS = 1.0e-6;
const float PI = 3.14159265358979323846264338327950288;
const float TWO_PI = 2.0 * PI;
const float HALF_PI = 0.5 * PI;
const float INV_PI = 1.0 / PI;
const float INV_TWO_PI = 1.0 / TWO_PI;
const float INV_HALF_PI = 1.0 / HALF_PI;

const float HALF_FLOAT_MAX = 65504.0;

// ----------------------------------------------------------------------------
// Parameters
// ----------------------------------------------------------------------------

// Camera
uniform vec3 u_cameraPos;

// Light
uniform vec3 u_lightPos;
uniform vec3 u_lightLe;

// Material properties
uniform vec3 u_diffColor = vec3(0.0, 0.0, 0.0);
uniform vec3 u_eta = vec3(0.049889, 0.053285, 0.049317);  // index of refraction (for silver)
uniform vec3 u_kappa = vec3(4.4869, 3.4101, 2.8545);      // extinction coefficient (for silver)
uniform float u_alpha = 0.001;
uniform sampler2D u_alphaTex;
uniform bool u_isAlphaTextured = false;

// Volume parameters
uniform vec3 u_albedo;
uniform int u_maxLOD;
uniform int u_sectionNum;

// Volume cube
uniform vec3 u_cubeCenter;
uniform vec3 u_marginCubeVertices[8];
uniform vec3 u_originalCubeVertices[8];
uniform sampler3D u_filteredTex;

// Lookup table for LTC based area lighting
const float LUT_SIZE  = 64.0;
const float LUT_SCALE = (LUT_SIZE - 1.0) / LUT_SIZE;
const float LUT_BIAS  = 0.5 / LUT_SIZE;
uniform sampler2D u_ltcMatTex;
uniform sampler2D u_ltcMagTex;

// Polygon object used in LTC
struct Polygon {
    vec3 coord[4];
    vec3 normal;
};

// ----------------------------------------------------------------------------
// GGX-based microfacet BRDF
// ----------------------------------------------------------------------------
vec3 FresnelConductor(vec3 L, vec3 N, vec3 eta, vec3 kappa) {
    float cosThetaI = max(0.0, dot(N, L));
    float cosThetaI2 = cosThetaI * cosThetaI;
    float sinThetaI2 = 1.0 - cosThetaI2;
    float sinThetaI4 = sinThetaI2*sinThetaI2;
    vec3 eta2 = eta * eta;
    vec3 k2 = kappa * kappa;

    vec3 temp0 = eta2 - k2 - sinThetaI2;
    vec3 a2pb2 = sqrt(max(vec3(0.0), temp0 * temp0 + 4.0 * k2 * eta2));
    vec3 a = sqrt(max(vec3(0.0), (a2pb2 + temp0) * 0.5));

    vec3 temp1 = a2pb2 + vec3(cosThetaI2);
    vec3 temp2 = 2.0 * a * cosThetaI;
    vec3 Rs2 = (temp1 - temp2) / (temp1 + temp2);

    vec3 temp3 = a2pb2 * cosThetaI2 + vec3(sinThetaI4);
    vec3 temp4 = temp2 * sinThetaI2;
    vec3 Rp2 = Rs2 * (temp3 - temp4) / (temp3 + temp4);

    return 0.5 * (Rp2 + Rs2);
}

float lambda(vec3 N, vec3 V, float alpha) {
    float c = dot(N, V);
    float c2 = c * c;
    float s2 = 1.0 - c2;
    float absTanTheta = abs(s2 / c2);
    if (isinf(absTanTheta)){ 
        return 0.0;
    }

    float alpha2Tan2Theta = alpha * alpha * absTanTheta * absTanTheta;
    return 0.5 * (-1.0 + sqrt(1.0 + alpha2Tan2Theta));
}

float SmithMasking(float lambdaL, float lambdaV) {
    return 1.0 / (1.0 + lambdaL + lambdaV); 
}

float GGX(vec3 H, vec3 N, float alpha) {
    float NdotH = max(dot(N, H), 0.0);
    float root = alpha / (1.0 + NdotH * NdotH * (alpha * alpha - 1.0));
    return min(root * root * INV_PI, HALF_FLOAT_MAX);
}

// ----------------------------------------------------------------------------
// LTC utility functions
// ----------------------------------------------------------------------------

void clipQuadToHorizon(out int n, vec3 nonClippedL[4], out vec3 L[5]) {
    // copy for clipping
    L[0] = nonClippedL[0];
    L[1] = nonClippedL[1];
    L[2] = nonClippedL[2];
    L[3] = nonClippedL[3];
    L[4] = nonClippedL[0];

    // detect clipping config
    int config = 0;
    if (nonClippedL[0].z > 0.0) { config += 1; }
    if (nonClippedL[1].z > 0.0) { config += 2; }
    if (nonClippedL[2].z > 0.0) { config += 4; }
    if (nonClippedL[3].z > 0.0) { config += 8; }

    // clip
    n = 0;

    switch (config) {
      case 0:
      {
        n = 0; // clip all
      }
      break;

      case 1:
      {
        n = 3;
        L[1] = -nonClippedL[1].z * nonClippedL[0] + nonClippedL[0].z * nonClippedL[1];
        L[2] = -nonClippedL[3].z * nonClippedL[0] + nonClippedL[0].z * nonClippedL[3];
      }
      break;

      case 2:
      {
        n = 3;
        L[0] = -nonClippedL[0].z * nonClippedL[1] + nonClippedL[1].z * nonClippedL[0];
        L[2] = -nonClippedL[2].z * nonClippedL[1] + nonClippedL[1].z * nonClippedL[2];
      }
      break;

      case 3:
      {
        n = 4;
        L[2] = -nonClippedL[2].z * nonClippedL[1] + nonClippedL[1].z * nonClippedL[2];
        L[3] = -nonClippedL[3].z * nonClippedL[0] + nonClippedL[0].z * nonClippedL[3];
      }
      break;

      case 4:
      {
        n = 3;
        L[0] = -nonClippedL[3].z * nonClippedL[2] + nonClippedL[2].z * nonClippedL[3];
        L[1] = -nonClippedL[1].z * nonClippedL[2] + nonClippedL[2].z * nonClippedL[1];
      }
      break;

      case 5:
      {
        n = 0;
      }
      break;

      case 6:
      {
        n = 4;
        L[0] = -nonClippedL[0].z * nonClippedL[1] + nonClippedL[1].z * nonClippedL[0];
        L[3] = -nonClippedL[3].z * nonClippedL[2] + nonClippedL[2].z * nonClippedL[3];
      }
      break;

      case 7:
      {
        n = 5;
        L[4] = -nonClippedL[3].z * nonClippedL[0] + nonClippedL[0].z * nonClippedL[3];
        L[3] = -nonClippedL[3].z * nonClippedL[2] + nonClippedL[2].z * nonClippedL[3];
      }
      break;

      case 8:
      {
        n = 3;
        L[0] = -nonClippedL[0].z * nonClippedL[3] + nonClippedL[3].z * nonClippedL[0];
        L[1] = -nonClippedL[2].z * nonClippedL[3] + nonClippedL[3].z * nonClippedL[2];
        L[2] =  nonClippedL[3];
      }
      break;

      case 9:
      {
        n = 4;
        L[1] = -nonClippedL[1].z * nonClippedL[0] + nonClippedL[0].z * nonClippedL[1];
        L[2] = -nonClippedL[2].z * nonClippedL[3] + nonClippedL[3].z * nonClippedL[2];
      }
      break;

      case 10:
      {
        n = 0;
      }
      break;

      case 11:
      {
        n = 5;
        L[4] = nonClippedL[3];
        L[3] = -nonClippedL[2].z * nonClippedL[3] + nonClippedL[3].z * nonClippedL[2];
        L[2] = -nonClippedL[2].z * nonClippedL[1] + nonClippedL[1].z * nonClippedL[2];
      }
      break;

      case 12:
      {
        n = 4;
        L[1] = -nonClippedL[1].z * nonClippedL[2] + nonClippedL[2].z * nonClippedL[1];
        L[0] = -nonClippedL[0].z * nonClippedL[3] + nonClippedL[3].z * nonClippedL[0];
      }
      break;

      case 13:
      {
        n = 5;
        L[4] = nonClippedL[3];
        L[3] = nonClippedL[2];
        L[2] = -nonClippedL[1].z * nonClippedL[2] + nonClippedL[2].z * nonClippedL[1];
        L[1] = -nonClippedL[1].z * nonClippedL[0] + nonClippedL[0].z * nonClippedL[1];
      }
      break;

      case 14:
      {
        n = 5;
        L[4] = -nonClippedL[0].z * nonClippedL[3] + nonClippedL[3].z * nonClippedL[0];
        L[0] = -nonClippedL[0].z * nonClippedL[1] + nonClippedL[1].z * nonClippedL[0];
      }
      break;

      case 15:
      {
        n = 4;
      }
      break;
    }

    if (n == 3) {
        L[3] = nonClippedL[0];
        L[4] = nonClippedL[0];
      }
    if (n == 4) {
      L[4] = nonClippedL[0];
    }
}

void calcTransformMats(vec3 N, vec3 V, mat3 invM, out mat3 toCC) {
  vec3 T1, T2;
  T1 = normalize(V - N * dot(V, N));
  T2 = cross(N, T1);

  mat3 toTS = transpose(mat3(T1, T2, N));

  toCC = invM * toTS; // transformation matrix from world space to clamped-cosine(CC) space
}

void calcLvector (mat3 toCC, vec3 pos, Polygon polygon, out vec3 nonClippedL[4]) {
  // calculate area light non-clipped coordinates in clamped-cosine space using transformation matrix
  nonClippedL[0] = toCC * (polygon.coord[0] - pos);
  nonClippedL[1] = toCC * (polygon.coord[1] - pos);
  nonClippedL[2] = toCC * (polygon.coord[2] - pos);
  nonClippedL[3] = toCC * (polygon.coord[3] - pos);
}

vec3 calcFvector(vec3 v1, vec3 v2) {
    // integration for one edge (v1 to v2) of area light, please refer to the URL for calculation details
    // Eric Heitz et. at. SIGGRAPH2016 talk slides http://advances.realtimerendering.com/s2016/s2016_ltc_rnd.pdf
    float cosTheta = dot(v1, v2);
    float absCosTheta = abs(cosTheta);

    float a = 5.42031 + (3.12829 + 0.0902326 * absCosTheta) * absCosTheta;
    float b = 3.45068 + (4.18814 + absCosTheta) * absCosTheta;
    float thetaOverSinTheta = a / b;

    if (cosTheta < 0.0) {
        thetaOverSinTheta = PI * inversesqrt(1.0 - cosTheta * cosTheta) - thetaOverSinTheta;
    }

    return thetaOverSinTheta * cross(v1, v2);
}

vec3 evaluateLTCspec(vec3 L[5], int n, bool twoSided, out vec3 totF) {
    
    // skipe if the entire area light is below the reflective surface
    if (n == 0) { return vec3(0.0); }

    // project onto sphere
    vec3 L0 = normalize(L[0]);
    vec3 L1 = normalize(L[1]);
    vec3 L2 = normalize(L[2]);
    vec3 L3 = normalize(L[3]);
    vec3 L4 = normalize(L[4]);

    totF = vec3(0.0); // storing integrated value in vector form
    float sum = 0.0;

    // integrate edges one by one
    totF += calcFvector(L0, L1);
    totF += calcFvector(L1, L2);
    totF += calcFvector(L2, L3);

    if (n >= 4) { // if quadrangular area light
      totF += calcFvector(L3, L4);
    }
    if (n == 5) { // if pentagonal area light
      totF += calcFvector(L4, L0);
   }

    sum = totF.z; // get the z-coord value to complete integration
    sum = twoSided ? abs(sum) : max(0.0, sum);

    vec3 Lo_i = vec3(sum);

    return Lo_i;
}

// ----------------------------------------------------------------------------
// Volume indirect illumination
// ----------------------------------------------------------------------------

vec3 evaluateDiffBySampling(vec3 pos, vec3 norm) {
    /*
     * As explained in the paper, our current implementation computes the diffuse indirect
     * illumination just using a simple sampling based strategy. This method is much faster
     * than that using LPAL-based accumulation and its result is reasonable in practice.
     */
    float density = textureLod(u_filteredTex, vec3(0.5, 0.5, 0.5), u_maxLOD).w;
    vec3 sigmaS = u_albedo * density;
    vec3 sigmaA = density - sigmaS;
    vec3 sigmaT = sigmaS + sigmaA;

    float avgRadius = 0.5 * length(u_marginCubeVertices[0] - u_marginCubeVertices[1]);
    vec3 avgAttn = exp(-sigmaT * avgRadius);

    int divide = 3;
    int total = divide * divide * divide;
    vec3 sum = vec3(0.0);
    for (int i = 0; i < total; i++) {
        float u = float(i % (divide * divide));
        float v = float((i / divide) % divide);
        float w = float(i / (divide * divide));
        u = (u + 0.5) / float(divide);
        v = (v + 0.5) / float(divide);
        w = (w + 0.5) / float(divide);
        
        vec3 p1 = (1.0 - u) * u_marginCubeVertices[0] + u * u_marginCubeVertices[1];
        vec3 p2 = (1.0 - u) * u_marginCubeVertices[2] + u * u_marginCubeVertices[4];
        vec3 p3 = (1.0 - u) * u_marginCubeVertices[5] + u * u_marginCubeVertices[7];
        vec3 p4 = (1.0 - u) * u_marginCubeVertices[3] + u * u_marginCubeVertices[6];
        
        vec3 q1 = (1.0 - v) * p1 + v * p2;
        vec3 q2 = (1.0 - v) * p3 + v * p4;

        vec3 p = (1.0 - w) * q1 + w * q2;

        float dist = length(p - pos);
        vec3 L = normalize(p - pos);
        float attn = dot(L, norm) / (dist * dist);

        sum += textureLod(u_filteredTex, vec3(u, v, w), 0.0).rgb * attn;
    }

    float regularCubeVolume = 8.0; // [-1, 1]^3
    vec3 avgAlbedo = textureLod(u_filteredTex, vec3(0.5, 0.5, 0.5), u_maxLOD).rgb;

    return abs(avgAlbedo * regularCubeVolume * sum * avgAttn / total);
}

void initPolygon(out Polygon polygon, vec3 norm) {
    polygon.coord[0] = vec3(0.0);
    polygon.coord[1] = vec3(0.0);
    polygon.coord[2] = vec3(0.0);
    polygon.coord[3] = vec3(0.0);
    polygon.normal = norm;
}

float calcRadian(vec3 n, vec3 v1, vec3 v2) {
    // compute the angular difference between two vectors "v1" and "v2"
    float phi = acos(dot(v1, v2));
    float theta = dot(n, cross(v1, v2));
    if (theta < 0.0) { // correction for opposite-wise angle, ex. anticlock-wise angle to clock-wise angle
        return TWO_PI - phi;
    }
    return phi;
}

vec4 multQuat (vec4 a, vec4 b) {
    // quaternion multiplication
    float abX = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    float abY = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    float abZ = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    float abW = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    return vec4(abX, abY, abZ, abW);
}

vec3 rotateVector (vec3 p, vec3 rotAxis, float rotRad) {
    // vector rotation using quaternion
    float s = sin(0.5 * rotRad);
    float qX = rotAxis.x * s;
    float qY = rotAxis.y * s;
    float qZ = rotAxis.z * s;
    float qW = cos(0.5 * rotRad);
    vec4 quat = vec4(qX, qY, qZ, qW);

    vec4 qv = multQuat(quat, vec4(p, 0.0));
    return multQuat(qv, vec4(vec3(-quat.xyz), quat.w)).xyz;
}

void createTSD(vec3 cubeZ, out vec3 TSDvertices[8]) {
    //*****details are written in our paper, in the section 3.5 Volume domain transform.*****//

    // set base axes Ex, Ey and Ez
    vec3 Ex = vec3(1.0, 0.0, 0.0);
    vec3 Ey = vec3(0.0, 1.0, 0.0);
    vec3 Ez = vec3(0.0, 0.0, 1.0);

    // calculate axes of rotated bounding cube using vector "CubeZ" which is facing the reflective point from original cube center
    vec3 proj = normalize(vec3(cubeZ.x, 0.0, cubeZ.z));
    float rotRad = calcRadian(Ey, Ez, proj);
    vec3 cubeX = rotateVector(Ex, Ey, rotRad);
    vec3 cubeY = cross(cubeZ, cubeX);

    // ortho projection for expanding rotated cube
    float l0 = abs(dot(u_originalCubeVertices[5] - u_cubeCenter, cubeX));
    float l1 = abs(dot(u_originalCubeVertices[3] - u_cubeCenter, cubeX));
    float l2 = abs(dot(u_originalCubeVertices[6] - u_cubeCenter, cubeX));
    float l3 = abs(dot(u_originalCubeVertices[7] - u_cubeCenter, cubeX));
    float size = max(max(max(l0, l1), l2), l3);

    vec3 hX = size * cubeX;
    vec3 hY = size * cubeY;
    vec3 hZ = size * cubeZ;

    // store Transformed Slicing Domain coordinates
    TSDvertices[0] = u_cubeCenter - hX - hY - hZ;
    TSDvertices[1] = u_cubeCenter + hX - hY - hZ;
    TSDvertices[2] = u_cubeCenter - hX + hY - hZ;
    TSDvertices[3] = u_cubeCenter - hX - hY + hZ;
    TSDvertices[4] = u_cubeCenter + hX + hY - hZ;
    TSDvertices[5] = u_cubeCenter - hX + hY + hZ;
    TSDvertices[6] = u_cubeCenter + hX - hY + hZ;
    TSDvertices[7] = u_cubeCenter + hX + hY + hZ;
}

vec3 calcTexcoord(Polygon polygon, vec3 pos, vec3 dir, mat3 M, vec3 eAxis[3], float edgeLengths[3], out vec3 intersectPoint) {
    // calculate texture coordinates by evaluating intersection between reflected ray direction (variable : dir) and the polygon
    vec3 dirWorld = normalize(M * dir);

    vec3 q = polygon.coord[0];

    vec3 polygonN = polygon.normal;
    vec3 x0 = pos;

    float t = -dot(polygonN, x0 - q) / dot(polygonN, dirWorld);
    intersectPoint = x0 + t * dirWorld;

    vec3 posInCube = intersectPoint - u_marginCubeVertices[0];

    float u = dot(posInCube, eAxis[0]) / edgeLengths[0];
    float v = dot(posInCube, eAxis[1]) / edgeLengths[1];
    float w = dot(posInCube, eAxis[2]) / edgeLengths[2];

    return vec3(u, v, w);
}

float calcArea(int n, vec3 L[5]) {
    // calculate the area of polygonal light
    switch(n) {
    case 0:
    {
        return EPS;
    }

    case 3:
    {
        vec3 v1 = L[1] - L[0];
        vec3 v2 = L[2] - L[0];
        return 0.5 * length(cross(v1, v2));
    }

    case 4:
    {
        vec3 v1 = L[1] - L[0];
        vec3 v2 = L[2] - L[0];
        vec3 v3 = L[3] - L[0];

        float a1 = length(cross(v1, v2));
        float a2 = length(cross(v2, v3));

        return 0.5 * (a1 + a2);
    }

    case 5:
    {
        vec3 v1 = L[1] - L[0];
        vec3 v2 = L[2] - L[0];
        vec3 v3 = L[3] - L[0];
        vec3 v4 = L[4] - L[0];

        float a1 = length(cross(v1, v2));
        float a2 = length(cross(v2, v3));
        float a3 = length(cross(v3, v4));

        return 0.5 * (a1 + a2 + a3);
    }
    }
}

bool isZero(vec3 v) {
    return length(v) == 0.0;
}

vec3 gammaCorrection(vec3 color) {
    return pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));
}

void main(void) {
    vec3 out_rgb = vec3(0.0);

    vec3 pos = f_vertPosWorld;    
    vec3 V = normalize(u_cameraPos - pos);
    vec3 L = normalize(u_lightPos - pos);
    vec3 H = normalize(L + V);
    vec3 N = normalize(f_normalWorld);
    vec3 R = normalize(reflect(-V, N));

    float alpha = u_alpha;
    if(u_isAlphaTextured) { 
        alpha = pow(texture(u_alphaTex, vec2(f_texcoord.x, 1.0 - f_texcoord.y)).x, 2.2);
    }
    
    // ----------
    // Point light shading (GGX-based microfacet BRDF)
    // ----------
    float NdotL = max(EPS, dot(N, L));
    float dist2L = length(u_lightPos - pos);

    vec3 F = FresnelConductor(L, N, u_eta, u_kappa);
    float G = SmithMasking(lambda(N, V, alpha), lambda(N, L, alpha));
    float D = GGX(H, N, alpha);
    vec3 microBRDF = (F * G * D) / (4.0 * dot(V, N));

    vec3 Re = u_diffColor * NdotL * INV_PI + F * microBRDF;
    vec3 factor = u_lightLe / (dist2L * dist2L);
    out_rgb += factor * Re;

    // ----------
    // Volume indirect illumination    
    // ----------
    vec3 distDir = normalize(u_cubeCenter - pos);
    
    // Diffuse indirect illumination
    vec3 diffIndirect = vec3(0.0);
    if (!isZero(u_diffColor)) {
        diffIndirect = u_diffColor * evaluateDiffBySampling(pos, N);
    }

    // Specular indirect illumination
    vec3 specIndirect = vec3(0.0);
    if (!isZero(u_eta)) {
        // Calculate texture coordinates for LTC-based area integration
        float theta = acos(dot(N, V));
        vec2 uv = vec2(alpha, theta  * INV_HALF_PI);
        uv = uv * LUT_SCALE + vec2(LUT_BIAS);
    
        vec4 t = texture(u_ltcMatTex, uv);
        mat3 invM = mat3(
            vec3(1.0, 0.0, t.y),
            vec3(0.0, t.z, 0.0),
            vec3(t.w, 0.0, t.x)
        );
    
        // Calculate transformation matrix for the shading position
        mat3 toCC;
        calcTransformMats(N, V, invM, toCC);
    
        // Volume domain transformation (TSD = transformed slicing domain)
        // See Sec. 3.5 of our paper.
        vec3 cornersTSD[8];
        createTSD(-distDir, cornersTSD);
        
        //*****compute axes and their lengths for following calculation*****//
        vec3 eAxis[3];
        eAxis[0] = vec3(normalize(u_marginCubeVertices[1] - u_marginCubeVertices[0]));
        eAxis[1] = vec3(normalize(u_marginCubeVertices[2] - u_marginCubeVertices[0]));
        eAxis[2] = vec3(normalize(u_marginCubeVertices[3] - u_marginCubeVertices[0]));

        float edgeLengths[3];
        edgeLengths[0] = length(u_marginCubeVertices[1] - u_marginCubeVertices[0]);
        edgeLengths[1] = length(u_marginCubeVertices[2] - u_marginCubeVertices[0]);
        edgeLengths[2] = length(u_marginCubeVertices[3] - u_marginCubeVertices[0]);
        //**********//

        Polygon polygon;
        initPolygon(polygon, distDir);
    
        polygon.coord[0] = cornersTSD[7];
        polygon.coord[1] = cornersTSD[6];
        polygon.coord[2] = cornersTSD[3];
        polygon.coord[3] = cornersTSD[5];
    
        vec3 prevSpecPolyRad = vec3(0.0);
        vec3 prevSigmaT = vec3(0.0);
        vec3 prevAveSigmaT = vec3(0.0);
        vec3 extFactor = vec3(1.0);
        vec3 polyFresnel = FresnelConductor(R, N, u_eta, u_kappa);

        vec3 texcoord;
        vec3 intersectPoint;
        texcoord = calcTexcoord(polygon, pos, R, mat3(1.0), eAxis, edgeLengths, intersectPoint);
    
        // Variables for uniform slicing
        vec3 fixedStride = (cornersTSD[0] - cornersTSD[3]) / float(u_sectionNum + 1);
        float fixedStrideLength = length(fixedStride);
    
        // Vector from "intersectpoint on current LPAL" to "intersect point on next LPAL"   
        // this vector describes the difference on intersect point in world space (***)
        float RprojStride = dot(R, normalize(fixedStride));
        vec3 diff_intersectpoint = (fixedStrideLength / (RprojStride + EPS)) * R;
    
        float du = dot(diff_intersectpoint, eAxis[0]) / edgeLengths[0];
        float dv = dot(diff_intersectpoint, eAxis[1]) / edgeLengths[1];
        float dw = dot(diff_intersectpoint, eAxis[2]) / edgeLengths[2];
        vec3 diff_texcoord = vec3(du, dv, dw); // project (***) to uv space and obtain the difference vector in texture space
    
        // loops for volume integration, update polygon in each loop
        if (dot(distDir, R) > EPS) { // skip if the volume is located opposite of BRDF direction
            for (int sectionIndex = 0; sectionIndex < u_sectionNum; sectionIndex++) {
                // slicing updates
                intersectPoint += diff_intersectpoint;
                texcoord += diff_texcoord;
                polygon.coord[0] = cornersTSD[7] + sectionIndex * fixedStride; // update LPAL vertex No.1
                polygon.coord[1] = cornersTSD[6] + sectionIndex * fixedStride; // update LPAL vertex No.2
                polygon.coord[2] = cornersTSD[3] + sectionIndex * fixedStride; // update LPAL vertex No.3
                polygon.coord[3] = cornersTSD[5] + sectionIndex * fixedStride; // update LPAL vertex No.4
    
                // GL_CLAMP_TO_BORDER, continue operation for out-of-space uv coordinates
                if (any(lessThan(texcoord, vec3(0.0))) ||
                    any(greaterThan(texcoord, vec3(1.0)))) {
                    continue;
                }
    
                // LPAL integration using LTC
                vec3 nonClippedL[4];
                vec3 L[5];
                int n;
                vec3 totF;
                mat3 M;
    
                calcLvector(toCC, pos, polygon, nonClippedL); // calculate area light coordinates in clamped-cosine space
                clipQuadToHorizon(n, nonClippedL, L); // clipping area light
    
                // texture fetching, tuning LOD using certain values
                float pr = length(intersectPoint - pos) + 1.0;
                float A = calcArea(n, L);
                float sig =  sqrt(sqrt((pr * pr * pr / (2.0 * A))));
                float ca = (pow(2.0, u_maxLOD - 2.6) - 1.0) * alpha;
                float LOD = log2(ca + 1.0);
                LOD *= sig;
    
                vec3 s = INV_TWO_PI * evaluateLTCspec(L, n, false, totF);  // s is the result of integration, which is in the range of [0,1]
                float mag = texture(u_ltcMagTex, uv).x;
                s *= mag;
                s = clamp(s, vec3(0.0), vec3(mag));
    
                vec3 polyColor = textureLod(u_filteredTex, texcoord, LOD).xyz; // color of LPAL
    
                float density = textureLod(u_filteredTex, texcoord, LOD).w * s.x;
                vec3 sigmaS = u_albedo * density;
                vec3 sigmaA = density - sigmaS;
                vec3 sigmaT = sigmaS + sigmaA;
                vec3 aveSigmaT = 0.5 * (prevSigmaT + sigmaT);
    
                // Skip empty volume slice
                if (all(lessThan(polyColor, vec3(EPS))) && all(lessThan(prevSpecPolyRad, vec3(EPS)))) {
                    extFactor *= exp(-prevAveSigmaT * fixedStrideLength);    
                    prevSpecPolyRad = vec3(0.0);
                    prevSigmaT = sigmaT;
                    prevAveSigmaT = aveSigmaT;    
                    continue;
                }
    
                // Contribution from LPAL located in the bak of the slice
                vec3 specPolyRad = s * polyColor * polyFresnel;
    
                // Integration for frustum
                // See Eq.(8) of our paper.
                vec3 index = aveSigmaT * fixedStrideLength;
                vec3 expMinusIndex = exp(-index);
                vec3 expIndex = exp(index);
                vec3 factSquared = index * index;
                vec3 denom = 1.0 / max(factSquared, vec3(EPS));
    
                vec3 I1 = (index + expMinusIndex - 1.0) * denom;
                vec3 I2 = expMinusIndex * (- index - 1.0 + expIndex) * denom;
    
                vec3 specVolRad = (prevSpecPolyRad * I1 + specPolyRad * I2);
                specVolRad *= fixedStrideLength;
    
                // Accumulate light attenuation and contributions from slice
                extFactor *= exp(-prevAveSigmaT * fixedStrideLength);
                specIndirect += specVolRad * extFactor; 
    
                // Store parameters of current slice for the next slice
                prevSpecPolyRad = specPolyRad;
                prevSigmaT = sigmaT;
                prevAveSigmaT = aveSigmaT;
            }
        }
    }    

    // Gamma correction
    out_rgb += diffIndirect + specIndirect;
    out_color = vec4(gammaCorrection(out_rgb), 1.0);
}
