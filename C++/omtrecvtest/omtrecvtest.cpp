#include <iostream>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

using namespace std;

#include "libomt.h"



// We will use this to dump info about the incoming OMT
static int dumpOMTMediaFrameInfo(OMTMediaFrame * video);


int main(int argc, const char * argv[])
{
	omt_send_t * sndloop;
    int nativeReceiveMode = 0;
    int sixteenBitReceiveMode = 0;
    
    // optionally setup logging 
    string filename = "omtrecvtest.log";
    omt_setloggingfilename(filename.c_str());
  
  	// this example can just take a Stream name, plus it can optionally also have either nativevmx or 16bit as a second parameter 
  	// to request compressed VMX data instead of uncompressed video, or to request specifically 16-bit uncompressed video
	if (argc<2)
	{
		 printf("Usage : omtrecvtest \"HOST (OMTSOURCE)\" [nativevmx|16bit]");
		 exit(0);
	}
	
	// this example receives OMT then sends it back out again through another stream.
	// Create a loop out stream
    sndloop = omt_send_create("OMLoopBack", OMTQuality_Default);
    
    // the instance of an OMT receiver.
    omt_receive_t* recv;

	// check for parameters
	if (argc>2)
	{
		if (!strcasecmp((char *)argv[2],"nativevmx"))
		{
			nativeReceiveMode  = 1;
		}
		if (!strcasecmp((char *)argv[2],"16bit"))
		{
			sixteenBitReceiveMode  = 1;
		}
	}

	// setup an OMT Receiver. We specify the types of data we are interested in and then the format, and an optional flag.
	if (nativeReceiveMode)
	{
		// force receive of compressed data only
		recv = omt_receive_create((const char *)argv[1], (OMTFrameType)(OMTFrameType_Video | OMTFrameType_Audio | OMTFrameType_Metadata), (OMTPreferredVideoFormat)OMTPreferredVideoFormat_UYVYorUYVAorP216orPA16, (OMTReceiveFlags)OMTReceiveFlags_CompressedOnly);
	}
	else
	{
		// optionally force 16bit receive
		if (sixteenBitReceiveMode)
		{
			recv = omt_receive_create((const char *)argv[1], (OMTFrameType)(OMTFrameType_Video | OMTFrameType_Audio | OMTFrameType_Metadata), (OMTPreferredVideoFormat)OMTPreferredVideoFormat_P216, (OMTReceiveFlags)OMTReceiveFlags_None);
		}
		else
		{
			recv = omt_receive_create((const char *)argv[1], (OMTFrameType)(OMTFrameType_Video | OMTFrameType_Audio | OMTFrameType_Metadata), (OMTPreferredVideoFormat)OMTPreferredVideoFormat_UYVYorUYVAorP216orPA16, (OMTReceiveFlags)OMTReceiveFlags_None);
		}
	}
 
    while(1)
    {
        OMTMediaFrame frame = {}; // loop out frame
        OMTMediaFrame * theOMTFrame;
        OMTFrameType t = OMTFrameType_None;
        
        // capture a frame of video, audio or metadata from the OMT Source
        theOMTFrame = omt_receive(recv, (OMTFrameType)(OMTFrameType_Video | OMTFrameType_Audio | OMTFrameType_Metadata), 40);
        if (theOMTFrame)
        {
            t = theOMTFrame->Type;
            
			// dump what we got to the console
			dumpOMTMediaFrameInfo(theOMTFrame);


			// we are going to loop the OMT stream back out, so let's make a copy of the Frame
			memcpy(&frame,theOMTFrame,sizeof(OMTMediaFrame));
			switch (t)
			{
				case OMTFrameType_Video:
				{
					// send it back out..  If its native VMX we need to move the ComressedData into Data and CompressedLength into DataLength
					if (nativeReceiveMode &&  theOMTFrame->Codec == OMTCodec_VMX1)
					{
						frame.Data = theOMTFrame->CompressedData;
						frame.DataLength = theOMTFrame->CompressedLength;
						frame.CompressedData = NULL;
						frame.CompressedLength = 0;
					}
					omt_send(sndloop, &frame);
				}
				break;
			
				case OMTFrameType_Audio: case OMTFrameType_Metadata:
				{
					omt_send(sndloop, &frame);
				}
				break;           
			
				case OMTFrameType_None: default:
			 
				break;
			}
        }
    }
   	omt_receive_destroy(recv);
}



#define FourCharCodeToString(code) (char[5]){ (code) & 0xFF, ((code) >> 8) & 0xFF, ((code) >> 16) & 0xFF, ((code) >> 24) & 0xFF, 0 }

static int dumpOMTMediaFrameInfo(OMTMediaFrame * video)
{
    printf("----------------------------------------------\n");
    if (video)
    {
        if (video->Type == OMTFrameType_Video)
        {
                printf("VIDEO FRAME:\n");
                printf("Timestamp=%llu\n",(unsigned long long) video->Timestamp);
                printf("Codec=%.4s\n", FourCharCodeToString(video->Codec));
                printf("Width=%d\n", video->Width);
                printf("Height=%d\n", video->Height);
                printf("Stride=%d\n", video->Stride);
                printf("Flags=%d\n", video->Flags);
                printf("FrameRateN=%d\n", video->FrameRateN);
                printf("FrameRateD=%d\n", video->FrameRateD);
                printf("AspectRatio=%.2f\n", video->AspectRatio);
                printf("ColorSpace=%d\n", video->ColorSpace);
                printf("Data=%llu\n", (unsigned long long)video->Data);
                printf("DataLength=%d\n", video->DataLength);
                printf("CompressedData=%llu\n",(unsigned long long) video->CompressedData);
                printf("CompressedLength=%llu\n", (unsigned long long)video->CompressedLength);
                printf("FrameMetadata=%llu\n", (unsigned long long)video->FrameMetadata);
                printf("FrameMetadataLength=%llu\n", (unsigned long long)video->FrameMetadataLength);
        }
        
        if (video->Type ==  OMTFrameType_Audio)
        {
                printf("AUDIO FRAME:\n");
                printf("Timestamp=%llu\n", (unsigned long long)video->Timestamp);
                printf("Codec=%.4s\n", FourCharCodeToString(video->Codec));
                printf("Flags=%d\n", video->Flags);
                printf("SampleRate=%d\n", video->SampleRate);
                printf("Channels=%d\n", video->Channels);
                printf("SamplesPerChannel=%d\n", video->SamplesPerChannel);
                printf("Data=%llu\n", (unsigned long long)video->Data);
                printf("DataLength=%d\n", video->DataLength);
                printf("FrameMetadata=%llu\n", (unsigned long long)video->FrameMetadata);
                printf("FrameMetadataLength=%llu\n", (unsigned long long)video->FrameMetadataLength);
        }
        
    	if (video->Type ==  OMTFrameType_Metadata)
        {
                printf("METADATA FRAME:\n");
                printf("Timestamp=%llu\n", (unsigned long long)video->Timestamp);
                printf("Flags=%d\n", video->Flags);
                printf("Data=%llu\n", (unsigned long long)video->Data);
                printf("DataLength=%d\n", video->DataLength);
                printf("FrameMetadata=%llu\n", (unsigned long long)video->FrameMetadata);
                printf("FrameMetadataLength=%llu\n", (unsigned long long)video->FrameMetadataLength);
        }

    }
	return 0;
}


