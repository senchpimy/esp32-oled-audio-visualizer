
# esp32-oled-audio-visualizer

Este proyecto implementa un visualizador de espectro de audio en una pantalla OLED de 128x32 usando el micrófono del esp32 M5 Atom

## Características

* Visualización en tiempo real del espectro de audio (FFT) en 16 bandas.
* Animación de forma de onda de audio.

---

## Hardware

* ESP32  M5Stack Atom
* Pantalla OLED SSD1306 (128x32, I2C)

---

## Conexiones

| Componente | Pin ESP32 |
| ---------- | --------- |
| SDA (OLED) | GPIO 25   |
| SCL (OLED) | GPIO 21   |

---



## Cómo funciona

* Se inicializa el micrófono I2S para capturar muestras de audio.
* Se realiza una FFT sobre 512 muestras y se renderizan las magnitudes en 16 barras.
* También se muestra la forma de onda en la parte inferior del visualizador.
* Un conjunto de frases en japonés se elige aleatoriamente y se desplaza continuamente por el área izquierda de la pantalla.

---

## Vista previa

![Demo](./video.gif)

---
