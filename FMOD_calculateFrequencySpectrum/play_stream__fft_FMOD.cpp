/*==============================================================================
 * Combinant un exemple de reproducció en streaming de la instalaciçió de FMOD en el càlcul de l'espectre <https://katyscode.wordpress.com/2013/01/16/cutting-your-teeth-on-fmod-part-4-frequency-analysis-graphic-equalizer-beat-detection-and-bpm-estimation/>
 * 
 * + FMOD v2.02.11 dsp_effect_per_speaker.cpp
 * 
 * ???      * https://forum.hardware.fr/hfr/Programmation/C-2/fmod-probleme-getspectrum-sujet_91099_1.htm

 * play_stream.cpp -> /home/magusti/docencia/2k18-2k19/docenciaEnXarxa/FMOD_instalacio/FMOD/fmodstudioapi20000linux/api/core/examples/
 
  * documentació FMOD --> 10.1.9 FFT https://www.fmod.com/docs/2.02/api/effects-reference.html#fft
   --> https://www.fmod.com/docs/2.02/api/core-api-common-dsp-effects.html#fmod_dsp_fft
 
Play Stream Example
Copyright (c), Firelight Technologies Pty, Ltd 2004-2019.

This example shows how to simply play a stream such as an MP3 or WAV. The stream
behaviour is achieved by specifying FMOD_CREATESTREAM in the call to 
System::createSound. This makes FMOD decode the file in realtime as it plays,
instead of loading it all at once which uses far less memory in exchange for a
small runtime CPU hit.



#g++ -pthread -m64 -O2 -o play_stream ../play_stream.cpp ../common.cpp ../common_platform.cpp -Wl,-rpath=\$ORIGIN/../../../core/lib/x86_64/ ../../../core/lib/x86_64/libfmod.so -I../../../core/inc
 

g++ -pthread  -m64 -O2 -o play_stream__fft_FMOD play_stream__fft_FMOD.cpp FMOD_instalacio__articleDocentAnterior/FMOD/fmodstudioapi20000linux/api/core/examples/common.cpp FMOD_instalacio__articleDocentAnterior/FMOD/fmodstudioapi20000linux/api/core/examples/common_platform.cpp  FMOD_instalacio__articleDocentAnterior/FMOD/fmodstudioapi20000linux/api/core/lib/x86_64/libfmod.so   -IFMOD_instalacio__articleDocentAnterior/FMOD/fmodstudioapi20000linux/api/core/inc/  -IFMOD_instalacio__articleDocentAnterior/FMOD/fmodstudioapi20000linux/api/core/examples/ -LFMOD_instalacio__articleDocentAnterior/FMOD/fmodstudioapi20000linux/api/core/lib/x86_64/ -lfmod



g++ -pthread  -m64 -O2 -o play_stream__fft_FMOD play_stream__fft_FMOD.cpp FMOD_v2.02.11/fmodstudioapi20211linux/api/core/examples/common.cpp FMOD_v2.02.11/fmodstudioapi20211linux/api/core/examples/common_platform.cpp  FMOD_v2.02.11/fmodstudioapi20211linux/api/core/lib/x86_64/libfmod.so   -IFMOD_v2.02.11/fmodstudioapi20211linux/api/core/inc/  -FMOD_v2.02.11/fmodstudioapi20211linux/api/core/examples/ -LFMOD_v2.02.11/fmodstudioapi20211linux/api/core/lib/x86_64/ -lfmod

magusti@verdet:~/docencia/2k22-2k23/docenciaEnXarxa/articles/pintarEspectreDeFreqs_FMOD_OpenGL$ play_stream__fft_FMOD
play_stream__fft_FMOD: error while loading shared libraries: libfmod.so.11: cannot open shared object file: No such file or directory


The reason behind this error is that the libraries of the program have been installed in a place where dynamic linker cannot find it.

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/magusti/docencia/2k22-2k23/docenciaEnXarxa/articles/pintarEspectreDeFreqs_FMOD_OpenGL/FMOD_instalacio__articleDocentAnterior/FMOD//fmodstudioapi20000linux/api/core/lib/x86_64/ play_stream__fft_FMOD stero.ogg


LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/magusti/docencia/2k22-2k23/docenciaEnXarxa/articles/pintarEspectreDeFreqs_FMOD_OpenGL/FMOD_instalacio__articleDocentAnterior/FMOD//fmodstudioapi20000linux/api/core/lib/x86_64/ play_stream__fft_FMOD ./FMOD_v2.02.11/fmodstudioapi20211linux/api/core/examples/media/stereo.ogg
==============================================================================*/
#include "fmod.hpp"
#include "common.h"

// int FMOD_Main()
//FMOD_instalacio__articleDocentAnterior/FMOD/fmodstudioapi20000linux/api/core/examples/common_platform.h:10: note: macro "FMOD_Main" defined here
 //#define FMOD_Main() main(int, char**)

//int FMOD_Main(int argc, char *argv[])
int main(int argc, char *argv[])
{
    FMOD::System     *system;
    FMOD::Sound      *sound, *sound_to_play;
    FMOD::Channel    *channel = 0;
    FMOD_RESULT       result;
    unsigned int      version;
    void             *extradriverdata = 0;
    int               numsubsounds;
    
    printf("argc %d + argv[1]=%s\n", argc, argv[1]);
//exit(1);
    
    if (argc < 2) {
        printf("Me falta el fitxer a reproduir. Algo com ./FMOD_v2.02.11/fmodstudioapi20211linux/api/core/examples/media/wave_vorbis.fsb + un fitxer si vols canviar-lo.\n");
        exit(1);
    }
    
    
//     printf("1\n");
    
    Common_Init(&extradriverdata);

//     printf("2\n");

    /*
        Create a System object and initialize.
    */
    result = FMOD::System_Create(&system);
    ERRCHECK(result);
//     printf("3\n");

    result = system->getVersion(&version);
    ERRCHECK(result);
//     printf("4\n");

    if (version < FMOD_VERSION)
    {
        Common_Fatal("FMOD lib version %08x doesn't match header version %08x", version, FMOD_VERSION);
    }

//     printf("5\n");

    //     FMOD_RESULT System::init( int maxchannels, FMOD_INITFLAGS flags, void *extradriverdata );
    result = system->init(32, FMOD_INIT_NORMAL, extradriverdata);
    ERRCHECK(result);

    /*
        This example uses an FSB file, which is a preferred pack format for fmod containing multiple sounds.
        This could just as easily be exchanged with a wav/mp3/ogg file for example, but in this case you wouldnt need to call getSubSound.
        Because getNumSubSounds is called here the example would work with both types of sound file (packed vs single).
    */    
//     printf("6\n");

    printf("Treballant en %s\n", argv[1]);

//      result = system->createStream(Common_MediaPath("wave_vorbis.fsb"), FMOD_LOOP_NORMAL | FMOD_2D, 0, &sound);    
    // No me funciona en "wave_vorbis.fsb", sí en stero.ogg
    //
    //./FMOD_v2.02.11/fmodstudioapi20211linux/api/core/examples/media/wave_vorbis.fsb
    result = system->createStream(Common_MediaPath( argv[1] ), FMOD_LOOP_NORMAL | FMOD_2D, 0, &sound);
    //
//     result = system->createStream(argv[1], FMOD_LOOP_NORMAL | FMOD_2D, 0, &sound);

//     printf("7\n");
//     FMOD_RESULT System::createStream( const char *name_or_data,  FMOD_MODE mode,   FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound );
//     ERRCHECK(result);
//     exit(1);

        printf("8\n");

    result = sound->getNumSubSounds(&numsubsounds);
    ERRCHECK(result);

    if (numsubsounds)
    {
        sound->getSubSound(0, &sound_to_play);
        ERRCHECK(result);
    }
    else
    {
        sound_to_play = sound;
    }

    /*
        Play the sound.
    */
    result = system->playSound(sound_to_play, 0, false, &channel);
    ERRCHECK(result);
//     printf("9\n");
// exit(1);
    /*
        Main loop.
    */
    do
    {
        Common_Update();

        if (Common_BtnPress(BTN_ACTION1))
        {
            bool paused;
            result = channel->getPaused(&paused);
            ERRCHECK(result);
            result = channel->setPaused(!paused);
            ERRCHECK(result);
        }

        result = system->update(); // // Per-frame FMOD update ('system' is a pointer to FMOD::System)
        ERRCHECK(result);

        {
            unsigned int ms = 0;
            unsigned int lenms = 0;
            bool         playing = false;
            bool         paused = false;

            if (channel)
            {
                result = channel->isPlaying(&playing);
                if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE))
                {
                    ERRCHECK(result);
                }

                result = channel->getPaused(&paused);
                if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE))
                {
                    ERRCHECK(result);
                }

                result = channel->getPosition(&ms, FMOD_TIMEUNIT_MS);
                if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE))
                {
                    ERRCHECK(result);
                }
               
                result = sound_to_play->getLength(&lenms, FMOD_TIMEUNIT_MS);
                if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE))
                {
                    ERRCHECK(result);
                }
            }

            Common_Draw("==================================================");
            Common_Draw("Play Stream Example.");
            Common_Draw("Copyright (c) Firelight Technologies 2004-2019.");
            Common_Draw("==================================================");
            Common_Draw("");
            Common_Draw("Press %s to toggle pause", Common_BtnStr(BTN_ACTION1));
            Common_Draw("Press %s to quit", Common_BtnStr(BTN_QUIT));
            Common_Draw("");
            Common_Draw("Time %02d:%02d:%02d/%02d:%02d:%02d : %s", ms / 1000 / 60, ms / 1000 % 60, ms / 10 % 100, lenms / 1000 / 60, lenms / 1000 % 60, lenms / 10 % 100, paused ? "Paused " : playing ? "Playing" : "Stopped");
        }
            
//
            // obtindreElEspectre();
//            
            // getSpectrum() performs the frequency analysis, see explanation below
int sampleSize = 64;
 
float *specLeft, *specRight, *spec;
 
specLeft = new float[sampleSize];
specRight = new float[sampleSize];
 
// Get spectrum for left and right stereo channels
/*
channel->getSpectrum(specLeft, sampleSize, 0, FMOD_DSP_FFT_WINDOW_RECT);
channel->getSpectrum(specRight, sampleSize, 1, FMOD_DSP_FFT_WINDOW_RECT);

FMOD Studio Core API Versus FMOD Ex Core API
 System::getSpectrum and System::getWaveData removed !!!!!!!!!!!!!!!!!!!!!!!
 
 
 Add a custom DSP unit to capture DSP wavedata from the output stage. Use the master channelgroup's DSP head with System::getMasterChannelGroup and ChannelControl::getDSP.
Add a built in FFT DSP unit type to capture spectrum data from the output stage. Create a built in FFT unit with System::createDSPByType and FMOD_DSP_TYPE_FFT, then add the effect to the master ChannelGroup with ChannelControl::addDSP. Use DSP::getParameterData to get the raw spectrum data or use DSP::getParameterFloat to get the dominant frequency from the signal.
 
 FMOD_RESULT System::createDSPByType(
  FMOD_DSP_TYPE type,                   --> FMOD_DSP_TYPE.
  FMOD::DSP **dsp
);
 */
 FMOD::DSP *elDSP;
 
//  result = system->getMasterChannelGroup(&mastergroup);
//     ERRCHECK(result);
//     result = mastergroup->addDSP(0, mydsp);
//     ERRCHECK(result);

//  Create the DSP effects.
  result = system-->createDSPByType(FMOD_DSP_TYPE_FFT, &elDSP);
  if ((result != FMOD_OK) ) // && (result != FMOD_ERR_INVALID_HANDLE))
  {
    ERRCHECK(result);
  }

//   Connect up the DSP network
//   FMOD::ChannelGroup  *mastergroup;
//   result = system->getMasterChannelGroup(&mastergroup);
//   ERRCHECK(result);
    
  elDSP->setParameterInt((int)FMOD.DSP_FFT.WINDOWTYPE, (int)FMOD_DSP_FFT_WINDOW_RECT ); //FMOD.DSP_FFT_WINDOW.HANNING);????
  ERRCHECK(result);
  elDSP->setParameterInt((int)FMOD.DSP_FFT.WINDOWSIZE, sampleSize*2); //WindowSize * 2)
//           The number of frequency bins is half the FFT size.?????   Yes. To be precise the first half of the FFT buffer contains the positive frequencies, which is what you’re interested in. The second half contains the negative frequencies, which is symmetrical to the positive frequencies.
   ERRCHECK(result);

 result = channel->addDSP(0, elDSO); //specLeft);
  if ((result != FMOD_OK) ) 
  {
    ERRCHECK(result);
  }
 result = channel->addDSP(1, elDSP); //specRight);
  if ((result != FMOD_OK) ) 
  {
    ERRCHECK(result);
  }

// spec = new float[sampleSize];
// for (int i = 0; i < sampleSize; i++)
//     spec[i] = (specLeft[i] + specRight[i]) / 2;
// 
//         
// // Find max volume
// auto maxIterator = std::max_element(&spec[0], &spec[sampleSize]);
// float maxVol = *maxIterator;
//  
// // Normalize
// if (maxVol != 0)
//   std::transform(&spec[0], &spec[sampleSize], &spec[0], [maxVol] (float dB) -> float { return dB / maxVol; });
// 
//  // Find frequency range of each array item
// // float hzRange = (44100 / 2) / static_cast(sampleSize);
// // Don’t forget to change 44100 to the sample rate (in Hz) of the audio.
//  float laFreq;
//  int laPrioritat;
//  
//  sound->getDefaults( &laFreq, laPrioritat );
//  float hzRange = (laFreq/ 2) / static_cast(sampleSize);
//     
//  Text(10, 30, "Sample size: " + StringFactory(sampleSize) + "  -  Range per sample: " + StringFactory(hzRange) + "Hz  -  Max vol this frame: " + StringFactory(maxVol), Colour::White, MakeTextFormat(L"Verdana", 14.0f));
//  
//  // Numerical FFT display
//     int nPerRow = 16;
//  
//     for (int y = 0; y < sampleSize / nPerRow; y++)
//         for (int x = 0; x < nPerRow; x++)
//             Text(x * 40 + 10, y * 20 + 60, StringFactory(floor(spec[y * nPerRow + x] * 1000)), Colour::White, freqTextFormat);
//  
// // Clean up
//     delete [] spec;
//     delete [] specLeft;
//     delete [] specRight;//
// // 
// //            
            
/*
 // VU bars
 
int blockGap = 4 / (sampleSize / 64);
int blockWidth = static_cast((static_cast(ResolutionX) * 0.8f) / static_cast(sampleSize) - blockGap);
int blockMaxHeight = 200;
 
// Parameters: Left-hand X co-ordinate of bar, left-hand Y co-ordinate of bar, width of bar, height of bar (negative to draw upwards), paintbrush to use
 
for (int b = 0; b < sampleSize - 1; b++)
    FillRectangleWH(static_cast(ResolutionX * 0.1f + (blockWidth + blockGap) * b),
                    ResolutionY - 50,
                    blockWidth,
                    static_cast(-blockMaxHeight * spec[b]),
                    freqGradient);
*/

        Common_Sleep(50);
    } while (!Common_BtnPress(BTN_QUIT));

    /*
        Shut down
    */
    result = sound->release();  /* Release the parent, not the sound that was retrieved with getSubSound. */
    ERRCHECK(result);
    result = system->close();
    ERRCHECK(result);
    result = system->release();
    ERRCHECK(result);

    Common_Close();

    return 0;
}
