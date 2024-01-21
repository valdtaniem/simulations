#include <SDL.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>  // Include for rand()

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SPHERE_RADIUS = 20;
const double GRAVITY = 0.2;
const double BOUNCE_FACTOR = 0.8;

class Vector2 {
public:
    double x, y;

    Vector2() : x(0.0), y(0.0) {}
    Vector2(double _x, double _y) : x(_x), y(_y) {}

    Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }

    Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }

    Vector2 operator*(double scalar) const {
        return Vector2(x * scalar, y * scalar);
    }

    Vector2 operator*(const Vector2& other) const {
        return Vector2(x * other.x, y * other.y);
    }
};


struct Sphere {
    Vector2 position;
    Vector2 velocity;
    Uint32 color;
    double bounceFactor; // Added member for bounce factor
};

void setPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
    if (x >= 0 && x < surface->w && y >= 0 && y < surface->h) {
        Uint32* pixels = static_cast<Uint32*>(surface->pixels) + y * surface->pitch / 4 + x;
        *pixels = color;
    }
}

void drawFilledCircle(SDL_Surface* surface, const Vector2& center, int radius, Uint32 color) {
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                setPixel(surface, center.x + static_cast<int>(x), center.y + static_cast<int>(y), color);
            }
        }
    }
}

void handleSphereCollision(Sphere& sphere1, Sphere& sphere2) {
    // Calculate the vector between the centers of the spheres
    Vector2 relativePosition = sphere2.position - sphere1.position;
    double distance = std::sqrt(relativePosition.x * relativePosition.x + relativePosition.y * relativePosition.y);
    double totalRadius = 2 * SPHERE_RADIUS;

    // Check if spheres overlap
    if (distance < totalRadius) {
        // Calculate the penetration depth
        double penetration = totalRadius - distance;

        // Normalize the relative position vector
        Vector2 normal = relativePosition * (1.0 / distance);

        // Calculate the relative velocity
        Vector2 relativeVelocity = sphere2.velocity - sphere1.velocity;
        double relativeSpeed = normal.x * relativeVelocity.x + normal.y * relativeVelocity.y;

        // If spheres are moving toward each other, perform collision response
        if (relativeSpeed < 0) {
            double impulse = (2.0 * relativeSpeed) / (1 / sphere1.bounceFactor + 1 / sphere2.bounceFactor);

            // Update sphere velocities with a smaller impulse
            sphere1.velocity.x += impulse * (normal.x * (1.0 / sphere1.bounceFactor));
            sphere1.velocity.y += impulse * (normal.y * (1.0 / sphere1.bounceFactor));
            sphere2.velocity.y -= impulse * (normal.y * (1.0 / sphere2.bounceFactor));
            sphere2.velocity.x -= impulse * (normal.x * (1.0 / sphere2.bounceFactor));

            // Move spheres to avoid overlap
            double moveDistance = penetration * 0.5;
            sphere1.position.x -= moveDistance * normal.x;
            sphere1.position.y -= moveDistance * normal.y;
            sphere2.position.x += moveDistance * normal.x;
            sphere2.position.y += moveDistance * normal.y;
        }
    }
}


int main(int argc, char* args[]) {
    SDL_Window* window = nullptr;
    SDL_Surface* surface = nullptr;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    window = SDL_CreateWindow("SDL Sphere", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    surface = SDL_GetWindowSurface(window);
    if (surface == nullptr) {
        std::cerr << "Surface could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::vector<Sphere> spheres;

    SDL_Event e;
    bool quit = false;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                // Add a new sphere when the left mouse button is clicked
                Sphere newSphere;
                newSphere.position = Vector2(e.button.x, e.button.y);
                newSphere.velocity = Vector2(0.0, 0.0);
                newSphere.color = SDL_MapRGB(surface->format, rand() % 256, rand() % 256, rand() % 256);
                newSphere.bounceFactor = BOUNCE_FACTOR;
                spheres.push_back(newSphere);
            }
        }

        // Update all spheres
        for (auto& sphere : spheres) {
            // Apply gravity to the velocity
            sphere.velocity.y += GRAVITY;

            // Update the sphere's position based on its velocity
            sphere.position = sphere.position + sphere.velocity;

            // Check for collision with the bottom of the window
            if (sphere.position.y + SPHERE_RADIUS >= SCREEN_HEIGHT) {
                // Adjust the position to be just above the bottom
                sphere.position.y = SCREEN_HEIGHT - SPHERE_RADIUS;

                // Reverse the vertical velocity with a factor (bounce)
                sphere.velocity.y = -sphere.velocity.y * BOUNCE_FACTOR;
            }

            // Check for collision with the sides of the window
            if (sphere.position.x - SPHERE_RADIUS <= 0) {
                // Adjust the position to be just inside the left side
                sphere.position.x = SPHERE_RADIUS;

                // Reverse the horizontal velocity with a factor (bounce)
                sphere.velocity.x = -sphere.velocity.x * BOUNCE_FACTOR;
            }
            else if (sphere.position.x + SPHERE_RADIUS >= SCREEN_WIDTH) {
                // Adjust the position to be just inside the right side
                sphere.position.x = SCREEN_WIDTH - SPHERE_RADIUS;

                // Reverse the horizontal velocity with a factor (bounce)
                sphere.velocity.x = -sphere.velocity.x * BOUNCE_FACTOR;
            }

            // Check for collisions with other spheres
            for (auto& otherSphere : spheres) {
                if (&sphere != &otherSphere) {
                    handleSphereCollision(sphere, otherSphere);
                }
            }
        }

        // Clear the surface
        SDL_FillRect(surface, nullptr, SDL_MapRGB(surface->format, 255, 255, 255));

        // Draw all spheres
        for (const auto& sphere : spheres) {
            drawFilledCircle(surface, sphere.position, SPHERE_RADIUS, sphere.color);
        }

        // Update the window surface
        SDL_UpdateWindowSurface(window);

        // Delay to control the frame rate
        SDL_Delay(16);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
