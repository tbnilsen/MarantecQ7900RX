# MarantecQ7900RX
Photon Particle project for receiving garage door RF remote key codes

I wanted to make my garage doors controllable via the internet using a Particle Photon. In order to obtain the RF codes that my garage door remote openers send I wrote this code to decode them using a cheap 315MHz RF receiver I bought on Amazon.

I connected the RF receiver's output to D1 of the Photon. Whenever the garage door opener is pressed this code captures the 24bit code and prints it to serial monitor screen.

Next, use this printed hex code (24bits) in my program MarantecQ7900TX.
