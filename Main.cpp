/*
  ==============================================================================
    This file contains the basic startup code for a JUCE application.
  ==============================================================================
*/

#include <JuceHeader.h>
#include "MusicIO.hpp"

using namespace juce;

int main (int argc, char* argv[])
{
    ScopedJuceInitialiser_GUI initialiser; // required for JUCE console app

//    String pathToAudioInFile("/Users/wayne391/Documents/Projects/MyPluginHost/console/Source/test.wav");
//    String pathToAudioOutFile("/Users/wayne391/Documents/Projects/MyPluginHost/console/Source/test_outout.wav");
//    String pathToMidiInFile("/Users/wayne391/Documents/Projects/MyPluginHost/console/Source/test.mid");
    String pathToPlugin("/Library/Audio/Plug-Ins/Components/ValhallaShimmer.component");
    
    // ====================================================
    // read wav file
    
//    AudioBuffer<float> buffer;
//    auto inputFileInfo = MusicIO::readWavFile(pathToAudioInFile, buffer);
//
//    int sampleLength = inputFileInfo.sampleLength;
//    int numChannels = inputFileInfo.numChannels;
//    int sampleRate = inputFileInfo.sampleRate;
//    int bitsPerSample = inputFileInfo.bitsPerSample;
//
//    std::cout << "=========================" << std::endl;
//    std::cout << "[Input Audio File Info]" << std::endl;
//    std::cout << " > sample length: " << inputFileInfo.sampleLength << std::endl;
//    std::cout << " >      channels: " << inputFileInfo.numChannels << std::endl;
//    std::cout << " >   sample rate: " << inputFileInfo.sampleRate << std::endl;
//    std::cout << " >    bits depth: " << inputFileInfo.bitsPerSample << std::endl;
//    std::cout << "=========================" << std::endl;
//
    // ====================================================
    // wrtie wav file
//    MusicIO::writeWavFile(pathToAudioInFile, buffer, sampleRate, bitsPerSample);
    
    // ====================================================
    // read midi file
    
//    MidiBuffer midiBuffer;
//    MusicIO::readMidiFile(pathToMidiInFile, sampleRate, midiBuffer);
//    std::cout << " [midi] time of last evnet: " << midiBuffer.getLastEventTime() << std::endl;
    
    // ====================================================
    // load plugin
    int sampleRate = 44100;
    int bufferSize = 512;
    
    auto plugin = MusicIO::loadPlugin(
        pathToPlugin,
        sampleRate,
        bufferSize);
    return 0;
}
