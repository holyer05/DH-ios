#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory>
#include <vector>
#include <string.h>
#include <mutex>
#include<iostream>
using namespace std;
//#include "NumCpp.hpp"
#include "gjdigit.h"
#include "jmat.h"
#include "wenet.h"
#include "munet.h"
#include "malpha.h"
#include "blendgram.h"
#include  "wavcache.h"
#include "aesmain.h"
#include "netwav.h"

/*
	•	ai_wenet：语音识别模型 Wenet 对象，处理音频的功能。
	•	ai_munet：图像识别模型 Mobunet 对象，用于处理图像。
	•	ai_malpha：图像处理模型 MAlpha 对象，处理图像相关的功能。
	•	lock_munet：一个互斥锁，用于线程安全地访问 ai_munet（图像识别模型）。
	•	lock_wenet：一个互斥锁，用于线程安全地访问 ai_wenet（语音识别模型）。
	•	bnf_cache：音频特征数据缓存。
	•	cnt_wenet：语音识别返回的特征块数量。
	•	inx_wenet：当前正在处理的语音索引。
	•	mat_gpg：一个图像矩阵，用于图像相关操作。
*/
struct gjdigit_s{
    Wenet       *ai_wenet;
    Mobunet     *ai_munet;
    MAlpha      *ai_malpha;
    //JMat        *mat_wenet;
    std::mutex  *lock_munet;
    std::mutex  *lock_wenet;
    MBnfCache   *bnf_cache;
    int         cnt_wenet;
    int         inx_wenet;
    JMat*       mat_gpg;
};

//分配内存给 gjdigit_t 结构体，并初始化其部分成员
int gjdigit_alloc(gjdigit_t** pdg){
    gjdigit_t* dg = (gjdigit_t*)malloc(sizeof(gjdigit_t));
    memset(dg,0,sizeof(gjdigit_t));
    dg->lock_munet = new std::mutex();
    dg->lock_wenet = new std::mutex();
    dg->bnf_cache = new MBnfCache();
    *pdg = dg;
    return 0;
}

//初始化语音识别模型 Wenet，并加载指定的模型文件。
int gjdigit_initWenet(gjdigit_t* dg,char* fnwenet){
    dg->lock_wenet->lock();
    if(dg->ai_wenet){
        delete dg->ai_wenet;
        dg->ai_wenet = NULL;
    }
    dg->ai_wenet = new Wenet(fnwenet);
    dg->lock_wenet->unlock();
    return 0;
}

//初始化图像处理模型 MAlpha，并加载指定的参数和二进制文件。
int gjdigit_initMalpha(gjdigit_t* dg,char* fnparam,char* fnbin){
    if(1)return 0;
    if(dg->ai_malpha){
        delete dg->ai_malpha;
        dg->ai_malpha = NULL;
    }
    std::string fb(fnbin);
    std::string fp(fnparam);
    dg->ai_malpha = new MAlpha(fb.c_str(),fp.c_str());
    return 0;
}

//初始化图像识别模型 Mobunet，并加载指定的参数、二进制和掩膜文件。
int gjdigit_initMunet(gjdigit_t* dg,char* fnparam,char* fnbin,char* fnmsk){
    dg->lock_munet->lock();
    if(dg->ai_munet){
        delete dg->ai_munet;
        dg->ai_munet = NULL;
    }
    dg->ai_munet = new Mobunet(fnbin,fnparam,fnmsk);
    dg->lock_munet->unlock();
    return 0;
}

// static int calcinx(gjdigit_t* dg, KWav* wavmat,int index){
//     float* pwav = NULL;
//     float* pmfcc = NULL;
//     float* pbnf = NULL;
//     int melcnt = 0;
//     int bnfcnt = 0;
//     int rst = wavmat->calcbuf(index, &pwav,&pmfcc,&melcnt);
//     //LOGE(TAG,"===tooken calcinx %d index %d \n",index,rst);
//     if(rst == index){
//         dg->ai_wenet->calcmfcc(pwav,pmfcc);
//     //double t0 = ncnn::get_current_time();
//     JMat* feat_mat = nullptr;
//     int n_feat = 0;
//     MBnfCache* bnf_cache;
//     wavmat->get_wenet_buffers(&feat_mat, &n_feat, &bnf_cache);
//         dg->ai_wenet->calcbnf(feat_mat, n_feat, bnf_cache);
//     //double t1 = ncnn::get_current_time();
//     //float dist = t1-t0;
//         //dumpfloat(pbnf,10);
//         wavmat->finishone(index);
//     }
//     return 0;
// }

//这个函数 calcinx 的作用是处理音频文件中的一个索引，计算该索引对应的音频特征，并将这些特征传递给语音识别模型进行处理。
//它使用了音频处理库中的 KWav 类来提取音频特征，并将其传递给 ai_wenet（一个语音识别模型）。
static int calcinx(gjdigit_t* dg,KWav* wavmat, int index) {
    float* pwav = NULL;
    float* pmfcc = NULL;
    std::vector<JMat*>* output_vector = NULL;
    int melcnt = 0;
    int rst = wavmat->calcbuf(index, &pwav, &pmfcc, &melcnt);
    if (rst == index) {
        dg->ai_wenet->calcmfcc(pwav, pmfcc);
        //double t0 = ncnn::get_current_time();
        // dg->ai_wenet->calcbnf(pmfcc, melcnt, output_vector);
        //double t1 = ncnn::get_current_time();
        //float dist = t1-t0;
            //dumpfloat(pbnf,10);
        wavmat->finishone(index);
    }
    return 0;
}

static int calcall(gjdigit_t* dg, KWav* wavmat){
    int rst =wavmat->readyall();
    int cnt = 0;
    while(cnt<1000){
        rst = wavmat->isready();
        if(!rst)break;
        calcinx(dg,wavmat,rst);
    }
    // wenet feat
    JMat* feat_mat = nullptr;
    int n_feat = 0;
    MBnfCache* bnf_cache;
    wavmat->get_wenet_buffers(&feat_mat, &n_feat, &bnf_cache);
    return dg->ai_wenet->calcbnf(feat_mat, n_feat, bnf_cache);
}

/*
	作用：处理一个音频文件，计算其音频特征并返回识别结果。
	细节：
	•	使用 KWav 类加载音频文件，并计算音频特征。
	•	调用语音识别模型 ai_wenet 进行处理，返回音频特征块数量。
    */
int gjdigit_onewav(gjdigit_t* dg,const char* wavfn,float duration){
    cout<<"onewav";
    if(!dg->ai_wenet)return -999;
    /*
       if(dg->mat_wenet){
       delete dg->mat_wenet;
       dg->mat_wenet = NULL;
       }
       */

    int  rst = 0;
    KWav* net_wavmat = new KWav(wavfn,dg->bnf_cache,duration);
    if(net_wavmat->duration()>0){
       calcall(dg,net_wavmat); //
        rst =  net_wavmat->bnfblocks();//net_curl->checked();
        dg->cnt_wenet = rst;
        dg->inx_wenet = 0;
    }else{
        rst = dg->ai_wenet->nextwav(wavfn,dg->bnf_cache,duration);
        dg->bnf_cache->debug();
        dg->cnt_wenet = rst;
        dg->inx_wenet = 0;
    }
    delete net_wavmat;
    return rst;
}

//用于处理图像文件，并将图像特征与特定索引的数据结合起来，传递给 ai_munet 模型进行图像识别。
int gjdigit_picrst(gjdigit_t* dg,const char* picfn,int* box,int index){
    if(!dg->ai_munet)return -999;
    //if(!dg->mat_wenet)return -1;
    if(index<0)return -2;
    if(index>=dg->cnt_wenet)return -3;

    /*
       float* pwenet =  dg->ai_wenet->nextbnf(dg->mat_wenet,index);
       if(!pwenet){
       return -11;
       }
    //JMat feat(256, 20, pwenet, 1);
    */
    JMat* feat = dg->bnf_cache->inxBuf(index);

    std::string picfile(picfn);
    JMat* mat_pic = new JMat(picfile,1);
    int* arr = mat_pic->tagarr();
    arr[10] = box[0];
    arr[11] = box[1];
    arr[12] = box[2];
    arr[13] = box[3];
    dg->ai_munet->process(mat_pic, arr, feat);
    delete mat_pic;
    return 0;
}

//函数用于处理图像数据，并将其与音频特征结合，传递给 ai_munet 模型进行图像处理。该函数与前面的 gjdigit_picrst 类似，主要区别在于它直接使用 uint8_t* buf（图像的原始数据）来创建图像矩阵 JMat，而不是从文件路径加载图像
int gjdigit_matrst(gjdigit_t* dg,uint8_t* buf,int width,int height,int* box,int index){
    if(!dg->ai_munet)return -999;
    //if(!dg->mat_wenet)return -1;
    if(index<0)return -2;
    if(index>=dg->cnt_wenet)return -3;
    /*
       float* pwenet = dg->ai_wenet->nextbnf(dg->mat_wenet,index);
       if(!pwenet){
       return -11;
       }
       JMat feat(256, 20, pwenet, 1);
       */
    JMat* feat = dg->bnf_cache->inxBuf(index);

    JMat* mat_pic = new JMat(width,height,buf);
    /*
       int* arr = mat_pic->tagarr();
       arr[10] = box[0];
       arr[11] = box[1];
       arr[12] = box[2];
       arr[13] = box[3];
       */
    dg->lock_munet->lock();
    dg->ai_munet->process(mat_pic, box, feat);
    dg->lock_munet->unlock();
    delete mat_pic;
    delete feat;
    return 0;
}

/*
gjdigit_simprst 函数的作用是处理图像数据，并通过模型执行处理，最后返回音频特征。在这段代码中，图像数据和音频特征的交互主要体现在 MWorkMat 和 ai_munet->domodel() 调用过程中。具体来说，函数实现了图像处理与音频特征计算的结合，使用图像处理模型 ai_munet 和相应的特征缓存来进行模型的计算。
*/
int gjdigit_simprst(gjdigit_t* dg,uint8_t* bpic,int width,int height,int* box,int index){
    if(!dg->ai_munet)return -999;
    //if(!dg->mat_wenet)return -1;
    if(index<0)return -2;
    if(index>=dg->cnt_wenet)return -3;
    dg->bnf_cache->debug();
    JMat* feat = dg->bnf_cache->inxBuf(index);
    dg->bnf_cache->debug();
    //JMat* feat = dg->bnf_cache->inxBuf(0);

    JMat* mat_pic = new JMat(width,height,bpic);
    //创建 MWorkMat 对象，它用于处理图像和边界框数据。premunet() 方法用于进行预处理。
    MWorkMat wmat(mat_pic,NULL,box);
    
    wmat.premunet();
    JMat* mpic;
    JMat* mmsk;
    //调用 munet() 方法进行图像的处理，输出 mpic（处理后的图像）和 mmsk（掩膜）。
    wmat.munet(&mpic,&mmsk);
    dg->lock_munet->lock();
    dg->ai_munet->domodel(mpic, mmsk, feat);
    dg->lock_munet->unlock();
    wmat.finmunet(mat_pic);
    delete mat_pic;
    delete feat;
    return 0;
}

//gjdigit_maskrst 函数主要用于处理图像和掩膜数据，并通过模型进行计算。它将图像数据、掩膜以及其他输入（如边框）传递给图像处理模型 ai_munet，然后返回处理后的结果。这个函数的处理流程包括了图像的预处理、掩膜应用、模型计算以及结果的后处理。

int gjdigit_maskrst(gjdigit_t* dg,uint8_t* bpic,int width,int height,int* box,uint8_t* bmsk,uint8_t* bfg,uint8_t* bbg,int index){
    if(!dg->ai_munet)return -999;
    //if(!dg->mat_wenet)return -1;
    if(index<0)return -2;
    if(index>=dg->cnt_wenet)return -3;
    /*
       float* pwenet = dg->ai_wenet->nextbnf(dg->mat_wenet,index);
       if(!pwenet){
       return -11;
       }
       JMat feat(256, 20, pwenet, 1);
       */
    dg->bnf_cache->debug();
    JMat* feat = dg->bnf_cache->inxBuf(index);
    dg->bnf_cache->debug();
    //JMat* feat = dg->bnf_cache->inxBuf(0);

    JMat* mat_pic = new JMat(width,height,bpic);
    JMat* mat_msk = new JMat(width,height,bmsk);
    JMat* mat_fg = new JMat(width,height,bfg);
    //int* arr = mat_pic->tagarr();
    //arr[10] = box[0];
    //arr[11] = box[1];
    //arr[12] = box[2];
    //arr[13] = box[3];
    MWorkMat wmat(mat_pic,mat_msk,box);
    wmat.premunet();
    JMat* mpic;
    JMat* mmsk;
    wmat.munet(&mpic,&mmsk);
    //JMat realb("real_B.bmp",1);
    //JMat maskb("mask_B.bmp",1);
    dg->lock_munet->lock();
    dg->ai_munet->domodel(mpic, mmsk, feat);
    dg->lock_munet->unlock();
    //dg->ai_munet->domodel(&realb, &maskb, &feat);
    wmat.finmunet(mat_fg);
    //printf("===aaa\n");
    //mat_fg->show("aaa");
    //printf("===bfg %p mat %p\n",bfg,mat_fg->data());
    //cv::waitKey(0);
    /*
       wmat.prealpha();
       JMat* mreal;
       JMat* mimg;
       JMat* mpha;
       wmat.alpha(&mreal,&mimg,&mpha);
       dg->ai_malpha->doModel(mreal,mimg,mpha);
       wmat.finalpha();
       */
    //mat_msk->show("msk");
    //mat_pic->show("picaaa");
    if(bbg) BlendGramAlpha3(bbg,bmsk,bfg,width,height);
    //mat_pic->show("picbbb");
    //cv::waitKey(0);
    delete mat_pic;
    delete mat_msk;
    delete mat_fg;
    delete feat;
    return 0;
}

int gjdigit_free(gjdigit_t** pdg){
    if(!pdg)return -1;
    gjdigit_t* dg = *pdg;
    if(dg->lock_munet){
        dg->lock_munet->lock();
        dg->lock_munet->unlock();
        delete dg->lock_munet;
    }
    if(dg->ai_wenet){
        delete dg->ai_wenet;
        dg->ai_wenet = NULL;
    }
    if(dg->ai_munet){
        delete dg->ai_munet;
        dg->ai_munet = NULL;
    }
    if(dg->bnf_cache){
        delete dg->bnf_cache;
        dg->bnf_cache = NULL;
    }
    if(dg->lock_wenet){
        delete dg->lock_wenet;
        dg->lock_wenet = NULL;
    }
    if(dg->mat_gpg){
        delete dg->mat_gpg;
        dg->mat_gpg = NULL;
    }
    free(dg);
    *pdg = NULL;
    return 0;
}

int gjdigit_test(gjdigit_t* dg){
    dg->cnt_wenet = 10;
    dg->inx_wenet = 0;
    return 2;
}

int gjdigit_processmd5(gjdigit_t* dg,int enc,const char* infn,const char* outfn){
    //
    std::string s_in(infn);
    std::string s_out(outfn);
    int rst = mainenc(enc,(char*)s_in.c_str(),(char*)s_out.c_str());
    return rst;
}

int gjdigit_startgpg(gjdigit_t* dg,const char* infn,const char* outfn){
    std::string s_in(infn);
    std::string s_out(outfn);
    if(!dg->mat_gpg)dg->mat_gpg = new JMat();
    int rst = dg->mat_gpg->loadjpg(s_in);
    if(rst)return rst;
    rst = dg->mat_gpg->savegpg(s_out);
    return rst;
}

int gjdigit_stopgpg(gjdigit_t* dg){
    if(dg->mat_gpg){
        delete dg->mat_gpg;
        dg->mat_gpg = NULL;
    }
    return 0;
}

