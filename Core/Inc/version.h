#ifndef __VERSION_H__
#define __VERSION_H__

// X.x.x: Major version of the stack
#define VERSION_MAJOR 0
// x.X.x: Minor version of the stack
#define VERSION_MINOR 4
// x.x.X: Build of the stack
#define VERSION_BUILD 225

// Provides the version
#define FW_VERSION ((VERSION_MAJOR) << 24 | \
                       (VERSION_MINOR) << 16 | \
                       (VERSION_BUILD) << 8)

#define BUILD_DATE "03/02/2026, 20:24:37"

#endif /* __VERSION_H__ */