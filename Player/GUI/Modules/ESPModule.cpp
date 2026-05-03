#include "ESPModule.h"
#include "../PathUtils.h"
#include <windows.h>
#include <gdiplus.h>
#include <GL/gl.h>
#include <jni.h>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#pragma comment(lib, "gdiplus.lib")

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_LINEAR_MIPMAP_LINEAR
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#endif

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

using namespace Gdiplus;

extern JavaVM* g_vm;
extern bool g_initialized;

extern jclass mcClass, timerClass, entityClass, worldClass, listClass, playerClass;

extern jfieldID mcInstanceField;
extern jfieldID localPlayerField;
extern jfieldID worldField;
extern jfieldID renderManagerField;
extern jfieldID timerField;

extern jfieldID viewerPosXField;
extern jfieldID viewerPosYField;
extern jfieldID viewerPosZField;
extern jfieldID playerViewYField;
extern jfieldID playerViewXField;

extern jfieldID partialTicksField;

extern jfieldID lastTickPosXField;
extern jfieldID lastTickPosYField;
extern jfieldID lastTickPosZField;

extern jfieldID posXField;
extern jfieldID posYField;
extern jfieldID posZField;

extern jfieldID playersListField;
extern jmethodID toArrayMethod;

extern jclass entityLivingBaseClass;
extern jmethodID getHealthMethod;
extern jmethodID getMaxHealthMethod;

extern jmethodID getTeamMethod;
extern jmethodID isSameTeamMethod;

typedef void (APIENTRY* PFNGLUSEPROGRAMPROC)(GLuint program);
typedef void (APIENTRY* PFNGLGENERATEMIPMAPPROC)(GLenum target);

static PFNGLUSEPROGRAMPROC glUseProgramPtr = nullptr;
static PFNGLGENERATEMIPMAPPROC glGenerateMipmapPtr = nullptr;

static GLuint vineTexture = 0;
static bool vineTextureLoaded = false;
static const wchar_t* HUD_FRAME_PATHS[6] = {
    L"GUI\\sprites\\frame_R1_C1.png",
    L"GUI\\sprites\\frame_R1_C2.png",
    L"GUI\\sprites\\frame_R1_C3.png",
    L"GUI\\sprites\\frame_R2_C1.png",
    L"GUI\\sprites\\frame_R2_C2.png",
    L"GUI\\sprites\\frame_R2_C3.png"
};

static const wchar_t* HUD_HEART_PATHS[6][3] = {
    {
        L"GUI\\sprites\\heart_R1_C1_top.png",
        L"GUI\\sprites\\heart_R1_C1_mid.png",
        L"GUI\\sprites\\heart_R1_C1_bot.png"
    },
    {
        L"GUI\\sprites\\heart_R1_C2_top.png",
        L"GUI\\sprites\\heart_R1_C2_mid.png",
        L"GUI\\sprites\\heart_R1_C2_bot.png"
    },
    {
        L"GUI\\sprites\\heart_R1_C3_top.png",
        L"GUI\\sprites\\heart_R1_C3_mid.png",
        L"GUI\\sprites\\heart_R1_C3_bot.png"
    },
    {
        L"GUI\\sprites\\heart_R2_C1_top.png",
        L"GUI\\sprites\\heart_R2_C1_mid.png",
        L"GUI\\sprites\\heart_R2_C1_bot.png"
    },
    {
        L"GUI\\sprites\\heart_R2_C2_top.png",
        L"GUI\\sprites\\heart_R2_C2_mid.png",
        L"GUI\\sprites\\heart_R2_C2_bot.png"
    },
    {
        L"GUI\\sprites\\heart_R2_C3_top.png",
        L"GUI\\sprites\\heart_R2_C3_mid.png",
        L"GUI\\sprites\\heart_R2_C3_bot.png"
    }
};

static const wchar_t* HUD_CONFIG_PATH_REL = L"GUI\\health_hud_config.json";

static void skipJsonWs(const char*& p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
        p++;
    }
}

static bool jsonReadFloat(const std::string& json, const char* key, float& out) {
    const std::string q = std::string("\"") + key + "\"";
    size_t pos = json.find(q);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos + q.size());
    if (pos == std::string::npos) return false;
    const char* p = json.c_str() + pos + 1;
    skipJsonWs(p);
    char* end = nullptr;
    double v = strtod(p, &end);
    if (end == p) return false;
    out = (float)v;
    return true;
}

static bool jsonReadInt(const std::string& json, const char* key, int& out) {
    const std::string q = std::string("\"") + key + "\"";
    size_t pos = json.find(q);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos + q.size());
    if (pos == std::string::npos) return false;
    const char* p = json.c_str() + pos + 1;
    skipJsonWs(p);
    char* end = nullptr;
    long v = strtol(p, &end, 10);
    if (end == p) return false;
    out = (int)v;
    return true;
}

struct Vec3 {
    double x, y, z;
};

struct PlayerData {
    Vec3 pos;
    Vec3 lastTickPos;
    bool isValid;
    bool isTeammate;
    float health;
    float maxHealth;
    std::string name;
};

static float clampFloat(float value, float minValue, float maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

ESPModule::ESPModule() : Module("ESP", true) {
    addSetting("Vines", true);
    addSetting("Tracers", false);
    addSetting("Tracer Radius", 35.0f, 5.0f, 200.0f);
}

std::string ESPModule::getPlayerName(JNIEnv* env, jobject p) {
    if (!p) return "Unknown";
    jclass cls = env->GetObjectClass(p);
    jmethodID mid = env->GetMethodID(cls, "e_", "()Ljava/lang/String;");
    if (!mid) { env->ExceptionClear(); mid = env->GetMethodID(cls, "getName", "()Ljava/lang/String;"); }
    
    if (mid) {
        jstring js = (jstring)env->CallObjectMethod(p, mid);
        if (js) {
            const char* str = env->GetStringUTFChars(js, nullptr);
            std::string res(str); env->ReleaseStringUTFChars(js, str);
            env->DeleteLocalRef(js); 
            env->DeleteLocalRef(cls);
            return res;
        }
    }
    env->ExceptionClear();
    if (cls) env->DeleteLocalRef(cls);
    return "Unknown";
}

void ESPModule::refreshHealthHudLayout() {
    static ULARGE_INTEGER s_lastWrite = {};

    std::wstring fullPath = PathUtils::GetPath(HUD_CONFIG_PATH_REL);
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(fullPath.c_str(), GetFileExInfoStandard, &fad)) {
        return;
    }

    ULARGE_INTEGER lw;
    lw.LowPart = fad.ftLastWriteTime.dwLowDateTime;
    lw.HighPart = fad.ftLastWriteTime.dwHighDateTime;

    if (lw.QuadPart == s_lastWrite.QuadPart) {
        return;
    }

    FILE* fp = _wfopen(fullPath.c_str(), L"rb");
    if (!fp) {
        return;
    }
    fseek(fp, 0, SEEK_END);
    long n = ftell(fp);
    if (n <= 0) {
        fclose(fp);
        return;
    }
    fseek(fp, 0, SEEK_SET);
    std::string json;
    json.resize((size_t)n);
    if (fread(&json[0], 1, (size_t)n, fp) != (size_t)n) {
        fclose(fp);
        return;
    }
    fclose(fp);

    float f = 0.0f;
    int k = 0;

    if (jsonReadFloat(json, "fw", f)) hudLayout.frameW = clampFloat(f, 1.0f, 10000.0f);
    if (jsonReadFloat(json, "fh", f)) hudLayout.frameH = clampFloat(f, 1.0f, 10000.0f);
    if (jsonReadFloat(json, "fshift", f)) hudLayout.frameYShift = clampFloat(f, -10.0f, 10.0f);
    if (jsonReadFloat(json, "hw", f)) hudLayout.heartW = clampFloat(f, 1.0f, 10000.0f);
    if (jsonReadFloat(json, "hh", f)) hudLayout.heartH = clampFloat(f, 1.0f, 10000.0f);
    if (jsonReadFloat(json, "hspacing", f)) hudLayout.heartSpacing = clampFloat(f, 0.0f, 10.0f);
    if (jsonReadFloat(json, "hxoffset", f)) hudLayout.heartXOffset = clampFloat(f, -10.0f, 10.0f);
    if (jsonReadFloat(json, "hystart", f)) hudLayout.heartYStart = clampFloat(f, -10.0f, 10.0f);
    if (jsonReadInt(json, "heartCount", k)) hudLayout.heartCount = (int)clampFloat((float)k, 1.0f, 60.0f);
    if (jsonReadFloat(json, "billboardScale", f)) hudLayout.billboardScale = clampFloat(f, 0.00001f, 1.0f);
    if (jsonReadFloat(json, "sideOffset", f)) hudLayout.sideOffset = clampFloat(f, -10.0f, 10.0f);
    if (jsonReadFloat(json, "layerNudge", f)) hudLayout.layerNudge = clampFloat(f, -1.0f, 1.0f);
    if (jsonReadFloat(json, "animFps", f)) hudLayout.animFps = clampFloat(f, 0.1f, 120.0f);

    s_lastWrite = lw;
}

static bool loadTextureFromFile(
    const wchar_t* path,
    GLuint& outTexture,
    bool pixelated,
    bool useMipmaps,
    int* outWidth = nullptr,
    int* outHeight = nullptr,
    bool blackToTransparent = false
) {
    Bitmap* bitmap = Bitmap::FromFile(path);

    if (!bitmap || bitmap->GetLastStatus() != Ok) {
        if (bitmap) delete bitmap;
        return false;
    }

    UINT width = bitmap->GetWidth();
    UINT height = bitmap->GetHeight();

    if (outWidth) *outWidth = (int)width;
    if (outHeight) *outHeight = (int)height;

    BitmapData data;
    Rect rect(0, 0, width, height);

    if (bitmap->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &data) != Ok) {
        delete bitmap;
        return false;
    }

    unsigned char* src = (unsigned char*)data.Scan0;
    std::vector<unsigned char> pixels(width * height * 4);

    for (UINT y = 0; y < height; y++) {
        unsigned char* row = src + y * data.Stride;

        for (UINT x = 0; x < width; x++) {
            UINT srcIndex = x * 4;
            UINT dstIndex = (y * width + x) * 4;

            unsigned char b = row[srcIndex + 0];
            unsigned char g = row[srcIndex + 1];
            unsigned char r = row[srcIndex + 2];
            unsigned char a = row[srcIndex + 3];

            if (blackToTransparent && r <= 4 && g <= 4 && b <= 4) {
                a = 0;
            }

            pixels[dstIndex + 0] = r;
            pixels[dstIndex + 1] = g;
            pixels[dstIndex + 2] = b;
            pixels[dstIndex + 3] = a;
        }
    }

    glGenTextures(1, &outTexture);
    glBindTexture(GL_TEXTURE_2D, outTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (pixelated) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    } else {
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            useMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels.data()
    );

    if (useMipmaps && glGenerateMipmapPtr) {
        glGenerateMipmapPtr(GL_TEXTURE_2D);

        float maxAniso = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);

        if (glGetError() == GL_NO_ERROR && maxAniso > 0.0f) {
            float aniso = maxAniso;
            if (aniso > 8.0f) aniso = 8.0f;
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
        } else {
            glGetError();
        }
    }

    bitmap->UnlockBits(&data);
    delete bitmap;

    return true;
}

void ESPModule::loadDungeonTexture() {
    bool allHudLoaded = true;
    for (int i = 0; i < HUD_ANIM_FRAME_COUNT; i++) {
        if (!hudAnimationTexturesLoaded[i]) {
            allHudLoaded = false;
            break;
        }
        for (int v = 0; v < HUD_HEART_VARIANT_COUNT; v++) {
            if (!hudHeartTexturesLoaded[i][v]) {
                allHudLoaded = false;
                break;
            }
        }
        if (!allHudLoaded) break;
    }
    if (textureLoaded && vineTextureLoaded && allHudLoaded) return;

    if (!glGenerateMipmapPtr) {
        glGenerateMipmapPtr = (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
    }

    if (!textureLoaded) {
        textureLoaded = loadTextureFromFile(
            PathUtils::GetPath(L"GUI\\health_bar.png").c_str(),
            atlasTexture,
            true,
            false
        );
    }

    for (int i = 0; i < HUD_ANIM_FRAME_COUNT; i++) {
        if (!hudAnimationTexturesLoaded[i]) {
            hudAnimationTexturesLoaded[i] = loadTextureFromFile(
                PathUtils::GetPath(HUD_FRAME_PATHS[i]).c_str(),
                hudAnimationTextures[i],
                true,
                false,
                &hudAnimationTextureWidths[i],
                &hudAnimationTextureHeights[i],
                false
            );
        }
    }

    for (int i = 0; i < HUD_ANIM_FRAME_COUNT; i++) {
        for (int v = 0; v < HUD_HEART_VARIANT_COUNT; v++) {
            if (!hudHeartTexturesLoaded[i][v]) {
                hudHeartTexturesLoaded[i][v] = loadTextureFromFile(
                    PathUtils::GetPath(HUD_HEART_PATHS[i][v]).c_str(),
                    hudHeartTextures[i][v],
                    true,
                    false,
                    nullptr,
                    nullptr,
                    false
                );
            }
        }
    }

    if (!vineTextureLoaded) {
        vineTextureLoaded = loadTextureFromFile(
            PathUtils::GetPath(L"GUI\\vines.png").c_str(),
            vineTexture,
            false,
            true
        );
    }
}

double ESPModule::getAnimationTimeSeconds() const {
    LARGE_INTEGER freq;
    LARGE_INTEGER counter;

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);

    return (double)counter.QuadPart / (double)freq.QuadPart;
}

void ESPModule::unloadTextures() {
    if (atlasTexture != 0) {
        glDeleteTextures(1, &atlasTexture);
        atlasTexture = 0;
    }

    for (int i = 0; i < HUD_ANIM_FRAME_COUNT; i++) {
        if (hudAnimationTextures[i] != 0) {
            glDeleteTextures(1, &hudAnimationTextures[i]);
            hudAnimationTextures[i] = 0;
        }
        hudAnimationTexturesLoaded[i] = false;
        hudAnimationTextureWidths[i] = 0;
        hudAnimationTextureHeights[i] = 0;

        for (int v = 0; v < HUD_HEART_VARIANT_COUNT; v++) {
            if (hudHeartTextures[i][v] != 0) {
                glDeleteTextures(1, &hudHeartTextures[i][v]);
                hudHeartTextures[i][v] = 0;
            }
            hudHeartTexturesLoaded[i][v] = false;
        }
    }

    if (vineTexture != 0) {
        glDeleteTextures(1, &vineTexture);
        vineTexture = 0;
    }

    textureLoaded = false;
    vineTextureLoaded = false;
}

void ESPModule::onDisable() {
    unloadTextures();
}

void ESPModule::drawAnimatedHealthHudBillboard(
    float x,
    float y,
    float z,
    float w,
    float h,
    float rightX,
    float rightZ
) {
    static const int FRAME_COUNT = HUD_ANIM_FRAME_COUNT;
    int loadedFrameCount = 0;
    for (int i = 0; i < FRAME_COUNT; i++) {
        if (hudAnimationTexturesLoaded[i] && hudAnimationTextures[i] != 0) {
            loadedFrameCount++;
        }
    }

    if (loadedFrameCount <= 0) {
        return;
    }

    float x1 = x - rightX * (w / 2.0f);
    float z1 = z - rightZ * (w / 2.0f);
    float x2 = x + rightX * (w / 2.0f);
    float z2 = z + rightZ * (w / 2.0f);

    glBegin(GL_QUADS);

    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(x1, y, z1);

    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(x2, y, z2);

    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(x2, y + h, z2);

    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(x1, y + h, z1);

    glEnd();
}

void drawTexturedBillboard(
    float x,
    float y,
    float z,
    float w,
    float h,
    float rightX,
    float rightZ,
    float u1,
    float v1,
    float u2,
    float v2
) {
    float x1 = x - rightX * (w / 2.0f);
    float z1 = z - rightZ * (w / 2.0f);
    float x2 = x + rightX * (w / 2.0f);
    float z2 = z + rightZ * (w / 2.0f);

    glBegin(GL_QUADS);

    glTexCoord2f(u1, v2);
    glVertex3f(x1, y, z1);

    glTexCoord2f(u2, v2);
    glVertex3f(x2, y, z2);

    glTexCoord2f(u2, v1);
    glVertex3f(x2, y + h, z2);

    glTexCoord2f(u1, v1);
    glVertex3f(x1, y + h, z1);

    glEnd();
}

static const float VINE_U1 = 0.030f;
static const float VINE_V1 = 0.365f;
static const float VINE_U2 = 0.955f;
static const float VINE_V2 = 0.555f;

static void drawVineQuad(
    float ax,
    float ay,
    float az,
    float bx,
    float by,
    float bz,
    float sx,
    float sy,
    float sz,
    float u1,
    float u2
) {
    glBegin(GL_QUADS);

    glTexCoord2f(u1, VINE_V2);
    glVertex3f(ax - sx, ay - sy, az - sz);

    glTexCoord2f(u2, VINE_V2);
    glVertex3f(bx - sx, by - sy, bz - sz);

    glTexCoord2f(u2, VINE_V1);
    glVertex3f(bx + sx, by + sy, bz + sz);

    glTexCoord2f(u1, VINE_V1);
    glVertex3f(ax + sx, ay + sy, az + sz);

    glEnd();
}

static void drawVineSegment3D(
    float ax,
    float ay,
    float az,
    float bx,
    float by,
    float bz,
    float thickness,
    float horizontalUvScale,
    bool forceSimpleUv
) {
    float dx = bx - ax;
    float dy = by - ay;
    float dz = bz - az;

    float length = sqrtf(dx * dx + dy * dy + dz * dz);
    if (length <= 0.001f) return;

    float half = thickness * 0.5f;

    float adx = fabsf(dx);
    float ady = fabsf(dy);
    float adz = fabsf(dz);

    float s1x = 0.0f;
    float s1y = 0.0f;
    float s1z = 0.0f;

    float s2x = 0.0f;
    float s2y = 0.0f;
    float s2z = 0.0f;

    bool verticalEdge = (ady >= adx && ady >= adz);

    if (verticalEdge) {
        s1x = half;
        s2z = half;
    } else if (adx >= ady && adx >= adz) {
        s1y = half;
        s2z = half;
    } else {
        s1y = half;
        s2x = half;
    }

    float uRange = VINE_U2 - VINE_U1;
    float uvScale = verticalEdge ? 1.0f : horizontalUvScale;

    if (forceSimpleUv) {
        uvScale = verticalEdge ? 0.72f : horizontalUvScale;
    }

    if (uvScale < 0.18f) uvScale = 0.18f;
    if (uvScale > 1.0f) uvScale = 1.0f;

    float u1 = VINE_U1;
    float u2 = VINE_U1 + (uRange * uvScale);

    drawVineQuad(ax, ay, az, bx, by, bz, s1x, s1y, s1z, u1, u2);
    drawVineQuad(ax, ay, az, bx, by, bz, s2x, s2y, s2z, u1, u2);
}

static void drawTexturedVineHitbox(
    float boxW,
    float boxH,
    float vineThickness,
    float horizontalUvScale,
    bool simplifiedUv
) {
    float x0 = -boxW;
    float x1 = boxW;

    float z0 = -boxW;
    float z1 = boxW;

    float y0 = 0.0f;
    float y1 = boxH;

    drawVineSegment3D(x0, y0, z0, x1, y0, z0, vineThickness, horizontalUvScale, simplifiedUv);
    drawVineSegment3D(x1, y0, z0, x1, y0, z1, vineThickness, horizontalUvScale, simplifiedUv);
    drawVineSegment3D(x1, y0, z1, x0, y0, z1, vineThickness, horizontalUvScale, simplifiedUv);
    drawVineSegment3D(x0, y0, z1, x0, y0, z0, vineThickness, horizontalUvScale, simplifiedUv);

    drawVineSegment3D(x0, y1, z0, x1, y1, z0, vineThickness, horizontalUvScale, simplifiedUv);
    drawVineSegment3D(x1, y1, z0, x1, y1, z1, vineThickness, horizontalUvScale, simplifiedUv);
    drawVineSegment3D(x1, y1, z1, x0, y1, z1, vineThickness, horizontalUvScale, simplifiedUv);
    drawVineSegment3D(x0, y1, z1, x0, y1, z0, vineThickness, horizontalUvScale, simplifiedUv);

    drawVineSegment3D(x0, y0, z0, x0, y1, z0, vineThickness, horizontalUvScale, simplifiedUv);
    drawVineSegment3D(x1, y0, z0, x1, y1, z0, vineThickness, horizontalUvScale, simplifiedUv);
    drawVineSegment3D(x1, y0, z1, x1, y1, z1, vineThickness, horizontalUvScale, simplifiedUv);
    drawVineSegment3D(x0, y0, z1, x0, y1, z1, vineThickness, horizontalUvScale, simplifiedUv);
}

static void drawFarVineHitbox(float boxW, float boxH, float distanceFromCamera) {
    float x0 = -boxW;
    float x1 = boxW;

    float z0 = -boxW;
    float z1 = boxW;

    float y0 = 0.0f;
    float y1 = boxH;

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float lineWidth = 3.0f + (distanceFromCamera * 0.035f);
    if (lineWidth > 6.0f) lineWidth = 6.0f;

    glLineWidth(lineWidth);

    glColor4f(0.05f, 0.16f, 0.04f, 0.95f);

    glBegin(GL_LINES);

    glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0);
    glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1);
    glVertex3f(x1, y0, z1); glVertex3f(x0, y0, z1);
    glVertex3f(x0, y0, z1); glVertex3f(x0, y0, z0);

    glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
    glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);
    glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
    glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);

    glVertex3f(x0, y0, z0); glVertex3f(x0, y1, z0);
    glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0);
    glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1);
    glVertex3f(x0, y0, z1); glVertex3f(x0, y1, z1);

    glEnd();

    glLineWidth(lineWidth * 0.62f);
    glColor4f(0.22f, 0.95f, 0.22f, 0.95f);

    glBegin(GL_LINES);

    glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0);
    glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1);
    glVertex3f(x1, y0, z1); glVertex3f(x0, y0, z1);
    glVertex3f(x0, y0, z1); glVertex3f(x0, y0, z0);

    glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
    glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);
    glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
    glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);

    glVertex3f(x0, y0, z0); glVertex3f(x0, y1, z0);
    glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0);
    glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1);
    glVertex3f(x0, y0, z1); glVertex3f(x0, y1, z1);

    glEnd();

    glLineWidth(1.0f);
}

static void drawStandardLineHitbox(float boxW, float boxH) {
    float x0 = -boxW;
    float x1 = boxW;

    float z0 = -boxW;
    float z1 = boxW;

    float y0 = 0.0f;
    float y1 = boxH;

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glLineWidth(2.0f);
    glColor4f(0.0f, 1.0f, 0.0f, 0.95f);

    glBegin(GL_LINES);

    glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0);
    glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1);
    glVertex3f(x1, y0, z1); glVertex3f(x0, y0, z1);
    glVertex3f(x0, y0, z1); glVertex3f(x0, y0, z0);

    glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
    glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);
    glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
    glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);

    glVertex3f(x0, y0, z0); glVertex3f(x0, y1, z0);
    glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0);
    glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1);
    glVertex3f(x0, y0, z1); glVertex3f(x0, y1, z1);

    glEnd();

    glLineWidth(1.0f);
}

static void drawVineLoDHitbox(float boxW, float boxH, float distanceFromCamera) {
    if (distanceFromCamera < 11.0f) {
        drawTexturedVineHitbox(
            boxW,
            boxH,
            0.42f,
            0.42f,
            false
        );
        return;
    }

    if (distanceFromCamera < 24.0f) {
        drawTexturedVineHitbox(
            boxW,
            boxH,
            0.55f,
            0.28f,
            true
        );
        return;
    }

    drawFarVineHitbox(boxW, boxH, distanceFromCamera);
}

static void drawPlayerDepthOccluder(float halfW, float height) {
    float x0 = -halfW;
    float x1 = halfW;

    float z0 = -halfW;
    float z1 = halfW;

    float y0 = 0.0f;
    float y1 = height;

    glBegin(GL_QUADS);

    glVertex3f(x0, y0, z1);
    glVertex3f(x1, y0, z1);
    glVertex3f(x1, y1, z1);
    glVertex3f(x0, y1, z1);

    glVertex3f(x1, y0, z0);
    glVertex3f(x0, y0, z0);
    glVertex3f(x0, y1, z0);
    glVertex3f(x1, y1, z0);

    glVertex3f(x1, y0, z1);
    glVertex3f(x1, y0, z0);
    glVertex3f(x1, y1, z0);
    glVertex3f(x1, y1, z1);

    glVertex3f(x0, y0, z0);
    glVertex3f(x0, y0, z1);
    glVertex3f(x0, y1, z1);
    glVertex3f(x0, y1, z0);

    glVertex3f(x0, y1, z1);
    glVertex3f(x1, y1, z1);
    glVertex3f(x1, y1, z0);
    glVertex3f(x0, y1, z0);

    glVertex3f(x0, y0, z0);
    glVertex3f(x1, y0, z0);
    glVertex3f(x1, y0, z1);
    glVertex3f(x0, y0, z1);

    glEnd();
}

void ESPModule::onRender() {
    if (!g_vm || !g_initialized) return;

    refreshHealthHudLayout();

    bool allHudLoaded = true;
    for (int i = 0; i < HUD_ANIM_FRAME_COUNT; i++) {
        if (!hudAnimationTexturesLoaded[i]) {
            allHudLoaded = false;
            break;
        }
        for (int v = 0; v < HUD_HEART_VARIANT_COUNT; v++) {
            if (!hudHeartTexturesLoaded[i][v]) {
                allHudLoaded = false;
                break;
            }
        }
        if (!allHudLoaded) break;
    }
    if (!textureLoaded || !vineTextureLoaded || !allHudLoaded) {
        loadDungeonTexture();
    }

    if (!glUseProgramPtr) {
        glUseProgramPtr = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    }

    if (!glGenerateMipmapPtr) {
        glGenerateMipmapPtr = (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
    }

    JNIEnv* env = nullptr;

    if (g_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK || !env) {
        return;
    }

    jobject mc = env->GetStaticObjectField(mcClass, mcInstanceField);

    if (!mc) {
        return;
    }

    jobject world = env->GetObjectField(mc, worldField);
    jobject localPlayer = env->GetObjectField(mc, localPlayerField);
    jobject renderManager = env->GetObjectField(mc, renderManagerField);
    jobject timer = env->GetObjectField(mc, timerField);

    if (world && renderManager && timer && partialTicksField) {
        float partialTicks = env->GetFloatField(timer, partialTicksField);

        double camX = env->GetDoubleField(renderManager, viewerPosXField);
        double camY = env->GetDoubleField(renderManager, viewerPosYField) + 1.62;
        double camZ = env->GetDoubleField(renderManager, viewerPosZField);

        float yaw = env->GetFloatField(renderManager, playerViewYField);
        float pitch = env->GetFloatField(renderManager, playerViewXField);

        jobject localTeam = nullptr;

        if (localPlayer && getTeamMethod) {
            localTeam = env->CallObjectMethod(localPlayer, getTeamMethod);

            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                localTeam = nullptr;
            }
        }

        std::vector<PlayerData> players;

        jobject playersList = env->GetObjectField(world, playersListField);

        if (playersList) {
            jobjectArray playersArray = (jobjectArray)env->CallObjectMethod(playersList, toArrayMethod);

            if (playersArray) {
                jint count = env->GetArrayLength(playersArray);

                for (int i = 0; i < count; i++) {
                    jobject p = env->GetObjectArrayElement(playersArray, i);

                    if (p && !env->IsSameObject(p, localPlayer) && env->IsInstanceOf(p, playerClass)) {
                        PlayerData pd;

                        pd.lastTickPos.x = env->GetDoubleField(p, lastTickPosXField);
                        pd.lastTickPos.y = env->GetDoubleField(p, lastTickPosYField);
                        pd.lastTickPos.z = env->GetDoubleField(p, lastTickPosZField);

                        pd.pos.x = env->GetDoubleField(p, posXField);
                        pd.pos.y = env->GetDoubleField(p, posYField);
                        pd.pos.z = env->GetDoubleField(p, posZField);

                        pd.name = getPlayerName(env, p);

                        if (getHealthMethod && getMaxHealthMethod) {
                            pd.health = env->CallFloatMethod(p, getHealthMethod);
                            pd.maxHealth = env->CallFloatMethod(p, getMaxHealthMethod);

                            if (env->ExceptionCheck()) {
                                env->ExceptionClear();
                                pd.health = 20.0f;
                                pd.maxHealth = 20.0f;
                            }
                        } else {
                            pd.health = 20.0f;
                            pd.maxHealth = 20.0f;
                        }

                        pd.isTeammate = false;

                        if (localTeam && getTeamMethod && isSameTeamMethod) {
                            jobject targetTeam = env->CallObjectMethod(p, getTeamMethod);

                            if (env->ExceptionCheck()) {
                                env->ExceptionClear();
                                targetTeam = nullptr;
                            }

                            if (targetTeam) {
                                jboolean sameTeam = env->CallBooleanMethod(targetTeam, isSameTeamMethod, localTeam);

                                if (env->ExceptionCheck()) {
                                    env->ExceptionClear();
                                    sameTeam = JNI_FALSE;
                                }

                                pd.isTeammate = sameTeam == JNI_TRUE;

                                env->DeleteLocalRef(targetTeam);
                            }
                        }

                        pd.isValid = true;
                        players.push_back(pd);
                    }

                    if (p) {
                        env->DeleteLocalRef(p);
                    }
                }

                env->DeleteLocalRef(playersArray);
            }

            env->DeleteLocalRef(playersList);
        }

        if (localTeam) {
            env->DeleteLocalRef(localTeam);
        }

        if (!players.empty()) {
            glPushAttrib(GL_ALL_ATTRIB_BITS);

            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);

            float aspect = (float)viewport[2] / (float)viewport[3];

            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();

            float fov = 95.0f;
            float fH = tan(fov / 360.0f * 3.14159f) * 0.1f;
            float fW = fH * aspect;

            glFrustum(-fW, fW, -fH, fH, 0.1f, 1000.0f);

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();

            glRotatef(pitch, 1, 0, 0);
            glRotatef(yaw + 180.0f, 0, 1, 0);

            glDisable(GL_LIGHTING);
            glDisable(GL_CULL_FACE);

            glEnable(GL_MULTISAMPLE);

            glEnable(GL_ALPHA_TEST);
            glAlphaFunc(GL_GREATER, 0.05f);

            if (glUseProgramPtr) {
                glUseProgramPtr(0);
            }

            float rad = yaw * 3.14159f / 180.0f;
            float rightX = -cos(rad);
            float rightZ = -sin(rad);

            double currentTime = getAnimationTimeSeconds();
            float dt = (lastRenderTime > 0.0) ? (float)(currentTime - lastRenderTime) : 0.0f;
            lastRenderTime = currentTime;

            Setting* vinesSetting = getSetting("Vines");
            bool useVines = vinesSetting ? vinesSetting->getBool() : false;

            Setting* tracersSetting = getSetting("Tracers");
            bool useTracers = tracersSetting ? tracersSetting->getBool() : false;
            float tracerRadius = 35.0f;
            if (Setting* s = getSetting("Tracer Radius")) tracerRadius = s->value;

            for (const auto& p : players) {
                PlayerState& state = playerStates[p.name];
                if (state.displayedHealth < 0.0f) {
                    state.displayedHealth = p.health;
                }

                bool healthChanging = fabsf(state.displayedHealth - p.health) > 0.01f;
                if (healthChanging) {
                    // Smoothly transition health
                    state.displayedHealth += (p.health - state.displayedHealth) * 5.0f * dt;
                } else {
                    state.displayedHealth = p.health;
                }
                
                // Continuous animation
                state.animationTime += (double)dt;

                double interpX = p.lastTickPos.x + (p.pos.x - p.lastTickPos.x) * partialTicks;
                double interpY = p.lastTickPos.y + (p.pos.y - p.lastTickPos.y) * partialTicks;
                double interpZ = p.lastTickPos.z + (p.pos.z - p.lastTickPos.z) * partialTicks;

                float dx = (float)(interpX - camX);
                float dy = (float)(interpY - camY);
                float dz = (float)(interpZ - camZ);

                float distanceFromCamera = sqrtf(dx * dx + dy * dy + dz * dz);

                if (useTracers && distanceFromCamera <= tracerRadius) {
                    glDisable(GL_TEXTURE_2D);
                    glDisable(GL_DEPTH_TEST);
                    glDepthMask(GL_FALSE);
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glLineWidth(1.5f);
                    glColor4f(1.0f, 1.0f, 1.0f, 0.65f);

                    glBegin(GL_LINES);
                    glVertex3f(0, 0, 0);
                    glVertex3f(dx, dy + 1.0f, dz);
                    glEnd();

                    glEnable(GL_DEPTH_TEST);
                    glDepthMask(GL_TRUE);
                }

                glPushMatrix();
                glTranslatef(dx, dy, dz);

                float boxW = 0.35f;
                float boxH = 1.8f;

                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LEQUAL);
                glDepthMask(GL_TRUE);

                glDisable(GL_TEXTURE_2D);
                glDisable(GL_BLEND);

                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                drawPlayerDepthOccluder(0.31f, boxH);
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

                if (useVines && vineTextureLoaded && vineTexture != 0) {
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_LEQUAL);
                    glDepthMask(GL_FALSE);

                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, vineTexture);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

                    drawVineLoDHitbox(boxW, boxH, distanceFromCamera);

                    glDepthMask(GL_TRUE);
                } else {
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_LEQUAL);
                    glDepthMask(GL_FALSE);

                    drawStandardLineHitbox(boxW, boxH);

                    glDepthMask(GL_TRUE);
                }

                double animFps = (hudLayout.animFps > 0.0) ? (double)hudLayout.animFps : 10.0;
                int currentFrame = ((int)floor(state.animationTime * animFps)) % HUD_ANIM_FRAME_COUNT;
                if (currentFrame < 0) currentFrame += HUD_ANIM_FRAME_COUNT;

                bool frameOk = hudAnimationTexturesLoaded[currentFrame] && hudAnimationTextures[currentFrame] != 0;
                bool heartsOk = true;
                for (int hv = 0; hv < HUD_HEART_VARIANT_COUNT; hv++) {
                    if (!hudHeartTexturesLoaded[currentFrame][hv] || hudHeartTextures[currentFrame][hv] == 0) {
                        heartsOk = false;
                        break;
                    }
                }

                if (frameOk && heartsOk) {
                    glDisable(GL_DEPTH_TEST);
                    glDepthMask(GL_FALSE);

                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                    glEnable(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                    glColor4f(1.0f, 1.0f, 1.0f, 0.95f);

                    float scale = hudLayout.billboardScale;
                    float barH = hudLayout.frameH * scale;
                    float barW = hudLayout.frameW * scale;

                    float offset = boxW + hudLayout.sideOffset;
                    float barCX = rightX * offset;
                    float barCZ = rightZ * offset;

                    float frameBaseY = (boxH * 0.5f) - (barH * 0.5f) + hudLayout.frameYShift * barH;

                    float layerNudge = hudLayout.layerNudge;
                    barCX += rightX * layerNudge;
                    barCZ += rightZ * layerNudge;

                    glColor4f(1.0f, 1.0f, 1.0f, 0.50f); // 50% opaque frame
                    glBindTexture(GL_TEXTURE_2D, hudAnimationTextures[currentFrame]);
                    drawAnimatedHealthHudBillboard(
                        barCX,
                        frameBaseY,
                        barCZ,
                        barW,
                        barH,
                        rightX,
                        rightZ
                    );
                    glColor4f(1.0f, 1.0f, 1.0f, 0.95f); // Restore for hearts


                    float logicFw = hudLayout.frameW > 0.0f ? hudLayout.frameW : 120.0f;
                    float logicFh = hudLayout.frameH > 0.0f ? hudLayout.frameH : 379.0f;
                    float heartW = barW * (hudLayout.heartW / logicFw);
                    float heartH = barH * (hudLayout.heartH / logicFh);
                    float heartCx = barCX + rightX * (hudLayout.heartXOffset * barW);
                    float heartCz = barCZ + rightZ * (hudLayout.heartXOffset * barW);

                    float maxHp = p.maxHealth;
                    if (maxHp < 1.0f) {
                        maxHp = 20.0f;
                    }
                    float health = clampFloat(state.displayedHealth, 0.0f, maxHp);
                    float damage = maxHp - health;
                    int displayHearts = hudLayout.heartCount > 0 ? hudLayout.heartCount : 7;
                    float hpPerHeart = maxHp / (float)displayHearts;

                    for (int hi = 0; hi < displayHearts; hi++) {
                        float damageAmount = (damage - (float)hi * hpPerHeart) / hpPerHeart;
                        int variant = 0;
                        if (damageAmount > 0.0f && damageAmount < 1.0f) {
                            variant = 1;
                        } else if (damageAmount >= 1.0f) {
                            variant = 2;
                        }

                        float topY = frameBaseY + barH - barH * hudLayout.heartYStart - (float)hi * barH * hudLayout.heartSpacing;
                        float bottomY = topY - heartH;

                        glBindTexture(GL_TEXTURE_2D, hudHeartTextures[currentFrame][variant]);
                        drawAnimatedHealthHudBillboard(
                            heartCx,
                            bottomY,
                            heartCz,
                            heartW,
                            heartH,
                            rightX,
                            rightZ
                        );
                    }

                    glDepthMask(GL_TRUE);
                }

                glPopMatrix();
            }

            glMatrixMode(GL_PROJECTION);
            glPopMatrix();

            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();

            glPopAttrib();
        }
    }

    if (world) env->DeleteLocalRef(world);
    if (localPlayer) env->DeleteLocalRef(localPlayer);
    if (renderManager) env->DeleteLocalRef(renderManager);
    if (timer) env->DeleteLocalRef(timer);

    env->DeleteLocalRef(mc);
}