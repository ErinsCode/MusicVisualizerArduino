#include <FastLED.h> // FastLED library: https://github.com/FastLED/FastLED
#include <arduinoFFT.h>

//https://medium.com/@yolandaluqueh/music-to-led-strip-tutorial-using-fourier-transform-3d203a48fe14

#define SAMPLES 64               //determines the resolution of the frequency reads.  Lower value due to Arduino storage and speed restraints.  Value must be power of 2
#define SAMPLING_FREQUENCY 40000 //Nyquim Theory states that this must be 2x as large as the frequency we want to sample.  Human hear from 20khz - 20,000khz

#define ANIMATION_DELAY 0 //adjusts speed the leds will transition

arduinoFFT FFT = arduinoFFT();
unsigned int sampling_period_us; //determines how long our sample time is
unsigned long microseconds;      // current time when arduino started

double vReal[SAMPLES]; //real frequency the arduino picks up
double vImag[SAMPLES]; //used to computer fft
int bandValues[8];     //stores computed frequency values
int oldBandValues[8];  //create animation for transition

// Define pins used
#define LED_PIN 2
#define SOUND_PIN A0

// Define led layout
#define NUM_LEDS 128
#define COLS 8
#define ROWS 16

// Other constants
#define ANIMATION_DELAY 1

// FastLED array
CRGB leds[NUM_LEDS];

// Turn FastLED 1d array into alternating 2d array
int grid[ROWS][COLS] = {
    {127, 96, 95, 64, 63, 32, 31, 0},
    {126, 97, 94, 65, 62, 33, 30, 1},
    {125, 98, 93, 66, 61, 34, 29, 2},
    {124, 99, 92, 67, 60, 35, 28, 3},
    {123, 100, 91, 68, 59, 36, 27, 4},
    {122, 101, 90, 69, 58, 37, 26, 5},
    {121, 102, 89, 70, 57, 38, 25, 6},
    {120, 103, 88, 71, 56, 39, 24, 7},
    {119, 104, 87, 72, 55, 40, 23, 8},
    {118, 105, 86, 73, 54, 41, 22, 9},
    {117, 106, 85, 74, 53, 42, 21, 10},
    {116, 107, 84, 75, 52, 43, 20, 11},
    {115, 108, 83, 76, 51, 44, 19, 12},
    {114, 109, 82, 77, 50, 45, 18, 13},
    {113, 110, 81, 78, 49, 46, 17, 14},
    {112, 111, 80, 79, 48, 47, 16, 15}};

//LED colors for each column
CRGB colors[COLS] = {
    CRGB::Red,
    CRGB::OrangeRed,
    CRGB::Yellow,
    CRGB::Green,
    CRGB::Blue,
    CRGB::Cyan,
    CRGB::Purple,
    CRGB::FairyLight,
};

void setup()
{
    sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));  //delay between each fft sample

    // Set pins to correct mode
    pinMode(LED_PIN, OUTPUT);
    pinMode(SOUND_PIN, INPUT);

    // FastLED setup
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(30);

    for (int i = 0; i < 8; i++)
    {
        oldBandValues[i] = 0;
    }

    for (int i = 0; i < 128; i++)
    {
        leds[i] = CRGB::Black;
    }
}

void loop()
{
    //reset frequency values
    for (int i = 0; i < 8; i++)
    {
        bandValues[i] = 0;
    }

    /*SAMPLING*/
    //https://medium.com/@yolandaluqueh/music-to-led-strip-tutorial-using-fourier-transform-3d203a48fe14

    //clear out data from old samples and set vReal to hold current input from microphone
    for (int i = 0; i < SAMPLES; i++)
    {
        // microseconds = micros();    //Overflows after around 70 minutes
        vReal[i] = 0;
        vReal[i] = analogRead(A0);
        vImag[i] = 0;

        while (micros() < (microseconds + sampling_period_us))
        { // Delay sampling period
        }
    }

    //computefft
    FFT.DCRemoval();                                                 //remove dc offset.  Make it so that waveform is centered areound 0  https://manual.audacityteam.org/man/dc_offset.html
    FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD); //get rid of spectral leakage https://www.youtube.com/watch?v=Q8N8pZ8P3f4
    FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);                 //calculate fft
    FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);

    /*https://github.com/s-marley/ESP32_FFT_VU/blob/master/ESP32_FFT_VU/ESP32_FFT_VU.ino
    each element in vReal is a different frequency bit and the value of this element is that frequency's amplitude.
    since we are sampling at 64 we have 32 frequency bins that are available to us, but the first two bins vReal[0] & vReal[1] is mostly DC voltage hum so we have 30 bins.
    We must split up our 30 bins into 8 different bands since our LED strip has 8 columns.  Taking 20000/64 we see that the frequency width of each bin is 625, so each
    element of vReal has 625 khz in it.  It is suggested that each bandValue should be determined by a logrithmic function of the amount of frequency bands, but due to the 
    low sample rate we were unable to do this perfectly.*/
    for (int i = 2; i < SAMPLES / 2; i++)
    {

        if (vReal[i] > 500)
        {
            if (i > 1 && i <= 3)
            {
                bandValues[0] += (int)vReal[i];
            }
            if (i > 3 && i <= 5)
            {
                bandValues[2] += (int)vReal[i];
            }
            if (i > 5 && i <= 7)
            {
                bandValues[3] += (int)vReal[i];
            }
            if (i > 7 && i <= 10)
            {
                bandValues[1] += (int)vReal[i];
            }
            if (i > 10 && i <= 14)
            {
                bandValues[4] += (int)vReal[i];
            }
            if (i > 14 && i <= 23)
            {
                bandValues[5] += (int)vReal[i];
            }
            if (i > 23 && i <= 26)
            {
                bandValues[6] += (int)vReal[i];
            }
            if (i > 26)
            {
                bandValues[7] += (int)vReal[i];
            }
        }
    }

    //make contents of bandValues fit within our led range
    for (int i = 0; i < 8; i++)
    {
        bandValues[i] = bandValues[i] / 1000;

        if (bandValues[i] > 15)
        {
            bandValues[i] = 15;
        }
        //    Serial.println(bandValues[i]);
    }

    // constrain amplitude within the boundaries of the led array
    for (int i = 0; i < 8; i++)
    {
        if (bandValues[i] < 0)
        {
            bandValues[i] = 0;
        }
        else if (bandValues[i] > 15)
        {
            bandValues[i] = 15;
        }
    }

    //set led colors make it so that there is a smooth transition from previous colors to new ones by slowing going down rows of each column
    for (int r = ROWS - 1; r <= 0; r--)
    {
        for (int c = 0; c < COLS; c++)
        {
            if (bandValues[c] < oldBandValues[c] && oldBandValues[c] == r)
            {
                leds[grid[r][c]] = CRGB::Black;
                oldBandValues[c]--;

                FastLED.show();
                delay(ANIMATION_DELAY);
            }
        }
    }

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            if (r < bandValues[c])
            {
                leds[grid[r][c]] = colors[c];
            }
            else if (r > bandValues[c])
            {
                leds[grid[r][c]] = CRGB::Black;
            }
        }
        FastLED.show();
        delay(ANIMATION_DELAY);
    }

    //set old band values for next loop
    for (int i = 0; i < 8; i++)
    {
        oldBandValues[i] = bandValues[i];
    }
}
