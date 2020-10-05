//
//  MusicIO.cpp
//  console_renderer - ConsoleApp
//
//  Created by Wen-Yi hsiao on 2020/10/5.
//

#include "MusicIO.hpp"


using namespace juce;

using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;
using NodeID = juce::AudioProcessorGraph::NodeID;
using Node = juce::AudioProcessorGraph::Node;

// Simple IO
MusicIO::AudioFileInfo MusicIO::readWavFile(
    String pathToAudioInFile,
    AudioBuffer<float>& inBuffer,
    bool isMono)
{
    AudioFileInfo inputFileInfo;
    AudioFormatManager formatManager;
    
    formatManager.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> reader (formatManager.createReaderFor (File (pathToAudioInFile)));

    int numChannels = 2;
    bool flag = true;
    if(isMono)
    {
        numChannels = 1;
        flag = false; // fillLeftoverChannelsWithCopies
    }
    
    
    if (reader != nullptr)
    {
        inBuffer.setSize(numChannels, (int)reader->lengthInSamples);
        reader->read(&inBuffer,
                     0,
                     (int)reader->lengthInSamples,
                     0,
                     true,
                     flag);
    }
    inputFileInfo.sampleRate = (int)reader->sampleRate;
    inputFileInfo.bitsPerSample = (int)reader->bitsPerSample;
    inputFileInfo.sampleLength = (int)reader->lengthInSamples;
    inputFileInfo.numChannels = numChannels;
    
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


std::unique_ptr< AudioPluginInstance > MusicIO::loadPlugin(
    String pathToPlugin,
    int sampleRate,
    int bufferSize,
    int inChannel,
    int outChannel,
    bool isInstrument,
    String stateString)
{
    std::unique_ptr< AudioPluginInstance > plugin;
    
    juce::OwnedArray<PluginDescription> pluginDescriptions;
    juce::KnownPluginList pluginList;
    juce::AudioPluginFormatManager pluginFormatManager;
    String errorMessage;
    
    pluginFormatManager.addDefaultFormats();
    for (int i = pluginFormatManager.getNumFormats(); --i >= 0;)
    {
        pluginList.scanAndAddFile (
            String (pathToPlugin),
            true,
            pluginDescriptions,
            *pluginFormatManager.getFormat(i));
    }
    
    plugin = pluginFormatManager.createPluginInstance (
            *pluginDescriptions[0],
            sampleRate,
            bufferSize,
            errorMessage);
    
    // error handling
    // std::cout << "[plugin] error message:" << errorMessage.toStdString() << std::endl;
    // if (plugin == nullptr) {
    //     std::cout << "plugin is null"<< std::endl;
    // }
    
    // load state
    if(stateString != "")
    {
        std::cout << " [i] using state" << std::endl;
        MemoryBlock m;
        m.fromBase64Encoding(stateString);
        plugin->setStateInformation (m.getData(), (int) m.getSize());
    }
    
    // set I/O channels
    if(not isInstrument)
    {
        plugin->setPlayConfigDetails (inChannel, outChannel, sampleRate, bufferSize);
    }
    
    // non-realtime
    plugin->setNonRealtime (true);
    
    // prepare to play
    plugin->prepareToPlay (sampleRate, bufferSize);
    return plugin;
}

//==============================================================================
// AudioGraph


std::unique_ptr<MusicIO::GraphRunnerProcessor> MusicIO::loadGraph(
    String pathToGraph,
    int sampleRate,
    int bufferSize)
{
 
    std::unique_ptr<MusicIO::GraphRunnerProcessor> graphRunner(
                    new MusicIO::GraphRunnerProcessor(pathToGraph));
    
    graphRunner->setPlayConfigDetails (2, 2, sampleRate, bufferSize);
    graphRunner->prepareToPlay (sampleRate, bufferSize);
    return graphRunner;
}
