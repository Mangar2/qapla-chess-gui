#pragma once

/**
 * Initializes background image, quad, and shaders from memory array.
 * Must be called after OpenGL context is active.
 * 
 * @param imageData Pointer to image data in memory (JPEG/PNG format).
 * @param dataSize Size of image data in bytes.
 */
void initBackgroundImageFromMemory(const void* imageData, unsigned int dataSize);

/**
 * Initializes background image, quad, and shaders from file.
 * Must be called after OpenGL context is active.
 * 
 * @param imagePath Path to background image (PNG or JPG).
 */
void initBackgroundImage(const char* imagePath);

/**
 * Draws the fullscreen background image.
 * Call once per frame before ImGui::NewFrame().
 */
void drawBackgroundImage();

/**
 * Indicates if the background image was successfully loaded.
 */
extern bool backgroundImageLoaded;
