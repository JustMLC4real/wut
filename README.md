
# camtest
A simple example to test the Wii U webcam and microphone.

AFAIK this is the first open source Wii U webcam example ever! :D

# Raw image format
The webcam uses the NV12 format.

![screenshot of webcam app showing Mario](samples/make/camtest/selfie-mario.png)

If you'd like to investigate the raw format of the above image: [raw image](samples/make/camtest/selfie-mario.nv12)

**NOTE**: Cemu does not support a webcam. (I just copied a raw image from my Wii U into Cemu's sdcard folder.)

# Raw audio format
signed 16-bit PCM, big-endian, mono, 32000 Hz

You can convert it with [audacity](https://www.audacityteam.org)

### Compile
Get latest devkitpro.

```
$ cd samples/make/camtest
$ make
```

# Credits
Thanks to [xhp-creations](https://github.com/xhp-creations) for the header files:
* [camera.h](https://github.com/xhp-creations/libcafe/blob/master/include/wiiu/dynlib/camera.h)
* [mic.h](https://github.com/xhp-creations/libcafe/blob/master/include/wiiu/dynlib/mic.h)

Thanks to [NessieHax](https://github.com/NessieHax)...
* for a [microphone example](https://github.com/devkitPro/wut/pull/214#issuecomment-1140265818)
* for adding [mic.h](https://github.com/devkitPro/wut/blob/master/include/mic/mic.h) to wut
  (Although I'm still using the libcafe version.)

The libcafe headers I have adapted for my wut fork:
* [camera.h](https://github.com/revvv/wut/blob/camtest/include/camera/camera.h)
* [mic.h](https://github.com/revvv/wut/blob/camtest/include/mic/mic.h)
