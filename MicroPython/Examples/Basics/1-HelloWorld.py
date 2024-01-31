"""ILI9341 demo (fonts)."""
from time import sleep
from ili9341_cyd import Display, color565
from machine import Pin, SPI
from xglcd_font import XglcdFont

#RGB LED pin defines
red_led = Pin(4, Pin.OUT)
green_led = Pin(16, Pin.OUT)
blue_led = Pin(17, Pin.OUT)
#Turn them off by setting the (active low) pins high
red_led.on()
green_led.on()
blue_led.on()

def printText():
    """Prints Hello World in two different fonts"""
    # Baud rate of 40000000 seems about the max
    # bl = backlight pin (CYD backlight is connected to pin 21)
    spi = SPI(1, baudrate=40000000, sck=Pin(14), mosi=Pin(13))
    display = Display(spi, dc=Pin(2), cs=Pin(15), bl=Pin(21), rotation=180)
    
    #draw some text with the MicroPython built in 8x8 font
    display.draw_text8x8(20, 80, 'Hello World!', color565(255, 0, 255))
        
    #draw some Robotron style text
    robotron = XglcdFont('fonts/Robotron13x21.c', 13, 21)
    display.draw_text(20, 155, 'HELLO WORLD!', robotron, color565(255, 255, 255))

    sleep(10)
    display.clear()
    display.cleanup()
    display.backlight_off()

printText()

