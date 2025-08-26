#include <iostream>
#include <portaudio.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <sndfile.h>
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512

struct AudioData
{
    std::vector<float> samples;
    size_t position = 0;
    float volumeLeft = 0.0f;
    float volumeRight = 0.0f;
};

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
This draws the ascii bar for the volume of whatever audio is being taken in
*/

static void AsciiBar(float *input, AudioData *audio = nullptr, int displaySize = 100)
{
    float volumeLeft = 0.0f;
    float volumeRight = 0.0f;

    if (input) // live mic input
    {
        for (unsigned long i = 0; i < FRAMES_PER_BUFFER * 2; i += 2)
        {
            volumeLeft = std::max(volumeLeft, std::abs(input[i]));
            volumeRight = std::max(volumeRight, std::abs(input[i + 1]));
        }
    }
    else if (audio) // file playback
    {

        volumeLeft = audio->volumeLeft;
        volumeRight = audio->volumeRight;
    }

    printf("\r");
    for (int i = 0; i < displaySize; i++)
    {
        float barProportion = (float)i / displaySize;
        if (barProportion <= volumeLeft && barProportion <= volumeRight)
            printf("█");
        else if (barProportion <= volumeLeft)
            printf("▀");
        else if (barProportion <= volumeRight)
            printf("▄");
        else
            printf(" ");
    }
    fflush(stdout);
}

static int paTempCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    float *input = (float *)inputBuffer; // input is initially a pointer to a void datatype (unknown)
    (void)outputBuffer;                  // no sound other mic input needed
    AsciiBar(input, nullptr);
    return 0;
}

/*
outputs audio from the file and computes the peaks ready to be printed to ascii*/
static int fileCallBack(const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData)
{
    AudioData *data = (AudioData *)userData;
    float *output = (float *)outputBuffer;
    (void)inputBuffer; // im just outputting the file sound

    size_t framesLeft = data->samples.size() - data->position;
    size_t framesToWrite = std::min((size_t)framesPerBuffer * 2, framesLeft);

    float bufferMaxLeft = 0.0f;
    float bufferMaxRight = 0.0f;

    for (size_t i = 0; i < framesToWrite; i += 2)
    {
        float left = data->samples[data->position++];
        float right = data->samples[data->position++];

        *output++ = left;
        *output++ = right;

        bufferMaxLeft = std::max(bufferMaxLeft, std::abs(left));
        bufferMaxRight = std::max(bufferMaxRight, std::abs(right));
    }

    // Fill the rest of the buffer with silence gotta pad it or something
    for (size_t i = framesToWrite; i < framesPerBuffer * 2; i++)
    {
        *output++ = 0.0f;
    }

    // Update shared volume for ASCII display
    data->volumeLeft = bufferMaxLeft;
    data->volumeRight = bufferMaxRight;

    return (data->position >= data->samples.size()) ? paComplete : paContinue;
}

int main()
{

    char userinput;
    std::cout << "Do you want to take a file or mic? (f/m)\n"
              << std::flush;

    std::cin >> userinput;
    switch (userinput)
    {

    case 'f':
    {

        SF_INFO sfinfo;
        SNDFILE *sndfile = sf_open("C:/Users/44756/satin-sounds/test.wav", SFM_READ, &sfinfo);
        if (!sndfile)
        {
            std::cerr << "nah man no file\n";
            return 1;
        }

        std::vector<float> buffer(sfinfo.frames * sfinfo.channels);
        sf_readf_float(sndfile, buffer.data(), sfinfo.frames);
        sf_close(sndfile);

        AudioData audio{buffer, 0};

        Pa_Initialize();
        PaStream *stream;
        Pa_OpenDefaultStream(&stream, 0, sfinfo.channels, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, fileCallBack, &audio);
        Pa_StartStream(stream);

        while (Pa_IsStreamActive(stream))
        {
            Pa_Sleep(50);
            AsciiBar(nullptr, &audio);
            fflush(stdout);
        }

        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        break;
    }

    case 'm':

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
        break;
    }
    default:
    {
        std::cout << "\nYou idiot\n";
        main();
    }
    }
    Pa_Terminate();
    return 0;
}
