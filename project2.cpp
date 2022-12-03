#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <deque>
using namespace std;

//creating deque used for holding cards in the deck
deque<int> deck;

//global variables
int numOfRounds = 2;
int roundNum = 1;
const int numOfCards = 52;
int seed = 0;
bool roundWin = false;
int playersTurn = 0;
FILE * pFile;

//intitializing a hand array for each player
int hand1 [2];
int hand2 [2];
int hand3 [2];
int hand4 [2];

//pthread variables for all players
pthread_t playerThread1;
pthread_t playerThread2;
pthread_t playerThread3;
pthread_t playerThread4;
pthread_t dealerThread;

//mutex and condition variables for the threads
pthread_mutex_t playerLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
pthread_mutex_t dealerLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t winCondition = PTHREAD_COND_INITIALIZER;


void parseArg(char *argv[]);
void seedRand();
void createDeck();
void shuffle();
void startRound();
void deal();
void *dealer(void *d);
void *player(void *p);
void playTurn(int player);


int main(int argc, char *argv[]){

    //explain usage
    if (argc < 2 || argc > 2 ){
        cout << "Must enter a single non-negative integer after program name argument." << endl;
        cout << "This integer will be used to seed the random number generation." << endl;
        exit(0);
    }
    //for log file
    pFile = fopen("log.txt", "a");
    //parse arguments and seed the random number generation
    parseArg(argv);
    seedRand();
    //start the rounds
    while (roundNum <= 2){
        createDeck();
        startRound();
        roundNum++;
        roundWin = false;
    }
}

void parseArg(char *argv[]){
    seed = atoi(argv[1]);
}

void seedRand(){
    srand(seed);
}

void createDeck(){
    for (int i = 0; i < 52; i++){
        deck.push_back((i%13)+1);
    }

}

void shuffle(){
    int numCards = numOfCards;
    //swaps each card in the deck with another card at random position
    for (int i = 0; i < (numCards-1); i++){
        int currCard = deck.at(i);
        int randNumber = rand () % (numCards - i) + i;
        deck.at(i) = deck.at(randNumber);
        deck.at(randNumber) = currCard;
    }
}

void deal(){

    if (roundNum == 1){
        hand1[0] = deck.front();
        deck.pop_front();
        hand2[0] = deck.front();
        deck.pop_front();
        hand3[0] = deck.front();
        deck.pop_front();
        hand4[0] = deck.front();
        deck.pop_front(); 
    }else {
        hand2[0] = deck.front();
        deck.pop_front();
        hand3[0] = deck.front();
        deck.pop_front();
        hand4[0] = deck.front();
        deck.pop_front();
        hand1[0] = deck.front();
        deck.pop_front();
    }
}

void startRound(){
    cout << "Round " << roundNum << " has started." <<endl;
    

    //creating the threads
    pthread_create(&dealerThread, NULL, &dealer, NULL);
    pthread_create(&playerThread1, NULL, &player, (void *)1); 
    pthread_create(&playerThread2, NULL, &player, (void *)2); 
    pthread_create(&playerThread3, NULL, &player, (void *)3); 
    pthread_create(&playerThread4, NULL, &player, (void *)4); 

    //joining the threads
    pthread_join(dealerThread, NULL);
    pthread_join(playerThread1, NULL);
    pthread_join(playerThread2, NULL);
    pthread_join(playerThread3, NULL);
    pthread_join(playerThread4, NULL);

}

void *dealer(void *arg){
    shuffle();
    cout << "DEALER: shuffles cards" << endl;
    deal();
    cout << "DEALER: deals cards" << endl; 
    if (roundNum == 1){
        playersTurn = 1;
    } else{
        playersTurn = 2;
    }

    //broadcasts that the players can use the deck
    pthread_cond_broadcast(&condition);
    // lock the dealer thread
    pthread_mutex_lock(&dealerLock);
    //loop and wait for player to win 
    while (roundWin == false){
        pthread_cond_wait(&winCondition, &dealerLock);
    }
    //then unlock the dealer and exit the dealer thread
    pthread_mutex_unlock(&dealerLock);
    cout << "DEALER: exits round" << endl;
    pthread_exit(NULL);

    return NULL;

}


void *player(void *pl){

    //keep track of the player
    int currPlayer = *(int *)&pl;

    //loop until round is won
    while (roundWin == false){
        //lock player
        pthread_mutex_lock(&playerLock);
        //loop until its players turn or round is won
        while (currPlayer != playersTurn && roundWin == false){
            //waiting for signal so they can play their turn
            pthread_cond_wait(&condition, &playerLock);
        }
        //round not won so player takes their turn
        if (roundWin == false){
            playTurn(currPlayer);
        }
        //unlock player thread
        pthread_mutex_unlock(&playerLock);

    
    }
    cout << "PLAYER " << currPlayer << ": exits round" << endl;
    pthread_exit(NULL);

    return NULL;
    
}

void playTurn(int player){

    //keep track of current players hand
    int hand [2];
    if (player == 1){
        hand[0] = hand1[0];
    }else if (player == 2){
        hand[0] = hand2[0];
    }else if (player == 3){
        hand[0] = hand3[0];
    }else if (player == 4){
        hand[0] = hand4[0];
    }
    
    cout << "PLAYER " << player << ": hand ";
    cout << hand[0];
    cout << endl;

    //establish the hand of the player's teammate
    int teammateHand[2];

    if (player == 1){
        for (int i = 0; i < 2; i++) {
        teammateHand[i] = hand3[i];
        }
    }else if (player == 3){
        for (int i = 0; i < 2; i++) {
        teammateHand[i] = hand1[i];
        }
    }else if (player ==2 ){
        for (int i = 0; i < 2; i++) {
        teammateHand[i] = hand4[i];
        }
    }else {
        for (int i = 0; i < 2; i++) {
        teammateHand[i] = hand2[i];
        }
    }

    //set the second card in player hand to card drawn from deck
    hand[1] = deck.front();
    cout << "PLAYER " << player << ": draws " << deck.front() << endl;
    //remove card from top of deck
    deck.pop_front();
    
    //compare player hand to teammate hand
    if (hand[0] == teammateHand[0] || hand[1] == teammateHand[0] ){
        roundWin = true;
        switch(player){
            case 1:
                cout << "PLAYER " << player << ": hand " << "(" << hand[0] << "," << 
                hand[1] << ") <> PLAYER 3 has " << teammateHand[0] << endl;
                break;
            case 2:
                cout << "PLAYER " << player << ": hand " << "(" << hand[0] << "," << 
                hand[1] << ") <> PLAYER 4 has " << teammateHand[0] << endl;
                break;
            case 3: 
                cout << "PLAYER " << player << ": hand " << "(" << hand[0] << "," << 
                hand[1] << ") <> PLAYER 1 has " << teammateHand[0] << endl;
                break;
            case 4: 
                cout << "PLAYER " << player << ": hand " << "(" << hand[0] << "," << 
                hand[1] << ") <> PLAYER 2 has " << teammateHand[0] << endl;
                break; 
        }
        //signal that the round has been won
        pthread_cond_signal(&winCondition);

    }else {
        //randomly choose card to put on bottom of deck
        int randCard = rand() % 2;
        cout << "PLAYER " << player << ": discards " << hand[randCard] << " at random" << endl;
        deck.push_back(hand[randCard]);
        if (randCard == 0){
            //set the second card back to position 0 in hand array for convenience
            hand[0] = hand[1];
        }else {
            //set the second card as a number that is not even in deck (so basically empty card)
            hand[1] = 15;
        }
        cout << "PLAYER " << player << ": hand " << hand[0] << endl;
        //update global hand array for teammate
        switch(player){
            case 1:
                hand1[0] = hand[0];
                break;
            case 2:
                hand2[0] = hand[0];
                break;
            case 3:
                hand3[0] = hand[0];
                break;
            case 4:
                hand4[0] = hand[0];
        }
    }
    cout << "DECK: ";
    for (auto it = deck.begin(); it != deck.end(); ++it)
        cout << ' ' << *it;
    cout << endl;

    //move to next player in round robin fashion
    playersTurn++;
    if (playersTurn > 4){
        playersTurn = 1;
    }
    //broadcast that the next player is free to play their turn
    pthread_cond_broadcast(&condition);
}