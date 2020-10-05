//
//  MusicIO.cpp
//  console_renderer - ConsoleApp
//
//  Created by Wen-Yi hsiao on 2020/10/5.
//

#include "MusicIO.hpp"


using namespace juce;


MusicIO::AudioFileInfo MusicIO::readWavFile(String pathToAudioInFile, AudioBuffer<float>& inBuffer)
{
    AudioFileInfo inputFileInfo;
    AudioFormatManager formatManager;
    
    formatManager.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> reader (formatManager.createReaderFor (File (pathToAudioInFile)));

    if (reader != nullptr)
    {
        inBuffer.setSize(2, (int)reader->lengthInSamples);
        reader->read(&inBuffer,
                     0,
                     (int)reader->lengthInSamples,
                     0,
                     true,
                     true);
    }
    inputFileInfo.sampleRate = (int)reader->sampleRate;
    inputFileInfo.bitsPerSample = (int)reader->bitsPerSample;
    inputFileInfo.sampleLength = (int)reader->lengthInSamples;
    inputFileInfo.numChannels = 2;
    
    std::cout << "sampleLength:" << inputFileInfo.sampleLength << std::endl;
    std::cout << "numChannels:" << inputFileInfo.numChannels << std::endl;
    return inputFileInfo;
}


void MusicIO::writeWavFile(
    String pathToAudioOutFile,
    AudioBuffer<float>& outBuffer,
    int sampleRate,
    int bitsPerSample)
{

    File outFile(pathToAudioOutFile);
    outFile.deleteFile();

    WavAudioFormat format;
    std::unique_ptr<AudioFormatWriter> writer;

    writer.reset(
        format.createWriterFor(new FileOutputStream(outFile),
        sampleRate,
        outBuffer.getNumChannels(),
        bitsPerSample,
        {},
        0));

    if (writer != nullptr)
        writer->writeFromAudioSampleBuffer(outBuffer, 0, outBuffer.getNumSamples());
}


void MusicIO::readMidiFile(String pathToAudioInFile, int sampleRate, MidiBuffer& midiBuffer)
{
    FileInputStream fileStream(pathToAudioInFile);
    
    // laod file
    MidiFile Mfile;
    Mfile.readFrom(fileStream);
    Mfile.convertTimestampTicksToSeconds();
    
    // initialize buffer
    midiBuffer.clear();
    
    // add event to buffer
    for (int t = 0; t < Mfile.getNumTracks(); t++) {
        const MidiMessageSequence* track = Mfile.getTrack(t);
        for (int i = 0; i < track->getNumEvents(); i++) {
            MidiMessage& m = track->getEventPointer(i)->message;
            // std::cout<<m.getDescription ()<<" time:" << m.getTimeStamp() << "\n";
            int sampleOffset = (int)(sampleRate * m.getTimeStamp());
            midiBuffer.addEvent(m, sampleOffset);
        }
    }
}
