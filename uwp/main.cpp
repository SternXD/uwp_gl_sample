#include <Windows.h>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <map>
#include <cmath>

#include "glad/glad.h"
#include "SDL2/SDL.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// Glyph information
struct Glyph {
	float x0, y0, x1, y1;  // Texture coordinates
	float xoff, yoff;      // Offset from baseline
	float advance;         // Advance width
	int width, height;     // Glyph dimensions
};

// Simple text rendering using OpenGL
struct TextRenderer {
	GLuint vao, vbo, ebo, program, fontTexture;
	int windowWidth, windowHeight;
	int fontTextureWidth, fontTextureHeight;
	float fontSize;
	std::map<unsigned char, Glyph> glyphs;
	float fontScale;
	
	TextRenderer() : vao(0), vbo(0), ebo(0), program(0), fontTexture(0), 
					 windowWidth(0), windowHeight(0), fontTextureWidth(0), 
					 fontTextureHeight(0), fontSize(32.0f), fontScale(1.0f) {}
};

struct CubeRenderer {
	GLuint vao, vbo, ebo, program;
	CubeRenderer() : vao(0), vbo(0), ebo(0), program(0) {}
};

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform vec2 screenSize;

void main() {
	// Convert to normalized device coordinates
	vec2 pos = (aPos / screenSize) * 2.0 - 1.0;
	pos.y = -pos.y; // Flip Y axis
	gl_Position = vec4(pos, 0.0, 1.0);
	TexCoord = aTexCoord;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D fontTexture;
uniform vec3 textColor;

void main() {
	float alpha = texture(fontTexture, TexCoord).r;
	FragColor = vec4(textColor, alpha);
}
)";

const char* cubeVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 uMVP;

out vec3 vColor;

void main() {
	vColor = aColor;
	gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

const char* cubeFragmentShaderSource = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
	FragColor = vec4(vColor, 1.0);
}
)";

void setPixel(unsigned char* fontData, int textureWidth, int startX, int startY, int x, int y) {
	int pixelX = startX + x;
	int pixelY = startY + y;
	int idx = pixelY * textureWidth + pixelX;
	fontData[idx] = 255;
}

void drawChar(unsigned char* fontData, int textureWidth, int startX, int startY, unsigned char c) {
	
	switch (c) {
		case ' ': break;
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			if (c == '0') {
				for (int y = 1; y < 7; y++) {
					setPixel(fontData, textureWidth, startX, startY, 1, y);
					setPixel(fontData, textureWidth, startX, startY, 6, y);
				}
				for (int x = 1; x < 7; x++) {
					setPixel(fontData, textureWidth, startX, startY, x, 1);
					setPixel(fontData, textureWidth, startX, startY, x, 6);
				}
			} else if (c == '1') {
				for (int y = 1; y < 7; y++) setPixel(fontData, textureWidth, startX, startY, 3, y);
				setPixel(fontData, textureWidth, startX, startY, 2, 1);
				setPixel(fontData, textureWidth, startX, startY, 2, 6);
			} else if (c == '2') {
				for (int x = 1; x < 7; x++) {
					setPixel(fontData, textureWidth, startX, startY, x, 1);
					setPixel(fontData, textureWidth, startX, startY, x, 4);
					setPixel(fontData, textureWidth, startX, startY, x, 6);
				}
				setPixel(fontData, textureWidth, startX, startY, 6, 2);
				setPixel(fontData, textureWidth, startX, startY, 6, 3);
				setPixel(fontData, textureWidth, startX, startY, 1, 4);
				setPixel(fontData, textureWidth, startX, startY, 1, 5);
			} else if (c == '3') {
				for (int x = 1; x < 7; x++) {
					setPixel(fontData, textureWidth, startX, startY, x, 1);
					setPixel(fontData, textureWidth, startX, startY, x, 4);
					setPixel(fontData, textureWidth, startX, startY, x, 6);
				}
				for (int y = 2; y < 4; y++) setPixel(fontData, textureWidth, startX, startY, 6, y);
				for (int y = 4; y < 6; y++) setPixel(fontData, textureWidth, startX, startY, 6, y);
			} else if (c == '4') {
				for (int y = 1; y < 5; y++) setPixel(fontData, textureWidth, startX, startY, 1, y);
				for (int x = 1; x < 7; x++) setPixel(fontData, textureWidth, startX, startY, x, 4);
				for (int y = 1; y < 7; y++) setPixel(fontData, textureWidth, startX, startY, 6, y);
			} else if (c == '5') {
				for (int x = 1; x < 7; x++) {
					setPixel(fontData, textureWidth, startX, startY, x, 1);
					setPixel(fontData, textureWidth, startX, startY, x, 3);
					setPixel(fontData, textureWidth, startX, startY, x, 6);
				}
				setPixel(fontData, textureWidth, startX, startY, 1, 2);
				for (int y = 4; y < 6; y++) setPixel(fontData, textureWidth, startX, startY, 6, y);
			} else if (c == '6') {
				for (int y = 1; y < 7; y++) setPixel(fontData, textureWidth, startX, startY, 1, y);
				for (int x = 1; x < 7; x++) {
					setPixel(fontData, textureWidth, startX, startY, x, 1);
					setPixel(fontData, textureWidth, startX, startY, x, 4);
					setPixel(fontData, textureWidth, startX, startY, x, 6);
				}
				for (int y = 4; y < 6; y++) setPixel(fontData, textureWidth, startX, startY, 6, y);
			} else if (c == '7') {
				for (int x = 1; x < 7; x++) setPixel(fontData, textureWidth, startX, startY, x, 1);
				for (int y = 2; y < 7; y++) setPixel(fontData, textureWidth, startX, startY, 6, y);
			} else if (c == '8') {
				for (int y = 1; y < 7; y++) {
					setPixel(fontData, textureWidth, startX, startY, 1, y);
					setPixel(fontData, textureWidth, startX, startY, 6, y);
				}
				for (int x = 1; x < 7; x++) {
					setPixel(fontData, textureWidth, startX, startY, x, 1);
					setPixel(fontData, textureWidth, startX, startY, x, 4);
					setPixel(fontData, textureWidth, startX, startY, x, 6);
				}
			} else if (c == '9') {
				for (int y = 1; y < 5; y++) setPixel(fontData, textureWidth, startX, startY, 6, y);
				for (int x = 1; x < 7; x++) {
					setPixel(fontData, textureWidth, startX, startY, x, 1);
					setPixel(fontData, textureWidth, startX, startY, x, 4);
					setPixel(fontData, textureWidth, startX, startY, x, 6);
				}
				for (int y = 4; y < 6; y++) setPixel(fontData, textureWidth, startX, startY, 1, y);
			}
			break;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
		case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
			if (c >= 'A' && c <= 'Z') {
				if (c == 'A') {
					for (int y = 2; y < 7; y++) {
						setPixel(fontData, textureWidth, startX, startY, 1, y);
						setPixel(fontData, textureWidth, startX, startY, 6, y);
					}
					for (int x = 1; x < 7; x++) setPixel(fontData, textureWidth, startX, startY, x, 1);
					for (int x = 2; x < 6; x++) setPixel(fontData, textureWidth, startX, startY, x, 4);
				} else if (c == 'B') {
					for (int y = 1; y < 7; y++) setPixel(fontData, textureWidth, startX, startY, 1, y);
					for (int x = 1; x < 6; x++) {
						setPixel(fontData, textureWidth, startX, startY, x, 1);
						setPixel(fontData, textureWidth, startX, startY, x, 4);
						setPixel(fontData, textureWidth, startX, startY, x, 6);
					}
					setPixel(fontData, textureWidth, startX, startY, 6, 2);
					setPixel(fontData, textureWidth, startX, startY, 6, 3);
					setPixel(fontData, textureWidth, startX, startY, 6, 5);
				} else if (c == 'C') {
					for (int y = 2; y < 6; y++) setPixel(fontData, textureWidth, startX, startY, 1, y);
					for (int x = 2; x < 6; x++) {
						setPixel(fontData, textureWidth, startX, startY, x, 1);
						setPixel(fontData, textureWidth, startX, startY, x, 6);
					}
				} else if (c == 'D') {
					for (int y = 1; y < 7; y++) setPixel(fontData, textureWidth, startX, startY, 1, y);
					for (int y = 2; y < 6; y++) setPixel(fontData, textureWidth, startX, startY, 6, y);
					setPixel(fontData, textureWidth, startX, startY, 2, 1);
					setPixel(fontData, textureWidth, startX, startY, 3, 1);
					setPixel(fontData, textureWidth, startX, startY, 4, 1);
					setPixel(fontData, textureWidth, startX, startY, 5, 1);
					setPixel(fontData, textureWidth, startX, startY, 2, 6);
					setPixel(fontData, textureWidth, startX, startY, 3, 6);
					setPixel(fontData, textureWidth, startX, startY, 4, 6);
					setPixel(fontData, textureWidth, startX, startY, 5, 6);
				} else if (c == 'E') {
					for (int y = 1; y < 7; y++) setPixel(fontData, textureWidth, startX, startY, 1, y);
					for (int x = 1; x < 7; x++) {
						setPixel(fontData, textureWidth, startX, startY, x, 1);
						setPixel(fontData, textureWidth, startX, startY, x, 4);
						setPixel(fontData, textureWidth, startX, startY, x, 6);
					}
				} else if (c == 'G') {
					for (int y = 2; y < 6; y++) setPixel(fontData, textureWidth, startX, startY, 1, y);
					for (int x = 2; x < 6; x++) {
						setPixel(fontData, textureWidth, startX, startY, x, 1);
						setPixel(fontData, textureWidth, startX, startY, x, 6);
					}
					setPixel(fontData, textureWidth, startX, startY, 6, 4);
					setPixel(fontData, textureWidth, startX, startY, 6, 5);
					setPixel(fontData, textureWidth, startX, startY, 5, 4);
				} else if (c == 'H') {
					for (int y = 1; y < 7; y++) {
						setPixel(fontData, textureWidth, startX, startY, 1, y);
						setPixel(fontData, textureWidth, startX, startY, 6, y);
					}
					for (int x = 1; x < 7; x++) setPixel(fontData, textureWidth, startX, startY, x, 4);
				} else if (c == 'I') {
					for (int y = 1; y < 7; y++) setPixel(fontData, textureWidth, startX, startY, 3, y);
					for (int x = 1; x < 7; x++) {
						setPixel(fontData, textureWidth, startX, startY, x, 1);
						setPixel(fontData, textureWidth, startX, startY, x, 6);
					}
				} else if (c == 'L') {
					for (int y = 1; y < 7; y++) setPixel(fontData, textureWidth, startX, startY, 1, y);
					for (int x = 1; x < 7; x++) setPixel(fontData, textureWidth, startX, startY, x, 6);
				} else if (c == 'M') {
					for (int y = 1; y < 7; y++) {
						setPixel(fontData, textureWidth, startX, startY, 1, y);
						setPixel(fontData, textureWidth, startX, startY, 6, y);
					}
					setPixel(fontData, textureWidth, startX, startY, 2, 2);
					setPixel(fontData, textureWidth, startX, startY, 3, 3);
					setPixel(fontData, textureWidth, startX, startY, 4, 3);
					setPixel(fontData, textureWidth, startX, startY, 5, 2);
				} else if (c == 'N') {
					for (int y = 1; y < 7; y++) {
						setPixel(fontData, textureWidth, startX, startY, 1, y);
						setPixel(fontData, textureWidth, startX, startY, 6, y);
					}
					setPixel(fontData, textureWidth, startX, startY, 2, 2);
					setPixel(fontData, textureWidth, startX, startY, 3, 3);
					setPixel(fontData, textureWidth, startX, startY, 4, 4);
					setPixel(fontData, textureWidth, startX, startY, 5, 5);
				} else if (c == 'O') {
					for (int y = 2; y < 6; y++) {
						setPixel(fontData, textureWidth, startX, startY, 1, y);
						setPixel(fontData, textureWidth, startX, startY, 6, y);
					}
					for (int x = 2; x < 6; x++) {
						setPixel(fontData, textureWidth, startX, startY, x, 1);
						setPixel(fontData, textureWidth, startX, startY, x, 6);
					}
				} else if (c == 'P') {
					for (int y = 1; y < 7; y++) setPixel(fontData, textureWidth, startX, startY, 1, y);
					for (int x = 1; x < 6; x++) {
						setPixel(fontData, textureWidth, startX, startY, x, 1);
						setPixel(fontData, textureWidth, startX, startY, x, 4);
					}
					setPixel(fontData, textureWidth, startX, startY, 6, 2);
					setPixel(fontData, textureWidth, startX, startY, 6, 3);
				} else if (c == 'R') {
					for (int y = 1; y < 7; y++) setPixel(fontData, textureWidth, startX, startY, 1, y);
					for (int x = 1; x < 6; x++) {
						setPixel(fontData, textureWidth, startX, startY, x, 1);
						setPixel(fontData, textureWidth, startX, startY, x, 4);
					}
					setPixel(fontData, textureWidth, startX, startY, 6, 2);
					setPixel(fontData, textureWidth, startX, startY, 6, 3);
					setPixel(fontData, textureWidth, startX, startY, 5, 5);
					setPixel(fontData, textureWidth, startX, startY, 6, 6);
				} else if (c == 'S') {
					for (int x = 1; x < 7; x++) {
						setPixel(fontData, textureWidth, startX, startY, x, 1);
						setPixel(fontData, textureWidth, startX, startY, x, 4);
						setPixel(fontData, textureWidth, startX, startY, x, 6);
					}
					setPixel(fontData, textureWidth, startX, startY, 1, 2);
					setPixel(fontData, textureWidth, startX, startY, 1, 3);
					setPixel(fontData, textureWidth, startX, startY, 6, 5);
				} else if (c == 'T') {
					for (int x = 1; x < 7; x++) setPixel(fontData, textureWidth, startX, startY, x, 1);
					for (int y = 1; y < 7; y++) setPixel(fontData, textureWidth, startX, startY, 3, y);
				} else if (c == 'U') {
					for (int y = 1; y < 6; y++) {
						setPixel(fontData, textureWidth, startX, startY, 1, y);
						setPixel(fontData, textureWidth, startX, startY, 6, y);
					}
					for (int x = 1; x < 7; x++) setPixel(fontData, textureWidth, startX, startY, x, 6);
				} else if (c == 'V') {
					for (int y = 1; y < 5; y++) {
						setPixel(fontData, textureWidth, startX, startY, 1, y);
						setPixel(fontData, textureWidth, startX, startY, 6, y);
					}
					setPixel(fontData, textureWidth, startX, startY, 2, 5);
					setPixel(fontData, textureWidth, startX, startY, 5, 5);
					setPixel(fontData, textureWidth, startX, startY, 3, 6);
					setPixel(fontData, textureWidth, startX, startY, 4, 6);
				} else if (c == 'X') {
					for (int i = 0; i < 6; i++) {
						setPixel(fontData, textureWidth, startX, startY, 1 + i, 1 + i);
						setPixel(fontData, textureWidth, startX, startY, 6 - i, 1 + i);
					}
				} else if (c == 'Y') {
					setPixel(fontData, textureWidth, startX, startY, 3, 1);
					setPixel(fontData, textureWidth, startX, startY, 4, 1);
					for (int y = 2; y < 5; y++) {
						setPixel(fontData, textureWidth, startX, startY, 2, y);
						setPixel(fontData, textureWidth, startX, startY, 5, y);
					}
					for (int y = 4; y < 7; y++) setPixel(fontData, textureWidth, startX, startY, 3, y);
					setPixel(fontData, textureWidth, startX, startY, 4, 4);
				} else {
					for (int y = 1; y < 7; y++) {
						setPixel(fontData, textureWidth, startX, startY, 1, y);
						setPixel(fontData, textureWidth, startX, startY, 6, y);
					}
					for (int x = 1; x < 7; x++) {
						setPixel(fontData, textureWidth, startX, startY, x, 1);
						setPixel(fontData, textureWidth, startX, startY, x, 6);
					}
				}
			}
			break;
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
		case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
		case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
			drawChar(fontData, textureWidth, startX, startY, c - 32);
			break;
		case ':':
			setPixel(fontData, textureWidth, startX, startY, 3, 2);
			setPixel(fontData, textureWidth, startX, startY, 3, 5);
			break;
		case '.':
			setPixel(fontData, textureWidth, startX, startY, 3, 6);
			setPixel(fontData, textureWidth, startX, startY, 4, 6);
			break;
		case '-':
			for (int x = 2; x < 6; x++) setPixel(fontData, textureWidth, startX, startY, x, 3);
			break;
		case '=':
			for (int x = 1; x < 7; x++) {
				setPixel(fontData, textureWidth, startX, startY, x, 2);
				setPixel(fontData, textureWidth, startX, startY, x, 5);
			}
			break;
		case '/':
			for (int i = 0; i < 6; i++) {
				setPixel(fontData, textureWidth, startX, startY, 5 - i, 1 + i);
			}
			break;
		case '(':
			for (int y = 2; y < 6; y++) setPixel(fontData, textureWidth, startX, startY, 2, y);
			setPixel(fontData, textureWidth, startX, startY, 3, 1);
			setPixel(fontData, textureWidth, startX, startY, 3, 6);
			break;
		case ')':
			for (int y = 2; y < 6; y++) setPixel(fontData, textureWidth, startX, startY, 5, y);
			setPixel(fontData, textureWidth, startX, startY, 4, 1);
			setPixel(fontData, textureWidth, startX, startY, 4, 6);
			break;
		default:
			for (int y = 1; y < 7; y++) {
				setPixel(fontData, textureWidth, startX, startY, 1, y);
				setPixel(fontData, textureWidth, startX, startY, 6, y);
			}
			for (int x = 1; x < 7; x++) {
				setPixel(fontData, textureWidth, startX, startY, x, 1);
				setPixel(fontData, textureWidth, startX, startY, x, 6);
			}
			break;
	}
}

bool loadTTFFont(const char* fontPath, TextRenderer& renderer, float fontSize) {
	std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		return false;
	}
	
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	
	std::vector<unsigned char> fontBuffer(size);
	if (!file.read((char*)fontBuffer.data(), size)) {
		return false;
	}
	file.close();
	
	stbtt_fontinfo font;
	if (!stbtt_InitFont(&font, fontBuffer.data(), stbtt_GetFontOffsetForIndex(fontBuffer.data(), 0))) {
		return false;
	}
	
	float scale = stbtt_ScaleForPixelHeight(&font, fontSize);
	renderer.fontScale = scale;
	renderer.fontSize = fontSize;
	
	const int atlasWidth = 512;
	const int atlasHeight = 512;
	unsigned char* atlasData = new unsigned char[atlasWidth * atlasHeight];
	memset(atlasData, 0, atlasWidth * atlasHeight);
	
	int x = 0, y = 0;
	int lineHeight = (int)(fontSize * 1.2f);
	
	for (int c = 32; c < 127; c++) {
		int glyphIndex = stbtt_FindGlyphIndex(&font, c);
		
		int advance, lsb;
		stbtt_GetGlyphHMetrics(&font, glyphIndex, &advance, &lsb);
		
		int width, height, xoff, yoff;
		unsigned char* bitmap = stbtt_GetGlyphBitmap(&font, scale, scale, glyphIndex, &width, &height, &xoff, &yoff);
		
		if (bitmap) {
			if (x + width + 1 > atlasWidth) {
				x = 0;
				y += lineHeight;
				if (y + height > atlasHeight) {
					stbtt_FreeBitmap(bitmap, NULL);
					delete[] atlasData;
					return false;
				}
			}
			
			for (int gy = 0; gy < height; gy++) {
				for (int gx = 0; gx < width; gx++) {
					int atlasX = x + gx;
					int atlasY = y + gy;
					if (atlasX < atlasWidth && atlasY < atlasHeight) {
						int glyphIdx = gy * width + gx;
						int atlasIdx = atlasY * atlasWidth + atlasX;
						atlasData[atlasIdx] = bitmap[glyphIdx];
					}
				}
			}
			
			Glyph glyph;
			glyph.x0 = (float)x / atlasWidth;
			glyph.y0 = (float)y / atlasHeight;
			glyph.x1 = (float)(x + width) / atlasWidth;
			glyph.y1 = (float)(y + height) / atlasHeight;
			glyph.xoff = (float)xoff;
			glyph.yoff = (float)yoff;
			glyph.advance = (float)advance * scale;
			glyph.width = width;
			glyph.height = height;
			
			renderer.glyphs[(unsigned char)c] = glyph;
			
			x += width + 1;
		}
		
		if (bitmap) {
			stbtt_FreeBitmap(bitmap, NULL);
		}
	}
	
	// Create OpenGL texture
	glGenTextures(1, &renderer.fontTexture);
	glBindTexture(GL_TEXTURE_2D, renderer.fontTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, atlasData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	renderer.fontTextureWidth = atlasWidth;
	renderer.fontTextureHeight = atlasHeight;
	
	delete[] atlasData;
	return true;
}

// Create a simple bitmap font texture (this is only a fallback)
void createBitmapFontTexture(GLuint* texture) {
	const int charWidth = 8;
	const int charHeight = 8;
	const int charsPerRow = 16;
	const int numRows = 8;
	
	const int textureWidth = charsPerRow * charWidth;  // 128 pixels
	const int textureHeight = numRows * charHeight;    // 64 pixels
	const int bufferSize = textureWidth * textureHeight; // 8192 bytes
	unsigned char* fontData = new unsigned char[bufferSize];
	memset(fontData, 0, bufferSize);
	
	for (int c = 32; c < 127; c++) {
		int charIndex = c - 32;
		int row = charIndex / charsPerRow;
		int col = charIndex % charsPerRow;
		
		int startX = col * charWidth;
		int startY = row * charHeight;
		
		drawChar(fontData, textureWidth, startX, startY, (unsigned char)c);
	}
	
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, textureWidth, textureHeight, 0, GL_RED, GL_UNSIGNED_BYTE, fontData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	delete[] fontData;
}

void renderText(const std::string& text, float x, float y, float scale, TextRenderer& renderer) {
	if (text.empty()) return;
	
	glUseProgram(renderer.program);
	glBindVertexArray(renderer.vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderer.fontTexture);
	glUniform1i(glGetUniformLocation(renderer.program, "fontTexture"), 0);
	
	glUniform2f(glGetUniformLocation(renderer.program, "screenSize"), 
				(float)renderer.windowWidth, (float)renderer.windowHeight);
	glUniform3f(glGetUniformLocation(renderer.program, "textColor"), 1.0f, 1.0f, 1.0f);
	
	float currentX = x;
	
	// Use TTF glyphs if available
	if (!renderer.glyphs.empty()) {
		for (size_t i = 0; i < text.length(); i++) {
			unsigned char c = text[i];
			if (c < 32 || c >= 127) continue;
			
			auto it = renderer.glyphs.find(c);
			if (it == renderer.glyphs.end()) continue;
			
			const Glyph& glyph = it->second;
			
			float charX = currentX + glyph.xoff * scale;
			float charY = y + glyph.yoff * scale;
			float charWidth = glyph.width * scale;
			float charHeight = glyph.height * scale;
			
			float vertices[] = {
				charX, charY + charHeight, glyph.x0, glyph.y1,
				charX + charWidth, charY + charHeight, glyph.x1, glyph.y1,
				charX + charWidth, charY, glyph.x1, glyph.y0,
				charX, charY, glyph.x0, glyph.y0
			};
			
			unsigned int indices[] = {
				0, 1, 2,
				2, 3, 0
			};
			
			glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
			
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);
			
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
			glEnableVertexAttribArray(1);
			
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			
			currentX += glyph.advance * scale;
		}
	} else {
		// Fallback to bitmap font
		float charWidth = 8.0f * scale;
		float charHeight = 8.0f * scale;
		float charSpacing = 2.0f * scale;
		const int charsPerRow = 16;
		const int numRows = 8;
		
		for (size_t i = 0; i < text.length(); i++) {
			char c = text[i];
			if (c < 32 || c >= 127) continue;
			
			int charIndex = c - 32;
			int texRow = charIndex / charsPerRow;
			int texCol = charIndex % charsPerRow;
			
			float charX = currentX;
			float charY = y;
			
			float texLeft = (float)texCol / charsPerRow;
			float texRight = (float)(texCol + 1) / charsPerRow;
			float texTop = (float)texRow / numRows;
			float texBottom = (float)(texRow + 1) / numRows;
			
			float vertices[] = {
				charX, charY + charHeight, texLeft, texBottom,
				charX + charWidth, charY + charHeight, texRight, texBottom,
				charX + charWidth, charY, texRight, texTop,
				charX, charY, texLeft, texTop
			};
			
			unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };
			
			glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
			
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);
			
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
			glEnableVertexAttribArray(1);
			
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			
			currentX += charWidth + charSpacing;
		}
	}
}

float measureTextWidth(const std::string& text, float scale, const TextRenderer& renderer) {
	if (!renderer.glyphs.empty()) {
		float w = 0.0f;
		for (size_t i = 0; i < text.length(); i++) {
			unsigned char c = text[i];
			if (c < 32 || c >= 127) continue;
			auto it = renderer.glyphs.find(c);
			if (it != renderer.glyphs.end()) {
				w += it->second.advance * scale;
			}
		}
		return w;
	} else {
		float charWidth = 8.0f * scale;
		float charSpacing = 2.0f * scale;
		return (charWidth + charSpacing) * (float)text.size();
	}
}

bool initTextRenderer(TextRenderer& renderer, int width, int height) {
	renderer.windowWidth = width;
	renderer.windowHeight = height;
	
	// Compile vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	
	// Check compilation
	GLint success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		printf("Vertex shader compilation failed: %s\n", infoLog);
	}
	
	// Compile fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		printf("Fragment shader compilation failed: %s\n", infoLog);
	}
	
	renderer.program = glCreateProgram();
	glAttachShader(renderer.program, vertexShader);
	glAttachShader(renderer.program, fragmentShader);
	glLinkProgram(renderer.program);
	
	glGetProgramiv(renderer.program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(renderer.program, 512, NULL, infoLog);
		printf("Program linking failed: %s\n", infoLog);
	}
	
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	
	// Create VAO, VBO, EBO
	glGenVertexArrays(1, &renderer.vao);
	glGenBuffers(1, &renderer.vbo);
	glGenBuffers(1, &renderer.ebo);
	
	const char* fontPaths[] = {
		"fonts/RobotoMono-Medium.ttf",
		"uwp/fonts/RobotoMono-Medium.ttf",
		"RobotoMono-Medium.ttf"
	};
	
	bool fontLoaded = false;
	for (int i = 0; i < 3; i++) {
		std::ifstream testFile(fontPaths[i], std::ios::binary);
		if (testFile.good()) {
			testFile.close();
			if (loadTTFFont(fontPaths[i], renderer, 32.0f)) {
				fontLoaded = true;
				break;
			}
		}
	}
	
	if (!fontLoaded) {
		createBitmapFontTexture(&renderer.fontTexture);
	}
	
	return true;
}

bool initCubeRenderer(CubeRenderer& renderer) {
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &cubeVertexShaderSource, NULL);
	glCompileShader(vs);

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &cubeFragmentShaderSource, NULL);
	glCompileShader(fs);

	renderer.program = glCreateProgram();
	glAttachShader(renderer.program, vs);
	glAttachShader(renderer.program, fs);
	glLinkProgram(renderer.program);

	glDeleteShader(vs);
	glDeleteShader(fs);

	// 36 vertices (position xyz, color rgb) for 12 triangles
	const float v[] = {
		// Front (red)
		-1, -1,  1, 1,0,0,   1, -1,  1, 1,0,0,   1,  1,  1, 1,0,0,
		-1, -1,  1, 1,0,0,   1,  1,  1, 1,0,0,  -1,  1,  1, 1,0,0,
		// Back (green)
		-1, -1, -1, 0,1,0,  -1,  1, -1, 0,1,0,   1,  1, -1, 0,1,0,
		-1, -1, -1, 0,1,0,   1,  1, -1, 0,1,0,   1, -1, -1, 0,1,0,
		// Left (blue)
		-1, -1, -1, 0,0,1,  -1, -1,  1, 0,0,1,  -1,  1,  1, 0,0,1,
		-1, -1, -1, 0,0,1,  -1,  1,  1, 0,0,1,  -1,  1, -1, 0,0,1,
		// Right (yellow)
		 1, -1, -1, 1,1,0,   1,  1, -1, 1,1,0,   1,  1,  1, 1,1,0,
		 1, -1, -1, 1,1,0,   1,  1,  1, 1,1,0,   1, -1,  1, 1,1,0,
		// Top (cyan)
		-1,  1, -1, 0,1,1,  -1,  1,  1, 0,1,1,   1,  1,  1, 0,1,1,
		-1,  1, -1, 0,1,1,   1,  1,  1, 0,1,1,   1,  1, -1, 0,1,1,
		// Bottom (magenta)
		-1, -1, -1, 1,0,1,   1, -1, -1, 1,0,1,   1, -1,  1, 1,0,1,
		-1, -1, -1, 1,0,1,   1, -1,  1, 1,0,1,  -1, -1,  1, 1,0,1
	};

	glGenVertexArrays(1, &renderer.vao);
	glGenBuffers(1, &renderer.vbo);

	glBindVertexArray(renderer.vao);
	glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	return true;
}

static void mat4Identity(float m[16]) {
	memset(m, 0, sizeof(float) * 16);
	m[0] = m[5] = m[10] = m[15] = 1.0f;
}
static void mat4Multiply(float out[16], const float a[16], const float b[16]) {
	float r[16];
	for (int c = 0; c < 4; ++c) {
		for (int rIdx = 0; rIdx < 4; ++rIdx) {
			r[c*4 + rIdx] = a[0*4 + rIdx]*b[c*4 + 0] + a[1*4 + rIdx]*b[c*4 + 1] + a[2*4 + rIdx]*b[c*4 + 2] + a[3*4 + rIdx]*b[c*4 + 3];
		}
	}
	memcpy(out, r, sizeof(r));
}
static void mat4Translate(float m[16], float x, float y, float z) {
	mat4Identity(m);
	m[12] = x; m[13] = y; m[14] = z;
}
static void mat4RotateY(float m[16], float angle) {
	mat4Identity(m);
	float c = cosf(angle), s = sinf(angle);
	m[0] = c; m[2] = s; m[8] = -s; m[10] = c;
}
static void mat4RotateX(float m[16], float angle) {
	mat4Identity(m);
	float c = cosf(angle), s = sinf(angle);
	m[5] = c; m[6] = s; m[9] = -s; m[10] = c;
}
static void mat4Perspective(float m[16], float fovyRadians, float aspect, float znear, float zfar) {
	float f = 1.0f / tanf(fovyRadians * 0.5f);
	memset(m, 0, sizeof(float) * 16);
	m[0] = f / aspect;
	m[5] = f;
	m[10] = (zfar + znear) / (znear - zfar);
	m[11] = -1.0f;
	m[14] = (2.0f * zfar * znear) / (znear - zfar);
}

// Get system information
std::vector<std::string> getSystemInfo() {
	std::vector<std::string> info;
	
	SDL_version sdlVersion;
	SDL_GetVersion(&sdlVersion);
	info.push_back("SDL Version: " + std::to_string(sdlVersion.major) + "." + 
				   std::to_string(sdlVersion.minor) + "." + std::to_string(sdlVersion.patch));
	
	int cpuCores = SDL_GetCPUCount();
	if (cpuCores <= 1) {
		SYSTEM_INFO sysInfo = {};
		GetSystemInfo(&sysInfo);
		if ((int)sysInfo.dwNumberOfProcessors > cpuCores) {
			cpuCores = (int)sysInfo.dwNumberOfProcessors;
		}
	}
	info.push_back("CPU Cores: " + std::to_string(cpuCores));
	info.push_back(std::string("SIMD: ") +
		(SDL_HasRDTSC() ? "RDTSC " : "") +
		(SDL_HasAltiVec() ? "AltiVec " : "") +
		(SDL_HasMMX() ? "MMX " : "") +
		(SDL_Has3DNow() ? "3DNow " : "") +
		(SDL_HasSSE() ? "SSE " : "") +
		(SDL_HasSSE2() ? "SSE2 " : "") +
		(SDL_HasSSE3() ? "SSE3 " : "") +
		(SDL_HasSSE41() ? "SSE4.1 " : "") +
		(SDL_HasSSE42() ? "SSE4.2 " : "") +
		(SDL_HasAVX() ? "AVX " : "") +
		(SDL_HasAVX2() ? "AVX2 " : "") +
		(SDL_HasNEON() ? "NEON " : "")); // more then likely will never be shown because we mostly target Xbox hardware
	info.push_back("CPU CacheLine: " + std::to_string(SDL_GetCPUCacheLineSize()) + " bytes");
	
	int sdlRamMb = SDL_GetSystemRAM();
	long long ramMb = sdlRamMb;
	if (ramMb <= 0) {
		MEMORYSTATUSEX memStatus = {};
		memStatus.dwLength = sizeof(memStatus);
		if (GlobalMemoryStatusEx(&memStatus)) {
			ramMb = static_cast<long long>(memStatus.ullTotalPhys / (1024ull * 1024ull));
		}
	}
	info.push_back("System RAM: " + std::to_string(ramMb) + " MB");
	{
		int secs = 0, pct = 0;
		SDL_PowerState pwr = SDL_GetPowerInfo(&secs, &pct);
		if (pwr != SDL_POWERSTATE_UNKNOWN) {
			std::string pwrStr = "Power: ";
			switch (pwr) {
				case SDL_POWERSTATE_ON_BATTERY: pwrStr += "OnBattery"; break;
				case SDL_POWERSTATE_NO_BATTERY: pwrStr += "NoBattery"; break;
				case SDL_POWERSTATE_CHARGING: pwrStr += "Charging"; break;
				case SDL_POWERSTATE_CHARGED: pwrStr += "Charged"; break;
				default: pwrStr += "Unknown"; break;
			}
			if (pct >= 0) pwrStr += ", Battery: " + std::to_string(pct) + "%";
			if (secs > 0) pwrStr += ", TimeLeft: " + std::to_string(secs) + "s";
			info.push_back(pwrStr);
		}
	}
	
	info.push_back("Platform: " + std::string(SDL_GetPlatform()));
	
	const char* videoDriver = SDL_GetCurrentVideoDriver();
	if (videoDriver) {
		info.push_back("Video Driver: " + std::string(videoDriver));
	}
	
	int displayCount = SDL_GetNumVideoDisplays();
	info.push_back("Display Count: " + std::to_string(displayCount));
	
	for (int i = 0; i < displayCount && i < 6; i++) {
		SDL_DisplayMode mode;
		if (SDL_GetDesktopDisplayMode(i, &mode) == 0) {
			std::ostringstream oss;
			const char* name = SDL_GetDisplayName(i);
			if (name) {
				oss << "Display " << i << " (" << name << ")";
			} else {
				oss << "Display " << i;
			}
			oss << ": " << mode.w << "x" << mode.h;
			if (mode.refresh_rate > 0) {
				oss << " @ " << mode.refresh_rate << "Hz";
			}
			info.push_back(oss.str());
			float ddpi=0, hdpi=0, vdpi=0;
			if (SDL_GetDisplayDPI(i, &ddpi, &hdpi, &vdpi) == 0) {
				std::ostringstream dpi;
				dpi << "  DPI: d=" << (int)ddpi << " h=" << (int)hdpi << " v=" << (int)vdpi;
				info.push_back(dpi.str());
			}
			SDL_Rect bounds{0};
			if (SDL_GetDisplayBounds(i, &bounds) == 0) {
				std::ostringstream b;
				b << "  Bounds: (" << bounds.x << "," << bounds.y << ") " << bounds.w << "x" << bounds.h;
				info.push_back(b.str());
			}
		}
	}
	
	return info;
}

// Get OpenGL information
std::vector<std::string> getOpenGLInfo() {
	std::vector<std::string> info;
	
	const char* glVersion = (const char*)glGetString(GL_VERSION);
	if (glVersion) {
		info.push_back("OpenGL Version: " + std::string(glVersion));
	}
	
	const char* glRenderer = (const char*)glGetString(GL_RENDERER);
	if (glRenderer) {
		info.push_back("OpenGL Renderer: " + std::string(glRenderer));
	}
	
	const char* glVendor = (const char*)glGetString(GL_VENDOR);
	if (glVendor) {
		info.push_back("OpenGL Vendor: " + std::string(glVendor));
	}
	
	const char* glslVersion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
	if (glslVersion) {
		info.push_back("GLSL Version: " + std::string(glslVersion));
	}
	
	GLint majorVersion = 0, minorVersion = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	std::ostringstream oss;
	oss << "OpenGL Context: " << majorVersion << "." << minorVersion;
	info.push_back(oss.str());
	
	GLint profileMask = 0, contextFlags = 0;
	glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profileMask);
	glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);
	{
		std::string profile = "Profile: ";
		if (profileMask & GL_CONTEXT_CORE_PROFILE_BIT) profile += "Core"; else if (profileMask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) profile += "Compat"; else profile += "Unknown";
		info.push_back(profile);
	}
	{
		std::string flags = "Flags:";
		if (contextFlags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT) flags += " FwdCompat";
		if (contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT) flags += " Debug";
		if (contextFlags & GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT) flags += " Robust";
		if (contextFlags & GL_CONTEXT_FLAG_NO_ERROR_BIT) flags += " NoError";
		info.push_back(flags);
	}
	
	GLint maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	info.push_back("Max Texture Size: " + std::to_string(maxTextureSize));
	GLint max3DTexture = 0; glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3DTexture); info.push_back("Max 3D Texture: " + std::to_string(max3DTexture));
	GLint maxCube = 0; glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &maxCube); info.push_back("Max CubeMap: " + std::to_string(maxCube));
	GLint maxLayers = 0; glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxLayers); info.push_back("Max Array Layers: " + std::to_string(maxLayers));
	
	GLint maxViewportDims[2];
	glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewportDims);
	std::ostringstream oss2;
	oss2 << "Max Viewport: " << maxViewportDims[0] << "x" << maxViewportDims[1];
	info.push_back(oss2.str());
	
	GLint maxColorAttachments = 0;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
	info.push_back("Max Color Attachments: " + std::to_string(maxColorAttachments));
	
	GLint maxTextureUnits = 0;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
	info.push_back("Max Texture Units: " + std::to_string(maxTextureUnits));
	GLint maxVertTex = 0; glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxVertTex); info.push_back("Max Vertex Texture Units: " + std::to_string(maxVertTex));
	GLint maxGeomTex = 0; glGetIntegerv(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, &maxGeomTex); info.push_back("Max Geometry Texture Units: " + std::to_string(maxGeomTex));
	GLint maxDrawBuffers = 0; glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers); info.push_back("Max Draw Buffers: " + std::to_string(maxDrawBuffers));
	GLint maxSamples = 0; glGetIntegerv(GL_MAX_SAMPLES, &maxSamples); info.push_back("Max MSAA Samples: " + std::to_string(maxSamples));
	GLint maxRBO = 0; glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRBO); info.push_back("Max Renderbuffer Size: " + std::to_string(maxRBO));
	
	GLint maxVSUniform = 0; glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxVSUniform); info.push_back("Max VS Uniform Components: " + std::to_string(maxVSUniform));
	GLint maxFSUniform = 0; glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &maxFSUniform); info.push_back("Max FS Uniform Components: " + std::to_string(maxFSUniform));
	GLint maxUBOSize = 0; glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize); info.push_back("Max UBO Size: " + std::to_string(maxUBOSize));
	GLint maxSSBOBindings = 0; glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxSSBOBindings); info.push_back("Max SSBO Bindings: " + std::to_string(maxSSBOBindings));
	GLint maxSSBOSize = 0; glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxSSBOSize); info.push_back("Max SSBO Block Size: " + std::to_string(maxSSBOSize));
	GLint maxComputeInv = 0; glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxComputeInv); info.push_back("Max Compute Invocations: " + std::to_string(maxComputeInv));
	GLint wgCount[3]; glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &wgCount[0]); glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &wgCount[1]); glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &wgCount[2]);
	{ std::ostringstream c; c << "Compute WorkGroup Count: [" << wgCount[0] << "," << wgCount[1] << "," << wgCount[2] << "]"; info.push_back(c.str()); }
	GLint wgSize[3]; glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &wgSize[0]); glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &wgSize[1]); glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &wgSize[2]);
	{ std::ostringstream c; c << "Compute WorkGroup Size: [" << wgSize[0] << "," << wgSize[1] << "," << wgSize[2] << "]"; info.push_back(c.str()); }
	
	// Extensions
	GLint numExt = 0; glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
	info.push_back("Extensions: " + std::to_string(numExt));
	for (int i = 0; i < numExt; ++i) {
		const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
		if (ext) info.push_back(std::string("  ") + ext);
	}
	
	GLint maxCombinedTextureUnits = 0;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTextureUnits);
	info.push_back("Max Combined Texture Units: " + std::to_string(maxCombinedTextureUnits));
	
	GLint maxVertexAttribs = 0;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
	info.push_back("Max Vertex Attributes: " + std::to_string(maxVertexAttribs));
	
	GLint maxUniformBufferBindings = 0;
	glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniformBufferBindings);
	info.push_back("Max Uniform Buffer Bindings: " + std::to_string(maxUniformBufferBindings));
	
	return info;
}

// You can locally declare a SDL_main function or call to a DLL export (mingw works nice for this) 
int SDL_main(int argc, char* argv[])
{
	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		return -1;
	}

	// Set OpenGL attributes
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// Create window
	SDL_Window* window = SDL_CreateWindow(
		"OpenGL System Info",       // Window title
		SDL_WINDOWPOS_UNDEFINED,    // Window positions not used
		SDL_WINDOWPOS_UNDEFINED,
		1920,                       // Width of framebuffer
		1080,                        // Height of framebuffer
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
	);

	if (!window) {
		SDL_Quit();
		return -1;
	}

	// Create OpenGL context
	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	if (!glContext) {
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	SDL_GL_MakeCurrent(window, glContext);

	// Load extensions after creating context
	gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
	
	// Get window dimensions
	int windowWidth, windowHeight;
	SDL_GL_GetDrawableSize(window, &windowWidth, &windowHeight);
	glViewport(0, 0, windowWidth, windowHeight);
	
	// Compute DPI scale so text stays reasonable across DPI settings
	float dpiScale = 1.0f;
	int displayIndex = SDL_GetWindowDisplayIndex(window);
	float ddpi=96.0f, hdpi=96.0f, vdpi=96.0f;
	if (displayIndex >= 0 && SDL_GetDisplayDPI(displayIndex, &ddpi, &hdpi, &vdpi) == 0 && ddpi > 0.0f) {
		dpiScale = 96.0f / ddpi; // >1.0 on low DPI, <1.0 on high DPI
	}
	
	// Initialize text renderer
	TextRenderer textRenderer;
	textRenderer.fontSize = 24.0f * dpiScale;
	if (!initTextRenderer(textRenderer, windowWidth, windowHeight)) {
		SDL_GL_DeleteContext(glContext);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}
	
	// Initialize 3D cube renderer
	CubeRenderer cubeRenderer;
	initCubeRenderer(cubeRenderer);
	
	// Get system and OpenGL info
	std::vector<std::string> systemInfo = getSystemInfo();
	std::vector<std::string> openGLInfo = getOpenGLInfo();

	std::vector<std::string> glCoreInfo;
	std::vector<std::string> glExtInfo;
	bool inExt = false;
	int extCount = 0;
	for (const auto& s : openGLInfo) {
		if (!inExt) {
			if (s.rfind("Extensions:", 0) == 0) { // begins with "Extensions:"
				inExt = true;
				glExtInfo.push_back(s);
				size_t pos = s.find(':');
				if (pos != std::string::npos) {
					try { extCount = std::stoi(s.substr(pos+1)); } catch(...) {}
				}
			} else {
				glCoreInfo.push_back(s);
			}
		} else {
			glExtInfo.push_back(s);
		}
	}
	
	std::vector<std::string> leftInfo;
	leftInfo.push_back("OPENGL INFORMATION");
	for (const auto& line : glCoreInfo) leftInfo.push_back(line);
	leftInfo.push_back("");
	leftInfo.push_back("SYSTEM INFORMATION");
	for (const auto& line : systemInfo) leftInfo.push_back(line);
	
	SDL_Event event;
	bool running = true;
	while (running) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				running = false;
				break;
			} else if (event.type == SDL_WINDOWEVENT) {
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					SDL_GL_GetDrawableSize(window, &windowWidth, &windowHeight);
					textRenderer.windowWidth = windowWidth;
					textRenderer.windowHeight = windowHeight;
					glViewport(0, 0, windowWidth, windowHeight);
				}
			}
		}

		// Ensure viewport matches drawable size each frame (handles DPI scaling)
		SDL_GL_GetDrawableSize(window, &windowWidth, &windowHeight);
		textRenderer.windowWidth = windowWidth;
		textRenderer.windowHeight = windowHeight;
		glViewport(0, 0, windowWidth, windowHeight);
		
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.05f, 0.10f, 0.25f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glUseProgram(cubeRenderer.program);
		glBindVertexArray(cubeRenderer.vao);
		float t = (float)SDL_GetTicks() * 0.001f;
		float proj[16], view[16], rotY[16], rotX[16], model[16], mv[16], mvp[16];
		float aspect = (float)windowWidth / (float)windowHeight;
		mat4Perspective(proj, 60.0f * 3.14159265f / 180.0f, aspect, 0.1f, 100.0f);
		mat4Translate(view, 0.0f, 0.0f, -4.0f);
		mat4RotateY(rotY, t * 1.2f);
		mat4RotateX(rotX, t * 0.7f);
		mat4Multiply(model, rotY, rotX);
		mat4Multiply(mv, view, model);
		mat4Multiply(mvp, proj, mv);
		GLint loc = glGetUniformLocation(cubeRenderer.program, "uMVP");
		glUniformMatrix4fv(loc, 1, GL_FALSE, mvp);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		
		// Enable blending for text
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		float baseTextPx = (float)windowHeight / 60.0f;
		if (baseTextPx < 12.0f) baseTextPx = 12.0f;
		if (baseTextPx > 20.0f) baseTextPx = 20.0f;
		
		float scaleLeft;
		float lineHeightLeft;
		if (textRenderer.glyphs.empty()) {
			scaleLeft = baseTextPx / 8.0f;
			lineHeightLeft = baseTextPx * 1.3f;
		} else {
			scaleLeft = baseTextPx / textRenderer.fontSize;
			lineHeightLeft = baseTextPx * 1.3f;
		}
		float yLeft = baseTextPx;
		for (const auto& line : leftInfo) {
			renderText(line, 28.0f, yLeft, scaleLeft, textRenderer);
			yLeft += lineHeightLeft;
		}

		float scaleRight;
		float lineHeightRight;
		if (textRenderer.glyphs.empty()) {
			scaleRight = (baseTextPx * 1.0f) / 8.0f;
			lineHeightRight = baseTextPx * 1.10f;
		} else {
			scaleRight = (baseTextPx * 0.85f) / textRenderer.fontSize;
			lineHeightRight = baseTextPx * 0.95f;
		}
		const float rightMargin = baseTextPx * 0.75f;
		float yRightTop = baseTextPx;
		float yRight = yRightTop;
		float bottomLimit = (float)windowHeight - baseTextPx;
		float currentRightEdge = (float)windowWidth - rightMargin;
		float colPadding = baseTextPx;
		float maxColWidth = 0.0f;
		if (!glExtInfo.empty()) {
			for (const auto& line : glExtInfo) {
				if (yRight + lineHeightRight > bottomLimit) {
					currentRightEdge -= (maxColWidth + colPadding);
					yRight = yRightTop;
					maxColWidth = 0.0f;
					if (currentRightEdge < 0.0f) currentRightEdge = (float)windowWidth - rightMargin;
				}

				float textWidth = measureTextWidth(line, scaleRight, textRenderer);
				if (textWidth > maxColWidth) maxColWidth = textWidth;
				float startX = currentRightEdge - textWidth;
				if (startX < 0.0f) startX = 0.0f;
				renderText(line, startX, yRight, scaleRight, textRenderer);
				yRight += lineHeightRight;
			}
		}
		
		glDisable(GL_BLEND);

		// Swap buffers
		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	glDeleteVertexArrays(1, &cubeRenderer.vao);
	glDeleteBuffers(1, &cubeRenderer.vbo);
	glDeleteProgram(cubeRenderer.program);
	glDeleteTextures(1, &textRenderer.fontTexture);
	glDeleteVertexArrays(1, &textRenderer.vao);
	glDeleteBuffers(1, &textRenderer.vbo);
	glDeleteBuffers(1, &textRenderer.ebo);
	glDeleteProgram(textRenderer.program);
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;

}

// Entry point into app (Note, SDL doesn't like being init from here you must call SDL_main)
int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR argv, int argc)
{
	return SDL_WinRTRunApp(SDL_main, NULL);
}
