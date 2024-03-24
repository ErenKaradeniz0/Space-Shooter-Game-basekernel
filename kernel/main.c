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

#define MAX_X 1024
#define MAX_Y 768
#define SIDE_BAR_WIDTH 192 // 24 * 8

#define ROCKET_WIDTH 32    // 6 * 8
#define ROCKET_HEIGHT 40

#define SPACE_SHIP_WIDTH 80  // 6 * 8
#define SPACE_SHIP_HEIGHT 64 // 8 * 8

#define BULLET_SPEED 8
#define MAX_BULLETS 30

#define ROCKET_SPEED 8
#define MAX_ROCKETS 6
#define ROCKET_MOVE_DELAY 14
#define BULLET_MOVE_DELAY 2

#define RAND_MAX 112 // 944 / 8
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

int quit_flag = 0;
int pause_flag = 0;
char current_key = '1';
int bullet_count = MAX_BULLETS;
int ship_x;
int ship_y;
int score = 0;
char score_str[3];   // maximum number of digits is 3
char bullets_str[2]; // maximum number of digits is 2

void clear_screen(struct graphics *g)
{
    graphics_clear(g, 0, 0, MAX_X, MAX_Y);
}

void drawBoundaries(struct graphics *g)
{

    graphics_rect(g,0,0,MAX_X,8);
    graphics_rect(g,0,MAX_Y-8,MAX_X,8);
    graphics_rect(g,0,0,8,MAX_Y-8);
    graphics_rect(g,SIDE_BAR_WIDTH,0,8,MAX_Y-8);
    graphics_rect(g,MAX_X-8,0,8,MAX_Y);
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

void printScore(struct graphics *g, int x, int y)
{
    int num_digits = int_to_string(score, score_str);
    graphics_write_string(g, x, y, score_str);
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

void printBulletCount(struct graphics *g, int x, int y)
{

    int num_digits = int_to_string(bullet_count, bullets_str);
    graphics_write_string(g, x, y, bullets_str);
    // kprint_at(x, y, bullets_str);
    if (bullet_count < 10)
        graphics_write_string(g, x + 8, y, " ");
}

void info(struct graphics *g)
{
    graphics_write_string(g, 16, 8, "Welcome!");
    graphics_write_string(g, 16, 16, "Save the World!");
    graphics_write_string(g, 16, 24, "by Eren Karadeniz");
    graphics_write_string(g, 16, 32, "200101070");

    graphics_write_string(g, 16, 48, "Keys");
    graphics_write_string(g, 16, 56, "A to move left");
    graphics_write_string(g, 16, 64, "D to move right");
    graphics_write_string(g, 16, 72, "Space to Shot");
    graphics_write_string(g, 16, 80, "Q to quit game");
    graphics_write_string(g, 16, 88, "R to restart game");
    graphics_write_string(g, 16, 96, "P to pause game");
    graphics_write_string(g, 16, 112, "Win after reach");
    graphics_write_string(g, 16, 120, "25 Score");
}

void intro(struct graphics *g)
{
    drawBoundaries(g);

    info(g);

    graphics_write_string(g, 16, 136, "Bullets:");
    printBulletCount(g, 88, 136);

    graphics_write_string(g, 16, 144, "Score:");
    printScore(g, 80, 144);
}

// Draw A
void draw_a(struct graphics *g, int x, int y, int w, int h)
{
    graphics_line(g, x, y, w + 3, h + 3);
    graphics_line(g, x, y, -w - 3, h + 3);
    graphics_line(g, x - 10, y + 10, 0, h + 5);
    graphics_line(g, x + 10, y + 10, 0, h + 5);
    graphics_line(g, x - 10, y + 15, 20, 0);
}

void drawSpaceship(struct graphics *g, int x, int y, int w, int h)
{
    //  Draw A
    draw_a(g, x, y, w, h);
    draw_a(g, x + 70, y, w, h);
    draw_a(g, x, y + 20, w, h);
    draw_a(g, x + 70, y + 20, w, h);

    // Draw I
    graphics_line(g, x + 35, y, 0, h + 6);
    // Draw -
    graphics_line(g, x + 35 - w + 2, y, 2 * w - 3, 0);
    graphics_line(g, x + 35 - w + 2, y + h + 6, 2 * w - 3, 0);

    // Draw /
    graphics_line(g, x + 30, y + 20, -w - 6, h + 6);
    // Draw '\'
    graphics_line(g, x + 40, y + 20, w + 6, h + 6);
    // Draw -
    graphics_line(g, x + 30, y + 22, w + 6, 0);

    // Draw /
    graphics_line(g, x + 10, y + 45, -w - 12, h + 12);
    // Draw o
    graphics_circle(g, x + 20, y + 45, w + 2);
    graphics_circle(g, x + 35, y + 45, w + 2);
    graphics_circle(g, x + 50, y + 45, w + 2);
    // Draw '\'
    graphics_line(g, x + 60, y + 45, w + 12, h + 12);
}

void clearSpaceship(struct graphics *g, int x, int y, int w, int h)
{
    // Calculate bottom-right corner coordinates
    int x2 = x + 96;
    int y2 = y + 56; // Maximum y-coordinate for the spaceship

    // Clear the rectangle covering the spaceship
    graphics_clear(g, x - 10, y, x2 - x, y2 - y);
}

void moveBullet(struct graphics *g, int index)
{
    if (bulletMoveCounter % BULLET_MOVE_DELAY == 0)
    {
        graphics_char(g, bullets[index].x, bullets[index].y, ' '); // Clear previous bullet position
        bullets[index].y -= BULLET_SPEED;                          // Move the bullet upwards
        if (bullets[index].y > 0)
        {
            graphics_char(g, bullets[index].x, bullets[index].y, '^'); // Draw the bullet
        }
        else
            bullets[index].active = 0;
    }
}

void move_bullets(struct graphics *g)
{

    // Move all active bullets
    for (int index = 0; index < MAX_BULLETS; index++)
    {
        if (!pause_flag)
        {
            if (bullets[index].active && !bullets[index].avaible)
            {
                graphics_char(g, bullets[index].x, bullets[index].y, '^');
                moveBullet(g, index);
            }
        }
    }
    // Increment the bullet move counter
    bulletMoveCounter++;
    // Reset the counter to prevent overflow
    if (bulletMoveCounter >= BULLET_MOVE_DELAY)
        bulletMoveCounter = 0;
}

void shot_bullet(Bullet *bullet)
{
    bullet->active = 1;
    bullet->avaible = 0;
    bullet->x = ship_x + 32; // Adjust bullet position to appear from spaceship's center
    bullet->y = ship_y - 16;
}

void drawRocket(struct graphics *g, int x, int y)
{

    // \||/
    graphics_line(g, x, y - 10 - 8, 10, 10);
    graphics_line(g, x + 10, y - 8, 0, -20);
    graphics_line(g, x + 20, y - 8, 0, -20);
    graphics_line(g, x + 20, y - 8, 10, -10);

    //___
    graphics_line(g, x, y, 30, 0);
    //|o|
    graphics_line(g, x, y + 25, 0, -45);
    graphics_circle(g, x + 15, y + 10, 4);
    graphics_line(g, x + 30, y + 25, 0, -45);

    // Draw '\'
    graphics_line(g, x, y + 15, 15, 15);
    // Draw /
    graphics_line(g, x + 30, y + 15, -15, 15);
}

void clearRocket(struct graphics *g, int x, int y)
{
    // Calculate bottom-right corner coordinates
    int x2 = x + ROCKET_WIDTH;
    int y2 = y + 80; // Maximum y-coordinate for the spaceship

    // Clear the rectangle covering the spaceship
    graphics_clear(g, x, y - 40, x2 - x, y2 - y);
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

int randRocketAxis()
{
    int min_x = SIDE_BAR_WIDTH / 8 + 1;
    int max_x = MAX_X / 8 - ROCKET_WIDTH / 8;
    int x = rand(); // 1 - 88
    while (min_x > x || x > max_x)
    {
        x = rand();
    }
    return x + 8;
}

// Function to generate a single rocket from passive rocket
void generateRocket(Rocket *rocket)
{
    int newRocketX, newRocketY;
    int collisionDetected;

    do
    {
        // Generate random position for the new rocket
        newRocketX = 8 * randRocketAxis(); // Adjust range to prevent overflow
        newRocketY = 48;                   // Adjust range as needed

        // Check for collision with existing rockets based on X position only
        collisionDetected = 0;
        for (int j = 0; j < MAX_ROCKETS; j++)
        {
            if (rockets[j].active &&
                (newRocketX >= rockets[j].x - 20 - ROCKET_WIDTH && newRocketX <= rockets[j].x + ROCKET_WIDTH)) // Check only X position
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

void moveRocket(struct graphics *g, int index)
{
    if (rocketMoveCounter % ROCKET_MOVE_DELAY == 0)
    {                                                       // Move the rocket every ROCKET_MOVE_DELAY frames
        clearRocket(g, rockets[index].x, rockets[index].y); // Clear previous rocket position
        rockets[index].y += ROCKET_SPEED;                   // Move the rocket downwards
        drawRocket(g, rockets[index].x, rockets[index].y);
    }
}

void move_rockets(struct graphics *g)
{
    // Draw and move the rocket
    for (int i = 0; i < MAX_ROCKETS; i++)
    {
        if (!pause_flag)
        {
            if (rockets[i].active)
            {
                drawRocket(g, rockets[i].x, rockets[i].y);
                moveRocket(g, i);
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

void initRockets()
{
    for (int i = 0; i < MAX_ROCKETS; i++)
    {
        int newRocketX, newRocketY;
        int collisionDetected;

        do
        {
            // Generate random position for the new rocket
            newRocketX = 8 * randRocketAxis();
            newRocketY = 48;

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

int collisionBullet(struct graphics *g)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].active)
        {
            for (int j = 0; j < MAX_ROCKETS; j++)
            {
                if (rockets[j].active &&
                    bullets[i].x >= rockets[j].x - 8 && bullets[i].x < rockets[j].x + ROCKET_WIDTH &&
                    bullets[i].y >= rockets[j].y && bullets[i].y < rockets[j].y + ROCKET_HEIGHT)
                {
                    score += 1;

                    printScore(g, 80, 144);
                    bullets[i].active = 0; // Deactivate bullet
                    rockets[j].active = 0; // Deactivate rocket
                    graphics_write_string(g, bullets[i].x, bullets[i].y, " ");
                    clearRocket(g, rockets[j].x, rockets[j].y);
                    break;
                }
            }
        }
    }
}

void gameOver(struct graphics *g)
{
    clear_screen(g);
    drawBoundaries(g);
    info(g);
    graphics_write_string(g, MAX_X / 2, MAX_Y / 2 + 8, "You lost, Press R for Play Again");
    graphics_write_string(g, MAX_X / 2, MAX_Y / 2 + 16, "Score: ");
    graphics_write_string(g, MAX_X / 2 + 64, MAX_Y / 2 + 16, score_str);
}

// Function to check for collision between rocket and spaceship
void collisionSpaceShip(struct graphics *g)
{
    for (int i = 0; i < MAX_ROCKETS; i++)
    {
        // Check if any of the edges of the rocket box lie outside the spaceship box

        if (ship_x <= rockets[i].x + ROCKET_WIDTH - 1 && ship_x + SPACE_SHIP_WIDTH - 1 >= rockets[i].x && rockets[i].y + ROCKET_HEIGHT - 1 >= ship_y)
        {
            quit_flag = 1;
            gameOver(g);
            graphics_write_string(g, MAX_X / 2, MAX_Y / 2, "Spaceship destroyed by rocket");
        }
    }
}

void init(struct graphics *g)
{

    clear_screen(g);
    initBullets();
    initRockets();
    intro(g);
    drawBoundaries(g);

    ship_x = (MAX_X + SIDE_BAR_WIDTH) / 2 - SPACE_SHIP_WIDTH / 4; // base x of spaceship 49th pixel
    ship_y = MAX_Y - SPACE_SHIP_HEIGHT;                           // base y of spaceship 87th pixel
}

void quitGame(struct graphics *g)
{
    clear_screen(g);
    drawBoundaries(g);
    info(g);
    graphics_write_string(g, MAX_X / 2, MAX_Y / 2, "Press R for Play Again");
}

void restartGame(struct graphics *g)
{
    init(g); // Initialize the game
}

void handleUserInput(struct graphics *g, char current_key, Bullet bullets[MAX_BULLETS])
{
    if (!pause_flag)
    {
        switch (current_key)
        {
        case 'a':
            if (ship_x - 1 > SIDE_BAR_WIDTH + 20)
            {
                clearSpaceship(g, ship_x, ship_y, 4, 4);
                ship_x -= 8;
            }
            break;
        case 'd':
            if (ship_x + SPACE_SHIP_WIDTH < MAX_X - 16)
            {
                clearSpaceship(g, ship_x, ship_y, 4, 4);
                ship_x += 8;
            }
            break;
        case ' ':
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (!bullets[i].active && bullets[i].avaible)
                {
                    shot_bullet(&bullets[i]);
                    bullet_counter();
                    printBulletCount(g, 88, 136);
                    break;
                }
            }
            break;
        case 'q':
            score = 0;
            quitGame(g);
            quit_flag = 1;
            graphics_delete(g);
            break;
        case 'r':
            score = 0;
            quit_flag = 0;
            bullet_count = MAX_BULLETS;
            restartGame(g); // Restart the game
            break;
        case 'p':
            pause_flag = !pause_flag; // Toggle pause_flag
            if (pause_flag)
            {
                graphics_write_string(g, MAX_X / 2, MAX_Y / 2, "Paused, Press p to continue");
            }
            break;
        }
    }
    else
    {
        if (current_key == 'p')
        {
            pause_flag = 0;
            graphics_write_string(g, MAX_X / 2, MAX_Y / 2, "                           ");
        }
    }
}

void winGame(struct graphics *g)
{
    clear_screen(g);
    drawBoundaries(g);
    info(g);
    graphics_write_string(g, MAX_X / 2, MAX_Y / 2, "You Win!");
    graphics_write_string(g, MAX_X / 2, MAX_Y / 2 + 8, "Press R for Play Again");
}

int continueGame(struct graphics *g)
{
    // Check if all rockets have reached the bottom of the screen

    int rocketReachedBottom = 0;
    for (int i = 0; i < MAX_ROCKETS; i++)
    {
        if (rockets[i].y + ROCKET_HEIGHT >= MAX_Y)
        {
            rocketReachedBottom = 1;
            if (rocketReachedBottom)
            {
                quit_flag = 1;
                graphics_write_string(g, MAX_X / 2, MAX_Y / 2, "Rockets Reached Bottom.");
                gameOver(g);
                return 0;
            }
        }
    }

    if (score == 25)
    {
        quit_flag = 1;
        winGame(g);
        score = 0;
        return 0;
    }

    return 1;
}

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

int kernel_main()
{

    page_init();
    kmalloc_init((char *)KMALLOC_START, KMALLOC_LENGTH);
    interrupt_init();
    rtc_init();
    keyboard_init();
    process_init();

    struct graphics *g = graphics_create_root();
    console_init(g);
    console_addref(&console_root);

    init(g);

    // Game loop
    while (1)
    {
        while (quit_flag == 0 && continueGame(g))
        {

            current_key = keyboard_read(1); // non blocking

            handleUserInput(g, current_key, bullets);
            if (current_key == 'q')
            {

                break;
            }
            drawSpaceship(g, ship_x, ship_y, 4, 4);

            move_bullets(g);
            move_rockets(g);
            //  Check for collision between bullets and rockets
            collisionBullet(g);
            collisionSpaceShip(g);

            busy_wait(800); // Wait for 50 milliseconds using busy wait
        }
        current_key = keyboard_read(1); // non blocking
        if (current_key == 'r')
        {
            quit_flag = 0;
            bullet_count = MAX_BULLETS;
            restartGame(g); // Restart the game
        }
    }
    return 0;
}
