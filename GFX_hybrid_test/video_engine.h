#ifndef VIDEO_ENGINE_H_
#define VIDEO_ENGINE_H_

void EVE_initVideoFIFO(void);

void EVE_PlayVideo_Start(void);

void EVE_StreamVideoFile(FIL *videoFile);

void EVE_PlaySplashScreen(void);

void EVE_PlayIntroVideo(void);

#endif  //VIDEO_ENGINE_H_