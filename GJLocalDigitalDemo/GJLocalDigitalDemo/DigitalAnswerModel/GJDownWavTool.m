//
//  GJDownWavTool.m
//  Digital
//
//  Created by cunzhi on 2023/11/23.
//

#import "GJDownWavTool.h"
#import "HttpClient.h"

@interface GJDownWavTool ()

@property (nonatomic, strong) HttpClient *client;
@property (nonatomic, strong) NSMutableArray *arrList;
@property (nonatomic, strong) NSFileManager *fileManager;
@property (nonatomic, strong) NSString *cachesPath;

@end
//static GJDownWavTool *instance = nil;
@implementation GJDownWavTool
//实现单例模式的代码，确保 GJDownWavTool 类在应用中只有一个实例。
+ (GJDownWavTool *)sharedTool {
    static GJDownWavTool *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken,^{
        instance = [[GJDownWavTool alloc] init];
    });

    return instance;
}

//初始化 GJDownWavTool 类的实例变量。
//•    self.client：一个 HttpClient 实例，负责网络请求。
//•    self.arrList：一个数组，存储正在下载的 WAV 文件模型。
//•    self.fileManager：一个 NSFileManager 实例，用于文件操作。
- (instancetype)init
{
    self = [super init];
    if (self) {
 
        self.client = [[HttpClient alloc] init];
        self.arrList=[[NSMutableArray alloc] init];
        self.fileManager = [NSFileManager defaultManager];

    }
    return self;
}

//初始化缓存路径，准备存储下载文件的文件夹。释掉了具体路径的设置，可能是通过外部方法来设置实际路径。

- (void)initCachesPath {
    
//    self.cachesPath =[GlobalFunc getHistoryCachePath:[NSString stringWithFormat:@"DownVideo/%@",[GlobalFunc getCurrentTimeYYYYMMDDHHMM]]];
    [self cencelDown];
}

/**
 开始下载一个 WAV 文件。
     •    将 model（WAV 文件的描述模型）加入下载队列 arrList。
     •    如果队列中已有任务，则直接返回，不再添加。
     •    如果这是队列中的第一个任务，调用 downloadWithModel: 方法开始下载。
 **/
- (void)downWavWithModel:(GJLDigitalAnswerModel *)model{
    

    [self.arrList addObject:model];

    if (self.arrList.count > 1) {
        return;
    }
    [self downloadWithModel:model];


}

/*/
 处理下载逻辑，首先检查文件是否已经缓存。
     •    使用 getHistoryCachePath: 获取缓存文件夹路径。
     •    构建文件路径并检查文件是否已经存在。如果文件存在，则标记为已下载，调用代理通知下载完成，并终止下载流程。
     •    如果文件不存在，开始下载。
 */
/*
 如果文件不存在，则通过 HttpClient 来发起下载请求。
     •    对于 .wav 文件，通过 HttpClient 的 downloadWithURL:savePathURL:pathExtension:progress:success:fail: 方法下载文件。
     •    下载成功后，更新模型的 localPath，并通知代理下载完成。
     •    如果下载失败或文件不存在，调用失败回调。
 */
- (void)downloadWithModel:(GJLDigitalAnswerModel *)model {
    
    NSLog(@"model.filePath:%@",model.filePath);
//    if (IS_Agent_EMPTY(self.cachesPath)) {
//        self.cachesPath =[GlobalFunc getHistoryCachePath:[NSString stringWithFormat:@"DownVideo/%@",[GlobalFunc getCurrentTimeYYYYMMDDHHMM]]];
//    }
    self.cachesPath = [self getHistoryCachePath:@"DownVideo"];
    NSString * filename = [model.filePath lastPathComponent];
    NSString *filePathStr = [NSString stringWithFormat:@"%@/%@",self.cachesPath,filename];
    NSString * pathExtern = [[filePathStr lastPathComponent] pathExtension];
    if(pathExtern==nil)
    {
        filePathStr = [NSString stringWithFormat:@"%@/%@.wav", self.cachesPath ,filename];
    }
    BOOL fileExists = [self.fileManager fileExistsAtPath:filePathStr];
    if (fileExists) {
        model.download = YES;
        model.localPath = filePathStr;
        [self downloadEnd:model];
        NSLog(@"下载==有缓存==%@\n本地=%@",model.filePath,filePathStr);
        if (self.delegate && [self.delegate respondsToSelector:@selector(downloadFinish:finish:)]) {
            [self.delegate downloadFinish:model finish:YES];
        }
    } else {
        __weak typeof(self)weakSelf = self;
        if ([[model.filePath.pathExtension lowercaseString] isEqualToString:@"wav"]) {
            NSURL *fileUrl = [NSURL fileURLWithPath:self.cachesPath];

            [[HttpClient manager] downloadWithURL:model.filePath savePathURL:fileUrl pathExtension:@"wav" progress:^(NSProgress *progress) {
                
            } success:^(NSURLResponse *response, NSURL *filePath) {
                model.download = YES;
                NSString *localPath = [filePath.absoluteString stringByReplacingOccurrencesOfString:@"file://" withString:@""];
                BOOL fileExists = [weakSelf.fileManager fileExistsAtPath:localPath];
                if (fileExists) {
                    model.localPath = localPath;
                    if (weakSelf.delegate && [weakSelf.delegate respondsToSelector:@selector(downloadFinish:finish:)]) {
                        [weakSelf.delegate downloadFinish:model finish:YES];
                    }
                    NSLog(@"下载==成功==%@\n本地=%@",model.filePath,localPath);
                } else {
                    if (weakSelf.delegate && [weakSelf.delegate respondsToSelector:@selector(downloadFinish:finish:)]) {
                        NSLog(@"下载==结束==音频不存在");
                        [weakSelf.delegate downloadFinish:model finish:NO];
                    }
                }
                [weakSelf downloadEnd:model];
            } fail:^(NSError *error) {
                model.download = YES;
                [weakSelf downloadEnd:model];
                if (weakSelf.delegate && [weakSelf.delegate respondsToSelector:@selector(downloadFinish:finish:)]) {
        
                    [weakSelf.delegate downloadFinish:model finish:NO];
                }
                NSLog(@"下载==失败==%@",error.localizedDescription);
            }];
        } else {
            
            [[HttpClient manager] requestWithDownUrl:model.filePath para:nil headers:nil httpMethod:HttpMethodGet success:^(NSURLSessionDataTask *task, id responseObject) {
                if([responseObject isKindOfClass:[NSData class]])
                {
                    NSString *base64String = [responseObject base64EncodedStringWithOptions:0];
                    NSData *data = [[NSData alloc] initWithBase64EncodedString:base64String options:0];
                    if (data) {
                        
                        NSString * filename2 = [[model.filePath lastPathComponent] stringByDeletingPathExtension];
                        NSString *filePath2 = [NSString stringWithFormat:@"%@/%@.wav",self.cachesPath,filename2];
                        [weakSelf.fileManager createFileAtPath:filePath2 contents:data attributes:nil];
          
//                        [[WebCacheHelpler sharedWebCache] storeDataToDiskCache:data key:model.filePath extension:@"wav"];
//                        NSString *path = [[WebCacheHelpler sharedWebCache] isCachePathForKey:model.filePath extension:@"wav"];
                        NSString *localPath = [filePath2 stringByReplacingOccurrencesOfString:@"file://" withString:@""];
                        BOOL fileExists = [weakSelf.fileManager fileExistsAtPath:localPath];
                        if (fileExists) {
                            model.localPath = localPath;
                            if (weakSelf.delegate && [weakSelf.delegate respondsToSelector:@selector(downloadFinish:finish:)]) {
                                [weakSelf.delegate downloadFinish:model finish:YES];
                            }
                            NSLog(@"下载==成功==%@\n本地=%@",model.filePath,localPath);
                        } else {
                            if (weakSelf.delegate && [weakSelf.delegate respondsToSelector:@selector(downloadFinish:finish:)]) {
                                NSLog(@"下载==结束==音频不存在");
                                [weakSelf.delegate downloadFinish:model finish:NO];
                            }
                        }
                        [weakSelf downloadEnd:model];
                        
                    } else {
                        if (weakSelf.delegate && [weakSelf.delegate respondsToSelector:@selector(downloadFinish:finish:)]) {
                            NSLog(@"下载==结束==音频不存在");
                            [weakSelf.delegate downloadFinish:model finish:NO];
                        }
                        [weakSelf downloadEnd:model];
                    }
                }
            } failure:^(NSURLSessionDataTask *task, NSError *error) {
                 
            }];
        }
        
    }
}

/*/
 下载任务完成后，移除下载队列中的该任务。
     •    如果队列中还有其他任务，继续下载下一个任务。
 */
- (void)downloadEnd:(GJLDigitalAnswerModel *)model {
    
    [self.arrList removeObject:model];
    if (self.arrList.count >0) {
        GJLDigitalAnswerModel * answer_model=self.arrList[0];
        [self downloadWithModel:answer_model];

    }
}

/*/
 取消当前所有下载任务，清空下载队列 arrList。*/
- (void)cencelDown {
    
    [self.arrList removeAllObjects];
    
}

/*/
 据提供的文件夹名称 pathName，获取缓存路径，并确保该路径存在。
     •    如果路径不存在，创建相应的目录。
 */
-(NSString *)getHistoryCachePath:(NSString*)pathName
{
    NSString* folderPath =[[self getFInalPath] stringByAppendingPathComponent:pathName];
    //创建文件管理器
    NSFileManager *fileManager = [NSFileManager defaultManager];
    //判断temp文件夹是否存在
    BOOL fileExists = [fileManager fileExistsAtPath:folderPath];
    //如果不存在说创建,因为下载时,不会自动创建文件夹
    if (!fileExists)
    {
        [fileManager createDirectoryAtPath:folderPath withIntermediateDirectories:YES attributes:nil error:nil];
    }
    return folderPath;
}

/*
 获取应用的缓存路径，并确保路径存在。
     •    该路径通常用于存储临时文件或下载的文件。
 */
- (NSString *)getFInalPath
{
    NSString* folderPath =[[NSHomeDirectory() stringByAppendingPathComponent:@"Library"] stringByAppendingPathComponent:@"Cache"];
    //创建文件管理器
    NSFileManager *fileManager = [NSFileManager defaultManager];
    //判断temp文件夹是否存在
    BOOL fileExists = [fileManager fileExistsAtPath:folderPath];
    //如果不存在说创建,因为下载时,不会自动创建文件夹
    if (!fileExists) {
        [fileManager createDirectoryAtPath:folderPath withIntermediateDirectories:YES attributes:nil error:nil];
    }
    
    return folderPath;
}



@end
