/*
 * @Author: your name
 * @Date: 2020-08-03 18:51:01
 * @LastEditTime: 2020-08-05 15:39:09
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \git\NvidiaDecoder\nvidia_decoder.h
 */
#pragma once
#include <atomic>
#include "nvidia_decoer_api.h"

class NvidiaDecoder : public DecoderApi
{
public:
    ~NvidiaDecoder() override;
    void stop() override;
    void * context();
    int fps() override;
    void decode(const std::string &source, const bool useDeviceFrame, const std::function<void(void *ptr, const int format, const int width, const int height, const std::string &error)> call_back) override;

private:
    int fps_ = 0;
    std::atomic_bool isStopedRequested_;
    void *cur_ctx_ {nullptr};
};