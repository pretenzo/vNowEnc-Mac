# vNowEnc-Mac
A Hasbro VideoNow application to encode raw video files into VideoNow WAV/VDN format on Macs. The output file will be called "VDN Track 01.wav" - it needs to be burned as an Audio CD with the two filler tracks provided in the source code. There's a limit of approximately 40 minutes of video per disc - because we have to cut it down, we need some empty space on the burned disc so we're not cutting into our data. < br / > Once burned, the disc must be cut down to approx. 108mm to fit in an unmodded VideoNow player. To make a disc image instead, you can also use my Wav2Bin program to generate a .bin/.cue file that can be burned to disc or decoded using PVDTools to ensure it was transcoded properly. There's are also a test video/audio file combination that is known working with this implementation. You can use to ensure everything is working properly or to help debug.

This program is expecting a RAW video formatted 24-bit bitmap with 8-bit audio at 17640Hz. You can convert your input video into the proper format by encoding it using ffmpeg.< br / > 
To encode the video: ffmpeg -i input.mp4 -vf "scale=432:160" -r 18 -f rawvideo -pix_fmt rgb24 video.rgb< br / > 
To encode the audio: ffmpeg -i input.mp4 -af "aresample=17640,volume=8dB" -f wav -ar 17640 -acodec pcm_u8 audio.wav< br / > 

The original version of vNowEnc is brought to you by trevlac (http://www.trevlac.us/vidNowEnc.zip) and additional documentation is available at: https://forum.videohelp.com/threads/203081-VideoNow-Color-%28and-Jr%29-Video-Conversions/page5
