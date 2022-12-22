#include <sgtty.h>

gtty(fd, buf) struct sgtty *buf; { return (ioctl(fd, TIOCGETP, buf)); }
stty(fd, buf) struct sgtty *buf; { return (ioctl(fd, TIOCSETP, buf)); }
