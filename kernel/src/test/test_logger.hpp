#include <stdio.h>

#define __LOGGER__

#define logInfo(msg...)			printf("%s", msg); printf("\n")
#define logInfon(msg...)		printf("%s", msg);

#define logWarn(msg...)			printf("%s", msg); printf("\n");
#define logWarnn(msg...)		printf("%s", msg);

#define logDebug(msg...)		printf("%s", msg); printf("\n");
#define logDebugn(msg...)		printf("%s", msg);
