/*******************************************************************************************
*
*   Audio Processing experiment - JK
*
********************************************************************************************/

#include "raylib.h"
#include <stdio.h>
#include <assert.h>

#include <vector>

#include "FFT.h"
#include "mcmpq.h"

void SetMuted(bool bIsMuted) 
{
    SetMasterVolume(bIsMuted ? 0.0f : 1.0f);
}

//------------------------------------------------------------------------------------
// Module Functions Declaration
//------------------------------------------------------------------------------------
static void AudioProcessor(void *buffer, unsigned int frames);

///////////////////////////////////////////////////
// Audio Queues
// Each used to communicate in a single direction
///////////////////////////////////////////////////

queue_t QueueMainToAudio;
queue_t QueueAudioToMain;

const int N = 512;          // number of samples to process in each FFT
float AudioBuffer[N];       // buffer used by audio thread to send snapshot of time domain signal to main thread
float MainAudioBuffer[N];   // buffer used by main thread to capture latest snapshot of time domain signal

//------------------------------------------------------------------------------------
// Display modes
//------------------------------------------------------------------------------------

enum class EDisplayMode : int {
    TimeDomain,
    FrequencyDomain,
    Octaves
};

EDisplayMode DisplayMode = EDisplayMode::TimeDomain;

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1200;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "raylib - JK - Audio Processing experiment with FFT");

    // Initialize audio queues
    enqueue(&QueueMainToAudio, AudioBuffer);
    
    // Initialize audio device
    InitAudioDevice();
    
    Music music = LoadMusicStream("resources/02 - Eruption.mp3");
    
    AttachAudioStreamProcessor(music.stream, AudioProcessor);

    PlayMusicStream(music);

    bool bIsMuted = false;
    SetMuted(bIsMuted);

    // Define the camera to look into our 3d world
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 5.0f, 10.0f }; // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    Vector3 cubePosition = { 0.0f, 0.0f, 0.0f };

    // NOTE: hide the cursor and limit it to the game window
    DisableCursor();                    // Limit cursor to relative movement inside the window
    bool bIsCursorEnabled = false;

    SetTargetFPS(1000);                   // Set our game to run at 60 frames-per-second

    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera, CAMERA_FREE);

        UpdateMusicStream(music);   // Update music buffer with new stream data

        // receive time domain signal from audio thread
        if (try_dequeue(&QueueAudioToMain, AudioBuffer)) 
        {
            // take local copy of the audio buffer
            memcpy(MainAudioBuffer, AudioBuffer, sizeof(MainAudioBuffer));

            // send audio buffer back to the audio thread
            enqueue(&QueueMainToAudio, AudioBuffer);
        }

        if (IsKeyPressed('Z')) camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };

        if (IsKeyPressed('F')) 
        {
            switch (DisplayMode)
            {
                case EDisplayMode::TimeDomain:
                    DisplayMode = EDisplayMode::FrequencyDomain;
                    break;
                case EDisplayMode::FrequencyDomain:
                    DisplayMode = EDisplayMode::Octaves;
                    break;
                case EDisplayMode::Octaves:
                default:
                    DisplayMode = EDisplayMode::TimeDomain;
                    break;
            };
        }    

        if (IsKeyPressed('C')) 
        {
            bIsCursorEnabled = !bIsCursorEnabled;
            if (bIsCursorEnabled) 
            {
                EnableCursor();
            } else {
                DisableCursor();
            }
        }

        if (IsKeyPressed('R'))
        {
            SeekMusicStream(music, 0.0f);
        }

        if (IsKeyPressed('M'))
        {
            bIsMuted = !bIsMuted;

            SetMuted(bIsMuted);
        }

        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------

        // NOTE: drawing starts, and will finish in call to EndDrawing()
        BeginDrawing();

            // NOTE: clear the render buffer
            // ClearBackground(Colour)
            ClearBackground(RAYWHITE);

            // NOTE: Start 3D rendering operations for this frame (until matching call to EndMode3D())
            // BeginMode3D(Camera3D)
            BeginMode3D(camera);

                // 3DFreeCamera code
                /*
                // NOTE: axes X = left/right, Y = up/down, Z = in/out of screen
                // DrawCube(Location, X, Y, Z, FillColour)
                DrawCube(cubePosition, 2.0f, 2.0f, 2.0f, RED);

                // DrawCubeWires(Location, X, Y, Z, StrokeColour)
                DrawCubeWires(cubePosition, 2.0f, 2.0f, 2.0f, MAROON);
                */

                switch (DisplayMode)
                {
                    case EDisplayMode::TimeDomain:
                    {
                        for (int i=0; i<N; i++) 
                        {
                            const float value = MainAudioBuffer[i];

                            const static float CubeWidth = 10.0f / float(N);
                            const static float CubeHeight = 10.0f;
                            const Vector3 cubePosition = { (float(i) * CubeWidth) - (N * 0.5f * CubeWidth), 0.0f, 0.0f };
                            const Color Colour = ColorFromHSV(256.0f * float(i) / float(N), 1.0f, 1.0f);
                            DrawCube(cubePosition, CubeWidth, CubeHeight * value, CubeWidth, Colour);
                        }
                    }
                    break;
                    case EDisplayMode::FrequencyDomain:
                    {
                        // render frequency domain
                        std::vector<ComplexNumber> frequencies = fft(MainAudioBuffer, N);

                        // render frequencies up to nyquist
                        for (int i=0; i<N/2; i++) 
                        {
                            const float valueLinear = frequencies[i].magnitude();;

                            // convert to decibels, relative to 0dB at full volume
                            const float valueDecibels = 20.0f * log10f(valueLinear);

                            constexpr float DecibelsRange = 6.0f;
                            const float value = std::max(valueDecibels + DecibelsRange, 0.0f) / DecibelsRange;

                            const static float CubeWidth = 10.0f / float(N/2);
                            const float CubeHeight = value * 0.5f;
                            const static float CubeDepth = 0.1f;
                            const Vector3 cubePosition = { (float(i) * CubeWidth) - (N/2 * 0.5f * CubeWidth), CubeHeight * 0.5f, 0.0f };
                            const Color Colour = ColorFromHSV(256.0f * float(i) / float(N/2), 1.0f, 1.0f);
                            DrawCube(cubePosition, CubeWidth, CubeHeight, CubeDepth, Colour);
                        }
                    }
                    break;
                    case EDisplayMode::Octaves:
                    default:
                    {
                        // render frequency domain
                        std::vector<ComplexNumber> frequencies = fft(MainAudioBuffer, N);

                        const int NumOctaves = 8;
                        std::vector<float> Octaves(NumOctaves, 0.0f);
                        int Next = 0;
                        for (int Octave=0; Octave<NumOctaves; Octave++) 
                        {
                            const int First = Next;
                            const int NumFrequencies = pow(2, Octave);
                            const int End = First + NumFrequencies;

                            float Value = 0.0f;
                            
                            for (int i=First; i<End; i++) 
                            {
                                Value += frequencies[i].magnitude();
                            }
                            Value /= float(NumFrequencies);

                            Octaves[Octave] = Value;
                        }

                        // render octaves
                        for (int i=0; i<NumOctaves; i++) 
                        {
                            const float valueLinear = Octaves[i];

                            // convert to decibels, relative to 0dB at full volume
                            const float valueDecibels = 20.0f * log10f(valueLinear);

                            constexpr float DecibelsRange = 6.0f;
                            const float value = std::max(valueDecibels + DecibelsRange, 0.0f) / DecibelsRange;

                            const static float CubeWidth = 10.0f / float(NumOctaves);
                            const float CubeHeight = value * 0.5f;
                            const static float CubeDepth = 0.1f;
                            const Vector3 cubePosition = { (float(i) * CubeWidth) - (NumOctaves * 0.5f * CubeWidth), CubeHeight * 0.5f, 0.0f };
                            const Color Colour = ColorFromHSV(256.0f * float(i) / float(NumOctaves), 1.0f, 1.0f);
                            DrawCube(cubePosition, CubeWidth, CubeHeight, CubeDepth, Colour);
                        }
                    }
                    break;
                }

                // DrawGrid(NumRows/Cols, Spacing)
                DrawGrid(10, 1.0f);

            // NOTE: Finish 3D rendering operations, which started in call to BeginMode3D()
            EndMode3D();

            DrawFPS(10, 10);

            /*
            // NOTE: 2D rendering
            // DrawRectangle(X, Y, Width, Height, FillColour)
            DrawRectangle( 10, 10, 320, 93, Fade(SKYBLUE, 0.5f));

            // DrawRectangleLines(X, Y, Width, Height, StrokeColour)
            DrawRectangleLines( 10, 10, 320, 93, BLUE);

            // DrawText(String, X, Y, FontSize, Colour)
            DrawText("Free camera default controls:", 20, 20, 10, BLACK);
            DrawText("- Mouse Wheel to Zoom in-out", 40, 40, 10, DARKGRAY);
            DrawText("- Mouse Wheel Pressed to Pan", 40, 60, 10, DARKGRAY);
            DrawText("- Z to zoom to (0, 0, 0)", 40, 80, 10, DARKGRAY);
            */

        // NOTE: finish drawing to back buffer, that started in BeginDrawing()
        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    
    UnloadMusicStream(music);   // Unload music stream buffers from RAM

    CloseAudioDevice();         // Close audio device (music streaming is automatically stopped)
    
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definition
//------------------------------------------------------------------------------------

int AudioBufferProgress = 0;

static void AudioProcessor(void *buffer, unsigned int frames)
{
    if (AudioBufferProgress == 0) 
    {
        // we need to capture an audio buffer
        void* Ptr = nullptr;
        if (!try_dequeue(&QueueMainToAudio, AudioBuffer)) 
        {
            return;   
        }
    }

    float *bufferData = (float *)buffer;
    for (unsigned int i = 0; (i < frames*2) && (AudioBufferProgress < N); i += 2, AudioBufferProgress++)
    {
        const float l = bufferData[i];
        const float r = bufferData[i + 1];

        // mix down to mono
        const float sample = (l + r) * 0.5f;

        // write to Audio Buffer
        AudioBuffer[AudioBufferProgress] = sample;
    }

    assert(AudioBufferProgress > 0);

    if (AudioBufferProgress == N)
    {
        AudioBufferProgress = 0;
        
        enqueue(&QueueAudioToMain, AudioBuffer);
    }
}