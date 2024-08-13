# TimeSync 

A basic watch with hopes to be a more advanced smart watch with time.

### System Design
To see how the code is designed and the structure behind this TimeSync System, please see my FigJam file that shows a visual representation of the code.
[FigJam](https://www.figma.com/file/E1JAc8ijfQZMej6hGPKK3q/TimeSync---System-Design?type=whiteboard&node-id=0%3A1&t=UBQqavuqREo1Pac7-1)

### Goals

- Use the ESP-IDF framework (Instead of Ardunio)
- Create a watch that pairs to the phone, allowing for various syncing (time, notifications, etc.)
- Use the FreeRTOS operating system for easier handling and more fine tuning control

### How to Run

- Current hardware I am using is the ESP-32-WROVER
- I am using PlatformIO to upload the firemare to the device


xtensa-esp32-elf-addr2line -pfiaC -e C:\Users\nateg\OneDrive\Documents\GitHub\TimeSync\.pio\build\esp32s3box\firmware.elf 