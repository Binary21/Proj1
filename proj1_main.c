/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* HAL and Application includes */
#include <Application.h>
#include <HAL/HAL.h>
#include <HAL/Timer.h>

#define move_x 15
#define move_y 10

#define bRIGHT 64
#define bLEFT 44
#define bUP 44
#define bDOWN 74

/**
 * The main entry point of your project. The main function should immediately
 * stop the Watchdog timer, call the Application constructor, and then
 * repeatedly call the main super-loop function. The Application constructor
 * should be responsible for initializing all hardware components as well as all
 * other finite state machines you choose to use in this project.
 *
 * THIS FUNCTION IS ALREADY COMPLETE. Unless you want to temporarily experiment
 * with some behavior of a code snippet you may have, we DO NOT RECOMMEND
 * modifying this function in any way.
 */



int main(void)
{

    // Stop Watchdog Timer - THIS SHOULD ALWAYS BE THE FIRST LINE OF YOUR MAIN
    WDT_A_holdTimer();

    // Initialize the system clock and background hardware timer, used to enable
    // software timers to time their measurements properly.
    InitSystemTiming();

    // Initialize the main Application object and HAL object
    HAL hal = HAL_construct();
    Application app = Application_construct();
    InitialApplicationValues(&app);




    // Main super-loop! In a polling architecture, this function should call
    // your main FSM function over and over.
    while (true)
    {
        HAL_refresh(&hal);
        Application_loop(&app, &hal);
        //Application_screen(&app, &hal);
    }


}

/**
 * A helper function which increments a value with a maximum. If incrementing
 * the number causes the value to hit its maximum, the number wraps around
 * to 0.
 */
// assigns necessary initial variables before the tamagachi game begins
void InitialApplicationValues(Application* app_p)
{
    app_p->xdirection = 44;
    app_p->ydirection = 44;
    app_p->radius = 10;
    app_p->energyLevel = 5;
    app_p->happyLevel = 5;
    app_p->totalMoves = 0;

}

uint32_t CircularIncrement(uint32_t value, uint32_t maximum)
{
    return (value + 1) % maximum;
}

/**
 * The main constructor for your application. This function should initialize
 * each of the FSMs which implement the application logic of your project.
 *
 * @return a completely initialized Application object
 */
Application Application_construct()
{
    Application app;

    // Initialize local application state variables here!
    app.baudChoice = BAUD_9600;
    app.firstCall = true;
    app.age = 0;

    // a 3-second timer (i.e. 3000 ms as specified in the SWTimer_contruct)
    app.threeSecondTimer = SWTimer_construct(3000);
    SWTimer_start(&app.threeSecondTimer);

    return app;
}

/**
 * The main super-loop function of the application. We place this inside of a
 * single infinite loop in main. In this way, we can model a polling system of
 * FSMs. Every cycle of this loop function, we poll each of the FSMs one time,
 * followed by refreshing all inputs to the system through a convenient
 * [HAL_refresh()] call.
 *
 * @param app_p:  A pointer to the main Application object.
 * @param hal_p:  A pointer to the main HAL object
 */
void Application_loop(Application* app_p, HAL* hal_p)
{


    // checks if the game has reached the gameover conditions
    if(app_p->energyLevel <= 1 && app_p->happyLevel <= 1)
        {
            if (app_p->happyLevel == 1)
            {
                Graphics_clearDisplay(&hal_p->g_sContext);
                app_p->happyLevel -= 1;
            }
            // shows gameover screen
            Application_EndScreen(hal_p);

        }
    else
    {
        Application_showGamecreen(app_p, hal_p);
            Application_characterLocation(app_p, hal_p);

            if (SWTimer_expired(&app_p->threeSecondTimer))
                {
                // incrimentally increases the age every 3 seconds
                    SWTimer_start(&app_p->threeSecondTimer);
                    app_p->age +=1;
                    // reduces the energy and happiness value every 3 seconds
                    if (app_p->happyLevel > 0)
                        app_p->happyLevel -= 1;
                    if (app_p->energyLevel > 0)
                        app_p->energyLevel -= 1;
                    //clears the current energy and happiness fill level then changes the enregy value
                    Graphics_setForegroundColor(&hal_p->g_sContext, GRAPHICS_COLOR_WHITE);
                    Graphics_fillRectangle(&hal_p->g_sContext, &app_p->R_ENERGY);
                    Graphics_fillRectangle(&hal_p->g_sContext, &app_p->R_HAPPY);
                }
            // reduces the energy level if the tamagachi has moved 3 times
            if(app_p->totalMoves == 3)
            {
                if(app_p->energyLevel > 0)
                {
                    app_p->energyLevel -= 1;
                                    Graphics_setForegroundColor(&hal_p->g_sContext, GRAPHICS_COLOR_WHITE);
                                    Graphics_fillRectangle(&hal_p->g_sContext, &app_p->R_ENERGY);
                                    app_p->totalMoves = 0;
                }

            }
    }



    // Not a real TODO: Ensure that your code never blocks turning on this LED.
    if (Button_isPressed(&hal_p->launchpadS1))
        LED_turnOn(&hal_p->launchpadLED1);
    else
        LED_turnOff(&hal_p->launchpadLED1);

    // Restart/Update communications if either this is the first time the application is
    // run or if Boosterpack S2 is pressed (which means a new baudrate is being set up)
    if (Button_isTapped(&hal_p->boosterpackS2) || app_p->firstCall) {
        Application_updateCommunications(app_p, hal_p);
    }

    // A 3-second timer is repeatedly started, whenever it expires 1 year is added to the age


    if (UART_hasChar(&hal_p->uart)) {
        // The character received from your serial terminal
        char rxChar = UART_getChar(&hal_p->uart);

        char txChar = Application_interpretIncomingChar(rxChar);

        // Only send a character if the UART module can send it
        if (UART_canSend(&hal_p->uart))
            UART_putChar(&hal_p->uart, txChar);
    }


}


/**
 * Updates which LEDs are lit and what baud rate the UART module communicates
 * with, based on what the application's baud choice is at the time this
 * function is called.
 *
 * @param app_p:  A pointer to the main Application object.
 * @param hal_p:  A pointer to the main HAL object
 */
void Application_updateCommunications(Application* app_p, HAL* hal_p)
{
    // When this application first loops, the proper LEDs aren't lit. The
    // firstCall flag is used to ensure that the
    if (app_p->firstCall) {
        app_p->firstCall = false;
    }

    // When Boosterpack S1 is tapped, circularly increment which baud rate is used.
    else
    {
        uint32_t newBaudNumber = CircularIncrement((uint32_t) app_p->baudChoice, NUM_BAUD_CHOICES);
        app_p->baudChoice = (UART_Baudrate) newBaudNumber;
    }

    // Start/update the baud rate according to the one set above.
    UART_SetBaud_Enable(&hal_p->uart, app_p->baudChoice);

    // Based on the new application choice, turn on the correct LED.
    // To make your life easier, we recommend turning off all LEDs before
    // selectively turning back on only the LEDs that need to be relit.
    // -------------------------------------------------------------------------
    LED_turnOff(&hal_p->boosterpackRed);
    LED_turnOff(&hal_p->boosterpackGreen);
    LED_turnOff(&hal_p->boosterpackBlue);

    // FSM that switches the color of the booster pack LED
    // dependin on the baudrate
    switch (app_p->baudChoice)
    {

        case BAUD_9600:


            LED_turnOff(&hal_p->boosterpackGreen);
            LED_turnOff(&hal_p->boosterpackBlue);
            LED_turnOn(&hal_p->boosterpackRed);
            break;


        case BAUD_19200:



            LED_turnOff(&hal_p->boosterpackGreen);
            LED_turnOff(&hal_p->boosterpackRed);
            LED_turnOn(&hal_p->boosterpackBlue);

            break;


        case BAUD_38400:



            LED_turnOn(&hal_p->boosterpackGreen);
            LED_turnOff(&hal_p->boosterpackRed);
            LED_turnOff(&hal_p->boosterpackBlue);
            break;


        case BAUD_57600:


            LED_turnOn(&hal_p->boosterpackGreen);
            LED_turnOn(&hal_p->boosterpackRed);
            LED_turnOn(&hal_p->boosterpackBlue);
            break;

        // In the default case, this program will do nothing.
        default:
            break;
    }
}

// this is the end game screen that prints "game over"
void Application_EndScreen(HAL* hal_p)
{

    Graphics_setBackgroundColor(&hal_p->g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_setForegroundColor(&hal_p->g_sContext, GRAPHICS_COLOR_RED);
    Graphics_drawString(&hal_p->g_sContext, (int8_t *)"Game OVER", -1, 22, 40, true);
}
/**
 * Interprets a character which was incoming and returns an interpretation of
 * that character. If the input character is a letter, it return L for Letter, if a number
 * return N for Number, and if something else, it return O for Other.
 *
 * @param rxChar: Input character
 * @return :  Output character
 */
char Application_interpretIncomingChar(char rxChar)
{
    // The character to return back to sender. By default, we assume the letter
    // to send back is an 'O' (assume the character is an "other" character)
    char txChar = 'O';

    // Numbers - if the character entered was a number, transfer back an 'N'
    if (rxChar >= '0' && rxChar <= '9') {
        txChar = 'N';
    }

    // Letters - if the character entered was a letter, transfer back an 'L'
    if ((rxChar >= 'a' && rxChar <= 'z') || (rxChar >= 'A' && rxChar <= 'Z')) {
        txChar = 'L';
    }

    return (txChar);
}

//



// this function converst the baudrate from enums to intigers
// to be used to represtent the baudrate on screen
// it returns the baudrate representation as an int
int BaudRateRepresentation(Application* app_p)
{
    int baudLevel;
    if(app_p->baudChoice == BAUD_9600)
        baudLevel = 0;
    else if(app_p->baudChoice == BAUD_19200)
        baudLevel = 1;
    else if(app_p->baudChoice == BAUD_38400)
        baudLevel = 2;
    else if(app_p->baudChoice == BAUD_57600)
        baudLevel = 3;

    return(baudLevel);
}


// this function controls all the drawings of the game screen aside from the tamagachi
void Application_showGamecreen(Application* app_p, HAL* hal_p)
{
    // strings that hold the updated tamagachis age and the updated baudrate
        char ageRep[11];
        char baudvalue[11];
        // converst the tamagachis age and the baudrate to strings
        snprintf(ageRep, 8, "AGE %03d", app_p->age);
        snprintf(baudvalue, 8, "BR %d", BaudRateRepresentation(app_p));
        // creates rectangles for the tamagachis screen and for its energy and health bars
        Graphics_Rectangle R_screen, R_bar, R_bar2;
        R_screen.xMin = 15; R_screen.xMax = 113; R_screen.yMin = 18; R_screen.yMax = 101;
        R_bar.xMin = 60; R_bar.xMax = 120; R_bar.yMin = 104; R_bar.yMax = 114;
        R_bar2.xMin = 60; R_bar2.xMax = 120; R_bar2.yMin = 116; R_bar2.yMax = 126;



        Graphics_fillCircle(&hal_p->g_sContext, app_p->xdirection, app_p->ydirection, app_p->radius);
        Application_manageScreen(app_p, hal_p);
        Graphics_setForegroundColor(&hal_p->g_sContext, GRAPHICS_COLOR_BLACK);

        // draws the box around the tamagachi
        Graphics_drawRectangle(&hal_p->g_sContext, &R_screen);
        // draws the rectangles for the tamagachis health and enregy
        Graphics_drawRectangle(&hal_p->g_sContext, &R_bar);
        Graphics_drawRectangle(&hal_p->g_sContext, &R_bar2);
        // writes the string for the tamagachis age and for the baudrate
        Graphics_drawString(&hal_p->g_sContext, (int8_t *)ageRep, -1, 1, 1, true);
        Graphics_drawString(&hal_p->g_sContext, (int8_t *)baudvalue, -1, 90, 1, true);
        // draws the string that indicates what each bar is
        Graphics_drawString(&hal_p->g_sContext, (int8_t *)"HAPPY", -1, 1, 115, true);
        Graphics_drawString(&hal_p->g_sContext, (int8_t *)"ENERGY", -1, 1, 103, true);
        // updates the energy and health values
        Application_EnergyValues(app_p, hal_p);
        Application_HappyValues(app_p, hal_p);
        // draws the energy and happiness bars of the tamagachi initially
        Graphics_fillRectangle(&hal_p->g_sContext, &app_p->R_ENERGY);
        Graphics_fillRectangle(&hal_p->g_sContext, &app_p->R_HAPPY);



}

// manages the tamagachi being drawn on screen
void Application_manageScreen(Application* app_p, HAL* hal_p)
{
    // converst the tamagachis age from an int to an enum
    if(app_p->age == 0)
        app_p->ageVisual = EGG;
    if(app_p->age >= 1 && app_p->ageVisual == EGG)
        app_p->ageVisual = CHILD;
    if(app_p->age >= 4 && app_p->energyLevel >= 4 && app_p->happyLevel >= 4)
        app_p->ageVisual = ADULT;
    // draws the tamagachi pink
    if(app_p->ageVisual == EGG)
    {
        Graphics_setForegroundColor(&hal_p->g_sContext, GRAPHICS_COLOR_PINK);
                            Application_DrawAndFillCircle(app_p, hal_p);
    }
    // draws the tamagachi red
    else if(app_p->ageVisual == CHILD)
            {
            Graphics_setForegroundColor(&hal_p->g_sContext, GRAPHICS_COLOR_RED);
                                Application_DrawAndFillCircle(app_p, hal_p);
            }
    // draws the tamagachi blue
    else if(app_p->ageVisual == ADULT)
    {
        Graphics_setForegroundColor(&hal_p->g_sContext, GRAPHICS_COLOR_BLUE);
                            Application_DrawAndFillCircle(app_p, hal_p);
    }

}
void UART_messageCheck(Application* app_p, HAL* hal_p)
{
    app_p->rChar = UART_getChar(&hal_p->uart);
            app_p->tChar = Application_interpretIncomingChar(app_p->rChar);
            if(UART_canSend(&hal_p->uart))
            {
                UART_putChar(&hal_p->uart, app_p->tChar);
            }
}


void Application_HappyValues(Application* app_p, HAL* hal_p)
{
    // converts happiness level from intiger to enum
    if(app_p->happyLevel == 0)
        app_p->valueHappy = EMPTY;
    else if(app_p->happyLevel == 1)
        app_p->valueHappy = FIRST_LEVEL;
    else if(app_p->happyLevel == 2)
        app_p->valueHappy = SECOND_LEVEL;
    else if(app_p->happyLevel == 3)
        app_p->valueHappy = THIRD_LEVEL;
    else if(app_p->happyLevel == 4)
        app_p->valueHappy = FOURTH_LEVEL;
    else if(app_p->happyLevel == 5)
        app_p->valueHappy = FULL;

    // FMS that switches between enum value of happiness and changes the
    // happiness bar length correspondingly
    switch(app_p->valueHappy)
    {
    case EMPTY:
                app_p->R_HAPPY.xMin = 0;
                app_p->R_HAPPY.xMax = 0;
                app_p->R_HAPPY.yMin = 0;
                app_p->R_HAPPY.yMax = 0;
        break;
    case FIRST_LEVEL:
                app_p->R_HAPPY.xMin = 60;
                app_p->R_HAPPY.xMax = 71;
                app_p->R_HAPPY.yMin = 117;
                app_p->R_HAPPY.yMax = 125;
                break;
    case SECOND_LEVEL:
                    app_p->R_HAPPY.xMin = 60;
                    app_p->R_HAPPY.xMax = 82;
                    app_p->R_HAPPY.yMin = 117;
                    app_p->R_HAPPY.yMax = 125;
                    break;
    case THIRD_LEVEL:
                    app_p->R_HAPPY.xMin = 60;
                    app_p->R_HAPPY.xMax = 93;
                    app_p->R_HAPPY.yMin = 117;
                    app_p->R_HAPPY.yMax = 125;
                    break;
    case FOURTH_LEVEL:
                        app_p->R_HAPPY.xMin = 60;
                        app_p->R_HAPPY.xMax = 104;
                        app_p->R_HAPPY.yMin = 117;
                        app_p->R_HAPPY.yMax = 125;
                        break;
    case FULL:
                    app_p->R_HAPPY.xMin = 60;
                    app_p->R_HAPPY.xMax = 119;
                    app_p->R_HAPPY.yMin = 117;
                    app_p->R_HAPPY.yMax = 125;
                    break;

    }
}

void Application_EnergyValues(Application* app_p, HAL* hal_p)
{
    // converts energy level from intiger to enum
    if(app_p->energyLevel == 0)
        app_p->valueEnergy = EMPTY;
    else if(app_p->energyLevel == 1)
        app_p->valueEnergy = FIRST_LEVEL;
    else if(app_p->energyLevel == 2)
        app_p->valueEnergy = SECOND_LEVEL;
    else if(app_p->energyLevel == 3)
        app_p->valueEnergy = THIRD_LEVEL;
    else if(app_p->energyLevel == 4)
        app_p->valueEnergy = FOURTH_LEVEL;
    else if(app_p->energyLevel == 5)
        app_p->valueEnergy = FULL;

    // FMS that switches between enum value of energy and changes the
    // energy bar length correspondingly
    switch(app_p->valueEnergy)
    {
    case EMPTY:
                app_p->R_ENERGY.xMin = 0;
                app_p->R_ENERGY.xMax = 0;
                app_p->R_ENERGY.yMin = 0;
                app_p->R_ENERGY.yMax = 0;
        break;
    case FIRST_LEVEL:
                app_p->R_ENERGY.xMin = 60;
                app_p->R_ENERGY.xMax = 71;
                app_p->R_ENERGY.yMin = 105;
                app_p->R_ENERGY.yMax = 113;
                break;
    case SECOND_LEVEL:
                    app_p->R_ENERGY.xMin = 60;
                    app_p->R_ENERGY.xMax = 82;
                    app_p->R_ENERGY.yMin = 105;
                    app_p->R_ENERGY.yMax = 113;
                    break;
    case THIRD_LEVEL:
                    app_p->R_ENERGY.xMin = 60;
                    app_p->R_ENERGY.xMax = 93;
                    app_p->R_ENERGY.yMin = 105;
                    app_p->R_ENERGY.yMax = 113;
                    break;
    case FOURTH_LEVEL:
                            app_p->R_ENERGY.xMin = 60;
                            app_p->R_ENERGY.xMax = 104;
                            app_p->R_ENERGY.yMin = 105;
                            app_p->R_ENERGY.yMax = 113;
                            break;
    case FULL:
                    app_p->R_ENERGY.xMin = 60;
                    app_p->R_ENERGY.xMax = 119;
                    app_p->R_ENERGY.yMin = 105;
                    app_p->R_ENERGY.yMax = 113;
                    break;

    }
}

// this to move the Tamagchi depending on the input from UART
void Application_characterLocation(Application* app_p, HAL* hal_p)
{
    // this checks if UART has a character that has been sent
    if(UART_hasChar(&hal_p->uart))
    {
        // this function is called multiple times to assign UARTS
        // input to a variable
        UART_messageCheck(app_p, hal_p);
        // this function is called to convert the UART value sent from the keyboard
        // to directional enums to be used to move the Tamagachi
        Application_LetterToDirection(app_p);

    }
    // this is an FSM that manages the movement of the Tamagachi and the feeding
    switch(app_p->direction)
    {
    case RIGHT:
        // checks if the tamagachi is at the edge of the screen and if it has enough
        // enegy to move
        if(app_p->xdirection <= bRIGHT && app_p->energyLevel > 0)
        {
            // sets the foregound color to white to delete the tamagachis previous location
            Graphics_setForegroundColor(&hal_p->g_sContext, GRAPHICS_COLOR_WHITE);
            Application_DrawAndFillCircle(app_p, hal_p);
            Application_HappinesRecalculator(app_p, hal_p);
            app_p->xdirection += 40;
            app_p->totalMoves += 1;
        }

            Application_manageScreen(app_p, hal_p);
            // moves to nothing to prevent repitition
            app_p->direction = NOTHING;
        break;
    case LEFT:
        // checks if the tamagachi is at the edge of the screen and if it has enough
                // enegy to move
        if(app_p->xdirection > bLEFT && app_p->energyLevel > 0)
                {
            // sets the foregound color to white to delete the tamagachis previous location

                    Graphics_setForegroundColor(&hal_p->g_sContext, GRAPHICS_COLOR_WHITE);
                    Application_DrawAndFillCircle(app_p, hal_p);
                    Application_HappinesRecalculator(app_p, hal_p);
                    app_p->xdirection -= 40;
                    app_p->totalMoves += 1;
                }

                    Application_manageScreen(app_p, hal_p);
                    // moves to nothing to prevent repitition
                    app_p->direction = NOTHING;
        break;
    case UP:
        // checks if the tamagachi is at the edge of the screen and if it has enough
                // enegy to move
        if(app_p->ydirection > bUP && app_p->energyLevel > 0)
                {
            // sets the foregound color to white to delete the tamagachis previous location

                    Graphics_setForegroundColor(&hal_p->g_sContext, GRAPHICS_COLOR_WHITE);
                    // function that draws and fills in the tamagachi
                    Application_DrawAndFillCircle(app_p, hal_p);
                    // function that updates the tamagachis health and health bar
                    Application_HappinesRecalculator(app_p, hal_p);
                    // moves the tamagachi by changing is stored location
                    app_p->ydirection -= 20;
                    app_p->totalMoves += 1;
                }
                    // draws the tamagachis with its respective color
                    Application_manageScreen(app_p, hal_p);
                    // moves to nothing to prevent repitition
                    app_p->direction = NOTHING;
        break;
    case DOWN:
        // checks if the tamagachi is at the edge of the screen and if it has enough
                // enegy to move
        if(app_p->ydirection < bDOWN && app_p->energyLevel > 0)
                {
            // sets the foregound color to white to delete the tamagachis previous location

                    Graphics_setForegroundColor(&hal_p->g_sContext, GRAPHICS_COLOR_WHITE);
                    Application_DrawAndFillCircle(app_p, hal_p);
                    Application_HappinesRecalculator(app_p, hal_p);
                    app_p->ydirection += 20;
                    app_p->totalMoves += 1;
                }

                    Application_manageScreen(app_p, hal_p);
                    // moves to nothing to prevent repitition
                    app_p->direction = NOTHING;
                    break;
        // this feeds the tamagachi when it receives an input "f" from UART
    case FEED:
        // checks if the tamagachis energy level is max, if not it feeds the tamagachi
        if(app_p->energyLevel < 5 )
        {
            app_p->energyLevel += 1;
            // updates the tamagachis energy value
            Application_EnergyValues(app_p, hal_p);
            // updates the tamagachis energy bar
            Graphics_fillRectangle(&hal_p->g_sContext, &app_p->R_ENERGY);
            // moves to nothing to prevent repitition
            app_p->direction = NOTHING;
        }
                    break;
    case NOTHING:

        break;
    }
}


// draws the tamagachi and fills it in
void Application_DrawAndFillCircle(Application* app_p, HAL* hal_p)
{
    Graphics_drawCircle(&hal_p->g_sContext, app_p->xdirection, app_p->ydirection, app_p->radius);
    Graphics_fillCircle(&hal_p->g_sContext, app_p->xdirection, app_p->ydirection, app_p->radius);
}

// function that updates the tamagachis health and health bar
void Application_HappinesRecalculator(Application* app_p, HAL* hal_p)
{
    if(app_p->happyLevel <= 4)
                                {
                                    app_p->happyLevel += 1;
                                    Application_HappyValues(app_p, hal_p);
                                    Graphics_fillRectangle(&hal_p->g_sContext, &app_p->R_HAPPY);
                                }
}

// function that converst the UART input to tamagachi direcitonals
void Application_LetterToDirection(Application* app_p)
{
    if(app_p->rChar == 'd')
                app_p->direction = RIGHT;
            else if (app_p->rChar == 'a')
                app_p->direction = LEFT;
            else if (app_p->rChar == 'w')
                app_p->direction = UP;
            else if (app_p->rChar == 's')
                app_p->direction = DOWN;
            else if (app_p->rChar == 'f')
                app_p->direction = FEED;
            else
                app_p->direction = NOTHING;
}
