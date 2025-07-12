#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define TERRAIN_SEGMENTS 800
#define GRAVITY 0.1
#define MAX_POWER 100
#define PI 3.14159265358979323846
#define TANK_WIDTH 20
#define TANK_HEIGHT 10
#define MAX_PARTICLES 200
#define MAX_PROJECTILES 20
#define MAX_EXPLOSIONS 10

// Game states
typedef enum
{
    STATE_AIMING,
    STATE_FIRING,
    STATE_EXPLOSION,
    STATE_SWITCHING_PLAYER,
    STATE_GAME_OVER
} GameState;

// Weapon types
typedef enum
{
    WEAPON_SMALL_MISSILE,
    WEAPON_BIG_MISSILE,
    WEAPON_DRILL,
    WEAPON_CLUSTER,
    WEAPON_NUKE,
    WEAPON_COUNT
} WeaponType;

// Structure for projectiles
typedef struct
{
    double x, y;
    double dx, dy;
    WeaponType weapon_type;
    bool active;
    double travel_distance;
    int sub_projectiles;
} Projectile;

// Structure for explosions
typedef struct
{
    double x, y;
    double radius;
    double max_radius;
    double growth_rate;
    bool active;
    int damage;
    int terrain_deformation;
} Explosion;

// Structure for particles
typedef struct
{
    double x, y;
    double dx, dy;
    double lifetime;
    double max_lifetime;
    double size;
    bool active;
} Particle;

// Structure for tanks
typedef struct
{
    double x, y;
    int health;
    int score;
    int angle;
    int power;
    WeaponType current_weapon;
    char name[20];
    int moves_left;
} Tank;

// Structure for weapon properties
typedef struct
{
    char name[20];
    int damage;
    double explosion_radius;
    int terrain_deformation;
    int sub_projectiles;
    double drill_capability;
} WeaponProperty;

// Structure for the game
typedef struct
{
    double terrain[TERRAIN_SEGMENTS];
    Tank players[2];
    int current_player;
    GameState state;
    Projectile projectiles[MAX_PROJECTILES];
    Explosion explosions[MAX_EXPLOSIONS];
    Particle particles[MAX_PARTICLES];
    double wind;
    WeaponProperty weapon_properties[WEAPON_COUNT];
    int frame_count;
    bool game_paused;
} Game;

// Function prototypes
static void generate_terrain(Game *game);
static void init_game(Game *game);
static void update_game(Game *game);
static void render_game(GtkDrawingArea *drawing_area, cairo_t *cr, int width, int height, gpointer user_data);
static void key_pressed(GtkEventController *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data);
static void fire_weapon(Game *game);
static void create_explosion(Game *game, double x, double y, double radius, int damage, int terrain_deformation);
static void apply_explosion_to_terrain(Game *game, double x, double y, double radius, int deformation);
static void create_particles(Game *game, double x, double y, int count, double power);
static void check_tank_positions(Game *game);
static double get_terrain_height(Game *game, int x);
static void reset_game(Game *game);
static void spawn_cluster_bombs(Game *game, double x, double y);
static gboolean tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data);
static void activate(GtkApplication *app, gpointer user_data);
static void update_wind_display(Game *game);

// Global variables
Game game;
GtkWidget *window;

// Add this new activate function above main
static void activate(GtkApplication *app, gpointer user_data)
{
    Game *game = (Game *)user_data;

    // Create window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Artillery Game");
    gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_WIDTH, WINDOW_HEIGHT);

    // Initialize game
    init_game(game);

    // Create drawing area
    GtkWidget *drawing_area = gtk_drawing_area_new();
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area), render_game, game, NULL);
    gtk_widget_set_size_request(drawing_area, WINDOW_WIDTH, WINDOW_HEIGHT);
    gtk_window_set_child(GTK_WINDOW(window), drawing_area);

    // Add key event controller
    GtkEventController *key_controller = gtk_event_controller_key_new();
    g_signal_connect(key_controller, "key-pressed", G_CALLBACK(key_pressed), game);
    gtk_widget_add_controller(window, key_controller);

    // Add tick callback
    gtk_widget_add_tick_callback(drawing_area, tick, NULL, NULL);

    // Show window
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[])
{
    GtkApplication *app;
    int status;

    srand(time(NULL));

    // Create GTK application
    app = gtk_application_new("org.example.ArtilleryGame", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &game);

    // Run application
    status = g_application_run(G_APPLICATION(app), argc, argv);

    // Cleanup
    g_object_unref(app);

    return status;
}

// Function to initialize weapon properties
void init_weapons(Game *game)
{
    // Small missile
    strcpy(game->weapon_properties[WEAPON_SMALL_MISSILE].name, "Small Missile");
    game->weapon_properties[WEAPON_SMALL_MISSILE].damage = 25;
    game->weapon_properties[WEAPON_SMALL_MISSILE].explosion_radius = 20;
    game->weapon_properties[WEAPON_SMALL_MISSILE].terrain_deformation = 10;
    game->weapon_properties[WEAPON_SMALL_MISSILE].sub_projectiles = 0;
    game->weapon_properties[WEAPON_SMALL_MISSILE].drill_capability = 0;

    // Big missile
    strcpy(game->weapon_properties[WEAPON_BIG_MISSILE].name, "Big Missile");
    game->weapon_properties[WEAPON_BIG_MISSILE].damage = 40;
    game->weapon_properties[WEAPON_BIG_MISSILE].explosion_radius = 40;
    game->weapon_properties[WEAPON_BIG_MISSILE].terrain_deformation = 25;
    game->weapon_properties[WEAPON_BIG_MISSILE].sub_projectiles = 0;
    game->weapon_properties[WEAPON_BIG_MISSILE].drill_capability = 0;

    // Drill
    strcpy(game->weapon_properties[WEAPON_DRILL].name, "Drill");
    game->weapon_properties[WEAPON_DRILL].damage = 35;
    game->weapon_properties[WEAPON_DRILL].explosion_radius = 15;
    game->weapon_properties[WEAPON_DRILL].terrain_deformation = 30;
    game->weapon_properties[WEAPON_DRILL].sub_projectiles = 0;
    game->weapon_properties[WEAPON_DRILL].drill_capability = 1.0;

    // Cluster
    strcpy(game->weapon_properties[WEAPON_CLUSTER].name, "Cluster Bomb");
    game->weapon_properties[WEAPON_CLUSTER].damage = 15;
    game->weapon_properties[WEAPON_CLUSTER].explosion_radius = 10;
    game->weapon_properties[WEAPON_CLUSTER].terrain_deformation = 5;
    game->weapon_properties[WEAPON_CLUSTER].sub_projectiles = 5;
    game->weapon_properties[WEAPON_CLUSTER].drill_capability = 0;

    // Nuke
    strcpy(game->weapon_properties[WEAPON_NUKE].name, "Nuke");
    game->weapon_properties[WEAPON_NUKE].damage = 75;
    game->weapon_properties[WEAPON_NUKE].explosion_radius = 80;
    game->weapon_properties[WEAPON_NUKE].terrain_deformation = 70;
    game->weapon_properties[WEAPON_NUKE].sub_projectiles = 0;
    game->weapon_properties[WEAPON_NUKE].drill_capability = 0;
}

// Function to initialize the game
static void init_game(Game *game)
{
    game->current_player = 0;
    game->state = STATE_AIMING;
    game->frame_count = 0;
    game->game_paused = false;
    game->players[0].moves_left = 3;
    game->players[1].moves_left = 3;

    // Initialize weapons
    init_weapons(game);

    // Initialize players
    strcpy(game->players[0].name, "Player 1");
    game->players[0].health = 100;
    game->players[0].score = 0;
    game->players[0].angle = 45;
    game->players[0].power = 50;
    game->players[0].current_weapon = WEAPON_SMALL_MISSILE;

    strcpy(game->players[1].name, "Player 2");
    game->players[1].health = 100;
    game->players[1].score = 0;
    game->players[1].angle = 135;
    game->players[1].power = 50;
    game->players[1].current_weapon = WEAPON_SMALL_MISSILE;

    // Generate random wind
    do
    {
        game->wind = (rand() % 21 - 10) * 0.01;
    } while (fabs(game->wind) < 0.02);

    // Initialize projectiles
    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        game->projectiles[i].active = false;
    }

    // Initialize explosions
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        game->explosions[i].active = false;
    }

    // Initialize particles
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        game->particles[i].active = false;
    }

    // Generate terrain
    generate_terrain(game);

    // Position tanks on the terrain
    game->players[0].x = WINDOW_WIDTH * 0.25;
    game->players[1].x = WINDOW_WIDTH * 0.75;

    // Set Y positions based on terrain
    check_tank_positions(game);
}

// Function to generate terrain using sine waves
static void generate_terrain(Game *game)
{
    // Base height
    double base_height = WINDOW_HEIGHT * 0.7;

    // Generate terrain using multiple layers
    for (int i = 0; i < TERRAIN_SEGMENTS; i++)
    {
        double x = (double)i / TERRAIN_SEGMENTS * WINDOW_WIDTH;
        double height = base_height;

        // Large mountains
        height += sin(x * 0.002) * 120;

        // Medium hills
        height += sin(x * 0.01) * 50;
        height += cos(x * 0.005) * 40;

        // Small hills
        height += sin(x * 0.03) * 20 * (cos(x * 0.001) + 1);

        // Rough terrain details
        height += sin(x * 0.2) * 5;

        // Random noise for texture
        height += (rand() % 10 - 5) * (sin(x * 0.01) + 1);

        // Ensure height stays within bounds
        height = fmax(height, WINDOW_HEIGHT * 0.3);
        height = fmin(height, WINDOW_HEIGHT * 0.85);

        game->terrain[i] = height;
    }

    // Smooth the terrain
    double smoothing_passes = 2;
    while (smoothing_passes-- > 0)
    {
        double prev = game->terrain[0];
        for (int i = 1; i < TERRAIN_SEGMENTS - 1; i++)
        {
            double current = game->terrain[i];
            game->terrain[i] = (prev + current + game->terrain[i + 1]) / 3.0;
            prev = current;
        }
    }

    // Add small terrain features
    for (int i = 1; i < TERRAIN_SEGMENTS - 1; i++)
    {
        if (rand() % 50 == 0)
        { // Random small bumps
            double bump_width = 5 + (rand() % 10);
            double bump_height = 5 + (rand() % 10);

            for (int j = -bump_width; j <= bump_width; j++)
            {
                if (i + j >= 0 && i + j < TERRAIN_SEGMENTS)
                {
                    double factor = cos((j / bump_width) * PI) * 0.5 + 0.5;
                    game->terrain[i + j] += bump_height * factor;
                }
            }
        }
    }
}

// Function to get terrain height at a specific x coordinate
static double get_terrain_height(Game *game, int x)
{
    if (x < 0)
        return WINDOW_HEIGHT;
    if (x >= WINDOW_WIDTH)
        return WINDOW_HEIGHT;

    int index = (int)((double)x / WINDOW_WIDTH * TERRAIN_SEGMENTS);
    if (index < 0)
        index = 0;
    if (index >= TERRAIN_SEGMENTS)
        index = TERRAIN_SEGMENTS - 1;

    return game->terrain[index];
}

// Function to check and adjust tank positions based on terrain
static void check_tank_positions(Game *game)
{
    for (int i = 0; i < 2; i++)
    {
        int tank_x = (int)game->players[i].x;
        game->players[i].y = get_terrain_height(game, tank_x) - TANK_HEIGHT / 2;
    }
}

// Function to handle key press events
static void key_pressed(GtkEventController *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
    Game *game = (Game *)user_data;

    // Special handling for R key - allow it to work even when game is over
    if (keyval == GDK_KEY_r || keyval == GDK_KEY_R)
    {
        // Reset scores but keep other player info
        int p1_score = game->players[0].score;
        int p2_score = game->players[1].score;

        // Full game reset
        init_game(game);

        // Reset player health and positions
        game->players[0].health = 100;
        game->players[1].health = 100;

        // Restore scores
        game->players[0].score = p1_score;
        game->players[1].score = p2_score;

        // Reset game state and player turns
        game->state = STATE_AIMING;
        game->current_player = 0;
        game->players[0].moves_left = 3;
        game->players[1].moves_left = 3;

        // Clear all active projectiles, explosions, and particles
        for (int i = 0; i < MAX_PROJECTILES; i++)
        {
            game->projectiles[i].active = false;
        }
        for (int i = 0; i < MAX_EXPLOSIONS; i++)
        {
            game->explosions[i].active = false;
        }
        for (int i = 0; i < MAX_PARTICLES; i++)
        {
            game->particles[i].active = false;
        }

        // Generate new wind
        do
        {
            game->wind = (rand() % 21 - 10) * 0.01;
        } while (fabs(game->wind) < 0.02);

        // Force redraw
        if (window != NULL)
        {
            gtk_widget_queue_draw(window);
        }
        return;
    }

    // Skip if game is over or not in aiming state
    if (game->state == STATE_GAME_OVER || game->state != STATE_AIMING)
        return;

    Tank *current_tank = &game->players[game->current_player];

    switch (keyval)
    {
    case GDK_KEY_Left:
        // Decrease angle
        current_tank->angle = (current_tank->angle - 1) % 360;
        if (current_tank->angle < 0)
            current_tank->angle += 360;
        break;

    case GDK_KEY_Right:
        // Increase angle
        current_tank->angle = (current_tank->angle + 1) % 360;
        break;

    case GDK_KEY_Up:
        // Increase power
        if (current_tank->power < MAX_POWER)
            current_tank->power++;
        break;

    case GDK_KEY_Down:
        // Decrease power
        if (current_tank->power > 1)
            current_tank->power--;
        break;

    case GDK_KEY_w:
    case GDK_KEY_W:
        // Select next weapon
        current_tank->current_weapon = (current_tank->current_weapon + 1) % WEAPON_COUNT;
        break;

    case GDK_KEY_s:
    case GDK_KEY_S:
        // Select previous weapon
        current_tank->current_weapon = (current_tank->current_weapon - 1 + WEAPON_COUNT) % WEAPON_COUNT;
        break;

    case GDK_KEY_space:
        // Fire weapon
        fire_weapon(game);
        break;

    case GDK_KEY_r:
    case GDK_KEY_R:
        // Allow reset at any time, regardless of game state
        {
            // Reset scores but keep other player info
            int p1_score = game->players[0].score;
            int p2_score = game->players[1].score;

            // Full game reset
            init_game(game);

            // Reset player health and positions
            game->players[0].health = 100;
            game->players[1].health = 100;

            // Restore scores
            game->players[0].score = p1_score;
            game->players[1].score = p2_score;

            // Reset game state and player turns
            game->state = STATE_AIMING;
            game->current_player = 0;
            game->players[0].moves_left = 3;
            game->players[1].moves_left = 3;

            // Clear all active projectiles, explosions, and particles
            for (int i = 0; i < MAX_PROJECTILES; i++)
            {
                game->projectiles[i].active = false;
            }
            for (int i = 0; i < MAX_EXPLOSIONS; i++)
            {
                game->explosions[i].active = false;
            }
            for (int i = 0; i < MAX_PARTICLES; i++)
            {
                game->particles[i].active = false;
            }

            // Generate new wind
            do
            {
                game->wind = (rand() % 21 - 10) * 0.01;
            } while (fabs(game->wind) < 0.02);

            // Force redraw
            if (window != NULL)
            {
                gtk_widget_queue_draw(window);
            }
        }
        break;

    case GDK_KEY_p:
    case GDK_KEY_P:
        // Toggle pause
        game->game_paused = !game->game_paused;
        break;

    case GDK_KEY_a:
    case GDK_KEY_A:
        if (current_tank->moves_left > 0)
        {
            // Move left
            current_tank->x -= 22.0;
            if (current_tank->x < TANK_WIDTH / 2)
            {
                current_tank->x = TANK_WIDTH / 2;
            }
            check_tank_positions(game);
            current_tank->moves_left--;
        }
        break;

    case GDK_KEY_d:
    case GDK_KEY_D:
        if (current_tank->moves_left > 0)
        {
            // Move right
            current_tank->x += 22.0;
            if (current_tank->x > WINDOW_WIDTH - TANK_WIDTH / 2)
            {
                current_tank->x = WINDOW_WIDTH - TANK_WIDTH / 2;
            }
            check_tank_positions(game);
            current_tank->moves_left--;
        }
        break;
    }
}

// Function to fire the current weapon
static void fire_weapon(Game *game)
{
    if (game->state != STATE_AIMING)
        return;

    Tank *current_tank = &game->players[game->current_player];

    // Find an inactive projectile
    int proj_index = -1;
    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        if (!game->projectiles[i].active)
        {
            proj_index = i;
            break;
        }
    }

    if (proj_index == -1)
        return; // No available projectiles

    // Activate projectile
    Projectile *proj = &game->projectiles[proj_index];
    proj->active = true;
    proj->weapon_type = current_tank->current_weapon;

    // Set starting position (tank barrel)
    double angle_rad = current_tank->angle * PI / 180.0;
    double barrel_length = 20.0;

    // Adjust starting position based on player
    if (game->current_player == 0)
    {
        proj->x = current_tank->x + cos(angle_rad) * barrel_length;
        proj->y = current_tank->y - sin(angle_rad) * barrel_length;
    }
    else
    {
        // For player 2, adjust the angle calculation
        proj->x = current_tank->x + cos(angle_rad) * barrel_length;
        proj->y = current_tank->y - sin(angle_rad) * barrel_length;
    }

    // Set velocity based on power and angle
    double power_factor = (double)current_tank->power / MAX_POWER * 10.0;
    proj->dx = cos(angle_rad) * power_factor;
    proj->dy = -sin(angle_rad) * power_factor;

    proj->travel_distance = 0;
    proj->sub_projectiles = game->weapon_properties[current_tank->current_weapon].sub_projectiles;

    // Change state to firing
    game->state = STATE_FIRING;
}

// Function to create an explosion
static void create_explosion(Game *game, double x, double y, double radius, int damage, int terrain_deformation)
{
    // Find an inactive explosion
    int exp_index = -1;
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        if (!game->explosions[i].active)
        {
            exp_index = i;
            break;
        }
    }

    if (exp_index == -1)
        return; // No available explosions

    // Activate explosion
    Explosion *exp = &game->explosions[exp_index];
    exp->active = true;
    exp->x = x;
    exp->y = y;
    exp->radius = 1.0;
    exp->max_radius = radius;
    exp->growth_rate = radius / 10.0; // Grow to full size in 10 frames
    exp->damage = damage;
    exp->terrain_deformation = terrain_deformation;

    // Create particles for visual effect
    create_particles(game, x, y, 30, radius);

    // Apply damage to tanks if in explosion radius
    for (int i = 0; i < 2; i++)
    {
        double dx = game->players[i].x - x;
        double dy = game->players[i].y - y;
        double distance = sqrt(dx * dx + dy * dy);

        if (distance < radius * 1.5) // Increase damage radius by 50%
        {
            // Apply damage with falloff based on distance, but with a higher minimum damage
            double damage_factor = 1.0 - (distance / (radius * 1.5));
            damage_factor = fmax(damage_factor, 0.3); // Minimum 30% damage even at edge of radius

            int applied_damage = (int)(damage * damage_factor * 1.5); // Multiply damage by 1.5
            game->players[i].health -= applied_damage;

            // Ensure health doesn't go below 0
            if (game->players[i].health < 0)
                game->players[i].health = 0;
        }
    }

    // Apply explosion to terrain
    apply_explosion_to_terrain(game, x, y, radius, terrain_deformation);

    // Set game state to explosion
    game->state = STATE_EXPLOSION;
}

// Function to spawn cluster bombs
static void spawn_cluster_bombs(Game *game, double x, double y)
{
    int count = 5; // Number of sub-projectiles

    for (int i = 0; i < count; i++)
    {
        // Find an inactive projectile
        int proj_index = -1;
        for (int j = 0; j < MAX_PROJECTILES; j++)
        {
            if (!game->projectiles[j].active)
            {
                proj_index = j;
                break;
            }
        }

        if (proj_index == -1)
            continue; // No available projectiles

        // Activate projectile
        Projectile *proj = &game->projectiles[proj_index];
        proj->active = true;
        proj->weapon_type = WEAPON_SMALL_MISSILE; // Use small missile properties

        // Set starting position (slightly randomized)
        proj->x = x + (rand() % 11 - 5);
        proj->y = y + (rand() % 11 - 5);

        // Set random velocity
        double angle = (rand() % 360) * PI / 180.0;
        double power = (rand() % 5) + 3.0;

        proj->dx = cos(angle) * power;
        proj->dy = -sin(angle) * power;

        proj->travel_distance = 0;
        proj->sub_projectiles = 0; // No more sub-projectiles
    }
}

// Function to apply explosion to terrain
static void apply_explosion_to_terrain(Game *game, double x, double y, double radius, int deformation)
{
    // Calculate the range of affected terrain segments
    int start_index = (int)((x - radius) / WINDOW_WIDTH * TERRAIN_SEGMENTS);
    int end_index = (int)((x + radius) / WINDOW_WIDTH * TERRAIN_SEGMENTS);

    // Clamp indices
    if (start_index < 0)
        start_index = 0;
    if (end_index >= TERRAIN_SEGMENTS)
        end_index = TERRAIN_SEGMENTS - 1;

    // Apply crater effect
    for (int i = start_index; i <= end_index; i++)
    {
        double segment_x = (double)i / TERRAIN_SEGMENTS * WINDOW_WIDTH;
        double dx = segment_x - x;
        double distance = fabs(dx);

        if (distance < radius)
        {
            // Crater shape (semicircle)
            double crater_depth = sqrt(radius * radius - dx * dx) / radius * deformation;
            game->terrain[i] += crater_depth;
        }
    }

    // Check if tanks need to be repositioned
    check_tank_positions(game);
}

// Function to create particles
static void create_particles(Game *game, double x, double y, int count, double power)
{
    for (int i = 0; i < count; i++)
    {
        // Find an inactive particle
        int part_index = -1;
        for (int j = 0; j < MAX_PARTICLES; j++)
        {
            if (!game->particles[j].active)
            {
                part_index = j;
                break;
            }
        }

        if (part_index == -1)
            continue; // No available particles

        // Activate particle
        Particle *part = &game->particles[part_index];
        part->active = true;
        part->x = x;
        part->y = y;

        // Random velocity in all directions
        double angle = (rand() % 360) * PI / 180.0;
        double speed = (rand() % (int)(power * 0.5)) + power * 0.2;

        part->dx = cos(angle) * speed;
        part->dy = sin(angle) * speed;

        // Random lifetime and size
        part->lifetime = (rand() % 30) + 20;
        part->max_lifetime = part->lifetime;
        part->size = (rand() % 3) + 2;
    }
}

// Function to reset the game
static void reset_game(Game *game)
{
    // Reset scores but keep other player info
    int p1_score = game->players[0].score;
    int p2_score = game->players[1].score;

    init_game(game);

    // Restore scores
    game->players[0].score = p1_score;
    game->players[1].score = p2_score;

    // Ensure game state is reset to aiming
    game->state = STATE_AIMING;
    game->current_player = 0;
    game->players[0].moves_left = 3;
    game->players[1].moves_left = 3;
}

// Function to update game state
static void update_game(Game *game)
{
    if (game->game_paused)
        return;

    game->frame_count++;

    // Update projectiles
    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        if (game->projectiles[i].active)
        {
            Projectile *proj = &game->projectiles[i];

            // Apply wind and gravity
            proj->dx += game->wind * 0.25;
            proj->dy += GRAVITY;

            // Update position
            proj->x += proj->dx;
            proj->y += proj->dy;

            // Track distance traveled
            proj->travel_distance += sqrt(proj->dx * proj->dx + proj->dy * proj->dy);

            // Check for terrain collision
            if (proj->y >= get_terrain_height(game, (int)proj->x))
            {
                // Handle drill weapons differently
                double drill_capability = game->weapon_properties[proj->weapon_type].drill_capability;

                if (drill_capability > 0 && proj->travel_distance < 100)
                {
                    // Drill through terrain
                    proj->dx *= 0.8; // Slow down when drilling
                    proj->dy *= 0.8;
                }
                else
                {
                    // Explosion
                    proj->active = false;

                    // Get weapon properties
                    WeaponProperty *wp = &game->weapon_properties[proj->weapon_type];

                    // Create explosion
                    create_explosion(game, proj->x, proj->y, wp->explosion_radius, wp->damage, wp->terrain_deformation);

                    // Handle cluster bombs
                    if (proj->sub_projectiles > 0)
                    {
                        spawn_cluster_bombs(game, proj->x, proj->y);
                    }
                }
            }

            // Check if out of bounds
            if (proj->x < 0 || proj->x > WINDOW_WIDTH || proj->y > WINDOW_HEIGHT)
            {
                proj->active = false;
            }
        }
    }

    // Update explosions
    bool all_explosions_done = true;
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        if (game->explosions[i].active)
        {
            Explosion *exp = &game->explosions[i];

            // Grow explosion
            exp->radius += exp->growth_rate;

            // Check if explosion is done
            if (exp->radius >= exp->max_radius)
            {
                exp->active = false;
            }
            else
            {
                all_explosions_done = false;
            }
        }
    }

    // Update particles
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (game->particles[i].active)
        {
            Particle *part = &game->particles[i];

            // Apply gravity
            part->dy += GRAVITY * 0.1;

            // Update position
            part->x += part->dx;
            part->y += part->dy;

            // Check for terrain collision
            if (part->y >= get_terrain_height(game, (int)part->x))
            {
                part->dy *= -0.5; // Bounce
                part->dx *= 0.8;  // Friction
                part->y = get_terrain_height(game, (int)part->x) - 1;
            }

            // Decrease lifetime
            part->lifetime--;

            // Check if particle is done
            if (part->lifetime <= 0 || part->x < 0 || part->x > WINDOW_WIDTH || part->y > WINDOW_HEIGHT)
            {
                part->active = false;
            }
        }
    }

    // Check if all projectiles and explosions are done
    bool all_projectiles_done = true;
    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        if (game->projectiles[i].active)
        {
            all_projectiles_done = false;
            break;
        }
    }

    // State transitions
    if ((game->state == STATE_FIRING || game->state == STATE_EXPLOSION) && all_projectiles_done && all_explosions_done)
    {
        // Check if game is over
        if (game->players[0].health <= 0 || game->players[1].health <= 0)
        {
            game->state = STATE_GAME_OVER;

            // Award score to winner
            if (game->players[0].health <= 0)
            {
                game->players[1].score++;
            }
            else
            {
                game->players[0].score++;
            }
        }
        else
        {
            // Switch player
            game->current_player = 1 - game->current_player;
            game->state = STATE_AIMING;

            // Reset moves for the new player's turn
            game->players[game->current_player].moves_left = 3;

            // Generate new random wind
            srand(time(NULL) + game->frame_count); // Ensure better randomization
            double wind_magnitude = (0.02 + (rand() % 31) / 1000.0);
            int direction = (rand() % 2) * 2 - 1;
            game->wind = wind_magnitude * direction;

            // Ensure wind is never exactly zero
            if (fabs(game->wind) < 0.02)
            {
                game->wind = (game->wind >= 0) ? 0.02 : -0.02;
            }

            // Update wind display
            update_wind_display(game);

            // Debug print to verify wind changes
            printf("New Wind: %.3f\n", game->wind);

            // Force redraw of the window to update wind display
            // if (window != NULL)
            // {
            //     gtk_widget_queue_draw(window);
            // }
        }
    }

    // Always check tank positions to adjust for terrain changes
    check_tank_positions(game);
}

// Add this new function implementation after update_game
static void update_wind_display(Game *game)
{
    // Debug print
    printf("New Wind: %.3f\n", game->wind);

    // Force immediate redraw
    if (window != NULL)
    {
        gtk_widget_queue_draw(window);

        // Process any pending events using GTK4's event system
        while (g_main_context_iteration(NULL, FALSE))
            ;
    }
}

// Function to render the game
static void render_game(GtkDrawingArea *drawing_area, cairo_t *cr, int width, int height, gpointer user_data)
{
    Game *game = (Game *)user_data;

    // Clear background
    cairo_set_source_rgb(cr, 0.2, 0.6, 0.9); // Sky blue
    cairo_paint(cr);

    // Create terrain path
    cairo_move_to(cr, 0, WINDOW_HEIGHT);
    for (int i = 0; i < TERRAIN_SEGMENTS; i++)
    {
        double x = (double)i / TERRAIN_SEGMENTS * WINDOW_WIDTH;
        cairo_line_to(cr, x, game->terrain[i]);
    }
    cairo_line_to(cr, WINDOW_WIDTH, WINDOW_HEIGHT);
    cairo_close_path(cr);

    // Draw terrain with gradient
    cairo_pattern_t *terrain_gradient = cairo_pattern_create_linear(0, 0, 0, WINDOW_HEIGHT);
    cairo_pattern_add_color_stop_rgb(terrain_gradient, 0.0, 0.2, 0.5, 0.1); // Dark green top
    cairo_pattern_add_color_stop_rgb(terrain_gradient, 0.3, 0.3, 0.6, 0.2); // Medium green middle
    cairo_pattern_add_color_stop_rgb(terrain_gradient, 1.0, 0.1, 0.4, 0.1); // Deep green bottom
    cairo_set_source(cr, terrain_gradient);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(terrain_gradient);

    // Modify the grass rendering section in render_game:

    // Add this at the start of render_game function to seed random number generator
    static unsigned int grass_seed = 0;
    srand(grass_seed); // Use consistent seed for grass

    // Replace the grass layer section with this:
    // Add grass layer on top
    for (int i = 0; i < TERRAIN_SEGMENTS - 1; i++)
    {
        double x = (double)i / TERRAIN_SEGMENTS * WINDOW_WIDTH;
        double y = game->terrain[i];

        // Use deterministic random based on position
        if ((i * 7919) % 17 < 6)
        { // Prime numbers for better distribution
            double grass_height = 2 + ((i * 3779) % 4);
            // Brighter, more varied greens
            cairo_set_source_rgba(cr,
                                  0.2 + ((i * 1597) % 20) / 100.0, // Red component
                                  0.6 + ((i * 2389) % 30) / 100.0, // Green component
                                  0.1 + ((i * 3571) % 15) / 100.0, // Blue component
                                  0.9);                            // More opaque

            cairo_set_line_width(cr, 1.0);
            cairo_move_to(cr, x, y);
            // Use position-based randomization for angle
            double grass_angle = ((i * 4463) % 40 - 20) * PI / 180.0;
            cairo_line_to(cr,
                          x + cos(grass_angle) * grass_height,
                          y - grass_height);
            cairo_stroke(cr);
        }
    }

    // Add terrain texture and details
    for (int i = 0; i < TERRAIN_SEGMENTS; i += 2)
    {
        double x = (double)i / TERRAIN_SEGMENTS * WINDOW_WIDTH;
        double y = game->terrain[i];

        // Rock details
        if (rand() % 20 == 0)
        {
            cairo_set_source_rgba(cr, 0.4 + (rand() % 20) / 100.0,
                                  0.3 + (rand() % 20) / 100.0,
                                  0.2 + (rand() % 20) / 100.0,
                                  0.7);
            double rock_size = 2 + (rand() % 4);
            cairo_arc(cr, x, y - rock_size / 2, rock_size, 0, 2 * PI);
            cairo_fill(cr);
        }

        // Soil texture
        for (int j = 0; j < 3; j++)
        {
            double dx = (rand() % 5) - 2;
            double dy = (rand() % 10);
            if (y + dy < WINDOW_HEIGHT)
            {
                cairo_set_source_rgba(cr, 0.2, 0.5, 0.1, 0.1); // Green soil texture
                cairo_rectangle(cr, x + dx, y + dy, 1, 1);
                cairo_fill(cr);
            }
        }
    }

    // Add terrain contours
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.1);
    cairo_set_line_width(cr, 0.5);
    for (int i = 0; i < TERRAIN_SEGMENTS - 10; i += 10)
    {
        double x1 = (double)i / TERRAIN_SEGMENTS * WINDOW_WIDTH;
        double x2 = (double)(i + 10) / TERRAIN_SEGMENTS * WINDOW_WIDTH;
        double y1 = game->terrain[i];
        double y2 = game->terrain[i + 10];

        cairo_move_to(cr, x1, y1);
        cairo_curve_to(cr,
                       x1 + 3, y1,
                       x2 - 3, y2,
                       x2, y2);
        cairo_stroke(cr);
    }

    // Add terrain shadows
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.2);
    for (int i = 1; i < TERRAIN_SEGMENTS; i++)
    {
        double x = (double)i / TERRAIN_SEGMENTS * WINDOW_WIDTH;
        double y = game->terrain[i];
        double prev_y = game->terrain[i - 1];

        if (y > prev_y)
        { // Create shadow on rising slopes
            cairo_move_to(cr, x, y);
            cairo_line_to(cr, x, prev_y);
            cairo_stroke(cr);
        }
    }

    // Draw tanks
    for (int i = 0; i < 2; i++)
    {
        // Choose color based on player
        if (i == 0)
        {
            cairo_set_source_rgb(cr, 0.8, 0.2, 0.2); // Red
        }
        else
        {
            cairo_set_source_rgb(cr, 0.2, 0.2, 0.8); // Blue
        }

        // Draw tank body
        cairo_rectangle(cr, game->players[i].x - TANK_WIDTH / 2, game->players[i].y - TANK_HEIGHT / 2, TANK_WIDTH, TANK_HEIGHT);
        cairo_fill(cr);

        // Draw tank barrel
        double angle_rad = game->players[i].angle * PI / 180.0;
        double barrel_length = 20.0;
        double barrel_width = 3.0;

        // Barrel start position
        double barrel_start_x = game->players[i].x;
        double barrel_start_y = game->players[i].y - TANK_HEIGHT / 4;

        // Barrel end position
        double barrel_end_x = barrel_start_x + cos(angle_rad) * barrel_length;
        double barrel_end_y = barrel_start_y - sin(angle_rad) * barrel_length;

        // Draw barrel
        cairo_set_line_width(cr, barrel_width);
        cairo_move_to(cr, barrel_start_x, barrel_start_y);
        cairo_line_to(cr, barrel_end_x, barrel_end_y);
        cairo_stroke(cr);

        // Draw health bar
        cairo_set_source_rgb(cr, 0.8, 0.2, 0.2); // Red background
        cairo_rectangle(cr, game->players[i].x - TANK_WIDTH / 2, game->players[i].y - TANK_HEIGHT - 10, TANK_WIDTH, 5);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 0.2, 0.8, 0.2); // Green health
        cairo_rectangle(cr, game->players[i].x - TANK_WIDTH / 2, game->players[i].y - TANK_HEIGHT - 10, TANK_WIDTH * game->players[i].health / 100.0, 5);
        cairo_fill(cr);
    }

    // Draw projectiles
    for (int i = 0; i < MAX_PROJECTILES; i++)
    {
        if (game->projectiles[i].active)
        {
            Projectile *proj = &game->projectiles[i];

            // Replace the projectile coloring section with brighter colors:
            switch (proj->weapon_type)
            {
            case WEAPON_SMALL_MISSILE:
                cairo_set_source_rgb(cr, 1.0, 0.9, 0.2); // Bright yellow
                cairo_arc(cr, proj->x, proj->y, 3, 0, 2 * PI);
                cairo_fill(cr);
                // Add glow effect
                cairo_set_source_rgba(cr, 1.0, 0.9, 0.2, 0.3);
                cairo_arc(cr, proj->x, proj->y, 5, 0, 2 * PI);
                cairo_fill(cr);
                break;

            case WEAPON_BIG_MISSILE:
                cairo_set_source_rgb(cr, 1.0, 0.5, 0.0); // Bright orange
                cairo_arc(cr, proj->x, proj->y, 5, 0, 2 * PI);
                cairo_fill(cr);
                // Add glow effect
                cairo_set_source_rgba(cr, 1.0, 0.5, 0.0, 0.3);
                cairo_arc(cr, proj->x, proj->y, 7, 0, 2 * PI);
                cairo_fill(cr);
                break;

            case WEAPON_DRILL:
                cairo_set_source_rgb(cr, 0.7, 0.7, 0.9); // Bright metallic
                cairo_save(cr);
                cairo_translate(cr, proj->x, proj->y);
                double angle = atan2(proj->dy, proj->dx);
                cairo_rotate(cr, angle);
                cairo_move_to(cr, 0, 0);
                cairo_line_to(cr, 8, -3);
                cairo_line_to(cr, 8, 3);
                cairo_close_path(cr);
                cairo_fill(cr);
                cairo_restore(cr);
                break;

            case WEAPON_CLUSTER:
                cairo_set_source_rgb(cr, 1.0, 0.3, 1.0); // Bright purple
                cairo_arc(cr, proj->x, proj->y, 4, 0, 2 * PI);
                cairo_fill(cr);
                // Add glow effect
                cairo_set_source_rgba(cr, 1.0, 0.3, 1.0, 0.3);
                cairo_arc(cr, proj->x, proj->y, 6, 0, 2 * PI);
                cairo_fill(cr);
                break;

            case WEAPON_NUKE:
                // Draw blinking nuke symbol
                if (game->frame_count % 10 < 5)
                {
                    cairo_set_source_rgb(cr, 0.8, 0.0, 0.0); // Red
                }
                else
                {
                    cairo_set_source_rgb(cr, 1.0, 1.0, 0.0); // Yellow
                }
                cairo_arc(cr, proj->x, proj->y, 6, 0, 2 * PI);
                cairo_fill(cr);

                // Draw radiation symbol
                cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // Black
                double radius = 4;
                for (int j = 0; j < 3; j++)
                {
                    double angle = j * (2 * PI / 3);
                    cairo_save(cr);
                    cairo_translate(cr, proj->x, proj->y);
                    cairo_rotate(cr, angle);
                    cairo_move_to(cr, 0, 0);
                    cairo_arc(cr, 0, -radius, radius / 2, 0, PI);
                    cairo_close_path(cr);
                    cairo_fill(cr);
                    cairo_restore(cr);
                }
                break;
            }
        }
    }

    // Draw explosions
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        if (game->explosions[i].active)
        {
            Explosion *exp = &game->explosions[i];

            // Draw explosion with gradient
            cairo_pattern_t *pattern = cairo_pattern_create_radial(
                exp->x, exp->y, 0,
                exp->x, exp->y, exp->radius);

            cairo_pattern_add_color_stop_rgba(pattern, 0.0, 1.0, 0.7, 0.0, 0.8); // Orange center
            cairo_pattern_add_color_stop_rgba(pattern, 0.7, 0.8, 0.2, 0.0, 0.5); // Red middle
            cairo_pattern_add_color_stop_rgba(pattern, 1.0, 0.5, 0.0, 0.0, 0.0); // Transparent edge

            cairo_set_source(cr, pattern);
            cairo_arc(cr, exp->x, exp->y, exp->radius, 0, 2 * PI);
            cairo_fill(cr);

            cairo_pattern_destroy(pattern);
        }
    }

    // Draw particles
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (game->particles[i].active)
        {
            Particle *part = &game->particles[i];

            // Fade out based on lifetime
            double alpha = part->lifetime / part->max_lifetime;
            cairo_set_source_rgba(cr, 0.5, 0.3, 0.1, alpha); // Brown with fade

            cairo_arc(cr, part->x, part->y, part->size, 0, 2 * PI);
            cairo_fill(cr);
        }
    }

    // Draw UI
    // Wind indicator text with direction
    char wind_text[100];
    char direction[10];
    if (game->wind > 0)
    {
        strcpy(direction, "RIGHT");
    }
    else
    {
        strcpy(direction, "LEFT");
    }
    sprintf(wind_text, "Wind: %.3f (%s)", fabs(game->wind), direction);

    // Make wind display more prominent
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 24);                   // Increased size
    cairo_move_to(cr, WINDOW_WIDTH / 2 - 120, 40); // Adjusted position
    cairo_show_text(cr, wind_text);

    // Draw wind arrow (make it more visible)
    double arrow_center_x = WINDOW_WIDTH / 2 + 150;
    double arrow_y = 35;
    double arrow_length = game->wind * 1200.0; // Increased scale for better visibility
    double arrow_width = 4.0;                  // Thicker arrow

    // Calculate arrow start and end positions based on direction
    double arrow_start_x, arrow_end_x;
    if (game->wind > 0)
    {
        arrow_start_x = arrow_center_x - fabs(arrow_length) / 2;
        arrow_end_x = arrow_center_x + fabs(arrow_length) / 2;
    }
    else
    {
        arrow_start_x = arrow_center_x + fabs(arrow_length) / 2;
        arrow_end_x = arrow_center_x - fabs(arrow_length) / 2;
    }

    // Draw arrow line
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.8);
    cairo_set_line_width(cr, arrow_width);
    cairo_move_to(cr, arrow_start_x, arrow_y);
    cairo_line_to(cr, arrow_end_x, arrow_y);
    cairo_stroke(cr);

    // Draw larger arrow head
    double arrow_head_size = 8.0;
    if (game->wind > 0)
    {
        cairo_move_to(cr, arrow_end_x, arrow_y);
        cairo_line_to(cr, arrow_end_x - arrow_head_size, arrow_y - arrow_head_size);
        cairo_line_to(cr, arrow_end_x - arrow_head_size, arrow_y + arrow_head_size);
    }
    else
    {
        cairo_move_to(cr, arrow_end_x, arrow_y);
        cairo_line_to(cr, arrow_end_x + arrow_head_size, arrow_y - arrow_head_size);
        cairo_line_to(cr, arrow_end_x + arrow_head_size, arrow_y + arrow_head_size);
    }
    cairo_close_path(cr);
    cairo_fill(cr);

    // In the render_game function, modify the player info section:

    // Player info
    for (int i = 0; i < 2; i++)
    {
        // Position text based on player
        double text_x = (i == 0) ? 20 : WINDOW_WIDTH - 320; // Adjusted x position for larger text

        // Highlight current player
        if (i == game->current_player && game->state == STATE_AIMING)
        {
            cairo_set_source_rgb(cr, 0.7, 0.0, 0.0); // Dark red for current player
        }
        else
        {
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // Black for others
        }

        // Player name and score
        char player_text[100];
        sprintf(player_text, "%s: %d pts (Health: %d)", game->players[i].name, game->players[i].score, game->players[i].health);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 20); // Increased from 14 to 20

        cairo_move_to(cr, text_x, 50); // Adjusted y position
        cairo_show_text(cr, player_text);

        // Current weapon
        char weapon_text[100];
        sprintf(weapon_text, "Weapon: %s", game->weapon_properties[game->players[i].current_weapon].name);
        cairo_move_to(cr, text_x, 80); // Adjusted spacing
        cairo_show_text(cr, weapon_text);

        // Angle and power
        char angle_text[50];
        sprintf(angle_text, "Angle: %d°", game->players[i].angle);
        cairo_move_to(cr, text_x, 110); // Adjusted spacing
        cairo_show_text(cr, angle_text);

        char power_text[50];
        sprintf(power_text, "Power: %d/%d", game->players[i].power, MAX_POWER);
        cairo_move_to(cr, text_x, 140); // Adjusted spacing
        cairo_show_text(cr, power_text);

        // Moves left
        char moves_text[50];
        sprintf(moves_text, "Moves left: %d", game->players[i].moves_left);
        cairo_move_to(cr, text_x, 170); // Adjusted spacing
        cairo_show_text(cr, moves_text);
    }

    // Game state messages
    if (game->state == STATE_GAME_OVER)
    {
        // Determine winner
        int winner = (game->players[0].health <= 0) ? 1 : 0;

        char winner_text[100];
        sprintf(winner_text, "%s wins! Press R to play again.", game->players[winner].name);

        cairo_set_source_rgb(cr, 0.8, 0.0, 0.0);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 24);

        // Center text
        cairo_text_extents_t extents;
        cairo_text_extents(cr, winner_text, &extents);
        cairo_move_to(cr, (WINDOW_WIDTH - extents.width) / 2, WINDOW_HEIGHT / 2);
        cairo_show_text(cr, winner_text);
    }
    else if (game->game_paused)
    {
        // Paused message
        char paused_text[] = "GAME PAUSED - Press P to continue";

        cairo_set_source_rgb(cr, 0.0, 0.0, 0.8);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 24);

        // Center text
        cairo_text_extents_t extents;
        cairo_text_extents(cr, paused_text, &extents);
        cairo_move_to(cr, (WINDOW_WIDTH - extents.width) / 2, WINDOW_HEIGHT / 2);
        cairo_show_text(cr, paused_text);
    }

    // Draw controls help
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12);

    char controls_text[] = "Controls: Arrows (aim/power), W/S (weapon), A/D (move), Space (fire), R (reset), P (pause)";
    cairo_move_to(cr, 10, WINDOW_HEIGHT - 10);
    cairo_show_text(cr, controls_text);
}

// GTK tick callback
static gboolean tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data)
{
    update_game(&game);
    gtk_widget_queue_draw(widget);
    return G_SOURCE_CONTINUE;
}