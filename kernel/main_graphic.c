#include "console.h"
#include "page.h"
#include "process.h"
#include "keyboard.h"
#include "mouse.h"
#include "interrupt.h"
#include "clock.h"
#include "ata.h"
#include "device.h"
#include "cdromfs.h"
#include "string.h"
#include "graphics.h"
#include "kernel/ascii.h"
#include "kernel/syscall.h"
#include "rtc.h"
#include "kernelcore.h"
#include "kmalloc.h"
#include "memorylayout.h"
#include "kshell.h"
#include "cdromfs.h"
#include "diskfs.h"
#include "serial.h"


#define SIDE_BAR_WIDTH 20

#define ROCKET_WIDTH 4
#define ROCKET_HEIGHT 3

#define SPACE_SHIP_HEIGHT 4
#define SPACE_SHIP_WIDTH 6

#define BULLET_SPEED 1
#define MAX_BULLETS 30

#define ROCKET_SPEED 1
#define MAX_ROCKETS 4
#define ROCKET_MOVE_DELAY 14
#define BULLET_MOVE_DELAY 2

#define RAND_MAX 80
int quit_flag = 0;

typedef struct
{
    int x;
    int y;
    int active;  // Flag to bullet is active or not
    int avaible; // Flag to bullet is avaible to shot
} Bullet;

typedef struct
{
    int x;
    int y;
    int active; // Flag to rocket is active or not
} Rocket;

Bullet bullets[MAX_BULLETS];
Rocket rockets[MAX_ROCKETS];

int rocketMoveCounter = 0; // Counter to control rocket movement speed
int bulletMoveCounter = 0; // Counter to control bullet movement speed

int flag = 0; // flag 1 when key pressed
int pause_flag = 0;
char current_key = '\0';
int bullet_count = MAX_BULLETS;
int x;
int y;
int score = 0;
char score_str[3];   // maximum number of digits is 3
char bullets_str[2]; // maximum number of digits is 2

struct graphics *graphics;
int X_SIZE, Y_SIZE;

void drawBoundaries(struct graphics *graphics)
{
    // Draw vertical boundaries
    for (int i = 0; i < Y_SIZE; i++)
    {
        kprint_at(graphics, 0, i, "#");              // Left boundary
        kprint_at(graphics, SIDE_BAR_WIDTH, i, "#"); // Right boundary
        kprint_at(graphics, X_SIZE - 1, i, "#");     // Right-most boundary
    }

    // Draw horizontal boundaries
    for (int i = 0; i < X_SIZE; i++)
    {
        kprint_at(graphics, i, 0, "#");          // Top boundary
        kprint_at(graphics, i, Y_SIZE - 1, "#"); // Bottom boundary
    }
}

// Function to convert an integer to its string representation
int int_to_string(int num, char *buffer)
{
    int i = 0;
    int digits = 0; // Variable to store the number of digits

    if (num == 0)
    {
        buffer[i++] = '0';
        digits = 1; // If the number is zero, it has one digit
    }
    else
    {
        // Calculate the number of digits
        int temp = num;
        while (temp != 0)
        {
            digits++;
            temp /= 10;
        }

        // Convert each digit to character and store in the buffer
        while (num != 0)
        {
            int digit = num % 10;
            buffer[i++] = '0' + digit;
            num /= 10;
        }
    }
    buffer[i] = '\0';

    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end)
    {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }

    return digits; // Return the number of digits
}

void printScore(struct graphics *graphics, int x, int y)
{
    int_to_string(score, score_str);
    kprint_at(graphics, x, y, score_str); // Modified line
}

void bullet_counter()
{
    bullet_count = 0;
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].avaible)
        {
            bullet_count += 1;
        }
    }
}

void printBulletCount(struct graphics *graphics, int x, int y)
{
    int_to_string(bullet_count, bullets_str);
    kprint_at(graphics, x, y, bullets_str); // Modified line
    if (bullet_count < 10)
        kprint_at(graphics, x + 1, y, " ");
}

void info(struct graphics *graphics)
{
    // Display the welcome message and instructions
    kprint_at(graphics, 2, 1, "Welcome!");
    kprint_at(graphics, 2, 2, "Save the World!");
    kprint_at(graphics, 2, 3, "by Eren Karadeniz");
    kprint_at(graphics, 2, 4, "200101070");

    kprint_at(graphics, 2, 6, "Keys");
    kprint_at(graphics, 2, 7, "A to move left");
    kprint_at(graphics, 2, 8, "D to move right");
    kprint_at(graphics, 2, 9, "Space to Shot");
    kprint_at(graphics, 2, 10, "Q to quit game");
    kprint_at(graphics, 2, 11, "R to restart game");
    kprint_at(graphics, 2, 12, "P to pause game");
    kprint_at(graphics, 2, 14, "Win after reach");
    kprint_at(graphics, 2, 15, "25 Score");
}

void winGame(struct graphics *graphics)
{
    console_reset(graphics);
    drawBoundaries(graphics);
    info(graphics);
    kprint_at(graphics, 44, 12, "You Win");
    kprint_at(graphics, 37, 13, "Press R for Play Again");
}

void intro(struct graphics *graphics)
{
    // Clear the screen and draw boundaries
    console_reset(graphics);
    drawBoundaries(graphics);

    info(graphics);

    kprint_at(graphics, 2, 17, "Bullets:");
    printBulletCount(graphics, 11, 17);

    kprint_at(graphics, 2, 18, "Score:");
    printScore(graphics, 10, 18);
}

void drawSpaceship(struct graphics *graphics, int x, int y)
{
    kprint_at(graphics, x, y, "A  I  A");
    kprint_at(graphics, x, y + 1, "A /-\\ A");
    kprint_at(graphics, x, y + 2, "\\  U  /");
    kprint_at(graphics, x, y + 3, "/o o o\\");
}

void clearSpaceship(struct graphics *graphics, int x, int y)
{
    // Clear the old position of the spaceship
    kprint_at(graphics, x, y, "       ");
    kprint_at(graphics, x, y + 1, "       ");
    kprint_at(graphics, x, y + 2, "       ");
    kprint_at(graphics, x, y + 3, "       ");
}

void drawRocket(struct graphics *graphics, int x, int y)
{
    kprint_at(graphics, x, y, "\\||/");
    kprint_at(graphics, x, y + 1, "|oo|");
    kprint_at(graphics, x, y + 2, " \\/");
}

void clearRocket(struct graphics *graphics, int x, int y)
{
    kprint_at(graphics, x, y, "    ");
    kprint_at(graphics, x, y + 1, "    ");
    kprint_at(graphics, x, y + 2, "   ");
}

void moveRocket(struct graphics *graphics, int index)
{
    if (rocketMoveCounter % ROCKET_MOVE_DELAY == 0)
    {
        clearRocket(graphics, rockets[index].x, rockets[index].y); // Clear previous rocket position
        rockets[index].y += ROCKET_SPEED;                         // Move the rocket downwards
        drawRocket(graphics, rockets[index].x, rockets[index].y);
    }
}

void moveBullet(struct graphics *graphics, int index)
{
    if (bulletMoveCounter % BULLET_MOVE_DELAY == 0)
    {
        kprint_at(graphics, bullets[index].x, bullets[index].y, " "); // Clear previous bullet position
        bullets[index].y -= BULLET_SPEED;                            // Move the bullet upwards
        if (bullets[index].y > 0)
        {
            kprint_at(graphics, bullets[index].x, bullets[index].y, "^"); // Draw the bullet
        }
        else
        {
            bullets[index].active = 0;
        }
    }
}

unsigned int get_system_timer_value()
{
    unsigned int val;
    // Read the value of the system timer (assuming x86 architecture)
    asm volatile("rdtsc" : "=a"(val));
    return val;
}

// Define some global variables for the random number generator
static unsigned long next;
// A function to generate a pseudo-random integer
int rand(void)
{
    next = get_system_timer_value();
    next = next * 1103515245 + 12345;
    return (unsigned int)(next / 65536) % RAND_MAX;
}

// Define a function to wait for a specified number of milliseconds
void busy_wait(unsigned int milliseconds)
{
    // Calculate the number of iterations needed for the desired milliseconds
    unsigned int iterations = milliseconds * 10000; // Adjust this value as needed based on your system's clock speed

    // Execute an empty loop for the specified number of iterations
    for (unsigned int i = 0; i < iterations; ++i)
    {
        // Do nothing, just wait
    }
}

void initBullets()
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        bullets[i].x = 1;
        bullets[i].y = 1;
        bullets[i].active = 0;
        bullets[i].avaible = 1;
    }
}

int randRocketAxis()
{
    int min_x = SIDE_BAR_WIDTH + 1;        // 21
    int max_x = X_SIZE - ROCKET_WIDTH - 1; // 73
    int x = rand();
    while (min_x > x || x > max_x)
    {
        x = rand();
    }
    return x;
}

void initRockets()
{
    for (int i = 0; i < MAX_ROCKETS; i++)
    {
        int newRocketX, newRocketY;
        int collisionDetected;

        do
        {
            // Generate random position for the new rocket
            newRocketX = randRocketAxis(); // Adjust range to prevent overflow
            newRocketY = 1;                // Adjust range as needed

            // Check for collision with existing rockets based on X position only
            collisionDetected = 0;
            for (int j = 0; j < i; j++)
            {
                if (rockets[j].active &&
                    (newRocketX >= rockets[j].x - ROCKET_WIDTH && newRocketX <= rockets[j].x + ROCKET_WIDTH)) // Check only X position
                {
                    collisionDetected = 1;
                    i = 0;
                    break;
                }
            }
        } while (collisionDetected);

        // Set the position of the new rocket
        rockets[i].x = newRocketX;
        rockets[i].y = newRocketY;
        rockets[i].active = 1;
    }
}

// Function to check for collision between bullet and rocket
void collisionBullet(struct graphics *graphics)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].active)
        {
            for (int j = 0; j < MAX_ROCKETS; j++)
            {
                if (rockets[j].active &&
                    bullets[i].x >= rockets[j].x && bullets[i].x < rockets[j].x + 7 &&
                    bullets[i].y >= rockets[j].y && bullets[i].y < rockets[j].y + 3)
                {
                    score += 1;
                    printScore(graphics, 10, 18);                         // Print updated score
                    bullets[i].active = 0;                               // Deactivate bullet
                    rockets[j].active = 0;                               // Deactivate rocket
                    kprint_at(graphics, bullets[i].x, bullets[i].y, " "); // Clear bullet
                    clearRocket(graphics, rockets[j].x, rockets[j].y);    // Clear rocket
                    break;
                }
            }
        }
    }
}

void gameOver(struct graphics *graphics)
{
    console_reset(graphics);  // Clear the entire screen
    drawBoundaries(graphics); // Redraw the boundaries
    info(graphics);           // Print game info
    kprint_at(graphics, 35, 12, "You lost, Press R for Play Again");
    kprint_at(graphics, 46, 13, "Score: ");
    printScore(graphics, 54, 13); // Print the score
}

// Function to check for collision between rocket and spaceship
void collisionSpaceShip(struct graphics *graphics)
{
    for (int i = 0; i < MAX_ROCKETS; i++)
    {
        // Check if any of the edges of the rocket box lie outside the spaceship box
        if (x <= rockets[i].x + ROCKET_WIDTH - 1 && x + SPACE_SHIP_WIDTH - 1 >= rockets[i].x && rockets[i].y + ROCKET_HEIGHT >= y)
        {
            quit_flag = 1;
            gameOver(graphics);                                           // Pass the console to the gameOver function
            kprint_at(graphics, 36, 11, "Spaceship destroyed by rocket"); // Print message on the specified console
        }
    }
}

void quitGame(struct graphics *graphics)
{
    console_reset(graphics);                               // Clear the screen of the specified console
    drawBoundaries(graphics);                              // Redraw the boundaries on the specified console
    info(graphics);                                        // Display game info on the specified console
    kprint_at(graphics, 35, 12, "Press R for Play Again"); // Print message on the specified console
}

void shot_bullet(Bullet *bullet)
{
    bullet->active = 1;
    bullet->avaible = 0;
    bullet->x = x + 3; // Adjust bullet position to appear from spaceship's center
    bullet->y = y - 1;
}

void init(struct graphics *graphics)
{
    initBullets();
    initRockets();
    intro(graphics);
    drawBoundaries(graphics);

    x = (X_SIZE - SPACE_SHIP_WIDTH + SIDE_BAR_WIDTH) / 2; // Starting position for spaceship
    y = Y_SIZE - SPACE_SHIP_HEIGHT - 1;                   // Adjusted starting position for the spaceship
}

void restartGame(struct graphics *graphics)
{
    console_reset(graphics); // Clear the screen of the provided console
    init(graphics);          // Initialize the game with the provided console
}

void handleUserInput(struct graphics *graphics, char current_key, Bullet bullets[MAX_BULLETS])
{
    if (!pause_flag)
    {
        switch (current_key)
        {
        case 'a':
            if (x - 1 > SIDE_BAR_WIDTH)
            {
                clearSpaceship(graphics, x, y);
                (x)--;
            }
            break;
        case 'd':
            if (x + 1 < X_SIZE - 7)
            {
                clearSpaceship(graphics, x, y);
                (x)++;
            }
            break;
        case ' ':
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (!bullets[i].active && bullets[i].avaible)
                {
                    shot_bullet(&bullets[i]);
                    bullet_counter();
                    printBulletCount(graphics, 11, 17);
                    break;
                }
            }
            break;
        case 'q':
            score = 0;
            quitGame(graphics);
            bullet_count = MAX_BULLETS;
            quit_flag = 1;
            break;
        case 'r':
            score = 0;
            quit_flag = 0;
            bullet_count = MAX_BULLETS;
            restartGame(graphics); // Restart the game
            break;
        case 'p':
            pause_flag = !pause_flag; // Toggle pause_flag
            if (pause_flag)
            {
                kprint_at(graphics, 35, 10, "Paused, Press p to continue");
            }
            break;
        }
        flag = 0;
    }
    else
    {
        if (current_key == 'p')
        {
            pause_flag = 0;
            kprint_at(graphics, 35, 10, "                                 ");
            flag = 0;
        }
    }
}

void move_bullets(struct graphics *graphics)
{
    // Move all active bullets
    for (int index = 0; index < MAX_BULLETS; index++)
    {
        if (!pause_flag)
        {
            if (bullets[index].active && !bullets[index].avaible)
            {
                kprint_at(graphics, bullets[index].x, bullets[index].y, "^");
                moveBullet(graphics, index);
            }
        }
    }
    // Increment the bullet move counter
    bulletMoveCounter++;
    // Reset the counter to prevent overflow
    if (bulletMoveCounter >= BULLET_MOVE_DELAY)
        bulletMoveCounter = 0;
}

// Function to generate a single rocket from passive rocket
void generateRocket(Rocket *rocket)
{
    int newRocketX, newRocketY;
    int collisionDetected;

    do
    {
        // Generate random position for the new rocket
        newRocketX = randRocketAxis(); // Adjust range to prevent overflow
        newRocketY = 1;                // Adjust range as needed

        // Check for collision with existing rockets based on X position only
        collisionDetected = 0;
        for (int j = 0; j < MAX_ROCKETS; j++)
        {
            if (rockets[j].active &&
                (newRocketX >= rockets[j].x - ROCKET_WIDTH && newRocketX <= rockets[j].x + ROCKET_WIDTH)) // Check only X position
            {
                collisionDetected = 1;
                break;
            }
        }
    } while (collisionDetected);

    // Set the position of the new rocket
    rocket->x = newRocketX;
    rocket->y = newRocketY;
    rocket->active = 1;
}

void generate_rockets()
{
    // Generate new rockets if there are inactive rockets
    for (int i = 0; i < MAX_ROCKETS; i++)
    {
        if (!rockets[i].active)
        {
            generateRocket(&rockets[i]);
        }
    }
}

void move_rockets(struct graphics *graphics)
{
    // Draw and move the rocket
    for (int i = 0; i < MAX_ROCKETS; i++)
    {
        if (!pause_flag)
        {
            if (rockets[i].active)
            {
                drawRocket(graphics, rockets[i].x, rockets[i].y);
                moveRocket(graphics, i);
            }
        }
    }

    // Increment the rocket move counter
    rocketMoveCounter++;
    // Reset the counter to prevent overflow
    if (rocketMoveCounter >= ROCKET_MOVE_DELAY)
        rocketMoveCounter = 0;
    if (current_key != 'p')
    {
        generate_rockets();
    }
}

int continueGame(struct graphics *graphics)
{
    // Check if all rockets have reached the bottom of the screen

    int rocketReachedBottom = 0;
    for (int i = 0; i < MAX_ROCKETS; i++)
    {
        if (rockets[i].y >= Y_SIZE)
        {
            rocketReachedBottom = 1;
            if (rocketReachedBottom)
            {
                quit_flag = 1;
                gameOver(graphics);
                return 0;
            }
        }
    }

    if (score == 25)
    {
        quit_flag = 1;
        winGame(graphics);
        score = 0;
        return 0;
    }

    return 1;
}

int kernel_main()
{
    //graphics_size(graphics, &X_SIZE, &Y_SIZE);

    page_init();
    kmalloc_init((char *)KMALLOC_START, KMALLOC_LENGTH);
    interrupt_init();
    keyboard_init();
    process_init();

    struct graphics *root_graphics = graphics_create_root();
    struct graphics *window_graphics = graphics_create(root_graphics);
    int x = 10;
    int y = 10;
    int X_SIZE = 100;
    int Y_SIZE = 50;
    graphics_rect(window_graphics, x, y, X_SIZE, Y_SIZE);

    //init(graphics);

    //Game loop
    while (1)
    {
        while (quit_flag == 0 && continueGame(graphics))
        {
                current_key = console_getchar(graphics);

                // Convert the character to a string
                char key_string[2];
                key_string[0] = current_key;
                key_string[1] = '\0'; // Null-terminate the string
                kprint_at(graphics, 10, 20, key_string);

                handleUserInput(graphics, current_key, bullets);
                if (current_key == 'q')
                {
                    break;
                }
            drawSpaceship(graphics, x, y);
            move_bullets(graphics);
            move_rockets(graphics);
            // Check for collision between bullets and rockets
            collisionBullet(graphics);
            collisionSpaceShip(graphics);

            busy_wait(1000); // Wait for 50 milliseconds using busy wait
        }
        if (current_key == 'r')
        {
            quit_flag = 0;
            bullet_count = MAX_BULLETS;
            restartGame(graphics); // Restart the game
        }
        // Clean up resources
        graphics_delete(graphics);
        return 0;
    }

}
