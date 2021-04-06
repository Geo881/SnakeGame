/*
 * A Snake Game implementation
 * Created by Geovanni Roberts and Joseph Pelletier
 *
 *		*************
 *		*
 *		*	Snake
 *		*
 *		*****+  7
 */


#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <sys/select.h>

//game debug stuff (Joe)
struct lnkstr{
	char str[255];
	struct lnkstr *next;
} *lshead = NULL;
#define dbpf(pattern, ...) {struct lnkstr *nlnkstr = malloc(sizeof(struct lnkstr));\
		sprintf(nlnkstr->str, pattern, __VA_ARGS__);\
		nlnkstr -> next = lshead;\
		lshead = nlnkstr;}
#define drawdb {move(sg_rows-1, 0); while(lshead) {\
	addstr(lshead->str); addstr("; ");\
	void *olsp = lshead; lshead = lshead->next; free(olsp);}}
//structs
struct coord { //coordinates
	int x;
	int y;
};
#define coordIs(a, b) (a.x == b.x && a.y == b.y)
/*macro for testing coord equality
Author Joe*/

#define chcol(c) attron(COLOR_PAIR(c))
#define drawch(ch, x, y) mvaddch(y, x, ch)
#define drawchc(ch, loc) mvaddch(loc.y, loc.x, ch)
#define drawstr(str, x, y) mvaddstr(y, x, str)
#define mvcur(x, y) move(y, x)
#define setbg(c) bkgd(COLOR_PAIR(c))
#define drawf(x, y, fmt, ...) mvprintw(y, x, fmt, __VA_ARGS__)
#define updateres {sg_rows = LINES; sg_cols = COLS; }
/*a bunch of useless macros that make it easier for me
 and harder for you, to read my code
Author Joe*/

struct Trophy { //trophies
	struct coord location;
	int time;
	int value;
} ctrophy; //current trophy

#define max_username 10
struct scoreent { //score entry
	int score;
	char username[max_username + 1];
};
//globals and defines
int sg_cols, sg_rows;
WINDOW *screen;
struct scoreent highScores[4][5];//a list of the top 5 scores [4 difficulties]
//^ this can be read or writen directly from or to the file
#define max_coords 0x1000 /*length of the corditate arrays or 4096 if you prefer*/
struct coord snake[max_coords], obstacles[max_coords];
//these coords define the snake and obstacles
char username[max_username + 1]; //the username of the current player to be stored allong with their score
int alive; //true if snake is alive (game not over)
int difficulty = 0; //difficulty level
int snakelen = 1; //the curent length of the snake
int growamt = 0; //the amount the snakes neets to grow ofer the comming ticks
int obstacleCount = 0; //ammount of elements in obstacles (for iteration)
int moveDirection = 0; //direction snake is moving (one of the following)
#define mdup 0
#define mddown 1
#define mdleft 2
#define mdright 3

//protypyes
int sg_clock();
/*return 0 to quit game
calls tick and drawFrame and manages timing
Author Joe*/

void tick();
/*calls all code that must be continualy executed
Author Joe*/

int mspt();
/*Milliseconds per tick
Author Joe*/

void getInput();
/*gets user input from the keyboard without blocking
and sets moveDirection approriatly
Author Joe*/

void moveSnake();
/*moves the snake in move direction
does not detect win or loss or eat trophies
but does need to manage snake growth
Author Geovanni*/

void drawFrame();
/*renders the obstacles, border, and snake
Author Joe*/

void chooseDifficulty();
/*renders a diffuculty select screen
and allows the user to chose a difficulty
Author Geovanni*/
void askUsername();
/*get the username from the player
Author Geovanni*/

void placeObstacles();
/*places the obstacles usisg the obstacle corrds array for
hard and insane difficulty
Author Geovanni*/
int searchObs(struct coord);
/*returns 1 if there is an obsticle at this coordinate 0 otherwise*/

void placeTrophy();
/*create a new random trophy
Author Geovanni*/

void gameOver();
/*display the high score / game over screen
and tell the user if this was a high score and sort the score
Author Joe*/

int winScreen();
/*you won continue playing?
(score will always be the snakes win length when this method is called)
return weather to continue the game
Author Joe*/

void loadhs();
/*load the high scores from the scores file
Aithor Geovanni*/

void savehs();
/*load the high scores from the file
Author Geovanni*/

void displayhs();
/*load the high scores from the file
Author Geovanni*/

int placeScore();
/*sort the current score (snakelen) into the list (highScores)
and return 1 if the score was the highest
Author Geovanni*/

void initgame();
/*called for every new game to place the snake,
reset the stake length to one and a rendom start pos and direction
place the first trophy by calling place trophy
Author Joe*/

void makeColors();
/*Initalize colors and pairs for the game;
Athor Joe*/

int menu();
/* this is our welcome, menu, and credit screen
returns an option defined above the implibentation
Author: this function inherits project credits*/

/*entry point
game and curses initalization
Author Joe*/
int main(int argc, char const *argv[]) {
	srand(time(0)); //initalize the rng with the current time as the seed
	loadhs(); //load the high scores
	screen = initscr(); //init curses
	makeColors(); //our code to initalize colors and color pairs
	cbreak(); //console gets characters without enter press
	noecho(); //turn off echo
	keypad(stdscr, 1); //arrow keys are picked up by getch
	nonl(); //supposed to allow us to get enter key
	updateres; //sets sg_rows and sg_cols
	while(sg_clock()); //call the game clock
	echo(); //turn echo back on
	nocbreak(); //turn off cbreak
	endwin(); //de-init curses
	savehs(); //save the high scores
	return 0;
}
#define sgc_background_fg 1
#define sgc_background_bg 2
#define sgc_head_fg 3
#define sgc_head_bg 4
#define sgc_tail_fg 5
#define sgc_tail_bg 6
#define sgc_rem4_fg 7
#define sgc_rem4_bg 8
#define sgc_trophy_fg 9
#define sgc_trophy_bg 10
#define sgc_obstacle_fg 11
#define sgc_obstacle_bg 12
#define sgc_border_fg 13
#define sgc_border_bg 14

#define sgcp_background 1
#define sgcp_head 2
#define sgcp_tail 3
#define sgcp_rem4 4
#define sgcp_trophy 5
#define sgcp_obstacle 6
#define sgcp_border 7

void makeColors() {
	start_color(); //start colors for curses
	//background
	init_color(sgc_background_fg, 1000, 1000, 1000);
	init_color(sgc_background_bg, 0, 100, 0);
	init_pair(sgcp_background, sgc_background_fg, sgc_background_bg);
	//trophy
	init_color(sgc_trophy_fg, 1000, 900, 0);
	init_color(sgc_trophy_bg, 250, 150, 0);
	init_pair(sgcp_trophy, sgc_trophy_fg, sgc_trophy_bg);
	//obstacle
	init_color(sgc_obstacle_fg, 1000, 0, 0);
	init_color(sgc_obstacle_bg, 200, 0, 0);
	init_pair(sgcp_obstacle, sgc_obstacle_fg, sgc_obstacle_bg);
	//border
	init_color(sgc_border_fg, 400, 0, 0);
	init_color(sgc_border_bg, 200, 0, 0);
	init_pair(sgcp_border, sgc_border_fg, sgc_border_bg);
	//head
	init_color(sgc_head_fg, 0, 900, 0);
	init_color(sgc_head_bg, 0, 100, 0);
	init_pair(sgcp_head, sgc_head_fg, sgc_head_bg);
	//tail
	init_color(sgc_tail_fg, 200, 600, 200);
	init_color(sgc_tail_bg, 0, 100, 0);
	init_pair(sgcp_tail, sgc_tail_fg, sgc_tail_bg);
	//rem4
	init_color(sgc_rem4_fg, 100, 400, 100);
	init_color(sgc_rem4_bg, 0, 100, 0);
	init_pair(sgcp_rem4, sgc_rem4_fg, sgc_rem4_bg);
}
#define playgame 0
#define savequit 1
#define disphs 2
int menu() {
	clear();
	setbg(sgcp_head);
	drawstr(" ____    _   _     ___     _   _    ____ ", 3, 3);
	drawstr("/  __|  | \\ | |   / _ \\   | | / /  |  __|", 3, 4);
	drawstr("| |__   |  \\| |  / / \\ \\  | |/ /   | |__ ", 3, 5);
	drawstr("\\__  \\  |     |  | |_| |  |   <    |  __|", 3, 6);
	drawstr(" __| |  | |\\  |  |  _  |  | |\\ \\   | |__ ", 3, 7);
	drawstr("|____/  |_| \\_|  |_| |_|  |_| \\_\\  |____|", 3, 8);
	drawstr("by Joseph Pelletier and Geovanni Roberts", 3, 10);
	drawstr("Press (h) to view the high scores or", 3, 13);
	drawstr("Press (s) to save and quit or", 3, 14);
	drawstr("Press any key to play ... ", 3, 15);
	refresh();
	switch (getch()) {
		case 'h':
		case 'H':
			return disphs;
		case 's':
		case 'S':
			return savequit;
		default:
			return playgame;
	}
}
int sg_clock(){
	switch (menu()) {
		case disphs:
			displayhs();
			return 1;
		case savequit:
			return 0;
		case playgame:
			break;
	}

	chooseDifficulty();
	askUsername();
	initgame();
	alive = 1;
	while (alive){
		tick(); //call repeat code
		drawFrame(); //draw a game frame
		napms(mspt()); //wait a very long time for the next tick
	}
	gameOver();
}
int iswin = 0;
void tick() {
	//dbpf("len %d winlen %d", snakelen, sg_cols + sg_rows);
	getInput();
	moveSnake();

	/*death conditions*/
	//obstacle death
	for(int i = 0; i < obstacleCount; i ++)
		if(coordIs(obstacles[i], snake[0]))
			alive = false;
	//self death
	for(int i = 1; i < snakelen; i ++)
		if(coordIs(snake[i], snake[0]))
			alive = false;
	//border death sg_rows sg_cols
	if(snake[0].x < 1  || snake[0].y < 1 ||
		snake[0].x > sg_cols - 2  || snake[0].y > sg_rows - 2)
		alive = false;

	//eat the trophy and make a new one
	if(coordIs(snake[0], ctrophy.location)){
		growamt += ctrophy.value;
		placeTrophy();
	}
	//age the trophy
	if(!ctrophy.time)
		placeTrophy();
	ctrophy.time --;

	//win condition (imposile to happen in the same tick as death)
	if(!iswin && (snakelen == sg_cols + sg_rows)) {
		iswin = true;
		winScreen();
	}
}
void getInput(){
	nodelay(screen, 1); //set getch to non-blocking mode
	for(int gnk = 1; gnk;)
	switch (getch()) {
		case KEY_UP:
		case 'w':
		case 'W':
			moveDirection = mdup;
			break;
		case KEY_DOWN:
		case 's':
		case 'S':
			moveDirection = mddown;
			break;
		case KEY_LEFT:
		case 'a':
		case 'A':
			moveDirection = mdleft;
			break;
		case KEY_RIGHT:
		case 'd':
		case 'D':
			moveDirection = mdright;
			break;
		case ERR:
			gnk = 0;
			break;
	}
	nodelay(screen, 0); //return getch() to the normal expected behaviour
}
int mspt() {
	return 7000 / /*tpts*/ (30 + difficulty * 10 + snakelen);
}

void drawFrame(){ //this should be made more colorful
	clear(); // clear the virtual screen
	/*draw border*/
	chcol(sgcp_border);
	for(int i = 0; i < sg_rows; i ++){
		drawch('*', 0, i);
		drawch('*', sg_cols - 1, i);
	}
	for(int i = 0; i < sg_cols; i ++){
		drawch('*', i, 0);
		drawch('*', i, sg_rows - 1);
	}

	/*draw obstacles*/
	chcol(sgcp_obstacle);
	for(int i = 0; i < obstacleCount; i ++)
		drawchc('X', obstacles[i]);

	/*draw snake*/
	chcol(sgcp_head);
	drawchc('0', snake[0]);
	for(int i = 1; i < snakelen; i ++) {
		if(!(i % 4))
			chcol(sgcp_rem4);
		else
			chcol(sgcp_tail);
		drawchc('0', snake[i]);
	}

	/*draw trophy*/
	chcol(sgcp_trophy);
	drawchc('0' + ctrophy.value, ctrophy.location);

	/*draw obstacles*/
	chcol(sgcp_obstacle);
	for(int i = 0; i < obstacleCount; i ++)
		drawchc('X', obstacles[i]);

	drawdb;

	mvcur(sg_cols - 1, sg_rows - 1); //reset cur pos
	refresh(); // push screen state changes to the console
}

void initgame(){ //may be inproved if time is available
	updateres;
	obstacleCount = 0;
	snakelen = 1;
	moveDirection = rand() % 4;
	/*snake will be placed in the border but
	will leave befor the first death check*/
	switch (moveDirection) {
		case mdup:
			snake[0].x = rand() % (sg_cols - 3) + 1;
			snake[0].y = sg_rows - 1;
			break;
		case mddown:
			snake[0].x = rand() % (sg_cols - 3) + 1;
			snake[0].y = 0;
			break;
		case mdleft:
			snake[0].y = rand() % (sg_rows - 3) + 1;
			snake[0].x = sg_cols - 1;
			break;
		case mdright:
			snake[0].y = rand() % (sg_rows - 3) + 1;
			snake[0].x = 0;
			break;
	}
	placeObstacles();
	placeTrophy();
}
void moveSnake(){
	struct coord snakeend = snake[snakelen - 1];

	//for(int i = snakelen - 1; i > 0; i --)
	//	snake[i] = snake[i - 1];

	struct coord curr = snake[0];
	struct coord next;
	struct coord toDel;
	int currLen = snakelen;


	for(int i = 1; i <= currLen; i++){
		next = snake[i];
		snake[i] = curr;
		curr = next;

	}

	switch (moveDirection) {
		case mdup:
			snake[0].y --;
			break;
		case mddown:
			snake[0].y ++;
			break;
		case mdleft:
			snake[0].x --;
			break;
		case mdright:
			snake[0].x ++;
			break;
	}

	if(growamt) {
		growamt --;
		snake[snakelen ++] = snakeend;
	}
}
void chooseDifficulty(){
	clear();
	setbg(sgcp_head);
	drawstr("1. (e) Easy", 3, 5);
	drawstr("2. (m) Medium", 3, 6);
	drawstr("3. (h) Hard", 3, 7);
	drawstr("4. (i) Insane", 3, 8);
	drawstr("Select a difficulty: ", 7, 3);
	refresh();
	for(int aa = 1; aa; aa--)
		switch (getch()) {
			case '1':
			case 'e':
			case 'E':
				difficulty = 0;
				break;
			case '2':
			case 'm':
			case 'M':
				difficulty = 1;
				break;
			case '3':
			case 'h':
			case 'H':
				difficulty = 2;
				break;
			case '4':
			case 'i':
			case 'I':
				difficulty = 3;
				break;
			default:
				aa ++;
		}
}
void askUsername(){
	clear();
	drawstr("Enter a username: ", 1, 1);
	mvcur(2, 3);
	refresh();
	for(int c = 1, len = 0, ch; c;) {
		ch = getchar();
		if(len && (ch == KEY_BACKSPACE || ch == 127 || ch == '\b')) {
			username[-- len] = '\0';
			drawch(' ', len + 2, 3);
			mvcur(len + 2, 3);
		} else if(len < max_username && ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
				(ch >= '0' && ch <= '9') || ch == '_')) {
			username[len] = ch;
			username[++ len] = '\0';
			drawch(ch, len + 1,3);

		} else if(len && (ch == '\n' || ch == '\r'))
			c = 0; //break loop if user presses enter and entered some text


		refresh();
	}
}
int searchObs(struct coord obs){
	for(int i = 0; i < obstacleCount; i++){
		if(coordIs(obs, obstacles[i]) ){
				return 1;
		}
	}
	return 0;
}
void placeObstacles(){
	int width = sg_cols-2;
	int height = sg_rows-2;
	int amount;
	struct coord obs;
	if(difficulty == 3){
		amount = (width*height) / 80;
		for(int i = 0; i < amount; i++){
				obs.x = (rand() % (width-1))+1;
				obs.y = (rand() % (height-1))+1;
				while (searchObs(obs)){
					obs.x = (rand() % (width-1))+1;
					obs.y = (rand() % (height-1))+1;
				}
				obstacles[i] = obs;
				obstacleCount++;
		}
	}
	if(difficulty == 2){
		amount = (width*height) / 140;
		for(int i = 0; i < amount; i++){
				obs.x = (rand() % (width-1))+1;
				obs.y = (rand() % (height-1))+1;
				while (searchObs(obs)){
					obs.x = (rand() % (width-1))+1;
					obs.y = (rand() % (height-1))+1;
				}
				obstacles[i] = obs;
				obstacleCount++;
		}
	}
}

void placeTrophy(){
	int width = sg_cols-2;
	int height = sg_rows-2;
	int num = (rand() % 9)+1;
	ctrophy.value = num;
	do {
		ctrophy.location.x = (rand() % (width-1)) + 1; //removed -1
		ctrophy.location.y = (rand() % (height-1)) + 1; //removed -1
	} while (searchObs(ctrophy.location));
	ctrophy.time = ((rand() % 5000 + 4000) +
		(500 * (3 - difficulty))) / mspt(); //addd rand time
}
//(Joe) renamed x to vpos and defined an expression for hpos
void displayhs() {
	setbg(sgcp_obstacle);
	clear();
	drawstr("[ High scores ]" , 1, 1);
	//int level = 1;
	#define hpos (1 + (i * (max_username + 7)))
	for(int i = 0; i < 4; i ++){
		move(3, hpos);
		switch (i) { //(Joe) difficulty by name rather than value)
			case 0: addstr("      Easy:"); break;
			case 1: addstr("    Medium:"); break;
			case 2: addstr("      Hard:"); break;
			case 3: addstr("    Insane:"); break;
		}
		int vpos = 4;
		for(int j = 0; j < 5; j ++){
			if(highScores[i][j].score){ //(Joe) check if the score is valid
				move(++ vpos, hpos);
				printw("%*s: %03d", max_username, highScores[i][j].username,
					highScores[i][j].score); //(Joe) added space padding
			}
		}
		//x++;
		//move(++x,10);
		//level ++;
	}
	//x++;
	move(12, 1);
	printw("Press any key go back ... ");
	refresh();
	getchar();
}
void gameOver(){ // dummy
	//there is delibratly no clear here, you can see how you lost
	setbg(sgcp_obstacle);
	if(placeScore())
		drawstr("[ HIGH SCORE ]", sg_cols / 2 - 7, sg_rows / 2 - 1);
	drawf(sg_cols / 2 - 7, sg_rows / 2, "[ Score: %03d ]", snakelen - 1);
	drawstr("[ Game over. ]", sg_cols / 2 - 7, sg_rows / 2 + 1);
	refresh();
	for(;;) switch (getch()) { //control keys don't exit game over
		case KEY_UP:
		case KEY_DOWN:
		case KEY_LEFT:
		case KEY_RIGHT:
		case 'w':
		case 'W':
		case 's':
		case 'S':
		case 'a':
		case 'A':
		case 'd':
		case 'D':
			break;
		default:
			return;
	}
}
int winScreen(){
	clear();
	setbg(sgcp_trophy);
	drawstr("__                __   _____    _   _    _ ", 3, 3);
	drawstr("\\ \\      __      / /  |_   _|  | \\ | |  | |", 3, 4);
	drawstr(" \\ \\    /  \\    / /     | |    |  \\| |  | |", 3, 5);
	drawstr("  \\ \\  / /\\ \\  / /      | |    |     |  |_|", 3, 6);
	drawstr("   \\ \\/ /  \\ \\/ /      _| |_   | |\\  |   _ ", 3, 7);
	drawstr("    \\__/    \\__/      |_____|  |_| \\_|  |_|", 3, 8);
	drawstr("You won! But how long can you go?", 3, 10);
	drawstr("Continue Playing? [y/n]", 3, 11);
	refresh();
	for(int a = 1; a;) {
		switch (getch()) {
			case 'n':
				alive = 0;
				a = 0;
				break;
			case 'y':
				a = 0;
				break;
		}
	}
}
#define HS_FILE "highscores"

void loadhs(){ //dummy implementation
	char *byt = (char*) highScores; //clearing any garbage data
	for(int i = 0; i < sizeof(highScores); i ++)
		byt[i] = 0;

	FILE *fptr;
	if ((fptr = fopen(HS_FILE, "ab+")) == NULL){
		fptr = fopen(HS_FILE,"wb");
	}
	else{
		fread(highScores, sizeof(highScores),1,fptr);
	}
	fclose(fptr);
}
void savehs(){//dummy implementation
	FILE *fptr = fopen(HS_FILE,"wb");
	fwrite(highScores, sizeof(highScores), 1, fptr);
	fclose(fptr);
}
int placeScore() {
	struct scoreent thisscore;
	thisscore.score = snakelen - 1;
	for(int i = 0; i < sizeof(username); i ++)
		thisscore.username[i] = username[i];

	for(int i = 0; i < 5; i++) {
		if(thisscore.score > highScores[difficulty][i].score) {
			int j = i;
			int placement = i;
			struct scoreent next;
			struct scoreent curr = thisscore;
			while(j < 5) {
				next = highScores[difficulty][j];
				highScores[difficulty][j] = curr;
				curr = next;
				j++;
			}
			return !i;
		}
	}
	return 0;
}
/*
COLOR_BLACK
COLOR_RED
COLOR_GREEN
COLOR_YELLOW
COLOR_BLUE
COLOR_MAGENTA
COLOR_CYAN
COLOR_WHITE
*/
