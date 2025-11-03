#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <termios.h>
#include <errno.h>

// State for the simulation
typedef enum { IDLE, ACCELERATING } ButtonState;
ButtonState current_button_state = IDLE;

double speed = 0.0;
double distance = 0.0;
float odometer = 12345.6;
int gear = 0; // Start in Neutral

// Function Prototypes
void send_to_arduino(int fd, const char* msg);
void beep_for_gear_change(int fd);
void display_music(int fd, int direction);
void display_gear(int fd);

// Configures the serial port with the correct settings (baud rate, etc.)
int configure_serial(int fd) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) { return -1; }
    cfsetospeed(&tty, B9600); cfsetispeed(&tty, B9600);
    tty.c_cflag &= ~PARENB; tty.c_cflag &= ~CSTOPB; tty.c_cflag &= ~CSIZE; tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS; tty.c_cflag |= CREAD | CLOCAL;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~(OPOST | ONLCR);
    tty.c_cc[VTIME] = 0; tty.c_cc[VMIN] = 0;
    if (tcsetattr(fd, TCSANOW, &tty) != 0) { return -1; }
    return 0;
}

// Sends a message to the Arduino.
void send_to_arduino(int fd, const char* msg) {
    write(fd, msg, strlen(msg));
    write(fd, "\n", 1);
    tcdrain(fd);
}

// Formats the current system time and sends it to the Arduino.
void display_time(int fd) {
    time_t now = time(NULL);
    char buf[64];
    strftime(buf, sizeof(buf), "TIME|%H:%M:%S", localtime(&now));
    send_to_arduino(fd, buf);
}

// Changes the music track based on direction and sends the update.
void display_music(int fd, int direction) {
    static int track_index = 0;
    const char* tracks[] = {"Solar Sailer", "Adagio - Tron", "Armory", "The Grid", "Recognizer"};
    const int num_tracks = sizeof(tracks) / sizeof(tracks[0]);
    track_index += direction;
    if (track_index >= num_tracks) track_index = 0;
    else if (track_index < 0) track_index = num_tracks - 1;
    char buf[64];
    sprintf(buf, "MUSIC|%s", tracks[track_index]);
    send_to_arduino(fd, buf);
}

// **CODE CHANGED HERE: Sends speed only when the value changes.**
// Formats the current speed and sends it to the Arduino.
void display_speed(int fd) {
    static int last_sent_speed = -1; // Remember the last speed we sent
    int current_speed = (int)speed;

    // Only send an update if the speed has changed
    if (current_speed != last_sent_speed) {
        char buf[64];
        sprintf(buf, "SPEED|%d", current_speed);
        send_to_arduino(fd, buf);
        last_sent_speed = current_speed; // Update the last sent speed
    }
}

// Formats the total distance and sends it to the Arduino.
void display_distance(int fd) {
    char buf[64];
    sprintf(buf, "DIST|%.1f", odometer + distance);
    send_to_arduino(fd, buf);
}

// Formats the current gear and sends it to the Arduino.
void display_gear(int fd) {
    char buf[64];
    if (gear == 0) {
        sprintf(buf, "GEAR|N"); // Send "N" for Neutral
    } else {
        sprintf(buf, "GEAR|%d", gear);
    }
    send_to_arduino(fd, buf);
}

// Processes all inputs, including new music controls.
void handle_input(int fd) {
    static char serial_buffer[512];
    static int buffer_len = 0;
    int n = read(fd, serial_buffer + buffer_len, sizeof(serial_buffer) - buffer_len - 1);
    if (n > 0) {
        buffer_len += n;
        serial_buffer[buffer_len] = '\0';
    }
    char* newline_ptr;
    while ((newline_ptr = strchr(serial_buffer, '\n')) != NULL) {
        *newline_ptr = '\0';
        if (strstr(serial_buffer, "ACCEL")) current_button_state = ACCELERATING;
        else if (strstr(serial_buffer, "IDLE")) current_button_state = IDLE;
        else if (strstr(serial_buffer, "NEXT")) display_music(fd, 1);
        else if (strstr(serial_buffer, "PREV")) display_music(fd, -1);
        buffer_len -= (newline_ptr - serial_buffer + 1);
        memmove(serial_buffer, newline_ptr + 1, buffer_len);
        serial_buffer[buffer_len] = '\0';
    }
}

// Calculates the car's physics (speed, distance) and handles gear changes.
void update_physics_and_gear(int fd, double delta_time) {
    const double ACCELERATION = 25.0;
    const double NATURAL_DECELERATION = 15.0;

    if (current_button_state == ACCELERATING) speed += ACCELERATION * delta_time;
    else speed -= NATURAL_DECELERATION * delta_time;

    if (speed < 0) speed = 0;
    if (speed > 180) speed = 180;
    distance += (speed / 3600.0) * delta_time;
    
    int old_gear = gear;
    // New gear logic with Neutral state
    if (speed == 0) gear = 0; // Neutral gear
    else if (speed < 20) gear = 1;
    else if (speed < 40) gear = 2;
    else if (speed < 65) gear = 3;
    else if (speed < 95) gear = 4;
    else if (speed < 130) gear = 5;
    else gear = 6;
    
    if (gear != old_gear) {
        beep_for_gear_change(fd);
        display_gear(fd);
    }
}

// Sends a command to flash the LED.
void beep_for_gear_change(int fd) {
    send_to_arduino(fd, "BEEP|1");
}

// The main scheduler thread that controls when to send data to the Arduino.
void* scheduler(void* arg) {
    int fd = *(int*)arg;
    struct timespec last_time, current_time;
    clock_gettime(CLOCK_MONOTONIC, &last_time);

    double q1_timer = 0, q2_timer = 0, q3_timer = 0;
    const double UPDATE_INTERVAL_Q1 = 0.05;

    sleep(2);
    display_gear(fd); // Display initial gear state (Neutral)

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        double delta_time = (current_time.tv_sec - last_time.tv_sec) +
                             (current_time.tv_nsec - last_time.tv_nsec) / 1e9;
        last_time = current_time;

        handle_input(fd);
        update_physics_and_gear(fd, delta_time);

        q1_timer += delta_time;
        if (q1_timer >= UPDATE_INTERVAL_Q1) {
            display_speed(fd);
            q1_timer = 0;
        }

        q2_timer += delta_time;
        if (q2_timer >= 1.0) {
            display_time(fd);
            q2_timer = 0;
        }

        q3_timer += delta_time;
        if (q3_timer >= 3.0) {
            display_distance(fd);
            q3_timer = 0;
        }

        usleep(16000);
    }
    return NULL;
}

// The main entry point of the program.
int main() {
    const char* port = "/dev/pts/3";
    printf("Opening serial port: %s\n", port);
    int fd = open(port, O_RDWR | O_NOCTTY);
    if (fd < 0) { perror("Failed to open serial port"); return 1; }
    if (configure_serial(fd) < 0) { close(fd); return 1; }
    printf("Serial port configured. Starting simulation...\n");
    tcflush(fd, TCIOFLUSH);
    pthread_t tid;
    pthread_create(&tid, NULL, scheduler, &fd);
    pthread_join(tid, NULL);
    close(fd);
    return 0;
}