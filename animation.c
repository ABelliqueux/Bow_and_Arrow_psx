 
// pad stuff

typedef struct {
    u_short u0, v0;       // coordinates of top-left corner of uv
    //~ short pause;
    short Xoffset;
    short Yoffset;
    short index;
    } frame;
    
typedef struct {
    char timer;
    char animCursor;
    char hit;
    frame *anim;
    } charSprite;

frame iddle[1];
frame aim[6];
frame jump[6];
frame reload[6];
frame reload_arrow[3];
frame arrow[1];
frame balloon[8];
frame ammunitions[1];
frame parchment[1];
frame messages[2];
frame bubble[1];
frame bee[3];
frame target[1];
frame menu[1];
frame inst[1];

frame *currentFrame;

void setAnimSheet(char line, frame array[], short arraySize, char gridWidth, char gridHeight, short Xoffset, short Yoffset){
        for (int i = 0; i < arraySize ;i++){
            array[i].u0 = (i    * gridWidth ) + Xoffset; 
            array[i].v0 = (line * gridHeight) + Yoffset;
            
            //array[i].pause = 40;
            
            array[i].Xoffset = 0;
            array[i].Yoffset = 0;
            
            array[i].index = i;
        }

}
