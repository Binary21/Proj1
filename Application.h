/*
 * Application.h
 *
 *  Created on: Dec 29, 2019
 *      Author: Matthew Zhong
 *  Supervisor: Leyla Nazhand-Ali
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <HAL/HAL.h>

// directional enums
enum _Direction
{
    UP = 'w', LEFT = 'a', RIGHT = 'd', DOWN = 's', FEED = 'F', NOTHING
};
typedef enum _Direction Direction;
// enums for happiness and energy
enum _Value
{
    EMPTY, FIRST_LEVEL, SECOND_LEVEL, THIRD_LEVEL, FOURTH_LEVEL, FULL
};
typedef enum _Value Value;
// age enums
enum _Age
{
    EGG, CHILD, ADULT
};
typedef enum _Age Age;



struct _Application
{
    // Put your application members and FSM state variables here!
    // =========================================================================
    UART_Baudrate baudChoice;
    Direction direction;
    Age ageVisual;
    Value valueEnergy;
    Value valueHappy;
    bool firstCall;
    char rChar;
    char tChar;
    int xdirection;
    int ydirection;
    int radius;
    int age;
    int energyLevel;
    int happyLevel;
    int totalMoves;
    Graphics_Rectangle R_HAPPY, R_ENERGY;

    SWTimer threeSecondTimer;
    //_appSmallFSMstate smallFSMstate;
};
typedef struct _Application Application;

// Called only a single time - inside of main(), where the application is constructed
Application Application_construct();

// Called once per super-loop of the main application.
void Application_loop(Application* app, HAL* hal);

// Called whenever the UART module needs to be updated
void Application_updateCommunications(Application* app, HAL* hal);

// Interprets an incoming character and echoes back to terminal what kind of
// character was received (number, letter, or other)
char Application_interpretIncomingChar(char);

// Generic circular increment function
uint32_t CircularIncrement(uint32_t value, uint32_t maximum);
// assigns the message from UART to a variable
void UART_messageCheck(Application* app_p, HAL* hal_p);
// this to move the Tamagchi depending on the input from UART
void Application_characterLocation(Application* app_p, HAL* hal_p);
// this function controls all the drawings of the game screen aside from the tamagachi
void Application_showGamecreen(Application* app_p, HAL* hal_p);
// assigns necessary initial variables before the tamagachi game begins
void InitialApplicationValues(Application* app_p);
// converst the tamagachi energy values to enums and location
void Application_EnergyValues(Application* app_p, HAL* hal_p);
// converst the tamagachi energy values to enums and location
void Application_HappyValues(Application* app_p, HAL* hal_p);
// draws the circle and fills it in
void Application_DrawAndFillCircle(Application* app_p, HAL* hal_p);
// updates happines value
void Application_HappinesRecalculator(Application* app_p, HAL* hal_p);
// converst directional input from UART to enums
void Application_LetterToDirection(Application* app_p);
// manages the tamagachi being drawn on screen
void Application_manageScreen(Application* app_p, HAL* hal_p);
// shows the end screen and the "GAME OVER" title
void Application_EndScreen(HAL* hal_p);


#endif /* APPLICATION_H_ */
