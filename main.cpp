#include <stdio.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#include <vector>

#include <sys/types.h>  // stat()
#include <sys/stat.h>


#ifdef _WIN32 // getcwd()
#undef main
#include <direct.h>
#define getcwd _getcwd // stupid MSFT "deprecation" warning
#else
#include <unistd.h>
#endif

using namespace std;

const int W = 400, H = 400;

void error1(const char what[]) {
    printf("%s: %s\n",what ,SDL_GetError() );
    exit(1);
}
void error2(const char what[],const char quote[]) {
    printf("%s '%s': %s\n",what ,quote ,SDL_GetError() );
    exit(2);
}

// absolute path struct
const int BUFF_LEN = 1024;
struct AbsPath {
    char path[BUFF_LEN];
    int fname_len, suffix_len;
    struct stat buf; // to check if file exists
};
void AbsPath_init(AbsPath *ap) {
    getcwd(ap->path,BUFF_LEN);
    ap->fname_len = strlen(ap->path);
    ap->suffix_len = 0;
}
void AbsPath_reset(AbsPath *ap) {
    ap->path[ap->fname_len] = 0;
}
void AbsPath_setSuffix(AbsPath *ap,const char suffix[]) {
    if(ap->suffix_len) ap->fname_len -= ap->suffix_len;
    ap->path[ap->fname_len] = 0;
    ap->suffix_len = strlen(suffix);
    strcat(ap->path,suffix);
    ap->fname_len += ap->suffix_len;
}
bool AbsPath_cat(AbsPath *ap,const char name[],const char category[]) {
    AbsPath_reset(ap);
    strcat(ap->path,name);
    printf("Loading %s: %s", category, ap->path);
    return !stat(ap->path, &ap->buf);
}

const SDL_Color ClWhite = {255,255,255,0};
const SDL_Color ClBlack = {0,0,0,0};
const SDL_Color ClDarkBlue = {0,0,128,0};

// Texture struct
struct Tex {
    SDL_Rect pos;
    SDL_Texture *tex;
    SDL_Renderer* ren;
};
void Tex_destroy(Tex *te) {
    if(te->tex) SDL_DestroyTexture(te->tex); // ~4
    te->tex = 0;
}
void Tex_load_at(Tex *te,TTF_Font *font,const char str[],const SDL_Color &fg,SDL_Renderer *ren,int x,int y) {
    Tex_destroy(te);
    te->ren = ren;
    SDL_Surface *img = TTF_RenderText_Solid(font,str,fg); // ~4
    if(!img) error2("Failed to render text",str);
    te->tex = SDL_CreateTextureFromSurface(ren, img); // 3
    SDL_FreeSurface(img); // ~4
    if(!te->tex) error2("Failed to create texture from text",str);
    SDL_QueryTexture(te->tex,0,0,&te->pos.w,&te->pos.h);
    te->pos.x = x;
    te->pos.y = y;
}
void Tex_load(Tex *te,const char path[],SDL_Renderer *ren) {
    Tex_destroy(te);
    te->ren = ren;
    te->pos.x = 0;
    te->pos.y = 0;
    SDL_RWops *rwop = SDL_RWFromFile(path, "rb");
    SDL_Surface *img = IMG_LoadPNG_RW(rwop); // 4
    if(!img) error2("Failed to load",path);
    te->tex = SDL_CreateTextureFromSurface(ren, img); // 3
    SDL_FreeSurface(img); // ~4
    if(!te->tex) error2("Failed to create texture from",path);
    SDL_QueryTexture(te->tex,0,0,&te->pos.w,&te->pos.h);
}
void Tex_draw(Tex *te) {
    SDL_RenderCopy(te->ren, te->tex, 0, &te->pos);
}
void Tex_draw(Tex *te,SDL_Rect *s,SDL_Rect *d) {
    SDL_RenderCopy(te->ren, te->tex, s, d);
}
void Tex_set_xy(Tex *te,int x, int y) {
    te->pos.x = x;
    te->pos.y = y;
}
void SDL_Rect_set_xywh(SDL_Rect *re, int x, int y, int w, int h) {
    re->x = x;
    re->y = y;
    re->w = w;
    re->h = h;
}
SDL_Rect *Tex_pos(Tex *te) {
    return &te->pos;
}


// TextureManager struct
enum ETx { TxBird, TxBG, TxEnd, TxReady, TxGround, TxInstruct, TxBPipe, TxTPipe, TxTotal};
struct TextureManager {
    Tex tx[TxTotal];
    SDL_Renderer *ren;
};
void TextureManager_init(TextureManager *tm,AbsPath *folder, SDL_Window *win) {
    memset(tm,0,sizeof(TextureManager));
    tm->ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC); // 1
    if (!tm->ren) error1("SDL_CreateRenderer Error");
    int flag = IMG_INIT_PNG|IMG_INIT_JPG;
    if( (IMG_Init(flag) & flag) != flag ) // 2
        error1("Failed to init PNG and JPG support");
    static const char files[TxTotal][24] = { // urutan harus sama dengan enum I_Tex
                                             "avatar.png",
                                             "background.png",
                                             "gameover.png",
                                             "getready.png",
                                             "ground.png",
                                             "instructions.png",
                                             "obstacle_bottom.png",
                                             "obstacle_top.png"
                                           };
    AbsPath_setSuffix(folder,"/i/");
    for(int z=0;z<TxTotal;++z) {
        if(!AbsPath_cat(folder,files[z],"image"))
            error2("File does not exists",folder->path);
        Tex_load(&tm->tx[z],folder->path,tm->ren);
    }
}
void TextureManager_destroy(TextureManager *tm) {
    IMG_Quit(); // ~2
    SDL_DestroyRenderer(tm->ren); // ~1
}
void TextureManager_draw(TextureManager *tm,ETx idx) {
    Tex_draw(&tm->tx[idx]);
}
Tex *TextureManager_get(TextureManager *tm,ETx idx) {
    return &tm->tx[idx];
}
void TextureManager_begin_draw(TextureManager *tm) {
    SDL_RenderClear(tm->ren);
}
/*void display() {
  draw(TxBG);
  draw(TxBIRD);
}*/
void TextureManager_end_draw(TextureManager *tm) {
    SDL_RenderPresent(tm->ren);
}


// SoundManager struct
enum EAu { AuClick, AuCrash, AuDie, AuHit, AuPoint, AuWing, AuStart, AuWall, AuTotal};
struct SoundManager {
    Mix_Chunk *au[AuTotal];
};
void SoundManager_init(SoundManager *sm,AbsPath *folder) {
    int flag = MIX_INIT_OGG|MIX_INIT_MP3;
    if( (Mix_Init(flag) & flag) != flag ) // 1
        error1("Failed to init OGG and MP3 support");
    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1)
        error1("Failed to open audio");
    static const char files[AuTotal][24] = { // urutan harus sama dengan EAu
                                             "click.wav",
                                             "crash.wav",
                                             "sfx_die.wav",
                                             "sfx_hit.wav",
                                             "sfx_point.wav",
                                             "sfx_wing.wav",
                                             "start.wav",
                                             "wall.wav"
                                           };
    AbsPath_setSuffix(folder,"/a/");
    for(int z=0;z<AuTotal;++z) {
        if(!AbsPath_cat(folder,files[z],"audio"))
            error2("File does not exists",folder->path);
        Mix_Chunk *mus = sm->au[z] = Mix_LoadWAV(folder->path); // 2
        if(!mus) error2("Failed to load",folder->path);
    }
}
void SoundManager_destroy(SoundManager *sm) {
    for(int z=0;z<AuTotal;++z) Mix_FreeChunk(sm->au[z]); // ~2
    Mix_Quit(); // ~1
}
void SoundManager_play(SoundManager *sm,EAu idx,int count=0) {
    Mix_PlayChannel(-1,sm->au[idx],count);
}

// FontManager struct
enum EFt { FtArial, FtVerdana, FtSourceCodePro, FtTotal};
const size_t MAX_CACHE = 4096;
struct FontManager {
    TTF_Font *ft[FtTotal];
    Tex* tx[MAX_CACHE];
    size_t tx_count;
    SDL_Renderer *ren;
};
void FontManager_init(FontManager *fm,AbsPath *folder,SDL_Renderer *ren) {
    fm->tx_count = 0;
    fm->ren = ren;
    if( TTF_Init() ) // 1
        error1("Failed to init TTF support");
    static const char files[FtTotal][24] = { // urutan harus sama dengan EFt
                                             "arial.ttf",
                                             "verdana.ttf",
                                             "source_code_pro.ttf",
                                           };
    AbsPath_setSuffix(folder,"/f/");
    for(int z=0;z<FtTotal;++z) {
        if(!AbsPath_cat(folder,files[z],"font"))
            error2("File does not exists",folder->path);
        TTF_Font *fnt = fm->ft[z] = TTF_OpenFont(folder->path,18); // 2
        if(!fnt) error2("Failed to load",folder->path);
    }
}
void FontManager_destroy(FontManager *fm) {
    for(size_t z=0;z<fm->tx_count;++z) delete fm->tx[z]; // ~3
    for(int z=0;z<FtTotal;++z) TTF_CloseFont(fm->ft[z]); // ~2
    TTF_Quit(); // ~1
}
int FontManager_draw(FontManager *fm,EFt idx,const char* str,const SDL_Color &fg,int x,int y) {
    Tex *tex = new Tex();
    Tex_load_at(tex,fm->ft[idx],str,fg,fm->ren,x,y);
    if(fm->tx_count<MAX_CACHE) fm->tx[fm->tx_count++] = tex;
    else exit(99);
    return fm->tx_count-1;
}
void FontManager_draw_from_cache(FontManager *fm,int cache) {
    Tex_draw(fm->tx[cache]);
}

// enum game state
enum EGs { GsMENU, GsPLAY, GsEND, GsEXIT, GsTOTAL };

// Bird struct!
struct Bird {
    bool is_hidden;
    double vy, ay, g; // v=velocity, a=acceleration, g=gravity
    Tex *bird;
    SoundManager *sM;
    int point; // +1 for every pipe passed
};

void Bird_reset(Bird *b) {
    b->point = 0;
    b->is_hidden = false;
    b->vy = 0;
    b->ay = 0;
    b->g = 2;
    Tex_set_xy(b->bird,100,150);
}
void Bird_init(Bird *b,TextureManager *tm,SoundManager *sm) {
    b->bird = TextureManager_get(tm,TxBird);
    b->sM = sm;
    Bird_reset(b);
}
void Bird_jump(Bird *b) {
    b->vy = -10;
    SoundManager_play(b->sM,AuWing);
}
void Bird_stabilize(Bird *b) {
    if(b->bird->pos.y>150) Bird_jump(b);
}
void Bird_tick(Bird *b) {
    if(b->is_hidden) return;
    b->bird->pos.y += b->vy;
    b->vy += b->g;
    b->vy -= b->ay;
}
void Bird_draw(Bird *b) {
    if(b->is_hidden) return;
    Tex_draw(b->bird);
}
int Bird_bottom(const Bird *b) { return b->bird->pos.y+b->bird->pos.h; }
int Bird_left(const Bird *b) { return b->bird->pos.x; }
const SDL_Rect* Bird_rect(const Bird *b) {
    return &b->bird->pos;
}
void Bird_hide(Bird *b) { b->is_hidden = true; }
void Bird_show(Bird *b) { b->is_hidden = false; }

// Pipe struct
struct Pipe {
    bool is_hidden;
    int up, left;
    int speed, space;
    Tex *top, *btm;
    SDL_Rect st, dt, sb, db;
};
void Pipe_reset(Pipe *wa) {
    wa->up = 40 + rand() % 240;
    int h = wa->top->pos.h, w = wa->top->pos.w;
    SDL_Rect_set_xywh(&wa->st,0,h-wa->up,w,wa->up);
    SDL_Rect_set_xywh(&wa->dt,wa->left,0,w,wa->up);
    h = wa->btm->pos.h;
    w = wa->btm->pos.w;
    int hh = H-wa->up-wa->space;
    SDL_Rect_set_xywh(&wa->sb,0,0,w,hh);
    SDL_Rect_set_xywh(&wa->db,wa->left,wa->up+wa->space,w,hh);
}
void Pipe_init(Pipe *wa,TextureManager *tm,int speed,int left=W) {
    wa->left = left;
    wa->speed = speed ;
    wa->space = 80;
    wa->top = TextureManager_get(tm,TxTPipe);
    wa->btm = TextureManager_get(tm,TxBPipe);
    Pipe_reset(wa);
}
void Pipe_tick(Pipe *wa) {
    int x = (wa->dt.x += -wa->speed);
    if(x+wa->dt.w<0) {
        wa->left = 400;
        Pipe_reset(wa);
    }
    wa->db.x = x;
}
void Pipe_restart(Pipe *wa,int left) {
    wa->left = left;
    Pipe_reset(wa);
}
int Pipe_width(Pipe *wa) {
    return wa->top->pos.w;
}
void Pipe_draw(Pipe *wa) {
    Tex_draw(wa->top,&wa->st,&wa->dt);
    Tex_draw(wa->btm,&wa->sb,&wa->db);
}
bool Pipe_check_hit(Pipe *wa,Bird *b) {
    return SDL_HasIntersection(Bird_rect(b),&wa->dt) ||
            SDL_HasIntersection(Bird_rect(b),&wa->db);
}
bool Pipe_check_point(Pipe *wa,Bird *b) {
    int x = wa->dt.x, l = Bird_left(b);
    return (x>l && x<=l+wa->speed);
}
void Pipe_hide(Pipe *wa) { wa->is_hidden = true; }
void Pipe_show(Pipe *wa) { wa->is_hidden = false; }

// ScrollingBackground
struct ScrollingBackground {
    bool is_hidden;
    // background and ground rendered twice, because it's scrolling
    Tex *back;
    SDL_Rect bs1, bs2, bd1, bd2; // background size, background delta
    Tex *ground;
    SDL_Rect gs1, gs2, gd1, gd2; // ground size, ground delta
    int baseline; // ground position
    int speed; // progress speed
    SoundManager *sM;
    Pipe p1, p2, p3; // 1st pipe, 2nd pipe, 3rd pipe
    bool show_pipe;
};
void ScrollingBackground_reset(ScrollingBackground *sb) {
    sb->show_pipe = false;
    sb->is_hidden = false;
    SDL_Rect_set_xywh(&sb->bs1,0,0,W,H);
    SDL_Rect_set_xywh(&sb->bs2,0,0,W,H);
    SDL_Rect_set_xywh(&sb->bd1,0,0,W,H);
    SDL_Rect_set_xywh(&sb->bd2,W,0,W,H);
    int h = sb->baseline, z = H-h;
    SDL_Rect_set_xywh(&sb->gs1,0,0,W,h);
    SDL_Rect_set_xywh(&sb->gs2,0,0,W,h);
    SDL_Rect_set_xywh(&sb->gd1,0,z,W,h);
    SDL_Rect_set_xywh(&sb->gd2,W,z,W,h);
}
void ScrollingBackground_init(ScrollingBackground *sb,TextureManager *tm,SoundManager *sm) {
    sb->back = TextureManager_get(tm,TxBG);
    sb->ground = TextureManager_get(tm,TxGround);
    sb->baseline = sb->ground->pos.h;
    sb->speed = 3;
    sb->sM = sm;
    Pipe_init(&sb->p1,tm,sb->speed);
    Pipe_init(&sb->p2,tm,sb->speed);
    Pipe_init(&sb->p3,tm,sb->speed);
    ScrollingBackground_reset(sb);
}
void ScrollingBackground_play(ScrollingBackground *sb) {
    sb->show_pipe = true;
    Pipe_restart(&sb->p1,W);
    int w = W + Pipe_width(&sb->p2);
    Pipe_restart(&sb->p2,W+w/3);
    Pipe_restart(&sb->p3,W+w*2/3);
}
void ScrollingBackground_tick(ScrollingBackground *sb) {
    if(sb->is_hidden) return;
    sb->bs1.x += sb->speed;
    sb->bs1.x %= W;
    sb->bs1.w += (W-sb->speed);
    sb->bs1.w %= W;
    int w = sb->bs1.w;
    sb->bs2.w = W-w;

    sb->bd1.w = w;
    sb->bd2.x = w;
    sb->bd2.w = sb->bs2.w;

    sb->gs1.x = sb->bs1.x;
    sb->gs1.w = sb->bs1.w;
    sb->gs2.w = sb->bs2.w;

    sb->gd1.w = sb->bd1.w;
    sb->gd2.x = sb->bd2.x;
    sb->gd2.w = sb->bd2.w;

    if(sb->show_pipe) {
        Pipe_tick(&sb->p1);
        Pipe_tick(&sb->p2);
        Pipe_tick(&sb->p3);
    }
}
void ScrollingBackground_draw(ScrollingBackground *sb) {
    if(sb->is_hidden) return;
    Tex_draw(sb->back,&sb->bs1,&sb->bd1);
    Tex_draw(sb->back,&sb->bs2,&sb->bd2);
    if(sb->show_pipe) {
        Pipe_draw(&sb->p1);
        Pipe_draw(&sb->p2);
        Pipe_draw(&sb->p3);
    }
    Tex_draw(sb->ground,&sb->gs1,&sb->gd1);
    Tex_draw(sb->ground,&sb->gs2,&sb->gd2);
}
bool ScrollingBackground_check_hit(ScrollingBackground *sb,Bird *b) {
    if(Pipe_check_point(&sb->p1,b)
            || Pipe_check_point(&sb->p2,b)
            || Pipe_check_point(&sb->p3,b)) {
        SoundManager_play(sb->sM,AuPoint);
        b->point += 1;
    }
    if(b->bird->pos.y < 0
            || Bird_bottom(b)>=H-sb->baseline
            || Pipe_check_hit(&sb->p1,b)
            || Pipe_check_hit(&sb->p2,b)
            || Pipe_check_hit(&sb->p3,b)) {
        SoundManager_play(sb->sM,AuHit);
        return true;
    }
    return false;
}
void ScrollingBackground_hide(ScrollingBackground *sb) { sb->is_hidden = true; }
void ScrollingBackground_show(ScrollingBackground *sb) { sb->is_hidden = false; }

// FlappyGame struct
// tick = calculation
// draw = rendering
// input = check for user input
struct FlappyGame {
    // engine related:
    TextureManager *tM;
    SoundManager *sM;
    FontManager *fM;
    enum EFr { FrFPS = 20, FrRate = 1000/FrFPS };
    // game related:
    EGs state;
    ScrollingBackground *bg;
    Bird *bird;
    SDL_Window *win;
    AbsPath path;
    int end_score_text_cache;
};
void FlappyGame_init(FlappyGame *fg) {
    fg->state = GsMENU;
    puts( "Initializing SDL.." );
    if(SDL_Init(SDL_INIT_EVERYTHING)) error1("SDL_Init Error"); // 1
    fg->win = SDL_CreateWindow("Flappy Bird!", 100, 100, W, H, SDL_WINDOW_SHOWN); // 2
    if (!fg->win) error1("SDL_CreateWindow Error");
    fg->sM = (SoundManager*) malloc(sizeof(SoundManager));
    AbsPath_init(&fg->path);
    SoundManager_init(fg->sM,&fg->path); // 3
    fg->tM = (TextureManager*) malloc(sizeof(TextureManager));
    TextureManager_init(fg->tM,&fg->path,fg->win); // 4a
    fg->fM = (FontManager*) malloc(sizeof(FontManager));
    FontManager_init(fg->fM,&fg->path,fg->tM->ren); // 4b
    fg->bg = (ScrollingBackground*) malloc(sizeof(ScrollingBackground));
    ScrollingBackground_init(fg->bg,fg->tM,fg->sM); // 5
    fg->bird = (Bird*)malloc(sizeof(Bird));
    Bird_init(fg->bird,fg->tM,fg->sM); // 6
    Tex_set_xy(TextureManager_get(fg->tM,TxReady),160,120);
    Tex_set_xy(TextureManager_get(fg->tM,TxInstruct),120,200);
    Tex_set_xy(TextureManager_get(fg->tM,TxEnd),120,100);
}
void FlappyGame_destroy(FlappyGame *fg) {
    free(fg->bird); // ~6
    free(fg->fM); // ~4b
    delete fg->bg; // ~5
    free(fg->tM); // ~4a
    delete fg->sM; // ~3
    SDL_DestroyWindow(fg->win); // ~2
    SDL_Quit(); // ~1
    puts( "Cleaning SDL.." );
}
void FlappyGame_input_menu(FlappyGame *fg,SDL_Event& e) {
    switch(e.type) {
    case SDL_KEYDOWN:
        switch(e.key.keysym.scancode) {
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_RETURN2:
        case SDL_SCANCODE_KP_ENTER:
        case SDL_SCANCODE_SPACE:
            fg->state = GsPLAY;
            ScrollingBackground_play(fg->bg);
            SoundManager_play(fg->sM,AuStart);
            break;
           case SDL_SCANCODE_ESCAPE:
            exit(0);
        default: break;
        }
        break;
    }
}
void FlappyGame_input_play(FlappyGame *fg,SDL_Event& e) {
    switch(e.type) {
    case SDL_KEYDOWN: // SDL_KEYUP
        switch(e.key.keysym.scancode) {
        case SDL_SCANCODE_SPACE:
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_RETURN2:
        case SDL_SCANCODE_KP_ENTER:
        case SDL_SCANCODE_UP:
            Bird_jump(fg->bird);
        default: break;
        }
        // e.key.keysym.scancode; // http://wiki.libsdl.org/SDL_Scancode
        // e.key.keysym.sym; // http://wiki.libsdl.org/SDL_Keycode
        // e.key.keysym.mod; // http://wiki.libsdl.org/SDL_Keymod
        break;
    case SDL_MOUSEMOTION:
        // e.motion.x, e.motion.y
        break;
    case SDL_MOUSEBUTTONDOWN: // SDL_MOUSEBUTTONUP
        switch(e.button.button) {
        case SDL_BUTTON_LEFT:
            Bird_jump(fg->bird);
            break;
        }
        // SDL_BUTTON_MIDDLE SDL_BUTTON_RIGHT
    }
}
void FlappyGame_input_end(FlappyGame *fg,SDL_Event& e) {
    switch(e.type) {
    case SDL_KEYDOWN:
        switch(e.key.keysym.scancode) {
        case SDL_SCANCODE_ESCAPE:
            Bird_reset(fg->bird);
            ScrollingBackground_reset(fg->bg);
            fg->state = GsMENU;
        default: break;
        }
        break;
    }
}
void FlappyGame_check_input(FlappyGame *fg) {
    static SDL_Event e;
    while(SDL_PollEvent(&e)) {
        switch(e.type) {
        case SDL_QUIT:
            fg->state = GsEXIT;
            return;
        default:
            switch(fg->state) {
            case GsMENU: FlappyGame_input_menu(fg,e); break;
            case GsPLAY: FlappyGame_input_play(fg,e); break;
            case GsEND: FlappyGame_input_end(fg,e); break;
            default: return;
            }
        }
    }
}
void FlappyGame_tick_menu(FlappyGame *fg) {
    ScrollingBackground_tick(fg->bg);
    Bird_tick(fg->bird);
    Bird_stabilize(fg->bird);
}
void FlappyGame_tick_play(FlappyGame *fg) {
    ScrollingBackground_tick(fg->bg);
    Bird_tick(fg->bird);
    if(ScrollingBackground_check_hit(fg->bg,fg->bird)) {
        char end_score_text[256];
        sprintf(end_score_text,"Your score: %d",fg->bird->point);
        fg->end_score_text_cache = FontManager_draw(fg->fM,FtVerdana,end_score_text,ClDarkBlue,80,240);
        fg->state = GsEND;
    }
}
void FlappyGame_tick_end(FlappyGame *fg) {
}
void FlappyGame_tick(FlappyGame *fg) {
    switch(fg->state) {
    case GsMENU: FlappyGame_tick_menu(fg); break;
    case GsPLAY: FlappyGame_tick_play(fg); break;
    case GsEND: FlappyGame_tick_end(fg); break;
    default: break;
    }
}
void FlappyGame_draw_menu(FlappyGame *fg) {
    static int press = FontManager_draw(fg->fM,FtArial,"Press enter to start",ClBlack,100,300);
    static int info = FontManager_draw(fg->fM,FtArial,"Press space/enter/click to flap",ClBlack,100,320);
    TextureManager_draw(fg->tM,TxReady);
    TextureManager_draw(fg->tM,TxInstruct);
    FontManager_draw_from_cache(fg->fM,press);
    FontManager_draw_from_cache(fg->fM,info);
}
void FlappyGame_draw_play(FlappyGame *fg) {

}
void FlappyGame_draw_end(FlappyGame *fg) {
    static int press = FontManager_draw(fg->fM,FtVerdana,"Press escape to return to menu",ClDarkBlue,80,200);
    FontManager_draw_from_cache(fg->fM,press);
    TextureManager_draw(fg->tM,TxEnd);
    FontManager_draw_from_cache(fg->fM,fg->end_score_text_cache);
}
void FlappyGame_draw(FlappyGame *fg) {
    ScrollingBackground_draw(fg->bg);
    Bird_draw(fg->bird);
    switch(fg->state) {
    case GsMENU: FlappyGame_draw_menu(fg); break;
    case GsPLAY: FlappyGame_draw_play(fg); break;
    case GsEND: FlappyGame_draw_end(fg); break;
    default: break;
    }
}
int FlappyGame_play(FlappyGame *fg) {
    unsigned fr;
    while(true) {
        fr = SDL_GetTicks();
        FlappyGame_check_input(fg);
        if(fg->state==GsEXIT) return 0;
        FlappyGame_tick(fg);
        TextureManager_begin_draw(fg->tM);
        FlappyGame_draw(fg);
        TextureManager_end_draw(fg->tM);
        fr = SDL_GetTicks()-fr;
        if (fr<fg->FrRate) SDL_Delay(fg->FrRate-fr);
    }
}

int main() {
    FlappyGame g;
    FlappyGame_init(&g);
    FlappyGame_play(&g);
    FlappyGame_destroy(&g);
}
