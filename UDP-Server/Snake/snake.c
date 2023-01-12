#include <stdio.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>

//Controls (arrow keys for Manjaro)
#define UP1  'A'
#define LEFT1  'D'
#define RIGHT1  'C'
#define DOWN1 'B'
#define UP2  'w'
#define LEFT2  'a'
#define RIGHT2  'd'
#define DOWN2 's'
#define ENTER_KEY 10
#define EXIT_BUTTON 27 //ESC
#define PAUSE_BUTTON 112 //P

#define SNAKE_ARRAY_SIZE 40
#define consoleWidth 152
#define consoleHeight 40

pthread_t snake2;
int snakeXY1[2][SNAKE_ARRAY_SIZE], snakeXY2[2][SNAKE_ARRAY_SIZE];; //Two Dimensional Array, the first array is for the X coordinates and the second array for the Y coordinates
int snakeLength1 = 4, snakeLength2 = 4; //Starting Length
int direction1 = LEFT1; //DO NOT CHANGE THIS TO RIGHT ARROW, THE GAME WILL INSTANTLY BE OVER IF YOU DO!!! <- Unless the prepareSnakeArray function is changed to take into account the direction....
char direction2 = LEFT2;
int foodXY1[2], foodXY2[2];
int score1 = 0, score2 = 0;
int speed1 = 1, speed2 = 1;
int gameOver1 = 0, gameOver2 = 0;

int waittime1 = 180000;
void savegame();
void replay();
int count=0;
struct node{
    int head[2],nail[2],length,foodx,foody,dir,speed,waitime,score;
    struct node *next,*pre;
};
typedef struct node Node;
Node *first, *current, *previous, *space;

//Linux Functions - These functions emulate some functions from the windows only conio header file
void gotoxy(int x,int y)
{
    printf("%c[%d;%df",0x1B,y,x);
}

int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if(ch != EOF){
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

char getch()
{
    char c;
    system("stty raw");
    c= getchar();
    system("stty sane");
    return(c);
}
//End linux Functions

//This function checks if a key has pressed, then checks if its any of the arrow keys/ p/esc key. It changes direction acording to the key pressed.
int checkKeysPressed1()
{
    int pressed1;
    //If a key has been pressed
    if(kbhit()){
        pressed1=getch();
        if (direction1 != pressed1){
            if(pressed1 == DOWN1 && direction1 != UP1)
                direction1 = pressed1;
            else if (pressed1 == UP1 && direction1 != DOWN1)
                direction1 = pressed1;
            else if (pressed1 == LEFT1 && direction1 != RIGHT1)
                direction1 = pressed1;
            else if (pressed1 == RIGHT1 && direction1 != LEFT1)
                direction1 = pressed1;
        }
    }
    return direction1;
}

char checkKeysPressed2()
{
    char pressed2;
    //If a key has been pressed
    if(kbhit()){
        pressed2 = getch();
        if (direction2 != pressed2){
            if(pressed2 == DOWN2 && direction2 != UP2)
                direction2 = pressed2;
            else if (pressed2 == UP2 && direction2 != DOWN2)
                direction2 = pressed2;
            else if (pressed2 == LEFT2 && direction2 != RIGHT2)
                direction2 = pressed2;
            else if (pressed2 == RIGHT2 && direction2 != LEFT2)
                direction2 = pressed2;
        }
    }
    return direction2;
}
//Cycles around checking if the x y coordinates ='s the snake coordinates as one of this parts
//One thing to note, a snake of length 4 cannot collide with itself, therefore there is no need to call this function when the snakes length is <= 4
int collisionSnake (int x, int y, int snakeXY[][SNAKE_ARRAY_SIZE], int snakeLength, int detect)
{
    int i;
    //Checks if the snake collided with itself
    for (i = detect; i < snakeLength; i++){
        if ( x == snakeXY[0][i] && y == snakeXY[1][i])
            return 1;
    }
    return 0;
}

//Generates food & Makes sure the food doesn't appear on top of the snake <- This sometimes causes a lag issue!!! Not too much of a problem tho
int generateFood1(int foodXY1[], int snakeXY1[][SNAKE_ARRAY_SIZE], int snakeLength1)
{
    do{
        srand ( time(NULL) );
        foodXY1[0] = rand() % (consoleWidth-3) + 2;
        if(foodXY1[0]%2==1)
            foodXY1[0]++;
        srand ( time(NULL) );
        foodXY1[1] = rand() % (consoleHeight-6) + 2;
    } while (collisionSnake(foodXY1[0], foodXY1[1], snakeXY1, snakeLength1, 0)); //This should prevent the "Food" from being created on top of the snake. - However the food has a chance to be created ontop of the snake, in which case the snake should eat it...
    gotoxy(foodXY1[0] ,foodXY1[1]);
    printf("\033[33mx\033[m");
    return 0;
}

int generateFood2(int foodXY2[], int snakeXY2[][SNAKE_ARRAY_SIZE], int snakeLength2)
{
    do{
        srand ( time(NULL) );
        foodXY2[0] = rand() % (consoleWidth-3) + 2;
        if(foodXY2[0]%2==1)
            foodXY2[0]++;
        srand ( time(NULL) );
        foodXY2[1] = rand() % (consoleHeight-6) + 2;
    } while (collisionSnake(foodXY2[0], foodXY2[1], snakeXY2, snakeLength2, 0)); //This should prevent the "Food" from being created on top of the snake. - However the food has a chance to be created ontop of the snake, in which case the snake should eat it...
    gotoxy(foodXY2[0] ,foodXY2[1]);
    printf("x");
    return 0;
}

void moveSnakeArray1(int snakeXY1[][SNAKE_ARRAY_SIZE], int snakeLength1, int direction1)
{
    int i;
    for( i = snakeLength1-1; i >= 1; i-- ){
        snakeXY1[0][i] = snakeXY1[0][i-1];
        snakeXY1[1][i] = snakeXY1[1][i-1];
    }
/*because we don't actually know the new snakes head x y,
we have to check the direction and add or take from it depending on the direction.*/
    switch(direction1){
    case DOWN1:
        snakeXY1[1][0]++;
        break;
    case RIGHT1:
        snakeXY1[0][0] = snakeXY1[0][0]+2;
        break;
    case UP1:
        snakeXY1[1][0]--;
        break;
    case LEFT1:
        snakeXY1[0][0] = snakeXY1[0][0]-2;
        break;
    
    }
    return;
}

void moveSnakeArray2(int snakeXY2[][SNAKE_ARRAY_SIZE], int snakeLength2, char direction2)
{
    int i;
    for( i = snakeLength2-1; i >= 1; i-- ){
        snakeXY2[0][i] = snakeXY2[0][i-1];
        snakeXY2[1][i] = snakeXY2[1][i-1];
    }
/*because we don't actually know the new snakes head x y,
we have to check the direction and add or take from it depending on the direction.*/
    switch(direction2){
    case DOWN2:
        snakeXY2[1][0]++;
        break;
    case RIGHT2:
        snakeXY2[0][0] = snakeXY2[0][0]+2;
        break;
    case UP2:
        snakeXY2[1][0]--;
        break;
    case LEFT2:
        snakeXY2[0][0] = snakeXY2[0][0]-2;
        break;
    
    }
    return;
}

void move1(int snakeXY1[][SNAKE_ARRAY_SIZE], int snakeLength1, int direction1)
{
    int x;
    int y;
    //Remove the tail ( HAS TO BE DONE BEFORE THE ARRAY IS MOVED!!!!! )
    x = snakeXY1[0][snakeLength1-1];
    y = snakeXY1[1][snakeLength1-1];
    gotoxy(x,y);
    printf(" ");
    //Changes the head of the snake to a body part
    gotoxy(snakeXY1[0][0],snakeXY1[1][0]);
    printf("\033[33m■\033[m");
    moveSnakeArray1(snakeXY1, snakeLength1, direction1);
    switch(direction1){
    case UP1:
        gotoxy(snakeXY1[0][0],snakeXY1[1][0]);
        printf("\033[33m▲\033[m");
        break;
    case DOWN1:
        gotoxy(snakeXY1[0][0],snakeXY1[1][0]);
        printf("\033[33m▼\033[m");
        break;
    case RIGHT1:
        gotoxy(snakeXY1[0][0],snakeXY1[1][0]);
        printf("\033[33m▶\033[m");
        break;
    case LEFT1:
        gotoxy(snakeXY1[0][0],snakeXY1[1][0]);
        printf("\033[33m◀\033[m");
        break;
    default:
        break;    
    }
    gotoxy(100,100); //Gets rid of the darn flashing underscore.
    
    return;
}

void move2(int snakeXY2[][SNAKE_ARRAY_SIZE], int snakeLength2, char direction2)
{
    int x;
    int y;

    //Remove the tail ( HAS TO BE DONE BEFORE THE ARRAY IS MOVED!!!!! )
    x = snakeXY2[0][snakeLength2-1];
    y = snakeXY2[1][snakeLength2-1];

    gotoxy(x,y);
    printf(" ");
    gotoxy(snakeXY2[0][0],snakeXY2[1][0]);
    printf("■");
    moveSnakeArray2(snakeXY2, snakeLength2, direction2);
    switch(direction2){
    case UP2:
    case 'W':
        gotoxy(snakeXY2[0][0],snakeXY2[1][0]);
        printf("▲");
        break;
    case DOWN2:
    case 'S':
        gotoxy(snakeXY2[0][0],snakeXY2[1][0]);
        printf("▼");
        break;
    case RIGHT2:
    case 'D':
        gotoxy(snakeXY2[0][0],snakeXY2[1][0]);
        printf("▶");
        break;
    case LEFT2:
    case 'A':
        gotoxy(snakeXY2[0][0],snakeXY2[1][0]);
        printf("◀");
        break;
    default:
        break;
    }
    gotoxy(100,100); //Gets rid of the darn flashing underscore.
    return;
}

//This function checks if the snakes head is on top of the food, if it is then it'll generate some more food...
int eatFood1(int snakeXY1[][SNAKE_ARRAY_SIZE], int foodXY1[],int direction1)
{
    switch(direction1){
    case DOWN1:
        if (snakeXY1[0][0] == foodXY1[0] && (snakeXY1[1][0] == foodXY1[1]||snakeXY1[1][0] - 1 == foodXY1[1])){
            if(snakeXY1[1][0] - 1 == foodXY1[1]){
                gotoxy(foodXY1[0],foodXY1[1]);
                printf(" ");
            }
            foodXY1[0] = 0;
            foodXY1[1] = 0;
            printf("\7");
            return 1;
        }
        else
            return 0;
    case UP1:
        if (snakeXY1[0][0] == foodXY1[0] && (snakeXY1[1][0] == foodXY1[1]||snakeXY1[1][0] + 1 == foodXY1[1])){
            if(snakeXY1[1][0] + 1 == foodXY1[1]){
                gotoxy(foodXY1[0],foodXY1[1]);
                printf(" ");
            }
            foodXY1[0] = 0;
            foodXY1[1] = 0;
            printf("\7");
            return 1;
        }
        else
            return 0;
    case LEFT1:
        if ((snakeXY1[0][0] == foodXY1[0] || snakeXY1[0][0] + 1 == foodXY1[0]) && snakeXY1[1][0] == foodXY1[1]){
            if(snakeXY1[0][0] + 1 == foodXY1[0]){
                gotoxy(foodXY1[0],foodXY1[1]);
                printf(" ");
            }
            foodXY1[0] = 0;
            foodXY1[1] = 0;
            printf("\7");
            return 1;
        }
        else
            return 0;
    case RIGHT1:
        if ((snakeXY1[0][0] == foodXY1[0] || snakeXY1[0][0] - 1 == foodXY1[0]) && snakeXY1[1][0] == foodXY1[1]){
            if(snakeXY1[0][0] - 1 == foodXY1[0]){
                gotoxy(foodXY1[0],foodXY1[1]);
                printf(" ");
            }
            foodXY1[0] = 0;
            foodXY1[1] = 0;
            printf("\7");
            return 1;
        }
        else
            return 0;
    default:
        break;
    }
}

int eatFood2(int snakeXY2[][SNAKE_ARRAY_SIZE], int foodXY2[],char direction2)
{
    switch(direction2){
    case DOWN2:
        if (snakeXY2[0][0] == foodXY2[0] && (snakeXY2[1][0] == foodXY2[1]||snakeXY2[1][0] - 1 == foodXY2[1])){
            if(snakeXY2[1][0] - 1 == foodXY2[1]){
                gotoxy(foodXY2[0],foodXY2[1]);
                printf(" ");
            }
            foodXY2[0] = 0;
            foodXY2[1] = 0;
            printf("\7");
            return 1;
        }
        else
            return 0;
    case UP2:
        if (snakeXY2[0][0] == foodXY2[0] && (snakeXY2[1][0] == foodXY2[1]||snakeXY2[1][0] + 1 == foodXY2[1])){
            if(snakeXY2[1][0] + 1 == foodXY2[1]){
                gotoxy(foodXY2[0],foodXY2[1]);
                printf(" ");
            }
            foodXY2[0] = 0;
            foodXY2[1] = 0;
            printf("\7");
            return 1;
        }
        else
            return 0;
    case LEFT2:
        if ((snakeXY2[0][0] == foodXY2[0] || snakeXY2[0][0] + 1 == foodXY2[0]) && snakeXY2[1][0] == foodXY2[1]){
            if(snakeXY2[0][0] + 1 == foodXY2[0]){
                gotoxy(foodXY2[0],foodXY2[1]);
                printf(" ");
            }
            foodXY2[0] = 0;
            foodXY2[1] = 0;
            printf("\7");
            return 1;
        }
        else
            return 0;
    case RIGHT2:
        if ((snakeXY2[0][0] == foodXY2[0] || snakeXY2[0][0] - 1 == foodXY2[0]) && snakeXY2[1][0] == foodXY2[1]){
            if(snakeXY2[0][0] - 1 == foodXY2[0]){
                gotoxy(foodXY2[0],foodXY2[1]);
                printf(" ");
            }
            foodXY2[0] = 0;
            foodXY2[1] = 0;
            printf("\7");
            return 1;
        }
        else
            return 0;
    default:
        break;
    }
}

int collisionDetection1(int snakeXY1[][SNAKE_ARRAY_SIZE], int snakeLength1 ) //Need to Clean this up a bit
{
    int colision1 = 0;
    if ((snakeXY1[0][0] == 1) || (snakeXY1[0][0] + 1 == 1) || (snakeXY1[1][0] == 1) || (snakeXY1[0][0] == consoleWidth) || (snakeXY1[1][0] == consoleHeight - 4)) //Checks if the snake collided wit the wall or it's self
        colision1 = 1;
    else{
        if (collisionSnake(snakeXY1[0][0], snakeXY1[1][0], snakeXY1, snakeLength1, 1)||collisionSnake(snakeXY1[0][0], snakeXY1[1][0], snakeXY2, snakeLength2, 1)) //If the snake collided with the wall, there's no point in checking if it collided with itself.
            colision1 = 1;
        else if(snakeXY1[0][0]==snakeXY2[0][0] && snakeXY1[1][0]==snakeXY2[1][0]){
            if(snakeLength1 > snakeLength2)
                colision1 = 0;
            else if(snakeLength2 > snakeLength1)
                colision1 = 1;
            else if(snakeLength1==snakeLength2){
                switch(direction1){
                case LEFT1:
                    direction1==UP1;
                    move1(snakeXY1, snakeLength1, direction1);
                    break;
                case RIGHT1:
                    direction2==DOWN1;
                    move1(snakeXY1, snakeLength1, direction1);
                    break;
                case UP1:
                    direction1==RIGHT1;
                    move1(snakeXY1, snakeLength1, direction1);
                    break;
                case DOWN1:
                    direction2==LEFT1;
                    move1(snakeXY1, snakeLength1, direction1);
                    break;
                default:
                    break;
                }
                colision1=0;
            }
        }
    }
    return colision1;
}

int collisionDetection2(int snakeXY2[][SNAKE_ARRAY_SIZE], int snakeLength2) //Need to Clean this up a bit
{
    int colision2 = 0;
    if((snakeXY2[0][0] == 1) || (snakeXY2[0][0] + 1 == 1) || (snakeXY2[1][0] == 1) || (snakeXY2[0][0] == consoleWidth) || (snakeXY2[1][0] == consoleHeight - 4)) //Checks if the snake collided wit the wall or it's self
        colision2 = 1;
    else{
        if(collisionSnake(snakeXY2[0][0], snakeXY2[1][0], snakeXY2, snakeLength2, 1)||collisionSnake(snakeXY2[0][0], snakeXY2[1][0], snakeXY1, snakeLength1, 1)) //If the snake collided with the wall, there's no point in checking if it collided with itself.
            colision2 = 1;
        else if(snakeXY1[0][0]==snakeXY2[0][0] && snakeXY1[1][0]==snakeXY2[1][0]){
            if(snakeLength1 > snakeLength2)
                colision2 = 1;
            else if(snakeLength2 > snakeLength1)
                colision2 = 0;
            else if(snakeLength1==snakeLength2){
                switch(direction2){
                case LEFT2:
                    direction2==UP2;
                    move2(snakeXY2, snakeLength2, direction2);
                    break;
                case RIGHT2:
                    direction2==DOWN2;
                    move2(snakeXY2, snakeLength2, direction2);
                    break;
                case UP2:
                    direction2==RIGHT2;
                    move2(snakeXY2, snakeLength2, direction2);
                    break;
                case DOWN2:
                    direction2==LEFT2;
                    move2(snakeXY2, snakeLength2, direction2);
                    break;
                default:
                    break;
                }
                colision2=0;
            }
        }
    }
    return colision2;
}

void refreshInfoBar1()
{
    gotoxy(20,37);
    printf("Score1: %d", score1);
    gotoxy(120,37);
    printf("Speed1: %d", speed1);
    return;
}

void refreshInfoBar2()
{
    gotoxy(20,38);
    printf("Score2: %d", score2);
    gotoxy(120,38);
    printf("Speed2: %d", speed2);
    return;
}


void gameOverScreen()
{
    int x = 52, y = 9;
    gotoxy(x,y++);
    printf(":'######::::::'###::::'##::::'##:'########:\n");
    gotoxy(x,y++);
    printf("'##... ##::::'## ##::: ###::'###: ##.....::\n");
    gotoxy(x,y++);
    printf(" ##:::..::::'##:. ##:: ####'####: ##:::::::\n");
    gotoxy(x,y++);
    printf(" ##::'####:'##:::. ##: ## ### ##: ######:::\n");
    gotoxy(x,y++);
    printf(" ##::: ##:: #########: ##. #: ##: ##...::::\n");
    gotoxy(x,y++);
    printf(" ##::: ##:: ##.... ##: ##:.:: ##: ##:::::::\n");
    gotoxy(x,y++);
    printf(". ######::: ##:::: ##: ##:::: ##: ########:\n");
    gotoxy(x,y++);
    printf(":......::::..:::::..::..:::::..::........::\n");
    gotoxy(x,y++);
    printf(":'#######::'##::::'##:'########:'########::'####:\n");
    gotoxy(x,y++);
    printf("'##.... ##: ##:::: ##: ##.....:: ##.... ##: ####:\n");
    gotoxy(x,y++);
    printf(" ##:::: ##: ##:::: ##: ##::::::: ##:::: ##: ####:\n");
    gotoxy(x,y++);
    printf(" ##:::: ##: ##:::: ##: ######::: ########::: ##::\n");
    gotoxy(x,y++);
    printf(" ##:::: ##:. ##:: ##:: ##...:::: ##.. ##::::..:::\n");
    gotoxy(x,y++);
    printf(" ##:::: ##::. ## ##::: ##::::::: ##::. ##::'####:\n");
    gotoxy(x,y++);
    printf(". #######::::. ###:::: ########: ##:::. ##: ####:\n");
    gotoxy(x,y++);
    printf(":.......::::::...:::::........::..:::::..::....::\n");
    sleep(1);
    system("clear");
    return;
}

void loadEnviroment()//This can be done in a better way... FIX ME!!!! Also i think it doesn't work properly in ubuntu <- Fixed
{
    int x=1, y=1;
    int rectangleHeight = consoleHeight - 4;
    system("clear"); //clear the console
    gotoxy(1,1); //Top left corner
    printf("╔");
    gotoxy(1,rectangleHeight);
    printf("╚");
    gotoxy(consoleWidth,1);
    printf("╗");
    gotoxy(consoleWidth,rectangleHeight);
    printf("╝");
    for (y=2; y < rectangleHeight; y++){
        gotoxy(x, y); //Left Wall
        printf("║");
        gotoxy(consoleWidth, y); //Right Wall
        printf("║");
    }
    y = 1;
    for (x=2; x < consoleWidth; x++){
        gotoxy(x, y); //ceiling
        printf("═");
        gotoxy(x, rectangleHeight); //floor
        printf("═");
    }
    return;
}

//Messy, need to clean this function up
void startGame1( int snakeXY1[][SNAKE_ARRAY_SIZE], int foodXY1[], int foodXY2[], int snakeLength1, int direction1)
{
    clock_t endWait1;
    int waittime1 = 180000;  //Sets the correct wait time according to the selected speed
    int tempScore1 = 10 * speed1;
    int oldDirection1;
    int canChangeDirection1 = 1;
    endWait1 = clock() + waittime1;
    savegame();
    while (gameOver1==0){
        if(canChangeDirection1){
            oldDirection1 = direction1;
            direction1 = checkKeysPressed1();
        }
        if(oldDirection1 != direction1)//Temp fix to prevent the snake from colliding with itself
            canChangeDirection1 = 0;
        if(clock() >= endWait1){ //it moves according to how fast the computer running it is...
            move1(snakeXY1, snakeLength1, direction1);
            canChangeDirection1 = 1;
            if(eatFood1(snakeXY1, foodXY1,direction1)){
                generateFood1(foodXY1,snakeXY1, snakeLength1); //Generate More Food
                snakeLength1++;
                score1+=10;
                if( score1 >= 10 * speed1 + tempScore1){
                    tempScore1 = score1;
                    if(speed1 < 6){
                        speed1++;
                        waittime1 = waittime1 - 20000;
                    }

                }
                refreshInfoBar1();
            }
            else{
                gotoxy(foodXY1[0],foodXY1[1]);
                printf("\033[33mx\033[m");
                gotoxy(100,100);
            }
            savegame();
            endWait1 = clock() + waittime1; 
        }
        gameOver1 = collisionDetection1(snakeXY1,snakeLength1);
        if(snakeLength1 >= SNAKE_ARRAY_SIZE-5){ //Just to make sure it doesn't get longer then the array size & crash
            gameOver1 = 2;//You Win! <- doesn't seem to work - NEED TO FIX/TEST THIS
        }
    }
    switch(gameOver1){
    case 1:
        printf("\7"); //Beep
        printf("\7"); //Beep
        gameOverScreen(snakeLength1);
        if(gameOver2==0){
            loadEnviroment();
            gotoxy(foodXY2[0],foodXY2[1]);
            printf("x");
            refreshInfoBar1();
            refreshInfoBar2();
        }
        break;
    case 2:
        gotoxy(69,20);
        printf("PLAYER1 WINS!\n");
    default:
        break;
    }
    return;
}   

void startGame2( int snakeXY2[][SNAKE_ARRAY_SIZE], int foodXY2[], int foodXY1[],int snakeLength2, char direction2)
{
    clock_t endWait2;
    int waittime2 = 180000;  //Sets the correct wait time according to the selected speed
    int tempScore2 = 10 * speed2;
    int oldDirection2;
    int canChangeDirection2 = 1;
    endWait2 = clock() + waittime2;
    while (gameOver2==0){
        if(canChangeDirection2){
            oldDirection2 = direction2;
            direction2 = checkKeysPressed2();
        }
        if(oldDirection2 != direction2)//Temp fix to prevent the snake from colliding with itself
            canChangeDirection2 = 0;
        if(clock() >= endWait2){ //it moves according to how fast the computer running it is...
            move2(snakeXY2, snakeLength2, direction2);
            canChangeDirection2 = 1;
            if(eatFood2(snakeXY2, foodXY2,direction2)){
                generateFood2( foodXY2, snakeXY2, snakeLength2); //Generate More Food
                snakeLength2++;
                score2+=10;
                if( score2 >= 10 * speed2 + tempScore2){
                    tempScore2 = score2;
                    if(speed2 < 6){
                        speed2++;
                        waittime2 = waittime2 - 20000;
                    }

                }
                refreshInfoBar2(score2, speed2);
            }
            else{
                gotoxy(foodXY2[0],foodXY2[1]);
                printf("x");
                gotoxy(100,100);
            }
            endWait2 = clock() + waittime2; 
        }
        gameOver2 = collisionDetection2(snakeXY2, snakeLength2);
        if(snakeLength2 >= SNAKE_ARRAY_SIZE-5){ //Just to make sure it doesn't get longer then the array size & crash
            gameOver2 = 2;//You Win! <- doesn't seem to work - NEED TO FIX/TEST THIS
        }
    }
    switch(gameOver2){
    case 1:
        printf("\7"); //Beep
        printf("\7"); //Beep
        gameOverScreen(snakeLength2);
        if(gameOver1==0){
            loadEnviroment();
            gotoxy(foodXY1[0],foodXY1[1]);
            printf("\033[33mx\033[m");
            refreshInfoBar1(score1, speed1);
            refreshInfoBar2(score2, speed2);
        }
        break;
    case 2:
        gotoxy(69,20);
        printf("PLAYER2 WINS!\n");
    default:
        break;
    }
    return;
}

void loadSnake1(int snakeXY1[][SNAKE_ARRAY_SIZE], int snakeLength1)
{
    int i;
    for (i = 0; i < snakeLength1; i++){
        gotoxy(snakeXY1[0][i], snakeXY1[1][i]);
        printf("\033[33m■\033[m"); //Meh, at some point I should make it so the snake starts off with a head...
    }
    return;
}

void loadSnake2(int snakeXY2[][SNAKE_ARRAY_SIZE], int snakeLength2)
{
    int i;
    for (i = 0; i < snakeLength2; i++){
        gotoxy(snakeXY2[0][i], snakeXY2[1][i]);
        printf("■"); //Meh, at some point I should make it so the snake starts off with a head...
    }
    return;
}

void prepairSnakeArray(int snakeXY[][SNAKE_ARRAY_SIZE], int snakeLength)
{
    int i;
    int snakeX = snakeXY[0][0];
    int snakeY = snakeXY[1][0];
    for(i = 1; i <= snakeLength; i++){
        snakeXY[0][i] = snakeX + 2*i;
        snakeXY[1][i] = snakeY;
    }
    return;
}

//This function loads the environment, snake, etc
void loadGame1()
{
    // int snakeXY1[2][SNAKE_ARRAY_SIZE]; //Two Dimensional Array, the first array is for the X coordinates and the second array for the Y coordinates
    // int snakeLength1 = 4; //Starting Length
    // int direction1 = LEFT1; //DO NOT CHANGE THIS TO RIGHT ARROW, THE GAME WILL INSTANTLY BE OVER IF YOU DO!!! <- Unless the prepareSnakeArray function is changed to take into account the direction....
    system("clear");
    //The starting location of the snake
    snakeXY1[0][0] = 40;
    snakeXY1[1][0] = 10;
    loadEnviroment(); //borders
    prepairSnakeArray(snakeXY1, snakeLength1);
    loadSnake1(snakeXY1, snakeLength1);
    foodXY1[0] = 70;
    foodXY1[1] = 20;
    gotoxy(foodXY1[0],foodXY1[1]);
    printf("\033[33mx\033[m");
    refreshInfoBar1(score1, speed1); //Bottom info bar. Score, Level etc
    startGame1(snakeXY1, foodXY1, foodXY2, snakeLength1, direction1);
    return;
    // pthread_exit(NULL);
}

void *loadGame2()
{
    int snakeXY2[2][SNAKE_ARRAY_SIZE]; //Two Dimensional Array, the first array is for the X coordinates and the second array for the Y coordinates
    int snakeLength2 = 4; //Starting Length
    char direction2 = LEFT2; //DO NOT CHANGE THIS TO RIGHT ARROW, THE GAME WILL INSTANTLY BE OVER IF YOU DO!!! <- Unless the prepareSnakeArray function is changed to take into account the direction....
    system("clear");
    //The starting location of the snake
    snakeXY2[0][0] = 40;
    snakeXY2[1][0] = 9;
    loadEnviroment(); //borders
    prepairSnakeArray(snakeXY2, snakeLength2);
    loadSnake2(snakeXY2, snakeLength2);
    foodXY2[0] = 75;
    foodXY2[1] = 25;
    gotoxy(foodXY2[0],foodXY2[1]);
    printf("x");
    refreshInfoBar2(score2, speed2); //Bottom info bar. Score, Level etc
    startGame2(snakeXY2, foodXY2, foodXY1, snakeLength2, direction2);
    // pthread_cancel(loadGame1);
    pthread_exit(NULL);
}


void replaymove(int head[2], int nail[2],int Length, int dir)
{
    //Remove the tail ( HAS TO BE DONE BEFORE THE ARRAY IS MOVED!!!!! )
    if(previous->length==current->length){
        gotoxy(nail[0],nail[1]);
        printf(" ");
    }
    gotoxy(head[0],head[1]);
    printf("\033[33m■\033[m");
    head[0]=current->next->head[0];
    head[1]=current->next->head[1];

    switch(dir){
    case UP1:
        gotoxy(head[0],head[1]);
        printf("\033[33m▲\033[m");
        break;
    case DOWN1:
        gotoxy(head[0],head[1]);
        printf("\033[33m▼\033[m");
        break;
    case RIGHT1:
        gotoxy(head[0],head[1]);
        printf("\033[33m▶\033[m");
        break;
    case LEFT1:
        gotoxy(head[0],head[1]);
        printf("\033[33m◀\033[m");
        break;
    default:
        break;    
    }
    gotoxy(100,100); //Gets rid of the darn flashing underscore.
    return;
}
void savegame(){
    int i,j;
    current=(Node*) malloc(sizeof(Node));
    current->head[0]=snakeXY1[0][0];
    current->head[1]=snakeXY1[1][0];
    current->nail[0]=snakeXY1[0][snakeLength1-1];
    current->nail[1]=snakeXY1[1][snakeLength1-1];
    current->length=snakeLength1;
    current->foodx=foodXY1[0];
    current->foody=foodXY1[1];
    current->dir=direction1;
    current->speed=speed1;
    current->waitime=waittime1;
    current->score=score1;
    if(count==0){
        first=current;
        count++;
    }
    else    previous->next=current;
    current->next=NULL;
    current->pre=previous;
    previous=current;
}
void prepairSnakeArray1(int snakeXY[][SNAKE_ARRAY_SIZE], int snakeLength)
{
    int i;
    int snakeX = snakeXY[0][0];
    int snakeY = snakeXY[1][0];
    for(i = 1; i <= snakeLength; i++){
        snakeXY[0][i] = snakeX + 2*i;
        snakeXY[1][i] = snakeY;
    }
    return;
}
void loadSnake(int snakeXY1[][SNAKE_ARRAY_SIZE], int snakeLength1)
{
    int i;
    gotoxy(snakeXY1[0][0],snakeXY1[1][0]);
        printf("\033[33m◀\033[m");  
    for (i = 1; i < snakeLength1; i++){
        gotoxy(snakeXY1[0][i], snakeXY1[1][i]);
        printf("\033[33m■\033[m"); //Meh, at some point I should make it so the snake starts off with a head...
    }
    return;
}
void replay(){
    char c;
    clock_t endWait;
    int tempScore1 = 10 * speed1;
    system("clear");
    // printf("Do you want to review ? [y/n]\n");
    // scanf("%c",&c);
    // if(c=='y'||c=='Y'){
        current=first;
        direction1=current->dir;
        snakeLength1=4;
        speed1=current->speed;
        snakeXY1[0][0] = 40;
        snakeXY1[1][0] = 10;
        waittime1 = 180000;
        loadEnviroment(); //borders
        prepairSnakeArray1(snakeXY1, snakeLength1);
        loadSnake(snakeXY1, snakeLength1);
        foodXY1[0] = 70;
        foodXY1[1] = 20;
        gotoxy(foodXY1[0],foodXY1[1]);
        printf("\033[33mx\033[m");
        refreshInfoBar1(score1, current->speed); //Bottom info bar. Score, Level etc
        endWait = clock() + current->waitime;

        while (current->next!=NULL){
            score1=current->score;
            if(clock() >= endWait){ //it moves according to how fast the computer running it is...
                replaymove(current->head, current->nail, current->length, current->dir);
                gotoxy(current->foodx,current->foody); //Generate More Food
                printf("\033[33mx\033[m");
                refreshInfoBar1();
                current=current->next;
                endWait = clock() + current->waitime;
            } 
        }
        printf("\7"); //Beep
        printf("\7"); //Beep
        gameOverScreen(snakeLength1);
        return;
    // }
    // else if(c=='n'||c=='N'){
        // return;
    // }
}






int main() //Need to fix this up
{
    char c;
    pthread_create(&snake2,NULL,loadGame2,NULL);
    pthread_detach(snake2);
    loadGame1();
    replay();
    // printf("Do you want to review ? [y/n]\n");
    // fflush(stdin);
    // scanf("%c",&c);
    // printf("c = %c",c);
    // if(c=='y'||c=='Y'){
    //     replay();
    // }
    // else if(c=='n'||c=='N')   
    // {
        return 0;
    // }
}