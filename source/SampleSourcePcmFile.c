//
//  SampleSourcePcmFile.c
//  MrsWatson
//
//  Created by Nik Reiman on 1/2/12.
//  Copyright (c) 2012 Teragon Audio. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SampleSourcePcmFile.h"

static boolean _openSampleSourcePcmFile(void* sampleSourcePtr) {
  SampleSource sampleSource = sampleSourcePtr;

  SampleSourcePcmFileData extraData = sampleSource->extraData;
  extraData->dataBufferNumItems = 0;
  extraData->fileHandle = fopen(sampleSource->sourceName->data, "rb");
  if(extraData == NULL) {
    // TODO: Error
    return false;
  }

  return true;
}

static void _convertPcmDataToSampleBuffer(const short* inPcmSamples, SampleBuffer sampleBuffer, const long numInterlacedSamples) {
  for(long interlacedIndex = 0, deinterlacedIndex = 0; interlacedIndex < numInterlacedSamples; interlacedIndex++) {
    for(int channelIndex = 0; channelIndex < sampleBuffer->numChannels; channelIndex++) {
      Sample convertedSample = (Sample)inPcmSamples[interlacedIndex] / 32768.0f;
      // Apply brickwall limiter to prevent clipping
      if(convertedSample > 1.0f) {
        convertedSample = 1.0f;
      }
      else if(convertedSample < -1.0f) {
        convertedSample = -1.0f;
      }
      sampleBuffer->samples[channelIndex][deinterlacedIndex] = convertedSample;
    }
    deinterlacedIndex++;
  }
}

static boolean _readBlockPcmFile(void* sampleSourcePtr, SampleBuffer sampleBuffer) {
  SampleSource sampleSource = sampleSourcePtr;
  SampleSourcePcmFileData extraData = sampleSource->extraData;
  if(extraData->dataBufferNumItems == 0) {
    extraData->dataBufferNumItems = (size_t)(sampleBuffer->numChannels * sampleBuffer->blocksize);
    extraData->interlacedPcmDataBuffer = malloc(sizeof(short) * extraData->dataBufferNumItems);
  }

  // Clear the PCM data buffer, or else the last block will have dirty samples in the end
  memset(extraData->interlacedPcmDataBuffer, 0, sizeof(short) * extraData->dataBufferNumItems);

  boolean result = true;
  size_t bytesRead = fread(extraData->interlacedPcmDataBuffer, sizeof(short), extraData->dataBufferNumItems, extraData->fileHandle);
  if(bytesRead < extraData->dataBufferNumItems * sizeof(short)) {
    // TODO: End of file reached -- do something special?
    result = false;
  }

  _convertPcmDataToSampleBuffer(extraData->interlacedPcmDataBuffer, sampleBuffer, extraData->dataBufferNumItems);
  return result;
}

static void _freeInputSourceDataPcmFile(void* sampleSourceDataPtr) {
  SampleSourcePcmFileData extraData = sampleSourceDataPtr;
  free(extraData->interlacedPcmDataBuffer);
  if(extraData->fileHandle != NULL) {
    fclose(extraData->fileHandle);
  }
  free(extraData);
}

SampleSource newSampleSourcePcmFile(const CharString sampleSourceName) {
  SampleSource sampleSource = malloc(sizeof(SampleSourceMembers));

  sampleSource->sampleSourceType = SAMPLE_SOURCE_TYPE_PCM_FILE;
  sampleSource->sourceName = newCharString();
  copyCharStrings(sampleSource->sourceName, sampleSourceName);
  // TODO: Need a way to pass in channels, bitrate, sample rate
  sampleSource->numChannels = 2;
  sampleSource->sampleRate = 44100.0f;

  sampleSource->openSampleSource = _openSampleSourcePcmFile;
  sampleSource->readSampleBlock = _readBlockPcmFile;
  sampleSource->freeSampleSourceData = _freeInputSourceDataPcmFile;

  SampleSourcePcmFileData extraData = malloc(sizeof(SampleSourcePcmFileDataMembers));
  extraData->fileHandle = NULL;
  extraData->dataBufferNumItems = 0;
  extraData->interlacedPcmDataBuffer = NULL;
  sampleSource->extraData = extraData;

  return sampleSource;
}