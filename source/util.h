#ifndef __UTIL_H__
#define __UTIL_H__

#ifdef DEBUG_LOG
void userAppInit(void);
void userAppExit(void);
#endif

void debugPrintf(const char *text, ...);

int ret0(void);
int ret1(void);
int retm1(void);

#endif // __UTIL_H__