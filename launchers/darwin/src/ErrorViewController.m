#import "ErrorViewController.h"
#import "Launcher.h"
#import "CustomUI.h"

@interface ErrorViewController()
@property (nonatomic, assign) IBOutlet NSImageView* backgroundImage;
@property (nonatomic, assign) IBOutlet NSImageView* smallLogo;
@property (nonatomic, assign) IBOutlet NSImageView* voxelImage;

@end

@implementation ErrorViewController

- (void) awakeFromNib
{
    [self.backgroundImage setImage:[NSImage imageNamed:hifiBackgroundFilename]];
    [self.smallLogo setImage:[NSImage imageNamed:hifiSmallLogoFilename]];
    [self.voxelImage setImage:[NSImage imageNamed:hifiVoxelFilename]];
}

-(IBAction)resartLauncher:(id)sender
{
    [[Launcher sharedLauncher] showLoginScreen];
}

@end
