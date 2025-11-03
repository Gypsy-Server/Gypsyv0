<<<<<<< HEAD
**Gypsy USB stick basic local fileview**
=======
# Gypsyv0   
*A local file viewer built on the ESP8266*

![Board Preview](images/usb-stick.png)
>>>>>>> 0007602 (removed old files)



**Gypsyv0** is a compact experiment in turning the humble **ESP8266** into a lightweight **local NAS-style file viewer**.  
It’s designed to serve files directly from an **SD card** or internal memory over Wi-Fi — no cloud, no dependencies, just you and your network.

The goal is simplicity: a small, self-contained board that can browse, read, and manage files locally through a web interface.

---

## What is here

- **Local file browsing** over Wi-Fi  
- **SD card (FAT32/MBR)** storage support   
- Runs standalone — powered via USB or external 5 V  or 3.3V
  
- Hardware layout inspired by the [WebStick project](https://github.com/tobychui/WebStick)

---

## Hardware

- **MCU:** ESP8266(12E)
- **Storage:** SD card(SPI MODE)  
- **Voltage Regulator AMS117 3.3V**
- **Ch340C USB TO TTL Converter chip**
- **Passive Components:** Resistor ,Capacitor


You can view the full schematic and PCB in the `usb_stick_kicadfiles/` folder (KiCad project).


---

##  What to do 

1. Flash gypsy.ino using Arduino IDE or PlatformIO.  
2. Open Serial Monitor to get the IP (eg 192.168.x.x)
3. Open in any browser on Computer or any device connected to that Network

---
## What you will see
![Home Page](gyp_red.png)
 
![File Handling](images/category_view.png)
 
 
![Anime girl](images/anime_girl.png)
 
 
![Full screnn Mp3](fullscreen.png)




##  What I’m Exploring Next

- Network share (maybe SMB or FTP?)  
- ESP32 port for faster WIFI throughput  
- Power optimization and case design  

---

##  Feedback Wanted

This is still a **work in progress**, and I’d love your thoughts on:

- Firmware structure and web interface flow  
- PCB layout or power-handling tips  
- Ideas for extending it into a full mini NAS

If you’ve built anything similar — or see a way to make this smarter, faster, or cleaner — I’m all ears.

---


![Prototype](images/prototype.jpg)
