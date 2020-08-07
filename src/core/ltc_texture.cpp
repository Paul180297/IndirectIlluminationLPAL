#include "ltc_texture.h"
#include "ltc.inc"

GLuint createLTCmatTex() {

	GLuint ltcMatTexId;
	glGenTextures(1, &ltcMatTexId);

	GLenum target = GL_TEXTURE_2D;
	GLenum filter = GL_LINEAR;
	GLenum address = GL_CLAMP_TO_EDGE;

	glBindTexture(target, ltcMatTexId);

	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);

	glTexParameteri(target, GL_TEXTURE_WRAP_S, address);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, address);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	float data[size][size][4];

	for (int v = 0; v < size; ++v)
	{
		for (int u = 0; u < size; ++u)
		{
			float a = tabM[u*size + v % size][0];
			float b = tabM[u*size + v % size][2];
			float c = tabM[u*size + v % size][4];
			float d = tabM[u*size + v % size][6];

			data[u][v][0] = a;
			data[u][v][1] = -b;
			data[u][v][2] = (a - b * d) / c;
			data[u][v][3] = -d;
		}
	}

	// upload
	glTexImage2D(target, 0, GL_RGBA32F, size, size, 0, GL_RGBA, GL_FLOAT, data);

	glBindTexture(target, 0);

	return ltcMatTexId;
}

GLuint createLTCmagTex() {

	GLuint ltcMagTexId;
	glGenTextures(1, &ltcMagTexId);

	GLenum target = GL_TEXTURE_2D;
	GLenum filter = GL_LINEAR;
	GLenum address = GL_CLAMP_TO_EDGE;

	glBindTexture(target, ltcMagTexId);

	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);

	glTexParameteri(target, GL_TEXTURE_WRAP_S, address);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, address);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	float data[size*size];

	for (int u = 0; u < size; ++u)
	{
		for (int v = 0; v < size; ++v)
		{
			float a = tabAmplitude[u*size + v % size];

			data[u*size + v % size] = a;
		}
	}

	// upload
	glTexImage2D(target, 0, GL_R32F, size, size, 0, GL_RED, GL_FLOAT, data);

	glBindTexture(target, 0);

	return ltcMagTexId;
}