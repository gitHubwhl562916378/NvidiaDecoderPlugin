/*
 * @Author: your name
 * @Date: 2020-08-03 18:51:10
 * @LastEditTime: 2020-08-06 17:44:52
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \git\NvidiaDecoder\nvidia_decoder.cpp
 */
#include <mutex>
#include "NvDecoder.h"
#include "FFmpegDemuxer.h"
#include "nvidia_decoder.h"

simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();
bool isInitsized = false;
int gcurIndex = 0;
std::mutex gmtx;
std::vector<std::pair<CUcontext,std::string>> m_ctxV;

bool initContext()
{
    gmtx.lock();
    if(!isInitsized){
        ck(cuInit(0));
        int nGpu = 0;
        ck(cuDeviceGetCount(&nGpu));
        for(int i = 0; i < nGpu; i++){
            CUdevice cuDevice = 0;
            ck(cuDeviceGet(&cuDevice, i));
            char szDeviceName[80];
            ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
            CUcontext cuContext = NULL;
            ck(cuCtxCreate(&cuContext, CU_CTX_SCHED_BLOCKING_SYNC, cuDevice));
            LOG(INFO) << "Find Gpu: " << szDeviceName << std::endl;

            CUVIDDECODECAPS videoDecodeCaps = {};
            videoDecodeCaps.eCodecType = cudaVideoCodec_H264;
            videoDecodeCaps.eChromaFormat = cudaVideoChromaFormat_420;
            videoDecodeCaps.nBitDepthMinus8 = 0;
            CUresult resCode;
            if ((resCode = cuvidGetDecoderCaps(&videoDecodeCaps)) == CUDA_SUCCESS){
                LOG(INFO) << "cuvid Decoder Caps nMaxWidth " << videoDecodeCaps.nMaxWidth << " nMaxHeigth " << videoDecodeCaps.nMaxHeight << std::endl;
                if(videoDecodeCaps.nMaxWidth >= 1920 && videoDecodeCaps.nMaxHeight >= 1080){
                    m_ctxV.push_back({cuContext,szDeviceName});
                }
            }else{
                LOG(INFO) << "cuvidGetDecoderCaps failed, Code: " << resCode << std::endl;
            }
        }
        isInitsized = true;
        LOG(INFO) << "nvidia decoder initsized end " << isInitsized << " ptr is " << &isInitsized << std::endl;
    }
    gmtx.unlock();

    if(m_ctxV.empty()){
        return false;
    }

    return true;
}

NvidiaDecoder::~NvidiaDecoder()
{

}

int NvidiaDecoder::fps()
{
    return fps_;
}

void NvidiaDecoder::stop()
{
    isStopedRequested_.store(true);
}

void * NvidiaDecoder::context()
{
    return cur_ctx_;
}

void NvidiaDecoder::decode(const std::string &source, const bool useDeviceFrame, const std::function<void(void *ptr, const int format, const int width, const int height, const std::string &error)> call_back)
{
    if(!initContext()){
        return;
    }
    std::pair<CUcontext,std::string> v = m_ctxV.at(gcurIndex++ % m_ctxV.size());
    cur_ctx_ = (void*)v.first;
    std::cout << "Use Contex in" << v.second << " ctx size " << m_ctxV.size() << std::endl;

    isStopedRequested_.store(false);
    try{
        FFmpegDemuxer demuxer(source.data());
        fps_ = demuxer.GetFps();
        if(fps_ <= 0)
        {
            return;
        }

        NvDecoder decoder(v.first, useDeviceFrame, FFmpeg2NvCodecId(demuxer.GetVideoCodec()), false, false);
        bool bDecodeOutSemiPlanar = false;
        int nVideoBytes = 0, nFrameReturned = 0, nFrame = 0;
        uint8_t *pVideo = NULL, *pFrame;
        do {
            demuxer.Demux(&pVideo, &nVideoBytes);
            nFrameReturned = decoder.Decode(pVideo, nVideoBytes);
            if (!nFrame && nFrameReturned)
                LOG(INFO) << decoder.GetVideoInfo();

            bDecodeOutSemiPlanar = (decoder.GetOutputFormat() == cudaVideoSurfaceFormat_NV12) || (decoder.GetOutputFormat() == cudaVideoSurfaceFormat_P016);

            for (int i = 0; i < nFrameReturned; i++) {
                pFrame = decoder.GetFrame();
                if (decoder.GetBitDepth() == 8) {
                    if (decoder.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444)
                        call_back(pFrame, AV_PIX_FMT_YUV444P, decoder.GetWidth(), decoder.GetHeight(), "");
                else // default assumed NV12
                    call_back(pFrame, AV_PIX_FMT_NV12, decoder.GetWidth(), decoder.GetHeight(), "");
                }else {
                    if (decoder.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444)
                        call_back(pFrame, AV_PIX_FMT_YUV444P16, decoder.GetWidth(), decoder.GetHeight(), "");
                    else // default assumed P016
                        call_back(pFrame, AV_PIX_FMT_P016, decoder.GetWidth(), decoder.GetHeight(), "");
                }
            }
            nFrame += nFrameReturned;
        } while (nVideoBytes && !isStopedRequested_.load());
    }catch(const std::exception &e){
        if(call_back){
            call_back(nullptr, -1, -1, -1, e.what());
        }
    }
}

DecoderApi* createDecoder()
{
    return new NvidiaDecoder;
}