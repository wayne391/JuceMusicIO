/*
  ==============================================================================
    This file contains the basic startup code for a JUCE application.
  ==============================================================================
*/

#include <JuceHeader.h>
#include "MusicIO.hpp"
#include "yaml-cpp/yaml.h"

using namespace juce;


/*
 The channels of audioFxInstance should be strictly larger than audio samples.
 */

template<class T>
void renderAudio(
        AudioBuffer<float>& inBuffer,
        AudioBuffer<float>& outBuffer,
        int bufferSize,
        int tailSeconds,
        int sampleRate,
        std::unique_ptr<T> &audioFxInstance)
{
    int sampleLength = inBuffer.getNumSamples();
    int numberOfBuffers = int (
        std::ceil ((sampleLength + sampleRate * tailSeconds) / bufferSize));

    int numberOfSamples = numberOfBuffers * bufferSize;
    int numRenderChannels = audioFxInstance->getTotalNumOutputChannels();
    int numAudioChannels = inBuffer.getNumChannels();
    
    std::cout << " [render audio]    plugin channels: " << numRenderChannels << std::endl;
    std::cout << " [render audio]     audio channels: " << numAudioChannels << std::endl;
    std::cout << " [render audio]  number of buffers: " << numberOfBuffers << std::endl;
    std::cout << " [render audio]  number of samples: " << numberOfSamples << std::endl;
   
    // initialize padded input buffer
    AudioBuffer<float> inputBuffer(numAudioChannels, numberOfSamples);
    for (int c = 0; c < numAudioChannels; ++c)
    {
        inputBuffer.copyFrom(c, 0, inBuffer, c, 0, inBuffer.getNumSamples());
    }

    // initialize output buffer
    outBuffer.setSize(inBuffer.getNumChannels(), numberOfSamples);
    
    // run
    MidiBuffer midi;
    for (int b = 0; b < numberOfBuffers; ++b) {

        // copy into processing buffer
        AudioBuffer<float> procBuffer(numRenderChannels, bufferSize);
        for (int c = 0; c < numAudioChannels; ++c)
        {
            procBuffer.copyFrom(c, 0, inputBuffer, c, b * bufferSize, bufferSize);
        }

        // process
        audioFxInstance->processBlock (procBuffer, midi);


        // copy out
        for (int c = 0; c < numAudioChannels; ++c)
        {
            outBuffer.copyFrom(c, b * bufferSize, procBuffer, c, 0, bufferSize);
        }
    }
}


template<class T>
void renderMidi(
        MidiBuffer& midiBuffer,
        AudioBuffer<float>& outBuffer,
        int bufferSize,
        int tailSeconds,
        int sampleRate,
        std::unique_ptr<T> &instrument)
{
    
    int numberOfBuffers = int (
            std::ceil ((midiBuffer.getLastEventTime() + sampleRate * tailSeconds) / bufferSize));
    
    
    int numberOfSamples = numberOfBuffers * bufferSize;
    std::cout << " [render midi] number of buffers :" << numberOfBuffers << std::endl;
    std::cout << " [render midi] number of samples :" << numberOfSamples << std::endl;
    outBuffer.setSize(2, numberOfSamples);
    
    // initialize render info
    MidiBuffer renderMidiBuffer;
    MidiBuffer::Iterator it(midiBuffer);
    MidiMessage m;
    int sampleNumber = -1;
    bool isMessageBetween;
    bool bufferRemaining = it.getNextEvent(m, sampleNumber);
    AudioBuffer<float> audioBuffer(instrument->getTotalNumOutputChannels(), bufferSize);
    
    // run
    for (int i = 0; i < numberOfBuffers; ++i)
    {

        double start = i * bufferSize;
        double end = (i + 1) * bufferSize;

        isMessageBetween = sampleNumber >= start && sampleNumber < end;
        do {
            if (isMessageBetween) {
                renderMidiBuffer.addEvent(m, sampleNumber - start);
                bufferRemaining = it.getNextEvent(m, sampleNumber);
                isMessageBetween = sampleNumber >= start && sampleNumber < end;
            }
        } while (isMessageBetween && bufferRemaining);

        // Turn Midi to audio via the vst.
        instrument->processBlock (audioBuffer, renderMidiBuffer);

        // copy out
        for (int c = 0; c < 2; ++c)
        {
            outBuffer.copyFrom(c, i * bufferSize, audioBuffer, c, 0, bufferSize);
        }

    }
}



int main (int argc, char* argv[])
{
    ScopedJuceInitialiser_GUI initialiser; // required for JUCE console app

    String pathToAudioInFile("/Users/wayne391/Documents/Projects/MyPluginHost/console/Source/test.wav");
    String pathToAudioOutFile("/Users/wayne391/Documents/Projects/MyPluginHost/console/Source/test_vsti.wav");
//    String pathToMidiInFile("/Users/wayne391/Documents/Projects/MyPluginHost/console/Source/test.mid");
//    String pathToInstrument("/Library/Audio/Plug-Ins/VST/Kontakt.vst");
    String pathToPlugin("/Library/Audio/Plug-Ins/Components/ValhallaShimmer.component");
//    String pathToGraph("/Users/wayne391/Documents/tal-reverb.filtergraph");
    
    // ====================================================
    // 1. audio IO
    // read wav file
    int bufferSize = 512;

    AudioBuffer<float> inbuffer;
    auto inputFileInfo = MusicIO::readWavFile(pathToAudioInFile, inbuffer);
    int sampleRate = inputFileInfo.sampleRate;
//
    // load plugin
    auto plugin = MusicIO::loadPlugin(
            pathToPlugin,
            sampleRate,
            bufferSize, 2, 2);
    
    std::cout << "====================" << std::endl;
    auto params = plugin->getParameters();
    for(int i=0; i < params.size(); ++i)
    {
        std::cout << "---" << std::endl;
        std::cout <<  params[i]->getName(50) << std::endl;
        std::cout <<  params[i]->getValue() << std::endl;
    }
    

//
//    // load graph
//    std::unique_ptr<MusicIO::GraphRunnerProcessor> graph;
//    graph = MusicIO::loadGraph(
//            pathToGraph,
//            sampleRate,
//            bufferSize);
//
//    AudioBuffer<float> outBuffer;
//    renderAudio(
//        inbuffer,
//        outBuffer,
//        512,
//        5,
//        sampleRate,
//        plugin);
//
//    MusicIO::writeWavFile(
//            pathToAudioOutFile,
//            outBuffer,
//            sampleRate,
//            16);
//
    
    
    // ====================================================
    // read midi file
    
//    MidiBuffer midiBuffer;
//    int sampleRate = 44100;
//    int bufferSize = 512;
//
//    MusicIO::readMidiFile(pathToMidiInFile, sampleRate, midiBuffer);
//    std::cout << " [midi] time of last evnet: " << midiBuffer.getLastEventTime() << std::endl;
//
//    // ====================================================
////     load plugin
//    String stateString("");
//
//    auto instrument = MusicIO::loadPlugin(
//            pathToInstrument,
//            sampleRate,
//            bufferSize,
//            1,
//            2,
//            true,
//            stateString);
//
//    std::cout << " [render midi]  input channels: " << instrument->getTotalNumInputChannels() << std::endl;
//    std::cout << " [render midi] output channels: " << instrument->getTotalNumOutputChannels() << std::endl;
//
//    // ====================================================
//    // render midi
//
//    AudioBuffer<float> outBuffer;
//
//    renderMidi(
//        midiBuffer,
//        outBuffer,
//        bufferSize,
//        5,
//        sampleRate,
//        instrument);
//
//
//    MusicIO::writeWavFile(
//        pathToAudioOutFile,
//        outBuffer,
//        sampleRate,
//        16);
    return 0;
}



