# CarOS Arduino Simulation

A car dashboard simulation using Arduino and SimulIDE. This project implements a digital dashboard that displays speed, gear, distance, time, and music information with physical controls for acceleration and music track selection.

## Overview

This project simulates a car's dashboard system using:

- Arduino UNO board
- ILI9341 TFT display
- Physical controls (buttons)
- LED indicator
- Buzzer for gear change notifications

## Requirements

- [SimulIDE](https://www.simulide.com/) - Circuit simulation software
- Arduino IDE
- GCC compiler for the host program
- Required Arduino Libraries:
  - Adafruit_GFX
  - Adafruit_ILI9341
- socat (for virtual serial ports)

## Hardware Setup

The simulation includes:

- ILI9341 TFT Display connected to pins:

  - CS: Pin 10
  - DC: Pin 8
  - RST: Pin 9
  - MOSI: Pin 11
  - SCK: Pin 13

- Control buttons:

  - Acceleration: Pin 2
  - Next Track: Pin 5
  - Previous Track: Pin 6
  - LED Indicator: Pin 4

## Features

- Real-time speed display (0-180 km/h)
- Automatic gear shifting (Neutral + 6 gears)
- Distance tracking (odometer)
- Current time display
- Music player controls
- Visual and audio feedback for gear changes
- Non-blocking LED notifications

## Usage

1. Create virtual serial ports:

```sh
socat -d -d PTY,link=/tmp/ttyV0,raw,echo=0 PTY,link=/tmp/ttyV1,raw,echo=0
```

- Note the two PTY endpoints created
- Update the serial port in `main.c` with one endpoint
- Configure SimulIDE's serial monitor with the other endpoint

2. Compile and run the host program:

```sh
gcc main.c -o carsim -pthread
./carsim
```

3. Open the simulation in SimulIDE:
   - Load the circuit file
   - Start the simulation
   - The dashboard should now be active and responding to inputs

## Building and Running

1. Load the Arduino sketch (`sketch_oct6b.ino`) to your Arduino board or simulator
2. Export the compiled sketch to firmware and load it in simulIDE
3. Compile the host program:

```sh
gcc main.c -o carsim -pthread
```

3. Run the simulation:

```sh
./carsim
```

## Display Layout

The TFT display is organized into several sections:

- Top: Current time
- Left tile: Speed and gear indicator
- Right tile: Distance (odometer)
- Bottom tile: Music information

## Communication Protocol

The host program communicates with the Arduino using the following commands:

- `TIME|HH:MM:SS` - Update time
- `SPEED|N` - Update speed value
- `DIST|N.N` - Update distance
- `GEAR|N` - Update gear (N for neutral, 1-6 for gears)
- `MUSIC|trackname` - Update music track
- `BEEP|1` - Trigger notification

## Input Controls

- Hold acceleration button to increase speed
- Release to naturally decelerate
- Press next/previous track buttons to change music
- Gear changes happen automatically based on speed
- Speed ranges for gears:
  - 0 km/h: Neutral
  - 1-19 km/h: 1st gear
  - 20-39 km/h: 2nd gear
  - 40-64 km/h: 3rd gear
  - 65-94 km/h: 4th gear
  - 95-129 km/h: 5th gear
  - 130+ km/h: 6th gear
