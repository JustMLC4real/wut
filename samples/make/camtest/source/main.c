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

#include <coreinit/memheap.h>
#include <coreinit/cache.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memory.h>
#include <coreinit/screen.h>
#include <proc_ui/procui.h>

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
      // event 1:  type=0 data0=274736480 data1=0 data2=1
      // event 2-: type=0 data0=0         data1=0 data2=1
      // NOTE: data0 = image pointer
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
      // TODO free mem an image
   }
}

void openCamera()
{
   closeCamera();

   CAMStreamInfo *streamInfo = alloc_zeroed(32, sizeof(CAMStreamInfo));
   streamInfo->type = CAM_STREAM_VIDEO;
   streamInfo->height = CAMERA_STREAM_HEIGHT;
   streamInfo->width = CAMERA_STREAM_WIDTH;

   size = CAMGetMemReq(streamInfo); // returns 5714944
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
   WHBLogPrintf("CAMInit: err=%d, handle=%d [size=%d]", err, cam, workMem->size);
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

void WHBLogConsoleDraw()
{
   WHBLogConsoleDrawClear();

   if (image != NULL)
      LoadNV12(image, false);

   if (drawText)
      WHBLogConsoleDrawText();

   WHBLogConsoleDrawFlip();
}

int main(int argc, char **argv)
{
   const char* filename = "selfie.nv12";

   WHBLogConsoleSetColor(0);
   WHBProcInit();
   WHBLogConsoleInit();
   WHBLogPrintf("camtest: A = exit B = selfie Y = stream R/L = save/load");
   WHBLogPrintf("         +/- = log on/off ");

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
