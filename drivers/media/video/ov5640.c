#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/videodev2.h>
#include <media/v4l2-subdev.h>
#include <media/soc_camera.h>
#include <media/v4l2-chip-ident.h>
#include <linux/delay.h>

/* add by cym 20130605 */
#include <linux/videodev2_samsung.h>
#include <linux/regulator/consumer.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>
/* end add */

/* add by cym 20130605 */
enum {
    AUTO_FOCUS_FAILED,
    AUTO_FOCUS_DONE,
    AUTO_FOCUS_CANCELLED,
};

#ifndef CONFIG_TC4_EVT
//static struct regulator *vdd18_5m_cam_regulator = NULL;
//static struct regulator *vdd28_5m_cam_regulator = NULL;
struct regulator *ov_vddaf_cam_regulator = NULL;
struct regulator *ov_vdd5m_cam_regulator = NULL;

struct regulator *ov_vdd18_cam_regulator = NULL;
struct regulator *ov_vdd28_cam_regulator = NULL;

int down_af_firmware_flag = 0;
#endif
/* end add */

/* ANSI Color codes */
#define VT(CODES)  "\033[" CODES "m"
#define VT_NORMAL  VT("")
#define VT_RED     VT("0;32;31")
#define VT_GREEN   VT("1;32")
#define VT_YELLOW  VT("1;33")
#define VT_BLUE    VT("1;34")
#define VT_PURPLE  VT("0;35")


#define OV5640_DBG

#define xprintk(fmt, ...) \
    printk("%s()->%d " fmt, __func__, __LINE__, ## __VA_ARGS__)

#ifdef OV5640_DBG

#define _DBG(color, fmt, ...)  \
    xprintk(color "" fmt VT_NORMAL, ## __VA_ARGS__)

#define OV_INFO(fmt, args...)  _DBG(VT_GREEN, fmt, ## args)
#define OV_ERR(fmt, args...)   _DBG(VT_RED, fmt, ## args)
#else
#define OV_INFO(fmt, args...)  do {} while(0)
#define OV_ERR(fmt, args...) do {} while(0)
#endif

#define _INFO(color, fmt, ...) \
    xprintk(color "::" fmt ""VT_NORMAL, ## __VA_ARGS__)

/* mainly used in test code */
#define INFO_PURLPLE(fmt, args...) _INFO(VT_PURPLE, fmt, ## args)
#define INFO_RED(fmt, args...)     _INFO(VT_RED, fmt, ## args)
#define INFO_GREEN(fmt, args...)   _INFO(VT_GREEN, fmt, ## args)
#define INFO_BLUE(fmt, args...)    _INFO(VT_BLUE, fmt, ## args)


#define OV5640_I2C_NAME  "ov5640"

/* 
 * I2C write address: 0x78, read: 0x79 , give up least significant bit.
 */
#define OV5640_I2C_ADDR  (0x78 >> 1)

/*
 * sensor ID
 */
#define OV5640  0x5640
#define VERSION(id, vers) ((id << 8) | (vers & 0XFF))


/* default format */
#define QVGA_WIDTH  320
#define QVGA_HEIGHT	240

#define VGA_WIDTH	640
#define VGA_HEIGHT	480

#define XGA_WIDTH	1024
#define XGA_HEIGHT	768

#define OV5640_720P_WIDTH       1280
#define OV5640_720P_HEIGHT      720


#define SXGA_WIDTH	1280
#define SXGA_HEIGHT	960

#define UXGA_WIDTH	1600
#define UXGA_HEIGHT	1200

#define QXGA_WIDTH	2048
#define QXGA_HEIGHT	1536

#define QSXGA_WIDTH	 2592
#define QSXGA_HEIGHT	 1944

#define CAPTURE_FRAME_RATE  500   /* multiplied by 100 */
#define PREVIEW_FRAME_RATE  1500   /* multiplied by 100 */

#define OV5640_COLUMN_SKIP 0
#define OV5640_ROW_SKIP    0
#define OV5640_MAX_WIDTH   (QSXGA_WIDTH)
#define OV5640_MAX_HEIGHT  (QSXGA_HEIGHT)

#define OV5640_HFLIP 0x1
#define OV5640_VFLIP 0x2


#if  1

#define MIN_FPS 15
#define MAX_FPS 30
#define DEFAULT_FPS 30

#define OV5640_XCLK_MIN 6000000
#define OV5640_XCLK_MAX 24000000

#define OV5640_CHIP_ID_HIGH_BYTE        0x300A
#define OV5640_CHIP_ID_LOW_BYTE         0x300B

enum ov5640_mode {
        ov5640_mode_MIN = 0,
        ov5640_mode_VGA_640_480 = 0,
        ov5640_mode_QVGA_320_240 = 1,
        ov5640_mode_NTSC_720_480 = 2,
        ov5640_mode_PAL_720_576 = 3,
        ov5640_mode_720P_1280_720 = 4,
        ov5640_mode_1080P_1920_1080 = 5,
        ov5640_mode_QSXGA_2592_1944 = 6,
        ov5640_mode_QCIF_176_144 = 7,
        ov5640_mode_XGA_1024_768 = 8,
        ov5640_mode_MAX = 8
};

enum ov5640_frame_rate {
        ov5640_15_fps,
        ov5640_30_fps
};

static int ov5640_framerates[] = {
        [ov5640_15_fps] = 15,
        [ov5640_30_fps] = 30,
};

struct reg_value {
        u16 u16RegAddr;
        u8 u8Val;
        u8 u8Mask;
        u32 u32Delay_ms;
};

struct ov5640_mode_info {
        enum ov5640_mode mode;
        u32 width;
        u32 height;
        struct reg_value *init_data_ptr;
        u32 init_data_size;
};


struct sensor_data {
        //const struct ov5642_platform_data *platform_data;
        struct v4l2_int_device *v4l2_int_device;
        struct i2c_client *i2c_client;
        struct v4l2_pix_format pix;
        struct v4l2_captureparm streamcap;
        bool on;

        /* control settings */
        int brightness;
        int hue;
        int contrast;
        int saturation;
        int red;
        int green;
        int blue;
        int ae_mode;

        u32 mclk;
        u8 mclk_source;
        int csi;

        void (*io_init)(void);
};


/*!
 * Maintains the information on the current state of the sesor.
 */
static struct sensor_data ov5640_data;
static int prev_sysclk;
static int AE_Target = 52, night_mode;
static int prev_HTS;
static int AE_high, AE_low;

static struct reg_value ov5640_global_init_setting[] = {
        {0x3008, 0x42, 0, 0},
        {0x3103, 0x03, 0, 0}, {0x3017, 0xff, 0, 0}, {0x3018, 0xff, 0, 0},
        {0x3034, 0x1a, 0, 0}, {0x3037, 0x13, 0, 0}, {0x3108, 0x01, 0, 0},
        {0x3630, 0x36, 0, 0}, {0x3631, 0x0e, 0, 0}, {0x3632, 0xe2, 0, 0},
        {0x3633, 0x12, 0, 0}, {0x3621, 0xe0, 0, 0}, {0x3704, 0xa0, 0, 0},
        {0x3703, 0x5a, 0, 0}, {0x3715, 0x78, 0, 0}, {0x3717, 0x01, 0, 0},
        {0x370b, 0x60, 0, 0}, {0x3705, 0x1a, 0, 0}, {0x3905, 0x02, 0, 0},
        {0x3906, 0x10, 0, 0}, {0x3901, 0x0a, 0, 0}, {0x3731, 0x12, 0, 0},
        {0x3600, 0x08, 0, 0}, {0x3601, 0x33, 0, 0}, {0x302d, 0x60, 0, 0},
        {0x3620, 0x52, 0, 0}, {0x371b, 0x20, 0, 0}, {0x471c, 0x50, 0, 0},
        {0x3a13, 0x43, 0, 0}, {0x3a18, 0x00, 0, 0}, {0x3a19, 0x7c, 0, 0},
        {0x3635, 0x13, 0, 0}, {0x3636, 0x03, 0, 0}, {0x3634, 0x40, 0, 0},
        {0x3622, 0x01, 0, 0}, {0x3c01, 0x34, 0, 0}, {0x3c04, 0x28, 0, 0},
        {0x3c05, 0x98, 0, 0}, {0x3c06, 0x00, 0, 0}, {0x3c07, 0x07, 0, 0},
        {0x3c08, 0x00, 0, 0}, {0x3c09, 0x1c, 0, 0}, {0x3c0a, 0x9c, 0, 0},
        {0x3c0b, 0x40, 0, 0}, {0x3810, 0x00, 0, 0}, {0x3811, 0x10, 0, 0},
        {0x3812, 0x00, 0, 0}, {0x3708, 0x64, 0, 0}, {0x4001, 0x02, 0, 0},
        {0x4005, 0x1a, 0, 0}, {0x3000, 0x00, 0, 0}, {0x3004, 0xff, 0, 0},
        {0x300e, 0x58, 0, 0}, {0x302e, 0x00, 0, 0}, {0x4300, 0x30, 0, 0},
        {0x501f, 0x00, 0, 0}, {0x440e, 0x00, 0, 0}, {0x5000, 0xa7, 0, 0},
        {0x3008, 0x02, 0, 0},
};

static struct reg_value ov5640_init_setting_30fps_VGA[] = {
        {0x3008, 0x42, 0, 0},
        {0x3103, 0x03, 0, 0}, {0x3017, 0xff, 0, 0}, {0x3018, 0xff, 0, 0},
        {0x3034, 0x1a, 0, 0}, {0x3035, 0x11, 0, 0}, {0x3036, 0x46, 0, 0},
        {0x3037, 0x13, 0, 0}, {0x3108, 0x01, 0, 0}, {0x3630, 0x36, 0, 0},
        {0x3631, 0x0e, 0, 0}, {0x3632, 0xe2, 0, 0}, {0x3633, 0x12, 0, 0},
        {0x3621, 0xe0, 0, 0}, {0x3704, 0xa0, 0, 0}, {0x3703, 0x5a, 0, 0},
        {0x3715, 0x78, 0, 0}, {0x3717, 0x01, 0, 0}, {0x370b, 0x60, 0, 0},
        {0x3705, 0x1a, 0, 0}, {0x3905, 0x02, 0, 0}, {0x3906, 0x10, 0, 0},
        {0x3901, 0x0a, 0, 0}, {0x3731, 0x12, 0, 0}, {0x3600, 0x08, 0, 0},
        {0x3601, 0x33, 0, 0}, {0x302d, 0x60, 0, 0}, {0x3620, 0x52, 0, 0},
        {0x371b, 0x20, 0, 0}, {0x471c, 0x50, 0, 0}, {0x3a13, 0x43, 0, 0},
        {0x3a18, 0x00, 0, 0}, {0x3a19, 0xf8, 0, 0}, {0x3635, 0x13, 0, 0},
        {0x3636, 0x03, 0, 0}, {0x3634, 0x40, 0, 0}, {0x3622, 0x01, 0, 0},
        {0x3c01, 0x34, 0, 0}, {0x3c04, 0x28, 0, 0}, {0x3c05, 0x98, 0, 0},
        {0x3c06, 0x00, 0, 0}, {0x3c07, 0x08, 0, 0}, {0x3c08, 0x00, 0, 0},
        {0x3c09, 0x1c, 0, 0}, {0x3c0a, 0x9c, 0, 0}, {0x3c0b, 0x40, 0, 0},
        {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0}, {0x3814, 0x31, 0, 0},
        {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0}, {0x3801, 0x00, 0, 0},
        {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0}, {0x3804, 0x0a, 0, 0},
        {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0}, {0x3807, 0x9b, 0, 0},
        {0x3808, 0x02, 0, 0}, {0x3809, 0x80, 0, 0}, {0x380a, 0x01, 0, 0},
        {0x380b, 0xe0, 0, 0}, {0x380c, 0x07, 0, 0}, {0x380d, 0x68, 0, 0},
        {0x380e, 0x03, 0, 0}, {0x380f, 0xd8, 0, 0}, {0x3810, 0x00, 0, 0},
        {0x3811, 0x10, 0, 0}, {0x3812, 0x00, 0, 0}, {0x3813, 0x06, 0, 0},
        {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0}, {0x3708, 0x64, 0, 0},
        {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x03, 0, 0},
        {0x3a03, 0xd8, 0, 0}, {0x3a08, 0x01, 0, 0}, {0x3a09, 0x27, 0, 0},
        {0x3a0a, 0x00, 0, 0}, {0x3a0b, 0xf6, 0, 0}, {0x3a0e, 0x03, 0, 0},
        {0x3a0d, 0x04, 0, 0}, {0x3a14, 0x03, 0, 0}, {0x3a15, 0xd8, 0, 0},
        {0x4001, 0x02, 0, 0}, {0x4004, 0x02, 0, 0}, {0x3000, 0x00, 0, 0},
        {0x3002, 0x1c, 0, 0}, {0x3004, 0xff, 0, 0}, {0x3006, 0xc3, 0, 0},
        {0x300e, 0x58, 0, 0}, {0x302e, 0x00, 0, 0}, {0x4300, 0x30, 0, 0},
        {0x501f, 0x00, 0, 0}, {0x4713, 0x03, 0, 0}, {0x4407, 0x04, 0, 0},
        {0x440e, 0x00, 0, 0}, {0x460b, 0x35, 0, 0}, {0x460c, 0x22, 0, 0},
        {0x4837, 0x22, 0, 0}, {0x3824, 0x02, 0, 0}, {0x5000, 0xa7, 0, 0},
        {0x5001, 0xa3, 0, 0}, {0x5180, 0xff, 0, 0}, {0x5181, 0xf2, 0, 0},
        {0x5182, 0x00, 0, 0}, {0x5183, 0x14, 0, 0}, {0x5184, 0x25, 0, 0},
        {0x5185, 0x24, 0, 0}, {0x5186, 0x09, 0, 0}, {0x5187, 0x09, 0, 0},
        {0x5188, 0x09, 0, 0}, {0x5189, 0x88, 0, 0}, {0x518a, 0x54, 0, 0},
        {0x518b, 0xee, 0, 0}, {0x518c, 0xb2, 0, 0}, {0x518d, 0x50, 0, 0},
        {0x518e, 0x34, 0, 0}, {0x518f, 0x6b, 0, 0}, {0x5190, 0x46, 0, 0},
        {0x5191, 0xf8, 0, 0}, {0x5192, 0x04, 0, 0}, {0x5193, 0x70, 0, 0},
        {0x5194, 0xf0, 0, 0}, {0x5195, 0xf0, 0, 0}, {0x5196, 0x03, 0, 0},
        {0x5197, 0x01, 0, 0}, {0x5198, 0x04, 0, 0}, {0x5199, 0x6c, 0, 0},
        {0x519a, 0x04, 0, 0}, {0x519b, 0x00, 0, 0}, {0x519c, 0x09, 0, 0},
        {0x519d, 0x2b, 0, 0}, {0x519e, 0x38, 0, 0}, {0x5381, 0x1e, 0, 0},
        {0x5382, 0x5b, 0, 0}, {0x5383, 0x08, 0, 0}, {0x5384, 0x0a, 0, 0},
        {0x5385, 0x7e, 0, 0}, {0x5386, 0x88, 0, 0}, {0x5387, 0x7c, 0, 0},
        {0x5388, 0x6c, 0, 0}, {0x5389, 0x10, 0, 0}, {0x538a, 0x01, 0, 0},
        {0x538b, 0x98, 0, 0}, {0x5300, 0x08, 0, 0}, {0x5301, 0x30, 0, 0},
        {0x5302, 0x10, 0, 0}, {0x5303, 0x00, 0, 0}, {0x5304, 0x08, 0, 0},
        {0x5305, 0x30, 0, 0}, {0x5306, 0x08, 0, 0}, {0x5307, 0x16, 0, 0},
        {0x5309, 0x08, 0, 0}, {0x530a, 0x30, 0, 0}, {0x530b, 0x04, 0, 0},
        {0x530c, 0x06, 0, 0}, {0x5480, 0x01, 0, 0}, {0x5481, 0x08, 0, 0},
        {0x5482, 0x14, 0, 0}, {0x5483, 0x28, 0, 0}, {0x5484, 0x51, 0, 0},
        {0x5485, 0x65, 0, 0}, {0x5486, 0x71, 0, 0}, {0x5487, 0x7d, 0, 0},
        {0x5488, 0x87, 0, 0}, {0x5489, 0x91, 0, 0}, {0x548a, 0x9a, 0, 0},
        {0x548b, 0xaa, 0, 0}, {0x548c, 0xb8, 0, 0}, {0x548d, 0xcd, 0, 0},
        {0x548e, 0xdd, 0, 0}, {0x548f, 0xea, 0, 0}, {0x5490, 0x1d, 0, 0},
        {0x5580, 0x02, 0, 0}, {0x5583, 0x40, 0, 0}, {0x5584, 0x10, 0, 0},
        {0x5589, 0x10, 0, 0}, {0x558a, 0x00, 0, 0}, {0x558b, 0xf8, 0, 0},
        {0x5800, 0x23, 0, 0}, {0x5801, 0x14, 0, 0}, {0x5802, 0x0f, 0, 0},
        {0x5803, 0x0f, 0, 0}, {0x5804, 0x12, 0, 0}, {0x5805, 0x26, 0, 0},
        {0x5806, 0x0c, 0, 0}, {0x5807, 0x08, 0, 0}, {0x5808, 0x05, 0, 0},
        {0x5809, 0x05, 0, 0}, {0x580a, 0x08, 0, 0}, {0x580b, 0x0d, 0, 0},
        {0x580c, 0x08, 0, 0}, {0x580d, 0x03, 0, 0}, {0x580e, 0x00, 0, 0},
        {0x580f, 0x00, 0, 0}, {0x5810, 0x03, 0, 0}, {0x5811, 0x09, 0, 0},
        {0x5812, 0x07, 0, 0}, {0x5813, 0x03, 0, 0}, {0x5814, 0x00, 0, 0},
        {0x5815, 0x01, 0, 0}, {0x5816, 0x03, 0, 0}, {0x5817, 0x08, 0, 0},
        {0x5818, 0x0d, 0, 0}, {0x5819, 0x08, 0, 0}, {0x581a, 0x05, 0, 0},
        {0x581b, 0x06, 0, 0}, {0x581c, 0x08, 0, 0}, {0x581d, 0x0e, 0, 0},
        {0x581e, 0x29, 0, 0}, {0x581f, 0x17, 0, 0}, {0x5820, 0x11, 0, 0},
        {0x5821, 0x11, 0, 0}, {0x5822, 0x15, 0, 0}, {0x5823, 0x28, 0, 0},
        {0x5824, 0x46, 0, 0}, {0x5825, 0x26, 0, 0}, {0x5826, 0x08, 0, 0},
        {0x5827, 0x26, 0, 0}, {0x5828, 0x64, 0, 0}, {0x5829, 0x26, 0, 0},
        {0x582a, 0x24, 0, 0}, {0x582b, 0x22, 0, 0}, {0x582c, 0x24, 0, 0},
        {0x582d, 0x24, 0, 0}, {0x582e, 0x06, 0, 0}, {0x582f, 0x22, 0, 0},
        {0x5830, 0x40, 0, 0}, {0x5831, 0x42, 0, 0}, {0x5832, 0x24, 0, 0},
        {0x5833, 0x26, 0, 0}, {0x5834, 0x24, 0, 0}, {0x5835, 0x22, 0, 0},
        {0x5836, 0x22, 0, 0}, {0x5837, 0x26, 0, 0}, {0x5838, 0x44, 0, 0},
        {0x5839, 0x24, 0, 0}, {0x583a, 0x26, 0, 0}, {0x583b, 0x28, 0, 0},
        {0x583c, 0x42, 0, 0}, {0x583d, 0xce, 0, 0}, {0x5025, 0x00, 0, 0},
        {0x3a0f, 0x30, 0, 0}, {0x3a10, 0x28, 0, 0}, {0x3a1b, 0x30, 0, 0},
        {0x3a1e, 0x26, 0, 0}, {0x3a11, 0x60, 0, 0}, {0x3a1f, 0x14, 0, 0},
        {0x3008, 0x02, 0, 0}, {0x3034, 0x1a, 0, 0}, {0x3035, 0x11, 0, 0},
        {0x3036, 0x46, 0, 0}, {0x3037, 0x13, 0, 0},
};

static struct reg_value ov5640_setting_30fps_VGA_640_480[] = {
        {0x3c07, 0x08, 0, 0}, {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0},
        {0x3814, 0x31, 0, 0}, {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0},
        {0x3801, 0x00, 0, 0}, {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0},
        {0x3804, 0x0a, 0, 0}, {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0},
        {0x3807, 0x9b, 0, 0}, {0x3808, 0x02, 0, 0}, {0x3809, 0x80, 0, 0},
        {0x380a, 0x01, 0, 0}, {0x380b, 0xe0, 0, 0}, {0x380c, 0x07, 0, 0},
        {0x380d, 0x68, 0, 0}, {0x380e, 0x03, 0, 0}, {0x380f, 0xd8, 0, 0},
        {0x3813, 0x06, 0, 0}, {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0},
        {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x0b, 0, 0},
        {0x3a03, 0x88, 0, 0}, {0x3a14, 0x0b, 0, 0}, {0x3a15, 0x88, 0, 0},
        {0x4004, 0x02, 0, 0}, {0x3002, 0x1c, 0, 0}, {0x3006, 0xc3, 0, 0},
        {0x4713, 0x03, 0, 0}, {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0},
        {0x460c, 0x22, 0, 0}, {0x4837, 0x22, 0, 0}, {0x3824, 0x02, 0, 0},
        {0x5001, 0xa3, 0, 0}, {0x3034, 0x1a, 0, 0}, {0x3035, 0x11, 0, 0},
        {0x3036, 0x46, 0, 0}, {0x3037, 0x13, 0, 0}, {0x3503, 0x00, 0, 0},
};

static struct reg_value ov5640_setting_15fps_VGA_640_480[] = {
        {0x3c07, 0x08, 0, 0}, {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0},
        {0x3814, 0x31, 0, 0}, {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0},
        {0x3801, 0x00, 0, 0}, {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0},
        {0x3804, 0x0a, 0, 0}, {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0},
        {0x3807, 0x9b, 0, 0}, {0x3808, 0x02, 0, 0}, {0x3809, 0x80, 0, 0},
        {0x380a, 0x01, 0, 0}, {0x380b, 0xe0, 0, 0}, {0x380c, 0x07, 0, 0},
        {0x380d, 0x68, 0, 0}, {0x380e, 0x03, 0, 0}, {0x380f, 0xd8, 0, 0},
        {0x3813, 0x06, 0, 0}, {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0},
        {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x0b, 0, 0},
        {0x3a03, 0x88, 0, 0}, {0x3a14, 0x0b, 0, 0}, {0x3a15, 0x88, 0, 0},
        {0x4004, 0x02, 0, 0}, {0x3002, 0x1c, 0, 0}, {0x3006, 0xc3, 0, 0},
        {0x4713, 0x03, 0, 0}, {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0},
        {0x460c, 0x22, 0, 0}, {0x4837, 0x22, 0, 0}, {0x3824, 0x02, 0, 0},
        {0x5001, 0xa3, 0, 0}, {0x3034, 0x1a, 0, 0}, {0x3035, 0x21, 0, 0},
        {0x3036, 0x46, 0, 0}, {0x3037, 0x13, 0, 0}, {0x3503, 0x00, 0, 0},
};

static struct reg_value ov5640_setting_30fps_QVGA_320_240[] = {
        {0x3c07, 0x08, 0, 0}, {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0},
        {0x3814, 0x31, 0, 0}, {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0},
        {0x3801, 0x00, 0, 0}, {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0},
        {0x3804, 0x0a, 0, 0}, {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0},
        {0x3807, 0x9b, 0, 0}, {0x3808, 0x01, 0, 0}, {0x3809, 0x40, 0, 0},
        {0x380a, 0x00, 0, 0}, {0x380b, 0xf0, 0, 0}, {0x380c, 0x07, 0, 0},
        {0x380d, 0x68, 0, 0}, {0x380e, 0x03, 0, 0}, {0x380f, 0xd8, 0, 0},
        {0x3813, 0x06, 0, 0}, {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0},
        {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x0b, 0, 0},
        {0x3a03, 0x88, 0, 0}, {0x3a14, 0x0b, 0, 0}, {0x3a15, 0x88, 0, 0},
        {0x4004, 0x02, 0, 0}, {0x3002, 0x1c, 0, 0}, {0x3006, 0xc3, 0, 0},
        {0x4713, 0x03, 0, 0}, {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0},
        {0x460c, 0x22, 0, 0}, {0x4837, 0x22, 0, 0}, {0x3824, 0x02, 0, 0},
        {0x5001, 0xa3, 0, 0}, {0x3034, 0x1a, 0, 0}, {0x3035, 0x11, 0, 0},
        {0x3036, 0x46, 0, 0}, {0x3037, 0x13, 0, 0},
};

static struct reg_value ov5640_setting_15fps_QVGA_320_240[] = {
        {0x3c07, 0x08, 0, 0}, {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0},
        {0x3814, 0x31, 0, 0}, {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0},
        {0x3801, 0x00, 0, 0}, {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0},
        {0x3804, 0x0a, 0, 0}, {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0},
        {0x3807, 0x9b, 0, 0}, {0x3808, 0x01, 0, 0}, {0x3809, 0x40, 0, 0},
        {0x380a, 0x00, 0, 0}, {0x380b, 0xf0, 0, 0}, {0x380c, 0x07, 0, 0},
        {0x380d, 0x68, 0, 0}, {0x380e, 0x03, 0, 0}, {0x380f, 0xd8, 0, 0},
        {0x3813, 0x06, 0, 0}, {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0},
        {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x0b, 0, 0},
        {0x3a03, 0x88, 0, 0}, {0x3a14, 0x0b, 0, 0}, {0x3a15, 0x88, 0, 0},
        {0x4004, 0x02, 0, 0}, {0x3002, 0x1c, 0, 0}, {0x3006, 0xc3, 0, 0},
        {0x4713, 0x03, 0, 0}, {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0},
        {0x460c, 0x22, 0, 0}, {0x4837, 0x22, 0, 0}, {0x3824, 0x02, 0, 0},
        {0x5001, 0xa3, 0, 0}, {0x3034, 0x1a, 0, 0}, {0x3035, 0x21, 0, 0},
        {0x3036, 0x46, 0, 0}, {0x3037, 0x13, 0, 0},
};

static struct reg_value ov5640_setting_30fps_NTSC_720_480[] = {
        {0x3c07, 0x08, 0, 0}, {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0},
        {0x3814, 0x31, 0, 0}, {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0},
        {0x3801, 0x00, 0, 0}, {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0},
        {0x3804, 0x0a, 0, 0}, {0x3805, 0x3f, 0, 0}, {0x3806, 0x06, 0, 0},
        {0x3807, 0xd4, 0, 0}, {0x3808, 0x02, 0, 0}, {0x3809, 0xd0, 0, 0},
        {0x380a, 0x01, 0, 0}, {0x380b, 0xe0, 0, 0}, {0x380c, 0x07, 0, 0},
        {0x380d, 0x68, 0, 0}, {0x380e, 0x03, 0, 0}, {0x380f, 0xd8, 0, 0},
        {0x3813, 0x06, 0, 0}, {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0},
        {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x0b, 0, 0},
        {0x3a03, 0x88, 0, 0}, {0x3a14, 0x0b, 0, 0}, {0x3a15, 0x88, 0, 0},
        {0x4004, 0x02, 0, 0}, {0x3002, 0x1c, 0, 0}, {0x3006, 0xc3, 0, 0},
        {0x4713, 0x03, 0, 0}, {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0},
        {0x460c, 0x22, 0, 0}, {0x4837, 0x22, 0, 0}, {0x3824, 0x02, 0, 0},
        {0x5001, 0xa3, 0, 0}, {0x3034, 0x1a, 0, 0}, {0x3035, 0x11, 0, 0},
        {0x3036, 0x46, 0, 0}, {0x3037, 0x13, 0, 0},
};

static struct reg_value ov5640_setting_15fps_NTSC_720_480[] = {
        {0x3c07, 0x08, 0, 0}, {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0},
        {0x3814, 0x31, 0, 0}, {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0},
        {0x3801, 0x00, 0, 0}, {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0},
        {0x3804, 0x0a, 0, 0}, {0x3805, 0x3f, 0, 0}, {0x3806, 0x06, 0, 0},
        {0x3807, 0xd4, 0, 0}, {0x3808, 0x02, 0, 0}, {0x3809, 0xd0, 0, 0},
        {0x380a, 0x01, 0, 0}, {0x380b, 0xe0, 0, 0}, {0x380c, 0x07, 0, 0},
        {0x380d, 0x68, 0, 0}, {0x380e, 0x03, 0, 0}, {0x380f, 0xd8, 0, 0},
        {0x3813, 0x06, 0, 0}, {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0},
        {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x0b, 0, 0},
        {0x3a03, 0x88, 0, 0}, {0x3a14, 0x0b, 0, 0}, {0x3a15, 0x88, 0, 0},
        {0x4004, 0x02, 0, 0}, {0x3002, 0x1c, 0, 0}, {0x3006, 0xc3, 0, 0},
        {0x4713, 0x03, 0, 0}, {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0},
        {0x460c, 0x22, 0, 0}, {0x4837, 0x22, 0, 0}, {0x3824, 0x02, 0, 0},
        {0x5001, 0xa3, 0, 0}, {0x3034, 0x1a, 0, 0}, {0x3035, 0x21, 0, 0},
        {0x3036, 0x46, 0, 0}, {0x3037, 0x13, 0, 0},
};

static struct reg_value ov5640_setting_30fps_PAL_720_576[] = {
        {0x3c07, 0x08, 0, 0}, {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0},
        {0x3814, 0x31, 0, 0}, {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0},
        {0x3801, 0x60, 0, 0}, {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0},
        {0x3804, 0x09, 0, 0}, {0x3805, 0x7e, 0, 0}, {0x3806, 0x07, 0, 0},
        {0x3807, 0x9b, 0, 0}, {0x3808, 0x02, 0, 0}, {0x3809, 0xd0, 0, 0},
        {0x380a, 0x02, 0, 0}, {0x380b, 0x40, 0, 0}, {0x380c, 0x07, 0, 0},
        {0x380d, 0x68, 0, 0}, {0x380e, 0x03, 0, 0}, {0x380f, 0xd8, 0, 0},
        {0x3813, 0x06, 0, 0}, {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0},
        {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x0b, 0, 0},
        {0x3a03, 0x88, 0, 0}, {0x3a14, 0x0b, 0, 0}, {0x3a15, 0x88, 0, 0},
        {0x4004, 0x02, 0, 0}, {0x3002, 0x1c, 0, 0}, {0x3006, 0xc3, 0, 0},
        {0x4713, 0x03, 0, 0}, {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0},
        {0x460c, 0x22, 0, 0}, {0x4837, 0x22, 0, 0}, {0x3824, 0x02, 0, 0},
        {0x5001, 0xa3, 0, 0}, {0x3034, 0x1a, 0, 0}, {0x3035, 0x11, 0, 0},
        {0x3036, 0x46, 0, 0}, {0x3037, 0x13, 0, 0},
};

static struct reg_value ov5640_setting_15fps_PAL_720_576[] = {
        {0x3c07, 0x08, 0, 0}, {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0},
        {0x3814, 0x31, 0, 0}, {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0},
        {0x3801, 0x60, 0, 0}, {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0},
        {0x3804, 0x09, 0, 0}, {0x3805, 0x7e, 0, 0}, {0x3806, 0x07, 0, 0},
        {0x3807, 0x9b, 0, 0}, {0x3808, 0x02, 0, 0}, {0x3809, 0xd0, 0, 0},
        {0x380a, 0x02, 0, 0}, {0x380b, 0x40, 0, 0}, {0x380c, 0x07, 0, 0},
        {0x380d, 0x68, 0, 0}, {0x380e, 0x03, 0, 0}, {0x380f, 0xd8, 0, 0},
        {0x3813, 0x06, 0, 0}, {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0},
        {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x0b, 0, 0},
        {0x3a03, 0x88, 0, 0}, {0x3a14, 0x0b, 0, 0}, {0x3a15, 0x88, 0, 0},
        {0x4004, 0x02, 0, 0}, {0x3002, 0x1c, 0, 0}, {0x3006, 0xc3, 0, 0},
        {0x4713, 0x03, 0, 0}, {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0},
        {0x460c, 0x22, 0, 0}, {0x4837, 0x22, 0, 0}, {0x3824, 0x02, 0, 0},
        {0x5001, 0xa3, 0, 0}, {0x3034, 0x1a, 0, 0}, {0x3035, 0x21, 0, 0},
        {0x3036, 0x46, 0, 0}, {0x3037, 0x13, 0, 0},
};

static struct reg_value ov5640_setting_30fps_720P_1280_720[] = {
        {0x3035, 0x21, 0, 0}, {0x3036, 0x69, 0, 0}, {0x3c07, 0x07, 0, 0},
        {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0}, {0x3814, 0x31, 0, 0},
        {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0}, {0x3801, 0x00, 0, 0},
        {0x3802, 0x00, 0, 0}, {0x3803, 0xfa, 0, 0}, {0x3804, 0x0a, 0, 0},
        {0x3805, 0x3f, 0, 0}, {0x3806, 0x06, 0, 0}, {0x3807, 0xa9, 0, 0},
        {0x3808, 0x05, 0, 0}, {0x3809, 0x00, 0, 0}, {0x380a, 0x02, 0, 0},
        {0x380b, 0xd0, 0, 0}, {0x380c, 0x07, 0, 0}, {0x380d, 0x64, 0, 0},
        {0x380e, 0x02, 0, 0}, {0x380f, 0xe4, 0, 0}, {0x3813, 0x04, 0, 0},
        {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0}, {0x3709, 0x52, 0, 0},
        {0x370c, 0x03, 0, 0}, {0x3a02, 0x02, 0, 0}, {0x3a03, 0xe0, 0, 0},
        {0x3a14, 0x02, 0, 0}, {0x3a15, 0xe0, 0, 0}, {0x4004, 0x02, 0, 0},
        {0x3002, 0x1c, 0, 0}, {0x3006, 0xc3, 0, 0}, {0x4713, 0x03, 0, 0},
        {0x4407, 0x04, 0, 0}, {0x460b, 0x37, 0, 0}, {0x460c, 0x20, 0, 0},
        {0x4837, 0x16, 0, 0}, {0x3824, 0x04, 0, 0}, {0x5001, 0x83, 0, 0},
        {0x3503, 0x00, 0, 0},
};

static struct reg_value ov5640_setting_15fps_720P_1280_720[] = {
        {0x3035, 0x41, 0, 0}, {0x3036, 0x69, 0, 0}, {0x3c07, 0x07, 0, 0},
        {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0}, {0x3814, 0x31, 0, 0},
        {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0}, {0x3801, 0x00, 0, 0},
        {0x3802, 0x00, 0, 0}, {0x3803, 0xfa, 0, 0}, {0x3804, 0x0a, 0, 0},
        {0x3805, 0x3f, 0, 0}, {0x3806, 0x06, 0, 0}, {0x3807, 0xa9, 0, 0},
        {0x3808, 0x05, 0, 0}, {0x3809, 0x00, 0, 0}, {0x380a, 0x02, 0, 0},
        {0x380b, 0xd0, 0, 0}, {0x380c, 0x07, 0, 0}, {0x380d, 0x64, 0, 0},
        {0x380e, 0x02, 0, 0}, {0x380f, 0xe4, 0, 0}, {0x3813, 0x04, 0, 0},
        {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0}, {0x3709, 0x52, 0, 0},
        {0x370c, 0x03, 0, 0}, {0x3a02, 0x02, 0, 0}, {0x3a03, 0xe0, 0, 0},
        {0x3a14, 0x02, 0, 0}, {0x3a15, 0xe0, 0, 0}, {0x4004, 0x02, 0, 0},
        {0x3002, 0x1c, 0, 0}, {0x3006, 0xc3, 0, 0}, {0x4713, 0x03, 0, 0},
        {0x4407, 0x04, 0, 0}, {0x460b, 0x37, 0, 0}, {0x460c, 0x20, 0, 0},
        {0x4837, 0x16, 0, 0}, {0x3824, 0x04, 0, 0}, {0x5001, 0x83, 0, 0},
        {0x3503, 0x00, 0, 0},
};

static struct reg_value ov5640_setting_30fps_QCIF_176_144[] = {
        {0x3c07, 0x08, 0, 0}, {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0},
        {0x3814, 0x31, 0, 0}, {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0},
        {0x3801, 0x00, 0, 0}, {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0},
        {0x3804, 0x0a, 0, 0}, {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0},
        {0x3807, 0x9b, 0, 0}, {0x3808, 0x00, 0, 0}, {0x3809, 0xb0, 0, 0},
        {0x380a, 0x00, 0, 0}, {0x380b, 0x90, 0, 0}, {0x380c, 0x07, 0, 0},
        {0x380d, 0x68, 0, 0}, {0x380e, 0x03, 0, 0}, {0x380f, 0xd8, 0, 0},
        {0x3813, 0x06, 0, 0}, {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0},
        {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x0b, 0, 0},
        {0x3a03, 0x88, 0, 0}, {0x3a14, 0x0b, 0, 0}, {0x3a15, 0x88, 0, 0},
        {0x4004, 0x02, 0, 0}, {0x3002, 0x1c, 0, 0}, {0x3006, 0xc3, 0, 0},
        {0x4713, 0x03, 0, 0}, {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0},
        {0x460c, 0x22, 0, 0}, {0x4837, 0x22, 0, 0}, {0x3824, 0x02, 0, 0},
        {0x5001, 0xa3, 0, 0}, {0x3034, 0x1a, 0, 0}, {0x3035, 0x11, 0, 0},
        {0x3036, 0x46, 0, 0}, {0x3037, 0x13, 0, 0},
};

static struct reg_value ov5640_setting_15fps_QCIF_176_144[] = {
        {0x3c07, 0x08, 0, 0}, {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0},
        {0x3814, 0x31, 0, 0}, {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0},
        {0x3801, 0x00, 0, 0}, {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0},
        {0x3804, 0x0a, 0, 0}, {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0},
        {0x3807, 0x9b, 0, 0}, {0x3808, 0x00, 0, 0}, {0x3809, 0xb0, 0, 0},
        {0x380a, 0x00, 0, 0}, {0x380b, 0x90, 0, 0}, {0x380c, 0x07, 0, 0},
        {0x380d, 0x68, 0, 0}, {0x380e, 0x03, 0, 0}, {0x380f, 0xd8, 0, 0},
        {0x3813, 0x06, 0, 0}, {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0},
        {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x0b, 0, 0},
        {0x3a03, 0x88, 0, 0}, {0x3a14, 0x0b, 0, 0}, {0x3a15, 0x88, 0, 0},
        {0x4004, 0x02, 0, 0}, {0x3002, 0x1c, 0, 0}, {0x3006, 0xc3, 0, 0},
        {0x4713, 0x03, 0, 0}, {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0},
        {0x460c, 0x22, 0, 0}, {0x4837, 0x22, 0, 0}, {0x3824, 0x02, 0, 0},
        {0x5001, 0xa3, 0, 0}, {0x3034, 0x1a, 0, 0}, {0x3035, 0x21, 0, 0},
        {0x3036, 0x46, 0, 0}, {0x3037, 0x13, 0, 0},
};

static struct reg_value ov5640_setting_30fps_XGA_1024_768[] = {
        {0x3c07, 0x08, 0, 0}, {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0},
        {0x3814, 0x31, 0, 0}, {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0},
        {0x3801, 0x00, 0, 0}, {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0},
        {0x3804, 0x0a, 0, 0}, {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0},
        {0x3807, 0x9b, 0, 0}, {0x3808, 0x04, 0, 0}, {0x3809, 0x00, 0, 0},
        {0x380a, 0x03, 0, 0}, {0x380b, 0x00, 0, 0}, {0x380c, 0x07, 0, 0},
        {0x380d, 0x68, 0, 0}, {0x380e, 0x03, 0, 0}, {0x380f, 0xd8, 0, 0},
        {0x3813, 0x06, 0, 0}, {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0},
        {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x0b, 0, 0},
        {0x3a03, 0x88, 0, 0}, {0x3a14, 0x0b, 0, 0}, {0x3a15, 0x88, 0, 0},
        {0x4004, 0x02, 0, 0}, {0x3002, 0x1c, 0, 0}, {0x3006, 0xc3, 0, 0},
        {0x4713, 0x03, 0, 0}, {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0},
        {0x460c, 0x20, 0, 0}, {0x4837, 0x22, 0, 0}, {0x3824, 0x01, 0, 0},
        {0x5001, 0xa3, 0, 0}, {0x3034, 0x1a, 0, 0}, {0x3035, 0x21, 0, 0},
        {0x3036, 0x69, 0, 0}, {0x3037, 0x13, 0, 0},
};

static struct reg_value ov5640_setting_15fps_XGA_1024_768[] = {
        {0x3c07, 0x08, 0, 0}, {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0},
        {0x3814, 0x31, 0, 0}, {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0},
        {0x3801, 0x00, 0, 0}, {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0},
        {0x3804, 0x0a, 0, 0}, {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0},
        {0x3807, 0x9b, 0, 0}, {0x3808, 0x04, 0, 0}, {0x3809, 0x00, 0, 0},
        {0x380a, 0x03, 0, 0}, {0x380b, 0x00, 0, 0}, {0x380c, 0x07, 0, 0},
        {0x380d, 0x68, 0, 0}, {0x380e, 0x03, 0, 0}, {0x380f, 0xd8, 0, 0},
        {0x3813, 0x06, 0, 0}, {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0},
        {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x0b, 0, 0},
        {0x3a03, 0x88, 0, 0}, {0x3a14, 0x0b, 0, 0}, {0x3a15, 0x88, 0, 0},
        {0x4004, 0x02, 0, 0}, {0x3002, 0x1c, 0, 0}, {0x3006, 0xc3, 0, 0},
        {0x4713, 0x03, 0, 0}, {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0},
        {0x460c, 0x20, 0, 0}, {0x4837, 0x22, 0, 0}, {0x3824, 0x01, 0, 0},
        {0x5001, 0xa3, 0, 0}, {0x3034, 0x1a, 0, 0}, {0x3035, 0x21, 0, 0},
        {0x3036, 0x46, 0, 0}, {0x3037, 0x13, 0, 0},
};


static struct reg_value ov5640_setting_15fps_1080P_1920_1080[] = {
        {0x3c07, 0x07, 0, 0}, {0x3820, 0x40, 0, 0}, {0x3821, 0x06, 0, 0},
        {0x3814, 0x11, 0, 0}, {0x3815, 0x11, 0, 0}, {0x3800, 0x00, 0, 0},
        {0x3801, 0x00, 0, 0}, {0x3802, 0x00, 0, 0}, {0x3803, 0xee, 0, 0},
        {0x3804, 0x0a, 0, 0}, {0x3805, 0x3f, 0, 0}, {0x3806, 0x05, 0, 0},
        {0x3807, 0xc3, 0, 0}, {0x3808, 0x07, 0, 0}, {0x3809, 0x80, 0, 0},
        {0x380a, 0x04, 0, 0}, {0x380b, 0x38, 0, 0}, {0x380c, 0x0b, 0, 0},
        {0x380d, 0x1c, 0, 0}, {0x380e, 0x07, 0, 0}, {0x380f, 0xb0, 0, 0},
        {0x3813, 0x04, 0, 0}, {0x3618, 0x04, 0, 0}, {0x3612, 0x2b, 0, 0},
        {0x3709, 0x12, 0, 0}, {0x370c, 0x00, 0, 0}, {0x3a02, 0x07, 0, 0},
        {0x3a03, 0xae, 0, 0}, {0x3a14, 0x07, 0, 0}, {0x3a15, 0xae, 0, 0},
        {0x4004, 0x06, 0, 0}, {0x3002, 0x1c, 0, 0}, {0x3006, 0xc3, 0, 0},
        {0x4713, 0x02, 0, 0}, {0x4407, 0x0c, 0, 0}, {0x460b, 0x37, 0, 0},
        {0x460c, 0x20, 0, 0}, {0x4837, 0x2c, 0, 0}, {0x3824, 0x01, 0, 0},
        {0x5001, 0x83, 0, 0}, {0x3034, 0x1a, 0, 0}, {0x3035, 0x21, 0, 0},
        {0x3036, 0x69, 0, 0}, {0x3037, 0x13, 0, 0},
};

static struct reg_value ov5640_setting_15fps_QSXGA_2592_1944[] = {
        {0x3c07, 0x07, 0, 0}, {0x3820, 0x40, 0, 0}, {0x3821, 0x06, 0, 0},
        {0x3814, 0x11, 0, 0}, {0x3815, 0x11, 0, 0}, {0x3800, 0x00, 0, 0},
        {0x3801, 0x00, 0, 0}, {0x3802, 0x00, 0, 0}, {0x3803, 0x00, 0, 0},
        {0x3804, 0x0a, 0, 0}, {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0},
        {0x3807, 0x9f, 0, 0}, {0x3808, 0x0a, 0, 0}, {0x3809, 0x20, 0, 0},
        {0x380a, 0x07, 0, 0}, {0x380b, 0x98, 0, 0}, {0x380c, 0x0b, 0, 0},
        {0x380d, 0x1c, 0, 0}, {0x380e, 0x07, 0, 0}, {0x380f, 0xb0, 0, 0},
        {0x3813, 0x04, 0, 0}, {0x3618, 0x04, 0, 0}, {0x3612, 0x2b, 0, 0},
        {0x3709, 0x12, 0, 0}, {0x370c, 0x00, 0, 0}, {0x3a02, 0x07, 0, 0},
        {0x3a03, 0xae, 0, 0}, {0x3a14, 0x07, 0, 0}, {0x3a15, 0xae, 0, 0},
        {0x4004, 0x06, 0, 0}, {0x3002, 0x1c, 0, 0}, {0x3006, 0xc3, 0, 0},
        {0x4713, 0x02, 0, 0}, {0x4407, 0x0c, 0, 0}, {0x460b, 0x37, 0, 0},
        {0x460c, 0x20, 0, 0}, {0x4837, 0x2c, 0, 0}, {0x3824, 0x01, 0, 0},
        {0x5001, 0x83, 0, 0}, {0x3034, 0x1a, 0, 0}, {0x3035, 0x21, 0, 0},
        {0x3036, 0x69, 0, 0}, {0x3037, 0x13, 0, 0},
};

static struct ov5640_mode_info ov5640_mode_info_data[2][ov5640_mode_MAX + 1] = {
        {
                {ov5640_mode_VGA_640_480,      640,  480,
                ov5640_setting_15fps_VGA_640_480,
                ARRAY_SIZE(ov5640_setting_15fps_VGA_640_480)},
                {ov5640_mode_QVGA_320_240,     320,  240,
                ov5640_setting_15fps_QVGA_320_240,
                ARRAY_SIZE(ov5640_setting_15fps_QVGA_320_240)},
                {ov5640_mode_NTSC_720_480,     720,  480,
                ov5640_setting_15fps_NTSC_720_480,
                ARRAY_SIZE(ov5640_setting_15fps_NTSC_720_480)},
                {ov5640_mode_PAL_720_576,      720,  576,
                ov5640_setting_15fps_PAL_720_576,
                ARRAY_SIZE(ov5640_setting_15fps_PAL_720_576)},
                {ov5640_mode_720P_1280_720,   1280,  720,
                ov5640_setting_15fps_720P_1280_720,
                ARRAY_SIZE(ov5640_setting_15fps_720P_1280_720)},
                {ov5640_mode_1080P_1920_1080, 1920, 1080,
                ov5640_setting_15fps_1080P_1920_1080,
                ARRAY_SIZE(ov5640_setting_15fps_1080P_1920_1080)},
                {ov5640_mode_QSXGA_2592_1944, 2592, 1944,
                ov5640_setting_15fps_QSXGA_2592_1944,
                ARRAY_SIZE(ov5640_setting_15fps_QSXGA_2592_1944)},
                {ov5640_mode_QCIF_176_144,     176,  144,
                ov5640_setting_15fps_QCIF_176_144,
                ARRAY_SIZE(ov5640_setting_15fps_QCIF_176_144)},
                {ov5640_mode_XGA_1024_768,    1024,  768,
                ov5640_setting_15fps_XGA_1024_768,
                ARRAY_SIZE(ov5640_setting_15fps_XGA_1024_768)},
        },
        {
                {ov5640_mode_VGA_640_480,      640,  480,
                ov5640_setting_30fps_VGA_640_480,
                ARRAY_SIZE(ov5640_setting_30fps_VGA_640_480)},
                {ov5640_mode_QVGA_320_240,     320,  240,
                ov5640_setting_30fps_QVGA_320_240,
                ARRAY_SIZE(ov5640_setting_30fps_QVGA_320_240)},
                {ov5640_mode_NTSC_720_480,     720,  480,
                ov5640_setting_30fps_NTSC_720_480,
                ARRAY_SIZE(ov5640_setting_30fps_NTSC_720_480)},
                {ov5640_mode_PAL_720_576,      720,  576,
                ov5640_setting_30fps_PAL_720_576,
                ARRAY_SIZE(ov5640_setting_30fps_PAL_720_576)},
                {ov5640_mode_720P_1280_720,   1280,  720,
                ov5640_setting_30fps_720P_1280_720,
                ARRAY_SIZE(ov5640_setting_30fps_720P_1280_720)},
                {ov5640_mode_1080P_1920_1080, 0, 0, NULL, 0},
                {ov5640_mode_QSXGA_2592_1944, 0, 0, NULL, 0},
                {ov5640_mode_QCIF_176_144,     176,  144,
                ov5640_setting_30fps_QCIF_176_144,
                ARRAY_SIZE(ov5640_setting_30fps_QCIF_176_144)},
                {ov5640_mode_XGA_1024_768,    1024,  768,
                ov5640_setting_30fps_XGA_1024_768,
                ARRAY_SIZE(ov5640_setting_30fps_XGA_1024_768)},
        },
};

static s32 ov5640_write_reg(u16 reg, u8 val)
{
        u8 au8Buf[3] = {0};

        au8Buf[0] = reg >> 8;
        au8Buf[1] = reg & 0xff;
        au8Buf[2] = val;

        if (i2c_master_send(ov5640_data.i2c_client, au8Buf, 3) < 0) {
                pr_err("%s:write reg error:reg=%x,val=%x\n",
                        __func__, reg, val);
                return -1;
        }

        return 0;
}

static s32 ov5640_read_reg(u16 reg, u8 *val)
{
        u8 au8RegBuf[2] = {0};
        u8 u8RdVal = 0;

        au8RegBuf[0] = reg >> 8;
        au8RegBuf[1] = reg & 0xff;

        if (2 != i2c_master_send(ov5640_data.i2c_client, au8RegBuf, 2)) {
                pr_err("%s:write reg error:reg=%x\n",
                                __func__, reg);
                return -1;
        }

        if (1 != i2c_master_recv(ov5640_data.i2c_client, &u8RdVal, 1)) {
                pr_err("%s:read reg error:reg=%x,val=%x\n",
                                __func__, reg, u8RdVal);
                return -1;
        }

        *val = u8RdVal;

        return u8RdVal;
}

static void ov5640_soft_reset(void)
{
        /* sysclk from pad */
        ov5640_write_reg(0x3103, 0x11);

        /* software reset */
        ov5640_write_reg(0x3008, 0x82);

        /* delay at least 5ms */
        msleep(10);
}

/* set sensor driver capability
 * 0x302c[7:6] - strength
        00     - 1x
        01     - 2x
        10     - 3x
        11     - 4x
 */
static int ov5640_driver_capability(int strength)
{
        u8 temp = 0;

        if (strength > 4 || strength < 1) {
                pr_err("The valid driver capability of ov5640 is 1x~4x\n");
                return -EINVAL;
        }

        ov5640_read_reg(0x302c, &temp);

        temp &= ~0xc0;	/* clear [7:6] */
        temp |= ((strength - 1) << 6);	/* set [7:6] */

        ov5640_write_reg(0x302c, temp);

        return 0;
}

/* calculate sysclk */
static int ov5640_get_sysclk(void)
{
        int xvclk = ov5640_data.mclk / 10000;
        int sysclk;
        int temp1, temp2;
        int Multiplier, PreDiv, VCO, SysDiv, Pll_rdiv, Bit_div2x, sclk_rdiv;
        int sclk_rdiv_map[] = {1, 2, 4, 8};
        u8 regval = 0;

        temp1 = ov5640_read_reg(0x3034, &regval);
        temp2 = temp1 & 0x0f;
        if (temp2 == 8 || temp2 == 10) {
                Bit_div2x = temp2 / 2;
        } else {
                pr_err("ov5640: unsupported bit mode %d\n", temp2);
                return -1;
        }

        temp1 = ov5640_read_reg(0x3035, &regval);
        SysDiv = temp1 >> 4;
        if (SysDiv == 0)
                SysDiv = 16;

        temp1 = ov5640_read_reg(0x3036, &regval);
        Multiplier = temp1;
        temp1 = ov5640_read_reg(0x3037, &regval);
        PreDiv = temp1 & 0x0f;
        Pll_rdiv = ((temp1 >> 4) & 0x01) + 1;

        temp1 = ov5640_read_reg(0x3108, &regval);
        temp2 = temp1 & 0x03;

        sclk_rdiv = sclk_rdiv_map[temp2];
        VCO = xvclk * Multiplier / PreDiv;
        sysclk = VCO / SysDiv / Pll_rdiv * 2 / Bit_div2x / sclk_rdiv;

        return sysclk;
}

/* read HTS from register settings */
static int ov5640_get_HTS(void)
{
        int HTS;
        u8 temp = 0;

        HTS = ov5640_read_reg(0x380c, &temp);
        HTS = (HTS<<8) + ov5640_read_reg(0x380d, &temp);
        return HTS;
}

/* read VTS from register settings */
static int ov5640_get_VTS(void)
{
        int VTS;
        u8 temp = 0;

        VTS = ov5640_read_reg(0x380e, &temp);
        VTS = (VTS<<8) + ov5640_read_reg(0x380f, &temp);

        return VTS;
}

/* write VTS to registers */
static int ov5640_set_VTS(int VTS)
{
        int temp;

        temp = VTS & 0xff;
        ov5640_write_reg(0x380f, temp);

        temp = VTS>>8;
        ov5640_write_reg(0x380e, temp);
        return 0;
}

/* read shutter, in number of line period */
static int ov5640_get_shutter(void)
{
        int shutter;
        u8 regval;

        shutter = (ov5640_read_reg(0x03500, &regval) & 0x0f);

        shutter = (shutter<<8) + ov5640_read_reg(0x3501, &regval);
        shutter = (shutter<<4) + (ov5640_read_reg(0x3502, &regval)>>4);

        return shutter;
}

/* write shutter, in number of line period */
static int ov5640_set_shutter(int shutter)
{
        int temp;

        shutter = shutter & 0xffff;
        temp = shutter & 0x0f;
        temp = temp<<4;
        ov5640_write_reg(0x3502, temp);

        temp = shutter & 0xfff;
        temp = temp>>4;
        ov5640_write_reg(0x3501, temp);

        temp = shutter>>12;
        ov5640_write_reg(0x3500, temp);

        return 0;
}

/* read gain, 16 = 1x */
static int ov5640_get_gain16(void)
{
        int gain16;
        u8 regval;

        gain16 = ov5640_read_reg(0x350a, &regval) & 0x03;
        gain16 = (gain16<<8) + ov5640_read_reg(0x350b, &regval);

        return gain16;
}

/* write gain, 16 = 1x */
static int ov5640_set_gain16(int gain16)
{
        int temp;

        gain16 = gain16 & 0x3ff;
        temp = gain16 & 0xff;

        ov5640_write_reg(0x350b, temp);
        temp = gain16>>8;

        ov5640_write_reg(0x350a, temp);
        return 0;
}

/* get banding filter value */
static int ov5640_get_light_freq(void)
{
        int temp, temp1, light_frequency;
        u8 regval;

        temp = ov5640_read_reg(0x3c01, &regval);
        if (temp & 0x80) {
                /* manual */
                temp1 = ov5640_read_reg(0x3c00, &regval);
                if (temp1 & 0x04) {
                        /* 50Hz */
                        light_frequency = 50;
                } else {
                        /* 60Hz */
                        light_frequency = 60;
                }
        } else {
                /* auto */
                temp1 = ov5640_read_reg(0x3c0c, &regval);
                if (temp1 & 0x01) {
                        /* 50Hz */
                        light_frequency = 50;
                } else {
                        /* 60Hz */
                        light_frequency = 60;
                }
        }

        return light_frequency;
}

static void ov5640_set_bandingfilter(void)
{
        int prev_VTS;
        int band_step60, max_band60, band_step50, max_band50;

        /* read preview PCLK */
        prev_sysclk = ov5640_get_sysclk();

        /* read preview HTS */
        prev_HTS = ov5640_get_HTS();

        /* read preview VTS */
        prev_VTS = ov5640_get_VTS();

        /* calculate banding filter */
        /* 60Hz */
        band_step60 = prev_sysclk * 100/prev_HTS * 100/120;
        ov5640_write_reg(0x3a0a, (band_step60 >> 8));
        ov5640_write_reg(0x3a0b, (band_step60 & 0xff));

        max_band60 = (int)((prev_VTS-4)/band_step60);
        ov5640_write_reg(0x3a0d, max_band60);

        /* 50Hz */
        band_step50 = prev_sysclk * 100/prev_HTS;
        ov5640_write_reg(0x3a08, (band_step50 >> 8));
        ov5640_write_reg(0x3a09, (band_step50 & 0xff));

        max_band50 = (int)((prev_VTS-4)/band_step50);
        ov5640_write_reg(0x3a0e, max_band50);
}

/* stable in high */
static int ov5640_set_AE_target(int target)
{
        int fast_high, fast_low;

        AE_low = target * 23 / 25; /* 0.92 */
        AE_high = target * 27 / 25; /* 1.08 */
        fast_high = AE_high << 1;

        if (fast_high > 255)
                fast_high = 255;
        fast_low = AE_low >> 1;

        ov5640_write_reg(0x3a0f, AE_high);
        ov5640_write_reg(0x3a10, AE_low);
        ov5640_write_reg(0x3a1b, AE_high);
        ov5640_write_reg(0x3a1e, AE_low);
        ov5640_write_reg(0x3a11, fast_high);
        ov5640_write_reg(0x3a1f, fast_low);

        return 0;
}

/* enable = 0 to turn off night mode
   enable = 1 to turn on night mode */
static int ov5640_set_night_mode(int enable)
{
        u8 mode;

        ov5640_read_reg(0x3a00, &mode);

        if (enable) {
                /* night mode on */
                mode |= 0x04;
                ov5640_write_reg(0x3a00, mode);
        } else {
                /* night mode off */
                mode &= 0xfb;
                ov5640_write_reg(0x3a00, mode);
        }

        return 0;
}

/* enable = 0 to turn off AEC/AGC
   enable = 1 to turn on AEC/AGC */
void ov5640_turn_on_AE_AG(int enable)
{
        u8 ae_ag_ctrl;

        ov5640_read_reg(0x3503, &ae_ag_ctrl);
        if (enable) {
                /* turn on auto AE/AG */
                ae_ag_ctrl = ae_ag_ctrl & ~(0x03);
        } else {
                /* turn off AE/AG */
                ae_ag_ctrl = ae_ag_ctrl | 0x03;
        }
        ov5640_write_reg(0x3503, ae_ag_ctrl);
}

/* download ov5640 settings to sensor through i2c */
static int ov5640_download_firmware(struct reg_value *pModeSetting, s32 ArySize)
{
        register u32 Delay_ms = 0;
        register u16 RegAddr = 0;
        register u8 Mask = 0;
        register u8 Val = 0;
        u8 RegVal = 0;
        int i, retval = 0;

        for (i = 0; i < ArySize; ++i, ++pModeSetting) {
                Delay_ms = pModeSetting->u32Delay_ms;
                RegAddr = pModeSetting->u16RegAddr;
                Val = pModeSetting->u8Val;
                Mask = pModeSetting->u8Mask;

                if (Mask) {
                        retval = ov5640_read_reg(RegAddr, &RegVal);
                        if (retval < 0)
                                goto err;

                        RegVal &= ~(u8)Mask;
                        Val &= Mask;
                        Val |= RegVal;
                }

                retval = ov5640_write_reg(RegAddr, Val);
                if (retval < 0)
                        goto err;

                if (Delay_ms)
                        msleep(Delay_ms);
        }
err:
        return retval;
}


static int ov5640_init_mode(void)
{
        struct reg_value *pModeSetting = NULL;
        int ArySize = 0, retval = 0;

        ov5640_soft_reset();

        pModeSetting = ov5640_global_init_setting;
        ArySize = ARRAY_SIZE(ov5640_global_init_setting);
        retval = ov5640_download_firmware(pModeSetting, ArySize);
        if (retval < 0)
                goto err;

        pModeSetting = ov5640_init_setting_30fps_VGA;
        ArySize = ARRAY_SIZE(ov5640_init_setting_30fps_VGA);
        retval = ov5640_download_firmware(pModeSetting, ArySize);
        if (retval < 0)
                goto err;

        /* change driver capability to 2x according to validation board.
         * if the image is not stable, please increase the driver strength.
         */
        ov5640_driver_capability(2);
        ov5640_set_bandingfilter();
        ov5640_set_AE_target(AE_Target);
        ov5640_set_night_mode(night_mode);

        /* skip 9 vysnc: start capture at 10th vsync */
        msleep(300);

        /* turn off night mode */
        night_mode = 0;
        ov5640_data.pix.width = 640;
        ov5640_data.pix.height = 480;
err:
        return retval;
}

/* change to or back to subsampling mode set the mode directly
 * image size below 1280 * 960 is subsampling mode */
static int ov5640_change_mode_direct(enum ov5640_frame_rate frame_rate,
                            enum ov5640_mode mode)
{
        struct reg_value *pModeSetting = NULL;
        s32 ArySize = 0;
        int retval = 0;

        if (mode > ov5640_mode_MAX || mode < ov5640_mode_MIN) {
                pr_err("Wrong ov5640 mode detected!\n");
                return -1;
        }


        pModeSetting = ov5640_mode_info_data[frame_rate][mode].init_data_ptr;
        ArySize =
                ov5640_mode_info_data[frame_rate][mode].init_data_size;

        ov5640_data.pix.width = ov5640_mode_info_data[frame_rate][mode].width;
        ov5640_data.pix.height = ov5640_mode_info_data[frame_rate][mode].height;

        if (ov5640_data.pix.width == 0 || ov5640_data.pix.height == 0 ||
          pModeSetting == NULL || ArySize == 0)
               return -EINVAL;

        /* set ov5640 to subsampling mode */
        retval = ov5640_download_firmware(pModeSetting, ArySize);

        printk("###### video width X height is %dx%d#########\n",ov5640_data.pix.width,ov5640_data.pix.height);


#if 1
        /* turn on AE AG for subsampling mode, in case the firmware didn't */
        ov5640_turn_on_AE_AG(1);

        /* calculate banding filter */
        ov5640_set_bandingfilter();

        /* set AE target */
        ov5640_set_AE_target(AE_Target);

        /* update night mode setting */
        ov5640_set_night_mode(night_mode);

        /* skip 9 vysnc: start capture at 10th vsync */
        if (mode == ov5640_mode_XGA_1024_768 && frame_rate == ov5640_30_fps) {
                pr_warning("ov5640: actual frame rate of XGA is 22.5fps\n");
                /* 1/22.5 * 9*/
                msleep(400);
                return retval;
        }

        if (frame_rate == ov5640_15_fps) {
                /* 1/15 * 9*/
                msleep(600);
        } else if (frame_rate == ov5640_30_fps) {
                /* 1/30 * 9*/
                msleep(300);
        }
#endif
        return retval;
}

/* change to scaling mode go through exposure calucation
 * image size above 1280 * 960 is scaling mode */
static int ov5640_change_mode_exposure_calc(enum ov5640_frame_rate frame_rate,
                            enum ov5640_mode mode)
{
        int prev_shutter, prev_gain16, average;
        int cap_shutter, cap_gain16;
        int cap_sysclk, cap_HTS, cap_VTS;
        int light_freq, cap_bandfilt, cap_maxband;
        long cap_gain16_shutter;
        u8 temp;
        struct reg_value *pModeSetting = NULL;
        s32 ArySize = 0;
        int retval = 0;

        /* check if the input mode and frame rate is valid */
        pModeSetting =
                ov5640_mode_info_data[frame_rate][mode].init_data_ptr;
        ArySize =
                ov5640_mode_info_data[frame_rate][mode].init_data_size;

        ov5640_data.pix.width =
                ov5640_mode_info_data[frame_rate][mode].width;
        ov5640_data.pix.height =
                ov5640_mode_info_data[frame_rate][mode].height;

        if (ov5640_data.pix.width == 0 || ov5640_data.pix.height == 0 ||
                pModeSetting == NULL || ArySize == 0)
                return -EINVAL;

        /* read preview shutter */
        prev_shutter = ov5640_get_shutter();

        /* read preview gain */
        prev_gain16 = ov5640_get_gain16();

        /* get average */
        average = ov5640_read_reg(0x56a1, &temp);

        /* turn off night mode for capture */
        ov5640_set_night_mode(0);

        /* turn off overlay */
        ov5640_write_reg(0x3022, 0x06);

        /* Write capture setting */
        retval = ov5640_download_firmware(pModeSetting, ArySize);
        if (retval < 0)
                goto err;

        /* turn off AE AG when capture image. */
        ov5640_turn_on_AE_AG(0);

        /* read capture VTS */
        cap_VTS = ov5640_get_VTS();
        cap_HTS = ov5640_get_HTS();
        cap_sysclk = ov5640_get_sysclk();

        /* calculate capture banding filter */
        light_freq = ov5640_get_light_freq();
        if (light_freq == 60) {
                /* 60Hz */
                cap_bandfilt = cap_sysclk * 100 / cap_HTS * 100 / 120;
        } else {
                /* 50Hz */
                cap_bandfilt = cap_sysclk * 100 / cap_HTS;
        }
        cap_maxband = (int)((cap_VTS - 4)/cap_bandfilt);
        /* calculate capture shutter/gain16 */
        if (average > AE_low && average < AE_high) {
                /* in stable range */
                cap_gain16_shutter =
                        prev_gain16 * prev_shutter * cap_sysclk/prev_sysclk *
                        prev_HTS/cap_HTS * AE_Target / average;
        } else {
                cap_gain16_shutter =
                        prev_gain16 * prev_shutter * cap_sysclk/prev_sysclk *
                        prev_HTS/cap_HTS;
        }

        /* gain to shutter */
        if (cap_gain16_shutter < (cap_bandfilt * 16)) {
                /* shutter < 1/100 */
                cap_shutter = cap_gain16_shutter/16;
                if (cap_shutter < 1)
                        cap_shutter = 1;
                cap_gain16 = cap_gain16_shutter/cap_shutter;
                if (cap_gain16 < 16)
                        cap_gain16 = 16;
        } else {
                if (cap_gain16_shutter > (cap_bandfilt*cap_maxband*16)) {
                        /* exposure reach max */
                        cap_shutter = cap_bandfilt*cap_maxband;
                        cap_gain16 = cap_gain16_shutter / cap_shutter;
                } else {
                        /* 1/100 < cap_shutter =< max, cap_shutter = n/100 */
                        cap_shutter =
                                ((int)(cap_gain16_shutter/16/cap_bandfilt))
                                * cap_bandfilt;
                        cap_gain16 = cap_gain16_shutter / cap_shutter;
                }
        }

        /* write capture gain */
        ov5640_set_gain16(cap_gain16);

        /* write capture shutter */
        if (cap_shutter > (cap_VTS - 4)) {
                cap_VTS = cap_shutter + 4;
                ov5640_set_VTS(cap_VTS);
        }

        ov5640_set_shutter(cap_shutter);

        /* skip 2 vysnc: start capture at 3rd vsync
         * frame rate of QSXGA and 1080P is 7.5fps: 1/7.5 * 2
         */
        pr_warning("ov5640: the actual frame rate of %s is 7.5fps\n",
                mode == ov5640_mode_1080P_1920_1080 ? "1080P" : "QSXGA");
        msleep(267);
err:
        return retval;
}

static int ov5640_change_mode(enum ov5640_frame_rate frame_rate,
                            enum ov5640_mode mode)
{
        int retval = 0;

        if (mode > ov5640_mode_MAX || mode < ov5640_mode_MIN) {
                pr_err("Wrong ov5640 mode detected!\n");
                return -1;
        }

        if (mode == ov5640_mode_1080P_1920_1080 ||
                        mode == ov5640_mode_QSXGA_2592_1944) {
                /* change to scaling mode go through exposure calucation
                 * image size above 1280 * 960 is scaling mode */
                retval = ov5640_change_mode_exposure_calc(frame_rate, mode);
        } else {
                /* change back to subsampling modem download firmware directly
                 * image size below 1280 * 960 is subsampling mode */
                retval = ov5640_change_mode_direct(frame_rate, mode);
        }

        return retval;
}


static int ov5640_power(int flag);
static int i2cc_get_reg(struct i2c_client *client,
                        unsigned short reg, unsigned char *value);
/*!
 * ioctl_dev_init - V4L2 sensor interface handler for vidioc_int_dev_init_num
 * @s: pointer to standard V4L2 device structure
 *
 * Initialise the device when slave attaches to the master.
 */
static int ioctl_dev_init(struct v4l2_int_device *s)
{

     //   struct sensor_data *sensor =(struct sensor_data *) s->priv;
        u32 tgt_xclk;	/* target xclk */
        u32 tgt_fps;	/* target frames per secound */
        enum ov5640_frame_rate frame_rate;
        int ret;
        u8 pid,version;
#if  1
        /* add by cym 20121121 */
        ov5640_power(1);
        /* end add */

        // check and show product ID and manufacturer ID
        ov5640_read_reg(0x300a, &pid);
        ov5640_read_reg(0x300b, &version);

        printk("%s: version = 0x%x%x\n", __FUNCTION__, pid, version);

        if (OV5640 != VERSION(pid, version)) {
            OV_ERR("ov5640 probed failed!!\n");
            return (-ENODEV);
        }

       // priv->model = 1;//cym V4L2_IDENT_OV5640;

#endif


        ov5640_data.on = true;

        /* mclk */
        tgt_xclk = ov5640_data.mclk;
        tgt_xclk = min(tgt_xclk, (u32)OV5640_XCLK_MAX);
        tgt_xclk = max(tgt_xclk, (u32)OV5640_XCLK_MIN);
        ov5640_data.mclk = tgt_xclk;

        pr_debug("   Setting mclk to %d MHz\n", tgt_xclk / 1000000);

        //set_mclk_rate(&ov5640_data.mclk, ov5640_data.mclk_source);

        /* Default camera frame rate is set in probe */
      //  tgt_fps = sensor->streamcap.timeperframe.denominator /
        //          sensor->streamcap.timeperframe.numerator;

          tgt_fps = ov5640_data.streamcap.timeperframe.denominator /
                    ov5640_data.streamcap.timeperframe.numerator;


        if (tgt_fps == 15)
                frame_rate = ov5640_15_fps;
        else if (tgt_fps == 30)
                frame_rate = ov5640_30_fps;
        else
                return -EINVAL; /* Only support 15fps or 30fps now. */

        ret = ov5640_init_mode();
        return ret;
}









#endif

enum ov5640_resolution {
    RESV_VGA = 1,
    RESV_XGA,
    RESV_720P,
    RESV_SXGA,
    RESV_UXGA,
    RESV_QXGA,
    RESV_QSXGA,
};


struct regval {
    unsigned short reg;
    unsigned char  val;
};

struct ov5640_color_format {
    enum v4l2_mbus_pixelcode code;
    enum v4l2_colorspace colorspace;
};

struct ov5640_win_size {
    char *name;
    enum ov5640_resolution resv;
    unsigned int  width;
    unsigned int  height;
    const struct regval *regs;
};


struct ov5640_priv {
    struct v4l2_subdev  subdev;
    const struct ov5640_color_format *cfmt;
    const struct ov5640_win_size *win;
    int  model;
    int brightness;
    int contrast;
    int saturation;
    int hue;
    int exposure;
    int sharpness;
    int colorfx;
    int flip_flag;
};


static const struct regval ov5640_init_regs[] = {
    /* for the setting , 24M Mlck input and 24M Plck output */
{0x3103, 0x11},
{0x3008, 0x82}, /* soft reset */
{0x3008, 0x42},
{0x3103, 0x03}, /* input clock, from PLL */
{0x3017, 0xff}, /* d[9:0] pins I/O ctrl */
{0x3018, 0xff},
/* system control */
{0x3034, 0x1a}, /* MIPI 10-bit mode */
{0x3035, 0x11}, /* clock, PLL sets */
{0x3036, 0x46},
{0x3037, 0x13},
{0x3108, 0x01}, /* SCCB CLK root divider */
{0x3630, 0x36},
{0x3631, 0x0e},
{0x3632, 0xe2},
{0x3633, 0x12},
{0x3621, 0xe0},
{0x3704, 0xa0},
{0x3703, 0x5a},
{0x3715, 0x78},
{0x3717, 0x01},
{0x370b, 0x60},
{0x3705, 0x1a},
{0x3905, 0x02},
{0x3906, 0x10},
{0x3901, 0x0a},
{0x3731, 0x12},
{0x3600, 0x08},
{0x3601, 0x33},
{0x302d, 0x60},
{0x3620, 0x52},
{0x371b, 0x20},
{0x471c, 0x50},
{0x3a13, 0x43},
{0x3a18, 0x00},
{0x3a19, 0xf8},
{0x3635, 0x13},
{0x3636, 0x03},
{0x3634, 0x40},
{0x3622, 0x01},

{0x3c01, 0x34},  /* 50/60HZ detector */
{0x3c04, 0x28},
{0x3c05, 0x98},
{0x3c06, 0x00},
{0x3c07, 0x08},
{0x3c08, 0x00},
{0x3c09, 0x1c},
{0x3c0a, 0x9c},
{0x3c0b, 0x40},

{0x3820, 0x41}, /* mirror and flip */
{0x3821, 0x07},

{0x3814, 0x31}, /* image windowing */
{0x3815, 0x31},
{0x3800, 0x00},
{0x3801, 0x00},
{0x3802, 0x00},
{0x3803, 0x04},
{0x3804, 0x0a},
{0x3805, 0x3f},
{0x3806, 0x07},
{0x3807, 0x9b},
{0x3808, 0x02}, /* 0x280==640 */
{0x3809, 0x80},
{0x380a, 0x01}, /* 0x1e0==480 */
{0x380b, 0xe0},
{0x380c, 0x07},
{0x380d, 0x68},
{0x380e, 0x03},
{0x380f, 0xd8},
{0x3810, 0x00},
{0x3811, 0x10},
{0x3812, 0x00},
{0x3813, 0x06},

{0x3618, 0x00},
{0x3612, 0x29},
{0x3708, 0x64},
{0x3709, 0x52},
{0x370c, 0x03},

{0x3a02, 0x03}, /* AEC/AGC control */
{0x3a03, 0xd8},
{0x3a08, 0x01},
{0x3a09, 0x27},
{0x3a0a, 0x00},
{0x3a0b, 0xf6},
{0x3a0e, 0x03},
{0x3a0d, 0x04},
{0x3a14, 0x03},
{0x3a15, 0xd8},

{0x4001, 0x02}, /* BLC start line */
{0x4004, 0x02}, /* BLC line number */
{0x3000, 0x00}, /* block enable */
{0x3002, 0x1c},
{0x3004, 0xff}, /* clock enable */
{0x3006, 0xc3},
{0x300e, 0x58},
{0x302e, 0x00},
{0x4300, 0x32}, /* format ctrl: YUV422 UYVY */
{0x501f, 0x00}, /* format MUX ctrl: ISP YUV422 */
{0x4713, 0x03}, /* jpeg mode select: mode 3 */
{0x4407, 0x04}, /* jpeg ctrl */
{0x440e, 0x00},
{0x460b, 0x35}, /* VFIFO ctrl */
{0x460c, 0x22},
{0x3824, 0x02},
{0x5000, 0xa7}, /* ISP top ctrl */
{0x5001, 0xa3},

{0x5180, 0xff}, /* AWB ctrl */
{0x5181, 0xf2},
{0x5182, 0x00},
{0x5183, 0x14},
{0x5184, 0x25},
{0x5185, 0x24},
{0x5186, 0x09},
{0x5187, 0x09},
{0x5188, 0x09},
{0x5189, 0x75},
{0x518a, 0x54},
{0x518b, 0xe0},
{0x518c, 0xb2},
{0x518d, 0x42},
{0x518e, 0x3d},
{0x518f, 0x56},
{0x5190, 0x46},
{0x5191, 0xf8},
{0x5192, 0x04},
{0x5193, 0x70},
{0x5194, 0xf0},
{0x5195, 0xf0},
{0x5196, 0x03},
{0x5197, 0x01},
{0x5198, 0x04},
{0x5199, 0x12},
{0x519a, 0x04},
{0x519b, 0x00},
{0x519c, 0x06},
{0x519d, 0x82},
{0x519e, 0x38},

{0x5381, 0x1e}, /* Color matrix */
{0x5382, 0x5b},
{0x5383, 0x08},
{0x5384, 0x0a},
{0x5385, 0x7e},
{0x5386, 0x88},
{0x5387, 0x7c},
{0x5388, 0x6c},
{0x5389, 0x10},
{0x538a, 0x01},
{0x538b, 0x98},

{0x5300, 0x08}, /* Color interpolation */
{0x5301, 0x30},
{0x5302, 0x10},
{0x5303, 0x00},
{0x5304, 0x08},
{0x5305, 0x30},
{0x5306, 0x08},
{0x5307, 0x16},
{0x5309, 0x08},
{0x530a, 0x30},
{0x530b, 0x04},
{0x530c, 0x06},

{0x5480, 0x01}, /* gamma ctrl */
{0x5481, 0x08},
{0x5482, 0x14},
{0x5483, 0x28},
{0x5484, 0x51},
{0x5485, 0x65},
{0x5486, 0x71},
{0x5487, 0x7d},
{0x5488, 0x87},
{0x5489, 0x91},
{0x548a, 0x9a},
{0x548b, 0xaa},
{0x548c, 0xb8},
{0x548d, 0xcd},
{0x548e, 0xdd},
{0x548f, 0xea},
{0x5490, 0x1d},

{0x5580, 0x02}, /* special digital effects(SDE) */
{0x5583, 0x40},
{0x5584, 0x10},
{0x5589, 0x10},
{0x558a, 0x00},
{0x558b, 0xf8},

{0x5800, 0x23}, /* LENC ctrl */
{0x5801, 0x14},
{0x5802, 0x0f},
{0x5803, 0x0f},
{0x5804, 0x12},
{0x5805, 0x26},
{0x5806, 0x0c},
{0x5807, 0x08},
{0x5808, 0x05},
{0x5809, 0x05},
{0x580a, 0x08},
{0x580b, 0x0d},
{0x580c, 0x08},
{0x580d, 0x03},
{0x580e, 0x00},
{0x580f, 0x00},
{0x5810, 0x03},
{0x5811, 0x09},
{0x5812, 0x07},
{0x5813, 0x03},
{0x5814, 0x00},
{0x5815, 0x01},
{0x5816, 0x03},
{0x5817, 0x08},
{0x5818, 0x0d},
{0x5819, 0x08},
{0x581a, 0x05},
{0x581b, 0x06},
{0x581c, 0x08},
{0x581d, 0x0e},
{0x581e, 0x29},
{0x581f, 0x17},
{0x5820, 0x11},
{0x5821, 0x11},
{0x5822, 0x15},
{0x5823, 0x28},
{0x5824, 0x46},
{0x5825, 0x26},
{0x5826, 0x08},
{0x5827, 0x26},
{0x5828, 0x64},
{0x5829, 0x26},
{0x582a, 0x24},
{0x582b, 0x22},
{0x582c, 0x24},
{0x582d, 0x24},
{0x582e, 0x06},
{0x582f, 0x22},
{0x5830, 0x40},
{0x5831, 0x42},
{0x5832, 0x24},
{0x5833, 0x26},
{0x5834, 0x24},
{0x5835, 0x22},
{0x5836, 0x22},
{0x5837, 0x26},
{0x5838, 0x44},
{0x5839, 0x24},
{0x583a, 0x26},
{0x583b, 0x28},
{0x583c, 0x42},
{0x583d, 0xce},
{0x5025, 0x00},

{0x3a0f, 0x30}, /* AEC functions */
{0x3a10, 0x28},
{0x3a1b, 0x30},
{0x3a1e, 0x26},
{0x3a11, 0x60},
{0x3a1f, 0x14},

{0x3008, 0x02}, /* soft reset/pwd default value */
{0x3035, 0x21}, /* SC PLL ctrl */

{0x3c01, 0xb4}, /* Band, 0x50Hz */
{0x3c00, 0x04},

{0x3a19, 0x7c}, /* gain ceiling */

{0x5800, 0x2c}, /* OV5640 LENC setting */
{0x5801, 0x17},
{0x5802, 0x11},
{0x5803, 0x11},
{0x5804, 0x15},
{0x5805, 0x29},
{0x5806, 0x08},
{0x5807, 0x06},
{0x5808, 0x04},
{0x5809, 0x04},
{0x580a, 0x05},
{0x580b, 0x07},
{0x580c, 0x06},
{0x580d, 0x03},
{0x580e, 0x01},
{0x580f, 0x01},
{0x5810, 0x03},
{0x5811, 0x06},
{0x5812, 0x06},
{0x5813, 0x02},
{0x5814, 0x01},
{0x5815, 0x01},
{0x5816, 0x04},
{0x5817, 0x07},
{0x5818, 0x06},
{0x5819, 0x07},
{0x581a, 0x06},
{0x581b, 0x06},
{0x581c, 0x06},
{0x581d, 0x0e},
{0x581e, 0x31},
{0x581f, 0x12},
{0x5820, 0x11},
{0x5821, 0x11},
{0x5822, 0x11},
{0x5823, 0x2f},
{0x5824, 0x12},
{0x5825, 0x25},
{0x5826, 0x39},
{0x5827, 0x29},
{0x5828, 0x27},
{0x5829, 0x39},
{0x582a, 0x26},
{0x582b, 0x33},
{0x582c, 0x24},
{0x582d, 0x39},
{0x582e, 0x28},
{0x582f, 0x21},
{0x5830, 0x40},
{0x5831, 0x21},
{0x5832, 0x17},
{0x5833, 0x17},
{0x5834, 0x15},
{0x5835, 0x11},
{0x5836, 0x24},
{0x5837, 0x27},
{0x5838, 0x26},
{0x5839, 0x26},
{0x583a, 0x26},
{0x583b, 0x28},
{0x583c, 0x14},
{0x583d, 0xee},
{0x4005, 0x1a}, /* BLC always update */

{0x5381, 0x26}, /* color matrix ctrl */
{0x5382, 0x50},
{0x5383, 0x0c},
{0x5384, 0x09},
{0x5385, 0x74},
{0x5386, 0x7d},
{0x5387, 0x7e},
{0x5388, 0x75},
{0x5389, 0x09},
{0x538b, 0x98},
{0x538a, 0x01},

{0x5580, 0x02}, /* (SDE)UVAdjust Auto Mode */
{0x5588, 0x01},
{0x5583, 0x40},
{0x5584, 0x10},
{0x5589, 0x0f},
{0x558a, 0x00},
{0x558b, 0x3f},

{0x5308, 0x25}, /* De-Noise, 0xAuto */
{0x5304, 0x08},
{0x5305, 0x30},
{0x5306, 0x10},
{0x5307, 0x20},

{0x5180, 0xff}, /* awb ctrl */
{0x5181, 0xf2},
{0x5182, 0x11},
{0x5183, 0x14},
{0x5184, 0x25},
{0x5185, 0x24},
{0x5186, 0x10},
{0x5187, 0x12},
{0x5188, 0x10},
{0x5189, 0x80},
{0x518a, 0x54},
{0x518b, 0xb8},
{0x518c, 0xb2},
{0x518d, 0x42},
{0x518e, 0x3a},
{0x518f, 0x56},
{0x5190, 0x46},
{0x5191, 0xf0},
{0x5192, 0xf},
{0x5193, 0x70},
{0x5194, 0xf0},
{0x5195, 0xf0},
{0x5196, 0x3},
{0x5197, 0x1},
{0x5198, 0x6},
{0x5199, 0x62},
{0x519a, 0x4},
{0x519b, 0x0},
{0x519c, 0x4},
{0x519d, 0xe7},
{0x519e, 0x38},

};

static const struct regval ov5640_qsxga_regs[] = {
{0x3820, 0x40},  /* diff. init */
{0x3821, 0x06},
{0x3814, 0x11}, /* image windowing */
{0x3815, 0x11},
{0x3803, 0x00},
{0x3807, 0x9f},
{0x3808, 0x0a}, /* 0x0a20==2592 */
{0x3809, 0x20},
{0x380a, 0x07}, /* 0x798==1944 */
{0x380b, 0x98},
{0x380c, 0x0b},
{0x380d, 0x1c},
{0x380e, 0x07},
{0x380f, 0xb0},
{0x3813, 0x04},

{0x3618, 0x04},
{0x3612, 0x4b},
{0x3708, 0x21},
{0x3709, 0x12},
{0x370c, 0x00},
{0x3a02, 0x07}, /* night mode */
{0x3a03, 0xb0},
{0x3a0e, 0x06},
{0x3a0d, 0x08},
{0x3a14, 0x07},
{0x3a15, 0xb0},

{0x4004, 0x06}, /* BLC line number */
{0x5000, 0x07}, /* black/white pixel cancell, color interp. enable */
{0x5181, 0x52}, /* AWB */
{0x5182, 0x00},
{0x5197, 0x01},
{0x519e, 0x38},

{0x3035, 0x21}, /* SC PLL */
{0x5000, 0x27},
{0x5001, 0x83}, /* special effect, color matrix, AWB enable */
{0x3035, 0x71},
{0x4713, 0x02}, /* jpeg mode 2 */
{0x3036, 0x69},
{0x4407, 0x0c}, /* jpeg ctrl */
{0x460b, 0x37},
{0x460c, 0x20},
{0x3824, 0x01},
{0x4005, 0x1A},

};

/*
static const struct regval ov5640_qsxga_to_qvga_regs[] = {

};
*/

static const struct regval ov5640_qsxga_to_vga_regs[] = {
{0x3800, 0x00}, /* image windowing */
{0x3801, 0x00},
{0x3802, 0x00},
{0x3803, 0x00},
{0x3804, 0xA },
{0x3805, 0x3f},
{0x3806, 0x7 },
{0x3807, 0x9f},
{0x3808, 0x2 }, /* 0x280== 640*/
{0x3809, 0x80},
{0x380a, 0x1 }, /* 0x1e0== 480*/
{0x380b, 0xe0},
{0x380c, 0xc },
{0x380d, 0x80},
{0x380e, 0x7 },
{0x380f, 0xd0},

{0x5001, 0xa3}, /* SDE, scaling, color matrix, AWB enable */
{0x5680, 0x0 }, /* AVG ctrl */
{0x5681, 0x0 },
{0x5682, 0xA },
{0x5683, 0x20},
{0x5684, 0x0 },
{0x5685, 0x0 },
{0x5686, 0x7 },
{0x5687, 0x98},

};

static struct regval ov5640_preview_vga_tbl[] = {
        //sys_clk 24*70/3/1/2/2.5/2=28M, dvp_pclk=56M
        //frame rate: 56000000/1896/984=30fps
        {0x3036,0x46},
//	{0x3037,0x13},	//vco freq=24*70/3=560M
//	{0x3034,0x1a},
        {0x3035,0x11},
//	{0x3108,0x01},	//sys_clk=560/1/2/2.5/2=56M, framerate=sys_clk/hts/vts=30
        {0x3824,0x02},
        {0x460C,0x22},	//dvp_pclk controlled by REG(0x3824)

        {0x3C07,0x08},
        {0x3820,0x41}, //ryj modify 0x47
        {0x3821,0x07}, //ryj modify 0x01
        {0x3814,0x31},
        {0x3815,0x31},
        {0x3803,0x04},
        {0x3807,0x9B},
        //DVP output size 640*480
        {0x3808,0x02},
        {0x3809,0x80},
        {0x380A,0x01},
        {0x380B,0xE0},
        //HTS 1896
        {0x380C,0x07},
        {0x380D,0x68},
        //VTS 984
        {0x380E,0x03},
        {0x380F,0xD8},

        {0x3813,0x06},
        {0x3618,0x00},
        {0x3612,0x29},
        {0x3708,0x64},
        {0x3709,0x52},
        {0x370C,0x03},
        {0x3A02,0x03},
        {0x3A03,0xD8},
        {0x3A0E,0x03},
        {0x3A0D,0x04},
        {0x3A14,0x03},
        {0x3A15,0xD8},
        {0x4004,0x02},
        {0x4713,0x03},
        {0x4407,0x04},
        {0x460B,0x35},
        {0x5001,0xA3},
        {0x3008,0x02},
        {0x3002,0x1c},
        {0x3006,0xc3},
        //enable AEC/AGC
        {0x3503,0x00},
};
static const struct regval ov5640_qsxga_to_xga_regs[] = {
{0x3800, 0x00},
{0x3801, 0x00},
{0x3802, 0x00},
{0x3803, 0x00},
{0x3804, 0xA },
{0x3805, 0x3f},
{0x3806, 0x7 },
{0x3807, 0x9f},
{0x3808, 0x4 }, /* 0x400==1024 */
{0x3809, 0x0 },
{0x380a, 0x3 }, /* 0x300== 768*/
{0x380b, 0x0 },
{0x380c, 0xc },
{0x380d, 0x80},
{0x380e, 0x7 },
{0x380f, 0xd0},
{0x5001, 0xa3},
{0x5680, 0x0 },
{0x5681, 0x0 },
{0x5682, 0xA },
{0x5683, 0x20},
{0x5684, 0x0 },
{0x5685, 0x0 },
{0x5686, 0x7 },
{0x5687, 0x98},

};

//dg add
static struct regval ov5640_preview_xga_tbl[] = {
    //sys_clk 24*70/3/1/2/2.5/2=56M, dvp_pclk=112M
    //frame rate: 56000000/1896/984=30fps
{0x3036,0x46},
//	{0x3037,0x13},	//vco freq=24*70/3=560M
//	{0x3034,0x1a},
{0x3035,0x11},
//	{0x3108,0x01},	//sys_clk=560/1/2/2.5/2=56M, framerate=sys_clk/hts/vts=30
{0x3824,0x01},
{0x460C,0x22},	//dvp_pclk controlled by REG(0x3824)

{0x3C07,0x08},
{0x3820,0x41}, //ryj modify 0x47
{0x3821,0x07}, //ryj modify 0x01
{0x3814,0x31},
{0x3815,0x31},
{0x3803,0x04},
{0x3807,0x9B},
//DVP output size 1024*768
{0x3808,0x04},
{0x3809,0x00},
{0x380A,0x03},
{0x380B,0x00},
//HTS 1896
{0x380C,0x07},
{0x380D,0x68},
//VTS 984
{0x380E,0x03},
{0x380F,0xD8},

{0x3813,0x06},
{0x3618,0x00},
{0x3612,0x29},
{0x3708,0x64},
{0x3709,0x52},
{0x370C,0x03},
{0x3A02,0x03},
{0x3A03,0xD8},
{0x3A0E,0x03},
{0x3A0D,0x04},
{0x3A14,0x03},
{0x3A15,0xD8},
{0x4004,0x02},
{0x4713,0x03},
{0x4407,0x04},
{0x460B,0x35},
{0x5001,0xA3},
{0x3008,0x02},
{0x3002,0x1c},
{0x3006,0xc3},
//enable AEC/AGC
{0x3503,0x00},
};


static const struct regval ov5640_qsxga_to_sxga_regs[] = {
{0x3800, 0x00},
{0x3801, 0x00},
{0x3802, 0x00},
{0x3803, 0x00},
{0x3804, 0xA },
{0x3805, 0x3f},
{0x3806, 0x7 },
{0x3807, 0x9f},
{0x3808, 0x5 }, /* 0x500==1280 */
{0x3809, 0x0 },
{0x380a, 0x3 }, /* 0x3c0==960 */
{0x380b, 0xc0},
{0x380c, 0xc },
{0x380d, 0x80},
{0x380e, 0x7 },
{0x380f, 0xd0},
{0x5001, 0xa3},
{0x5680, 0x0 },
{0x5681, 0x0 },
{0x5682, 0xA },
{0x5683, 0x20},
{0x5684, 0x0 },
{0x5685, 0x0 },
{0x5686, 0x7 },
{0x5687, 0x98},

};

//dg add
static struct regval ov5640_preview_sxga_tbl[] = {
    //sys_clk 24*70/3/1/2/2.5/2=56M, dvp_pclk=112M
    //frame rate: 56000000/1896/984=30fps
{0x3036,0x46},
//	{0x3037,0x13},	//vco freq=24*70/3=560M
//	{0x3034,0x1a},
{0x3035,0x11},
//	{0x3108,0x01},	//sys_clk=560/1/2/2.5/2=56M, framerate=sys_clk/hts/vts=30
{0x3824,0x01},
{0x460C,0x22},	//dvp_pclk controlled by REG(0x3824)

{0x3C07,0x08},
{0x3820,0x41}, //ryj modify 0x47
{0x3821,0x07}, //ryj modify 0x01
{0x3814,0x31},
{0x3815,0x31},
{0x3803,0x04},
{0x3807,0x9B},
//DVP output size 1280*960
{0x3808,0x05},
{0x3809,0x00},
{0x380A,0x03},
{0x380B,0xC0},
//HTS 1896
{0x380C,0x07},
{0x380D,0x68},
//VTS 984
{0x380E,0x03},
{0x380F,0xD8},

{0x3813,0x06},
{0x3618,0x00},
{0x3612,0x29},
{0x3708,0x64},
{0x3709,0x52},
{0x370C,0x03},
{0x3A02,0x03},
{0x3A03,0xD8},
{0x3A0E,0x03},
{0x3A0D,0x04},
{0x3A14,0x03},
{0x3A15,0xD8},
{0x4004,0x02},
{0x4713,0x03},
{0x4407,0x04},
{0x460B,0x35},
{0x5001,0xA3},
{0x3008,0x02},
{0x3002,0x1c},
{0x3006,0xc3},
//enable AEC/AGC
{0x3503,0x00},
};

static struct regval ov5640_preview_720p_tbl[] = {
    //sys_clk 24*70/3/1/2/2.5/2=56M, dvp_pclk=112M
    //frame rate: 56000000/1896/984=30fps
{0x3036,0x46},
//	{0x3037,0x13},	//vco freq=24*70/3=560M
//	{0x3034,0x1a},
{0x3035,0x11},
//	{0x3108,0x01},	//sys_clk=560/1/2/2.5/2=56M, framerate=sys_clk/hts/vts=30
{0x3824,0x01},
{0x460C,0x22},	//dvp_pclk controlled by REG(0x3824)

{0x3C07,0x08},
{0x3820,0x41}, //ryj modify 0x47
{0x3821,0x07}, //ryj modify 0x01
{0x3814,0x31},
{0x3815,0x31},
{0x3803,0x04},
{0x3807,0x9B},
//DVP output size 1280*720
{0x3808,0x05},
{0x3809,0x00},
{0x380A,0x02},
{0x380B,0xD0},
//HTS 1896
{0x380C,0x07},
{0x380D,0x68},
//VTS 984
{0x380E,0x03},
{0x380F,0xD8},

{0x3813,0x7E},
{0x3618,0x00},
{0x3612,0x29},
{0x3708,0x64},
{0x3709,0x52},
{0x370C,0x03},
{0x3A02,0x03},
{0x3A03,0xD8},
{0x3A0E,0x03},
{0x3A0D,0x04},
{0x3A14,0x03},
{0x3A15,0xD8},
{0x4004,0x02},
{0x4713,0x03},
{0x4407,0x04},
{0x460B,0x35},
{0x5001,0xA3},
{0x3008,0x02},
{0x3002,0x1c},
{0x3006,0xc3},
//enable AEC/AGC
{0x3503,0x00},
};
static const struct regval ov5640_qsxga_to_uxga_regs[] = {
#if 0
{0x3800, 0x00},
{0x3801, 0x00},
{0x3802, 0x00},
{0x3803, 0x00},
{0x3804, 0xA },
{0x3805, 0x3f},
{0x3806, 0x7 },
{0x3807, 0x9f},
{0x3808, 0x6 }, /* 0x640== 1600*/
{0x3809, 0x40},
{0x380a, 0x4 }, /* 0x4b0==1200 */
{0x380b, 0xb0},
{0x380c, 0xc },
{0x380d, 0x80},
{0x380e, 0x7 },
{0x380f, 0xd0},
{0x5001, 0xa3},
{0x5680, 0x0 },
{0x5681, 0x0 },
{0x5682, 0xA },
{0x5683, 0x20},
{0x5684, 0x0 },
{0x5685, 0x0 },
{0x5686, 0x7 },
{0x5687, 0x98},
#else
{0x3503, 0x07},
{0x3a00, 0x38},
{0x4050, 0x6e},
{0x4051, 0x8f},
{0x5302, 0x1c},
{0x5303, 0x08},
{0x5306, 0x0c},
{0x5307, 0x1c},
{0x3820, 0x40},
{0x3821, 0x06},
{0x3800, 0x00},
{0x3801, 0x00},
{0x3802, 0x00},
{0x3803, 0x00},
{0x3804, 0x0a},
{0x3805, 0x3f},
{0x3806, 0x07},
{0x3807, 0x9f},
{0x3808, 0x0a},
{0x3809, 0x20},
{0x380a, 0x07},
{0x380b, 0x98},
{0x3810, 0x00},
{0x3811, 0x10},
{0x3812, 0x00},
{0x3813, 0x04},
{0x3814, 0x11},
{0x3815, 0x11},

{0x3034, 0x1a},
{0x3035, 0x11},
{0x3036, 0x46},
{0x3037, 0x13},
{0x3038, 0x00},
{0x3039, 0x00},

{0x380c, 0x0b},
{0x380d, 0x1c},
{0x380e, 0x07},
{0x380f, 0xb0},

{0x3a08, 0x00},
{0x3a09, 0xc5},
{0x3a0e, 0x0a},
{0x3a0a, 0x00},
{0x3a0b, 0xa4},
{0x3a0d, 0x0c},

{0x3618, 0x04},
{0x3612, 0x2b},
{0x3709, 0x12},
{0x370c, 0x00},

{0x4004, 0x06},
{0x3002, 0x00},
{0x3006, 0xff},
{0x4713, 0x02},
{0x4407, 0x04},
{0x460b, 0x37},
{0x460c, 0x22},
{0x4837, 0x16},
{0x3824, 0x01},
{0x5001, 0x83},

{0x4202,0x00},
#endif
};


static struct regval  ov5640_capture_uxga_tbl[] = {
        //sys_clk 24*105/3/3/2/2.5/2=28M, dvp_pclk=56M
        //frame rate: 28000000/2844/1968=5fps
        {0x3035,0x31},
        {0x3036,0x69},
        {0x460c,0x22},
        {0x3824,0x01},
        {0x3C07,0x07},
        //1600*1200
        {0x3820,0x41}, //ryj modify 0x47
        {0x3821,0x07}, //ryj modify 0x01
        {0x3814,0x11},
        {0x3815,0x11},
        {0x3803,0x00},
        {0x3807,0x9f},
        {0x3808,0x06},
        {0x3809,0x40},
        {0x380a,0x04},
        {0x380b,0xb0},
        {0x380c,0x0b},
        {0x380d,0x1c},
        {0x380e,0x07},
        {0x380f,0xb0},
        {0x3811,0x20},
        {0x3813,0x10},
        {0x3618,0x04},
        {0x3612,0x4b},
        {0x3708,0x21},
        {0x3709,0x12},
        {0x370c,0x00},
        {0x3a02,0x07},
        {0x3a03,0xb0},
        {0x3a0e,0x06},
        {0x3a0d,0x08},
        {0x3a14,0x07},
        {0x3a15,0xb0},
        {0x4004,0x06},
//	{0x5000,0x07},
        {0x5181,0x52},
        {0x5182,0x00},
        {0x5197,0x01},
        {0x519e,0x38},
//	{0x5000,0x27},
        {0x5001,0xa3},
        {0x4713,0x02},
        {0x4407,0x0c},
        {0x460b,0x37},
        {0x4005,0x1a},
};


static const struct regval ov5640_qsxga_to_qxga_regs[] = {
{0x3800, 0x00},
{0x3801, 0x00},
{0x3802, 0x00},
{0x3803, 0x00},
{0x3804, 0xA },
{0x3805, 0x3f},
{0x3806, 0x7 },
{0x3807, 0x9f},
{0x3808, 0x8 }, /* 0x800==2048 */
{0x3809, 0x0 },
{0x380a, 0x6 }, /* 0x600==1536 */
{0x380b, 0x0 },
{0x380c, 0xc },
{0x380d, 0x80},
{0x380e, 0x7 },
{0x380f, 0xd0},
{0x5001, 0xa3},
{0x5680, 0x0 },
{0x5681, 0x0 },
{0x5682, 0xA },
{0x5683, 0x20},
{0x5684, 0x0 },
{0x5685, 0x0 },
{0x5686, 0x7 },
{0x5687, 0x98},

};
static struct regval ov5640_capture_qxga_tbl[] = {
        //sys_clk 24*105/3/3/2/2.5/2=28M, dvp_pclk=56M
        //frame rate: 28000000/2844/1968=5fps
        {0x3035,0x31},
        {0x3036,0x69},
        {0x460c,0x22},
        {0x3824,0x01},
        {0x3C07,0x07},
        //2048*1536
        {0x3820,0x41}, //ryj modify 0x47
        {0x3821,0x07}, //ryj modify 0x01
        {0x3814,0x11},
        {0x3815,0x11},
        {0x3803,0x00},
        {0x3807,0x9f},
        {0x3808,0x08},
        {0x3809,0x00},
        {0x380a,0x06},
        {0x380b,0x00},
        {0x380c,0x0b},
        {0x380d,0x1c},
        {0x380e,0x07},
        {0x380f,0xb0},
        {0x3811,0x20},
        {0x3813,0x10},
        {0x3618,0x04},
        {0x3612,0x4b},
        {0x3708,0x21},
        {0x3709,0x12},
        {0x370c,0x00},
        {0x3a02,0x07},
        {0x3a03,0xb0},
        {0x3a0e,0x06},
        {0x3a0d,0x08},
        {0x3a14,0x07},
        {0x3a15,0xb0},
        {0x4004,0x06},
//	{0x5000,0x07},
        {0x5181,0x52},
        {0x5182,0x00},
        {0x5197,0x01},
        {0x519e,0x38},
//	{0x5000,0x27},
        {0x5001,0xa3},
        {0x4713,0x02},
        {0x4407,0x0c},
        {0x460b,0x37},
        {0x4005,0x1a},
};

/*-----------2592x1944----------------------------------*/
static struct regval ov5640_capture_qsxga_tbl[] = {
        {0x3035 ,0x21}, // PLL
        {0x3036 ,0x46}, // PLL
        {0x460c ,0x20},
        {0x3824 ,0x01}, // PCLK manual divider
        {0x3c07 ,0x07}, // lightm eter 1 threshold[7:0]
        {0x3820 ,0x41}, // flip
        {0x3821 ,0x07}, // mirror
        {0x3814 ,0x11}, // timing X inc
        {0x3815 ,0x11}, // timing Y inc
        {0x3800 ,0x00}, // HS
        {0x3801 ,0x00}, // HS
        {0x3802 ,0x00}, // VS
        {0x3803 ,0x00}, // VS
        {0x3804 ,0x0a}, // HW (HE)
        {0x3805 ,0x3f}, // HW (HE)
        {0x3806 ,0x07}, // VH (VE)
        {0x3807 ,0x9f}, // VH {VE}
        {0x3808 ,0x0a}, // DVPHO
        {0x3809 ,0x20}, // DVPHO
        {0x380a ,0x07}, // DVPVO
        {0x380b ,0x98}, // DVPVO
        {0x380c ,0x0b}, // HTS
        {0x380d ,0x1c}, // HTS
        {0x380e ,0x07}, // VTS
        {0x380f ,0xb0}, // VTS
        {0x3811 ,0x10}, //??????  meida
        {0x3813 ,0x04}, // timing V offset
        {0x3618 ,0x04},
        {0x3612 ,0x2b}, //------------------line on meida  line down samsung----------------
        {0x3708 ,0x21}, //??????????
        {0x3709 ,0x12},
        {0x370c ,0x00},
        {0x3a02 ,0x07}, //??????
        {0x3a03 ,0xb0}, //??????
        {0x3a0e ,0x06}, //??????
        {0x3a0d ,0x08}, //??????
        {0x3a14 ,0x07}, //??????
        {0x3a15 ,0xb0}, //??????
        {0x4004 ,0x06}, // BLC line number
        //{0x5000,0x07},
        {0x5181,0x52},
        {0x5182,0x00},
        {0x5197,0x01},
        {0x519e,0x38},
        //{0x5000,0x27},
        {0x5001,0xa3},
        {0x4713,0x02},
        {0x4407,0x0c},
        {0x460b,0x37},
        {0x4005,0x1a},
};

#if 0
static const struct v4l2_queryctrl ov5640_controls[] = {
{
    .id		= V4L2_CID_VFLIP,
    .type		= V4L2_CTRL_TYPE_BOOLEAN,
    .name		= "Flip Vertically",
    .minimum	= 0,
    .maximum	= 1,
    .step		= 1,
    .default_value	= 0,
},
{
    .id		= V4L2_CID_HFLIP,
    .type		= V4L2_CTRL_TYPE_BOOLEAN,
    .name		= "Flip Horizontally",
    .minimum	= 0,
    .maximum	= 1,
    .step		= 1,
    .default_value	= 0,
},
};
#endif

/* QVGA: 320*240 */ 
/*
static const struct ov5640_win_size ov5640_win_qvga = {
        .name     = "QVGA",
        .width    = QVGA_WIDTH,
        .height   = QVGA_HEIGHT,
        .regs     = ov5640_qsxga_to_qvga_regs,
};
*/

static const struct ov5640_win_size ov5640_wins[] = {
{
    .name    = "VGA",    /* VGA: 640*480 */
    .resv    = RESV_VGA,
    .width   = VGA_WIDTH,
    .height  = VGA_HEIGHT,
    .regs    = ov5640_preview_vga_tbl,//ov5640_qsxga_to_vga_regs,
},
{
    .name    = "XGA",    /* XGA: 1024*768 */
    .resv    = RESV_XGA,
    .width   = XGA_WIDTH,
    .height  = XGA_HEIGHT,
    .regs    =  ov5640_preview_xga_tbl, /*ov5640_qsxga_to_xga_regs,*/
},

{
    .name    = "720P",    /* 720P: 1280*720  */
    .resv    = RESV_720P,
    .width   = OV5640_720P_WIDTH,
    .height  = OV5640_720P_HEIGHT,
    .regs    = ov5640_preview_720p_tbl,

},

{
    .name    = "SXGA",    /* SXGA: 1280*960  */
    .resv    = RESV_SXGA,
    .width   = SXGA_WIDTH,
    .height  = SXGA_HEIGHT,
    .regs    = ov5640_preview_sxga_tbl, /*ov5640_qsxga_to_sxga_regs*/
},
{
    .name    = "UXGA",    /* UXGA: 1600*1200 */
    .resv    = RESV_UXGA,
    .width   = UXGA_WIDTH,
    .height  = UXGA_HEIGHT,
    .regs    = ov5640_qsxga_to_uxga_regs,// ov5640_capture_uxga_tbl, /*ov5640_qsxga_to_uxga_regs*/
},
{
    .name    = "QXGA",    /* QXGA: 2048*1536 */
    .resv    = RESV_QXGA,
    .width   = QXGA_WIDTH,
    .height  = QXGA_HEIGHT,
    .regs    = ov5640_capture_qxga_tbl,//ov5640_qsxga_to_qxga_regs,
},
{
    .name    = "QSXGA",    /* QSXGA: 2592*1944*/
    .resv    = RESV_QSXGA,
    .width   = QSXGA_WIDTH,
    .height  = QSXGA_HEIGHT,
    .regs    = ov5640_qsxga_regs,//ov5640_capture_qsxga_tbl,     //ov5640_qsxga_regs,
},

};


/*
 * supported color format list
 */
static const struct ov5640_color_format ov5640_cfmts[] = {
{
    .code		= 2,//cym V4L2_MBUS_FMT_YUYV8_2X8_BE,
    .colorspace	= V4L2_COLORSPACE_JPEG,
},
{
    .code		= 3, //cym V4L2_MBUS_FMT_YUYV8_2X8_LE,
    .colorspace	= V4L2_COLORSPACE_JPEG,
},
{
    .code           = 0x2007, //cym V4L2_MBUS_FMT_YUYV8_2X8_LE,
    .colorspace     = V4L2_COLORSPACE_JPEG,
},
};

/* add by cym 20130605 */
static int ov5640_power(int flag)
{
    int err;
    printk("cym: ov5640 sensor is power %s\n",flag == 1 ?"on":"off");
    //Attention: Power On the all the camera module when system turn on
    //Here only control the reset&&powerdown pin

    /* Camera A */
    if(1 == flag)
    {
        //poweron
#ifndef CONFIG_TC4_EVT
        regulator_enable(ov_vdd18_cam_regulator);
        udelay(10);
        regulator_enable(ov_vdd28_cam_regulator);
        udelay(10);
        regulator_enable(ov_vdd5m_cam_regulator); //DOVDD  DVDD 1.8v
        udelay(10);
        regulator_enable(ov_vddaf_cam_regulator);         //AVDD 2.8v
        udelay(10);
#endif
#if 1
        //pwdn		GPF0_5: 2M PWDN
        err = gpio_request(EXYNOS4_GPF0(5), "GPF0_5");
        if (err)
            printk(KERN_ERR "#### failed to request GPF0_5 ####\n");
        s3c_gpio_setpull(EXYNOS4_GPF0(5), S3C_GPIO_PULL_NONE);
        gpio_direction_output(EXYNOS4_GPF0(5), 1);
        gpio_free(EXYNOS4_GPF0(5));
#endif
        //pwdn1	GPF2_4: 5M PWDN
        err = gpio_request(EXYNOS4_GPL0(3), "GPL0_3");
        if (err)
            printk(KERN_ERR "#### failed to request GPL0_3 ####\n");
        s3c_gpio_setpull(EXYNOS4_GPL0(3), S3C_GPIO_PULL_NONE);
        gpio_direction_output(EXYNOS4_GPL0(3), 0);
        gpio_free(EXYNOS4_GPL0(3));
        msleep(20);

        //reset
        err = gpio_request(EXYNOS4_GPL0(1), "GPL0_1");
        if (err)
            printk(KERN_ERR "#### failed to request GPL0_1 ####\n");
        s3c_gpio_setpull(EXYNOS4_GPL0(1), S3C_GPIO_PULL_NONE);
        gpio_direction_output(EXYNOS4_GPL0(1), 1);
        msleep(10);
        gpio_direction_output(EXYNOS4_GPL0(1), 0);
        msleep(10);
        gpio_direction_output(EXYNOS4_GPL0(1), 1);

        gpio_free(EXYNOS4_GPL0(1));
        msleep(20);
    }
    else
    {
        err = gpio_request(EXYNOS4_GPL0(1), "GPF0");
        if (err)
            printk(KERN_ERR "#### failed to request GPF0_4 ####\n");
        s3c_gpio_setpull(EXYNOS4_GPL0(1), S3C_GPIO_PULL_NONE);
        gpio_direction_output(EXYNOS4_GPL0(1), 0);
        gpio_free(EXYNOS4_GPL0(1));

#if 1
        //powerdown	GPF0_5: 2M PWDN
        err = gpio_request(EXYNOS4_GPF0(5), "GPF0_5");
        if (err)
            printk(KERN_ERR "#### failed to request GPE0_5 ####\n");
        s3c_gpio_setpull(EXYNOS4_GPF0(5), S3C_GPIO_PULL_NONE);
        gpio_direction_output(EXYNOS4_GPF0(5), 1);
        gpio_free(EXYNOS4_GPF0(5));
#endif
        //pwdn1			GPF2_4: 5M PWDN
        err = gpio_request(EXYNOS4_GPL0(3), "GPL0_3");
        if (err)
            printk(KERN_ERR "#### failed to request GPF2_4 ####\n");
        s3c_gpio_setpull(EXYNOS4_GPL0(3), S3C_GPIO_PULL_NONE);
        gpio_direction_output(EXYNOS4_GPL0(3), 1);
        gpio_free(EXYNOS4_GPL0(3));
#ifndef CONFIG_TC4_EVT
        regulator_disable(ov_vdd18_cam_regulator);
        udelay(10);
        regulator_disable(ov_vdd28_cam_regulator);
        udelay(10);
        regulator_disable(ov_vdd5m_cam_regulator);
        udelay(10);
        regulator_disable(ov_vddaf_cam_regulator);
        udelay(10);
#endif
    }

    return 0;
}

#define DEV_DBG_EN   	1 
#define REG_ADDR_STEP 2
#define REG_DATA_STEP 1
#define REG_STEP 			(REG_ADDR_STEP+REG_DATA_STEP)

#define csi_dev_err(x,arg...) printk(KERN_INFO"[OV5640]"x,##arg)
#define csi_dev_print(x,arg...) printk(KERN_INFO"[OV5640]"x,##arg)

struct regval_list {
    unsigned char reg_num[REG_ADDR_STEP];
    unsigned char value[REG_DATA_STEP];
};

static char ov5640_sensor_af_fw_regs[] = {
    0x02, 0x0f, 0xd6, 0x02, 0x0a, 0x39, 0xc2, 0x01, 0x22, 0x22, 0x00, 0x02, 0x0f, 0xb2, 0xe5, 0x1f, //0x8000,
    0x70, 0x72, 0xf5, 0x1e, 0xd2, 0x35, 0xff, 0xef, 0x25, 0xe0, 0x24, 0x4e, 0xf8, 0xe4, 0xf6, 0x08, //0x8010,
    0xf6, 0x0f, 0xbf, 0x34, 0xf2, 0x90, 0x0e, 0x93, 0xe4, 0x93, 0xff, 0xe5, 0x4b, 0xc3, 0x9f, 0x50, //0x8020,
    0x04, 0x7f, 0x05, 0x80, 0x02, 0x7f, 0xfb, 0x78, 0xbd, 0xa6, 0x07, 0x12, 0x0f, 0x04, 0x40, 0x04, //0x8030,
    0x7f, 0x03, 0x80, 0x02, 0x7f, 0x30, 0x78, 0xbc, 0xa6, 0x07, 0xe6, 0x18, 0xf6, 0x08, 0xe6, 0x78, //0x8040,
    0xb9, 0xf6, 0x78, 0xbc, 0xe6, 0x78, 0xba, 0xf6, 0x78, 0xbf, 0x76, 0x33, 0xe4, 0x08, 0xf6, 0x78, //0x8050,
    0xb8, 0x76, 0x01, 0x75, 0x4a, 0x02, 0x78, 0xb6, 0xf6, 0x08, 0xf6, 0x74, 0xff, 0x78, 0xc1, 0xf6, //0x8060,
    0x08, 0xf6, 0x75, 0x1f, 0x01, 0x78, 0xbc, 0xe6, 0x75, 0xf0, 0x05, 0xa4, 0xf5, 0x4b, 0x12, 0x0a, //0x8070,
    0xff, 0xc2, 0x37, 0x22, 0x78, 0xb8, 0xe6, 0xd3, 0x94, 0x00, 0x40, 0x02, 0x16, 0x22, 0xe5, 0x1f, //0x8080,
    0xb4, 0x05, 0x23, 0xe4, 0xf5, 0x1f, 0xc2, 0x01, 0x78, 0xb6, 0xe6, 0xfe, 0x08, 0xe6, 0xff, 0x78, //0x8090,
    0x4e, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0xa2, 0x37, 0xe4, 0x33, 0xf5, 0x3c, 0x90, 0x30, 0x28, 0xf0, //0x80a0,
    0x75, 0x1e, 0x10, 0xd2, 0x35, 0x22, 0xe5, 0x4b, 0x75, 0xf0, 0x05, 0x84, 0x78, 0xbc, 0xf6, 0x90, //0x80b0,
    0x0e, 0x8c, 0xe4, 0x93, 0xff, 0x25, 0xe0, 0x24, 0x0a, 0xf8, 0xe6, 0xfc, 0x08, 0xe6, 0xfd, 0x78, //0x80c0,
    0xbc, 0xe6, 0x25, 0xe0, 0x24, 0x4e, 0xf8, 0xa6, 0x04, 0x08, 0xa6, 0x05, 0xef, 0x12, 0x0f, 0x0b, //0x80d0,
    0xd3, 0x78, 0xb7, 0x96, 0xee, 0x18, 0x96, 0x40, 0x0d, 0x78, 0xbc, 0xe6, 0x78, 0xb9, 0xf6, 0x78, //0x80e0,
    0xb6, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0x90, 0x0e, 0x8c, 0xe4, 0x93, 0x12, 0x0f, 0x0b, 0xc3, 0x78, //0x80f0,
    0xc2, 0x96, 0xee, 0x18, 0x96, 0x50, 0x0d, 0x78, 0xbc, 0xe6, 0x78, 0xba, 0xf6, 0x78, 0xc1, 0xa6, //0x8100,
    0x06, 0x08, 0xa6, 0x07, 0x78, 0xb6, 0xe6, 0xfe, 0x08, 0xe6, 0xc3, 0x78, 0xc2, 0x96, 0xff, 0xee, //0x8110,
    0x18, 0x96, 0x78, 0xc3, 0xf6, 0x08, 0xa6, 0x07, 0x90, 0x0e, 0x95, 0xe4, 0x18, 0x12, 0x0e, 0xe9, //0x8120,
    0x40, 0x02, 0xd2, 0x37, 0x78, 0xbc, 0xe6, 0x08, 0x26, 0x08, 0xf6, 0xe5, 0x1f, 0x64, 0x01, 0x70, //0x8130,
    0x4a, 0xe6, 0xc3, 0x78, 0xc0, 0x12, 0x0e, 0xdf, 0x40, 0x05, 0x12, 0x0e, 0xda, 0x40, 0x39, 0x12, //0x8140,
    0x0f, 0x02, 0x40, 0x04, 0x7f, 0xfe, 0x80, 0x02, 0x7f, 0x02, 0x78, 0xbd, 0xa6, 0x07, 0x78, 0xb9, //0x8150,
    0xe6, 0x24, 0x03, 0x78, 0xbf, 0xf6, 0x78, 0xb9, 0xe6, 0x24, 0xfd, 0x78, 0xc0, 0xf6, 0x12, 0x0f, //0x8160,
    0x02, 0x40, 0x06, 0x78, 0xc0, 0xe6, 0xff, 0x80, 0x04, 0x78, 0xbf, 0xe6, 0xff, 0x78, 0xbe, 0xa6, //0x8170,
    0x07, 0x75, 0x1f, 0x02, 0x78, 0xb8, 0x76, 0x01, 0x02, 0x02, 0x4a, 0xe5, 0x1f, 0x64, 0x02, 0x60, //0x8180,
    0x03, 0x02, 0x02, 0x2a, 0x78, 0xbe, 0xe6, 0xff, 0xc3, 0x78, 0xc0, 0x12, 0x0e, 0xe0, 0x40, 0x08, //0x8190,
    0x12, 0x0e, 0xda, 0x50, 0x03, 0x02, 0x02, 0x28, 0x12, 0x0f, 0x02, 0x40, 0x04, 0x7f, 0xff, 0x80, //0x81a0,
    0x02, 0x7f, 0x01, 0x78, 0xbd, 0xa6, 0x07, 0x78, 0xb9, 0xe6, 0x04, 0x78, 0xbf, 0xf6, 0x78, 0xb9, //0x81b0,
    0xe6, 0x14, 0x78, 0xc0, 0xf6, 0x18, 0x12, 0x0f, 0x04, 0x40, 0x04, 0xe6, 0xff, 0x80, 0x02, 0x7f, //0x81c0,
    0x00, 0x78, 0xbf, 0xa6, 0x07, 0xd3, 0x08, 0xe6, 0x64, 0x80, 0x94, 0x80, 0x40, 0x04, 0xe6, 0xff, //0x81d0,
    0x80, 0x02, 0x7f, 0x00, 0x78, 0xc0, 0xa6, 0x07, 0xc3, 0x18, 0xe6, 0x64, 0x80, 0x94, 0xb3, 0x50, //0x81e0,
    0x04, 0xe6, 0xff, 0x80, 0x02, 0x7f, 0x33, 0x78, 0xbf, 0xa6, 0x07, 0xc3, 0x08, 0xe6, 0x64, 0x80, //0x81f0,
    0x94, 0xb3, 0x50, 0x04, 0xe6, 0xff, 0x80, 0x02, 0x7f, 0x33, 0x78, 0xc0, 0xa6, 0x07, 0x12, 0x0f, //0x8200,
    0x02, 0x40, 0x06, 0x78, 0xc0, 0xe6, 0xff, 0x80, 0x04, 0x78, 0xbf, 0xe6, 0xff, 0x78, 0xbe, 0xa6, //0x8210,
    0x07, 0x75, 0x1f, 0x03, 0x78, 0xb8, 0x76, 0x01, 0x80, 0x20, 0xe5, 0x1f, 0x64, 0x03, 0x70, 0x26, //0x8220,
    0x78, 0xbe, 0xe6, 0xff, 0xc3, 0x78, 0xc0, 0x12, 0x0e, 0xe0, 0x40, 0x05, 0x12, 0x0e, 0xda, 0x40, //0x8230,
    0x09, 0x78, 0xb9, 0xe6, 0x78, 0xbe, 0xf6, 0x75, 0x1f, 0x04, 0x78, 0xbe, 0xe6, 0x75, 0xf0, 0x05, //0x8240,
    0xa4, 0xf5, 0x4b, 0x02, 0x0a, 0xff, 0xe5, 0x1f, 0xb4, 0x04, 0x10, 0x90, 0x0e, 0x94, 0xe4, 0x78, //0x8250,
    0xc3, 0x12, 0x0e, 0xe9, 0x40, 0x02, 0xd2, 0x37, 0x75, 0x1f, 0x05, 0x22, 0x30, 0x01, 0x03, 0x02, //0x8260,
    0x04, 0xc0, 0x30, 0x02, 0x03, 0x02, 0x04, 0xc0, 0x90, 0x51, 0xa5, 0xe0, 0x78, 0x93, 0xf6, 0xa3, //0x8270,
    0xe0, 0x08, 0xf6, 0xa3, 0xe0, 0x08, 0xf6, 0xe5, 0x1f, 0x70, 0x3c, 0x75, 0x1e, 0x20, 0xd2, 0x35, //0x8280,
    0x12, 0x0c, 0x7a, 0x78, 0x7e, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0x78, 0x8b, 0xa6, 0x09, 0x18, 0x76, //0x8290,
    0x01, 0x12, 0x0c, 0x5b, 0x78, 0x4e, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0x78, 0x8b, 0xe6, 0x78, 0x6e, //0x82a0,
    0xf6, 0x75, 0x1f, 0x01, 0x78, 0x93, 0xe6, 0x78, 0x90, 0xf6, 0x78, 0x94, 0xe6, 0x78, 0x91, 0xf6, //0x82b0,
    0x78, 0x95, 0xe6, 0x78, 0x92, 0xf6, 0x22, 0x79, 0x90, 0xe7, 0xd3, 0x78, 0x93, 0x96, 0x40, 0x05, //0x82c0,
    0xe7, 0x96, 0xff, 0x80, 0x08, 0xc3, 0x79, 0x93, 0xe7, 0x78, 0x90, 0x96, 0xff, 0x78, 0x88, 0x76, //0x82d0,
    0x00, 0x08, 0xa6, 0x07, 0x79, 0x91, 0xe7, 0xd3, 0x78, 0x94, 0x96, 0x40, 0x05, 0xe7, 0x96, 0xff, //0x82e0,
    0x80, 0x08, 0xc3, 0x79, 0x94, 0xe7, 0x78, 0x91, 0x96, 0xff, 0x12, 0x0c, 0x8e, 0x79, 0x92, 0xe7, //0x82f0,
    0xd3, 0x78, 0x95, 0x96, 0x40, 0x05, 0xe7, 0x96, 0xff, 0x80, 0x08, 0xc3, 0x79, 0x95, 0xe7, 0x78, //0x8300,
    0x92, 0x96, 0xff, 0x12, 0x0c, 0x8e, 0x12, 0x0c, 0x5b, 0x78, 0x8a, 0xe6, 0x25, 0xe0, 0x24, 0x4e, //0x8310,
    0xf8, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0x78, 0x8a, 0xe6, 0x24, 0x6e, 0xf8, 0xa6, 0x09, 0x78, 0x8a, //0x8320,
    0xe6, 0x24, 0x01, 0xff, 0xe4, 0x33, 0xfe, 0xd3, 0xef, 0x94, 0x0f, 0xee, 0x64, 0x80, 0x94, 0x80, //0x8330,
    0x40, 0x04, 0x7f, 0x00, 0x80, 0x05, 0x78, 0x8a, 0xe6, 0x04, 0xff, 0x78, 0x8a, 0xa6, 0x07, 0xe5, //0x8340,
    0x1f, 0xb4, 0x01, 0x0a, 0xe6, 0x60, 0x03, 0x02, 0x04, 0xc0, 0x75, 0x1f, 0x02, 0x22, 0x12, 0x0c, //0x8350,
    0x7a, 0x78, 0x80, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0x12, 0x0c, 0x7a, 0x78, 0x82, 0xa6, 0x06, 0x08, //0x8360,
    0xa6, 0x07, 0x78, 0x6e, 0xe6, 0x78, 0x8c, 0xf6, 0x78, 0x6e, 0xe6, 0x78, 0x8d, 0xf6, 0x7f, 0x01, //0x8370,
    0xef, 0x25, 0xe0, 0x24, 0x4f, 0xf9, 0xc3, 0x78, 0x81, 0xe6, 0x97, 0x18, 0xe6, 0x19, 0x97, 0x50, //0x8380,
    0x0a, 0x12, 0x0c, 0x82, 0x78, 0x80, 0xa6, 0x04, 0x08, 0xa6, 0x05, 0x74, 0x6e, 0x2f, 0xf9, 0x78, //0x8390,
    0x8c, 0xe6, 0xc3, 0x97, 0x50, 0x08, 0x74, 0x6e, 0x2f, 0xf8, 0xe6, 0x78, 0x8c, 0xf6, 0xef, 0x25, //0x83a0,
    0xe0, 0x24, 0x4f, 0xf9, 0xd3, 0x78, 0x83, 0xe6, 0x97, 0x18, 0xe6, 0x19, 0x97, 0x40, 0x0a, 0x12, //0x83b0,
    0x0c, 0x82, 0x78, 0x82, 0xa6, 0x04, 0x08, 0xa6, 0x05, 0x74, 0x6e, 0x2f, 0xf9, 0x78, 0x8d, 0xe6, //0x83c0,
    0xd3, 0x97, 0x40, 0x08, 0x74, 0x6e, 0x2f, 0xf8, 0xe6, 0x78, 0x8d, 0xf6, 0x0f, 0xef, 0x64, 0x10, //0x83d0,
    0x70, 0x9e, 0xc3, 0x79, 0x81, 0xe7, 0x78, 0x83, 0x96, 0xff, 0x19, 0xe7, 0x18, 0x96, 0x78, 0x84, //0x83e0,
    0xf6, 0x08, 0xa6, 0x07, 0xc3, 0x79, 0x8c, 0xe7, 0x78, 0x8d, 0x96, 0x08, 0xf6, 0xd3, 0x79, 0x81, //0x83f0,
    0xe7, 0x78, 0x7f, 0x96, 0x19, 0xe7, 0x18, 0x96, 0x40, 0x05, 0x09, 0xe7, 0x08, 0x80, 0x06, 0xc3, //0x8400,
    0x79, 0x7f, 0xe7, 0x78, 0x81, 0x96, 0xff, 0x19, 0xe7, 0x18, 0x96, 0xfe, 0x78, 0x86, 0xa6, 0x06, //0x8410,
    0x08, 0xa6, 0x07, 0x79, 0x8c, 0xe7, 0xd3, 0x78, 0x8b, 0x96, 0x40, 0x05, 0xe7, 0x96, 0xff, 0x80, //0x8420,
    0x08, 0xc3, 0x79, 0x8b, 0xe7, 0x78, 0x8c, 0x96, 0xff, 0x78, 0x8f, 0xa6, 0x07, 0xe5, 0x1f, 0x64, //0x8430,
    0x02, 0x70, 0x69, 0x90, 0x0e, 0x91, 0x93, 0xff, 0x18, 0xe6, 0xc3, 0x9f, 0x50, 0x72, 0x12, 0x0c, //0x8440,
    0x4a, 0x12, 0x0c, 0x2f, 0x90, 0x0e, 0x8e, 0x12, 0x0c, 0x38, 0x78, 0x80, 0x12, 0x0c, 0x6b, 0x7b, //0x8450,
    0x04, 0x12, 0x0c, 0x1d, 0xc3, 0x12, 0x06, 0x45, 0x50, 0x56, 0x90, 0x0e, 0x92, 0xe4, 0x93, 0xff, //0x8460,
    0x78, 0x8f, 0xe6, 0x9f, 0x40, 0x02, 0x80, 0x11, 0x90, 0x0e, 0x90, 0xe4, 0x93, 0xff, 0xd3, 0x78, //0x8470,
    0x89, 0xe6, 0x9f, 0x18, 0xe6, 0x94, 0x00, 0x40, 0x03, 0x75, 0x1f, 0x05, 0x12, 0x0c, 0x4a, 0x12, //0x8480,
    0x0c, 0x2f, 0x90, 0x0e, 0x8f, 0x12, 0x0c, 0x38, 0x78, 0x7e, 0x12, 0x0c, 0x6b, 0x7b, 0x40, 0x12, //0x8490,
    0x0c, 0x1d, 0xd3, 0x12, 0x06, 0x45, 0x40, 0x18, 0x75, 0x1f, 0x05, 0x22, 0xe5, 0x1f, 0xb4, 0x05, //0x84a0,
    0x0f, 0xd2, 0x01, 0xc2, 0x02, 0xe4, 0xf5, 0x1f, 0xf5, 0x1e, 0xd2, 0x35, 0xd2, 0x33, 0xd2, 0x36, //0x84b0,
    0x22, 0xef, 0x8d, 0xf0, 0xa4, 0xa8, 0xf0, 0xcf, 0x8c, 0xf0, 0xa4, 0x28, 0xce, 0x8d, 0xf0, 0xa4, //0x84c0,
    0x2e, 0xfe, 0x22, 0xbc, 0x00, 0x0b, 0xbe, 0x00, 0x29, 0xef, 0x8d, 0xf0, 0x84, 0xff, 0xad, 0xf0, //0x84d0,
    0x22, 0xe4, 0xcc, 0xf8, 0x75, 0xf0, 0x08, 0xef, 0x2f, 0xff, 0xee, 0x33, 0xfe, 0xec, 0x33, 0xfc, //0x84e0,
    0xee, 0x9d, 0xec, 0x98, 0x40, 0x05, 0xfc, 0xee, 0x9d, 0xfe, 0x0f, 0xd5, 0xf0, 0xe9, 0xe4, 0xce, //0x84f0,
    0xfd, 0x22, 0xed, 0xf8, 0xf5, 0xf0, 0xee, 0x84, 0x20, 0xd2, 0x1c, 0xfe, 0xad, 0xf0, 0x75, 0xf0, //0x8500,
    0x08, 0xef, 0x2f, 0xff, 0xed, 0x33, 0xfd, 0x40, 0x07, 0x98, 0x50, 0x06, 0xd5, 0xf0, 0xf2, 0x22, //0x8510,
    0xc3, 0x98, 0xfd, 0x0f, 0xd5, 0xf0, 0xea, 0x22, 0xe8, 0x8f, 0xf0, 0xa4, 0xcc, 0x8b, 0xf0, 0xa4, //0x8520,
    0x2c, 0xfc, 0xe9, 0x8e, 0xf0, 0xa4, 0x2c, 0xfc, 0x8a, 0xf0, 0xed, 0xa4, 0x2c, 0xfc, 0xea, 0x8e, //0x8530,
    0xf0, 0xa4, 0xcd, 0xa8, 0xf0, 0x8b, 0xf0, 0xa4, 0x2d, 0xcc, 0x38, 0x25, 0xf0, 0xfd, 0xe9, 0x8f, //0x8540,
    0xf0, 0xa4, 0x2c, 0xcd, 0x35, 0xf0, 0xfc, 0xeb, 0x8e, 0xf0, 0xa4, 0xfe, 0xa9, 0xf0, 0xeb, 0x8f, //0x8550,
    0xf0, 0xa4, 0xcf, 0xc5, 0xf0, 0x2e, 0xcd, 0x39, 0xfe, 0xe4, 0x3c, 0xfc, 0xea, 0xa4, 0x2d, 0xce, //0x8560,
    0x35, 0xf0, 0xfd, 0xe4, 0x3c, 0xfc, 0x22, 0x75, 0xf0, 0x08, 0x75, 0x82, 0x00, 0xef, 0x2f, 0xff, //0x8570,
    0xee, 0x33, 0xfe, 0xcd, 0x33, 0xcd, 0xcc, 0x33, 0xcc, 0xc5, 0x82, 0x33, 0xc5, 0x82, 0x9b, 0xed, //0x8580,
    0x9a, 0xec, 0x99, 0xe5, 0x82, 0x98, 0x40, 0x0c, 0xf5, 0x82, 0xee, 0x9b, 0xfe, 0xed, 0x9a, 0xfd, //0x8590,
    0xec, 0x99, 0xfc, 0x0f, 0xd5, 0xf0, 0xd6, 0xe4, 0xce, 0xfb, 0xe4, 0xcd, 0xfa, 0xe4, 0xcc, 0xf9, //0x85a0,
    0xa8, 0x82, 0x22, 0xb8, 0x00, 0xc1, 0xb9, 0x00, 0x59, 0xba, 0x00, 0x2d, 0xec, 0x8b, 0xf0, 0x84, //0x85b0,
    0xcf, 0xce, 0xcd, 0xfc, 0xe5, 0xf0, 0xcb, 0xf9, 0x78, 0x18, 0xef, 0x2f, 0xff, 0xee, 0x33, 0xfe, //0x85c0,
    0xed, 0x33, 0xfd, 0xec, 0x33, 0xfc, 0xeb, 0x33, 0xfb, 0x10, 0xd7, 0x03, 0x99, 0x40, 0x04, 0xeb, //0x85d0,
    0x99, 0xfb, 0x0f, 0xd8, 0xe5, 0xe4, 0xf9, 0xfa, 0x22, 0x78, 0x18, 0xef, 0x2f, 0xff, 0xee, 0x33, //0x85e0,
    0xfe, 0xed, 0x33, 0xfd, 0xec, 0x33, 0xfc, 0xc9, 0x33, 0xc9, 0x10, 0xd7, 0x05, 0x9b, 0xe9, 0x9a, //0x85f0,
    0x40, 0x07, 0xec, 0x9b, 0xfc, 0xe9, 0x9a, 0xf9, 0x0f, 0xd8, 0xe0, 0xe4, 0xc9, 0xfa, 0xe4, 0xcc, //0x8600,
    0xfb, 0x22, 0x75, 0xf0, 0x10, 0xef, 0x2f, 0xff, 0xee, 0x33, 0xfe, 0xed, 0x33, 0xfd, 0xcc, 0x33, //0x8610,
    0xcc, 0xc8, 0x33, 0xc8, 0x10, 0xd7, 0x07, 0x9b, 0xec, 0x9a, 0xe8, 0x99, 0x40, 0x0a, 0xed, 0x9b, //0x8620,
    0xfd, 0xec, 0x9a, 0xfc, 0xe8, 0x99, 0xf8, 0x0f, 0xd5, 0xf0, 0xda, 0xe4, 0xcd, 0xfb, 0xe4, 0xcc, //0x8630,
    0xfa, 0xe4, 0xc8, 0xf9, 0x22, 0xeb, 0x9f, 0xf5, 0xf0, 0xea, 0x9e, 0x42, 0xf0, 0xe9, 0x9d, 0x42, //0x8640,
    0xf0, 0xe8, 0x9c, 0x45, 0xf0, 0x22, 0xe8, 0x60, 0x0f, 0xec, 0xc3, 0x13, 0xfc, 0xed, 0x13, 0xfd, //0x8650,
    0xee, 0x13, 0xfe, 0xef, 0x13, 0xff, 0xd8, 0xf1, 0x22, 0xe8, 0x60, 0x0f, 0xef, 0xc3, 0x33, 0xff, //0x8660,
    0xee, 0x33, 0xfe, 0xed, 0x33, 0xfd, 0xec, 0x33, 0xfc, 0xd8, 0xf1, 0x22, 0xe4, 0x93, 0xfc, 0x74, //0x8670,
    0x01, 0x93, 0xfd, 0x74, 0x02, 0x93, 0xfe, 0x74, 0x03, 0x93, 0xff, 0x22, 0xe6, 0xfb, 0x08, 0xe6, //0x8680,
    0xf9, 0x08, 0xe6, 0xfa, 0x08, 0xe6, 0xcb, 0xf8, 0x22, 0xec, 0xf6, 0x08, 0xed, 0xf6, 0x08, 0xee, //0x8690,
    0xf6, 0x08, 0xef, 0xf6, 0x22, 0xa4, 0x25, 0x82, 0xf5, 0x82, 0xe5, 0xf0, 0x35, 0x83, 0xf5, 0x83, //0x86a0,
    0x22, 0xd0, 0x83, 0xd0, 0x82, 0xf8, 0xe4, 0x93, 0x70, 0x12, 0x74, 0x01, 0x93, 0x70, 0x0d, 0xa3, //0x86b0,
    0xa3, 0x93, 0xf8, 0x74, 0x01, 0x93, 0xf5, 0x82, 0x88, 0x83, 0xe4, 0x73, 0x74, 0x02, 0x93, 0x68, //0x86c0,
    0x60, 0xef, 0xa3, 0xa3, 0xa3, 0x80, 0xdf, 0x90, 0x38, 0x04, 0x78, 0x52, 0x12, 0x0b, 0xfd, 0x90, //0x86d0,
    0x38, 0x00, 0xe0, 0xfe, 0xa3, 0xe0, 0xfd, 0xed, 0xff, 0xc3, 0x12, 0x0b, 0x9e, 0x90, 0x38, 0x10, //0x86e0,
    0x12, 0x0b, 0x92, 0x90, 0x38, 0x06, 0x78, 0x54, 0x12, 0x0b, 0xfd, 0x90, 0x38, 0x02, 0xe0, 0xfe, //0x86f0,
    0xa3, 0xe0, 0xfd, 0xed, 0xff, 0xc3, 0x12, 0x0b, 0x9e, 0x90, 0x38, 0x12, 0x12, 0x0b, 0x92, 0xa3, //0x8700,
    0xe0, 0xb4, 0x31, 0x07, 0x78, 0x52, 0x79, 0x52, 0x12, 0x0c, 0x13, 0x90, 0x38, 0x14, 0xe0, 0xb4, //0x8710,
    0x71, 0x15, 0x78, 0x52, 0xe6, 0xfe, 0x08, 0xe6, 0x78, 0x02, 0xce, 0xc3, 0x13, 0xce, 0x13, 0xd8, //0x8720,
    0xf9, 0x79, 0x53, 0xf7, 0xee, 0x19, 0xf7, 0x90, 0x38, 0x15, 0xe0, 0xb4, 0x31, 0x07, 0x78, 0x54, //0x8730,
    0x79, 0x54, 0x12, 0x0c, 0x13, 0x90, 0x38, 0x15, 0xe0, 0xb4, 0x71, 0x15, 0x78, 0x54, 0xe6, 0xfe, //0x8740,
    0x08, 0xe6, 0x78, 0x02, 0xce, 0xc3, 0x13, 0xce, 0x13, 0xd8, 0xf9, 0x79, 0x55, 0xf7, 0xee, 0x19, //0x8750,
    0xf7, 0x79, 0x52, 0x12, 0x0b, 0xd9, 0x09, 0x12, 0x0b, 0xd9, 0xaf, 0x47, 0x12, 0x0b, 0xb2, 0xe5, //0x8760,
    0x44, 0xfb, 0x7a, 0x00, 0xfd, 0x7c, 0x00, 0x12, 0x04, 0xd3, 0x78, 0x5a, 0xa6, 0x06, 0x08, 0xa6, //0x8770,
    0x07, 0xaf, 0x45, 0x12, 0x0b, 0xb2, 0xad, 0x03, 0x7c, 0x00, 0x12, 0x04, 0xd3, 0x78, 0x56, 0xa6, //0x8780,
    0x06, 0x08, 0xa6, 0x07, 0xaf, 0x48, 0x78, 0x54, 0x12, 0x0b, 0xb4, 0xe5, 0x43, 0xfb, 0xfd, 0x7c, //0x8790,
    0x00, 0x12, 0x04, 0xd3, 0x78, 0x5c, 0xa6, 0x06, 0x08, 0xa6, 0x07, 0xaf, 0x46, 0x7e, 0x00, 0x78, //0x87a0,
    0x54, 0x12, 0x0b, 0xb6, 0xad, 0x03, 0x7c, 0x00, 0x12, 0x04, 0xd3, 0x78, 0x58, 0xa6, 0x06, 0x08, //0x87b0,
    0xa6, 0x07, 0xc3, 0x78, 0x5b, 0xe6, 0x94, 0x08, 0x18, 0xe6, 0x94, 0x00, 0x50, 0x05, 0x76, 0x00, //0x87c0,
    0x08, 0x76, 0x08, 0xc3, 0x78, 0x5d, 0xe6, 0x94, 0x08, 0x18, 0xe6, 0x94, 0x00, 0x50, 0x05, 0x76, //0x87d0,
    0x00, 0x08, 0x76, 0x08, 0x78, 0x5a, 0x12, 0x0b, 0xc6, 0xff, 0xd3, 0x78, 0x57, 0xe6, 0x9f, 0x18, //0x87e0,
    0xe6, 0x9e, 0x40, 0x0e, 0x78, 0x5a, 0xe6, 0x13, 0xfe, 0x08, 0xe6, 0x78, 0x57, 0x12, 0x0c, 0x08, //0x87f0,
    0x80, 0x04, 0x7e, 0x00, 0x7f, 0x00, 0x78, 0x5e, 0x12, 0x0b, 0xbe, 0xff, 0xd3, 0x78, 0x59, 0xe6, //0x8800,
    0x9f, 0x18, 0xe6, 0x9e, 0x40, 0x0e, 0x78, 0x5c, 0xe6, 0x13, 0xfe, 0x08, 0xe6, 0x78, 0x59, 0x12, //0x8810,
    0x0c, 0x08, 0x80, 0x04, 0x7e, 0x00, 0x7f, 0x00, 0xe4, 0xfc, 0xfd, 0x78, 0x62, 0x12, 0x06, 0x99, //0x8820,
    0x78, 0x5a, 0x12, 0x0b, 0xc6, 0x78, 0x57, 0x26, 0xff, 0xee, 0x18, 0x36, 0xfe, 0x78, 0x66, 0x12, //0x8830,
    0x0b, 0xbe, 0x78, 0x59, 0x26, 0xff, 0xee, 0x18, 0x36, 0xfe, 0xe4, 0xfc, 0xfd, 0x78, 0x6a, 0x12, //0x8840,
    0x06, 0x99, 0x12, 0x0b, 0xce, 0x78, 0x66, 0x12, 0x06, 0x8c, 0xd3, 0x12, 0x06, 0x45, 0x40, 0x08, //0x8850,
    0x12, 0x0b, 0xce, 0x78, 0x66, 0x12, 0x06, 0x99, 0x78, 0x54, 0x12, 0x0b, 0xd0, 0x78, 0x6a, 0x12, //0x8860,
    0x06, 0x8c, 0xd3, 0x12, 0x06, 0x45, 0x40, 0x0a, 0x78, 0x54, 0x12, 0x0b, 0xd0, 0x78, 0x6a, 0x12, //0x8870,
    0x06, 0x99, 0x78, 0x61, 0xe6, 0x90, 0x60, 0x01, 0xf0, 0x78, 0x65, 0xe6, 0xa3, 0xf0, 0x78, 0x69, //0x8880,
    0xe6, 0xa3, 0xf0, 0x78, 0x55, 0xe6, 0xa3, 0xf0, 0x7d, 0x01, 0x78, 0x61, 0x12, 0x0b, 0xe9, 0x24, //0x8890,
    0x01, 0x12, 0x0b, 0xa6, 0x78, 0x65, 0x12, 0x0b, 0xe9, 0x24, 0x02, 0x12, 0x0b, 0xa6, 0x78, 0x69, //0x88a0,
    0x12, 0x0b, 0xe9, 0x24, 0x03, 0x12, 0x0b, 0xa6, 0x78, 0x6d, 0x12, 0x0b, 0xe9, 0x24, 0x04, 0x12, //0x88b0,
    0x0b, 0xa6, 0x0d, 0xbd, 0x05, 0xd4, 0xc2, 0x0e, 0xc2, 0x06, 0x22, 0x85, 0x08, 0x41, 0x90, 0x30, //0x88c0,
    0x24, 0xe0, 0xf5, 0x3d, 0xa3, 0xe0, 0xf5, 0x3e, 0xa3, 0xe0, 0xf5, 0x3f, 0xa3, 0xe0, 0xf5, 0x40, //0x88d0,
    0xa3, 0xe0, 0xf5, 0x3c, 0xd2, 0x34, 0xe5, 0x41, 0x12, 0x06, 0xb1, 0x09, 0x31, 0x03, 0x09, 0x35, //0x88e0,
    0x04, 0x09, 0x3b, 0x05, 0x09, 0x3e, 0x06, 0x09, 0x41, 0x07, 0x09, 0x4a, 0x08, 0x09, 0x5b, 0x12, //0x88f0,
    0x09, 0x73, 0x18, 0x09, 0x89, 0x19, 0x09, 0x5e, 0x1a, 0x09, 0x6a, 0x1b, 0x09, 0xad, 0x80, 0x09, //0x8900,
    0xb2, 0x81, 0x0a, 0x1d, 0x8f, 0x0a, 0x09, 0x90, 0x0a, 0x1d, 0x91, 0x0a, 0x1d, 0x92, 0x0a, 0x1d, //0x8910,
    0x93, 0x0a, 0x1d, 0x94, 0x0a, 0x1d, 0x98, 0x0a, 0x17, 0x9f, 0x0a, 0x1a, 0xec, 0x00, 0x00, 0x0a, //0x8920,
    0x38, 0x12, 0x0f, 0x74, 0x22, 0x12, 0x0f, 0x74, 0xd2, 0x03, 0x22, 0xd2, 0x03, 0x22, 0xc2, 0x03, //0x8930,
    0x22, 0xa2, 0x37, 0xe4, 0x33, 0xf5, 0x3c, 0x02, 0x0a, 0x1d, 0xc2, 0x01, 0xc2, 0x02, 0xc2, 0x03, //0x8940,
    0x12, 0x0d, 0x0d, 0x75, 0x1e, 0x70, 0xd2, 0x35, 0x02, 0x0a, 0x1d, 0x02, 0x0a, 0x04, 0x85, 0x40, //0x8950,
    0x4a, 0x85, 0x3c, 0x4b, 0x12, 0x0a, 0xff, 0x02, 0x0a, 0x1d, 0x85, 0x4a, 0x40, 0x85, 0x4b, 0x3c, //0x8960,
    0x02, 0x0a, 0x1d, 0xe4, 0xf5, 0x22, 0xf5, 0x23, 0x85, 0x40, 0x31, 0x85, 0x3f, 0x30, 0x85, 0x3e, //0x8970,
    0x2f, 0x85, 0x3d, 0x2e, 0x12, 0x0f, 0x46, 0x80, 0x1f, 0x75, 0x22, 0x00, 0x75, 0x23, 0x01, 0x74, //0x8980,
    0xff, 0xf5, 0x2d, 0xf5, 0x2c, 0xf5, 0x2b, 0xf5, 0x2a, 0x12, 0x0f, 0x46, 0x85, 0x2d, 0x40, 0x85, //0x8990,
    0x2c, 0x3f, 0x85, 0x2b, 0x3e, 0x85, 0x2a, 0x3d, 0xe4, 0xf5, 0x3c, 0x80, 0x70, 0x12, 0x0f, 0x16, //0x89a0,
    0x80, 0x6b, 0x85, 0x3d, 0x45, 0x85, 0x3e, 0x46, 0xe5, 0x47, 0xc3, 0x13, 0xff, 0xe5, 0x45, 0xc3, //0x89b0,
    0x9f, 0x50, 0x02, 0x8f, 0x45, 0xe5, 0x48, 0xc3, 0x13, 0xff, 0xe5, 0x46, 0xc3, 0x9f, 0x50, 0x02, //0x89c0,
    0x8f, 0x46, 0xe5, 0x47, 0xc3, 0x13, 0xff, 0xfd, 0xe5, 0x45, 0x2d, 0xfd, 0xe4, 0x33, 0xfc, 0xe5, //0x89d0,
    0x44, 0x12, 0x0f, 0x90, 0x40, 0x05, 0xe5, 0x44, 0x9f, 0xf5, 0x45, 0xe5, 0x48, 0xc3, 0x13, 0xff, //0x89e0,
    0xfd, 0xe5, 0x46, 0x2d, 0xfd, 0xe4, 0x33, 0xfc, 0xe5, 0x43, 0x12, 0x0f, 0x90, 0x40, 0x05, 0xe5, //0x89f0,
    0x43, 0x9f, 0xf5, 0x46, 0x12, 0x06, 0xd7, 0x80, 0x14, 0x85, 0x40, 0x48, 0x85, 0x3f, 0x47, 0x85, //0x8a00,
    0x3e, 0x46, 0x85, 0x3d, 0x45, 0x80, 0x06, 0x02, 0x06, 0xd7, 0x12, 0x0d, 0x7e, 0x90, 0x30, 0x24, //0x8a10,
    0xe5, 0x3d, 0xf0, 0xa3, 0xe5, 0x3e, 0xf0, 0xa3, 0xe5, 0x3f, 0xf0, 0xa3, 0xe5, 0x40, 0xf0, 0xa3, //0x8a20,
    0xe5, 0x3c, 0xf0, 0x90, 0x30, 0x23, 0xe4, 0xf0, 0x22, 0xc0, 0xe0, 0xc0, 0x83, 0xc0, 0x82, 0xc0, //0x8a30,
    0xd0, 0x90, 0x3f, 0x0c, 0xe0, 0xf5, 0x32, 0xe5, 0x32, 0x30, 0xe3, 0x74, 0x30, 0x36, 0x66, 0x90, //0x8a40,
    0x60, 0x19, 0xe0, 0xf5, 0x0a, 0xa3, 0xe0, 0xf5, 0x0b, 0x90, 0x60, 0x1d, 0xe0, 0xf5, 0x14, 0xa3, //0x8a50,
    0xe0, 0xf5, 0x15, 0x90, 0x60, 0x21, 0xe0, 0xf5, 0x0c, 0xa3, 0xe0, 0xf5, 0x0d, 0x90, 0x60, 0x29, //0x8a60,
    0xe0, 0xf5, 0x0e, 0xa3, 0xe0, 0xf5, 0x0f, 0x90, 0x60, 0x31, 0xe0, 0xf5, 0x10, 0xa3, 0xe0, 0xf5, //0x8a70,
    0x11, 0x90, 0x60, 0x39, 0xe0, 0xf5, 0x12, 0xa3, 0xe0, 0xf5, 0x13, 0x30, 0x01, 0x06, 0x30, 0x33, //0x8a80,
    0x03, 0xd3, 0x80, 0x01, 0xc3, 0x92, 0x09, 0x30, 0x02, 0x06, 0x30, 0x33, 0x03, 0xd3, 0x80, 0x01, //0x8a90,
    0xc3, 0x92, 0x0a, 0x30, 0x33, 0x0c, 0x30, 0x03, 0x09, 0x20, 0x02, 0x06, 0x20, 0x01, 0x03, 0xd3, //0x8aa0,
    0x80, 0x01, 0xc3, 0x92, 0x0b, 0x90, 0x30, 0x01, 0xe0, 0x44, 0x40, 0xf0, 0xe0, 0x54, 0xbf, 0xf0, //0x8ab0,
    0xe5, 0x32, 0x30, 0xe1, 0x14, 0x30, 0x34, 0x11, 0x90, 0x30, 0x22, 0xe0, 0xf5, 0x08, 0xe4, 0xf0, //0x8ac0,
    0x30, 0x00, 0x03, 0xd3, 0x80, 0x01, 0xc3, 0x92, 0x08, 0xe5, 0x32, 0x30, 0xe5, 0x12, 0x90, 0x56, //0x8ad0,
    0xa1, 0xe0, 0xf5, 0x09, 0x30, 0x31, 0x09, 0x30, 0x05, 0x03, 0xd3, 0x80, 0x01, 0xc3, 0x92, 0x0d, //0x8ae0,
    0x90, 0x3f, 0x0c, 0xe5, 0x32, 0xf0, 0xd0, 0xd0, 0xd0, 0x82, 0xd0, 0x83, 0xd0, 0xe0, 0x32, 0x90, //0x8af0,
    0x0e, 0x7e, 0xe4, 0x93, 0xfe, 0x74, 0x01, 0x93, 0xff, 0xc3, 0x90, 0x0e, 0x7c, 0x74, 0x01, 0x93, //0x8b00,
    0x9f, 0xff, 0xe4, 0x93, 0x9e, 0xfe, 0xe4, 0x8f, 0x3b, 0x8e, 0x3a, 0xf5, 0x39, 0xf5, 0x38, 0xab, //0x8b10,
    0x3b, 0xaa, 0x3a, 0xa9, 0x39, 0xa8, 0x38, 0xaf, 0x4b, 0xfc, 0xfd, 0xfe, 0x12, 0x05, 0x28, 0x12, //0x8b20,
    0x0d, 0xe1, 0xe4, 0x7b, 0xff, 0xfa, 0xf9, 0xf8, 0x12, 0x05, 0xb3, 0x12, 0x0d, 0xe1, 0x90, 0x0e, //0x8b30,
    0x69, 0xe4, 0x12, 0x0d, 0xf6, 0x12, 0x0d, 0xe1, 0xe4, 0x85, 0x4a, 0x37, 0xf5, 0x36, 0xf5, 0x35, //0x8b40,
    0xf5, 0x34, 0xaf, 0x37, 0xae, 0x36, 0xad, 0x35, 0xac, 0x34, 0xa3, 0x12, 0x0d, 0xf6, 0x8f, 0x37, //0x8b50,
    0x8e, 0x36, 0x8d, 0x35, 0x8c, 0x34, 0xe5, 0x3b, 0x45, 0x37, 0xf5, 0x3b, 0xe5, 0x3a, 0x45, 0x36, //0x8b60,
    0xf5, 0x3a, 0xe5, 0x39, 0x45, 0x35, 0xf5, 0x39, 0xe5, 0x38, 0x45, 0x34, 0xf5, 0x38, 0xe4, 0xf5, //0x8b70,
    0x22, 0xf5, 0x23, 0x85, 0x3b, 0x31, 0x85, 0x3a, 0x30, 0x85, 0x39, 0x2f, 0x85, 0x38, 0x2e, 0x02, //0x8b80,
    0x0f, 0x46, 0xe0, 0xa3, 0xe0, 0x75, 0xf0, 0x02, 0xa4, 0xff, 0xae, 0xf0, 0xc3, 0x08, 0xe6, 0x9f, //0x8b90,
    0xf6, 0x18, 0xe6, 0x9e, 0xf6, 0x22, 0xff, 0xe5, 0xf0, 0x34, 0x60, 0x8f, 0x82, 0xf5, 0x83, 0xec, //0x8ba0,
    0xf0, 0x22, 0x78, 0x52, 0x7e, 0x00, 0xe6, 0xfc, 0x08, 0xe6, 0xfd, 0x02, 0x04, 0xc1, 0xe4, 0xfc, //0x8bb0,
    0xfd, 0x12, 0x06, 0x99, 0x78, 0x5c, 0xe6, 0xc3, 0x13, 0xfe, 0x08, 0xe6, 0x13, 0x22, 0x78, 0x52, //0x8bc0,
    0xe6, 0xfe, 0x08, 0xe6, 0xff, 0xe4, 0xfc, 0xfd, 0x22, 0xe7, 0xc4, 0xf8, 0x54, 0xf0, 0xc8, 0x68, //0x8bd0,
    0xf7, 0x09, 0xe7, 0xc4, 0x54, 0x0f, 0x48, 0xf7, 0x22, 0xe6, 0xfc, 0xed, 0x75, 0xf0, 0x04, 0xa4, //0x8be0,
    0x22, 0x12, 0x06, 0x7c, 0x8f, 0x48, 0x8e, 0x47, 0x8d, 0x46, 0x8c, 0x45, 0x22, 0xe0, 0xfe, 0xa3, //0x8bf0,
    0xe0, 0xfd, 0xee, 0xf6, 0xed, 0x08, 0xf6, 0x22, 0x13, 0xff, 0xc3, 0xe6, 0x9f, 0xff, 0x18, 0xe6, //0x8c00,
    0x9e, 0xfe, 0x22, 0xe6, 0xc3, 0x13, 0xf7, 0x08, 0xe6, 0x13, 0x09, 0xf7, 0x22, 0xad, 0x39, 0xac, //0x8c10,
    0x38, 0xfa, 0xf9, 0xf8, 0x12, 0x05, 0x28, 0x8f, 0x3b, 0x8e, 0x3a, 0x8d, 0x39, 0x8c, 0x38, 0xab, //0x8c20,
    0x37, 0xaa, 0x36, 0xa9, 0x35, 0xa8, 0x34, 0x22, 0x93, 0xff, 0xe4, 0xfc, 0xfd, 0xfe, 0x12, 0x05, //0x8c30,
    0x28, 0x8f, 0x37, 0x8e, 0x36, 0x8d, 0x35, 0x8c, 0x34, 0x22, 0x78, 0x84, 0xe6, 0xfe, 0x08, 0xe6, //0x8c40,
    0xff, 0xe4, 0x8f, 0x37, 0x8e, 0x36, 0xf5, 0x35, 0xf5, 0x34, 0x22, 0x90, 0x0e, 0x8c, 0xe4, 0x93, //0x8c50,
    0x25, 0xe0, 0x24, 0x0a, 0xf8, 0xe6, 0xfe, 0x08, 0xe6, 0xff, 0x22, 0xe6, 0xfe, 0x08, 0xe6, 0xff, //0x8c60,
    0xe4, 0x8f, 0x3b, 0x8e, 0x3a, 0xf5, 0x39, 0xf5, 0x38, 0x22, 0x78, 0x4e, 0xe6, 0xfe, 0x08, 0xe6, //0x8c70,
    0xff, 0x22, 0xef, 0x25, 0xe0, 0x24, 0x4e, 0xf8, 0xe6, 0xfc, 0x08, 0xe6, 0xfd, 0x22, 0x78, 0x89, //0x8c80,
    0xef, 0x26, 0xf6, 0x18, 0xe4, 0x36, 0xf6, 0x22, 0x75, 0x89, 0x03, 0x75, 0xa8, 0x01, 0x75, 0xb8, //0x8c90,
    0x04, 0x75, 0x34, 0xff, 0x75, 0x35, 0x0e, 0x75, 0x36, 0x15, 0x75, 0x37, 0x0d, 0x12, 0x0e, 0x9a, //0x8ca0,
    0x12, 0x00, 0x09, 0x12, 0x0f, 0x16, 0x12, 0x00, 0x06, 0xd2, 0x00, 0xd2, 0x34, 0xd2, 0xaf, 0x75, //0x8cb0,
    0x34, 0xff, 0x75, 0x35, 0x0e, 0x75, 0x36, 0x49, 0x75, 0x37, 0x03, 0x12, 0x0e, 0x9a, 0x30, 0x08, //0x8cc0,
    0x09, 0xc2, 0x34, 0x12, 0x08, 0xcb, 0xc2, 0x08, 0xd2, 0x34, 0x30, 0x0b, 0x09, 0xc2, 0x36, 0x12, //0x8cd0,
    0x02, 0x6c, 0xc2, 0x0b, 0xd2, 0x36, 0x30, 0x09, 0x09, 0xc2, 0x36, 0x12, 0x00, 0x0e, 0xc2, 0x09, //0x8ce0,
    0xd2, 0x36, 0x30, 0x0e, 0x03, 0x12, 0x06, 0xd7, 0x30, 0x35, 0xd3, 0x90, 0x30, 0x29, 0xe5, 0x1e, //0x8cf0,
    0xf0, 0xb4, 0x10, 0x05, 0x90, 0x30, 0x23, 0xe4, 0xf0, 0xc2, 0x35, 0x80, 0xc1, 0xe4, 0xf5, 0x4b, //0x8d00,
    0x90, 0x0e, 0x7a, 0x93, 0xff, 0xe4, 0x8f, 0x37, 0xf5, 0x36, 0xf5, 0x35, 0xf5, 0x34, 0xaf, 0x37, //0x8d10,
    0xae, 0x36, 0xad, 0x35, 0xac, 0x34, 0x90, 0x0e, 0x6a, 0x12, 0x0d, 0xf6, 0x8f, 0x37, 0x8e, 0x36, //0x8d20,
    0x8d, 0x35, 0x8c, 0x34, 0x90, 0x0e, 0x72, 0x12, 0x06, 0x7c, 0xef, 0x45, 0x37, 0xf5, 0x37, 0xee, //0x8d30,
    0x45, 0x36, 0xf5, 0x36, 0xed, 0x45, 0x35, 0xf5, 0x35, 0xec, 0x45, 0x34, 0xf5, 0x34, 0xe4, 0xf5, //0x8d40,
    0x22, 0xf5, 0x23, 0x85, 0x37, 0x31, 0x85, 0x36, 0x30, 0x85, 0x35, 0x2f, 0x85, 0x34, 0x2e, 0x12, //0x8d50,
    0x0f, 0x46, 0xe4, 0xf5, 0x22, 0xf5, 0x23, 0x90, 0x0e, 0x72, 0x12, 0x0d, 0xea, 0x12, 0x0f, 0x46, //0x8d60,
    0xe4, 0xf5, 0x22, 0xf5, 0x23, 0x90, 0x0e, 0x6e, 0x12, 0x0d, 0xea, 0x02, 0x0f, 0x46, 0xe5, 0x40, //0x8d70,
    0x24, 0xf2, 0xf5, 0x37, 0xe5, 0x3f, 0x34, 0x43, 0xf5, 0x36, 0xe5, 0x3e, 0x34, 0xa2, 0xf5, 0x35, //0x8d80,
    0xe5, 0x3d, 0x34, 0x28, 0xf5, 0x34, 0xe5, 0x37, 0xff, 0xe4, 0xfe, 0xfd, 0xfc, 0x78, 0x18, 0x12, //0x8d90,
    0x06, 0x69, 0x8f, 0x40, 0x8e, 0x3f, 0x8d, 0x3e, 0x8c, 0x3d, 0xe5, 0x37, 0x54, 0xa0, 0xff, 0xe5, //0x8da0,
    0x36, 0xfe, 0xe4, 0xfd, 0xfc, 0x78, 0x07, 0x12, 0x06, 0x56, 0x78, 0x10, 0x12, 0x0f, 0x9a, 0xe4, //0x8db0,
    0xff, 0xfe, 0xe5, 0x35, 0xfd, 0xe4, 0xfc, 0x78, 0x0e, 0x12, 0x06, 0x56, 0x12, 0x0f, 0x9d, 0xe4, //0x8dc0,
    0xff, 0xfe, 0xfd, 0xe5, 0x34, 0xfc, 0x78, 0x18, 0x12, 0x06, 0x56, 0x78, 0x08, 0x12, 0x0f, 0x9a, //0x8dd0,
    0x22, 0x8f, 0x3b, 0x8e, 0x3a, 0x8d, 0x39, 0x8c, 0x38, 0x22, 0x12, 0x06, 0x7c, 0x8f, 0x31, 0x8e, //0x8de0,
    0x30, 0x8d, 0x2f, 0x8c, 0x2e, 0x22, 0x93, 0xf9, 0xf8, 0x02, 0x06, 0x69, 0x00, 0x00, 0x00, 0x00, //0x8df0,
    0x12, 0x01, 0x17, 0x08, 0x31, 0x15, 0x53, 0x54, 0x44, 0x20, 0x20, 0x20, 0x20, 0x20, 0x13, 0x01, //0x8e00,
    0x10, 0x01, 0x56, 0x40, 0x1a, 0x30, 0x29, 0x7e, 0x00, 0x30, 0x04, 0x20, 0xdf, 0x30, 0x05, 0x40, //0x8e10,
    0xbf, 0x50, 0x03, 0x00, 0xfd, 0x50, 0x27, 0x01, 0xfe, 0x60, 0x00, 0x11, 0x00, 0x3f, 0x05, 0x30, //0x8e20,
    0x00, 0x3f, 0x06, 0x22, 0x00, 0x3f, 0x01, 0x2a, 0x00, 0x3f, 0x02, 0x00, 0x00, 0x36, 0x06, 0x07, //0x8e30,
    0x00, 0x3f, 0x0b, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0x40, 0xbf, 0x30, 0x01, 0x00, //0x8e40,
    0xbf, 0x30, 0x29, 0x70, 0x00, 0x3a, 0x00, 0x00, 0xff, 0x3a, 0x00, 0x00, 0xff, 0x36, 0x03, 0x36, //0x8e50,
    0x02, 0x41, 0x44, 0x58, 0x20, 0x18, 0x10, 0x0a, 0x04, 0x04, 0x00, 0x03, 0xff, 0x64, 0x00, 0x00, //0x8e60,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04, 0x06, 0x06, 0x00, 0x03, 0x51, 0x00, 0x7a, //0x8e70,
    0x50, 0x3c, 0x28, 0x1e, 0x10, 0x10, 0x50, 0x2d, 0x28, 0x16, 0x10, 0x10, 0x02, 0x00, 0x10, 0x0c, //0x8e80,
    0x10, 0x04, 0x0c, 0x6e, 0x06, 0x05, 0x00, 0xa5, 0x5a, 0x00, 0xae, 0x35, 0xaf, 0x36, 0xe4, 0xfd, //0x8e90,
    0xed, 0xc3, 0x95, 0x37, 0x50, 0x33, 0x12, 0x0f, 0xe2, 0xe4, 0x93, 0xf5, 0x38, 0x74, 0x01, 0x93, //0x8ea0,
    0xf5, 0x39, 0x45, 0x38, 0x60, 0x23, 0x85, 0x39, 0x82, 0x85, 0x38, 0x83, 0xe0, 0xfc, 0x12, 0x0f, //0x8eb0,
    0xe2, 0x74, 0x03, 0x93, 0x52, 0x04, 0x12, 0x0f, 0xe2, 0x74, 0x02, 0x93, 0x42, 0x04, 0x85, 0x39, //0x8ec0,
    0x82, 0x85, 0x38, 0x83, 0xec, 0xf0, 0x0d, 0x80, 0xc7, 0x22, 0x78, 0xbe, 0xe6, 0xd3, 0x08, 0xff, //0x8ed0,
    0xe6, 0x64, 0x80, 0xf8, 0xef, 0x64, 0x80, 0x98, 0x22, 0x93, 0xff, 0x7e, 0x00, 0xe6, 0xfc, 0x08, //0x8ee0,
    0xe6, 0xfd, 0x12, 0x04, 0xc1, 0x78, 0xc1, 0xe6, 0xfc, 0x08, 0xe6, 0xfd, 0xd3, 0xef, 0x9d, 0xee, //0x8ef0,
    0x9c, 0x22, 0x78, 0xbd, 0xd3, 0xe6, 0x64, 0x80, 0x94, 0x80, 0x22, 0x25, 0xe0, 0x24, 0x0a, 0xf8, //0x8f00,
    0xe6, 0xfe, 0x08, 0xe6, 0xff, 0x22, 0xe5, 0x3c, 0xd3, 0x94, 0x00, 0x40, 0x0b, 0x90, 0x0e, 0x88, //0x8f10,
    0x12, 0x0b, 0xf1, 0x90, 0x0e, 0x86, 0x80, 0x09, 0x90, 0x0e, 0x82, 0x12, 0x0b, 0xf1, 0x90, 0x0e, //0x8f20,
    0x80, 0xe4, 0x93, 0xf5, 0x44, 0xa3, 0xe4, 0x93, 0xf5, 0x43, 0xd2, 0x06, 0x30, 0x06, 0x03, 0xd3, //0x8f30,
    0x80, 0x01, 0xc3, 0x92, 0x0e, 0x22, 0xa2, 0xaf, 0x92, 0x32, 0xc2, 0xaf, 0xe5, 0x23, 0x45, 0x22, //0x8f40,
    0x90, 0x0e, 0x5d, 0x60, 0x0e, 0x12, 0x0f, 0xcb, 0xe0, 0xf5, 0x2c, 0x12, 0x0f, 0xc8, 0xe0, 0xf5, //0x8f50,
    0x2d, 0x80, 0x0c, 0x12, 0x0f, 0xcb, 0xe5, 0x30, 0xf0, 0x12, 0x0f, 0xc8, 0xe5, 0x31, 0xf0, 0xa2, //0x8f60,
    0x32, 0x92, 0xaf, 0x22, 0xd2, 0x01, 0xc2, 0x02, 0xe4, 0xf5, 0x1f, 0xf5, 0x1e, 0xd2, 0x35, 0xd2, //0x8f70,
    0x33, 0xd2, 0x36, 0xd2, 0x01, 0xc2, 0x02, 0xf5, 0x1f, 0xf5, 0x1e, 0xd2, 0x35, 0xd2, 0x33, 0x22, //0x8f80,
    0xfb, 0xd3, 0xed, 0x9b, 0x74, 0x80, 0xf8, 0x6c, 0x98, 0x22, 0x12, 0x06, 0x69, 0xe5, 0x40, 0x2f, //0x8f90,
    0xf5, 0x40, 0xe5, 0x3f, 0x3e, 0xf5, 0x3f, 0xe5, 0x3e, 0x3d, 0xf5, 0x3e, 0xe5, 0x3d, 0x3c, 0xf5, //0x8fa0,
    0x3d, 0x22, 0xc0, 0xe0, 0xc0, 0x83, 0xc0, 0x82, 0x90, 0x3f, 0x0d, 0xe0, 0xf5, 0x33, 0xe5, 0x33, //0x8fb0,
    0xf0, 0xd0, 0x82, 0xd0, 0x83, 0xd0, 0xe0, 0x32, 0x90, 0x0e, 0x5f, 0xe4, 0x93, 0xfe, 0x74, 0x01, //0x8fc0,
    0x93, 0xf5, 0x82, 0x8e, 0x83, 0x22, 0x78, 0x7f, 0xe4, 0xf6, 0xd8, 0xfd, 0x75, 0x81, 0xcd, 0x02, //0x8fd0,
    0x0c, 0x98, 0x8f, 0x82, 0x8e, 0x83, 0x75, 0xf0, 0x04, 0xed, 0x02, 0x06, 0xa5, //0x8fe0
};

static int ov5640_sensor_read(struct v4l2_subdev *sd, unsigned char *reg,
                              unsigned char *value)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    u8 data[REG_STEP];
    struct i2c_msg msg;
    int ret,i;

    for(i = 0; i < REG_ADDR_STEP; i++)
        data[i] = reg[i];

    for(i = REG_ADDR_STEP; i < REG_STEP; i++)
        data[i] = 0xff;
    /*
         * Send out the register address...
         */
    msg.addr = client->addr;
    msg.flags = 0;
    msg.len = REG_ADDR_STEP;
    msg.buf = data;
    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret < 0) {
        csi_dev_err("Error %d on register write\n", ret);
        return ret;
    }
    /*
         * ...then read back the result.
         */

    msg.flags = I2C_M_RD;
    msg.len = REG_DATA_STEP;
    msg.buf = &data[REG_ADDR_STEP];

    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret >= 0) {
        for(i = 0; i < REG_DATA_STEP; i++)
            value[i] = data[i+REG_ADDR_STEP];
        ret = 0;
    }
    else {
        csi_dev_err("Error %d on register read\n", ret);
    }
    return ret;
}

static int ov5640_sensor_write(struct v4l2_subdev *sd, unsigned char *reg,
                               unsigned char *value)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct i2c_msg msg;
    unsigned char data[REG_STEP];
    int ret,i;

    for(i = 0; i < REG_ADDR_STEP; i++)
        data[i] = reg[i];
    for(i = REG_ADDR_STEP; i < REG_STEP; i++)
        data[i] = value[i-REG_ADDR_STEP];

    msg.addr = client->addr;
    msg.flags = 0;
    msg.len = REG_STEP;
    msg.buf = data;


    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret > 0) {
        ret = 0;
    }
    else if (ret < 0) {
        csi_dev_print("addr = 0x%4x, value = 0x%2x\n ",reg[0]*256+reg[1],value[0]);
        csi_dev_err("sensor_write error!\n");
    }
    return ret;
}

#if 0
static int sensor_write_im(struct v4l2_subdev *sd, unsigned int addr,
                           unsigned char value)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct i2c_msg msg;
    unsigned char data[REG_STEP];
    int ret;
    unsigned int retry=0;

    data[0] = (addr&0xff00)>>8;
    data[1] = addr&0x00ff;
    data[2] = value;

    msg.addr = client->addr;
    msg.flags = 0;
    msg.len = REG_STEP;
    msg.buf = data;

sensor_write_im_transfer:
    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret > 0) {
        ret = 0;
    }
    else if (ret < 0) {
        if(retry<3) {
            retry++;
            csi_dev_err("sensor_write retry %d!\n",retry);
            goto sensor_write_im_transfer;
        }

        csi_dev_err("addr = 0x%4x, value = 0x%4x\n ",addr,value);
        csi_dev_err("sensor_write error!\n");
    }
    return ret;
}
#endif

static int ov5640_sensor_write_array(struct v4l2_subdev *sd, struct regval_list *vals , uint size)
{
    int i,ret;
    unsigned int cnt;
    //	unsigned char rd;
    if (size == 0)
        return -EINVAL;

    for(i = 0; i < size ; i++)
    {
        if(vals->reg_num[0] == 0xff && vals->reg_num[1] == 0xff) {
            mdelay(vals->value[0]);
        }
        else {
            cnt=0;
            ret = ov5640_sensor_write(sd, vals->reg_num, vals->value);
            while( ret < 0 && cnt < 3)
            {
                if(ret<0)
                    csi_dev_err("sensor_write_err!\n");
                ret = ov5640_sensor_write(sd, vals->reg_num, vals->value);
                cnt++;
            }
            if(cnt>0)
                csi_dev_err("csi i2c retry cnt=%d\n",cnt);

            if(ret<0 && cnt >=3)
                return ret;
        }
        vals++;
    }

    return 0;
}

static int ov5640_sensor_write_continuous(struct v4l2_subdev *sd, int addr, char vals[] , uint size)
{
    int i,ret;
    struct regval_list reg_addr;

    if (size == 0)
        return -EINVAL;

    for(i = 0; i < size ; i++)
    {
        reg_addr.reg_num[0] = (addr&0xff00)>>8;
        reg_addr.reg_num[1] = (addr&0x00ff);

        ret = ov5640_sensor_write(sd, reg_addr.reg_num, &vals[i]);
        if (ret < 0)
        {
            csi_dev_err("sensor_write_err!\n");
            return ret;
        }
        addr++;
    }

    return 0;
}

/* stuff about auto focus */
static int ov5640_sensor_download_af_fw(struct v4l2_subdev *sd)
{
    int ret,cnt;
    struct regval_list regs;
    struct regval_list af_fw_reset_reg[] = {
    {{0x30,0x00},{0x20}},
};
    struct regval_list af_fw_start_reg[] = {
    {{0x30,0x22},{0x00}},
    {{0x30,0x23},{0x00}},
    {{0x30,0x24},{0x00}},
    {{0x30,0x25},{0x00}},
    {{0x30,0x26},{0x00}},
    {{0x30,0x27},{0x00}},
    {{0x30,0x28},{0x00}},
    {{0x30,0x29},{0x7f}},//0x7f
    {{0x30,0x00},{0x00}},	//start firmware for af
};

    //reset sensor MCU
    ret = ov5640_sensor_write_array(sd, af_fw_reset_reg, ARRAY_SIZE(af_fw_reset_reg));
    if(ret < 0) {
        csi_dev_err("reset sensor MCU error\n");
        return ret;
    }

    //download af fw
    ret =ov5640_sensor_write_continuous(sd, 0x8000, ov5640_sensor_af_fw_regs, ARRAY_SIZE(ov5640_sensor_af_fw_regs));
    if(ret < 0) {
        csi_dev_err("download af fw error\n");
        return ret;
    }
    //start af firmware
    ret = ov5640_sensor_write_array(sd, af_fw_start_reg, ARRAY_SIZE(af_fw_start_reg));
    if(ret < 0) {
        csi_dev_err("start af firmware error\n");
        return ret;
    }

    mdelay(10);
    //check the af firmware status
    regs.reg_num[0] = 0x30;
    regs.reg_num[1] = 0x29;
    regs.value[0] = 0xff;
    cnt = 0;
    while(regs.value[0]!=0x70) {
        mdelay(5);
        ret = ov5640_sensor_read(sd, regs.reg_num, regs.value);
        if (ret < 0)
        {
            csi_dev_err("sensor check the af firmware status err !\n");
            return ret;
        }
        cnt++;
        if(cnt > 200) {
            csi_dev_err("AF firmware check status time out !\n");
            return -EFAULT;
        }
    }
    csi_dev_print("AF firmware check status complete,0x3029 = 0x%x\n",regs.value[0]);

#if DEV_DBG_EN == 1	
    regs.reg_num[0] = 0x30;
    regs.reg_num[1] = 0x00;
    ov5640_sensor_read(sd, regs.reg_num, regs.value);
    csi_dev_print("0x3000 = 0x%x\n",regs.value[0]);

    regs.reg_num[0] = 0x30;
    regs.reg_num[1] = 0x04;
    ov5640_sensor_read(sd, regs.reg_num, regs.value);
    csi_dev_print("0x3004 = 0x%x\n",regs.value[0]);

    regs.reg_num[0] = 0x30;
    regs.reg_num[1] = 0x01;
    ov5640_sensor_read(sd, regs.reg_num, regs.value);
    csi_dev_print("0x3001 = 0x%x\n",regs.value[0]);

    regs.reg_num[0] = 0x30;
    regs.reg_num[1] = 0x05;
    ov5640_sensor_read(sd, regs.reg_num, regs.value);
    csi_dev_print("0x3005 = 0x%x\n",regs.value[0]);
#endif

    return 0;
}

static int ov5640_sensor_s_release_af(struct v4l2_subdev *sd)
{
    struct regval_list regs;
    int ret;
    //release focus
    csi_dev_print("sensor_s_release_af\n");

    //trig single af
    regs.reg_num[0] = 0x30;
    regs.reg_num[1] = 0x22;
    regs.value[0] = 0x03;
    ret = ov5640_sensor_write(sd, regs.reg_num, regs.value);
    if (ret < 0)
    {
        csi_dev_err("sensor tigger single af err !\n");
        return ret;
    }
    //release single af
    regs.reg_num[0] = 0x30;
    regs.reg_num[1] = 0x22;
    regs.value[0] = 0x08;
    ret = ov5640_sensor_write(sd, regs.reg_num, regs.value);
    if (ret < 0)
    {
        csi_dev_err("release focus err !\n");
        return ret;
    }
    return 0;
}

static int ov5640_sensor_s_af_zone(struct v4l2_subdev *sd, unsigned int xc, unsigned int yc)
{
    //struct sensor_info *info = to_state(sd);
    struct regval_list regs;
    int ret;

    csi_dev_print("sensor_s_af_zone\n");
    csi_dev_print("af zone input xc=%d,yc=%d\n",xc,yc);
#if 0	
    if(info->width == 0 || info->height == 0) {
        csi_dev_err("current width or height is zero!\n");
        return -EINVAL;
    }
#endif	
    xc = xc * 80 /640;

    yc = yc * 60 / 480;


    //csi_dev_dbg("af zone after xc=%d,yc=%d\n",xc,yc);

    //set x center
    regs.reg_num[0] = 0x30;
    regs.reg_num[1] = 0x24;
    regs.value[0] = xc;
    ret = ov5640_sensor_write(sd, regs.reg_num, regs.value);
    if (ret < 0)
    {
        csi_dev_err("sensor_s_af_zone_xc error!\n");
        return ret;
    }
    //set y center
    regs.reg_num[0] = 0x30;
    regs.reg_num[1] = 0x25;
    regs.value[0] = yc;
    ret = ov5640_sensor_write(sd, regs.reg_num, regs.value);
    if (ret < 0)
    {
        csi_dev_err("sensor_s_af_zone_yc error!\n");
        return ret;
    }

    //set af zone
    regs.reg_num[0] = 0x30;
    regs.reg_num[1] = 0x22;
    regs.value[0] = 0x81;
    ret = ov5640_sensor_write(sd, regs.reg_num, regs.value);
    if (ret < 0)
    {
        csi_dev_err("sensor_s_af_zone error!\n");
        return ret;
    }
    mdelay(5);
    return 0;
}

static int ov5640_sensor_s_pause_af(struct v4l2_subdev *sd)
{
    struct regval_list regs;
    int ret;
    //pause af poisition
    csi_dev_print("sensor_s_pause_af\n");
    regs.reg_num[0] = 0x30;
    regs.reg_num[1] = 0x22;
    regs.value[0] = 0x06;
    ret = ov5640_sensor_write(sd, regs.reg_num, regs.value);
    if (ret < 0)
    {
        csi_dev_err("sensor pause af err !\n");
        return ret;
    }
    mdelay(5);
    return 0;
}

static int ov5640_get_auto_focus_result(struct v4l2_subdev *sd,
                                        struct v4l2_control *ctrl)
{
    struct regval_list regs;
    int ret,cnt;

    regs.reg_num[0] = 0x30;
    regs.reg_num[1] = 0x23;
    regs.value[0] = 0xff;
    cnt = 0;
    while(regs.value[0]!=0x00)
    {
        mdelay(5);
        ret = ov5640_sensor_read(sd, regs.reg_num, regs.value);
        if (ret < 0)
        {
            ctrl->value = AUTO_FOCUS_FAILED;
            csi_dev_err("sensor get af status err !\n");
            return ret;
        }
        cnt++;
        if(cnt>1000) {
            ctrl->value = AUTO_FOCUS_FAILED;
            csi_dev_err("Single AF is timeout,value = 0x%x\n",regs.value[0]);
            return -EFAULT;
        }
    }

    ov5640_sensor_s_pause_af(sd);

    //ov5640_sensor_s_af_zone(sd, 640, 480);

    ctrl->value = AUTO_FOCUS_DONE;

    return 0;
}

static int ov5640_sensor_s_single_af(struct v4l2_subdev *sd)
{
    struct regval_list regs;
    int ret;
    csi_dev_print("sensor_s_single_af\n");
#if 1
    //trig single af
    regs.reg_num[0] = 0x30;
    regs.reg_num[1] = 0x23;
    regs.value[0] = 0x01;
    ret = ov5640_sensor_write(sd, regs.reg_num, regs.value);
    if (ret < 0)
    {
        csi_dev_err("sensor tigger single af err !\n");
        return ret;
    }
#endif
    regs.reg_num[0] = 0x30;
    regs.reg_num[1] = 0x22;
    regs.value[0] = 0x03;
    ret = ov5640_sensor_write(sd, regs.reg_num, regs.value);
    if (ret < 0)
    {
        csi_dev_err("sensor tigger single af err !\n");
        return ret;
    }
#if 0
    //wait for af complete
    regs.reg_num[0] = 0x30;
    regs.reg_num[1] = 0x23;
    regs.value[0] = 0xff;
    cnt = 0;
    while(regs.value[0]!=0x00)
    {
        mdelay(5);
        ret = ov5640_sensor_read(sd, regs.reg_num, regs.value);
        if (ret < 0)
        {
            csi_dev_err("sensor get af status err !\n");
            return ret;
        }
        cnt++;
        if(cnt>1000) {
            csi_dev_err("Single AF is timeout,value = 0x%x\n",regs.value[0]);
            return -EFAULT;
        }
    }
#else
    ;
#endif
    //csi_dev_print("Single AF is complete,value = 0x%x\n",regs.value[0]);
    return 0;
}
/* end add */

static int i2cc_get_reg(struct i2c_client *client, 
                        unsigned short reg, unsigned char *value)
{
    unsigned char buffer[2];
    int ret = 0;
    int err = 0;

    buffer[0] = (reg >> 8) & 0xFF;
    buffer[1] = reg & 0xFF;
    if (2 != (ret = i2c_master_send(client, buffer, 2))) {
        err = -2;
        OV_ERR("i2cc out error: ret == %d (should be 2)\n", ret);
    }

    if (1 != (ret = i2c_master_recv(client, buffer, 1))) {
        err = -1;
        OV_ERR("i2cc in error: ret == %d (should be 1)\n", ret);
    }
    //OV_INFO("ov5640 client: read 0x%x = 0x%x\n", reg, buffer[0]);
    *value = buffer[0];

    return (err);
}


static int i2cc_set_reg(struct i2c_client *client,
                        unsigned short reg, unsigned char value)
{
    unsigned char buffer[3];
    int ret = 0;
    int err = 0;

    buffer[0] = (reg >> 8) & 0xFF;
    buffer[1] = reg & 0xFF;
    buffer[2] = value;
    //OV_INFO("ov5640 client: writing 0x%x = 0x%x\n", reg, value);
    if (3 != (ret = i2c_master_send(client, buffer, 3))) {
        OV_ERR("i2cc out error: ret = %d (should be 3)\n", ret);
        err = -3;
    }
    return (err);
}


static inline struct ov5640_priv *to_ov5640(const struct i2c_client *client)
{
    return container_of(i2c_get_clientdata(client), struct ov5640_priv, subdev);
}


static int write_regs(struct i2c_client *client, 
                      const struct regval *regs, int array_len)
{
    int i;
    int ret = 0;

    for (i = 0; i < array_len; i++) {
        if ((ret = i2cc_set_reg(client, regs->reg, regs->val))) {
            OV_ERR("error to set reg:0x%d -> value:%d(index:%d)\n", regs->reg,
                   regs->val, i);
            break;
        }
        regs++;
    }

    //    INFO_BLUE("------*------- write array regs over -------*------ \n");

    return (ret);
}


static int ov5640_set_bus_param(struct soc_camera_device *icd, 
                                unsigned long flags)
{
    INFO_PURLPLE("\n");
    return 0;
}

static unsigned long ov5640_query_bus_param(struct soc_camera_device *icd)
{
    struct soc_camera_link *icl = to_soc_camera_link(icd);
    
    /* camera can do 10 bit transfers,  but omit it now */
    unsigned long flags = SOCAM_PCLK_SAMPLE_RISING | SOCAM_MASTER |
            SOCAM_VSYNC_ACTIVE_HIGH | SOCAM_HSYNC_ACTIVE_HIGH |
            SOCAM_DATA_ACTIVE_HIGH | SOCAM_DATAWIDTH_8;

    return soc_camera_apply_sensor_flags(icl, flags);
}


static int ov5640_g_chip_ident(struct v4l2_subdev *sd,
                               struct v4l2_dbg_chip_ident *id)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct ov5640_priv *priv = to_ov5640(client);

    id->ident = priv->model;
    id->revision = 0;

    return (0);
}


static const struct ov5640_win_size *select_win(
        unsigned int width, unsigned int height)
{
    const struct ov5640_win_size *ret = NULL;
    int size = ARRAY_SIZE(ov5640_wins);
    unsigned int diff = -1;
    unsigned int tmp;
    int i;

    for (i = 0; i < size; i++) {
        tmp = abs(width - ov5640_wins[i].width) +
                abs(height - ov5640_wins[i].height);
        if (tmp < diff) {
            diff = tmp;
            ret = ov5640_wins + i;
        }
    }
    return (ret);
}


static int ov5640_try_fmt(struct v4l2_subdev *sd,
                          struct v4l2_mbus_framefmt *vmf)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct ov5640_priv *priv = to_ov5640(client);
    const struct ov5640_win_size *win;
    int found = 0;
    int i;

    
    win = select_win(vmf->width, vmf->height);
    vmf->width = win->width;
    vmf->height = win->height;
    vmf->field = V4L2_FIELD_NONE;

    for (i = 0; i < ARRAY_SIZE(ov5640_cfmts); i++) {
        if (ov5640_cfmts[i].code == vmf->code) {
            found = 1;
            break;
        }
    }

    if (found) {
        vmf->colorspace = ov5640_cfmts[i].colorspace;
    } else {
        /* Unsupported format requested. Propose either */
        if (priv->cfmt) {
            /* the current one or */
            vmf->colorspace = priv->cfmt->colorspace;
            vmf->code = priv->cfmt->code;
        } else {
            /* the default one */
            vmf->colorspace = ov5640_cfmts[0].colorspace;
            vmf->code = ov5640_cfmts[0].code;
        }
    }

    INFO_GREEN("code:%d-%s %dX%d\n",
               vmf->code, win->name, vmf->width, vmf->height);

    return (0);
}



/* start or stop streaming from the device */
static int ov5640_s_stream(struct v4l2_subdev *sd, int enable)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct ov5640_priv *priv = to_ov5640(client);

    if (!enable) {
        INFO_PURLPLE("stream down\n");
        // set regs to enter sleep mode, already in DVP mode
        i2cc_set_reg(client, 0x3008, 0x42);
        return (0);
    }

    if (NULL == priv->win || NULL == priv->cfmt) {
        OV_ERR("win or cfmt select error!\n");
        return (-EPERM);
    }

    INFO_PURLPLE("stream on\n");
    //set regs to leave sleep mode, in DVP mode , needn't to set 0x300e to 0xc
    i2cc_set_reg(client, 0x3008, 0x02);
    
    INFO_GREEN("format: %d, win:%s\n", priv->cfmt->code, priv->win->name);




    // wait for 2 Vsync, and capture the 3rd frame, (1 / 7.5) * 2 =. 267ms
    mdelay(270);
#if 0
    if(0 == down_af_firmware_flag)
    {
        ov5640_sensor_download_af_fw(sd);

        down_af_firmware_flag = 1;
    }
#endif
    ov5640_sensor_s_single_af(sd);

    return (0);
}

static int reset_ov5640(struct i2c_client *client)
{
    int ret = i2cc_set_reg(client, 0x3008, 0x82);
    mdelay(5);
    return (ret);
}

#if 0  //remove by cym 20130807
static int ov5640_get_params(struct i2c_client *client, unsigned int *width, 
                             unsigned int *height, enum v4l2_mbus_pixelcode code)
{
    struct ov5640_priv *priv = to_ov5640(client);
    int ret = -EINVAL;
    int i;

    /* select format */
    priv->cfmt = NULL;
    for (i = 0; i < ARRAY_SIZE(ov5640_cfmts); i++) {
        if (code == ov5640_cfmts[i].code) {
            priv->cfmt = ov5640_cfmts + i;
            break;
        }
    }
    if (NULL == priv->cfmt) {
        return (ret);
    }

    priv->win = select_win(*width, *height);

    *width = priv->win->width;
    *height = priv->win->height;

    INFO_PURLPLE("current params: %s %dX%d\n",
                 priv->win->name, *width, *height);

    return (0);
}
#endif

static inline void init_setting_values(struct ov5640_priv *priv)
{
    priv->flip_flag = OV5640_HFLIP;
}

#if 1  //remove by cym 20130807
static void get_preview_params(struct i2c_client *client, 
                               unsigned int *exposure, unsigned short *maxlines, unsigned short *gain)
{
    unsigned char ret_h = 0;
    unsigned char ret_m = 0;
    unsigned char ret_l = 0;

    i2cc_get_reg(client, 0x3500, &ret_h);
    i2cc_get_reg(client, 0x3501, &ret_m);
    i2cc_get_reg(client, 0x3502, &ret_l);
    *exposure = ((ret_h << 16) + (ret_m << 8) + ret_l) >> 4;
    INFO_GREEN("expl:0x%x, expm:0x%x, exph:0x%x\n",
               ret_l, ret_m, ret_h);

    i2cc_get_reg(client, 0x350c, &ret_h);
    i2cc_get_reg(client, 0x350d, &ret_l);
    *maxlines = (ret_h << 8) + ret_l;

    //i2cc_get_reg(client, 0x350a, &ret_h);
    i2cc_get_reg(client, 0x350b, &ret_l);
    *gain = /*((ret_h & 0x1) << 8) + */ret_l;

    INFO_GREEN("exposure:0x%x, maxlines:0x%x, gain:0x%x\n",
               *exposure, *maxlines, *gain);
}
#endif

#if 1  //remove by cym 20130807
static void manual_set_exposure_and_gain(struct i2c_client *client,
                                         unsigned int p_exposure, unsigned short p_maxlines, unsigned short p_gain)
{
    unsigned char ret_h = 0;
    unsigned char ret_l = 0;
    unsigned char exp_h;
    unsigned char exp_m;
    unsigned char exp_l;
    unsigned char lines_10ms;
    unsigned short cap_maxlines;
    unsigned short cap_exposure;
    unsigned short cap_gain;
    //    unsigned short gain;
    unsigned int   cap_exp_gain;

    i2cc_get_reg(client, 0x350c, &ret_h);
    i2cc_get_reg(client, 0x350d, &ret_l);
    cap_maxlines = (ret_h << 8) + ret_l;

    //p_maxlines = 980;
    //cap_maxlines = 1964;
    INFO_GREEN("cap_maxlines: 0x%x\n", cap_maxlines);
    
    // for 50HZ, if 60HZ, devided by 12000
    lines_10ms = CAPTURE_FRAME_RATE * cap_maxlines / 10000 * 13 / 12;

    if (0 == p_maxlines) {
        p_maxlines = 1;
    }


    cap_exposure = ((p_exposure * CAPTURE_FRAME_RATE * cap_maxlines) /
                    (p_maxlines * PREVIEW_FRAME_RATE)) * 6 / 5;

    //cap_exp_gain = 1126 * cap_exposure * cap_gain;
    cap_exp_gain = cap_exposure * p_gain;  // in night mode, need multiply 2 again.
    //cap_exp_gain >>= 9;
    if (cap_exp_gain < (long)cap_maxlines * 16) {
        cap_exposure = cap_exp_gain / 16;
        if (cap_exposure > lines_10ms) {
            cap_exposure /= lines_10ms;
            cap_exposure *= lines_10ms;
        }
    } else {
        cap_exposure = cap_maxlines;
    }

    if (0 == cap_exposure) {
        cap_exposure = 1;
    }

    cap_gain = ((cap_exp_gain << 1) / cap_exposure + 1) >> 1;

    exp_l = (cap_exposure << 4) & 0xFF;
    exp_m = (cap_exposure >> 4) & 0xFF;
    exp_h = (cap_exposure >> 12) & 0xFF;

    INFO_GREEN("gain:0x%x, expl:0x%x, expm:0x%x, exph:0x%x, cap_maxlines:0x%x\n",
               cap_gain, exp_l, exp_m, exp_h, cap_maxlines);
    
    i2cc_set_reg(client, 0x350b, cap_gain);
    i2cc_set_reg(client, 0x3502, exp_l);
    i2cc_set_reg(client, 0x3501, exp_m);
    i2cc_set_reg(client, 0x3500, exp_h);

    /*
    * add delay-time, to avoid boundary problem between dark and bright
    */
    mdelay(100);
}
#endif

static int ov5640_set_params(struct i2c_client *client, unsigned int *width, 
                             unsigned int *height, enum v4l2_mbus_pixelcode code)
{
    struct ov5640_priv *priv = to_ov5640(client);
    int ret = -EINVAL;
    int i;
    const struct regval *reg_list = NULL;
    int list_len = 0;


     enum ov5640_mode   capture_mode = ov5640_mode_VGA_640_480;
     u32 tgt_fps=30;




    printk("%s(%d): code = %d\n", __FUNCTION__, __LINE__, code);

    printk("%s(%d): width = %d, height = %d\n", __FUNCTION__, __LINE__, *width, *height);

    /* select format */
    priv->cfmt = NULL;
    for (i = 0; i < ARRAY_SIZE(ov5640_cfmts); i++) {
        if (code == ov5640_cfmts[i].code) {
            priv->cfmt = ov5640_cfmts + i;
            break;
        }
    }
    if (NULL == priv->cfmt) {
        printk("%s(%d)\n", __FUNCTION__, __LINE__);
        return (ret);
    }

    /* select win size, now only one, so select directly */
    priv->win = select_win(*width, *height);

#if 0
    /* set hardware regs needed */
    if (RESV_VGA == priv->win->resv) {
        reset_ov5640(client);

        /* set default regs */
        write_regs(client, ov5640_init_regs, ARRAY_SIZE(ov5640_init_regs));
        init_setting_values(priv);
    }
#endif
    switch (priv->win->resv) {
    case RESV_VGA:{

        reg_list = ov5640_preview_vga_tbl;
        list_len = ARRAY_SIZE(ov5640_preview_vga_tbl);

        tgt_fps=30;
        capture_mode=ov5640_mode_VGA_640_480;

        break;
    }
    case RESV_XGA: {
        //   reg_list = ov5640_qsxga_to_xga_regs;
        //  list_len = ARRAY_SIZE(ov5640_qsxga_to_xga_regs);

        reg_list = ov5640_preview_xga_tbl;
        list_len = ARRAY_SIZE(ov5640_preview_xga_tbl);

        tgt_fps=30;
        capture_mode=ov5640_mode_XGA_1024_768;
        break;
    }
    case RESV_SXGA: {
        //    reg_list = ov5640_qsxga_to_sxga_regs;
        //    list_len = ARRAY_SIZE(ov5640_qsxga_to_sxga_regs);

        reg_list = ov5640_preview_sxga_tbl;
        list_len = ARRAY_SIZE(ov5640_preview_sxga_tbl);

        tgt_fps=30;
         capture_mode=ov5640_mode_XGA_1024_768; //no sxga on imax6

        break;
    }
    case RESV_720P: {

        reg_list = ov5640_preview_720p_tbl;
        list_len = ARRAY_SIZE(ov5640_preview_720p_tbl);


        tgt_fps=30;
        capture_mode=ov5640_mode_720P_1280_720;
        break;

    }
    case RESV_UXGA: {
        reg_list = ov5640_qsxga_to_uxga_regs;
        list_len = ARRAY_SIZE(ov5640_qsxga_to_uxga_regs);


       tgt_fps=30;
       capture_mode=ov5640_mode_720P_1280_720;  //no uxga  on imax6

        break;

    }
    case RESV_QXGA: {
        reg_list = ov5640_qsxga_to_qxga_regs;
        list_len = ARRAY_SIZE(ov5640_qsxga_to_qxga_regs);

        tgt_fps=15;
        capture_mode=ov5640_mode_QSXGA_2592_1944; // no qxga on imax6, result is not well.


        break;
    }
    case RESV_QSXGA: {

        reg_list = ov5640_qsxga_regs;
        list_len = ARRAY_SIZE(ov5640_qsxga_regs);

        tgt_fps=15;
        capture_mode=ov5640_mode_QSXGA_2592_1944; //not well

        break;
    }

    default:
        break;

    }

#if 1

    printk("%s(%d): capture_mode = %d  tgt_fps = %d \n", __FUNCTION__, __LINE__, capture_mode,tgt_fps);


        /* target frames per secound */
    enum ov5640_frame_rate frame_rate;



    if (tgt_fps == 15)
            frame_rate = ov5640_15_fps;
    else if (tgt_fps == 30)
            frame_rate = ov5640_30_fps;
    else {
            pr_err(" The camera frame rate is not supported!\n");
            return -EINVAL;
    }


    ret = ov5640_change_mode(frame_rate,capture_mode);
    if (ret < 0)
            return ret;

    ov5640_data.streamcap.timeperframe.denominator = frame_rate ;
    ov5640_data.streamcap.timeperframe.numerator =1 ;

    ov5640_data.streamcap.capturemode =capture_mode;


#endif
    //#if 0	//remove by cym 20130807
   //  if (RESV_VGA != priv->win->resv) {
#if 0
        unsigned int preview_exp;
        unsigned short preview_maxl;
        unsigned short preview_gain;

        /* manually set exposure and gain */
        //      i2cc_set_reg(client, 0x3503, 0x07); //dg use this.
        
        //      get_preview_params(client, &preview_exp, &preview_maxl, &preview_gain);

        //     write_regs(client, ov5640_qsxga_regs, ARRAY_SIZE(ov5640_qsxga_regs));
        //	write_regs(client, ov5640_qsxga_to_uxga_regs, ARRAY_SIZE(ov5640_qsxga_to_uxga_regs));

        if (NULL != reg_list) {
            write_regs(client, reg_list, list_len);
        }

        //      manual_set_exposure_and_gain(client, preview_exp, preview_maxl, preview_gain);
        

   // }
#endif


    *width = priv->win->width;
    *height = priv->win->height;

    INFO_PURLPLE("ok, params are width:%d-height:%d\n", *width, *height);

    return (0);
}



static int ov5640_g_fmt(struct v4l2_subdev *sd, 
                        struct v4l2_mbus_framefmt *vmf)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    //    struct ov5640_priv *priv = to_ov5640(client);

    struct soc_camera_device *icd = client->dev.platform_data;

#if 0
    if (NULL == priv->win || NULL == priv->cfmt) {
        unsigned int width = VGA_WIDTH;
        unsigned int height = VGA_HEIGHT;
        int ret = 0;

        ret = ov5640_get_params(client, &width, &height, 2);
        //cym V4L2_MBUS_FMT_YUYV8_2X8_BE);
        if (ret < 0) {
            return (ret);
        }
    }

    vmf->width = priv->win->width;
    vmf->height = priv->win->height;
    vmf->code = priv->cfmt->code;
    vmf->colorspace = priv->cfmt->colorspace;
    vmf->field = V4L2_FIELD_NONE;
#else
    //printk("%s(%d)\n", __FUNCTION__, __LINE__);
    vmf->width	= icd->user_width;
    //printk("%s(%d)\n", __FUNCTION__, __LINE__);
    vmf->height	= icd->user_height;
    //printk("%s(%d)\n", __FUNCTION__, __LINE__);
    vmf->code	= 2;//priv->cfmt->code;
    //printk("%s(%d)\n", __FUNCTION__, __LINE__);
    vmf->colorspace	= V4L2_COLORSPACE_JPEG;//priv->cfmt->colorspace;
    vmf->field	= V4L2_FIELD_NONE;
#endif
    INFO_GREEN("ok, get fmt w:%dXh:%d-code:%d-csp:%d\n",
               vmf->width, vmf->height, vmf->code, vmf->colorspace);

    return (0);
}


static int ov5640_s_fmt(struct v4l2_subdev *sd,
                        struct v4l2_mbus_framefmt *vmf)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct ov5640_priv *priv = to_ov5640(client);

    //ov5640_g_fmt(sd, vmf);

    //printk("*********** %s, line = %d(code = 0x%x, w:%d, h:%d)\n", __FUNCTION__, __LINE__,
    //						vmf->code, vmf->width, vmf->height);


    int ret = ov5640_set_params(client, &vmf->width, &vmf->height,
                                vmf->code);
    //printk("%s(%d): rest = %d\n", __FUNCTION__, __LINE__, ret);

    if (!ret)
        vmf->colorspace = priv->cfmt->colorspace;

    //INFO_PURLPLE("\n");

    return (ret);
}


//  TODO..... modify
static int ov5640_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *vcc)
{
    vcc->bounds.left = OV5640_COLUMN_SKIP;
    vcc->bounds.top  = OV5640_ROW_SKIP;
    vcc->bounds.width  = OV5640_MAX_WIDTH;
    vcc->bounds.height = OV5640_MAX_HEIGHT;
    vcc->defrect = vcc->bounds;  /* set default rect. */
    vcc->type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vcc->pixelaspect.numerator   = 1;
    vcc->pixelaspect.denominator = 1;

    INFO_PURLPLE("\n");

    return (0);
}


//  TODO..... modify
static int ov5640_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *vc)
{
    vc->c.left	= 0;
    vc->c.top	= 0;
    vc->c.width	= QSXGA_WIDTH;
    vc->c.height	= QSXGA_HEIGHT;
    vc->type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;

    INFO_BLUE("\n");

    return (0);
}


static int ov5640_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
                           enum v4l2_mbus_pixelcode *code)
{
    if (index >= ARRAY_SIZE(ov5640_cfmts))
        return -EINVAL;

    *code = ov5640_cfmts[index].code;
    OV_INFO("fmt index:%d\n", index);
    return 0;
}

static int ov5640_enum_framesizes(struct v4l2_subdev *sd,
                                  struct v4l2_frmsizeenum *fsize)
{
    //struct s5k4ecgx_state *state =
    //	container_of(sd, struct s5k4ecgx_state, sd);
    //struct i2c_client *client = v4l2_get_subdevdata(sd);
    //struct ov5640_priv *priv = to_ov5640(client);

    //printk("********************** %s(%d) **************************\n", __FUNCTION__, __LINE__);

    fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
    fsize->discrete.width = 640;//state->pix.width;
    fsize->discrete.height = 480;//state->pix.height;
    return 0;
}

static int ov5640_set_brightness(struct i2c_client *client, int bright)
{
    struct ov5640_priv *priv = to_ov5640(client);
    unsigned char reg5587, reg5588;

    if (bright < -4 || 4 < bright) {
        OV_ERR("brightness - %d is out of range[-4, 4]\n", bright);
        return (-ERANGE);
    }

    if (bright < 0) {
        reg5587 = 0x10 * (-bright);
        reg5588 = 0x09; /* bit[3] is Y bright sign */
    } else {
        reg5587 = 0x10 * bright;
        reg5588 = 0x01;
    }

    i2cc_set_reg(client, 0x5001, 0xff);
    i2cc_set_reg(client, 0x5587, reg5587);
    i2cc_set_reg(client, 0x5580, 0x04);
    i2cc_set_reg(client, 0x5588, reg5588);
    priv->brightness = bright;
    OV_INFO("brightness:%d, reg5587:0x%x, reg5588:0x%x\n",
            bright, reg5587, reg5588);
    return (0);
}


static int ov5640_set_contrast(struct i2c_client *client, int contrast)
{
    struct ov5640_priv *priv = to_ov5640(client);
    unsigned char y_gain, reg5588;

    if (contrast < -4 || 4 < contrast) {
        OV_ERR("contrast - %d is out of range[-4, 4]\n", contrast);
        return (-ERANGE);
    }

    if (0 == contrast) {
        reg5588 = 0x1;
    } else {
        reg5588 = 0x41;
    }
    y_gain = 0x20 + 0x4 * contrast;

    i2cc_set_reg(client, 0x5001, 0xff);
    i2cc_set_reg(client, 0x5580, 0x04);
    i2cc_set_reg(client, 0x5586, y_gain);
    i2cc_set_reg(client, 0x5585, y_gain);
    i2cc_set_reg(client, 0x5588, reg5588);
    priv->contrast = contrast;

    OV_INFO("contrast:%d, y_gain:0x%x, reg5588:0x%x\n", contrast, y_gain, reg5588);
    return (0);
}


/* auto, manual seperated is ok?? */
static int ov5640_set_saturation(struct i2c_client *client, int saturation)
{
    struct ov5640_priv *priv = to_ov5640(client);
    unsigned char uv_max, uv_min, reg5588;

    if (saturation < -4 || 4 < saturation) {
        OV_ERR("saturation - %d is out of range[-4, 4]\n", saturation);
        return (-ERANGE);
    }

    if (0 == saturation) {  /* different from application notes */
        uv_max = 0x40;   /* max value for UV adjust */
        uv_min = 0x10;   /* min value for UV adjust */
        reg5588 = 0x01;  /* bit[6]==0, auto saturation */
    } else {
        uv_max = 0x40 + 0x10 * saturation; /* U sat. */
        uv_min = uv_max; /* v sat */
        reg5588 = 0x41;  /* bit[6]==1, manual saturation */
    }

    i2cc_set_reg(client, 0x5001, 0xff);  /* init is 0xa3 */
    i2cc_set_reg(client, 0x5583, uv_max);
    i2cc_set_reg(client, 0x5584, uv_min);
    i2cc_set_reg(client, 0x5580, 0x02);  /* bit[1], enable(1)/disabe(0) saturation */
    i2cc_set_reg(client, 0x5588, reg5588);
    priv->saturation = saturation;
    OV_INFO("saturation:%d\n", saturation);
    return (0);
}

/* XXX:effect is reversed to note's picture exept -180. check it */
static int ov5640_set_hue(struct i2c_client *client, int hue)
{
    struct ov5640_priv *priv = to_ov5640(client);
    unsigned char reg5581, reg5582, reg5588;

    switch (hue) {
    case -180:
        reg5581 = 0x80;
        reg5582 = 0x00;
        reg5588 = 0x32;
        break;
    case -150:
        reg5581 = 0x6f;
        reg5582 = 0x40;
        reg5588 = 0x32;
        break;
    case -120:
        reg5581 = 0x40;
        reg5582 = 0x6f;
        reg5588 = 0x32;
        break;
    case -90:
        reg5581 = 0x00;
        reg5582 = 0x80;
        reg5588 = 0x02;
        break;
    case -60:
        reg5581 = 0x40;
        reg5582 = 0x6f;
        reg5588 = 0x02;
        break;
    case -30:
        reg5581 = 0x6f;
        reg5582 = 0x40;
        reg5588 = 0x02;
        break;
    case 0:
        reg5581 = 0x80;
        reg5582 = 0x00;
        reg5588 = 0x01;
        break;
    case 30:
        reg5581 = 0x6f;
        reg5582 = 0x40;
        reg5588 = 0x01;
        break;
    case 60:
        reg5581 = 0x40;
        reg5582 = 0x6f;
        reg5588 = 0x01;
        break;
    case 90:
        reg5581 = 0x00;
        reg5582 = 0x80;
        reg5588 = 0x31;
        break;
    case 120:
        reg5581 = 0x40;
        reg5582 = 0x6f;
        reg5588 = 0x31;
        break;
    case 150:
        reg5581 = 0x6f;
        reg5582 = 0x40;
        reg5588 = 0x31;
        break;
    default:
        OV_ERR("hue - %d is out of range[-180, 150]/step-30\n", hue);
        return (-ERANGE);
    }

    i2cc_set_reg(client, 0x5001, 0xff);
    i2cc_set_reg(client, 0x5580, 0x01);  /* XXXX:diff. from defualt value */
    i2cc_set_reg(client, 0x5581, reg5581);  /* hue cos coefficient */
    i2cc_set_reg(client, 0x5582, reg5582);  /* hue sin coefficient */
    i2cc_set_reg(client, 0x5588, reg5588);
    priv->hue = hue;
    OV_INFO("hue: %d, 5581:0x%x, 5582:0x%x, 5588:0x%x\n",
            hue, reg5581, reg5582, reg5588);

    return (0);
}


/* default value here is different from init one. */
static int ov5640_set_exposure_level(struct i2c_client *client, int level)
{
    struct ov5640_priv *priv = to_ov5640(client);
    unsigned char reg3a0f, reg3a10;
    unsigned char reg3a1b, reg3a1e;
    unsigned char reg3a11, reg3a1f;

    reg3a0f = 0x38 + 0x8 * level;
    reg3a10 = 0x30 + 0x8 * level;
    reg3a1b = reg3a0f;
    reg3a1e = reg3a10;
    reg3a1f = 0x10;

    switch (level) {
    case -5:  /* -1.7EV */
        reg3a11 = 0x20;
        break;
    case -4:  /* -1.3EV */
        reg3a11 = 0x30;
        break;
    case -3:  /* -1.0EV */
        reg3a11 = 0x41;
        break;
    case -2:  /* -0.7EV */
        reg3a11 = 0x51;
        break;
    case -1:  /* -0.3EV */
        reg3a11 = 0x61;
        break;
    case 0:  /* 0EV, default */
        reg3a11 = 0x61;
        break;
    case 1:  /* 0.3EV */
        reg3a11 = 0x71;
        break;
    case 2:  /* 0.7EV */
        reg3a11 = 0x80;
        reg3a1f = 0x20;
        break;
    case 3:  /* 1.0EV */
        reg3a11 = 0x90;
        reg3a1f = 0x20;
        break;
    case 4:  /* 1.3EV */
        reg3a11 = 0x91;
        reg3a1f = 0x20;
        break;
    case 5:  /* 1.7EV */
        reg3a11 = 0xa0;
        reg3a1f = 0x20;
        break;
    default:
        OV_ERR("exposure - %d is out of range[-5, 5]\n", level);
        return (-ERANGE);
    }

    OV_INFO("exposure: %d, 0x3a0f:0x%x, 0x3a10:0x%x\n", level, reg3a0f, reg3a10);
    //OV_INFO("0x3a1b:0x%x, 0x3a1e:0x%x\n", reg3a1b, reg3a1e);
    OV_INFO("0x3a11:0x%x, 0x3a1f:0x%x\n\n", reg3a11, reg3a1f);

    i2cc_set_reg(client, 0x3a0f, reg3a0f);  /* stable range high limit(enter) */
    i2cc_set_reg(client, 0x3a10, reg3a10);  /* stable range low limit(enter) */
    i2cc_set_reg(client, 0x3a11, reg3a11);  /* fast zone high limit */
    i2cc_set_reg(client, 0x3a1b, reg3a1b);  /* stable range high limit(go out) */
    i2cc_set_reg(client, 0x3a1e, reg3a1e);  /* stable range low limit(go out) */
    i2cc_set_reg(client, 0x3a1f, reg3a1f);  /* fast zone low limit */
    priv->exposure = level;
    
    return (0);
}


#define OV5640_FLIP_VAL  ((unsigned char)0x06)
#define OV5640_FLIP_MASK (~(OV5640_FLIP_VAL))
static int ov5640_set_flip(struct i2c_client *client, struct v4l2_control *ctrl)
{
    struct ov5640_priv *priv = to_ov5640(client);
    unsigned char reg3820, reg3821;

    OV_INFO("old flag: %d\n", priv->flip_flag);
    
    switch (ctrl->id) {
    case V4L2_CID_HFLIP:
        if (ctrl->value) {
            priv->flip_flag |= OV5640_HFLIP;
        } else {
            priv->flip_flag &= ~OV5640_HFLIP;
        }
        break;
    case V4L2_CID_VFLIP:
        if (ctrl->value) {
            priv->flip_flag |= OV5640_VFLIP;
        } else {
            priv->flip_flag &= ~OV5640_VFLIP;
        }
        break;
    default:
        OV_ERR("set flip out of range\n");
        return (-ERANGE);
    }

    OV_INFO("new flag: %d\n", priv->flip_flag);

    i2cc_get_reg(client, 0x3820, &reg3820);
    i2cc_get_reg(client, 0x3821, &reg3821);
    
    if (priv->flip_flag & OV5640_VFLIP) {
        reg3820 |= OV5640_FLIP_VAL;
    } else {
        reg3820 &= OV5640_FLIP_MASK;
    }
    
    if (priv->flip_flag & OV5640_HFLIP) {
        reg3821 |= OV5640_FLIP_VAL;
    } else {
        reg3821 &= OV5640_FLIP_MASK;
    }

    /* have a bug which flip a half picture only. */
    //i2cc_set_reg(client, 0x3212, 0x00);   /* enable group0, when add no flip */
    i2cc_set_reg(client, 0x3820, reg3820);
    i2cc_set_reg(client, 0x3821, reg3821);
    //i2cc_set_reg(client, 0x3212, 0x10);   /* end group0 */
    //i2cc_set_reg(client, 0x3212, 0xa1);   /* launch group1 */
    OV_INFO("0x3820:0x%x, 0x3821:0x%x\n", reg3820, reg3821);
    
    return (0);
}

static int ov5640_set_sharpness(struct i2c_client *client, int sharp)
{
    struct ov5640_priv *priv = to_ov5640(client);
    unsigned char reg5302;

    switch (sharp) {
    case -1:  /*auto sharpness*/
        break;
    case 0:   /* sharpness off */
        reg5302 = 0x00;
        break;
    case 1:
        reg5302 = 0x02;
        break;
    case 2:
        reg5302 = 0x04;
        break;
    case 3:
        reg5302 = 0x08;
        break;
    case 4:
        reg5302 = 0x0c;
        break;
    case 5:
        reg5302 = 0x10;
        break;
    case 6:
        reg5302 = 0x14;
        break;
    case 7:
        reg5302 = 0x18;
        break;
    case 8:
        reg5302 = 0x20;
        break;
    default:
        OV_ERR("set sharpness is out of range - %d[-1,8]\n", sharp);
        return (-ERANGE);

    }

    if (0 <= sharp) {
        i2cc_set_reg(client, 0x5308, 0x65);
        i2cc_set_reg(client, 0x5302, reg5302);
        OV_INFO("sharp:%d, 5302:0x%x\n", sharp, reg5302);
    } else {
        const struct regval ov5640_auto_sharpness[] = {
        {0x5308, 0x25},
        {0x5300, 0x08},
        {0x5301, 0x30},
        {0x5302, 0x10},
        {0x5303, 0x00},
        {0x5309, 0x08},
        {0x530a, 0x30},
        {0x530b, 0x04},
        {0x530c, 0x06},
    };
        int len = ARRAY_SIZE(ov5640_auto_sharpness);
        write_regs(client, ov5640_auto_sharpness, len);
        OV_INFO("sharp:%d, len:%d\n", sharp, len);
    }
    priv->sharpness = sharp;
    
    return (0);
}


static int ov5640_set_colorfx(struct i2c_client *client, int effect)
{
    struct ov5640_priv *priv = to_ov5640(client);
    unsigned char reg5583, reg5584, reg5001, reg5580;

    reg5001 = 0xff;
    reg5580 = 0x18;
    switch (effect) {
    case 0: /* normal */
        reg5001 = 0x7f;
        reg5580 = 0x00;
        break;
    case 1: /* black and white */
        reg5583 = 0x80;
        reg5584 = 0x80;
        break;
    case 2: /* sepia , antique */
        reg5583 = 0x40;
        reg5584 = 0xa0;
        break;
    case 3: /* negative */
        reg5001 = 0xff;
        reg5580 = 0x40;
        break;
    case 4: /* bluish */
        reg5583 = 0xa0;
        reg5584 = 0x40;
        break;
    case 5: /* greenish */
        reg5583 = 0x60;
        reg5584 = 0x60;
        break;
    case 6: /* reddish */
        reg5583 = 0x80;
        reg5584 = 0xc0;
        break;
    default:
        OV_ERR("set color effects out of range - %d[0,6]\n", effect);
        return (-ERANGE);

    }

    i2cc_set_reg(client, 0x5001, reg5001);
    i2cc_set_reg(client, 0x5580, reg5580);
    OV_INFO("effect:%d, 0x5001:0x%x, 0x5580:0x%x\n", effect, reg5001, reg5580);
    if (0 != effect && 3 != effect) {
        i2cc_set_reg(client, 0x5583, reg5583);
        i2cc_set_reg(client, 0x5584, reg5584);
        OV_INFO("0x5583:0x%x, 0x5584:0x%x\n", reg5583, reg5584);
    }

    priv->colorfx = effect;
    return (0);

}

/* Must be sorted from low to high control ID! */
static const u32 ov5640_user_ctrls[] = {
    V4L2_CID_USER_CLASS,
    V4L2_CID_BRIGHTNESS,
    V4L2_CID_CONTRAST,
    V4L2_CID_SATURATION,
    V4L2_CID_HUE,
    //	V4L2_CID_BLACK_LEVEL,
    V4L2_CID_AUTO_WHITE_BALANCE,
    //	V4L2_CID_DO_WHITE_BALANCE,
    //	V4L2_CID_RED_BALANCE,
    //	V4L2_CID_BLUE_BALANCE,
    //	V4L2_CID_GAMMA,
    V4L2_CID_EXPOSURE,
    V4L2_CID_AUTOGAIN,
    V4L2_CID_GAIN,
    V4L2_CID_HFLIP,
    V4L2_CID_VFLIP,
    V4L2_CID_POWER_LINE_FREQUENCY,
    //	V4L2_CID_HUE_AUTO,
    V4L2_CID_WHITE_BALANCE_TEMPERATURE,
    V4L2_CID_SHARPNESS,
    //	V4L2_CID_BACKLIGHT_COMPENSATION,
    //	V4L2_CID_CHROMA_AGC,
    //	V4L2_CID_CHROMA_GAIN,
    //	V4L2_CID_COLOR_KILLER,
    V4L2_CID_COLORFX,
    //	V4L2_CID_AUTOBRIGHTNESS,
    V4L2_CID_BAND_STOP_FILTER,
    //	V4L2_CID_ROTATE,
    //	V4L2_CID_BG_COLOR,
    0,
};

static const u32 ov5640_camera_ctrls[] = {
    V4L2_CID_CAMERA_CLASS,
    V4L2_CID_EXPOSURE_AUTO,
    //	V4L2_CID_EXPOSURE_ABSOLUTE,
    //	V4L2_CID_EXPOSURE_AUTO_PRIORITY,
    //	V4L2_CID_PAN_RELATIVE,
    //	V4L2_CID_TILT_RELATIVE,
    //	V4L2_CID_PAN_RESET,
    //	V4L2_CID_TILT_RESET,
    //	V4L2_CID_PAN_ABSOLUTE,
    //	V4L2_CID_TILT_ABSOLUTE,
    //	V4L2_CID_FOCUS_ABSOLUTE,
    //	V4L2_CID_FOCUS_RELATIVE,
    //	V4L2_CID_FOCUS_AUTO,
    //	V4L2_CID_ZOOM_ABSOLUTE,
    //	V4L2_CID_ZOOM_RELATIVE,
    //	V4L2_CID_ZOOM_CONTINUOUS,
    //	V4L2_CID_IRIS_ABSOLUTE,
    //	V4L2_CID_IRIS_RELATIVE,
    //	V4L2_CID_PRIVACY,
    //cym	V4L2_CID_SCENE_EXPOSURE,
    0,
};


static const u32 *ov5640_ctrl_classes[] = {
    ov5640_user_ctrls,
    ov5640_camera_ctrls,
    NULL,
};

static int ov5640_queryctrl(struct v4l2_subdev *sd,
                            struct v4l2_queryctrl *qc)
{
    //struct i2c_client *client = v4l2_get_subdevdata(sd);

    qc->id = v4l2_ctrl_next(ov5640_ctrl_classes, qc->id);
    if (qc->id == 0) {
        return (-EINVAL);
    }
    OV_INFO("%s: id-%s\n", __func__, v4l2_ctrl_get_name(qc->id));
    /* Fill in min, max, step and default value for these controls. */
    switch (qc->id) {
    /* Standard V4L2 controls */
    case V4L2_CID_USER_CLASS:
        return v4l2_ctrl_query_fill(qc, 0, 0, 0, 0);

    case V4L2_CID_BRIGHTNESS:
        return v4l2_ctrl_query_fill(qc, -4, 4, 1, 0);

    case V4L2_CID_CONTRAST:
        return v4l2_ctrl_query_fill(qc, -4, 4, 1, 0);

    case V4L2_CID_SATURATION:
        return v4l2_ctrl_query_fill(qc, -4, 4, 1, 0);

    case V4L2_CID_HUE:
        return v4l2_ctrl_query_fill(qc, -180, 150, 30, 0);
#if 0

        //	case V4L2_CID_BLACK_LEVEL:

    case V4L2_CID_AUTO_WHITE_BALANCE:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);

        //	case V4L2_CID_DO_WHITE_BALANCE:
        //	case V4L2_CID_RED_BALANCE:
        //	case V4L2_CID_BLUE_BALANCE:
        //	case V4L2_CID_GAMMA:
#endif
    case V4L2_CID_EXPOSURE:
        return v4l2_ctrl_query_fill(qc, -5, 5, 1, 0);
#if 0
    case V4L2_CID_AUTOGAIN:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
    case V4L2_CID_GAIN:
        return v4l2_ctrl_query_fill(qc, 0, 0xFFU, 1, 128);
#endif
    case V4L2_CID_HFLIP:
    case V4L2_CID_VFLIP:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
#if 0
    case V4L2_CID_POWER_LINE_FREQUENCY:
        return v4l2_ctrl_query_fill(qc, 0, 2, 1, 1);
        //	case V4L2_CID_HUE_AUTO:

    case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
        return v4l2_ctrl_query_fill(qc, 0, 3, 1, 0);
#endif
    case V4L2_CID_SHARPNESS:
        return v4l2_ctrl_query_fill(qc, -1, 8, 1, -1);
        //	case V4L2_CID_BACKLIGHT_COMPENSATION:
        //	case V4L2_CID_CHROMA_AGC:
        //	case V4L2_CID_CHROMA_GAIN:
        //	case V4L2_CID_COLOR_KILLER:

    case V4L2_CID_COLORFX:
        return v4l2_ctrl_query_fill(qc, 0, 6, 1, 0);
        //	case V4L2_CID_AUTOBRIGHTNESS:
#if 0
    case V4L2_CID_BAND_STOP_FILTER:
        return v4l2_ctrl_query_fill(qc, 0, 63, 1, 2);

        //	case V4L2_CID_ROTATE:
        //	case V4L2_CID_BG_COLOR:
#endif
    case V4L2_CID_CAMERA_CLASS:
        return v4l2_ctrl_query_fill(qc, 0, 0, 0, 0);
#if 0
    case V4L2_CID_EXPOSURE_AUTO:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);

        //	case V4L2_CID_EXPOSURE_ABSOLUTE:
        //	case V4L2_CID_EXPOSURE_AUTO_PRIORITY:
        //	case V4L2_CID_PAN_RELATIVE:
        //	case V4L2_CID_TILT_RELATIVE:
        //	case V4L2_CID_PAN_RESET:
        //	case V4L2_CID_TILT_RESET:
        //	case V4L2_CID_PAN_ABSOLUTE:
        //	case V4L2_CID_TILT_ABSOLUTE:
        //	case V4L2_CID_FOCUS_ABSOLUTE:
        //	case V4L2_CID_FOCUS_RELATIVE:
        //	case V4L2_CID_FOCUS_AUTO:
        //	case V4L2_CID_ZOOM_ABSOLUTE:
        //	case V4L2_CID_ZOOM_RELATIVE:
        //	case V4L2_CID_ZOOM_CONTINUOUS:
        //	case V4L2_CID_IRIS_ABSOLUTE:
        //	case V4L2_CID_IRIS_RELATIVE:
        //	case V4L2_CID_PRIVACY:

    case V4L2_CID_SCENE_EXPOSURE:
        return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
#endif
    default:
        OV_ERR("invalid control ctrl: %s\n", v4l2_ctrl_get_name(qc->id));
        qc->flags |= V4L2_CTRL_FLAG_DISABLED;
        return (-EINVAL);
    }

    return (0);
}


static int ov5640_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    //struct ov5640_priv *priv = to_ov5640(client);
    int ret = 0;
    //int val = 0;

    /* add by cym 20130812 */
    int err = 0;
    //int value = ctrl->value;
    /* end add */

    OV_INFO("%s: control ctrl- %s\n", __func__, v4l2_ctrl_get_name(ctrl->id));
    printk("%s: control ctrl- id = %d\n", __func__, ctrl->id-V4L2_CID_PRIVATE_BASE);

    switch (ctrl->id) {
    case V4L2_CID_BRIGHTNESS:
        ov5640_set_brightness(client, ctrl->value);
        break;

    case V4L2_CID_CONTRAST:
        ov5640_set_contrast(client, ctrl->value);
        break;

    case V4L2_CID_SATURATION:
        ov5640_set_saturation(client, ctrl->value);
        break;

    case V4L2_CID_HUE:
        ov5640_set_hue(client, ctrl->value);
        break;
#if 0

        //	case V4L2_CID_BLACK_LEVEL:
        //		ov5640_set_black_level(sd, ctrl->value);
        //		break;

    case V4L2_CID_AUTO_WHITE_BALANCE:
        ov5640_set_awb(client, ctrl->value);
        break;
        //	case V4L2_CID_DO_WHITE_BALANCE:
        //		ov5640_set_do_white_balance(sd, ctrl->value);
        //		break;
        //	case V4L2_CID_RED_BALANCE:
        //		ov5640_set_red_balance(sd, ctrl->value);
        //		break;
        //	case V4L2_CID_BLUE_BALANCE:
        //		ov5640_set_blue_balance(sd, ctrl->value);
        //		break;
        //	case V4L2_CID_GAMMA:
        //		ov5640_set_gamma(sd, ctrl->value);
        //		break;
#endif
    case V4L2_CID_EXPOSURE:
        ov5640_set_exposure_level(client, ctrl->value);
        break;
#if 0
    case V4L2_CID_AUTOGAIN:
        ov5640_set_autogain(sd, ctrl->value);
        break;
    case V4L2_CID_GAIN:
        ov5640_set_gain(sd, ctrl->value);
        break;
#endif
    case V4L2_CID_HFLIP:
    case V4L2_CID_VFLIP:
        ov5640_set_flip(client, ctrl);
        break;
#if 0
    case V4L2_CID_POWER_LINE_FREQUENCY:
        ov5640_set_power_line_frequency(sd, ctrl->value);
        break;
        //	case V4L2_CID_HUE_AUTO:
        //		ov5640_set_hue_auto(sd, ctrl->value);
        //		break;

    case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
        set_white_balance_temperature(client, ctrl->value);
        break;
#endif
    case V4L2_CID_SHARPNESS:
        ov5640_set_sharpness(client, ctrl->value);
        break;
        //	case V4L2_CID_BACKLIGHT_COMPENSATION:
        //		ov5640_set_backlight_compensation(sd, ctrl->value);
        //		break;
        //	case V4L2_CID_CHROMA_AGC:
        //		ov5640_set_chroma_agc(sd, ctrl->value);
        //		break;
        //	case V4L2_CID_CHROMA_GAIN:
        //		ov5640_set_chroma_gain(sd, ctrl->value);
        //		break;
        //	case V4L2_CID_COLOR_KILLER:
        //		ov5640_set_color_killer(sd, ctrl->value);
        //		break;

    case V4L2_CID_COLORFX:
        ov5640_set_colorfx(client, ctrl->value);
        break;
#if 0
        //	case V4L2_CID_AUTOBRIGHTNESS:
        //		ov5640_set_autobrightness(sd, ctrl->value);
        //		break;

    case V4L2_CID_BAND_STOP_FILTER:
        ov5640_set_band_stop_filter(sd, ctrl->value);
        break;

        //	case V4L2_CID_ROTATE:
        //		ov5640_set_rotate(sd, ctrl->value);
        //		break;
        //	case V4L2_CID_BG_COLOR:
        //		ov5640_set_bg_color(sd, ctrl->value);
        //		break;

    case V4L2_CID_EXPOSURE_AUTO:
        ov5640_set_exposure_auto(sd, ctrl->value);
        break;

        //	case V4L2_CID_EXPOSURE_ABSOLUTE:
        //	case V4L2_CID_EXPOSURE_AUTO_PRIORITY:
        //	case V4L2_CID_PAN_RELATIVE:
        //	case V4L2_CID_TILT_RELATIVE:
        //	case V4L2_CID_PAN_RESET:
        //	case V4L2_CID_TILT_RESET:
        //	case V4L2_CID_PAN_ABSOLUTE:
        //	case V4L2_CID_TILT_ABSOLUTE:
        //	case V4L2_CID_FOCUS_ABSOLUTE:
        //	case V4L2_CID_FOCUS_RELATIVE:
        //	case V4L2_CID_FOCUS_AUTO:
        //	case V4L2_CID_ZOOM_ABSOLUTE:
        //	case V4L2_CID_ZOOM_RELATIVE:
        //	case V4L2_CID_ZOOM_CONTINUOUS:
        //	case V4L2_CID_IRIS_ABSOLUTE:
        //	case V4L2_CID_IRIS_RELATIVE:
        //	case V4L2_CID_PRIVACY:

    case V4L2_CID_SCENE_EXPOSURE:
        ov5640_set_scene_exposure(sd, ctrl->value);
        break;
#endif
        /* add by cym 20130812 */
    case V4L2_CID_CAMERA_SET_AUTO_FOCUS:
        //printk("auto focus\n");
           err = ov5640_sensor_s_single_af(sd);
        break;
        /* end add */
    default:
        OV_ERR("invalid control ctrl: %s\n", v4l2_ctrl_get_name(ctrl->id));
        return (-EINVAL);
    }
    
    return (ret);
}

static int ov5640_init(struct v4l2_subdev *sd, u32 val)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct ov5640_priv *priv = to_ov5640(client);
    unsigned char pid;
    unsigned char version;
    //int err = -EINVAL;

    /* add by cym 20121121 */
    ov5640_power(1);
    /* end add */

    // check and show product ID and manufacturer ID
    i2cc_get_reg(client, 0x300a, &pid);
    i2cc_get_reg(client, 0x300b, &version);

    printk("%s: version = 0x%x%x\n", __FUNCTION__, pid, version);

    if (OV5640 != VERSION(pid, version)) {
        OV_ERR("ov5640 probed failed!!\n");
        return (-ENODEV);
    }

    priv->model = 1;//cym V4L2_IDENT_OV5640;

#if 1  //add by dg

    reset_ov5640(client);

    /* set default regs */
    write_regs(client, ov5640_init_regs, ARRAY_SIZE(ov5640_init_regs));
    init_setting_values(priv);


    ov5640_sensor_download_af_fw(sd);

    ov5640_sensor_s_single_af(sd);

#endif



    INFO_BLUE("ov5640 device probed success\n");

    return 0;
}


static int ov5640_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct ov5640_priv *priv = to_ov5640(client);

    int err = 0;

    OV_INFO("%s: control ctrl- %s\n", __func__, v4l2_ctrl_get_name(ctrl->id));
    printk("%s: control ctrl- id = %d\n", __func__, ctrl->id);

    switch (ctrl->id) {

    case V4L2_CID_BRIGHTNESS:
        ctrl->value = priv->brightness;
        break;

    case V4L2_CID_CONTRAST:
        ctrl->value = priv->contrast;
        break;

    case V4L2_CID_SATURATION:
        ctrl->value = priv->saturation;
        break;
    case V4L2_CID_HUE:
        ctrl->value = priv->hue;
        break;
        
        //  case V4L2_CID_BLACK_LEVEL:
        //      ctrl->value = priv->black_level;
        //      break;
#if 0
    case V4L2_CID_AUTO_WHITE_BALANCE:
        ctrl->value = priv->auto_white_balance;
        break;
        //  case V4L2_CID_DO_WHITE_BALANCE:
        //      ctrl->value = priv->do_white_balance;
        //      break;
        //  case V4L2_CID_RED_BALANCE:
        //      ctrl->value = priv->red_balance;
        //      break;
        //  case V4L2_CID_BLUE_BALANCE:
        //      ctrl->value = priv->blue_balance;
        //      break;
        //  case V4L2_CID_GAMMA:
        //      ctrl->value = priv->gamma;
        //      break;
#endif
    case V4L2_CID_EXPOSURE:
        ctrl->value = priv->exposure;
        break;
#if 0
    case V4L2_CID_AUTOGAIN:
        ctrl->value = priv->autogain;
        break;
    case V4L2_CID_GAIN:
        ctrl->value = priv->gain;
        break;
#endif
    case V4L2_CID_HFLIP:
        ctrl->value = !!(priv->flip_flag & OV5640_HFLIP);
        break;
    case V4L2_CID_VFLIP:
        ctrl->value = !!(priv->flip_flag & OV5640_VFLIP);
        break;
#if 0
    case V4L2_CID_POWER_LINE_FREQUENCY:
        ctrl->value = priv->power_line_frequency;
        break;
        //  case V4L2_CID_HUE_AUTO:
        //      ctrl->value = priv->hue_auto;
        //      break;

    case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
        ctrl->value = priv->white_balance_temperature;
        break;
#endif
    case V4L2_CID_SHARPNESS:
        ctrl->value = priv->sharpness;
        break;
        //  case V4L2_CID_BACKLIGHT_COMPENSATION:
        //      ctrl->value = priv->backlight_compensation;
        //      break;
        //  case V4L2_CID_CHROMA_AGC:
        //      ctrl->value = priv->chroma_agc;
        //      break;
        //  case V4L2_CID_CHROMA_GAIN:
        //      ctrl->value = priv->chroma_gain;
        //      break;
        //  case V4L2_CID_COLOR_KILLER:
        //      ctrl->value = priv->color_killer;
        //      break;

    case V4L2_CID_COLORFX:
        ctrl->value = priv->colorfx;
        break;
#if 0
        //  case V4L2_CID_AUTOBRIGHTNESS:
        //      ctrl->value = priv->autobrightness;
        //      break;

    case V4L2_CID_BAND_STOP_FILTER:
        ctrl->value = priv->band_stop_filter;
        break;

        //  case V4L2_CID_ROTATE:
        //      ctrl->value = priv->rotate;
        //      break;
        //  case V4L2_CID_BG_COLOR:
        //      ctrl->value = priv->bg_color;
        //      break;

    case V4L2_CID_EXPOSURE_AUTO:
        ctrl->value = priv->exposure_auto;
        break;

        //  case V4L2_CID_EXPOSURE_ABSOLUTE:
        //  case V4L2_CID_EXPOSURE_AUTO_PRIORITY:
        //  case V4L2_CID_PAN_RELATIVE:
        //  case V4L2_CID_TILT_RELATIVE:
        //  case V4L2_CID_PAN_RESET:
        //  case V4L2_CID_TILT_RESET:
        //  case V4L2_CID_PAN_ABSOLUTE:
        //  case V4L2_CID_TILT_ABSOLUTE:
        //  case V4L2_CID_FOCUS_ABSOLUTE:
        //  case V4L2_CID_FOCUS_RELATIVE:
        //  case V4L2_CID_FOCUS_AUTO:
        //  case V4L2_CID_ZOOM_ABSOLUTE:
        //  case V4L2_CID_ZOOM_RELATIVE:
        //  case V4L2_CID_ZOOM_CONTINUOUS:
        //  case V4L2_CID_IRIS_ABSOLUTE:
        //  case V4L2_CID_IRIS_RELATIVE:
        //  case V4L2_CID_PRIVACY:

    case V4L2_CID_SCENE_EXPOSURE:
        ctrl->value = priv->scene_exposure;
        break;
#endif

        /* add by cym 20130829 */
    case V4L2_CID_CAMERA_AUTO_FOCUS_RESULT:
        err = ov5640_get_auto_focus_result(sd, ctrl);
        break;
        /* end add */

    default:
        OV_ERR("invalid control ctrl: %s\n", v4l2_ctrl_get_name(ctrl->id));
        return (-EINVAL);
    }

    return (0);
}


static int ov5640_s_ext_ctrls(struct v4l2_subdev *sd, 
                              struct v4l2_ext_controls *ctrls)
{
    //struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct v4l2_control ctrl;

    int i;
    int err = 0;

    if ((ctrls->ctrl_class != V4L2_CTRL_CLASS_USER)
            && (ctrls->ctrl_class != V4L2_CTRL_CLASS_CAMERA)) {
        OV_ERR("ctrl class[%s] is illegal.\n", v4l2_ctrl_get_name(ctrls->ctrl_class));
        return (-EINVAL);
    }

    for (i = 0; i < ctrls->count; i++) {
        ctrl.id = ctrls->controls[i].id;
        ctrl.value = ctrls->controls[i].value;
        err = ov5640_s_ctrl(sd, &ctrl);
        //ctrls->controls[i].value = ctrl.value;
        if (err) {
            ctrls->error_idx = i;
            break;
        }
    }

    return (err);
}

static int ov5640_g_ext_ctrls(struct v4l2_subdev *sd, 
                              struct v4l2_ext_controls *ctrls)
{
    //struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct v4l2_control ctrl;

    int i;
    int err = 0;

    if ((ctrls->ctrl_class != V4L2_CTRL_CLASS_USER)
            && (ctrls->ctrl_class != V4L2_CTRL_CLASS_CAMERA)) {
        OV_ERR("ctrl class[%s] is illegal.\n", v4l2_ctrl_get_name(ctrls->ctrl_class));
        return (-EINVAL);
    }

    for (i = 0; i < ctrls->count; i++) {
        ctrl.id = ctrls->controls[i].id;
        //ctrl.value = ctrls->controls[i].value;
        err = ov5640_g_ctrl(sd, &ctrl);
        ctrls->controls[i].value = ctrl.value;
        if (err) {
            ctrls->error_idx = i;
            break;
        }
    }

    return (err);
}


static int ov5640_try_ctrl(struct v4l2_subdev *sd,
                           struct v4l2_ext_control *ctrl)
{
    struct v4l2_queryctrl qctrl;
    const char **menu_items = NULL;
    int err;

    qctrl.id = ctrl->id;
    err = ov5640_queryctrl(sd, &qctrl);
    if (err)
        return (err);
    if (qctrl.type == V4L2_CTRL_TYPE_MENU)
        menu_items = v4l2_ctrl_get_menu(qctrl.id);
    return v4l2_ctrl_check(ctrl, &qctrl, menu_items);
}

static int ov5640_try_ext_ctrls(struct v4l2_subdev *sd, 
                                struct v4l2_ext_controls *ctrls)
{
    //struct i2c_client *client = v4l2_get_subdevdata(sd);

    int i;
    int err = 0;

    if ((ctrls->ctrl_class != V4L2_CTRL_CLASS_USER)
            && (ctrls->ctrl_class != V4L2_CTRL_CLASS_CAMERA)) {
        OV_ERR("ctrl_class[%s] is illegal.\n", v4l2_ctrl_get_name(ctrls->ctrl_class));
        return (-EINVAL);
    }

    for (i = 0; i < ctrls->count; i++) {
        err = ov5640_try_ctrl(sd, &ctrls->controls[i]);
        if (err) {
            ctrls->error_idx = i;
            break;
        }
    }
    return (err);
}


#ifdef CONFIG_VIDEO_ADV_DEBUG

static int ov5640_g_register(struct v4l2_subdev *sd,
                             struct v4l2_dbg_register *reg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    unsigned char val;
    int err;

    reg->size = 2;
    if (reg->reg > 0x603f) {
        return (-EINVAL);
    }
    err = i2cc_get_reg(client, reg->reg, &val);
    if (err) {
        return (err);
    }
    reg->val = val;
    return (0);
}

static int ov5640_s_register(struct v4l2_subdev *sd,
                             struct v4l2_dbg_register *reg)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);

    if (reg->reg > 0x603f) {
        return (-EINVAL);
    }
    return (i2cc_set_reg(client, (unsigned short)reg->reg,(unsigned char)reg->val));
}

#endif

static struct soc_camera_ops ov5640_ops = {
    .set_bus_param = ov5640_set_bus_param,
    .query_bus_param = ov5640_query_bus_param,
    //.controls = ov5640_controls,
    //.num_controls = ARRAY_SIZE(ov5640_controls),
};


static struct v4l2_subdev_core_ops ov5640_subdev_core_ops = {
    .init = ov5640_init,//ioctl_dev_init,//ov5640_init,
    .g_ctrl		= ov5640_g_ctrl,
    .s_ctrl		= ov5640_s_ctrl,
    .queryctrl  = ov5640_queryctrl,
    .g_ext_ctrls = ov5640_g_ext_ctrls,
    .s_ext_ctrls = ov5640_s_ext_ctrls,
    .try_ext_ctrls = ov5640_try_ext_ctrls,
    .g_chip_ident	= ov5640_g_chip_ident,
#ifdef CONFIG_VIDEO_ADV_DEBUG
    .g_register	= ov5640_g_register,
    .s_register	= ov5640_s_register,
#endif
};

static struct v4l2_subdev_video_ops ov5640_subdev_video_ops = {
    .s_stream	= ov5640_s_stream,
    //.g_mbus_fmt	= ov5640_g_fmt,
    .s_mbus_fmt	= ov5640_s_fmt,
    .try_mbus_fmt	= ov5640_try_fmt,
    .cropcap	= ov5640_cropcap,
    .g_crop		= ov5640_g_crop,
    .enum_mbus_fmt	= ov5640_enum_fmt,
    .enum_framesizes = ov5640_enum_framesizes,
};


static struct v4l2_subdev_ops  ov5640_subdev_ops = {
    .core = &ov5640_subdev_core_ops,
    .video = &ov5640_subdev_video_ops,
};


#if 0  //remove by cym 20130807
static int ov5640_video_probe(struct soc_camera_device *icd, 
                              struct i2c_client *client)
{
    struct ov5640_priv *priv = to_ov5640(client);
    unsigned char pid;
    unsigned char version;

    /*
         * We must have a parent by now. And it cannot be a wrong one.
         * So this entire test is completely redundant.
         */
    if (NULL == icd->dev.parent ||
            to_soc_camera_host(icd->dev.parent)->nr != icd->iface) {
        OV_ERR("error : has a wrong parent\n");
        //return (-ENODEV);
    }

    // check and show product ID and manufacturer ID
    i2cc_get_reg(client, 0x300a, &pid);
    i2cc_get_reg(client, 0x300b, &version);

    if (OV5640 != VERSION(pid, version)) {
        OV_ERR("ov5640 probed failed!!\n");
        return (-ENODEV);
    }
    
    priv->model = 1;//cym V4L2_IDENT_OV5640;

    INFO_BLUE("ov5640 device probed success\n");
    
    return (0);
}
#endif


static int ov5640_probe(struct i2c_client *client, 
                        const struct i2c_device_id *did)
{
    struct ov5640_priv *priv;
    struct v4l2_subdev *sd = NULL;
    struct soc_camera_device *icd = client->dev.platform_data;
    //struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
    struct soc_camera_link *icl;
    int ret = 0;

    OV_INFO("ov5640 probe start...\n");
    if (NULL == icd) {
        ret = -EINVAL;
        OV_ERR("error: missing soc camera device\n");
        goto failed;
    }


    /* Set initial values for the sensor struct. */
    memset(&ov5640_data, 0, sizeof(ov5640_data));
    ov5640_data.mclk = 24000000; /* 6 - 54 MHz, typical 24MHz */
  //  ov5640_data.mclk = plat_data->mclk;
//    ov5640_data.mclk_source = plat_data->mclk_source;
//    ov5640_data.csi = plat_data->csi;
 //   ov5640_data.io_init = plat_data->io_init;

    ov5640_data.i2c_client = client;
    ov5640_data.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    ov5640_data.pix.width = 640;
    ov5640_data.pix.height = 480;
    ov5640_data.streamcap.capability = V4L2_MODE_HIGHQUALITY |
                                       V4L2_CAP_TIMEPERFRAME;
    ov5640_data.streamcap.capturemode = 0;
    ov5640_data.streamcap.timeperframe.denominator = DEFAULT_FPS;
    ov5640_data.streamcap.timeperframe.numerator = 1;



    /* add by cym 20130605 */
#ifndef CONFIG_TC4_EVT
    ov_vdd18_cam_regulator = regulator_get(NULL, "vdd18_cam");

    if (IS_ERR(ov_vdd18_cam_regulator)) {
        printk("%s: failed to get %s\n", __func__, "vdd18_cam");
        ret = -ENODEV;
        goto err_regulator;
    }

    ov_vdd28_cam_regulator = regulator_get(NULL, "vdda28_2m");

    if (IS_ERR(ov_vdd28_cam_regulator)) {
        printk("%s: failed to get %s\n", __func__, "vdda28_2m");
        ret = -ENODEV;
        goto err_regulator;
    }

    ov_vddaf_cam_regulator = regulator_get(NULL, "vdd28_af");

    if (IS_ERR(ov_vddaf_cam_regulator)) {
        printk("%s: failed to get %s\n", __func__, "vdd28_af");
        ret = -ENODEV;
        goto err_regulator;
    }

    ov_vdd5m_cam_regulator = regulator_get(NULL, "vdd28_cam");

    if (IS_ERR(ov_vdd5m_cam_regulator)) {
        printk("%s: failed to get %s\n", __func__, "vdd28_cam");
        ret = -ENODEV;
        goto err_regulator;
    }

    // ov5640_power(1);
#endif
    /* end add */

    icl = to_soc_camera_link(icd);
    if (NULL == icl) {
        ret = -EINVAL;
        OV_ERR("error: missing soc camera link\n");
        //cym goto failed;
    }
    
    ret = i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA);
    if (0 == ret) {
        ret = -EINVAL;
        OV_ERR("I2C DOESN'T support I2C_FUNC_SMBUS_BYTE_DATA\n");
        goto failed;
    }

    priv = (struct ov5640_priv *)kzalloc(sizeof(struct ov5640_priv), GFP_KERNEL);
    if (NULL == priv) {
        ret = -ENOMEM;
        OV_ERR("alloc ov5640_priv struct failed!\n");
        goto failed;
    }

    sd = &priv->subdev;
    strcpy(sd->name, OV5640_I2C_NAME);

    v4l2_i2c_subdev_init(sd, client, &ov5640_subdev_ops);

    icd->ops = &ov5640_ops;

#if 0
    ret = ov5640_video_probe(icd, client);
    if (ret) {
        icd->ops = NULL;
        kfree(priv);
        OV_ERR("error : failed to probe camera device");
    }
#endif

failed:
    //return (ret);
    return (0);
#ifndef CONFIG_TC4_EVT
err_regulator:
    ov5640_power(0);

    regulator_put(ov_vddaf_cam_regulator);
    regulator_put(ov_vdd5m_cam_regulator);

    /* add by cym 20130506 */
    regulator_put(ov_vdd18_cam_regulator);
    regulator_put(ov_vdd28_cam_regulator);
    /* end add */
    return ret;
#endif
}


static int ov5640_remove(struct i2c_client *client)
{
    struct ov5640_priv *priv = to_ov5640(client);
    struct soc_camera_device *icd = client->dev.platform_data;

    struct v4l2_subdev *sd = i2c_get_clientdata(client);

    v4l2_device_unregister_subdev(sd);

    //icd->ops = NULL;
    kfree(priv);
    INFO_RED("ov5640 device removed!\n");

#ifndef CONFIG_TC4_EVT
    ov5640_power(0);

    regulator_put(ov_vddaf_cam_regulator);
    regulator_put(ov_vdd5m_cam_regulator);

    regulator_put(ov_vdd18_cam_regulator);
    regulator_put(ov_vdd28_cam_regulator);

    down_af_firmware_flag = 0;
    /* end add */
#endif

    return (0);
}

#if 0
static int ov5640_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    strcpy(info->type, OV5640_I2C_NAME);
    return 0;
}
#endif

static const struct i2c_device_id ov5640_id[] = {
{ OV5640_I2C_NAME, 0 },
{ }
};
MODULE_DEVICE_TABLE(i2c, ov5640_id);


static const unsigned short ov5640_addrs[] = {
    OV5640_I2C_ADDR,
    I2C_CLIENT_END,
};

#if 0
struct i2c_board_info ov5640_info = {
    I2C_BOARD_INFO(OV5640_I2C_NAME, OV5640_I2C_ADDR),
};
#endif

static struct i2c_driver ov5640_i2c_driver = {
    .driver = {
        .name = OV5640_I2C_NAME,
        .owner = THIS_MODULE,
    },
    .class = I2C_CLASS_HWMON,
            .probe    = ov5640_probe,
            .remove   = ov5640_remove,
            .id_table = ov5640_id,
            .address_list = ov5640_addrs,
};


    /*
 * module function
 */

    static int __init ov5640_module_init(void)
    {
        OV_INFO("install module\n");
        printk("%s\n", __FUNCTION__);

        return i2c_add_driver(&ov5640_i2c_driver);
    }

    static void __exit ov5640_module_exit(void)
    {
        OV_INFO("uninstall module\n");
        i2c_del_driver(&ov5640_i2c_driver);
    }

    module_init(ov5640_module_init);
    module_exit(ov5640_module_exit);

    MODULE_DESCRIPTION("SoC Camera driver for ov5640");
    MODULE_AUTHOR("caoym@topeet.com ");
    MODULE_LICENSE("GPL v2");


