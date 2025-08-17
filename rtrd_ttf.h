#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define RTRD_FONT_RENDER_LIMIT 256

typedef struct rtrd_font
{

    int ftex;
    const char * fontloc;

// ASCII 32..126 is 95 glyphs
    stbtt_bakedchar cdata[96]; 
    uint32_t vbo;
    uint32_t vao;
    uint32_t program;
    int32_t mvploc;
    

}rtrd_font;

rtrd_font rtrd_default_font;

typedef struct rtrd_render_font_command
{
    struct 
    { 
        float x;
        float y;
        float u;
        float v;
    } 
    vdata[6];

}rtrd_render_font_command;

rtrd_render_font_command rtrd_render_buffer[RTRD_FONT_RENDER_LIMIT];

static const char* rtrd_v_shader= "#version 330\n"
"uniform mat4 MVP;"
"in vec2 vPos;"
"in vec2 UV;"
"out vec2 uv;"
"void main()"
"{"
"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);"
"    uv=UV;"
"}";

static const char* rtrd_f_shader = "#version 330\n"
"uniform sampler2D tex;"
"in vec2 uv;"
"out vec4 color;"
"void main()"
"{"
"    float r = texture(tex,uv).r;"
"    color = vec4(1.0,1.0,1.0, r);"
"}";

void rtrd_init_font_default(const char * fontloc)
{
   uint8_t * ttf_buffer = malloc(1<<20);
   rtrd_font font =  {} ;
   font.mvploc = -1;
   font.fontloc = fontloc;
   fread(ttf_buffer, 1, 1<<20, fopen(fontloc, "rb"));
   uint8_t * temp_bitmap = malloc(512*512);
   stbtt_BakeFontBitmap(ttf_buffer,0, 50.0, temp_bitmap,512,512, 32,96, font.cdata); // no guarantee this fits!
   // can free ttf_buffer at this point
   glGenTextures(1, &font.ftex);
   glBindTexture(GL_TEXTURE_2D, font.ftex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512,512, 0, GL_RED, GL_UNSIGNED_BYTE, temp_bitmap);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glGenBuffers(1, &font.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, font.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rtrd_render_buffer), rtrd_render_buffer, GL_DYNAMIC_DRAW);

    glGenVertexArrays(1, &font.vao);
    glBindVertexArray(font.vao);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 4, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 4, (void*) (sizeof(float) * 2) );


   free(ttf_buffer);
   free(temp_bitmap);

    const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &rtrd_v_shader, NULL);
    glCompileShader(vertex_shader);
    char log[1024];
    int32_t len;
    glGetShaderInfoLog(vertex_shader,sizeof(log),&len,log);


    const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &rtrd_f_shader, NULL);
    glCompileShader(fragment_shader);

    glGetShaderInfoLog(fragment_shader,sizeof(log),&len,log);

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

     font.mvploc = glGetUniformLocation(program, "MVP");

    font.program = program;

   rtrd_default_font = font; 
   
}

void rtrd_draw_text_default(const char * text,float x, float y , mat4x4 * mvp)
{
    char * basetext = text;

    uint32_t textcount = 0;
    while (*text) {
        if (*text >= 32 && *text < 128) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(rtrd_default_font.cdata, 512,512, *text-32, &x,&y,&q,1);//1=opengl & d3d10+,0=d3d9

            rtrd_render_font_command command = { 
                q.x0,q.y0, q.s0,q.t0,
                q.x1,q.y0,q.s1,q.t0,
                q.x1,q.y1,q.s1,q.t1,
                q.x1,q.y1,q.s1,q.t1,
                q.x0,q.y1,q.s0,q.t1,
                q.x0,q.y0, q.s0,q.t0
            };
            rtrd_render_buffer[textcount++] = command;

        }
        ++text;
    }
    uint32_t textbuffersize = textcount * sizeof(rtrd_render_font_command);
    glBindBuffer(GL_ARRAY_BUFFER,rtrd_default_font.vbo); 
    glBufferSubData(GL_ARRAY_BUFFER,0,textbuffersize ,rtrd_render_buffer); 

    glBindBuffer(GL_ARRAY_BUFFER,0); 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(rtrd_default_font.program);
    glUniformMatrix4fv(rtrd_default_font.mvploc, 1, GL_FALSE, (const GLfloat*) mvp);
    glBindTexture(GL_TEXTURE_2D, rtrd_default_font.ftex);
    glBindVertexArray(rtrd_default_font.vao);
    glDrawArrays(GL_TRIANGLES,0, textcount * (sizeof(rtrd_render_font_command)/sizeof(float)));
}

