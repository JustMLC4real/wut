
# camtest
A simple example to test the Wii U webcam.

Download [camtest-0.1.zip](camtest-0.1.zip)

# Raw image format
The webcam uses the NV12 format.

![screenshot of webcam app showing Mario](samples/make/camtest/selfie-mario.png)

If you'd like to investigate the raw format: [raw image](samples/make/camtest/selfie-mario.nv12)

**NOTE**: cemu does not support a webcam. (I just copied a raw image in its SD folder.)

### Compile
Get latest devkitpro.

```
cd samples/make/camtest
make
```

# Credits
I imported and adapted `camera.h` from [libcafe](https://github.com/xhp-creations/libcafe/blob/master/include/wiiu/dynlib/camera.h)

Thanks @xhp-creations
