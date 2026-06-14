import utime
import gc

from lcd_api import LcdApi

# PCF8574 pin definitions
MASK_RS = 0x01  # P0
MASK_RW = 0x02  # P1
MASK_E = 0x04  # P2

SHIFT_BACKLIGHT = 3  # P3
SHIFT_DATA = 4  # P4-P7


class I2cLcd(LcdApi):
    """
    I2cLcd provides an interface to control an HD44780 character LCD via a PCF8574 I2C I/O expander.

    This class implements the hardware abstraction layer (HAL) for the LcdApi base class, allowing
    communication with the LCD using I2C protocol. It supports basic LCD operations such as initialization,
    sending commands, writing data, and controlling the backlight.

    Args:
        i2c: An initialized I2C bus object.
        i2c_addr (int): The I2C address of the PCF8574 device.
        num_lines (int): Number of display lines on the LCD.
        num_columns (int): Number of columns per line on the LCD.

    Methods:
        hal_write_init_nibble(nibble):
            Sends an initialization nibble to the LCD during startup sequence.

        hal_backlight_on():
            Turns the LCD backlight on.

        hal_backlight_off():
            Turns the LCD backlight off.

        hal_write_command(cmd):
            Sends a command byte to the LCD.

        hal_write_data(data):
            Sends a data byte (character) to the LCD.

    Note:
        This class assumes the use of the PCF8574 I2C I/O expander and is intended for use with MicroPython.
        It requires the LcdApi base class and constants such as SHIFT_DATA, MASK_E, MASK_RS, SHIFT_BACKLIGHT,
        and LCD command codes to be defined elsewhere.
    """

    # Implements a HD44780 character LCD connected via PCF8574 on I2C

    def __init__(self, i2c, i2c_addr, num_lines, num_columns):
        """
        Initializes the I2C LCD display.

        Args:
            i2c: The I2C bus object used for communication.
            i2c_addr (int): The I2C address of the LCD display.
            num_lines (int): Number of display lines (rows) on the LCD.
            num_columns (int): Number of display columns on the LCD.

        This method initializes the LCD by sending the required reset and configuration
        commands over I2C, sets the display mode (4-bit), and configures the number of lines.
        It also ensures proper timing between commands as per the LCD datasheet.
        """
        self.i2c = i2c
        self.i2c_addr = i2c_addr
        self.i2c.writeto(self.i2c_addr, bytes([0]))
        utime.sleep_ms(20)  # Allow LCD time to powerup
        # Send reset 3 times
        self.hal_write_init_nibble(self.LCD_FUNCTION_RESET)
        utime.sleep_ms(5)  # Need to delay at least 4.1 msec
        self.hal_write_init_nibble(self.LCD_FUNCTION_RESET)
        utime.sleep_ms(1)
        self.hal_write_init_nibble(self.LCD_FUNCTION_RESET)
        utime.sleep_ms(1)
        # Put LCD into 4-bit mode
        self.hal_write_init_nibble(self.LCD_FUNCTION)
        utime.sleep_ms(1)
        LcdApi.__init__(self, num_lines, num_columns)
        cmd = self.LCD_FUNCTION
        if num_lines > 1:
            cmd |= self.LCD_FUNCTION_2LINES
        self.hal_write_command(cmd)
        gc.collect()

    def hal_write_init_nibble(self, nibble):
        """
        Sends an initialization nibble to the LCD via I2C.

        This method is specifically used during the LCD initialization sequence to
        send a 4-bit nibble. It shifts and masks the nibble as required by the LCD
        protocol, toggles the enable (E) line, and writes the data over I2C.

        Args:
            nibble (int): The 8-bit value containing the initialization nibble to send.
        """
        # Writes an initialization nibble to the LCD.
        # This particular function is only used during initialization.
        byte = ((nibble >> 4) & 0x0F) << SHIFT_DATA
        self.i2c.writeto(self.i2c_addr, bytes([byte | MASK_E]))
        self.i2c.writeto(self.i2c_addr, bytes([byte]))
        gc.collect()

    def hal_backlight_on(self):
        """
        Turns the LCD backlight on via the I2C interface.

        This method sends a command to the LCD to enable the backlight by writing
        the appropriate bit to the I2C device. It also triggers garbage collection
        after the operation to manage memory usage.

        Returns:
            None
        """
        # Allows the hal layer to turn the backlight on
        self.i2c.writeto(self.i2c_addr, bytes([1 << SHIFT_BACKLIGHT]))
        gc.collect()

    def hal_backlight_off(self):
        """
        Turns off the LCD backlight via the I2C interface.

        This method sends a command to the LCD to switch off its backlight by writing a zero byte
        to the device's I2C address. It also triggers garbage collection to free up memory.

        Returns:
            None
        """
        # Allows the hal layer to turn the backlight off
        self.i2c.writeto(self.i2c_addr, bytes([0]))
        gc.collect()

    def hal_write_command(self, cmd):
        """
        Sends a command byte to the LCD via I2C using the hardware abstraction layer.

        This method splits the command into two 4-bit nibbles and sends each nibble sequentially,
        latching the data on the falling edge of the enable (E) signal. It also manages the backlight state.
        For certain commands (home and clear), it introduces a delay to meet the LCD's timing requirements.

        Args:
            cmd (int): The command byte to send to the LCD.

        Side Effects:
            - Communicates with the LCD over I2C.
            - May introduce a delay for specific commands.
            - Triggers garbage collection after execution.
        """
        # Write a command to the LCD. Data is latched on the falling edge of E.
        byte = (self.backlight << SHIFT_BACKLIGHT) | (((cmd >> 4) & 0x0F) << SHIFT_DATA)
        self.i2c.writeto(self.i2c_addr, bytes([byte | MASK_E]))
        self.i2c.writeto(self.i2c_addr, bytes([byte]))
        byte = (self.backlight << SHIFT_BACKLIGHT) | ((cmd & 0x0F) << SHIFT_DATA)
        self.i2c.writeto(self.i2c_addr, bytes([byte | MASK_E]))
        self.i2c.writeto(self.i2c_addr, bytes([byte]))
        if cmd <= 3:
            # The home and clear commands require a worst case delay of 4.1 msec
            utime.sleep_ms(5)
        gc.collect()

    def hal_write_data(self, data):
        """
        Writes a byte of data to the LCD using the I2C interface.

        This method sends the data in two 4-bit nibbles, as required by the LCD in 4-bit mode.
        The data is latched on the falling edge of the Enable (E) signal.
        The method also manages the backlight state and triggers garbage collection after writing.

        Args:
            data (int): The byte of data to write to the LCD.
        """
        # Write data to the LCD. Data is latched on the falling edge of E.
        byte = (
            MASK_RS
            | (self.backlight << SHIFT_BACKLIGHT)
            | (((data >> 4) & 0x0F) << SHIFT_DATA)
        )
        self.i2c.writeto(self.i2c_addr, bytes([byte | MASK_E]))
        self.i2c.writeto(self.i2c_addr, bytes([byte]))
        byte = (
            MASK_RS
            | (self.backlight << SHIFT_BACKLIGHT)
            | ((data & 0x0F) << SHIFT_DATA)
        )
        self.i2c.writeto(self.i2c_addr, bytes([byte | MASK_E]))
        self.i2c.writeto(self.i2c_addr, bytes([byte]))
        gc.collect()
