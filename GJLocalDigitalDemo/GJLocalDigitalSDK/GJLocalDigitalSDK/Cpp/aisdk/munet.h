#pragma once
#include "jmat.h"
#include "ncnn/ncnn/net.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdio.h>
#include <vector>


class Mobunet{
    private:
        ncnn::Net unet;
        float mean_vals[3] = {0, 0, 0};
        float norm_vals[3] = {1 / 255.0f, 1 / 255.0f, 1 / 255.0f};
        JMat*   mat_weights = nullptr;
        int initModel(const char* binfn,const char* paramfn,const char* mskfn);
    public:
        int domodel(JMat* pic,JMat* msk,JMat* feat);
        int domodelold(JMat* pic,JMat* msk,JMat* feat);
        int preprocess(JMat* pic,JMat* feat);
        int process(JMat* pic,const int* boxs,JMat* feat);
        int fgprocess(JMat* pic,const int* boxs,JMat* feat,JMat* fg);
        int process2(JMat* pic,const int* boxs,JMat* feat);
        Mobunet(const char* modeldir,const char* modelid);
        Mobunet(const char* fnbin,const char* fnparam,const char* fnmsk);
        ~Mobunet();
};
