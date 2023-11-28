#include <coreinit/thread.h>
#include <coreinit/time.h>

#include <whb/proc.h>
//#include <whb/log.h>
#include "loggfx.h"
//#include <whb/log_console.h>
#include "log_consolegfx.h"
#include <vpad/input.h>
#include "../../../../include/camera/camera.h"
#include <malloc.h> // memalign()
#include <string.h> // memset()
#include <stdio.h>

#include "../../../../include/mic/mic.h"

#include <coreinit/memheap.h>
#include <coreinit/cache.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memory.h>
#include <coreinit/screen.h>
#include <proc_ui/procui.h>

#include "sound.h"

static CAMHandle cam = -1;
static CAMSurface* surface;
static int size = -1;
static void* mem;
static void* image;
static int image_size;
static bool drawText = true;

void LoadNV12(int *data, bool dataIs16To240);

void *alloc_zeroed(size_t alignment, size_t size)
{
   void *result = memalign(alignment, size);
   if (result)
      memset(result, 0, size);

   return result;
}

void camEventHandler(CAMEventHandlerInput* eventHandlerInput)
{
   if (eventHandlerInput->data0 != 0)
   {
      WHBLogPrintf("type=%d data0=%d data1=%d data2=%d",
            eventHandlerInput->type, eventHandlerInput->data0, eventHandlerInput->data1, eventHandlerInput->data2);
      // Wii U:  type=0 data0=imagePtr data1=0      data2=0 // photo taken
      // Wii U:  type=0 data0=0        data1=0      data2=1 // no photo taken
      // Cemu:   type=0 data0=imagePtr data1=552960 data2=0
      //                                     ^ CAMERA_YUV_BUFFER_SIZE
   }
}

void closeCamera()
{
   if (cam != -1)
   {
      CAMError err = CAMClose(cam);
      WHBLogPrintf("CAMClose: err=%d", err);
      CAMExit(cam);
      cam = -1;
      // TODO free mem and image
      /*
      if (mem != NULL)
         free(mem);
      if (image != NULL)
         free(image);
      */
   }
}

void getAllMICStates(MICHandle mic)
{
   MICError err;
   uint32_t value;
   err = MICGetState(mic, MIC_STATE_SAMPLE_RATE, &value);
   WHBLogPrintf("MICGetState: err=%d", err);
   if (err < 0)
      return;
   WHBLogPrintf("MICGetState: MIC_STATE_SAMPLE_RATE=%d", value); // 32000

   err = MICGetState(mic, MIC_STATE_GAIN_DB, &value);
   WHBLogPrintf("MICGetState: err=%d", err);
   if (err < 0)
      return;
   WHBLogPrintf("MICGetState: MIC_STATE_GAIN_DB=%d", value); // 6400

   err = MICGetState(mic, MIC_STATE_GAIN_MIN, &value);
   WHBLogPrintf("MICGetState: err=%d", err);
   if (err < 0)
      return;
   WHBLogPrintf("MICGetState: MIC_STATE_GAIN_MIN=%d", value); // 0

   err = MICGetState(mic, MIC_STATE_GAIN_MAX, &value);
   WHBLogPrintf("MICGetState: err=%d", err);
   if (err < 0)
      return;
   WHBLogPrintf("MICGetState: MIC_STATE_GAIN_MAX=%d", value); // 6400

   err = MICGetState(mic, MIC_STATE_GAIN_STEP, &value);
   WHBLogPrintf("MICGetState: err=%d", err);
   if (err < 0)
      return;
   WHBLogPrintf("MICGetState: MIC_STATE_GAIN_STEP=%d", value); // 128

//   /* Cemu crashes:
   err = MICGetState(mic, MIC_STATE_MUTE, &value);
   WHBLogPrintf("MICGetState: err=%d", err); // -1
//   if (err < 0)
//      return;
   WHBLogPrintf("MICGetState: MIC_STATE_MUTE=%d", value); // 128
//   */

   err = MICGetState(mic, MIC_STATE_ECHO_CANCELLATION, &value);
   WHBLogPrintf("MICGetState: err=%d", err);
//   if (err < 0)
//      return;
   WHBLogPrintf("MICGetState: MIC_STATE_ECHO_CANCELLATION=%d", value); // 0

   err = MICGetState(mic, MIC_STATE_AUTO_SELECTION, &value);
   WHBLogPrintf("MICGetState: err=%d", err);
//   if (err < 0)
//      return;
   WHBLogPrintf("MICGetState: MIC_STATE_AUTO_SELECTION=%d", value); // 0

//   /* Cemu crashes:
   err = MICGetState(mic, MIC_STATE_DIGITAL_GAIN_DB, &value);
   WHBLogPrintf("MICGetState: err=%d", err);
//   if (err < 0)
//      return;
   WHBLogPrintf("MICGetState: MIC_STATE_DIGITAL_GAIN_DB=%d", value); // 0
//   */
}

static int modulus = 0xFFFF; // at least 0x2800
static int16_t s_audioBuffer[2 * 0xFFFF];

// replace with a fancy equalizer
void dumpBuffer()
{
   int sum = 0;
   for (int i = 0; i < 2 * 0xFFFF; i++)
   {
      sum += s_audioBuffer[i];
   }

   WHBLogPrintf("mictest: input=%d", sum);
}

static MICHandle mic = -1;

void closeMicrophone()
{
   if (mic == -1)
      return;

   MICError err = MICClose(mic);
   WHBLogPrintf("MICClose: err=%d", err);
   if (err < 0)
      return;

   err = MICUninit(mic);
   WHBLogPrintf("MICUninit: err=%d", err);
   if (err < 0)
      return;

   mic = -1;
}

bool openMicrophone()
{
   if (mic != -1)
      return true;

   MICError err;
//   int ring_size = 1024;
   //int16_t* base = alloc_zeroed(32, ring_size * sizeof(int16_t));
   MICRingBuffer *ringBuffer = alloc_zeroed(32, sizeof(MICRingBuffer));
   ringBuffer -> modulus = modulus; //0x2000; // 2800
   ringBuffer -> base = s_audioBuffer;
//   MEMResourceElement* mem1 = alloc_zeroed(32, sizeof(MEMResourceElement));
//   mem1 -> size = ring_size;
//   MEMResource* res = alloc_zeroed(32, sizeof(MEMResource));
//   res -> numMemRes = 0;
//   res -> memRes  = mem1;

   mic = MICInit(MIC_INSTANCE_0, /*res*/ 0, ringBuffer, &err);
   WHBLogPrintf("MICInit: err=%d handle=%d", err, mic);
   if (err < 0)
      return false;

   err = MICOpen(mic);
   WHBLogPrintf("MICOpen: err=%d", err);
   if (err < 0)
      return false;

   return true;
}

void WHBLogConsoleDraw()
{
   WHBLogConsoleDrawClear();

   if (image != NULL)
      LoadNV12(image, false);

   if (drawText)
      WHBLogConsoleDrawText();

   WHBLogConsoleDrawFlip();
}

void recordMicrophone()
{
   WHBLogPrintf("mictest: recording 2s audio...");
   WHBLogConsoleDraw(); // flush previous line

   if (!openMicrophone())
      return;

   //getAllMICStates(mic);

//   err = MICSetDataConsumed(mic, 32768); // 8, 8*8, ... influences return values of MICGetStatus()
//   WHBLogPrintf("MICSetDataConsumed: err=%d", err); // -81 if not multiple of 8

//   MICStatus* status = alloc_zeroed(32, sizeof(MICStatus));
//   err = MICGetStatus(mic, status);
//   WHBLogPrintf("MICGetStatus: err=%d", err);
//   if (err < 0)
//      return;
//
//   WHBLogPrintf("MICGetStatus: flags=%d available=%d readPos=%d", status->flags, status->available, status->readPos); // 7, 0, 0
                                                                                                                        // 7, 8184, 8 (after MICSetDataConsumed(mic, 8))
   // record 2 seconds audio (s_audioBuffer can store about 2s)
   OSSleepTicks(OSMillisecondsToTicks(2000));

   const char* wavFilename = "mic.raw";
   FILE* wavFile = fopen(wavFilename, "wb");
   if (wavFile != NULL)
   {
      fwrite(s_audioBuffer, modulus * 2, 1, wavFile);
      fclose(wavFile);
      WHBLogPrintf("file written: %s", wavFilename);
      WHBLogPrintf("raw file format: signed 16-bit PCM, big-endian, mono, 32000 Hz");
   }
   else
   {
      WHBLogPrintf("Error writing file: %s", wavFilename);
   }

   closeMicrophone();
}

void openCamera()
{
   closeCamera();

   CAMStreamInfo *streamInfo = alloc_zeroed(32, sizeof(CAMStreamInfo));
   streamInfo->type = CAM_STREAM_VIDEO;
   streamInfo->height = CAMERA_STREAM_HEIGHT;
   streamInfo->width = CAMERA_STREAM_WIDTH;

   size = CAMGetMemReq(streamInfo); // returns 5714944 on Wii U and 1024 on Cemu
   WHBLogPrintf("CAMGetMemReq: size=%d", size);
   if (size < 0) // never happens
      return;

   mem = alloc_zeroed(32, size * sizeof(int));

   CAMWorkMem *workMem = alloc_zeroed(32, sizeof(CAMWorkMem));
   workMem->size = size;
   workMem->mem = mem;

   CAMMode *mode = alloc_zeroed(32, sizeof(CAMMode));
   mode->display = CAM_DISPLAY_MODE_DOWNSTREAM;
   mode->fps = CAM_FRAME_RATE_15;

   CAMSetupInfo *setupInfo = alloc_zeroed(32, sizeof(CAMSetupInfo));
   setupInfo->streamInfo = *streamInfo;
   setupInfo->workMem = *workMem;
   setupInfo->eventHandler = camEventHandler;
   setupInfo->mode = *mode;
   setupInfo->threadAffinity = OS_CORE_AFFINITY_0;

   CAMError err;
   cam = CAMInit(CAM_INSTANCE_0, setupInfo, &err);
   WHBLogPrintf("CAMInit: err=%d, handle=%d", err, cam);
   if (err < 0)
      return;

   err = CAMOpen(cam);
   WHBLogPrintf("CAMOpen: err=%d", err);
   if (err < 0)
      return;

   err = CAMCheckMemSegmentation(mem, size * sizeof(int));
   WHBLogPrintf("CAMCheckMemSegmentation: err=%d", err);
}

void initImage()
{
   if (image != NULL)
      return;
   image_size = CAMERA_YUV_BUFFER_SIZE * sizeof(int);
   image = alloc_zeroed(32, image_size);
}

void getCameraImage()
{
   initImage();

   surface = alloc_zeroed(32, sizeof(CAMSurface));
   surface->imageSize = CAMERA_YUV_BUFFER_SIZE;
   surface->imagePtr = image;
   surface->height = CAMERA_STREAM_HEIGHT;
   //surface->width = CAMERA_STREAM_WIDTH; // image begins always at different location (?)
   surface->width = CAMERA_STREAM_PITCH;
   surface->pitch = CAMERA_STREAM_PITCH;
   surface->alignment = CAMERA_YUV_BUFFER_ALIGNMENT;
   surface->tileMode = CAM_TILE_LINEAR;
   surface->pixelFormat = CAM_FORMAT_NV12;

   CAMError err = CAMSubmitTargetSurface(cam, surface);
   WHBLogPrintf("CAMSubmitTargetSurface: err=%d", err);
}

// play audio adapted from https://github.com/QuarkTheAwesome/LiveSynthesisU/blob/master/src/program.c
voiceData voice1;
AXVoiceOffsets voiceBuffer;
AXVoiceSrc voiceSrc;
AXVoiceDeviceMixData mix;

void axFrameCallback() {
   if (voice1.stopRequested) {
      //Stop the voice
      AXSetVoiceState(voice1.voice, 0);
      //Clear the flag
      voice1.stopRequested = 0;
      voice1.stopped = 1;
//      DCFlushRange(&voice1, sizeof(voice1));
   } else if (voice1.modified) {
      //does this really need to happen in the callback?
//      DCInvalidateRange(&voice1, sizeof(voice1));
//      memset(&voiceBuffer, 0, sizeof(voiceBuffer));
      voiceBuffer.data = voice1.samples;
      voiceBuffer.dataType = AX_VOICE_FORMAT_LPCM16;
      voiceBuffer.loopingEnabled = 0; // was 1
      voiceBuffer.currentOffset = 0;
      voiceBuffer.endOffset = voice1.numSamples - 1;
      voiceBuffer.loopOffset = 0;

      voiceSrc.ratio = (unsigned int)(0x00010000 * ((float)32000 / (float)AXGetInputSamplesPerSec()));

      AXSetVoiceOffsets(voice1.voice, &voiceBuffer);
      AXSetVoiceSrc(voice1.voice, &voiceSrc);
      AXSetVoiceSrcType(voice1.voice, 1);
      AXSetVoiceState(voice1.voice, 1);
      voice1.modified = 0;
      voice1.stopped = 0;
//      DCFlushRange(&voice1, sizeof(voice1));
   }
}

void playMicrophoneRecording()
{
   const char* wavFilename = "mic.raw";
   FILE* file = fopen(wavFilename, "rb");
   if (file != NULL)
   {
      fread(s_audioBuffer, modulus * 2, 1, file);
      fclose(file);
      WHBLogPrintf("file read: %s", wavFilename);
   }
   else
   {
      WHBLogPrintf("Error reading file: %s", wavFilename);
      return;
   }

   AXInitParams initparams = {
         .renderer = AX_INIT_RENDERER_32KHZ,
         .pipeline = AX_INIT_PIPELINE_SINGLE,
   };
   AXInitWithParams(&initparams);

   AXRegisterFrameCallback((void*)axFrameCallback); //this callback doesn't really do much
   voice1.samples = (unsigned char*) s_audioBuffer;
   voice1.stopped = 1;

   voice1.voice = AXAcquireVoice(25, 0, 0); //get a voice, priority 25. Just a random number I picked
   if (!voice1.voice) {
      WHBLogPrintf("sound error");
      return;
   }

   AXVoiceBegin(voice1.voice);
   AXSetVoiceType(voice1.voice, 0);

   AXVoiceVeData vol = {
         .volume = 0x8000,
   };

   AXSetVoiceVe(voice1.voice, &vol);

   //Device mix? Volume of DRC/TV?
   mix.bus[0].volume = 0x8000;
   mix.bus[2].volume = 0x8000;

   AXSetVoiceDeviceMix(voice1.voice, 0, 0, &mix);
   AXSetVoiceDeviceMix(voice1.voice, 1, 0, &mix);

   voice1.numSamples = modulus * 2;
   voice1.modified = 1;

   AXVoiceEnd(voice1.voice);

//   //flush the samples to memory
//   DCFlushRange(voice1.samples, modulus * 2);
//   //mark the voice as modified so the audio callback will reload it
//   voice1.modified = 1;
//   //flush changes to memory
//   DCFlushRange(&voice1, sizeof(voice1));

}

int main(int argc, char **argv)
{
   const char* filename = "selfie.nv12";

   WHBLogConsoleSetColor(0);
   WHBProcInit();
   WHBLogConsoleInit();
   WHBLogPrintf("camtest: A = exit B = selfie Y = stream R/L = save/load");
   WHBLogPrintf("mictest: X = open mic ZR = record 2 seconds audio");
   WHBLogPrintf("                      ZL = play recording");
   WHBLogPrintf("         +/- = log on/off");

   openCamera();

   while(WHBProcIsRunning())
   {
      VPADStatus vpadStatus;
      VPADRead(VPAD_CHAN_0, &vpadStatus, 1, NULL);
      uint32_t btn = vpadStatus.release;
      uint32_t btn_h = vpadStatus.hold;
      if (btn == VPAD_BUTTON_A)
      {
         // exit
         closeCamera();
         closeMicrophone();
         break;
      }
      if (btn == VPAD_BUTTON_B)
         getCameraImage();
      if (btn_h == VPAD_BUTTON_Y)
         getCameraImage();
      if (btn == VPAD_BUTTON_MINUS)
         drawText = false;
      if (btn == VPAD_BUTTON_PLUS)
         drawText = true;
      if (btn == VPAD_BUTTON_L)
      {
         initImage();
         FILE* file = fopen(filename, "rb");
         if (file != NULL)
         {
            fread(image, image_size, 1, file);
            fclose(file);
            WHBLogPrintf("file read: %s", filename);
         }
         else
         {
            WHBLogPrintf("Error reading file: %s", filename);
         }
      }
      if (btn == VPAD_BUTTON_R)
      {
         initImage();
         FILE* file = fopen(filename, "wb");
         if (file != NULL)
         {
            fwrite(image, image_size, 1, file);
            fclose(file);
            WHBLogPrintf("file written: %s", filename);
         }
         else
         {
            WHBLogPrintf("Error writing file: %s", filename);
         }
      }
      if (btn == VPAD_BUTTON_X)
      {
         if (mic == -1)
            openMicrophone();
         else
            closeMicrophone();
      }
      if (btn == VPAD_BUTTON_ZR)
      {
         recordMicrophone();
      }
      if (btn == VPAD_BUTTON_ZL)
      {
         playMicrophoneRecording();
      }

      if (mic != -1)
         dumpBuffer();

      WHBLogConsoleDraw();
      OSSleepTicks(OSMillisecondsToTicks(100));
   }

   WHBLogPrintf("Exiting... good bye.");
   WHBLogConsoleDraw();
   OSSleepTicks(OSMillisecondsToTicks(1000));

   WHBLogConsoleFree();
   WHBProcShutdown();
   return 0;
}

// https://gist.github.com/Thevncore/6336e3b151b620ec6fac00df19f23b48
// This snippet is based almost entirely on Neil Townsend's answer at: https://stackoverflow.com/questions/9325861/converting-yuv-rgbimage-processing-yuv-during-onpreviewframe-in-android
void LoadNV12(int *data, bool dataIs16To240)
{
   // the bitmap we want to fill with the image
   int imageWidth = CAMERA_STREAM_PITCH;
   int imageHeight = CAMERA_STREAM_HEIGHT;

   // Wii U
   imageWidth /= 2;
   imageHeight /= 2;
   int cropX = 64;

   int numPixels = imageWidth * imageHeight;

   // Holding variables for the loop calculation
   int R = 0;
   int G = 0;
   int B = 0;

   // Get each pixel, one at a time
   for (int y = 0; y < imageHeight; y++)
   {
       for (int x = 0; x < imageWidth - cropX; x++)
       {
          // Get the Y value, stored in the first block of data
          // The logical "AND 0xff" is needed to deal with the signed issue
          //float Y = (float)(data[y * imageWidth + x] & 0xff); // Android
          float Y = (float)(data[y * imageWidth + x / 2] & 0xff); // Wii U

          // Get U and V values, stored after Y values, one per 2x2 block
          // of pixels, interleaved. Prepare them as floats with correct range
          // ready for calculation later.
          //int xby2 = x / 2;
          //int yby2 = y / 2;

          // Wii U: MSB and LSB in data[numpixels + ...] are identical, each contains U and V (shift by 8)
          //        example: 0x7F787F78, U = 0x7F, V = 0x78

          // make this V for NV12/420SP
          //float U = (float)(data[numPixels + 2 * xby2 + yby2 * imageWidth]      & 0xff) - 128.0f; // Android
          float   U = (float)(data[numPixels + y * imageWidth / 2 + x / 2] >> 8 & 0xff) - 128.0f; // Wii U

          // make this U for NV12/420SP
          //float V = (float)(data[numPixels + 2 * xby2 + 1 + yby2 * imageWidth] & 0xff) - 128.0f; // Android
          float V = (float)(data[numPixels + y * imageWidth / 2 + x / 2] & 0xff) - 128.0f; // Wii U

          if (dataIs16To240)
          {
             // Correct Y to allow for the fact that it is [16..235] and not [0..255]
             Y = 1.164f * (Y - 16.0f);

             // Do the YUV -> RGB conversion
             // These seem to work, but other variations are quoted
             // out there.
             R = (int)(Y + 1.596f * V);
             G = (int)(Y - 0.813f * V - 0.391f * U);
             B = (int)(Y + 2.018f * U);
          }
          else
          {
             // No need to correct Y
             // These are the coefficients proposed by @AlexCohn
             // for [0..255], as per the wikipedia page referenced
             // above
             R = (int)(Y + 1.370705f * V);
             G = (int)(Y - 0.698001f * V - 0.337633f * U);
             B = (int)(Y + 1.732446f * U);
          }

          // Clip rgb values to 0-255
          R = R < 0 ? 0 : R > 255 ? 255 : R;
          G = G < 0 ? 0 : G > 255 ? 255 : G;
          B = B < 0 ? 0 : B > 255 ? 255 : B;
          int A = 0xFF;
          uint32_t color = (R << 24) | (G << 16) | (B << 8) | A;

          OSScreenPutPixelEx(SCREEN_TV, x, y, color);
          OSScreenPutPixelEx(SCREEN_DRC, x, y, color);
       }
   }
}
