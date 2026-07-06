#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/* ======================== 常量 ======================== */
#define WINDOW_W     760
#define WINDOW_H     640
#define MAX_CONTACTS 100
#define MAX_NAME      50
#define MAX_GENDER    10
#define MAX_PHONE     20
#define MAX_ADDRESS  200
#define MAX_EMAIL     50
#define MAX_WORKPLACE 100
#define MAX_INPUT     256
#define DEFAULT_FILE  "通讯录.txt"

#define FONT_TITLE   28
#define FONT_BIG     20
#define FONT_NORMAL  18
#define FONT_SMALL   14

/* ======================== 颜色 ======================== */
static SDL_Color C_BG       = {245,245,248,255};
static SDL_Color C_WHITE    = {255,255,255,255};
static SDL_Color C_BLACK    = { 30, 30, 35,255};
static SDL_Color C_GRAY     = {128,128,135,255};
static SDL_Color C_GRAY_LT  = {200,200,210,255};
static SDL_Color C_DARK     = { 60, 60, 65,255};

static SDL_Color C_BLUE     = { 24, 95,165,255};
static SDL_Color C_BLUE_LT  = {230,241,251,255};

static SDL_Color C_GREEN    = { 15,110, 86,255};
static SDL_Color C_GREEN_LT = {225,245,238,255};

static SDL_Color C_RED      = {163, 45, 45,255};
static SDL_Color C_RED_LT   = {252,235,235,255};

static SDL_Color C_AMBER    = {133, 79, 11,255};
static SDL_Color C_AMBER_LT = {250,238,218,255};

static SDL_Color C_PURPLE   = { 83, 74,183,255};
static SDL_Color C_PURPLE_LT= {238,237,254,255};

/* ======================== 结构体 ======================== */
struct Contact {
    char name[MAX_NAME];
    char gender[MAX_GENDER];
    char phone[MAX_PHONE];
    char address[MAX_ADDRESS];
    char email[MAX_EMAIL];
    char workplace[MAX_WORKPLACE];
};

struct InputField {
    SDL_Rect rect;
    char     buffer[MAX_INPUT];
    bool     active;
};

/* ======================== 全局变量 ======================== */
static SDL_Window*   gWin     = NULL;
static SDL_Renderer* gRen     = NULL;
static TTF_Font*     gFontT   = NULL;  /* title  */
static TTF_Font*     gFontB   = NULL;  /* big    */
static TTF_Font*     gFontN   = NULL;  /* normal */
static TTF_Font*     gFontS   = NULL;  /* small  */

static struct Contact contacts[MAX_CONTACTS];
static int contactCount = 0;

/* 应用状态 */
typedef enum {
    ST_MENU, ST_ADD, ST_DELETE, ST_SEARCH, ST_MODIFY, ST_DISPLAY, ST_AUTHOR
} AppState;

static AppState curState = ST_MENU;

/* 状态消息 */
static char      gMsg[512] = "欢迎使用通讯录系统！请选择功能。";
static SDL_Color gMsgColor;

/* —— 各界面输入字段 —— */
/* 增加联系人 */
static struct InputField addFields[6];
static int activeAddField = -1;

/* 删除联系人 */
static struct InputField delField;

/* 查找联系人 */
static struct InputField schField;
static int  schMode = 0;        /* 0 未选, 1 姓名, 2 电话 */
static bool schFound = false;
static struct Contact schResult;

/* 修改联系人 */
static struct InputField modNameField;
static struct InputField modValueField;
static int  modIndex  = -1;     /* 找到的联系人下标，-1 未查找 */
static int  modItem   = -1;     /* 选中的修改项 0-5，-1 未选 */
static bool modValueMode = false; /* true = 正在输入新值 */

/* 显示全部 */
static int  displayScroll = 0;
static struct InputField fileNameField;

/* ======================== 函数声明 ======================== */
/* SDL */
bool initSDL();
void cleanupSDL();
TTF_Font* loadFont(int size);
void renderText(const char* txt, int x, int y, SDL_Color c, TTF_Font* f);
void renderTextCentered(const char* txt, int cx, int cy, SDL_Color c, TTF_Font* f);
int  textWidth(const char* txt, TTF_Font* f);
int  textHeight(TTF_Font* f);
void drawRectFilled(SDL_Rect r, SDL_Color c);
void drawRectOutline(SDL_Rect r, SDL_Color c);
void drawBtn(SDL_Rect r, const char* txt, SDL_Color fill, bool hovered);
void drawInputField(struct InputField* f);
bool ptInRect(int x, int y, SDL_Rect r);
SDL_Color lighten(SDL_Color c);
void utf8Backspace(char* s);
void setStatus(const char* msg, SDL_Color c);

/* 核心逻辑 */
bool validatePhone(const char* p);
bool validateEmail(const char* e);
int  findByName(const char* name);
int  findByPhone(const char* phone);
void saveToFile(const char* fn);

/* 状态切换 */
void changeState(AppState ns);
void tryAddContact();
void tryDeleteContact();
void trySearchContact();
void tryModifyFind();
void tryModifyValue();

/* 事件 */
void handleEvent(SDL_Event* e);
void handleMenu(SDL_Event* e);
void handleAdd(SDL_Event* e);
void handleDelete(SDL_Event* e);
void handleSearch(SDL_Event* e);
void handleModify(SDL_Event* e);
void handleDisplay(SDL_Event* e);

/* 渲染 */
void render();
void renderMenu();
void renderAdd();
void renderDelete();
void renderSearch();
void renderModify();
void renderDisplay();
void renderAuthor();
void renderStatusBar();

/* 辅助 */
const char* getFieldName(int i);

/* ======================== main ======================== */
int main()
{
    if (!initSDL()) {
        fprintf(stderr, "SDL 初始化失败！\n");
        return 1;
    }
    gMsgColor = C_BLUE;

    /* 初始化输入字段矩形 */
    int labelW = 100, fieldX = 120, fieldW = 500, fieldH = 32;
    for (int i = 0; i < 6; i++) {
        addFields[i].rect = (SDL_Rect){fieldX, 90 + i*44, fieldW, fieldH};
        addFields[i].buffer[0] = '\0';
        addFields[i].active = false;
    }
    delField.rect     = (SDL_Rect){120, 110, 500, 36};
    delField.buffer[0] = '\0';
    delField.active   = false;
    schField.rect     = (SDL_Rect){120, 160, 500, 36};
    schField.buffer[0] = '\0';
    schField.active   = false;
    modNameField.rect  = (SDL_Rect){120, 110, 500, 36};
    modNameField.buffer[0] = '\0';
    modNameField.active = false;
    modValueField.rect  = (SDL_Rect){160, 400, 460, 36};
    modValueField.buffer[0] = '\0';
    modValueField.active = false;
    fileNameField.rect = (SDL_Rect){160, 560, 300, 32};
    strcpy(fileNameField.buffer, DEFAULT_FILE);
    fileNameField.active = false;

    bool running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            else handleEvent(&e);
        }
        render();
        SDL_RenderPresent(gRen);
        SDL_Delay(16);
    }

    cleanupSDL();
    return 0;
}

/* ======================== SDL 初始化 ======================== */
TTF_Font* loadFont(int size)
{
    const char* paths[] = {
        "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/simsun.ttc",
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        TTF_Font* f = TTF_OpenFont(paths[i], size);
        if (f) return f;
    }
    return NULL;
}

bool initSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return false;
    }
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        return false;
    }
    gWin = SDL_CreateWindow("通讯录系统 - SDL2版",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
    if (!gWin) { fprintf(stderr, "Window: %s\n", SDL_GetError()); return false; }

    gRen = SDL_CreateRenderer(gWin, -1, SDL_RENDERER_ACCELERATED);
    if (!gRen) gRen = SDL_CreateRenderer(gWin, -1, SDL_RENDERER_SOFTWARE);
    if (!gRen) { fprintf(stderr, "Renderer: %s\n", SDL_GetError()); return false; }

    gFontT = loadFont(FONT_TITLE);
    gFontB = loadFont(FONT_BIG);
    gFontN = loadFont(FONT_NORMAL);
    gFontS = loadFont(FONT_SMALL);

    if (!gFontN) {
        fprintf(stderr, "无法加载字体！请确认系统字体路径。\n");
        return false;
    }
    return true;
}

void cleanupSDL()
{
    if (gFontT) TTF_CloseFont(gFontT);
    if (gFontB) TTF_CloseFont(gFontB);
    if (gFontN) TTF_CloseFont(gFontN);
    if (gFontS) TTF_CloseFont(gFontS);
    if (gRen)   SDL_DestroyRenderer(gRen);
    if (gWin)   SDL_DestroyWindow(gWin);
    TTF_Quit();
    SDL_Quit();
}

/* ======================== 文字渲染 ======================== */
void renderText(const char* txt, int x, int y, SDL_Color c, TTF_Font* f)
{
    if (!txt || !f || txt[0] == '\0') return;
    SDL_Surface* s = TTF_RenderUTF8_Blended(f, txt, c);
    if (!s) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(gRen, s);
    if (t) {
        SDL_Rect d = {x, y, s->w, s->h};
        SDL_RenderCopy(gRen, t, NULL, &d);
        SDL_DestroyTexture(t);
    }
    SDL_FreeSurface(s);
}

void renderTextCentered(const char* txt, int cx, int cy, SDL_Color c, TTF_Font* f)
{
    if (!txt || !f || txt[0] == '\0') return;
    SDL_Surface* s = TTF_RenderUTF8_Blended(f, txt, c);
    if (!s) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(gRen, s);
    if (t) {
        SDL_Rect d = {cx - s->w/2, cy - s->h/2, s->w, s->h};
        SDL_RenderCopy(gRen, t, NULL, &d);
        SDL_DestroyTexture(t);
    }
    SDL_FreeSurface(s);
}

int textWidth(const char* txt, TTF_Font* f)
{
    int w = 0;
    TTF_SizeUTF8(f, txt, &w, NULL);
    return w;
}

int textHeight(TTF_Font* f)
{
    return TTF_FontHeight(f);
}

/* ======================== 绘图辅助 ======================== */
void drawRectFilled(SDL_Rect r, SDL_Color c)
{
    SDL_SetRenderDrawColor(gRen, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(gRen, &r);
}

void drawRectOutline(SDL_Rect r, SDL_Color c)
{
    SDL_SetRenderDrawColor(gRen, c.r, c.g, c.b, c.a);
    SDL_RenderDrawRect(gRen, &r);
}

SDL_Color lighten(SDL_Color c)
{
    SDL_Color r;
    r.r = c.r + (255 - c.r) * 3 / 10;
    r.g = c.g + (255 - c.g) * 3 / 10;
    r.b = c.b + (255 - c.b) * 3 / 10;
    r.a = 255;
    return r;
}

void drawBtn(SDL_Rect r, const char* txt, SDL_Color fill, bool hovered)
{
    SDL_Color f = hovered ? lighten(fill) : fill;
    drawRectFilled(r, f);
    drawRectOutline(r, C_GRAY_LT);
    renderTextCentered(txt, r.x + r.w/2, r.y + r.h/2, C_WHITE, gFontN);
}

void drawInputField(struct InputField* fld)
{
    SDL_Color border = fld->active ? C_BLUE : C_GRAY_LT;
    SDL_Color bg     = fld->active ? C_BLUE_LT : C_WHITE;
    drawRectFilled(fld->rect, bg);
    drawRectOutline(fld->rect, border);

    /* 渲染已输入文字 */
    if (fld->buffer[0] != '\0') {
        renderText(fld->buffer, fld->rect.x + 8, fld->rect.y + (fld->rect.h - textHeight(gFontN))/2,
                  C_BLACK, gFontN);
    }
    /* 光标闪烁 */
    if (fld->active) {
        int blink = SDL_GetTicks() / 500;
        if (blink % 2 == 0) {
            int tx = fld->rect.x + 8 + textWidth(fld->buffer, gFontN);
            SDL_Rect cursor = {tx, fld->rect.y + 6, 2, fld->rect.h - 12};
            drawRectFilled(cursor, C_BLUE);
        }
    }
}

bool ptInRect(int x, int y, SDL_Rect r)
{
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

void utf8Backspace(char* s)
{
    int len = (int)strlen(s);
    if (len == 0) return;
    int i = len - 1;
    while (i > 0 && (s[i] & 0xC0) == 0x80) i--;
    s[i] = '\0';
}

void setStatus(const char* msg, SDL_Color c)
{
    strncpy(gMsg, msg, sizeof(gMsg) - 1);
    gMsg[sizeof(gMsg)-1] = '\0';
    gMsgColor = c;
}

const char* getFieldName(int i)
{
    switch (i) {
        case 0: return "姓名";
        case 1: return "性别";
        case 2: return "电话";
        case 3: return "地址";
        case 4: return "邮箱";
        case 5: return "单位";
        default: return "";
    }
}

/* ======================== 核心逻辑 ======================== */
bool validatePhone(const char* p)
{
    int len = (int)strlen(p);
    if (len != 11) return false;
    for (int i = 0; i < len; i++)
        if (p[i] < '0' || p[i] > '9') return false;
    return true;
}

bool validateEmail(const char* e)
{
    const char* at = strchr(e, '@');
    if (!at || at == e) return false;
    const char* dot = strchr(at, '.');
    if (!dot || dot == e + strlen(e) - 1) return false;
    return true;
}

int findByName(const char* name)
{
    for (int i = 0; i < contactCount; i++)
        if (strcmp(contacts[i].name, name) == 0) return i;
    return -1;
}

int findByPhone(const char* phone)
{
    for (int i = 0; i < contactCount; i++)
        if (strcmp(contacts[i].phone, phone) == 0) return i;
    return -1;
}

void saveToFile(const char* fn)
{
    FILE* fp = fopen(fn, "w");
    if (!fp) {
        setStatus("文件保存失败！无法创建文件。", C_RED);
        return;
    }
    fprintf(fp, "═════════════════════════════════════════════════\n");
    fprintf(fp, "             通  讯  录  记  录                 \n");
    fprintf(fp, "═════════════════════════════════════════════════\n\n");
    for (int i = 0; i < contactCount; i++) {
        fprintf(fp, "【第 %d 位联系人】\n", i + 1);
        fprintf(fp, "  姓名：%-20s性别：%s\n", contacts[i].name, contacts[i].gender);
        fprintf(fp, "  电话：%-20s邮箱：%s\n", contacts[i].phone, contacts[i].email);
        fprintf(fp, "  地址：%s\n", contacts[i].address);
        fprintf(fp, "  单位：%s\n", contacts[i].workplace);
        fprintf(fp, "───────────────────────────────────────────────\n\n");
    }
    fprintf(fp, "共计 %d 位联系人\n", contactCount);
    fclose(fp);
    char buf[400];
    snprintf(buf, sizeof(buf), "联系人信息已保存到文件「%s」。（共%d位）", fn, contactCount);
    setStatus(buf, C_GREEN);
}

/* ======================== 状态切换 ======================== */
void changeState(AppState ns)
{
    SDL_StopTextInput();
    /* 清理输入字段 */
    for (int i = 0; i < 6; i++) { addFields[i].active = false; addFields[i].buffer[0] = '\0'; }
    activeAddField = -1;
    delField.active = false; delField.buffer[0] = '\0';
    schField.active = false; schField.buffer[0] = '\0'; schMode = 0; schFound = false;
    modNameField.active = false; modNameField.buffer[0] = '\0';
    modValueField.active = false; modValueField.buffer[0] = '\0';
    modIndex = -1; modItem = -1; modValueMode = false;
    displayScroll = 0;
    fileNameField.active = false;
    if (ns == ST_DISPLAY) strcpy(fileNameField.buffer, DEFAULT_FILE);
    curState = ns;
}

/* ======================== 业务操作 ======================== */
void tryAddContact()
{
    if (contactCount >= MAX_CONTACTS) {
        setStatus("通讯录已满，无法继续添加！", C_RED);
        return;
    }
    for (int i = 0; i < 6; i++) {
        if (addFields[i].buffer[0] == '\0') {
            setStatus("请填写所有字段后再添加！", C_RED);
            return;
        }
    }
    if (!validatePhone(addFields[2].buffer)) {
        setStatus("电话格式错误！必须为11位纯数字。", C_RED);
        return;
    }
    if (!validateEmail(addFields[4].buffer)) {
        setStatus("邮箱格式错误！必须含 '@' 和 '.'。", C_RED);
        return;
    }
    strcpy(contacts[contactCount].name,      addFields[0].buffer);
    strcpy(contacts[contactCount].gender,    addFields[1].buffer);
    strcpy(contacts[contactCount].phone,      addFields[2].buffer);
    strcpy(contacts[contactCount].address,   addFields[3].buffer);
    strcpy(contacts[contactCount].email,      addFields[4].buffer);
    strcpy(contacts[contactCount].workplace,  addFields[5].buffer);
    contactCount++;

    char buf[256];
    snprintf(buf, sizeof(buf), "联系人「%s」添加成功！当前共%d位。", contacts[contactCount-1].name, contactCount);
    setStatus(buf, C_GREEN);
    changeState(ST_MENU);
}

void tryDeleteContact()
{
    if (delField.buffer[0] == '\0') {
        setStatus("请输入要删除的联系人姓名！", C_RED);
        return;
    }
    int idx = findByName(delField.buffer);
    if (idx < 0) {
        setStatus("该联系人不存在！", C_RED);
        return;
    }
    char name[MAX_NAME];
    strcpy(name, contacts[idx].name);
    for (int i = idx; i < contactCount - 1; i++)
        contacts[i] = contacts[i + 1];
    contactCount--;
    char buf[256];
    snprintf(buf, sizeof(buf), "联系人「%s」已删除！当前共%d位。", name, contactCount);
    setStatus(buf, C_GREEN);
    changeState(ST_MENU);
}

void trySearchContact()
{
    if (schMode == 0) {
        setStatus("请先选择查找方式（按姓名或按电话）！", C_RED);
        return;
    }
    if (schField.buffer[0] == '\0') {
        setStatus("请输入查找内容！", C_RED);
        return;
    }
    int idx = (schMode == 1) ? findByName(schField.buffer) : findByPhone(schField.buffer);
    if (idx < 0) {
        schFound = false;
        setStatus("该联系人不存在！", C_RED);
    } else {
        schFound = true;
        schResult = contacts[idx];
        char buf[256];
        snprintf(buf, sizeof(buf), "找到联系人「%s」！", schResult.name);
        setStatus(buf, C_GREEN);
    }
}

void tryModifyFind()
{
    if (modNameField.buffer[0] == '\0') {
        setStatus("请输入待修改的联系人姓名！", C_RED);
        return;
    }
    int idx = findByName(modNameField.buffer);
    if (idx < 0) {
        setStatus("该联系人不存在！", C_RED);
        modIndex = -1;
        return;
    }
    modIndex = idx;
    modNameField.active = false;
    SDL_StopTextInput();
    setStatus("找到联系人！请选择要修改的项目。", C_BLUE);
}

void tryModifyValue()
{
    if (modValueField.buffer[0] == '\0') {
        setStatus("请输入新值！", C_RED);
        return;
    }
    /* 电话和邮箱需要校验 */
    if (modItem == 2 && !validatePhone(modValueField.buffer)) {
        setStatus("电话格式错误！必须为11位纯数字。", C_RED);
        return;
    }
    if (modItem == 4 && !validateEmail(modValueField.buffer)) {
        setStatus("邮箱格式错误！必须含 '@' 和 '.'。", C_RED);
        return;
    }
    switch (modItem) {
        case 0: strcpy(contacts[modIndex].name,     modValueField.buffer); break;
        case 1: strcpy(contacts[modIndex].gender,   modValueField.buffer); break;
        case 2: strcpy(contacts[modIndex].phone,    modValueField.buffer); break;
        case 3: strcpy(contacts[modIndex].address,  modValueField.buffer); break;
        case 4: strcpy(contacts[modIndex].email,     modValueField.buffer); break;
        case 5: strcpy(contacts[modIndex].workplace,modValueField.buffer); break;
    }
    char buf[256];
    snprintf(buf, sizeof(buf), "%s 修改成功！", getFieldName(modItem));
    setStatus(buf, C_GREEN);
    modValueField.buffer[0] = '\0';
    modValueField.active = false;
    modValueMode = false;
    modItem = -1;
    SDL_StopTextInput();
}

/* ======================== 事件分发 ======================== */
void handleEvent(SDL_Event* e)
{
    switch (curState) {
        case ST_MENU:    handleMenu(e);    break;
        case ST_ADD:     handleAdd(e);     break;
        case ST_DELETE:  handleDelete(e);  break;
        case ST_SEARCH:  handleSearch(e); break;
        case ST_MODIFY:  handleModify(e); break;
        case ST_DISPLAY: handleDisplay(e); break;
        case ST_AUTHOR: {
            /* 处理返回按钮点击 */
            if (e->type == SDL_MOUSEBUTTONDOWN) {
                int mx = e->button.x, my = e->button.y;
                SDL_Rect backBtn = {280, 500, 200, 48};
                if (ptInRect(mx, my, backBtn)) {
                    changeState(ST_MENU);
                }
            }
            break;
        }
        default: break;
    }
    /* ESC 返回菜单 */
    if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_ESCAPE) {
        if (curState != ST_MENU) changeState(ST_MENU);
    }
}

/* —— 菜单事件 —— */
void handleMenu(SDL_Event* e)
{
    if (e->type != SDL_MOUSEBUTTONDOWN) return;
    int mx = e->button.x, my = e->button.y;

    /* 6 个功能按钮 + 1 退出按钮 */
    SDL_Rect btns[7];
    int bw = 200, bh = 56;
    for (int i = 0; i < 3; i++) btns[i] = (SDL_Rect){80 + i*220, 130, bw, bh};
    for (int i = 0; i < 3; i++) btns[i+3] = (SDL_Rect){80 + i*220, 210, bw, bh};
    btns[6] = (SDL_Rect){280, 300, bw, bh};

    for (int i = 0; i < 7; i++) {
        if (ptInRect(mx, my, btns[i])) {
            if (i == 6) { /* 退出 */
                SDL_Event quit; quit.type = SDL_QUIT;
                SDL_PushEvent(&quit);
            } else {
                changeState((AppState)(i + 1));
            }
            return;
        }
    }
}

/* —— 增加联系人事件 —— */
void handleAdd(SDL_Event* e)
{
    if (e->type == SDL_MOUSEBUTTONDOWN) {
        int mx = e->button.x, my = e->button.y;
        /* 检查输入字段 */
        for (int i = 0; i < 6; i++) {
            if (ptInRect(mx, my, addFields[i].rect)) {
                for (int j = 0; j < 6; j++) addFields[j].active = false;
                addFields[i].active = true;
                activeAddField = i;
                SDL_StartTextInput();
                return;
            }
        }
        /* 确认按钮 */
        if (ptInRect(mx, my, (SDL_Rect){200, 360, 160, 48})) {
            tryAddContact();
            return;
        }
        /* 返回按钮 */
        if (ptInRect(mx, my, (SDL_Rect){400, 360, 160, 48})) {
            changeState(ST_MENU);
            return;
        }
        /* 点击空白处取消选中 */
        for (int j = 0; j < 6; j++) addFields[j].active = false;
        activeAddField = -1;
        SDL_StopTextInput();
    }
    else if (e->type == SDL_TEXTINPUT && activeAddField >= 0) {
        struct InputField* f = &addFields[activeAddField];
        int len = (int)strlen(f->buffer), add = (int)strlen(e->text.text);
        if (len + add < MAX_INPUT - 1) strcat(f->buffer, e->text.text);
    }
    else if (e->type == SDL_KEYDOWN && activeAddField >= 0) {
        if (e->key.keysym.sym == SDLK_BACKSPACE)
            utf8Backspace(addFields[activeAddField].buffer);
        else if (e->key.keysym.sym == SDLK_RETURN) {
            if (activeAddField < 5) {
                addFields[activeAddField].active = false;
                activeAddField++;
                addFields[activeAddField].active = true;
            } else {
                tryAddContact();
            }
        }
        else if (e->key.keysym.sym == SDLK_TAB) {
            addFields[activeAddField].active = false;
            activeAddField = (activeAddField + 1) % 6;
            addFields[activeAddField].active = true;
        }
    }
}

/* —— 删除联系人事件 —— */
void handleDelete(SDL_Event* e)
{
    if (e->type == SDL_MOUSEBUTTONDOWN) {
        int mx = e->button.x, my = e->button.y;
        if (ptInRect(mx, my, delField.rect)) {
            delField.active = true;
            SDL_StartTextInput();
            return;
        }
        if (ptInRect(mx, my, (SDL_Rect){200, 180, 160, 48})) { tryDeleteContact(); return; }
        if (ptInRect(mx, my, (SDL_Rect){400, 180, 160, 48})) { changeState(ST_MENU); return; }
        delField.active = false;
        SDL_StopTextInput();
    }
    else if (e->type == SDL_TEXTINPUT && delField.active) {
        int len = (int)strlen(delField.buffer), add = (int)strlen(e->text.text);
        if (len + add < MAX_INPUT - 1) strcat(delField.buffer, e->text.text);
    }
    else if (e->type == SDL_KEYDOWN && delField.active) {
        if (e->key.keysym.sym == SDLK_BACKSPACE) utf8Backspace(delField.buffer);
        else if (e->key.keysym.sym == SDLK_RETURN) tryDeleteContact();
    }
}

/* —— 查找联系人事件 —— */
void handleSearch(SDL_Event* e)
{
    if (e->type == SDL_MOUSEBUTTONDOWN) {
        int mx = e->button.x, my = e->button.y;
        /* 模式按钮 */
        if (ptInRect(mx, my, (SDL_Rect){200, 90, 160, 40})) {
            schMode = 1; schField.buffer[0] = '\0'; schFound = false;
            setStatus("已选择按姓名查找，请输入姓名。", C_BLUE);
            return;
        }
        if (ptInRect(mx, my, (SDL_Rect){400, 90, 160, 40})) {
            schMode = 2; schField.buffer[0] = '\0'; schFound = false;
            setStatus("已选择按电话查找，请输入电话。", C_BLUE);
            return;
        }
        if (ptInRect(mx, my, schField.rect)) {
            schField.active = true; SDL_StartTextInput(); return;
        }
        if (ptInRect(mx, my, (SDL_Rect){200, 210, 160, 48})) { trySearchContact(); return; }
        if (ptInRect(mx, my, (SDL_Rect){400, 210, 160, 48})) { changeState(ST_MENU); return; }
        schField.active = false; SDL_StopTextInput();
    }
    else if (e->type == SDL_TEXTINPUT && schField.active) {
        int len = (int)strlen(schField.buffer), add = (int)strlen(e->text.text);
        if (len + add < MAX_INPUT - 1) strcat(schField.buffer, e->text.text);
    }
    else if (e->type == SDL_KEYDOWN && schField.active) {
        if (e->key.keysym.sym == SDLK_BACKSPACE) utf8Backspace(schField.buffer);
        else if (e->key.keysym.sym == SDLK_RETURN) trySearchContact();
    }
}

/* —— 修改联系人事件 —— */
void handleModify(SDL_Event* e)
{
    /* 阶段1：输入姓名查找 */
    if (modIndex < 0) {
        if (e->type == SDL_MOUSEBUTTONDOWN) {
            int mx = e->button.x, my = e->button.y;
            if (ptInRect(mx, my, modNameField.rect)) {
                modNameField.active = true; SDL_StartTextInput(); return;
            }
            if (ptInRect(mx, my, (SDL_Rect){200, 170, 160, 48})) { tryModifyFind(); return; }
            if (ptInRect(mx, my, (SDL_Rect){400, 170, 160, 48})) { changeState(ST_MENU); return; }
            modNameField.active = false; SDL_StopTextInput();
        }
        else if (e->type == SDL_TEXTINPUT && modNameField.active) {
            int len = (int)strlen(modNameField.buffer), add = (int)strlen(e->text.text);
            if (len + add < MAX_INPUT - 1) strcat(modNameField.buffer, e->text.text);
        }
        else if (e->type == SDL_KEYDOWN && modNameField.active) {
            if (e->key.keysym.sym == SDLK_BACKSPACE) utf8Backspace(modNameField.buffer);
            else if (e->key.keysym.sym == SDLK_RETURN) tryModifyFind();
        }
        return;
    }

    /* 阶段2：选择修改项 或 输入新值 */
    if (!modValueMode) {
        /* 选择修改项 */
        if (e->type == SDL_MOUSEBUTTONDOWN) {
            int mx = e->button.x, my = e->button.y;
            /* 6 个字段按钮（3列2行） */
            for (int i = 0; i < 6; i++) {
                int col = i % 3, row = i / 3;
                SDL_Rect r = {80 + col*220, 300 + row*50, 200, 40};
                if (ptInRect(mx, my, r)) {
                    modItem = i;
                    modValueMode = true;
                    modValueField.buffer[0] = '\0';
                    modValueField.active = true;
                    SDL_StartTextInput();
                    char buf[128];
                    snprintf(buf, sizeof(buf), "请输入新的%s：", getFieldName(i));
                    setStatus(buf, C_BLUE);
                    return;
                }
            }
            /* 完成按钮 */
            if (ptInRect(mx, my, (SDL_Rect){280, 460, 200, 48})) {
                setStatus("修改完成！返回菜单。", C_GREEN);
                changeState(ST_MENU);
                return;
            }
        }
    } else {
        /* 输入新值 */
        if (e->type == SDL_MOUSEBUTTONDOWN) {
            int mx = e->button.x, my = e->button.y;
            if (ptInRect(mx, my, modValueField.rect)) {
                modValueField.active = true; SDL_StartTextInput(); return;
            }
            if (ptInRect(mx, my, (SDL_Rect){200, 460, 160, 48})) { tryModifyValue(); return; }
            if (ptInRect(mx, my, (SDL_Rect){400, 460, 160, 48})) {
                modValueMode = false; modItem = -1;
                modValueField.active = false; modValueField.buffer[0] = '\0';
                SDL_StopTextInput();
                setStatus("已取消修改该项。", C_GRAY);
                return;
            }
            modValueField.active = false; SDL_StopTextInput();
        }
        else if (e->type == SDL_TEXTINPUT && modValueField.active) {
            int len = (int)strlen(modValueField.buffer), add = (int)strlen(e->text.text);
            if (len + add < MAX_INPUT - 1) strcat(modValueField.buffer, e->text.text);
        }
        else if (e->type == SDL_KEYDOWN && modValueField.active) {
            if (e->key.keysym.sym == SDLK_BACKSPACE) utf8Backspace(modValueField.buffer);
            else if (e->key.keysym.sym == SDLK_RETURN) tryModifyValue();
        }
    }
}

/* —— 显示全部事件 —— */
void handleDisplay(SDL_Event* e)
{
    if (e->type == SDL_MOUSEBUTTONDOWN) {
        int mx = e->button.x, my = e->button.y;
        /* 文件名输入框 */
        if (ptInRect(mx, my, fileNameField.rect)) {
            fileNameField.active = true; SDL_StartTextInput(); return;
        }
        /* 保存按钮 */
        if (ptInRect(mx, my, (SDL_Rect){480, 560, 100, 32})) {
            saveToFile(fileNameField.buffer);
            return;
        }
        /* 返回按钮 */
        if (ptInRect(mx, my, (SDL_Rect){610, 560, 100, 32})) {
            changeState(ST_MENU); return;
        }
        fileNameField.active = false; SDL_StopTextInput();
    }
    else if (e->type == SDL_MOUSEWHEEL) {
        int maxScroll = contactCount * 80 - 440;
        if (maxScroll < 0) maxScroll = 0;
        displayScroll -= e->wheel.y * 40;
        if (displayScroll < 0) displayScroll = 0;
        if (displayScroll > maxScroll) displayScroll = maxScroll;
    }
    else if (e->type == SDL_KEYDOWN) {
        if (fileNameField.active) {
            if (e->key.keysym.sym == SDLK_BACKSPACE) utf8Backspace(fileNameField.buffer);
            else if (e->key.keysym.sym == SDLK_RETURN) { saveToFile(fileNameField.buffer); }
            return;
        }
        int maxScroll = contactCount * 80 - 440;
        if (maxScroll < 0) maxScroll = 0;
        if (e->key.keysym.sym == SDLK_UP || e->key.keysym.sym == SDLK_PAGEUP) {
            displayScroll -= 80; if (displayScroll < 0) displayScroll = 0;
        }
        else if (e->key.keysym.sym == SDLK_DOWN || e->key.keysym.sym == SDLK_PAGEDOWN) {
            displayScroll += 80; if (displayScroll > maxScroll) displayScroll = maxScroll;
        }
    }
    else if (e->type == SDL_TEXTINPUT && fileNameField.active) {
        int len = (int)strlen(fileNameField.buffer), add = (int)strlen(e->text.text);
        if (len + add < MAX_INPUT - 1) strcat(fileNameField.buffer, e->text.text);
    }
}

/* ======================== 渲染 ======================== */
void render()
{
    /* 背景清屏 */
    SDL_SetRenderDrawColor(gRen, C_BG.r, C_BG.g, C_BG.b, C_BG.a);
    SDL_RenderClear(gRen);

    switch (curState) {
        case ST_MENU:    renderMenu();    break;
        case ST_ADD:     renderAdd();     break;
        case ST_DELETE:  renderDelete();  break;
        case ST_SEARCH:  renderSearch(); break;
        case ST_MODIFY:  renderModify(); break;
        case ST_DISPLAY: renderDisplay(); break;
        case ST_AUTHOR:  renderAuthor();  break;
    }
    renderStatusBar();
}

void renderStatusBar()
{
    /* 底部状态栏 */
    SDL_Rect bar = {0, WINDOW_H - 50, WINDOW_W, 50};
    drawRectFilled(bar, C_WHITE);
    drawRectOutline(bar, C_GRAY_LT);
    renderText(gMsg, 20, WINDOW_H - 40, gMsgColor, gFontN);
}

void renderMenu()
{
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    /* 标题 */
    renderTextCentered("通讯录系统", WINDOW_W/2, 45, C_BLACK, gFontT);
    renderTextCentered("结构体数组 · SDL2 图形界面", WINDOW_W/2, 80, C_GRAY, gFontS);

    /* 6 个功能按钮 */
    const char* texts[7] = {
        "1. 增加联系人", "2. 删除联系人", "3. 查找联系人",
        "4. 修改联系人", "5. 输出全部联系人", "6. 查看作者信息",
        "0. 退出系统"
    };
    SDL_Color colors[7] = {C_GREEN, C_RED, C_BLUE, C_PURPLE, C_AMBER, C_GRAY, C_DARK};
    SDL_Rect btns[7];
    for (int i = 0; i < 3; i++) {
        btns[i]   = (SDL_Rect){80 + i*220, 130, 200, 56};
        btns[i+3] = (SDL_Rect){80 + i*220, 210, 200, 56};
    }
    btns[6] = (SDL_Rect){280, 300, 200, 56};

    for (int i = 0; i < 7; i++) {
        bool hov = ptInRect(mx, my, btns[i]);
        drawBtn(btns[i], texts[i], colors[i], hov);
    }

    /* 联系人计数 */
    char buf[64];
    snprintf(buf, sizeof(buf), "当前联系人数量：%d / %d", contactCount, MAX_CONTACTS);
    renderTextCentered(buf, WINDOW_W/2, 390, C_GRAY, gFontN);

    /* 提示 */
    renderTextCentered("点击按钮选择功能 · ESC 返回菜单", WINDOW_W/2, 420, C_GRAY, gFontS);
    renderTextCentered("通讯录系统 SDL2 版", WINDOW_W/2, 450, C_GRAY, gFontS);
}

void renderAdd()
{
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    renderTextCentered("增加联系人信息", WINDOW_W/2, 35, C_BLACK, gFontB);

    const char* labels[6] = {"姓名：", "性别：", "电话：", "地址：", "邮箱：", "单位："};
    const char* hints[6]  = {"", "", "（11位手机号）", "", "（含@和.）", ""};

    for (int i = 0; i < 6; i++) {
        /* 标签 */
        renderText(labels[i], 30, addFields[i].rect.y + 8, C_BLACK, gFontN);
        /* 输入框 */
        drawInputField(&addFields[i]);
        /* 提示 */
        if (hints[i][0])
            renderText(hints[i], addFields[i].rect.x + addFields[i].rect.w + 8,
                       addFields[i].rect.y + 8, C_GRAY, gFontS);
    }

    /* 按钮 */
    SDL_Rect confirmBtn = {200, 360, 160, 48};
    SDL_Rect backBtn    = {400, 360, 160, 48};
    drawBtn(confirmBtn, "确认添加", C_GREEN, ptInRect(mx, my, confirmBtn));
    drawBtn(backBtn, "返回菜单", C_GRAY, ptInRect(mx, my, backBtn));

    /* 操作提示 */
    renderText("点击输入框激活 · Tab/Enter 切换字段 · ESC 返回", 30, 420, C_GRAY, gFontS);
}

void renderDelete()
{
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    renderTextCentered("删除联系人信息", WINDOW_W/2, 35, C_BLACK, gFontB);
    renderText("请输入要删除的联系人姓名：", 30, 80, C_BLACK, gFontN);
    drawInputField(&delField);

    SDL_Rect confirmBtn = {200, 180, 160, 48};
    SDL_Rect backBtn    = {400, 180, 160, 48};
    drawBtn(confirmBtn, "确认删除", C_RED, ptInRect(mx, my, confirmBtn));
    drawBtn(backBtn, "返回菜单", C_GRAY, ptInRect(mx, my, backBtn));

    renderText("输入姓名后点击确认删除 · ESC 返回", 30, 250, C_GRAY, gFontS);
}

void renderSearch()
{
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    renderTextCentered("查找联系人信息", WINDOW_W/2, 35, C_BLACK, gFontB);

    /* 模式按钮 */
    SDL_Rect mode1 = {200, 90, 160, 40};
    SDL_Rect mode2 = {400, 90, 160, 40};
    drawBtn(mode1, "按姓名查找", schMode == 1 ? C_BLUE : C_GRAY, ptInRect(mx, my, mode1));
    drawBtn(mode2, "按电话查找", schMode == 2 ? C_BLUE : C_GRAY, ptInRect(mx, my, mode2));

    const char* label = schMode == 1 ? "请输入姓名：" : schMode == 2 ? "请输入电话：" : "请先选择查找方式：";
    renderText(label, 30, 140, C_BLACK, gFontN);
    drawInputField(&schField);

    SDL_Rect searchBtn = {200, 210, 160, 48};
    SDL_Rect backBtn   = {400, 210, 160, 48};
    drawBtn(searchBtn, "查找", C_BLUE, ptInRect(mx, my, searchBtn));
    drawBtn(backBtn, "返回菜单", C_GRAY, ptInRect(mx, my, backBtn));

    /* 显示查找结果 */
    if (schFound) {
        int y = 280;
        renderText("查找结果：", 30, y, C_GREEN, gFontN);
        y += 28;
        char line[256];
        snprintf(line, sizeof(line), "姓名：%s    性别：%s", schResult.name, schResult.gender);
        renderText(line, 50, y, C_BLACK, gFontN); y += 24;
        snprintf(line, sizeof(line), "电话：%s    邮箱：%s", schResult.phone, schResult.email);
        renderText(line, 50, y, C_BLACK, gFontN); y += 24;
        snprintf(line, sizeof(line), "地址：%s", schResult.address);
        renderText(line, 50, y, C_BLACK, gFontN); y += 24;
        snprintf(line, sizeof(line), "单位：%s", schResult.workplace);
        renderText(line, 50, y, C_BLACK, gFontN);
    }
}

void renderModify()
{
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    renderTextCentered("修改联系人信息", WINDOW_W/2, 35, C_BLACK, gFontB);

    /* 阶段1：输入姓名 */
    if (modIndex < 0) {
        renderText("请输入待修改的联系人姓名：", 30, 80, C_BLACK, gFontN);
        drawInputField(&modNameField);

        SDL_Rect findBtn = {200, 170, 160, 48};
        SDL_Rect backBtn = {400, 170, 160, 48};
        drawBtn(findBtn, "查找联系人", C_BLUE, ptInRect(mx, my, findBtn));
        drawBtn(backBtn, "返回菜单", C_GRAY, ptInRect(mx, my, backBtn));
        return;
    }

    /* 阶段2：显示当前信息 + 选择修改项 */
    struct Contact* c = &contacts[modIndex];
    int y = 80;
    renderText("当前联系人信息：", 30, y, C_BLACK, gFontN); y += 28;
    char line[256];
    snprintf(line, sizeof(line), "姓名：%s    性别：%s", c->name, c->gender);
    renderText(line, 50, y, C_BLACK, gFontN); y += 24;
    snprintf(line, sizeof(line), "电话：%s    邮箱：%s", c->phone, c->email);
    renderText(line, 50, y, C_BLACK, gFontN); y += 24;
    snprintf(line, sizeof(line), "地址：%s", c->address);
    renderText(line, 50, y, C_BLACK, gFontN); y += 24;
    snprintf(line, sizeof(line), "单位：%s", c->workplace);
    renderText(line, 50, y, C_BLACK, gFontN);

    if (!modValueMode) {
        /* 选择修改项 */
        renderText("请选择要修改的项目：", 30, 270, C_BLACK, gFontN);
        const char* items[6] = {"1. 姓名", "2. 性别", "3. 电话", "4. 地址", "5. 邮箱", "6. 单位"};
        for (int i = 0; i < 6; i++) {
            int col = i % 3, row = i / 3;
            SDL_Rect r = {80 + col*220, 300 + row*50, 200, 40};
            drawBtn(r, items[i], C_PURPLE, ptInRect(mx, my, r));
        }
        SDL_Rect doneBtn = {280, 460, 200, 48};
        drawBtn(doneBtn, "完成修改", C_GREEN, ptInRect(mx, my, doneBtn));
    } else {
        /* 输入新值 */
        char buf[128];
        snprintf(buf, sizeof(buf), "请输入新的%s：", getFieldName(modItem));
        renderText(buf, 30, 390, C_BLACK, gFontN);
        drawInputField(&modValueField);

        SDL_Rect okBtn = {200, 460, 160, 48};
        SDL_Rect cancelBtn = {400, 460, 160, 48};
        drawBtn(okBtn, "确认修改", C_GREEN, ptInRect(mx, my, okBtn));
        drawBtn(cancelBtn, "取消", C_GRAY, ptInRect(mx, my, cancelBtn));
    }
}

void renderDisplay()
{
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    char title[64];
    snprintf(title, sizeof(title), "所有联系人信息（共 %d 位）", contactCount);
    renderTextCentered(title, WINDOW_W/2, 35, C_BLACK, gFontB);

    if (contactCount == 0) {
        renderTextCentered("通讯录为空，无联系人记录！", WINDOW_W/2, 200, C_RED, gFontN);
    } else {
        /* 联系人列表区域 */
        SDL_Rect listArea = {30, 70, WINDOW_W - 60, 470};
        drawRectFilled(listArea, C_WHITE);
        drawRectOutline(listArea, C_GRAY_LT);

        /* 裁剪到列表区域 */
        SDL_RenderSetClipRect(gRen, &listArea);

        for (int i = 0; i < contactCount; i++) {
            int y = 80 + i * 80 - displayScroll;
            if (y + 80 < listArea.y || y > listArea.y + listArea.h) continue;

            /* 序号 */
            char hdr[32];
            snprintf(hdr, sizeof(hdr), "【第 %d 位】%s（%s）", i + 1, contacts[i].name, contacts[i].gender);
            renderText(hdr, 45, y, C_BLUE, gFontN);

            /* 信息行 */
            char line[256];
            snprintf(line, sizeof(line), "电话：%s    邮箱：%s", contacts[i].phone, contacts[i].email);
            renderText(line, 60, y + 22, C_BLACK, gFontS);
            snprintf(line, sizeof(line), "地址：%s", contacts[i].address);
            renderText(line, 60, y + 40, C_BLACK, gFontS);
            snprintf(line, sizeof(line), "单位：%s", contacts[i].workplace);
            renderText(line, 60, y + 56, C_BLACK, gFontS);

            /* 分隔线 */
            SDL_SetRenderDrawColor(gRen, C_GRAY_LT.r, C_GRAY_LT.g, C_GRAY_LT.b, 255);
            SDL_RenderDrawLine(gRen, 45, y + 74, WINDOW_W - 75, y + 74);
        }

        SDL_RenderSetClipRect(NULL);

        /* 滚动提示 */
        renderText("滚轮/方向键滚动列表", 30, 545, C_GRAY, gFontS);
    }

    /* 文件名输入 + 保存按钮 */
    renderText("文件名：", 80, 565, C_BLACK, gFontS);
    drawInputField(&fileNameField);

    SDL_Rect saveBtn = {480, 560, 100, 32};
    SDL_Rect backBtn = {610, 560, 100, 32};
    drawBtn(saveBtn, "保存", C_GREEN, ptInRect(mx, my, saveBtn));
    drawBtn(backBtn, "返回", C_GRAY, ptInRect(mx, my, backBtn));
}

void renderAuthor()
{
    renderTextCentered("系统开发作者信息", WINDOW_W/2, 35, C_BLACK, gFontB);

    int y = 100;
    int cx = WINDOW_W / 2;
    SDL_Rect panel = {60, 80, WINDOW_W - 120, 380};
    drawRectFilled(panel, C_PURPLE_LT);
    drawRectOutline(panel, C_PURPLE);

    renderTextCentered("通讯录系统", cx, y + 20, C_BLACK, gFontB); y += 50;
    renderTextCentered("数据结构：结构体数组", cx, y + 20, C_BLACK, gFontN); y += 30;
    renderTextCentered("图形库：SDL2 + SDL2_ttf", cx, y + 20, C_BLACK, gFontN); y += 30;
    renderTextCentered("开发环境：g++ / VS / VSCode", cx, y + 20, C_BLACK, gFontN); y += 30;
    renderTextCentered("功能：增、删、查、改、显示、文件保存", cx, y + 20, C_BLACK, gFontN); y += 30;
    renderTextCentered("", cx, y + 20, C_GRAY, gFontN); y += 20;
    renderTextCentered("设计要求：", cx, y + 20, C_BLACK, gFontN); y += 30;
    renderTextCentered("· 人机交互图形界面", cx, y + 20, C_BLACK, gFontS); y += 24;
    renderTextCentered("· 电话号码格式校验（11位）", cx, y + 20, C_BLACK, gFontS); y += 24;
    renderTextCentered("· 邮箱格式校验（含@和.）", cx, y + 20, C_BLACK, gFontS); y += 24;
    renderTextCentered("· 信息保存到 .txt 文件", cx, y + 20, C_BLACK, gFontS); y += 24;

    /* 返回按钮 */
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    SDL_Rect backBtn = {280, 500, 200, 48};
    drawBtn(backBtn, "返回菜单", C_GRAY, ptInRect(mx, my, backBtn));
}
