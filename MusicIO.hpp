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

// Graph processor
class GraphRunnerProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;
    using NodeID = juce::AudioProcessorGraph::NodeID;
    using Node = juce::AudioProcessorGraph::Node;

    //==============================================================================
    GraphRunnerProcessor(String pathToGraphFile)
        : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          mainProcessor  (new juce::AudioProcessorGraph())
    {
        
        xmlFileGraph = File (pathToGraphFile);
        formatManager.addDefaultFormats();
    }

    //==============================================================================
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        if (layouts.getMainInputChannelSet()  == juce::AudioChannelSet::disabled()
         || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
            return false;

        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
         && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
            return false;

        return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
    }

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        mainProcessor->setPlayConfigDetails (getMainBusNumInputChannels(),
                                             getMainBusNumOutputChannels(),
                                             sampleRate, samplesPerBlock);

        mainProcessor->prepareToPlay (sampleRate, samplesPerBlock);
        
        initialiseGraph();
    }

    void releaseResources() override
    {
        mainProcessor->releaseResources();
    }

    void processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override
    {
        for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
            buffer.clear (i, 0, buffer.getNumSamples());
        mainProcessor->processBlock (buffer, midiMessages);
        
    }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override          { return new juce::GenericAudioProcessorEditor (*this); }
    bool hasEditor() const override                              { return true; }

    //==============================================================================
    const juce::String getName() const override                  { return "Graph Tutorial"; }
    bool acceptsMidi() const override                            { return true; }
    bool producesMidi() const override                           { return true; }
    double getTailLengthSeconds() const override                 { return 0; }

    //==============================================================================
    int getNumPrograms() override                                { return 1; }
    int getCurrentProgram() override                             { return 0; }
    void setCurrentProgram (int) override                        {}
    const juce::String getProgramName (int) override             { return {}; }
    void changeProgramName (int, const juce::String&) override   {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock&) override       {}
    void setStateInformation (const void*, int) override         {}

private:
    //==============================================================================
    static void readBusLayoutFromXml (AudioProcessor::BusesLayout& busesLayout, AudioProcessor& plugin,
                                      const XmlElement& xml, bool isInput)
    {
        auto& targetBuses = (isInput ? busesLayout.inputBuses
                                     : busesLayout.outputBuses);
        int maxNumBuses = 0;

        if (auto* buses = xml.getChildByName (isInput ? "INPUTS" : "OUTPUTS"))
        {
            forEachXmlChildElementWithTagName (*buses, e, "BUS")
            {
                const int busIdx = e->getIntAttribute ("index");
                maxNumBuses = jmax (maxNumBuses, busIdx + 1);

                // the number of buses on busesLayout may not be in sync with the plugin after adding buses
                // because adding an input bus could also add an output bus
                for (int actualIdx = plugin.getBusCount (isInput) - 1; actualIdx < busIdx; ++actualIdx)
                    if (! plugin.addBus (isInput))
                        return;

                for (int actualIdx = targetBuses.size() - 1; actualIdx < busIdx; ++actualIdx)
                    targetBuses.add (plugin.getChannelLayoutOfBus (isInput, busIdx));

                auto layout = e->getStringAttribute ("layout");

                if (layout.isNotEmpty())
                    targetBuses.getReference (busIdx) = AudioChannelSet::fromAbbreviatedString (layout);
            }
        }

        // if the plugin has more buses than specified in the xml, then try to remove them!
        while (maxNumBuses < targetBuses.size())
        {
            if (! plugin.removeBus (isInput))
                return;

            targetBuses.removeLast();
        }
    }
    //==============================================================================
    
    void createNodeFromXml (const XmlElement& xml)
    {
        PluginDescription pd;

        forEachXmlChildElement (xml, e)
        {
            if (pd.loadFromXml (*e))
                break;
        }

        String errorMessage;
       

        if (auto instance = formatManager.createPluginInstance (pd, mainProcessor->getSampleRate(),
                                                                mainProcessor->getBlockSize(), errorMessage))
        {
            if (auto* layoutEntity = xml.getChildByName ("LAYOUT"))
            {
                auto layout = instance->getBusesLayout();

                readBusLayoutFromXml (layout, *instance, *layoutEntity, true);
                readBusLayoutFromXml (layout, *instance, *layoutEntity, false);

                instance->setBusesLayout (layout);
            }

            if (auto node = mainProcessor->addNode (std::move (instance), NodeID ((uint32) xml.getIntAttribute ("uid"))))
            {
                if (auto* state = xml.getChildByName ("STATE"))
                {
                    MemoryBlock m;
                    m.fromBase64Encoding (state->getAllSubText());

                    node->getProcessor()->setStateInformation (m.getData(), (int) m.getSize());
                }

            }
        }
    }
    
    //==============================================================================
    
    void restoreFromXml (const XmlElement& xml)
    {

        forEachXmlChildElementWithTagName (xml, e, "FILTER")
        {
            createNodeFromXml (*e);
        }

        forEachXmlChildElementWithTagName (xml, e, "CONNECTION")
        {
             mainProcessor->addConnection ({ { NodeID ((uint32) e->getIntAttribute ("srcFilter")), e->getIntAttribute ("srcChannel") },
                                   { NodeID ((uint32) e->getIntAttribute ("dstFilter")), e->getIntAttribute ("dstChannel") } });
            continue;
        }

        mainProcessor->removeIllegalConnections();
    }
    
    //==============================================================================
    
    void getIOMap (const XmlElement& xml)
    {
        
        NodeID audioInputDummy;
        NodeID audioOutputDummy;
        NodeID midiInputDummy;
        NodeID midiOutputDummy;
        
        
        forEachXmlChildElementWithTagName (xml, e, "FILTER")
       {
           auto currentNode = NodeID ((uint32) e->getAttributeValue(0).getIntValue () );
           auto pluginName = e->getChildByName("PLUGIN")->getAttributeValue(0);
           
           if (pluginName == "Audio Input")
           {
               std::cout << "Audio Input Detected " << std::endl;
               audioInputDummy = currentNode;
           }
            
           if (pluginName == "MIDI Input")
           {
               std::cout << "Midi Input Detected " << std::endl;
               audioOutputDummy = currentNode;
           }
           
           if (pluginName == "Audio Output")
           {
               std::cout << "Audio Output Detected " << std::endl;
               audioOutputDummy = currentNode;
           }

           if (pluginName == "MIDIOutput")
           {
               std::cout << "Midi Output Detected " << std::endl;
               midiOutputDummy = currentNode;
           }
       }
        
        
        forEachXmlChildElementWithTagName (xml, e, "CONNECTION")
        {
            auto srcNode = NodeID ((uint32) e->getIntAttribute ("srcFilter"));
            auto srcChannel = e->getIntAttribute ("srcChannel");
            auto dstNode = NodeID ((uint32) e->getIntAttribute ("dstFilter"));
            auto dstCahnnel = e->getIntAttribute ("dstChannel");
            
            // Inputs
            if(audioInputDummy == srcNode)
            {
                std::cout << "audio Input Connected " << std::endl;
                mainProcessor->addConnection ({
                    { audioInputNode->nodeID, srcChannel},
                    { dstNode, dstCahnnel}});
                continue;
            }
            
            if(midiInputDummy == srcNode){
                std::cout << "midi Input Connected " << std::endl;
                mainProcessor->addConnection ({
                    { midiInputNode->nodeID, srcChannel},
                    { dstNode, dstCahnnel}});
                continue;
            }
                
            // Outputs
            if(audioOutputDummy == dstNode)
            {
                std::cout << "Audio Output Connected " << std::endl;
                mainProcessor->addConnection ({
                   { srcNode, srcChannel},
                   { audioOutputNode->nodeID, dstCahnnel}});
                continue;
            }

            if(audioOutputDummy == dstNode)
           {
               std::cout << "Midi Output Connected " << std::endl;
               mainProcessor->addConnection ({
                  { srcNode, srcChannel},
                  { midiOutputNode->nodeID, dstCahnnel}});
               continue;
           }
        }
        
    }
    
    void initialiseGraph()
    {
        mainProcessor->clear();
        
        // load xml
        XmlDocument xmlDocGraph (xmlFileGraph);
        std::unique_ptr< XmlElement > xmlElementGraph = xmlDocGraph.getDocumentElementIfTagMatches ("FILTERGRAPH");
        restoreFromXml (*xmlElementGraph);
        
        // io nodes
        audioInputNode  = mainProcessor->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::audioInputNode));
        audioOutputNode = mainProcessor->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::audioOutputNode));
        midiInputNode   = mainProcessor->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::midiInputNode));
        midiOutputNode  = mainProcessor->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::midiOutputNode));

        // getIOMap
        getIOMap(*xmlElementGraph);
        
        // enable all buses
        for (auto node : mainProcessor->getNodes())
            node->getProcessor()->enableAllBuses();
    }
    
    //==============================================================================
    std::unique_ptr<juce::AudioProcessorGraph> mainProcessor;

    Node::Ptr audioInputNode;
    Node::Ptr audioOutputNode;
    Node::Ptr midiInputNode;
    Node::Ptr midiOutputNode;

    Node::Ptr gainNode;
    Node::Ptr osciNode;
    
    File xmlFileGraph;
    
    juce::AudioPluginFormatManager formatManager;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphRunnerProcessor)
};


AudioFileInfo readWavFile(
        String pathToAudioInFile,
        AudioBuffer<float>& inBuffer,
        bool isMono=false);
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
        int inChannel,
        int outChannel,
        bool isInstrument=false,
        String stateString="");
std::unique_ptr<GraphRunnerProcessor> loadGraph(
    String pathToGraph,
    int sampleRate,
    int bufferSize);

} // namespace MusicIO


#endif /* MusicIO_hpp */
