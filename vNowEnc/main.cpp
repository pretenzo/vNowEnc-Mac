//
//  main.cpp
//  vNowEnc
//
//  Created by Lorenzo Bachman on 2025-01-02

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <cstdlib>

// VideoNow format constants
constexpr int VDN_INPUT_WIDTH = 432;
constexpr int VDN_INPUT_HEIGHT = 160;
constexpr int VDN_FRAME_RATE = 18;
constexpr int VDN_AUDIO_SAMPLE_RATE = 17640;
constexpr int VDN_AUDIO_BITS = 8;
constexpr int VDN_FRAME_SIZE = 108 * 160;
constexpr int RGB_FRAME_SIZE = VDN_INPUT_WIDTH * VDN_INPUT_HEIGHT * 3;

// Audio format constants
constexpr int AUDIO_SAMPLES_PER_FRAME = 980;  // From Windows version
constexpr int AUDIO_CHANNELS = 2;             // Stereo
constexpr int AUDIO_BYTES_PER_SAMPLE = 1;     // 8-bit audio = 1 byte
constexpr int AUDIO_FRAME_BUFFER_SIZE = AUDIO_SAMPLES_PER_FRAME * AUDIO_CHANNELS * AUDIO_BYTES_PER_SAMPLE;

// Frame buffers
alignas(16) uint8_t frame[108 * 160];
alignas(16) uint8_t audio[AUDIO_FRAME_BUFFER_SIZE];  // Using correct size and alignment

// Input file handles
FILE* videoFile = nullptr;
FILE* audioFile = nullptr;
long audioDataStart = 0;  // Track start of audio data after WAV header

// Frame Headers
const uint8_t h1[] = { 0x81, 0xe3, 0xe3, 0xc7, 0xc7, 0x81, 0x81, 0xe3, 0xc7 };
const uint8_t h2[] = {
    0x00,0x00,0x02,0x01,0x04,0x02,0x06,0x03,0xFF,
    0x08,0x04,0x0A,0x05,0x0C,0x06,0x0E,0x07,0xFF,
    0x11,0x08,0x13,0x09,0x15,0x0A,0x17,0x0B,0xFF,
    0x19,0x0C,0x1B,0x0D,0x1D,0x0E,0x1F,0x0F,0xFF,
    0x00,0x28,0x02,0x29,0x04,0x2A,0x06,0x2B,0xFF,
    0x08,0x2C,0x0A,0x2D,0x0C,0x2E,0x0E,0x2F,0xFF,
    0x11,0x30,0x13,0x31,0x15,0x32,0x17,0x33,0xFF,
    0x19,0x34,0x1B,0x35,0x1D,0x36,0x1F,0x37,0xFF,
    0x00,0x38,0x02,0x39,0x04,0x3A,0x06,0x3B,0xFF,
    0x08,0x3C,0x0A,0x3D,0x0C,0x3E,0x0E,0x3F,0xFF,
    0x11,0x40,0x13,0x41,0x15,0x42,0x17,0x43,0xFF,
    0x19,0x44,0x1B,0x45,0x1D,0x46,0x1F,0x47,0xFF
};
const uint8_t h3[] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };

struct InputSpec {
    long totalFrames;
    long currentFrame;
};

#pragma pack(push, 1)
struct WAVHeader {
    char riffHeader[4];    // Contains "RIFF"
    uint32_t wavSize;      // Size of wav portion of file
    char waveHeader[4];    // Contains "WAVE"
    char formatHeader[4];  // Contains "fmt "
    uint32_t formatChunkSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char dataHeader[4];    // Contains "data"
    uint32_t dataBytes;    // Number of bytes in data
};
#pragma pack(pop)

InputSpec inputInfo = { 0, 0 };

class BitmapNormalizer {
public:
    static bool NormalizeRGBFile(const char* inputPath, const char* outputPath) {
        FILE* inFile = fopen(inputPath, "rb");
        if (!inFile) {
            std::cout << "Error: Could not open input file for normalization\n";
            return false;
        }

        // Get file size and frame count
        fseek(inFile, 0, SEEK_END);
        long fileSize = ftell(inFile);
        long frameCount = fileSize / RGB_FRAME_SIZE;
        fseek(inFile, 0, SEEK_SET);

        // Create output file
        FILE* outFile = fopen(outputPath, "wb");
        if (!outFile) {
            std::cout << "Error: Could not create normalized output file\n";
            fclose(inFile);
            return false;
        }

        // Allocate buffers
        uint8_t* frameBuffer = new uint8_t[RGB_FRAME_SIZE];
        uint8_t* normalizedFrame = new uint8_t[RGB_FRAME_SIZE];

        std::cout << "Normalizing " << frameCount << " frames...\n";

        // Process each frame
        for (long frame = 0; frame < frameCount; frame++) {
            if (fread(frameBuffer, 1, RGB_FRAME_SIZE, inFile) != RGB_FRAME_SIZE) {
                std::cout << "Error: Failed reading frame " << frame << "\n";
                break;
            }

            // Normalize current frame
            for (int y = 0; y < VDN_INPUT_HEIGHT; y++) {
                for (int x = 0; x < VDN_INPUT_WIDTH; x++) {
                    // Calculate source and destination indices
                    int srcIdx = (y * VDN_INPUT_WIDTH + x) * 3;
                    // Only flip vertically, no horizontal mirroring
                    int dstIdx = ((VDN_INPUT_HEIGHT - 1 - y) * VDN_INPUT_WIDTH + x) * 3;

                    // Swap R and B channels
                    normalizedFrame[dstIdx + 0] = frameBuffer[srcIdx + 2];  // R = B
                    normalizedFrame[dstIdx + 1] = frameBuffer[srcIdx + 1];  // G = G
                    normalizedFrame[dstIdx + 2] = frameBuffer[srcIdx + 0];  // B = R
                }
            }

            // Write normalized frame
            if (fwrite(normalizedFrame, 1, RGB_FRAME_SIZE, outFile) != RGB_FRAME_SIZE) {
                std::cout << "Error: Failed writing normalized frame " << frame << "\n";
                break;
            }

            if (frame % 10 == 0) {
                std::cout << "Normalized frame " << frame << " of " << frameCount << "\r";
                fflush(stdout);
            }
        }

        std::cout << "\nNormalization complete!\n";

        // Cleanup
        delete[] frameBuffer;
        delete[] normalizedFrame;
        fclose(inFile);
        fclose(outFile);
        return true;
    }
};

bool OpenInputFiles(const char* videoPath, const char* wavPath) {
    videoFile = fopen(videoPath, "rb");
    if (!videoFile) {
        std::cout << "Error: Could not open video file\n";
        return false;
    }

    audioFile = fopen(wavPath, "rb");
    if (!audioFile) {
        std::cout << "Error: Could not open audio file\n";
        fclose(videoFile);
        return false;
    }

    WAVHeader header;
    if (fread(&header, sizeof(WAVHeader), 1, audioFile) != 1) {
        std::cout << "Error: Could not read WAV header\n";
        fclose(videoFile);
        fclose(audioFile);
        return false;
    }

    if (memcmp(header.riffHeader, "RIFF", 4) != 0 ||
        memcmp(header.waveHeader, "WAVE", 4) != 0) {
        std::cout << "Error: Invalid WAV file format\n";
        fclose(videoFile);
        fclose(audioFile);
        return false;
    }

    if (header.sampleRate != VDN_AUDIO_SAMPLE_RATE ||
        header.bitsPerSample != VDN_AUDIO_BITS) {
        std::cout << "Error: WAV file must be " << VDN_AUDIO_SAMPLE_RATE << "Hz "
            << VDN_AUDIO_BITS << "-bit\n";
        std::cout << "Current format: " << header.sampleRate << "Hz "
            << header.bitsPerSample << "-bit\n";
        fclose(videoFile);
        fclose(audioFile);
        return false;
    }

    // Store audio data start position
    audioDataStart = sizeof(WAVHeader);

    fseek(videoFile, 0, SEEK_END);
    long videoSize = ftell(videoFile);
    fseek(videoFile, 0, SEEK_SET);

    inputInfo.totalFrames = videoSize / RGB_FRAME_SIZE;
    inputInfo.currentFrame = 0;

    std::cout << "\nInput files opened successfully:\n";
    std::cout << "Video: " << videoPath << " (" << inputInfo.totalFrames << " frames)\n";
    std::cout << "Audio: " << wavPath << " (" << header.sampleRate << "Hz, "
        << header.bitsPerSample << "-bit)\n\n";

    return true;
}

// [Video processing code remains the same as it works well]
bool GetVideo(int frameNum, uint8_t* buf) {
    uint8_t* rgbFrame = new uint8_t[RGB_FRAME_SIZE];
    uint8_t r0, r1, g0, g1, b0, b1;
    int h = 0, w = 0;

    std::cout << "\nVideo: Seeking to position " << frameNum * RGB_FRAME_SIZE << "\n";

    fseek(videoFile, frameNum * RGB_FRAME_SIZE, SEEK_SET);
    size_t bytesRead = fread(rgbFrame, 1, RGB_FRAME_SIZE, videoFile);
    if (bytesRead != RGB_FRAME_SIZE) {
        delete[] rgbFrame;
        std::cout << "Video read error: Expected " << RGB_FRAME_SIZE << " bytes, got " << bytesRead << "\n";
        return false;
    }

    uint8_t* pRGB = rgbFrame + (432 * 159 * 3);

    h = 160;
    do {
        if (h % 2 == 0) {  // Even lines
            pRGB += 3;  // Skip first pixel on RGB lines

            w = 36;
            do {
                pRGB += 2;  // Go to R component
                r0 = (*pRGB & 0xF0) >> 4;

                pRGB += 5;
                g0 = (*pRGB & 0xF0);

                pRGB += 5;
                b0 = (*pRGB & 0xF0) >> 4;

                pRGB += 8;
                r1 = (*pRGB & 0xF0);

                pRGB += 5;
                g1 = (*pRGB & 0xF0) >> 4;

                pRGB += 5;
                b1 = *pRGB & 0xF0;

                pRGB += 6;

                *buf++ = r0 | g0;
                *buf++ = b0 | r1;
                *buf++ = g1 | b1;

            } while (--w);

            pRGB -= 3;

        } else {  // Odd lines
            w = 36;
            do {
                pRGB++;
                g0 = (*pRGB & 0xF0) >> 4;

                pRGB += 5;
                b0 = (*pRGB & 0xF0);

                pRGB += 8;
                r0 = (*pRGB & 0xF0) >> 4;

                pRGB += 5;
                g1 = (*pRGB & 0xF0);

                pRGB += 5;
                b1 = (*pRGB & 0xF0) >> 4;

                pRGB += 8;
                r1 = *pRGB & 0xF0;

                pRGB += 4;

                *buf++ = g0 | b0;
                *buf++ = r0 | g1;
                *buf++ = b1 | r1;

            } while (--w);
        }

        pRGB -= (2 * 432 * 3);  // Move back 2 lines
    } while (--h);

    delete[] rgbFrame;
    std::cout << "Successfully read and converted video frame " << frameNum << "\n";
    return true;
}

// Updated audio processing code
bool GetAudio(int frameNum, uint8_t* buf) {
    long audioOffset = audioDataStart + (frameNum * AUDIO_SAMPLES_PER_FRAME * AUDIO_CHANNELS);
    std::cout << "Audio: Seeking to position " << audioOffset << "\n";
    
    if (fseek(audioFile, audioOffset, SEEK_SET) != 0) {
        std::cout << "Audio seek failed to offset " << audioOffset << std::endl;
        return false;
    }

    // Read stereo samples
    size_t bytesRead = 0;
    for (int i = 0; i < AUDIO_SAMPLES_PER_FRAME && !feof(audioFile); i++) {
        uint8_t stereoSample[2];  // Left and right channel
        size_t read = fread(stereoSample, 1, 2, audioFile);
        if (read != 2) {
            std::cout << "Failed reading stereo sample at " << i << std::endl;
            return false;
        }
        buf[bytesRead++] = stereoSample[0];
        buf[bytesRead++] = stereoSample[1];
    }

    std::cout << "Successfully read audio frame " << frameNum << std::endl;
    return (bytesRead == AUDIO_SAMPLES_PER_FRAME * AUDIO_CHANNELS);
}

bool writeFrames(int frameNum, FILE* fp) {
    const uint8_t* h2p = h2;
    uint8_t* aud = audio;
    uint8_t* vid = frame;

    if (GetVideo(frameNum, frame)) {
        if (GetAudio(frameNum, audio)) {
            // Reset pointers for writing
            aud = audio;
            
            // Frame Header 1 with Audio
            for (int i = 0; i < 24; i++) {
                if (fwrite(h1, 9, 1, fp) != 1) {
                    std::cout << "Failed writing header 1" << std::endl;
                    return false;
                }
                if (fwrite(aud++, 1, 1, fp) != 1) {
                    std::cout << "Failed writing audio in header 1" << std::endl;
                    return false;
                }
            }

            // Frame Header 2 with Audio
            for (int i = 0; i < 12; i++) {
                if (fwrite(h2p, 9, 1, fp) != 1) {
                    std::cout << "Failed writing header 2" << std::endl;
                    return false;
                }
                h2p = h2p + 9;
                if (fwrite(aud++, 1, 1, fp) != 1) {
                    std::cout << "Failed writing audio in header 2" << std::endl;
                    return false;
                }
            }

            // Frame Header 3 with Audio
            for (int i = 0; i < 4; i++) {
                if (fwrite(h3, 9, 1, fp) != 1) {
                    std::cout << "Failed writing header 3" << std::endl;
                    return false;
                }
                if (fwrite(aud++, 1, 1, fp) != 1) {
                    std::cout << "Failed writing audio in header 3" << std::endl;
                    return false;
                }
            }

            // Frame Data with Audio
            for (int i = 0; i < (160 * 12); i++) {
                if (fwrite(vid, 9, 1, fp) != 1) {
                    std::cout << "Failed writing video data" << std::endl;
                    return false;
                }
                vid = vid + 9;
                if (fwrite(aud++, 1, 1, fp) != 1) {
                    std::cout << "Failed writing audio data" << std::endl;
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

bool writeHeader(FILE* fp) {
    uint8_t dummyHeader[560] = {0};

    // Setup RIFF header to match VideoNow format
    struct WaveFormatEx {
        uint16_t wFormatTag;
        uint16_t nChannels;
        uint32_t nSamplesPerSec;
        uint32_t nAvgBytesPerSec;
        uint16_t nBlockAlign;
        uint16_t wBitsPerSample;
        uint16_t cbSize;
    } wf = {
        1,                          // wFormatTag (PCM)
        AUDIO_CHANNELS,             // nChannels (2 for stereo)
        VDN_AUDIO_SAMPLE_RATE,      // nSamplesPerSec (17640)
        VDN_AUDIO_SAMPLE_RATE * AUDIO_CHANNELS * (VDN_AUDIO_BITS/8), // nAvgBytesPerSec
        AUDIO_CHANNELS * (VDN_AUDIO_BITS/8), // nBlockAlign
        VDN_AUDIO_BITS,            // wBitsPerSample (8)
        0                          // cbSize
    };

    fwrite("RIFF\0\0\0\0WAVEfmt \20\0\0\0", 20, 1, fp);
    fwrite(&wf, 16, 1, fp);
    fwrite("data\0\0\0\0", 8, 1, fp);
    fwrite(dummyHeader, 560, 1, fp);

    return true;
}

void finalizeFile(FILE* fp) {
    uint32_t p;
    fflush(fp);
    p = ftell(fp);
    
    // Write RIFF chunk size
    fseek(fp, 4, SEEK_SET);
    uint32_t riffSize = p - 8;
    fwrite(&riffSize, sizeof(riffSize), 1, fp);
    
    // Write data chunk size
    fseek(fp, 40, SEEK_SET);
    uint32_t dataSize = p - 44;
    fwrite(&dataSize, sizeof(dataSize), 1, fp);
    
    fflush(fp);
    fclose(fp);
}

void CloseInputFiles() {
    if (videoFile) fclose(videoFile);
    if (audioFile) fclose(audioFile);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: vidNowEnc <video.rgb> <audio.wav> [frames_per_file]\n";
        return 1;
    }

    char normalizedPath[256];
    snprintf(normalizedPath, sizeof(normalizedPath), "%s.normalized", argv[1]);
    
    std::cout << "Pre-processing video file...\n";
    if (!BitmapNormalizer::NormalizeRGBFile(argv[1], normalizedPath)) {
        std::cout << "Failed to normalize input video\n";
        return 1;
    }

    char outFile[256];
    int fileCnt = 1;
    int framesPerFile = 0;
    time_t startTime, endTime;

    time(&startTime);

    if (argc > 3) framesPerFile = atoi(argv[3]);

    if (OpenInputFiles(normalizedPath, argv[2])) {
        if (framesPerFile == 0) framesPerFile = inputInfo.totalFrames;
        int iFramesThisFile;
        int iProcessedFrameCnt = 0;

        do {
            snprintf(outFile, sizeof(outFile), "VDN Track %02d.wav", fileCnt++);

            FILE* fp = fopen(outFile, "wb");
            if (!fp) {
                std::cout << "\nCould not open output file: " << outFile << "\n\n";
                break;
            }

            writeHeader(fp);

            iFramesThisFile = framesPerFile;
            do {
                std::cout << "Writing frame: " << iProcessedFrameCnt + 1
                     << " of " << inputInfo.totalFrames << " frames\r";
                if (!writeFrames(iProcessedFrameCnt++, fp)) {
                    std::cout << "\nError processing frame " << iProcessedFrameCnt << "\n";
                    break;
                }
            } while (--iFramesThisFile && (inputInfo.totalFrames > iProcessedFrameCnt));

            finalizeFile(fp);

        } while (inputInfo.totalFrames > iProcessedFrameCnt);

        CloseInputFiles();

        time(&endTime);
        std::cout << "\nDone in " << difftime(endTime, startTime) << " seconds\n";
    }

    // Clean up temporary file
    remove(normalizedPath);
    return 0;
}
