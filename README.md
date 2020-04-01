# Mechatronics-Final
Final Project for MEMS 1049 - Automatic RFID-based Dog Door

This code (main.c) was written in Atmel Studio 7 for the ATMega328P microcontroller.
The project was done in a group of three, however the code was primarily my responsibility,
with my groupmates working primarily on the physical and electrical parts of the project.

To run, create a new project in Atmel Studio for the ATMega328P and replace the default main.c with this one.
May require a simulator if hardware is not available.

# Project Overview
The idea behind this project was an "automatic dog door" that can be attached to any door without needing to cut a hole in it.
This required communicating with multiple motors, a temperature sensor, and 
an RFID sensor. The door only opens from the inside when a dog wearing the correct RFID tag collar comes within range and the
outside temperature is within a safe range. The door will open from the outside regardless of the temperature, in order to not 
lock the dog outside in unsafe weather. The microcontroller is also able to detect when the door is locked, and will not try to open
the door in that case.
