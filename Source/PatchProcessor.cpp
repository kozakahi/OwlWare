#include "PatchProcessor.h"
#include "PatchRegistry.h"
#include "MemoryBuffer.hpp"
#include "device.h"
#include "basicmaths.h"
#include <string.h>

#include "MidiController.h"
#include "OpenWareMidiControl.h"

#ifdef EXTERNAL_SRAM
#define BUFFER_LENGTH 262144
static float extbuffer[BUFFER_LENGTH] EXT;
#else /* no external SRAM */
#define BUFFER_LENGTH 16384
static float extbuffer[BUFFER_LENGTH];
#endif
static int extpos = 0;

PatchProcessor::PatchProcessor() 
  : patch(NULL), bufferCount(0) {}

PatchProcessor::~PatchProcessor(){
  clear();
}

void PatchProcessor::clear(){
  for(int i=0; i<bufferCount; ++i){
    extpos -= buffers[i]->getSize() * buffers[i]->getChannels();
    delete buffers[i];
  }
  delete patch;
  patch = NULL;
}

void PatchProcessor::setPatch(uint8_t patchIndex){
  clear();
  memset(parameterNames, 0, sizeof(parameterNames));
  bufferCount = 0;
  index = patchIndex;
  patch = registry.create(index);
}

void PatchProcessor::registerParameter(PatchParameterId pid, const char* name, const char* description){
  if(pid < NOF_ADC_VALUES)
    parameterNames[pid] = name;
}

const char* PatchProcessor::getParameterName(PatchParameterId pid){
  if(pid < NOF_ADC_VALUES)
    return parameterNames[pid];
  return NULL;
}

AudioBuffer* PatchProcessor::createMemoryBuffer(int channels, int size){
  // assert_param(bufferCount < MAX_BUFFERS_PER_PATCH];
  float* buffer = extbuffer+extpos;
  size = min(size, BUFFER_LENGTH-extpos);
  extpos += size;
  size /= channels;
  MemoryBuffer* buf = new MemoryBuffer(buffer, channels, size);
  buffers[bufferCount++] = buf;
  buf->clear();
  return buf;
}

float PatchProcessor::getParameterValue(PatchParameterId pid){
  return parameterValues[pid]/4096.0;
}

__attribute__ ((section (".coderam")))
void PatchProcessor::setParameterValues(uint16_t *params){
  /* Implements an exponential moving average (leaky integrator) to smooth ADC values
   * y(n) = (1-alpha)*y(n-1) + alpha*y(n)
   * with alpha=0.5, fs=48k, bs=128, then w0 ~= 18hz
   */
  for(int i=0; i<NOF_ADC_VALUES; ++i)
    parameterValues[i] = (parameterValues[i] + params[i]) >> 1;
  // memcpy(parameterValues, params, sizeof parameterValues);
}

// void PatchProcessor::process(AudioBuffer& buffer){
//   patch->processAudio(buffer);
// }

// int PatchProcessor::getBlockSize(){
//   return patch->getBlockSize();
// }

// double PatchProcessor::getSampleRate(){
//   return patch->getSampleRate();
// }
