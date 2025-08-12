/*
* MIT License
*
* Copyright (c) 2025 Open Media Transport Contributors
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*/


/*  omtsendtest.cpp demonstrates the process of creating a named OMT output, 
	and emitting an 8-bit image repeatedly, with the frame rate controlled by OMT 
	it also demonstrates how to setup a log destination, attach vendor information
	to the stream, retrieve OMT statistics on the output stream and also monitor tally  */


#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <stdlib.h>
#include <string.h>

// The header for the C/C++ wrapper of OMT
#include "libomt.h"
// link this exe with libomt, and make sure libomt and libvpx are accessible to the exe, either in the same folder, or linked explicitly via rpath or otherwise.
// libomt will dynamically open libvpx at runtime

using namespace std;

#include <random>

// Generate a random number between a and b
// used to generate audio noise.
float rand_FloatRange(float a, float b)
{
    return ((b - a) * ((float)rand() / (float)RAND_MAX)) + a;
}

int main()
{
    std::cout << "OMTSendTest\n";

    string filename = "omtsendtest.log";
    omt_setloggingfilename(filename.c_str());
    std::cout << "omt_setloggingfilename.success\n";

	// this is the name of the generated OMT Stream in the form HOSTNAME (Test)
    string name = "Test";
    
    // Create the OMT output stream using the default (medium) quality.
    omt_send_t * snd = omt_send_create(name.c_str(), OMTQuality_Default);
    if (snd)
    {
        std::cout << "omt_send_create.success\n";

		// Optionally attach some vendor specific information to the stream.
        OMTSenderInfo info = {};
        string ProductName = "SendTest";
        string Manufacturer =  "OMT";
        string Version = "1.0";
        ProductName.copy(&info.ProductName[0], OMT_MAX_STRING_LENGTH, 0);
        Manufacturer.copy(&info.Manufacturer[0], OMT_MAX_STRING_LENGTH, 0);
        Version.copy(&info.Version[0], OMT_MAX_STRING_LENGTH, 0);
        omt_send_setsenderinformation(snd, &info);

        std::cout << "omt_send_setsenderinformation.success\n";

		// Prepare the OMTMediaFrame Video Frame structure.  Be sure to zero any unused fields
		// here the frame = {} will zero-initialise the structure in C++ 
        OMTMediaFrame video_frame = {};
        
        // OMTMediaFrames can be Video, Audio or Metadata
        video_frame.Type = OMTFrameType_Video;
        video_frame.Width = 1920;
        video_frame.Height = 1080;
        
        // Select the format of data you are passing to OMT
        // This is typically UYVY or BGRA for 8bit or P216 for 10-bit (16 bit data).
        video_frame.Codec = OMTCodec_UYVY;
        
        // Setting Timestamp to -1 will force OMT to auto increment this from zero
        // -1 also enables output clocking so OMT will hold the output on each send
        // to enforce the correct frame rate if your code is able to send faster.
        video_frame.Timestamp = -1;
        
        // OMT uses BT709 for HD and UHD.  Use BT601 for SD streams
        video_frame.ColorSpace = OMTColorSpace_BT709;
        
        // if the Video Frame was interleaved (interlaced), pass OMTVideoFlags_Interlaced
        // OMT uses a single frame of data for Progressive and Interlaced sources.
        video_frame.Flags = OMTVideoFlags_None;
        
        // line stride in bytes, typically width*2 for UYVY and also P216 formats.
        // Can be a custom value in case you are padding lines for byte alignment efficiency,
        video_frame.Stride = video_frame.Width * 2;

        // Total size of the data
        video_frame.DataLength = video_frame.Stride * video_frame.Height;

        // A pointer to the UYVY data which will be passed to OMT
        video_frame.Data = malloc(video_frame.DataLength);
        
        
    
        // The target frame rate expressed as numerator and denominator. In this case 60 fps
        video_frame.FrameRateN = 60000;
        video_frame.FrameRateD = 1000;
        
        // we are passing uncompressed, rather than pre-compressed VMX codec data, so set these to zero
    //    video_frame.CompressedData = NULL;
	//	video_frame.CompressedLength = 0;
		
		// its possible to attach frame specific XML metadata to each frame, otherwise zero these.
	    video_frame.FrameMetadata = NULL;
		video_frame.FrameMetadataLength = 0;
        
      
		// load  sample UYVY data from the california-1080-uyvy.yuv file
        // make sure its in the same folder with the built executable
        void * uyvy = malloc(video_frame.DataLength);
        std::ifstream file("california-1080-uyvy.yuv", std::ios::binary | std::ios::in | std::ios::ate);
        if (file.is_open())
        {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            file.read((char*)uyvy, size);
            file.close();
        }

        // create some audio a stereo buffer exactly 1 frame long
        float * audioBuffer = (float *)malloc(800 * sizeof(float) * 2 );
        // fill the buffer with noise
        srand((unsigned int)time(NULL));
        for (int z=0;z<1600;z++)
        {
            audioBuffer[z] = rand_FloatRange(-1.0,+1.0);
        }

        // prepare an OMTMediaFrame for the audio
        OMTMediaFrame audio_frame = {};
        memset(&audio_frame,0,sizeof(OMTMediaFrame));
        audio_frame.Type = OMTFrameType_Audio;
        audio_frame.Timestamp = -1;
        audio_frame.Codec = OMTCodec_FPA1; // floating point planar data format
        audio_frame.SampleRate = 48000;
        audio_frame.Channels = 2;
        audio_frame.SamplesPerChannel = 800; // we are sending exactly 1 frame of audio per 60fps video frame
        audio_frame.Data = (void *)audioBuffer;
        audio_frame.DataLength = (800 * sizeof(float) * 2);
        audio_frame.FrameMetadata = NULL;
        audio_frame.FrameMetadataLength = 0;
        
		// used to create a dynamically changing image
        void* twoLines = malloc(video_frame.Stride * 2);
        memset(twoLines, 255, video_frame.Stride * 2);
        int linePos = 0;

        
		// structs ready to receive streaming statistics and tally
        OMTTally tally = {};
        OMTStatistics stats = {};

        int frameCount = 0;
        int bytes = 0;
        for (int i = 0; i < 10000; i++)
        {

       		//used to create a dynamically changing image by overwriting 2 lines moving down the image
           memcpy(video_frame.Data, uyvy, video_frame.DataLength);
           memcpy((char*)video_frame.Data + linePos, twoLines, video_frame.Stride * 2);
           linePos += video_frame.Stride * 2;
           if (linePos >= video_frame.DataLength)
           { 
             	linePos = 0;
           }

			// Send out the prepared OMT Video Frame.
            bytes += omt_send(snd, &video_frame);

			// gather and output statistics once per second
            frameCount += 1;
            if (frameCount >= 60) 
            {
                std::cout << "omt_send.ok: " << bytes << "\n";
                omt_send_gettally(snd, 0, &tally);
                std::cout << "omt_send.tally: " << tally.preview << " " << tally.program << "\n";

                OMTMediaFrame* frame = omt_send_receive(snd, 0);
                if (frame) {
                    char* sz = (char*)frame->Data;
                    std::cout << "omt_send.meta: " << sz << "\n";
                }

                int connections = omt_send_connections(snd);
                std::cout << "omt_send.connections: " << connections << "\n";

                omt_send_getvideostatistics(snd, &stats);
                std::cout << "omt_send_getvideostatistics: Bytes: " << stats.BytesSent << " Frames: " << stats.Frames << "\n";

                frameCount = 0;
                bytes = 0;
            }
            
            // Send out the prepared OMT Audio Frame.
            omt_send(snd, &audio_frame);
            // make some different noise for next frame
            for (int z=0;z<1600;z++)
            {
                audioBuffer[z] = rand_FloatRange(-1.0,+1.0);
            }
        }

		// close and clean up the OMT output
        omt_send_destroy(snd);
        std::cout << "omt_send_destroy.success\n";
    }
    else {
        std::cout << "omt_send_create.failed\n";
    }
    return 0;
}

