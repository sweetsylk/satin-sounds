#include <iostream>
#include <portaudio.h>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>
#include <stdio.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512
static void checkError(PaError err)
{
    if (err != paNoError)
    {
        std::cout << "PortAudio has an error man:\n"
                  << Pa_GetErrorText(err);
        exit(EXIT_FAILURE);
    }
}

static inline float max(float a, float b) // inline is for compiler optimisation (just calls function on the line its called on)
{
    return a > b ? a : b;
}
/*
essentially this is called by PortAudio to take in audio data and print the meter
it does some calculations using the left and right channels to make a meter for magnitude
*/
static int paTempCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    float *input = (float *)inputBuffer; // input is initially a pointer to a void datatype (unknown)
    (void)outputBuffer;                  // imma use this later
    int displaySize = 100;               // 100 characters for now
    printf("\r");

    float volume_right = 0;
    float volume_left = 0;

    for (unsigned long i = 0; i < framesPerBuffer * 2; i += 2)
    {
        // we need maximum amplitude (negative and positve both make sound)
        volume_left = max(volume_left, std::abs(input[i]));       // left deals with even indexes
        volume_right = max(volume_right, std::abs(input[i + 1])); // right deals with odd indexes
    }

    /*
    checks to actually draw a nice bar!
    if both are loud then we have a tall bar
    if left is loud the upper bar and if right then lower
    silence is empty
    */
    for (int i = 0; i < displaySize; i++)
    {
        float barProportion = ((i) / (float)displaySize) / 20; // made bars slightly taller since my mic sensitivity is low
        if (barProportion <= volume_left && barProportion <= volume_right)
        {
            printf("█");
        }
        else if (barProportion <= volume_left)
        {
            printf("▀");
        }
        else if (barProportion <= volume_right)
        {
            printf("▄");
        }
        else
        {
            printf(" ");
        }
    }

    fflush(stdout); // clearing input buffer for more inputs

    return 0;
}

int main()
{
    PaError err = Pa_Initialize();
    checkError(err);

    // checking number of device (i may remove this soon)
    int numDevices = Pa_GetDeviceCount();
    if (numDevices < 0)
    {
        std::cout << "whoops cant get your devices\n";
        exit(EXIT_FAILURE);
    }
    else if (numDevices == 0)
    {
        std::cout << "nothing there man\n";
        exit(EXIT_SUCCESS);
    }
    else
    {
        std::cout << "Number of devices: " << numDevices << '\n';
        int defaultOutput = Pa_GetDefaultOutputDevice();
        const PaDeviceInfo *output = Pa_GetDeviceInfo(defaultOutput);
        std::cout << "Currently default output: " << output->name << std::endl;
        int defaultInput = Pa_GetDefaultInputDevice();
        const PaDeviceInfo *input = Pa_GetDeviceInfo(defaultInput);
        std::cout << "Currently default input: " << input->name << std::endl;
    }

    /*
    this i can later use to check my devices (i got 67 audio devices idk why)
    const PaDeviceInfo *deviceInfo;
    for (int i = 0; i < numDevices; i++)
    {

        deviceInfo = Pa_GetDeviceInfo(i);
        std::cout << "Device " << i << "\n Name: " << deviceInfo->name << "\n MaxInputChannel " << deviceInfo->maxInputChannels << "\n MaxOutputChannels " << deviceInfo->maxOutputChannels << "\n defaultSampleRate " << deviceInfo->defaultSampleRate;
    }

    */

    // for me: device 1 (input) device 7 (output)

    int inputDevice = 1;
    int outputDevice = 7;

    // intialising values into the PaStreamParameter Struct (vital for avoiding crashes when passing to Pa_OpenStream)
    PaStreamParameters inputParameters;
    PaStreamParameters outputParameters;

    memset(&inputParameters, 0, sizeof(inputParameters));
    inputParameters.channelCount = 2;
    inputParameters.device = inputDevice;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputDevice)->defaultLowInputLatency;

    memset(&outputParameters, 0, sizeof(inputParameters));
    outputParameters.channelCount = 2;
    outputParameters.device = outputDevice;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputDevice)->defaultLowInputLatency;

    // opening an audio stream to take in audio
    PaStream *stream;

    err = Pa_OpenStream(
        &stream,
        &inputParameters,
        &outputParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paNoFlag,
        paTempCallback,
        NULL);
    checkError(err);

    // start taking audio
    err = Pa_StartStream(stream);
    checkError(err);

    Pa_Sleep(10 * 1000); // this is 10 seconds of audio input

    err = Pa_StopStream(stream);
    checkError(err);

    // closing stream and terminating port audio (gotta save memory man)
    err = Pa_CloseStream(stream);
    checkError(err);
    err = Pa_Terminate();
    checkError(err);

    std::cout << "\nSeems to have worked i guess!\n";
    Pa_Terminate();
    return 0;
}
