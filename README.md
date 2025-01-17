# VideoNow Color Video Encoder for Mac
An application to create custom videos for Hasbro's VideoNow Color players. It encodes raw video files into VideoNow WAV/VDN format, on Mac! This implementation removes the need for Avisynth and Virtualdub. Instead, it utilizes ffmpeg to handle the transcoding tasks to automate & simplify the conversion process.

# Usage 
This program is expecting a RAW RGB video format (24-bit bitmap) with 8-bit audio at 17640Hz. You can convert your input video into the proper format by encoding it using ffmpeg.<br/> <br/>
To encode the video: ```ffmpeg -i input.mp4 -vf "scale=432:160" -r 18 -f rawvideo -pix_fmt rgb24 video.rgb```<br/> 
To encode the audio: ```ffmpeg -i input.mp4 -af "aresample=17640,volume=8dB" -f wav -ar 17640 -acodec pcm_u8 audio.wav```<br/> 

# Creating A Custom VideoNow CD
The output file will be called "VDN Track 01.wav" - it needs to be burned as an Audio CD with the two filler tracks provided in the source code (VDN Track 00 and VDN Track 99). You can include several tracks by transcoding them and then naming them "VDN Track 01" and "VDN Track 02" etc. There's a limit of approximately 40 minutes of video per disc - because we have to cut it down, we need some empty space on the burned disc so we're not cutting into our data. The size of my Audio CDs usually doesn't exceed 450MB or so, just to err on the side of caution <br/><br/>Once burned, the disc must be cut down to approx. 108mm to fit in an unmodded VideoNow player.<br/><br/>
The structure of the Audio CD should be as follows:
```
Audio CD
|
+ -- VDN Track 00 (First)
|
+ -- VDN Track 01
|
+ -- VDN Track 02, 03, 04, 05, etc... (if applicable)
|
+ -- VDN Track 99 (Last)
```


# Making A Disc Image
To make a disc image instead, you can also use my Wav2Bin program to generate a .bin/.cue file that can be burned to disc or decoded using PVDTools to ensure it was transcoded properly, i.e. <br/><br/>```./Wav2Bin myFilename.bin myFilename.cue "VDN Track 00 (First).wav" "VDN Track 01.wav" "VDN Track 99 (Blank).wav"``` <br/><br/> There's are also a test video/audio file combination that is known working with this implementation. You can use to ensure everything is working properly or to help debug. 

# Thank You
The original version of vidNowEnc is brought to you by trevlac (http://www.trevlac.us/vidNowEnc.zip) and additional documentation is available at: https://forum.videohelp.com/threads/203081-VideoNow-Color-%28and-Jr%29-Video-Conversions/page5
