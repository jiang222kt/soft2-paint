// Resultの見直しと、interpret_command とコマンド結果の描画の切り分けをしたバージョン

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h> // for error catch


// Structure for canvas
typedef struct {
    int width;
    int height;
    char **canvas;
    int **color;
    char pen;
    int pencolor;
    int undo_num;
} Canvas;

typedef struct command Command;


struct command{
    char *str;
    size_t bufsize;
    Command *next;
};

typedef struct{
    Command *begin;
    size_t bufsize;
} History;

// functions for Canvas type
Canvas *init_canvas(int width, int height, char pen);
void reset_canvas(Canvas *c);
void print_canvas(Canvas *c);
void free_canvas(Canvas *c);

// display functions
void rewind_screen(unsigned int line);
void clear_command(void);
void clear_screen(void);

Command *push_front(History *his, const char *str);
Command *pop_front(History *his);
Command *push_back(History *his, const char *str);
Command *pop_back(History *his);
// enum for interpret_command results
// interpret_command の結果をより詳細に分割
typedef enum res{ EXIT, LINE, RECT, CIRCLE, LOAD, CHPEN, CHCOLOR, UNDO, REDO, SAVE, UNKNOWN, ERRNONINT, ERRLACKARGS} Result;
// Result 型に応じて出力するメッセージを返す
char *strresult(Result res);

int max(const int a, const int b);
void draw_line(Canvas *c, const int x0, const int y0, const int x1, const int y1);
void draw_rect(Canvas *c, const int x0, const int y0, const int width, const int height);
void draw_circle(Canvas *c, const int x0, const int y0, const int r);
void load_history(Canvas *c, History *his, const char *filename, History *redo_his);
void chpen(Canvas *c, History *his, const char *pen);
int colormap(const char *color);
void chcolor(Canvas *c, History *his, const char *color);
Result interpret_command(const char *command, History *his, Canvas *c, History *redo_his);
void save_history(const char *filename, History *his);


int main(int argc, char **argv)
{
    const int bufsize = 1000;
    History his = (History){.begin = NULL, .bufsize = bufsize};
    History redo_his = (History){.begin = NULL, .bufsize = bufsize};

    int width;
    int height;
    if (argc != 3){
	fprintf(stderr,"usage: %s <width> <height>\n",argv[0]);
	return EXIT_FAILURE;
    }
    else{
	char *e;
	long w = strtol(argv[1],&e,10);
	if (*e != '\0'){
	    fprintf(stderr, "%s: irregular character found %s\n", argv[1],e);
	    return EXIT_FAILURE;
	}
	long h = strtol(argv[2],&e,10);
	if (*e != '\0'){
	    fprintf(stderr, "%s: irregular character found %s\n", argv[2],e);
	    return EXIT_FAILURE;
	}
	width = (int)w;
	height = (int)h;    
    }
    char pen = '*';
    
    char buf[his.bufsize];

    Canvas *c = init_canvas(width,height, pen);

    printf("\n"); // required especially for windows env
    
    while (1) {
	print_canvas(c);
	printf("* > ");
	if(fgets(buf, bufsize, stdin) == NULL) break;
	
	const Result r = interpret_command(buf, &his,c, &redo_his);

	if (r == EXIT) break;

	// 返ってきた結果に応じてコマンド結果を表示
	clear_command();
	printf("%s\n",strresult(r));

	// LINEの場合はHistory構造体に入れる
	if (r == LINE || r == RECT || r == CIRCLE || r == CHPEN || r == CHCOLOR) {
        push_back(&his, buf);    
	}
	rewind_screen(2); // command results
	clear_command(); // command itself
	rewind_screen(height+2); // rewind the screen to command input
	
    }
    
    clear_screen();
    free_canvas(c);
    
    return 0;
}

Canvas *init_canvas(int width,int height, char pen)
{
    Canvas *new = (Canvas *)malloc(sizeof(Canvas));
    new->width = width;
    new->height = height;
    new->canvas = (char **)malloc(width * sizeof(char *));
    new->color = (int **)malloc(width * sizeof(int *));
    
    char *tmp = (char *)malloc(width*height*sizeof(char));
    memset(tmp, ' ', width*height*sizeof(char));

    int *tmp1 = (int *)malloc(width*height*sizeof(int));
    memset(tmp1, 37, width*height*sizeof(int));

    for (int i = 0 ; i < width ; i++){
	new->canvas[i] = tmp + i * height;
    new->color[i] = tmp1 +i * height;
    }
    
    new->pen = pen;
    new->pencolor = 37;
    new->undo_num = 0;
    return new;
}

void reset_canvas(Canvas *c)
{
    const int width = c->width;
    const int height = c->height;
    memset(c->canvas[0], ' ', width*height*sizeof(char));
    memset(c->color[0], 37, width*height*sizeof(int));

    c->pen = '*';
    c->pencolor = 37;
}


void print_canvas(Canvas *c)
{
    const int height = c->height;
    const int width = c->width;
    char **canvas = c->canvas;
    int **color = c->color;
    
    // 上の壁
    printf("+");
    for (int x = 0 ; x < width ; x++)
	printf("-");
    printf("+\n");
    
    // 外壁と内側
    for (int y = 0 ; y < height ; y++) {
	printf("|");
	for (int x = 0 ; x < width; x++){
	    const char c = canvas[x][y];
	    //putchar(c);
        printf("\e[%dm%c\e[0m", color[x][y], c);
	}
	printf("|\n");
    }
    
    // 下の壁
    printf("+");
    for (int x = 0 ; x < width ; x++)
	printf("-");
    printf("+\n");
    fflush(stdout);
}

void free_canvas(Canvas *c)
{
    free(c->canvas[0]); //  for 2-D array free
    free(c->canvas);
    free(c);
}

void rewind_screen(unsigned int line)
{
    printf("\e[%dA",line);
}

void clear_command(void)
{
    printf("\e[2K");
}

void clear_screen(void)
{
    printf("\e[2J");
}


int max(const int a, const int b)
{
    return (a > b) ? a : b;
}
void draw_line(Canvas *c, const int x0, const int y0, const int x1, const int y1)
{
    const int width = c->width;
    const int height = c->height;
    char pen = c->pen;
    int pencolor = c->pencolor;
    
    const int n = max(abs(x1 - x0), abs(y1 - y0));
    if ( (x0 >= 0) && (x0 < width) && (y0 >= 0) && (y0 < height)){
	c->canvas[x0][y0] = pen;
    c->color[x0][y0] = pencolor;
    }
    for (int i = 1; i <= n; i++) {
	const int x = x0 + i * (x1 - x0) / n;
	const int y = y0 + i * (y1 - y0) / n;
	if ( (x >= 0) && (x< width) && (y >= 0) && (y < height)){
	    c->canvas[x][y] = pen;
        c->color[x][y] = pencolor;
    }
    }
    c->undo_num = 0;
}

void draw_rect(Canvas *c, const int x0, const int y0, const int width, const int height){
    draw_line(c, x0, y0, x0 + width, y0);
    draw_line(c, x0, y0, x0, y0 + height);
    draw_line(c, x0 + width, y0, x0 + width, y0 + height);
    draw_line(c, x0, y0 + height, x0 + width, y0 + height);
}

void draw_circle(Canvas *c, const int x0, const int y0, const int r){
    const int width = c->width;
    const int height = c->height;
    char pen = c->pen;
    int pencolor = c->pencolor;

    for (int x = x0 - r; x <= x0 + r; x++) {
        for (int y = y0 - r; y <= y0 + r; y++){
            if((x - x0)*(x - x0) + (y - y0)*(y - y0) == r*r){
                if ( (x >= 0) && (x< width) && (y >= 0) && (y < height)){
                    c->canvas[x][y] = pen;
                    c->color[x][y] = pencolor;
                }
            }
        }
    }
    c->undo_num = 0;
}

void load_history(Canvas *c, History *his, const char *filename, History *redo_his){

    const char *default_history_file = "history.txt";
    if (filename == NULL)
	filename = default_history_file;

    FILE *fp;
    if ((fp = fopen(filename, "r")) == NULL) {
	fprintf(stderr, "error: cannot open %s.\n", filename);
	return;
    }

    char buf[his->bufsize];
    int bufsize = his->bufsize;

	while(fgets(buf, bufsize, fp) != NULL){
	
	const Result r = interpret_command(buf, his, c, redo_his);

	// 返ってきた結果に応じてコマンド結果を表示
	clear_command();
	printf("%s\n",strresult(r));

    push_back(his, buf);  
    }  

}

void chpen(Canvas *c, History *his, const char *pen){
    if(pen != NULL){
        c->pen = pen[0];
    }
    c->undo_num = 0;
}

void save_history(const char *filename, History *his)
{
    const char *default_history_file = "history.txt";
    if (filename == NULL)
	filename = default_history_file;
    
    FILE *fp;
    if ((fp = fopen(filename, "w")) == NULL) {
	fprintf(stderr, "error: cannot open %s.\n", filename);
	return;
    }
    
    Command *p = his->begin;
    while(p != NULL) {
	fprintf(fp, "%s", p->str);
    p = p->next;
    }
    
    fclose(fp);
}

//色をエスケープシーケンスの数字に変換
int colormap(const char *color){
    if(strcmp(color, "black") == 0){
        return 30;
    }else if(strcmp(color, "red") == 0){
        return 31;
    }else if(strcmp(color, "green") == 0){
        return 32;
    }else if(strcmp(color, "yellow") == 0){
        return 33;
    }else if(strcmp(color, "blue") == 0){
        return 34;
    }else if(strcmp(color, "magenta") == 0){
        return 35;
    }else if(strcmp(color, "cyan") == 0){
        return 36;
    }else if(strcmp(color, "white") == 0){
        return 37;
    }
return -1;
}

void chcolor(Canvas *c, History *his, const char *color){
    if(color != NULL){
        int i = colormap(color);
        if (i > 0) c->pencolor = i;
    }
    c->undo_num = 0;
}

Result interpret_command(const char *command, History *his, Canvas *c, History *redo_his)
{
    char buf[his->bufsize];
    strcpy(buf, command);
    buf[strlen(buf) - 1] = 0; // remove the newline character at the end
    
    const char *s = strtok(buf, " ");
    if (s == NULL) { // 改行だけ入力された場合
	return UNKNOWN;
    }
    // The first token corresponds to command
    if (strcmp(s, "line") == 0) {
	int p[4] = {0}; // p[0]: x0, p[1]: y0, p[2]: x1, p[3]: x1 
	char *b[4];
	for (int i = 0 ; i < 4; i++){
	    b[i] = strtok(NULL, " ");
	    if (b[i] == NULL){
		return ERRLACKARGS;
	    }
	}
	for (int i = 0 ; i < 4 ; i++){
	    char *e;
	    long v = strtol(b[i],&e, 10);
	    if (*e != '\0'){
		return ERRNONINT;
	    }
	    p[i] = (int)v;
	}
	
	draw_line(c,p[0],p[1],p[2],p[3]);
	return LINE;
    }
    
    if (strcmp(s, "rect") == 0) {
	int p[4] = {0}; // p[0]: x0, p[1]: y0, p[2]: width, p[3]: height 
	char *b[4];
	for (int i = 0 ; i < 4; i++){
	    b[i] = strtok(NULL, " ");
	    if (b[i] == NULL){
		return ERRLACKARGS;
	    }
	}
	for (int i = 0 ; i < 4 ; i++){
	    char *e;
	    long v = strtol(b[i],&e, 10);
	    if (*e != '\0'){
		return ERRNONINT;
	    }
	    p[i] = (int)v;
	}
	
	draw_rect(c,p[0],p[1],p[2],p[3]);
	return RECT;
    }

    if (strcmp(s, "circle") == 0) {
	int p[3] = {0}; // p[0]: x0, p[1]: y0, p[2]: r
	char *b[3];
	for (int i = 0 ; i < 3; i++){
	    b[i] = strtok(NULL, " ");
	    if (b[i] == NULL){
		return ERRLACKARGS;
	    }
	}
	for (int i = 0 ; i < 3 ; i++){
	    char *e;
	    long v = strtol(b[i],&e, 10);
	    if (*e != '\0'){
		return ERRNONINT;
	    }
	    p[i] = (int)v;
	}
	
	draw_circle(c,p[0],p[1],p[2]);
	return CIRCLE;
    }

    if (strcmp(s, "save") == 0) {
	s = strtok(NULL, " ");
	save_history(s, his);
	return SAVE;
    }
    
    if (strcmp(s, "load") == 0) {
        s = strtok(NULL, " ");
        load_history(c, his, s, redo_his);
        return LOAD;
    }

    if (strcmp(s, "chpen") == 0){
        s = strtok(NULL, " ");
        chpen(c, his, s);
        return CHPEN;
    }

    if (strcmp(s, "chcolor") == 0){
        s = strtok(NULL, " ");
        chcolor(c, his, s);
        return CHCOLOR;
    }

    if (strcmp(s, "undo") == 0) {
	    reset_canvas(c);

        Command *p = his->begin;
        Command *pp = redo_his->begin;

	    if (p != NULL){
            c->undo_num++;
            int tmp = c->undo_num;
            Command *popped_command = pop_back(his);

            push_back(redo_his, popped_command->str);
            free(popped_command->str);
            free(popped_command);

	        while(p != NULL) {
		        interpret_command(p->str, his, c, redo_his);
                p = p->next;
	        }
            c->undo_num = tmp;
        }

	    return UNDO;
    }
    
    if (strcmp(s, "redo") == 0) {

        if(redo_his->begin != NULL && c->undo_num > 0){
            c->undo_num--;
            int temp = c->undo_num;
            Command *popped_command = pop_back(redo_his);

            reset_canvas(c);
            push_back(his, popped_command->str);
            free(popped_command->str);
            free(popped_command);

            Command *p = his->begin;

	        while(p != NULL) {
		        interpret_command(p->str, his, c, redo_his);
                p = p->next;
	        }
            c->undo_num = temp;

        }

	    return REDO;
    }

    if (strcmp(s, "quit") == 0) {
	return EXIT;
    }
    
    return UNKNOWN;
}

char *strresult(Result res){
    switch(res) {
    case EXIT:
	break;
    case SAVE:
	return "history saved";
    case LINE:
	return "1 line drawn";
    case RECT:
    return "1 rect drawn";
    case CIRCLE:
    return "1 circle drawn";
    case LOAD:
    return "load history";
    case CHPEN:
    return "change pen";
    case CHCOLOR:
    return "change color";
    case UNDO:
	return "undo!";
    case REDO:
    return "redo!";
    case UNKNOWN:
	return "error: unknown command";
    case ERRNONINT:
	return "Non-int value is included";
    case ERRLACKARGS:
	return "Too few arguments";
    }
    return NULL;
}

Command *push_front(History *his, const char *str){
    Command *p = (Command *)malloc(sizeof(Command));
    char *s = (char *)malloc(strlen(str) + 1);
    strcpy(s, str);
    
    *p = (Command){.str = s, .next = his->begin};
    his->begin = p;
    
    return p;
}


Command *pop_front(History *his){
    Command *p = his->begin;
    
    if (p != NULL){
	// 次の要素を線形リストの先頭に設定
	his->begin = p->next;
	
    //free(p->str);
    //free(p);
    }
    
    return p;
}


Command *push_back(History *his, const char *str){
    Command *p = his->begin;
    
    if (p == NULL) return push_front(his,str);
    
    while (p->next != NULL) {
	p = p->next;
    }
    
    Command *q = (Command *)malloc(sizeof(Command));
    char *s = (char *)malloc(strlen(str) + 1);
    strcpy(s, str);
    
    *q = (Command){.str = s, .next = NULL};
    p->next = q;
    
    return q;
}

// pop_back の実装
Command *pop_back(History *his)
{
    Command *p = his->begin;
    Command *q = NULL;
    
    if (p == NULL) return NULL;
    
    while (p->next != NULL){
	// nextにいく前に今いるポインタを記憶する
	q = p;
	p = p->next;
    }
    
    if ( q == NULL ) return pop_front(his);
    // 新たな末尾の設定
    if ( q != NULL) q->next = NULL;
    
    //free(p->str);
    //free(p);
    return p;
}
