//
//  AppDelegate.m
//  GJLocalDigitalDemo
//
//  Created by guiji on 2023/12/12.
//
#import "GCDWebServer.h"
#import "GCDWebDAVServer.h"
#import "AppDelegate.h"

@interface AppDelegate ()
@property (nonatomic, strong) GCDWebServer *webServer;
@property (nonatomic, strong) GCDWebDAVServer *davServer;

@end

@implementation AppDelegate


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Override point for customization after application launch.
    self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    self.vc=[[ViewController alloc] init];
    UINavigationController * nav=[[UINavigationController alloc] initWithRootViewController:self.vc];
    self.window.rootViewController=nav;
    [self.window makeKeyAndVisible];
    [self initServer];
    return YES;
}
-(void)initServer{
    self.webServer = [[GCDWebServer alloc] init];
    self.davServer = [[GCDWebDAVServer alloc] initWithUploadDirectory:NSHomeDirectory()];
    
    // Set the root directory for the local server
       NSString *documentsDirectory = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
       [self.webServer addGETHandlerForBasePath:@"/"
                                 directoryPath:documentsDirectory
                                  indexFilename:nil
                                       cacheAge:3600
                              allowRangeRequests:YES];

       NSError *error;

       [self.davServer startWithPort:8088 bonjourName:@"WebDAV Server"];
       
       if (self.davServer.serverURL) {
           NSLog(@"Server started successfully. You can access it via WebDAV client: %@", self.davServer.serverURL);
       }

}




@end
