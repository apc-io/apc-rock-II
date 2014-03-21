/**
 * @filename  uG31xx_Platform.h
 *
 *  Define the platform for uG31xx driver
 *
 * @author  AllenTeng <allen_teng@upi-semi.com>
 */

#ifndef	_UG31XX_PLATFORM_H_
#define	_UG31XX_PLATFORM_H_

//#define uG31xx_OS_WINDOWS
#define uG31xx_OS_ANDROID

#ifdef  uG31xx_OS_ANDROID

  #define uG31xx_BOOT_LOADER
  
#endif  ///< end of uG31xx_OS_ANDROID

#endif	///< end of _UG31XX_PLATFORM_H_
