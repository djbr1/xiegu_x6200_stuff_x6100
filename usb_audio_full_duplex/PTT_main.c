#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

int main() {
    struct input_event ev;
    int fd = open("/dev/input/event0", O_RDONLY);

    if (fd < 0) { perror("open event0"); return 1; }

    while (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
        if (ev.type == EV_KEY) {
            if (ev.value == 1) {
                printf("PTT pressed\n");
            }
            else if (ev.value == 0) {
                printf("PTT released\n");
            }
        }
    }

    close(fd);
    return 0;
}
