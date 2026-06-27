# Application_file

# Application Flow

The application is located in Flash memory starting at **0x00500000**, with its vector table placed at **0x00501000**. Control is transferred to the application by the bootloader after it validates the application image, updates the Vector Table Offset Register (VTOR), sets the Main Stack Pointer (MSP), and jumps to the application's reset handler.

As soon as the application starts executing, it turns the LED ON for approximately two seconds. This serves as a visual indication that the bootloader has successfully transferred execution to the application.

After the startup indication, the application initializes the system clock, configures the GPIO pins, initializes the interrupt controller, and configures the PIT (Periodic Interrupt Timer). Unlike the bootloader, which uses a 1-second timer period, the application configures the PIT with a 500 ms period. This difference in timing allows the user to easily distinguish whether the bootloader or the application is currently running.

Once the hardware initialization is complete, the application enables global interrupts and enters its main execution loop. Every time the PIT interrupt occurs, the interrupt service routine sets a software flag. The main loop continuously monitors this flag and toggles the LED whenever the flag is set, producing a fast blinking pattern.

The application remains in this loop indefinitely, continuously blinking the LED every 500 ms while providing space for additional application-specific functionality to be implemented inside the main loop.

## Visual Indication

* **Bootloader Running:** LED blinks every **1 second**.
* **Application Started:** LED remains **ON for approximately 2 seconds**.
* **Application Running:** LED blinks every **500 ms**.

This blinking pattern provides a simple visual confirmation that the bootloader successfully transferred control to the application and that the application is executing correctly.
