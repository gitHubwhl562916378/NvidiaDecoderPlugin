/*
 * @Author: your name
 * @Date: 2020-08-03 19:00:06
 * @LastEditTime: 2021-03-03 17:23:47
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \git\NvidiaDecoder\nvidia_decoer_api.h
 */
#pragma once

#ifdef linux
#  define DECODER_EXPORT __attribute__((visibility("default")))
#else
#  define DECODER_EXPORT __declspec(dllexport)
#endif
#include <string>
#include <functional>

class DECODER_EXPORT DecoderApi
{
public:
    virtual ~DecoderApi(){};
    virtual int fps() = 0;
    virtual void stop() = 0;
    virtual void * context() = 0;
    virtual int memCpy(void *dst, const void *src, size_t count, int kind) = 0;
    virtual void decode(const std::string &source, const bool useDeviceFrame, const std::function<void(void *ptr, const int format, const int width, const int height, const std::string &error)> call_back) = 0;
};

extern "C"
{
DECODER_EXPORT DecoderApi* createDecoder();
}