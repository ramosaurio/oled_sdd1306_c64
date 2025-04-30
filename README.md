# OLED SSD1306 for Commodore 64 Fonts

This project provides a C driver for OLED displays based on the SSD1306 controller, using fonts inspired by the classic Commodore 64. It's perfect for retro computing enthusiasts and microcontroller-based projects with a vintage aesthetic.

## Features

- Compatible with SSD1306 OLED displays (128x64 resolution)
- Renders text using Commodore 64-style fonts
- I2C communication interface
- Lightweight C code suitable for microcontrollers

## Requirements

- A microcontroller with I2C support (e.g., ESP32, STM32, AVR)
- An SSD1306 OLED display
- A C compiler (e.g., GCC)

## Project Structure

- `oled_SSD1306C64.c` and `oled_SSD1306.h`: Main driver implementation
- `fonts/`: Contains C64-style font data
- `Makefile`: Basic file for building the project
- `README.md`: This documentation

## Installation & Usage

1. Clone the repository:

   ```bash
   git clone https://github.com/ramosaurio/oled_sdd1306_c64.git
   ```

2. Add `oled_SSD1306C64.c` and `oled_SSD1306.h` to your project.
3. Ensure the `fonts/` directory is available in your project path.
4. Connect the OLED display to your microcontroller's I2C pins (SDA, SCL).
5. Compile and upload your firmware.

## Example

```c
#include "oled_SSD1306.h"

int main(void) {
    oled_init();
    oled_clear();
    oled_set_cursor(0, 0);
    oled_print("Hello, world!");
    oled_display();
    while (1) {
        // Main loop
    }
}
```

## Notes

- When using the `echo` function to print new lines, a `\n` character is added to the buffer. This behavior is not currently handled by the code.
- The project is under active development. Check back for updates and improvements.

## Contributing

Contributions are welcome! Feel free to fork the repo and submit pull requests with your improvements.

## License

This project is licensed under the MIT License.
