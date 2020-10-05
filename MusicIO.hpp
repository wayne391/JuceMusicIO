//
//  MusicIO.hpp
//  console_renderer - ConsoleApp
//
//  Created by Wen-Yi hsiao on 2020/10/5.
//

#ifndef MusicIO_hpp
#define MusicIO_hpp

#include <JuceHeader.h>
#include <stdio.h>

using namespace juce;


namespace MusicIO {

struct AudioFileInfo{
    int sampleRate;
    int bitsPerSample;
    int sampleLength;
    int numChannels;
};

AudioFileInfo readWavFile(String pathToAudioInFile, AudioBuffer<float>& inBuffer);
void writeWavFile(
        String pathToAudioOutFile,
        AudioBuffer<float>& outBuffer,
        int sampleRate,
        int bitsPerSample);
void readMidiFile(String pathToAudioInFile, int sampleRate, MidiBuffer& midiBuffer);
std::unique_ptr< AudioPluginInstance > loadPlugin(
        String pathToPlugin,
        int sampleRate,
        int bufferSize,
        String stateString="");

} // namespace MusicIO


#endif /* MusicIO_hpp */
