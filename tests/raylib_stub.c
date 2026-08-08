/* Minimal raylib stand-in so the game logic can be exercised headless. */
#include "raylib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void BeginDrawing(void){} void EndDrawing(void){}
void BeginTextureMode(RenderTexture2D t){(void)t;} void EndTextureMode(void){}
void ClearBackground(Color c){(void)c;} void CloseWindow(void){}
void DrawRectangle(int a,int b,int c,int d,Color e){(void)a;(void)b;(void)c;(void)d;(void)e;}
void DrawTexture(Texture2D t,int x,int y,Color c){(void)t;(void)x;(void)y;(void)c;}
void DrawTexturePro(Texture2D t,Rectangle s,Rectangle d,Vector2 o,float r,Color c){(void)t;(void)s;(void)d;(void)o;(void)r;(void)c;}
void DrawTextureRec(Texture2D t,Rectangle s,Vector2 p,Color c){(void)t;(void)s;(void)p;(void)c;}
float GetFrameTime(void){return 1.0f/60.0f;}
int GetScreenHeight(void){return 720;} int GetScreenWidth(void){return 960;}
void InitWindow(int w,int h,const char*t){(void)w;(void)h;(void)t;}
bool IsKeyPressed(int k){(void)k;return false;}
RenderTexture2D LoadRenderTexture(int w,int h){(void)w;(void)h;RenderTexture2D r={0};return r;}
Texture2D LoadTextureFromImage(Image i){(void)i;Texture2D t={0};t.id=1;return t;}
void *MemAlloc(unsigned int n){return calloc(1,n);}
void MemFree(void*p){free(p);}
bool SaveFileData(const char *f, const void *d, int n);
void SetConfigFlags(unsigned int f){(void)f;} void SetExitKey(int k){(void)k;}
void SetTargetFPS(int f){(void)f;} void SetTextureFilter(Texture2D t,int f){(void)t;(void)f;}
void SetTraceLogLevel(int l){(void)l;} void ToggleFullscreen(void){}
void TraceLog(int l,const char*t,...){(void)l;(void)t;}

static char g_file[4096];
static int g_size = 0;
static bool g_has = false;
void StubClearSave(void);
void StubCorruptSave(void);
void UnloadImage(Image i){free(i.data);}
void UnloadRenderTexture(RenderTexture2D t){(void)t;}
void UnloadTexture(Texture2D t){(void)t;}
bool WindowShouldClose(void){return true;}
/* Real inflate is not needed: GfxLoadTexture handles a NULL decode by
   returning an id-0 texture, which is exactly the headless case. */
unsigned char *DecompressData(const unsigned char *d,int n,int *out){(void)d;(void)n;*out=0;return NULL;}

/* Saving writes into the same buffer LoadFileData reads back, so a test can
   save, then load, and check the round trip really survives. */
bool SaveFileData(const char *f, const void *d, int n)
{
    (void)f;
    if (n > (int)sizeof(g_file)) return false;
    memcpy(g_file, d, (size_t)n);
    g_size = n;
    g_has = true;
    return true;
}
void StubClearSave(void) { g_has = false; g_size = 0; }
void StubCorruptSave(void) { if (g_size > 5) g_file[4] ^= 0xFF; }

/* 1.4: the title screen checks for a save before offering LOAD. Backed by a
   real temp file so the load path is exercised rather than stubbed away. */
bool FileExists(const char *f) { (void)f; return g_has; }

unsigned char *LoadFileData(const char *f, int *size)
{
    (void)f;
    if (!g_has) { *size = 0; return NULL; }
    unsigned char *p = (unsigned char *)malloc((size_t)g_size);
    memcpy(p, g_file, (size_t)g_size);
    *size = g_size;
    return p;
}
void UnloadFileData(unsigned char *d) { free(d); }

/* 1.9: the hearth draws additively. Nothing is rasterised here, so these only
   have to exist. */
void BeginBlendMode(int mode) { (void)mode; }
void EndBlendMode(void) { }
